#pragma once
#ifndef CE_API_GLOBAL_CONTAINER_HASHTABLE_H
#define CE_API_GLOBAL_CONTAINER_HASHTABLE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Memory.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Helper/Types.h"

//Implementation of Dense Hash Table and Sparse Hash Table based on google's open source:
//	https://github.com/sparsehash/sparsehash/blob/master/src/sparsehash/internal/densehashtable.h
//	https://github.com/sparsehash/sparsehash/blob/master/src/sparsehash/internal/sparsehashtable.h
//
// These hash tables allow fast cache-efficient hashing. The dense hash table focuses on memory locality, 
// while the sparse hash table focuses on low memory usage.

#ifdef USE_NEW_CONTAINER

#ifdef min
#	pragma push_macro("min")
#	undef min
#	define POP_MACRO_MIN
#endif
#ifdef max
#	pragma push_macro("max")
#	undef max
#	define POP_MACRO_MAX
#endif

#ifndef CE_HT_PROBE_FUNC
#define CE_HT_PROBE_FUNC_D
//Linear probing:
//#define CE_HT_PROBE_FUNC(size, firstProbe, lastProbe, numProbes) ( ( lastProbe ) + 1 )

//Linear probing with swaping sign:
//#define CE_HT_PROBE_FUNC(size, firstProbe, lastProbe, numProbes) ( ( firstProbe ) + ( ( numProbes ) & 0x1 == 0 ? ( ( size ) + 1 - ( numProbes ) ) : ( numProbes ) ) )

//Quadratic probing:
#define CE_HT_PROBE_FUNC(size, firstProbe, lastProbe, numProbes) ( ( lastProbe ) + ( numProbes ) )

//Quadratic probing with swaping sign:
//#define CE_HT_PROBE_FUNC(size, firstProbe, lastProbe, numProbes) ( ( firstProbe ) + ( ( numProbes ) & 0x1 == 0 ? ( ( size ) + 1 - ( ( ( numProbes ) * ( numProbes ) ) & ( size ) ) ) : ( ( numProbes ) * ( numProbes ) ) ) )
#endif

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		//	V = Value Type
		//	K = Key/Hased Type
		//	H = Hash Functor (requires operator() that takes a value of type K and returns a size_t)
		//	ExtractKey = Functor that takes a value of type V and returns a value of type K
		//	SetKey = Functor that takes values of type V* and K and modifies the V* value in such a way, that ExtractKey(value) == key
		//	EqualityKey = Functor that checks whether two values of type K are the same (corespond to the same values)
		//	Alloc = STD-type allocator
		template<typename V, typename K, typename H, typename ExtractKey, typename SetKey, typename EqualityKey, typename Alloc> class HashTable {
			protected:
				typedef typename Alloc::template rebind<V>::other av_type;
			public:
				typedef K key_type;
				typedef V value_type;
				typedef Alloc allocator_type;

				typedef typename av_type::size_type size_type;
				typedef typename av_type::difference_type difference_type;
				typedef typename av_type::reference reference;
				typedef typename av_type::const_reference const_reference;
				typedef typename av_type::pointer pointer;
				typedef typename av_type::const_pointer const_pointer;

				CLOAK_CALL HashTable() = delete;
				CLOAK_CALL HashTable(In const HashTable&) = delete;
				CLOAK_CALL HashTable(In HashTable&&) = delete;
			protected:
				template<size_t MIN_BUCKETS> class Hash : public H {
					private:
						size_type m_enlargeThreshold;
						size_type m_shrinkThreshold;
						float m_enlargeFactor;
						float m_shrinkFactor;
						bool m_mayShrink;
						bool m_empty;
						bool m_deleted;
						size_t m_numCopies;

						template<typename KT> struct Mungler {
							public:
								static size_type CLOAK_CALL Mungle(In size_type hash) { return hash; }
						};
						template<typename KT> struct Mungler<KT*> {
							private:
								static constexpr size_type SHIFT = API::Global::Math::CompileTime::bitScanReverse(sizeof(void*));
							public:
								static size_type CLOAK_CALL Mungle(In size_type hash) { return (hash << ((sizeof(size_type) << 3) - SHIFT)) | (hash >> SHIFT); }
						};
					public:
						CLOAK_CALL Hash(In const H& hash, In const float minOcc, In const float maxOcc) : H(hash), m_enlargeThreshold(0), m_shrinkThreshold(0), m_mayShrink(false), m_empty(false), m_deleted(false), m_numCopies(0)
						{
							SetEnlargeFactor(maxOcc);
							SetShrinkFactor(minOcc);
						}

						void CLOAK_CALL_THIS SetEnlargeFactor(In float f) { m_enlargeFactor = f; }
						void CLOAK_CALL_THIS SetShrinkFactor(In float f) { m_shrinkFactor = f; }
						void CLOAK_CALL_THIS SetEnlargeThreshold(In size_type t) { m_enlargeThreshold = t; }
						void CLOAK_CALL_THIS SetShrinkThreshold(In size_type t) { m_shrinkThreshold = t; }
						void CLOAK_CALL_THIS SetConsiderShrink(In bool t) { m_mayShrink = t; }
						void CLOAK_CALL_THIS SetUseEmpty(In bool t) { m_empty = t; }
						void CLOAK_CALL_THIS SetUseDeleted(In bool t) { m_deleted = t; }

						float CLOAK_CALL_THIS GetEnlargeFactor() const { return m_enlargeFactor; }
						float CLOAK_CALL_THIS GetShrinkFactor() const { return m_shrinkFactor; }
						size_type CLOAK_CALL_THIS GetEnlargeThreshold() const { return m_enlargeThreshold; }
						size_type CLOAK_CALL_THIS GetShrinkThreshold() const { return m_shrinkThreshold; }
						size_type CLOAK_CALL_THIS GetEnlargeSize(In size_type x) const { return static_cast<size_type>(x * m_enlargeFactor); }
						size_type CLOAK_CALL_THIS GetShrinkSize(In size_type x) const { return static_cast<size_type>(x * m_shrinkFactor); }
						size_type CLOAK_CALL_THIS GetNumberOfCopies() const { return m_numCopies; }
						bool CLOAK_CALL_THIS ConsiderShrink() const { return m_mayShrink; }
						bool CLOAK_CALL_THIS UseEmpty() const { return m_empty; }
						bool CLOAK_CALL_THIS UseDeleted() const { return m_deleted; }

						size_type hash(In const key_type& v) const { return Mungler<K>::Mungle(H::operator()(v)); }
						void CLOAK_CALL_THIS IncreaseCopyCounter() { m_numCopies++; }
						void CLOAK_CALL_THIS Reset(In size_type bucketCount)
						{
							SetEnlargeThreshold(GetEnlargeSize(bucketCount));
							SetShrinkThreshold(GetShrinkSize(bucketCount));
							SetConsiderShrink(false);
						}
						void CLOAK_CALL_THIS Reset(In float shrink, In float grow, In size_type bucketCount)
						{
							CLOAK_ASSUME(shrink >= 0);
							CLOAK_ASSUME(grow <= 1);
							CLOAK_ASSUME(shrink <= grow);
							const float gh = 0.5f * grow;
							if (shrink > gh) { shrink = gh; }
							SetEnlargeFactor(grow);
							SetShrinkFactor(shrink);
							Reset(bucketCount);
						}
						size_type CLOAK_CALL_THIS GetMinBuckets(In_opt size_type minEnlargedSize = 0, In_opt size_type minBucketsWanted = 0) const
						{
							float enlarge = GetEnlargeFactor();
							size_type s = MIN_BUCKETS;
							while (s < minBucketsWanted || minEnlargedSize >= static_cast<size_type>(s * elnarge))
							{
								size_type ns = s << 1;
								if (ns < s) { throw std::length_error("resize overflow"); }
								s = ns;
							}
							return s;
						}
				};
		};

		template<typename V, typename K, class H, class ExtractKey, class SetKey, class EqualityKey, class Alloc> class DenseHashTable : public HashTable<V, K, H, ExtractKey, SetKey, EqualityKey, Alloc> {
			public:
				// Max percentage of occupancy before we resize. 0.8f would be ok without too much probing, but
				// we choose 0.5f as our default value for more speed at the cost of memory usage. This can also be
				// changed with SetResizeParameters
				static constexpr float MAX_OCCUPANCY = 0.5f;
				// Min percentage of occupancy before we automaticly shrink down. Should be lower then MAX_OCCUPANCY / 2.
				// Set this to zero to disable automatic shrink down. This value can also be changed with SetResizeParameters
				static constexpr float MIN_OCCUPANCY = 0.4f * MAX_OCCUPANCY;
				// Smallest number of buckets this hash table can contain. Must allways be a power of two, and must be at least 4.
				static constexpr size_t MIN_BUCKET_COUNT = 4;
				// Default container size if there is no explicit bucket count is given. Must allways be a power of two, and 
				// must be at least MIN_BUCKET_COUNT
				static constexpr size_t DEFAULT_BUCKET_COUNT = 32;

				static_assert(MAX_OCCUPANCY >= 0, "MAX_OCCUPANCY is too small");
				static_assert(MAX_OCCUPANCY <= 1, "MAX_OCCUPANCY is too high");
				static_assert(MIN_OCCUPANCY >= 0, "MIN_OCCUPANCY is too small");
				static_assert(MIN_OCCUPANCY < MAX_OCCUPANCY, "MIN_OCCUPANCY is too high");
				static_assert(MIN_BUCKET_COUNT >= 4, "MIN_BUCKET_COUNT is too small");
				static_assert(API::Global::Math::CompileTime::popcount(MIN_BUCKET_COUNT) == 1, "MIN_BUCKET_COUNT must be a power of two");
				static_assert(DEFAULT_BUCKET_COUNT >= MIN_BUCKET_COUNT, "DEFAULT_BUCKET_COUNT is too small");
				static_assert(API::Global::Math::CompileTime::popcount(DEFAULT_BUCKET_COUNT) == 1, "DEFAULT_BUCKET_COUNT must be a power of two");

				class iterator;
				class const_iterator;

				class iterator {
					friend class DenseHashTable;
					friend class const_iterator;
					public:
						typedef std::forward_iterator_tag iterator_category;
						typedef typename DenseHashTable::difference_type difference_type;
						typedef typename DenseHashTable::size_type size_type;
						typedef typename DenseHashTable::value_type value_type;
						typedef typename DenseHashTable::reference reference;
						typedef typename DenseHashTable::pointer pointer;
					protected:
						const DenseHashTable* m_parent;
						pointer m_start;
						pointer m_end;

						CLOAK_CALL iterator(In const DenseHashTable* h, In pointer start, In pointer end, In bool advance) : m_parent(h), m_start(start), m_end(end)
						{
							if (advance == true) { AdvancePastED(); }
						}
						void CLOAK_CALL_THIS AdvancePastED()
						{
							while (m_start != end && (m_parent->IsEmpty(*this) || m_parent->IsDeleted(*this))) { ++m_start; }
						}
					public:
						CLOAK_CALL iterator() : m_parent(nullptr), m_start(nullptr), m_end(nullptr) {}

						reference CLOAK_CALL_THIS operator*() const { return *m_start; }
						pointer operator->() const { return m_start; }
						iterator& CLOAK_CALL_THIS operator++()
						{
							CLOAK_ASSUME(m_start != m_end);
							++m_start;
							AdvancePastED();
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator++(int)
						{
							iterator tmp(*this);
							++*this;
							return tmp;
						}
						bool CLOAK_CALL_THIS operator==(In const iterator& it) { return m_start == it.m_start; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& it) { return m_start != it.m_start; }
				};
				class const_iterator {
					friend class DenseHashTable;
					public:
						typedef std::forward_iterator_tag iterator_category;
						typedef typename DenseHashTable::difference_type difference_type;
						typedef typename DenseHashTable::size_type size_type;
						typedef typename DenseHashTable::value_type value_type;
						typedef typename DenseHashTable::const_reference reference;
						typedef typename DenseHashTable::const_pointer pointer;
					private:
						const DenseHashTable* m_parent;
						pointer m_start;
						pointer m_end;
					protected:
						CLOAK_CALL const_iterator(In const DenseHashTable* h, In pointer start, In pointer end, In bool advance) : m_parent(h), m_start(start), m_end(end)
						{
							if (advance == true) { AdvancePastED(); }
						}
						void CLOAK_CALL_THIS AdvancePastED()
						{
							while (m_start != end && (m_parent->IsEmpty(*this) || m_parent->IsDeleted(*this))) { ++m_start; }
						}
					public:
						CLOAK_CALL const_iterator() : m_parent(nullptr), m_start(nullptr), m_end(nullptr) {}
						CLOAK_CALL const_iterator(In const iterator& o) : m_parent(o.m_parent), m_start(o.m_start), m_end(o.m_end) {}

						reference CLOAK_CALL_THIS operator*() const { return *m_start; }
						pointer operator->() const { return m_start; }
						const_iterator& CLOAK_CALL_THIS operator++()
						{
							CLOAK_ASSUME(m_start != m_end);
							++m_start;
							AdvancePastED();
							return *this;
						}
						const_iterator& CLOAK_CALL_THIS operator++(int)
						{
							const_iterator tmp(*this);
							++*this;
							return tmp;
						}
						bool CLOAK_CALL_THIS operator==(In const const_iterator& it) { return m_start == it.m_start; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& it) { return m_start != it.m_start; }
				};
			private:
				static constexpr size_type ILLEGAL_BUCKET = static_cast<size_type>(-1);

				template<typename A> struct alloc_impl : public A {
					pointer CLOAK_CALL reallocateSave(In pointer ptr, In size_t oldSize, In size_t newSize)
					{
						A::deallocate(ptr, oldSize);
						return A::allocate(newSize);
					}
				};
				template<typename A, typename M> struct alloc_impl<API::Global::Memory::Allocator<A, M>> : public API::Global::Memory::Allocator<A, M> {
					pointer CLOAK_CALL reallocateSave(In pointer ptr, In size_t oldSize, In size_t newSize) { return API::Global::Memory::Allocator<A, M>::reallocate(ptr, oldSize, newSize); }
				};

				struct HashSettings : public Hash<MIN_BUCKET_COUNT> {
					explicit CLOAK_CALL HashSettings(In const H& hash) : Hash(hash, MIN_OCCUPANCY, MAX_OCCUPANCY) {}
				};
				struct KeyInfo : public ExtractKey, public SetKey, public EqualityKey {
					CLOAK_CALL KeyInfo(In const ExtractKey& ek, In const SetKey& sk, In const EqualityKey& eq) : ExtractKey(ek), SetKey(sk), EqualityKey(eq) {}

					typename ExtractKey::result_type CLOAK_CALL_THIS GetKey(In const_reference v) const { return ExtractKey::operator()(v); }
					void CLOAK_CALL_THIS SetKey(In pointer v, In const key_type& k) const { SetKey::operator()(v, k); }
					bool CLOAK_CALL_THIS Equals(In const key_type& a, In const key_type& b) { return EqualityKey::operator()(a, b); }

					typename std::remove_const<key_type>::type Delete;
					typename std::remove_const<key_type>::type Empty;
				};
				struct ValInfo : public alloc_impl<av_type> {
					typedef alloc_impl<av_type> parent;
					typedef typename parent::value_type value_type;

					value_type Empty

					CLOAK_CALL ValInfo(In const parent& a) : parent(a), Empty() {}
					CLOAK_CALL ValInfo(In const ValInfo& v) : parent(v), Empty(v.Empty) {}
				};

				HashSettings m_hash;
				KeyInfo m_kinfo;
				ValInfo m_vinfo;

				size_type m_deleted;
				size_type m_size;
				size_type m_capacity;
				pointer m_table;
			
				void CLOAK_CALL_THIS SetValue(In pointer dst, In const_reference src)
				{
					dst->~value_type();
					new(dst)value_type(src);
				}
				void CLOAK_CALL_THIS Destroy(In size_type first, In size_type last)
				{
					CLOAK_ASSUME(first <= last);
					for (; first != last; ++first) { m_table[first].~value_type(); }
				}
				void CLOAK_CALL_THIS SquashDeleted()
				{
					if (m_deleted > 0)
					{
						DenseHashTable tmp(*this);
						swap(tmp);
					}
					CLOAK_ASSUME(m_deleted == 0);
				}
				bool CLOAK_CALL_THIS IsDeletedKey(In const key_type& key) const
				{
					CLOAK_ASSUME(m_hash.UseDeleted());
					return m_kinfo.Equals(m_kinfo.Delete, key);
				}
				bool CLOAK_CALL_THIS IsDeleted(In size_type id) const
				{
					CLOAK_ASSUME(m_hash.UseDeleted());
					return m_deleted > 0 && IsDeletedKey(m_kinfo.GetKey(m_table[id]));
				}
				bool CLOAK_CALL_THIS IsDeleted(In const iterator& id) const
				{
					CLOAK_ASSUME(m_hash.UseDeleted());
					return m_kinfo.Equals(m_kinfo.Delete, m_kinfo.GetKey(*id));
				}
				bool CLOAK_CALL_THIS IsDeleted(In const const_iterator& id) const
				{
					CLOAK_ASSUME(m_hash.UseDeleted());
					return m_kinfo.Equals(m_kinfo.Delete, m_kinfo.GetKey(*id));
				}
				bool CLOAK_CALL_THIS IsEmptyKey(In const key_type& key) const
				{
					CLOAK_ASSUME(m_hash.UseEmpty());
					return m_kinfo.Equals(m_kinfo.Empty, key);
				}
				bool CLOAK_CALL_THIS IsEmpty(In size_type id) const
				{
					CLOAK_ASSUME(m_hash.UseEmpty());
					return IsEmptyKey(m_kinfo.GetKey(m_table[id]));
				}
				bool CLOAK_CALL_THIS IsEmpty(In const iterator& id) const
				{
					CLOAK_ASSUME(m_hash.UseEmpty());
					return m_kinfo.Equals(m_kinfo.Empty, m_kinfo.GetKey(*id));
				}
				bool CLOAK_CALL_THIS IsEmpty(In const const_iterator& id) const
				{
					CLOAK_ASSUME(m_hash.UseEmpty());
					return m_kinfo.Equals(m_kinfo.Empty, m_kinfo.GetKey(*id));
				}
				bool CLOAK_CALL_THIS Delete(In const iterator& it)
				{
					bool r = IsDeleted(it) == false;
					m_kinfo.SetKey(&(*it), m_kinfo.Delete);
					return r;
				}
				bool CLOAK_CALL_THIS Delete(In const const_iterator& it)
				{
					bool r = IsDeleted(it) == false;
					m_kinfo.SetKey(&(*it), m_kinfo.Delete);
					return r;
				}
				void CLOAK_CALL_THIS FillWithEmpty(In pointer s, In pointer e)
				{
					CLOAK_ASSUME(reinterpret_cast<uintptr_t>(s) <= reinterpret_cast<uintptr_t>(e));
					for (; s != e; ++s)
					{
						::new(s)value_type();
						m_kinfo.SetKey(s, m_kinfo.Empty);
					}
				}
				bool CLOAK_CALL Resize(In_opt size_type delta = 0)
				{
					bool res = false;
					size_type ns = m_capacity;
					size_type rem = m_size;
					size_type deleted = m_deleted;
					if (m_hash.ConsiderShrink())
					{
						rem = rem - deleted;
						const size_type sthresh = m_hash.GetShrinkThreshold();
						if (sthresh > 0 && rem < sthresh && ns > DEFAULT_BUCKET_COUNT)
						{
							const float sf = m_hash.GetShrinkFactor();
							do { ns >>= 1; } while (ns > DEFAULT_BUCKET_COUNT && rem < static_cast<size_type>(ns * sf));
							res = true;
							deleted = 0;
						}
						m_hash.SetConsiderShrink(false);
					}
					if (delta > 0)
					{
						if (rem >= std::numeric_limits<size_type>::max() - delta) { throw std::length_error("resize overflow"); }
						if (ns < MIN_BUCKET_COUNT || rem + delta > m_hash.GetEnlargeSize(ns))
						{
							size_type req = m_hash.GetMinBuckets(rem + delta, 0);
							if (req > ns)
							{
								res = true;
								ns = m_hash.GetMinBuckets(rem + delta - deleted, ns);
								req = m_hash.GetMinBuckets(rem + delta - (deleted >> 2), 0);
								if (ns < req && ns < std::numeric_limits<size_type>::max() >> 1)
								{
									const size_type target = static_cast<size_type>(m_hash.GetShrinkSize(ns << 1));
									if (rem + delta - deleted >= target) { ns <<= 1; }
								}
							}
						}
					}
					if (res == true)
					{
						DenseHashTable tmp(*this, ns);
						swap(tmp);
					}
					return res;
				}
				void CLOAK_CALL_THIS ClearToSize(In size_type newBuckets)
				{
					if (m_table == nullptr) { m_table = m_vinfo.allocate(newBuckets); }
					else
					{
						Destroy(0, m_capacity);
						if (newBuckets != m_capacity) { m_table = m_vinfo.reallocateSave(m_table, m_capacity, newBuckets); }
					}
					CLOAK_ASSUME(m_table != nullptr);
					FillWithEmpty(m_table, &m_table[newBuckets]);
					m_size = 0;
					m_deleted = 0;
					m_capacity = newBuckets;
					m_hash.Reset(m_capacity);
				}
				void CLOAK_CALL_THIS CopyFrom(In const DenseHashTable& ht, In size_type minBuckets)
				{
					ClearToSize(m_hash.GetMinBuckets(ht.size(), minBuckets));
					const size_type cmo = m_capacity - 1;
					for (auto a = ht.begin(); a != ht.end(); ++a)
					{
						size_type numProbes = 0;
						size_type id = m_hash.hash(m_kinfo.GetKey(*a)) & cmo;
						const size_type fid = id;
						while (IsEmpty(id) == false)
						{
							numProbes++;
							id = CE_HT_PROBE_FUNC(cmo, fid, id, numProbes) & cmo;
							CLOAK_ASSUME(numProbes < m_capacity);
						}
						SetValue(&m_table[id], *it);
						m_size++;
					}
					m_hash.IncreaseCopyCounter();
				}
				// Returns two positions: 1st is the founded position, 2nd is where to insert. Exaclty one of these
				// two is allways ILLEGAL_BUCKET.
				std::pair<size_type, size_type> CLOAK_CALL_THIS FindPosition(In const key_type& key) const 
				{
					size_type numProbes = 0;
					const size_type cmo = m_capacity - 1;
					size_type id = m_hash.hash(key) & cmo;
					const size_type fid = id;
					size_type ip = ILLEGAL_BUCKET;
					while (true)
					{
						if (IsEmpty(id))
						{
							if (ip == ILLEGAL_BUCKET) { return std::pair<size_type, size_type>(ILLEGAL_BUCKET, id); }
							return std::pair<size_type, size_type>(ILLEGAL_BUCKET, ip);
						}
						else if (IsDeleted(id))
						{
							if (ip == ILLEGAL_BUCKET) { ip = id; }
						}
						else if (m_kinfo.Equals(key, m_kinfo.GetKey(m_table[id])))
						{
							return std::pair<size_type, size_type>(id, ILLEGAL_BUCKET);
						}
						numProbes++;
						id = CE_HT_PROBE_FUNC(cmo, fid, id, numProbes) & cmo;
						CLOAK_ASSUME(numProbes < m_capacity);
					}
				}
				iterator CLOAK_CALL_THIS InsertAt(In const_reference obj, In size_type pos)
				{
					if (size() >= max_size()) { throw std::length_error("insert overflow"); }
					if (IsDeleted(pos)) { m_deleted--; }
					else { m_size++; }
					SetValue(&m_table[pos], obj);
					return iterator(this, &m_table[pos], &m_table[m_capacity], false);
				}
				std::pair<iterator, bool> CLOAK_CALL_THIS InsertNoResize(In const_reference obj)
				{
					CLOAK_ASSUME((m_hash.UseEmpty() == false || m_kinfo.Equals(m_kinfo.GetKey(obj), m_kinfo.Empty) == false));
					CLOAK_ASSUME((m_hash.UseDeleted() == false || m_kinfo.Equals(m_kinfo.GetKey(obj), m_kinfo.Delete)) == false);
					const std::pair<size_type, size_type> p = FindPositions(m_kinfo.GetKey(obj));
					if (p.first != ILLEGAL_BUCKET) { return std::pair<iterator, bool>(iterator(this, &m_table[p.first], &m_table[m_capacity], false), false); }
					return std::pair<iterator, bool>(InsertAt(obj, p.second), true);
				}
				template<typename Iterator> void CLOAK_CALL_THIS Insert(In Iterator first, In Iterator last, In std::forward_iterator_tag)
				{
					const size_t d = std::distance(first, last);
					if (d >= std::numeric_limits<size_type>::max()) { throw std::length_error("insert-range overflow"); }
					Resize(d);
					for (; d > 0; --d, ++first) { InsertNoResize(*first); }
				}
				template<typename Iterator> void CLOAK_CALL_THIS Insert(In Iterator first, In Iterator last, In std::input_iterator_tag)
				{
					for (; first != last; ++first) { insert(*first); }
				}
			public:
				explicit CLOAK_CALL DenseHashTable(In_opt size_type expectedMinItems = 0, In_opt const H& hf = H(), In_opt const EqualityKey& eq = EqualityKey(), In_opt const ExtractKey& ek = ExtractKey(), In_opt const SetKey& sk = SetKey(), In_opt const Alloc& alloc = Alloc()) : m_hash(hf), m_kinfo(ek, sk, eq), m_deleted(0), m_size(0), m_capacity(0), m_table(nullptr), m_vinfo(alloc_impl<av_type>(alloc))
				{
					m_capacity = expectedMinItems == 0 ? DEFAULT_BUCKET_COUNT : m_hash.GetMinBuckets(expectedMinItems, 0);
					m_hash.Reset(m_capacity);
				}
				CLOAK_CALL DenseHashTable(In const DenseHashTable& ht, In_opt size_type minBuckets = DEFAULT_BUCKET_COUNT) : m_hash(ht.m_hash), m_kinfo(ht.m_kinfo), m_deleted(0), m_size(0), m_capacity(0), m_table(0), m_vinfo(ht.m_vinfo)
				{
					if (ht.m_hash.UseEmpty() == false)
					{
						CLOAK_ASSUME(ht.empty());
						m_capacity = m_hash.GetMinBuckets(ht.size(), minBuckets);
						m_hash.Reset(m_capacity);
						return;
					}
					m_hash.Reset(m_capacity);
					CopyFrom(ht, minBuckets);
				}
				CLOAK_CALL ~DenseHashTable()
				{
					if (m_table != nullptr)
					{
						Destroy(0, m_capacity);
						m_vinfo.deallocate(m_table, m_capacity);
					}
				}

				DenseHashTable& CLOAK_CALL_THIS operator=(In const DenseHashTable& ht)
				{
					if (&ht == this) { return *this; }
					if (ht.m_hash.UseEmpty() == false)
					{
						CLOAK_ASSUME(ht.empty());
						DenseHashTable tmp(ht);
						swap(tmp);
						return *this;
					}
					m_hash = ht.m_hash;
					m_kinfo = ht.m_kinfo;
					CopyFrom(ht, MIN_BUCKET_COUNT);
					return *this;
				}
				bool CLOAK_CALL_THIS operator==(In const DenseHashTable& ht) const
				{
					if (this == &ht) { return true; }
					if (size() != ht.size()) { return false; }
					for (const_iterator a = begin(); a != end(); ++a)
					{
						const_iterator b = ht.find(m_kinfo.GetKey(*a));
						if (b == ht.end() || *a != *b) { return false; }
					}
					return true;
				}
				bool CLOAK_CALL_THIS operator!=(In const DenseHashTable& ht) const { return !operator==(ht); }

				// The given key will be used to indicate a key as deleted. This key
				// should be an "impossible" entry, as it's impossible to retrieve
				// values with this key
				void CLOAK_CALL_THIS SetDeletedKey(In const key_type& key)
				{
					CLOAK_ASSUME(m_hash.UseEmpty() == false || m_kinfo.Equals(key, m_kinfo.Empty) == false);
					SquashDeleted();
					m_hash.SetUseDeleted(true);
					m_kinfo.Delete = key;
				}
				void CLOAK_CALL_THIS ClearDeletedKey()
				{
					SquashDeleted();
					m_hash.SetUseDeleted(false);
				}
				key_type CLOAK_CALL_THIS GetDeletedKey() const 
				{
					CLOAK_ASSUME(m_hash.UseDeleted());
					return m_kinfo.Delete;
				}
				// The given key will be used to indicate a key as empty. This key
				// should be an "impossible" entry, as it's impossible to retrieve
				// values with this key. This key can only be set once, and only 
				// before first use.
				void CLOAK_CALL_THIS SetEmptyKey(In const key_type& key)
				{
					CLOAK_ASSUME(m_hash.UseEmpty() == false);
					CLOAK_ASSUME(m_hash.UseDeleted() == false || m_kinfo.Equals(key, m_kinfo.Delete) == false);
					m_hash.SetUseEmpty(true);
					m_kinfo.Empty = key;

					CLOAK_ASSUME(m_table == nullptr);
					m_table = m_vinfo.allocate(m_capacity);
					FillWithEmpty(m_table, &m_table[m_capacity]);
				}
				key_type CLOAK_CALL_THIS GetEmptyKey() const
				{
					CLOAK_ASSUME(m_hash.UseEmpty());
					return m_kinfo.Empty;
				}
				void CLOAK_CALL_THIS GetResizingParameters(Out float* shrink, Out float* grow) const
				{
					CLOAK_ASSUME(shrink != nullptr && grow != nullptr);
					*shrink = m_hash.GetShrinkFactor();
					*grow = m_hash.GetEnlargeFactor();
				}
				void CLOAK_CALL_THIS SetResizingParameters(In float shrink, In float grow) { m_hash.Reset(shrink, grow, m_capacity); }
				// DV is a functor that takes an key and creates a coresponding default value
				template<typename DV> reference CLOAK_CALL_THIS FindOrInsert(In const key_type& key)
				{
					CLOAK_ASSUME((m_hash.UseEmpty() == false || m_kinfo.Equals(key, m_kinfo.GetKey(m_vinfo.Empty)) == false));
					CLOAK_ASSUME((m_hash.UseDeleted() == false || m_kinfo.Equals(key, m_kinfo.Delete)) == false);
					const std::pair<size_type, size_type> p = FindPosition(key);
					DV d;
					if (p.first != ILLEGAL_BUCKET) { return m_table[p.first]; }
					else if (Resize(1)) { return *InsertNoResize(d(key)).first; }
					return *InsertAt(d(key), p.second);
				}

				iterator CLOAK_CALL_THIS begin() { return iterator(this, m_table, &m_table[m_capacity], true); }
				const_iterator CLOAK_CALL_THIS begin() const { return const_iterator(this, m_table, &m_table[m_capacity], true); }
				const_iterator CLOAK_CALL_THIS cbegin() const { return const_iterator(this, m_table, &m_table[m_capacity], true); }
				iterator CLOAK_CALL_THIS end() { return iterator(this, &m_table[m_capacity], &m_table[m_capacity], true); }
				const_iterator CLOAK_CALL_THIS end() const { return const_iterator(this, &m_table[m_capacity], &m_table[m_capacity], true); }
				const_iterator CLOAK_CALL_THIS cend() const { return const_iterator(this, &m_table[m_capacity], &m_table[m_capacity], true); }
				size_type CLOAK_CALL_THIS size() const { return m_size - m_deleted; }
				size_type CLOAK_CALL_THIS max_size() const { return m_vinfo.max_size(); }
				bool CLOAK_CALL_THIS empty() const { return size() == 0; }
				size_type CLOAK_CALL_THIS bucket_count() const { return m_capacity; }
				size_type CLOAK_CALL_THIS max_bucket_count() const { return max_size(); }
				size_type CLOAK_CALL_THIS nonempty_bucket_count() const { return m_umElements; }
				void CLOAK_CALL_THIS resize(In size_type reqElements) { Resize(reqElements > m_size ? reqElements - m_size : 0); }
				void CLOAK_CALL_THIS swap(In DenseHashTable& ht)
				{
					{value_type t; SetValue(&t, m_vinfo.Empty); SetValue(&m_vinfo.Empty, ht.m_vinfo.Empty); SetValue(&ht.m_vinfo.Empty, t); }
					std::swap(m_hash, ht.m_hash);
					std::swap(m_kinfo, ht.m_kinfo);
					std::swap(m_deleted, ht.m_deleted);
					std::swap(m_size, ht.m_size);
					std::swap(m_capacity, ht.m_capacity);
					std::swap(m_table, ht.m_table);
					m_hash.Reset(m_capacity);
					ht.m_hash.Reset(ht.m_capacity);
				}
				void CLOAK_CALL_THIS clear()
				{
					const size_type nbs = m_hash.GetMinBuckets(0, 0);
					if (m_size == 0 && nbs == m_capacity) { return; }
					ClearToSize(nbs);
				}
				void CLOAK_CALL_THIS clearNoResize()
				{
					if (m_size > 0)
					{
						CLOAK_ASSUME(m_table != nullptr);
						Destroy(0, m_capacity);
						FillWithEmpty(m_table, &m_table[m_capacity]);
					}
					m_hash.Reset(m_capacity);
					m_size = 0;
					m_deleted = 0;
				}
				iterator CLOAK_CALL_THIS find(In const key_type& key)
				{
					if (m_size == m_deleted) { return end(); }
					const std::pair<size_type, size_type> p = FindPosition(key);
					if (p.first == ILLEGAL_BUCKET) { return end(); }
					return iterator(this, &m_table[p.first], &m_table[m_capacity], false);
				}
				const_iterator CLOAK_CALL_THIS find(In const key_type& key) const
				{
					if (m_size == m_deleted) { return end(); }
					const std::pair<size_type, size_type> p = FindPosition(key);
					if (p.first == ILLEGAL_BUCKET) { return end(); }
					return const_iterator(this, &m_table[p.first], &m_table[m_capacity], false);
				}
				std::pair<iterator, bool> CLOAK_CALL_THIS insert(In const_reference obj)
				{
					Resize(1);
					return insertNoResize(obj);
				}
				template<typename Iterator> void CLOAK_CALL_THIS insert(In Iterator first, In Iterator last) { insert(first, last, typename std::iterator_traits<Iterator>::iterator_category()); }
				size_type CLOAK_CALL_THIS erase(In const key_type& key)
				{
					CLOAK_ASSUME((m_hash.UseEmpty() == false || m_kinfo.Equals(key, m_kinfo.GetKey(m_vinfo.Empty)) == false));
					CLOAK_ASSUME((m_hash.UseDeleted() == false || m_kinfo.Equals(key, m_kinfo.Delete)) == false);
					iterator p = find(key);
					if (p != end())
					{
						Delete(p);
						m_deleted++;
						m_hash.SetConsiderShrink(true);
						return 1;
					}
					return 0;
				}
				void CLOAK_CALL_THIS erase(In const iterator& pos)
				{
					if (pos == end()) { return; }
					if (Delete(p))
					{
						m_deleted++;
						m_hash.SetConsiderShrink(true);
					}
				}
				void CLOAK_CALL_THIS erase(In const iterator& first, In const iterator& last)
				{
					for (; first != last; ++first)
					{
						if (Delete(first)) { m_deleted++; }
					}
					m_hash.SetConsiderShrink(true);
				}
				void CLOAK_CALL_THIS erase(In const const_iterator& pos)
				{
					if (pos == end()) { return; }
					if (Delete(p))
					{
						m_deleted++;
						m_hash.SetConsiderShrink(true);
					}
				}
				void CLOAK_CALL_THIS erase(In const const_iterator& first, In const const_iterator& last)
				{
					for (; first != last; ++first)
					{
						if (Delete(first)) { m_deleted++; }
					}
					m_hash.SetConsiderShrink(true);
				}
		};
	}
}

#ifdef CE_HT_PROBE_FUNC_D
#	undef CE_HT_PROBE_FUNC_D
#	undef CE_HT_PROBE_FUNC
#endif
#ifdef POP_MACRO_MIN
#	pragma pop_macro("min")
#	undef POP_MACRO_MIN
#endif
#ifdef POP_MACRO_MAX
#	pragma pop_macro("max")
#	undef POP_MACRO_MAX
#endif

#endif

#endif