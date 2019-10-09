#pragma once
#ifndef CE_API_GLOBAL_MEMORY_H
#define CE_API_GLOBAL_MEMORY_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Container/Allocator.h"
#include "CloakEngine/Helper/Predicate.h"
#include "CloakEngine/Helper/Types.h"
#include "CloakEngine/Global/Container/List.h"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>
#include <deque>
#include <queue>
#include <stack>
#include <string>
#include <assert.h>
#include <memory>
#include <type_traits>

//#define USE_MANAGED_STRINGS

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {

#ifndef USE_NEW_CONTAINER
		template<typename A, typename M = Global::Memory::MemoryHeap> class List : public std::vector<A, Global::Memory::Allocator<A, M>>
		{
			using std::vector<A, Global::Memory::Allocator<A, M>>::vector;
			public:
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename K, typename H = std::hash<K>, typename E = std::equal_to<K>, typename M = Global::Memory::MemoryHeap> class HashList : public std::unordered_set<K, H, E, Global::Memory::Allocator<K, M>>
		{
			using std::unordered_set<K, H, E, Global::Memory::Allocator<K, M>>::unordered_set;
			public:
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename K, typename V, typename H = std::hash<K>, typename E = std::equal_to<K>, typename M = Global::Memory::MemoryHeap> class HashMap : public std::unordered_map<K, V, H, E, Global::Memory::Allocator<std::pair<const K, V>, M>>
		{
			using std::unordered_map<K, V, H, E, Global::Memory::Allocator<std::pair<const K, V>, M>>::unordered_map;
			public:
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename K, typename V, typename P = std::less<K>, typename M = Global::Memory::MemoryHeap> class Map : public std::map<K, V, P, Global::Memory::Allocator<std::pair<const K, V>, M>>
		{
			using std::map<K, V, P, Global::Memory::Allocator<std::pair<const K, V>, M>>::map;
			public:
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename K, typename P = std::less<K>, typename M = Global::Memory::MemoryHeap> class Set : public std::set<K, P, Global::Memory::Allocator<K, M>>
		{
			using std::set<K, P, Global::Memory::Allocator<K, M>>::set;
			public:
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename T, typename M = Global::Memory::MemoryHeap> class Deque : public std::deque<T, Global::Memory::Allocator<T, M>>
		{
			using std::deque<T, Global::Memory::Allocator<T, M>>::deque;
			public:
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename T, typename D = Deque<T>> class Queue : public std::queue<T, D>
		{
			using std::queue<T, D>::queue;
			public:
				typedef typename D::iterator iterator;
				typedef typename D::const_iterator const_iterator;
				typedef typename D::reverse_iterator reverse_iterator;
				typedef typename D::const_reverse_iterator const_reverse_iterator;

				iterator CLOAK_CALL_THIS begin() { return c.begin(); }
				iterator CLOAK_CALL_THIS end() { return c.end(); }
				const_iterator CLOAK_CALL_THIS begin() const { return c.begin(); }
				const_iterator CLOAK_CALL_THIS end() const { return c.end(); }
				const_iterator CLOAK_CALL_THIS cbegin() const { return c.begin(); }
				const_iterator CLOAK_CALL_THIS cend() const { return c.end(); }
				reverse_iterator CLOAK_CALL_THIS rbegin() { return c.rbegin(); }
				reverse_iterator CLOAK_CALL_THIS rend() { return c.rend(); }
				const_reverse_iterator CLOAK_CALL_THIS crbegin() const { return c.crbegin(); }
				const_reverse_iterator CLOAK_CALL_THIS crend() const { return c.crend(); }

				void CLOAK_CALL_THIS clear() { while (size() > 0) { pop(); } }
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename T, typename Compare = std::less<T>, typename Allocator = Global::Memory::Allocator<T, Global::Memory::MemoryHeap>> class FlatSet : private std::vector<T,Allocator> {
			public:
				static_assert(!_ENFORCE_MATCHING_ALLOCATORS || std::is_same_v<T, typename Allocator::value_type>, _MISMATCHED_ALLOCATOR_MESSAGE("set<T, Compare, Allocator>", "T"));
				typedef std::vector<T, Allocator> container_type;
				typedef T key_type;
				typedef T value_type;
				typedef Compare key_compare;
				typedef Compare value_compare;

				using typename container_type::size_type;
				using typename container_type::difference_type;
				using typename container_type::allocator_type;
				using typename container_type::reference;
				using typename container_type::const_reference;
				using typename container_type::pointer;
				using typename container_type::const_pointer;
				using typename container_type::iterator;
				using typename container_type::const_iterator;
				using typename container_type::reverse_iterator;
				using typename container_type::const_reverse_iterator;

				using container_type::begin;
				using container_type::cbegin;
				using container_type::rbegin;
				using container_type::crbegin;
				using container_type::end;
				using container_type::cend;
				using container_type::rend;
				using container_type::crend;

				using container_type::empty;
				using container_type::size;
				using container_type::max_size;
			private:
				key_compare m_cmp;
			public:
				CLOAK_CALL FlatSet() : m_cmp(key_compare()) {}
				CLOAK_CALL FlatSet(In const FlatSet& o) : m_cmp(o.m_cmp), container_type(o) {}
				CLOAK_CALL FlatSet(In const FlatSet& o, In const allocator_type& al) : m_cmp(o.m_cmp), container_type(o, al) {}
				CLOAK_CALL FlatSet(In const key_compare& p, In const allocator_type& al) : m_cmp(p), container_type(al) {}
				template<typename I> CLOAK_CALL FlatSet(In I first, In I last) : m_cmp(key_compare()) { this->insert(first, last); }
				template<typename I> CLOAK_CALL FlatSet(In I first, In I last, In const key_compare& p, In const allocator_type& al) : m_cmp(p), container_type(al) { this->insert(first, last); }
				CLOAK_CALL FlatSet(In FlatSet&& o) : m_cmp(std::move(o.m_cmp)), container_type(o._Getal()) {}
				CLOAK_CALL FlatSet(In FlatSet&& o, In const allocator_type& al) : m_cmp(std::move(o.m_cmp)), container_type(al) {}
				CLOAK_CALL FlatSet(In std::initializer_list<value_type> l) : m_cmp(key_compare()) { insert(l.begin(), l.end()); }
				CLOAK_CALL FlatSet(In std::initializer_list<value_type> l, In const key_compare& p, In const allocator_type& al) : m_cmp(p), container_type(al) { insert(l.begin(), l.end()); }

				FlatSet& CLOAK_CALL_THIS operator=(In const FlatSet& o)
				{
					m_cmp = o.m_cmp;
					container_type::operator=(o);
					return *this;
				}
				FlatSet& CLOAK_CALL_THIS operator=(In FlatSet&& o)
				{
					m_cmp = std::move(o.m_cmp);
					container_type::operator=(std::move(o));
					return *this;
				}

				void CLOAK_CALL_THIS swap(In FlatSet& o) 
				{
					std::swap(m_cmp, o.m_cmp);
					container_type::swap(o); 
				}

				template<typename I> void CLOAK_CALL_THIS insert(In I first, In I last) { for (; first != last; ++first) { emplace(*first); } }
				iterator CLOAK_CALL_THIS insert(In const value_type& v) { return emplace(v).first; }
				iterator CLOAK_CALL_THIS insert(In value_type&& v) { return emplace(std::forward<value_type>(v)).first; }
				void CLOAK_CALL_THIS insert(In std::initializer_list<value_type> l) { insert(l.begin(), l.end()); }

				template<typename... Args> std::pair<iterator, bool> CLOAK_CALL_THIS emplace(In Args&&... args)
				{
					container_type::emplace_back(std::forward<Args>(args)...);
					iterator low = std::lower_bound(begin(), end(), container_type::back(), m_cmp);
					CLOAK_ASSUME(low != end());
					if (*low == container_type::back() && low + 1 != end()) { container_type::pop_back(); return std::make_pair(low, false); }
					if (low + 1 == end()) { return std::make_pair(low, true); }
					iterator res = (std::rotate(rbegin(), rbegin() + 1, reverse_iterator(low)) + 1).base();
					return std::make_pair(res, true);
				}
				iterator CLOAK_CALL_THIS find(In const key_type& key)
				{
					auto low = std::lower_bound(begin(), end(), key, m_cmp);
					if (low != end() && *low == key) { return low; }
					return end();
				}
				const_iterator CLOAK_CALL_THIS find(In const key_type& key) const
				{
					auto low = std::lower_bound(begin(), end(), key, m_cmp);
					if (low != end() && *low == key) { return low; }
					return end();
				}
				iterator CLOAK_CALL_THIS erase(In const_iterator i) { return container_type::erase(i); }
				size_type CLOAK_CALL_THIS erase(In const key_type& key)
				{
					auto low = find(key);
					if (low == end()) { return 0; }
					erase(low);
					return 1;
				}
				void CLOAK_CALL_THIS clear() { container_type::clear(); }
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename Key, typename T, typename Compare = std::less<Key>, typename Allocator = Global::Memory::Allocator<std::pair<Key, T>, Global::Memory::MemoryHeap>> class FlatMap : private std::vector<std::pair<Key, T>, Allocator> {
			public:
				static_assert(!_ENFORCE_MATCHING_ALLOCATORS || std::is_same_v<std::pair<Key, T>, typename Allocator::value_type>, _MISMATCHED_ALLOCATOR_MESSAGE("map<Key, Value, Compare, Allocator>", "pair<const Key, Value>"));
				typedef Key key_type;
				typedef T mapped_type;
				typedef std::pair<Key, T> value_type;
				typedef std::vector<value_type, Allocator> container_type;
				typedef Compare key_compare;
				struct value_compare {
					private:
						key_compare m_cmp;
					public:
						CLOAK_CALL value_compare(In const key_compare& c) : m_cmp(c) {}
						CLOAK_CALL value_compare(In key_compare&& c) : m_cmp(std::forward<key_compare>(c)) {}
						CLOAK_CALL value_compare(In const value_compare& c) : m_cmp(c.m_cmp) {}
						bool CLOAK_CALL_THIS operator()(In const value_type& a, In const value_type& b) const { return m_cmp(a.first, b.first); }
						bool CLOAK_CALL_THIS operator()(In const value_type& a, In const key_type& b) const { return m_cmp(a.first, b); }
				};

				using typename container_type::size_type;
				using typename container_type::difference_type;
				using typename container_type::allocator_type;
				using typename container_type::reference;
				using typename container_type::const_reference;
				using typename container_type::pointer;
				using typename container_type::const_pointer;
				using typename container_type::iterator;
				using typename container_type::const_iterator;
				using typename container_type::reverse_iterator;
				using typename container_type::const_reverse_iterator;

				using container_type::begin;
				using container_type::cbegin;
				using container_type::rbegin;
				using container_type::crbegin;
				using container_type::end;
				using container_type::cend;
				using container_type::rend;
				using container_type::crend;

				using container_type::empty;
				using container_type::size;
				using container_type::max_size;
			private:
				key_compare m_kc;
				value_compare m_vc;
			public:
				CLOAK_CALL FlatMap() : m_vc(key_compare()), m_kc(key_compare()) {}
				CLOAK_CALL FlatMap(In const FlatMap& o) : m_vc(o.m_vc), m_kc(o.m_kc), container_type(o) {}
				CLOAK_CALL FlatMap(In const FlatMap& o, In const allocator_type& al) : m_vc(o.m_vc), m_kc(o.m_kc), container_type(o,al) {}
				CLOAK_CALL FlatMap(In const key_compare& k, In const allocator_type& al) : m_vc(k), m_kc(k), container_type(al) {}
				template<typename I> CLOAK_CALL FlatMap(In I first, In I last) : m_vc(key_compare()), m_kc(key_compare()) { insert(first, last); }
				template<typename I> CLOAK_CALL FlatMap(In I first, In I last, In const key_compare& k, In const allocator_type& al) : m_vc(k), m_kc(k), container_type(al) { insert(first, last); }
				CLOAK_CALL FlatMap(In FlatMap&& o) : m_vc(key_compare()), m_kc(key_compare()), container_type(std::move(o)) {}
				CLOAK_CALL FlatMap(In FlatMap&& o, In const allocator_type& al) : m_vc(key_compare()), m_kc(key_compare()), container_type(std::move(o), al) {}
				CLOAK_CALL FlatMap(In std::initializer_list<value_type> il) : m_vc(key_compare()), m_kc(key_compare()) { insert(il.begin(), il.end()); }
				CLOAK_CALL FlatMap(In std::initializer_list<value_type> il, In const key_compare& k, In const allocator_type& al) : m_vc(k), m_kc(k), container_type(al) { insert(il.begin(), il.end()); }


				FlatMap& CLOAK_CALL_THIS operator=(In const FlatMap& o)
				{
					m_kc = o.m_kc;
					m_vc = o.m_vc;
					container_type::operator=(o);
					return *this;
				}
				FlatMap& CLOAK_CALL_THIS operator=(In FlatMap&& o)
				{
					m_kc = std::move(o.m_kc);
					m_vc = std::move(o.m_vc);
					container_type::operator=(std::move(o));
					return *this;
				}
				mapped_type& CLOAK_CALL_THIS operator[](In const key_type& k)
				{
					return emplace(k, mapped_type()).first->second;
				}
				mapped_type& CLOAK_CALL_THIS operator[](In key_type&& k)
				{
					return emplace(std::forward<key_type>(k), mapped_type()).first->second;
				}
				void CLOAK_CALL_THIS swap(In const FlatMap& o)
				{
					std::swap(m_kc, o.m_kc);
					std::swap(m_vc, o.m_vc);
					container_type::swap(o);
				}

				template<typename I> void CLOAK_CALL_THIS insert(In I first, In I last)
				{
					for (; first != last; ++first)
					{
						emplace(*first);
					}
				}
				iterator CLOAK_CALL_THIS insert(In const key_type& key, In const T& v) { return emplace(key, v).first; }
				iterator CLOAK_CALL_THIS isnert(In const key_type& key, In T&& v) { return emplace(key, std::move(v)).first; }
				iterator CLOAK_CALL_THIS insert(In key_type&& key, In const T& v) { return emplace(std::move(key), v).first; }
				iterator CLOAK_CALL_THIS insert(In key_type&& key, In T&& v) { return emplace(std::move(key), std::move(v)).first; }
				iterator CLOAK_CALL_THIS insert(In const value_type& v) { return emplace(v).first; }
				iterator CLOAK_CALL_THIS insert(In value_type&& v) { return emplace(std::move(v)).first; }
				void CLOAK_CALL_THIS insert(In std::initializer_list<value_type> il) { insert(il.begin(), il.end()); }
				
				template<typename... Args> std::pair<iterator, bool> CLOAK_CALL_THIS emplace(In Args&&... args)
				{
					container_type::emplace_back(std::forward<Args>(args)...);
					iterator low = std::lower_bound(begin(), end(), container_type::back(), m_vc);
					if (low->first == container_type::back().first && low + 1 != end())
					{
						container_type::pop_back();
						CLOAK_ASSUME(low->first == value_type(std::forward<Args>(args)...).first);
						return std::make_pair(low, false);
					}
					if (low + 1 == end())
					{
						CLOAK_ASSUME(low->first == value_type(std::forward<Args>(args)...).first);
						return std::make_pair(low, true);
					}
					iterator res = (std::rotate(rbegin(), rbegin() + 1, reverse_iterator(low)) + 1).base();
					CLOAK_ASSUME(res->first == value_type(std::forward<Args>(args)...).first);
					return std::make_pair(res, true);
				}

				iterator CLOAK_CALL_THIS find(In const key_type& key)
				{
					auto low = std::lower_bound(begin(), end(), key, m_vc);
					if (low != end() && low->first == key) { return low; }
					return end();
				}
				const_iterator CLOAK_CALL_THIS find(In const key_type& key) const
				{
					auto low = std::lower_bound(begin(), end(), key, m_vc);
					if (low != end() && low->first == key) { return low; }
					return end();
				}

				iterator CLOAK_CALL_THIS erase(In const_iterator pos)
				{
					return container_type::erase(pos);
				}
				size_type CLOAK_CALL_THIS erase(In const key_type& key)
				{
					iterator pos = find(key);
					if (pos == end()) { return 0; }
					container_type::erase(pos);
					return 1;
				}

				void CLOAK_CALL_THIS clear() { container_type::clear(); }
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<size_t ExpectedBits = 8, typename M = Global::Memory::MemoryHeap> class Basic_BitSet : private std::vector<typename int_by_size<(ExpectedBits+7)/8>::unsigned_t> {
			private:
				static constexpr size_t COUNT_BITS = ((ExpectedBits + 7) >> 3);
				static constexpr size_t COUNT_PER_ENTRY = COUNT_BITS << 3;
				static constexpr size_t COUNT_MASK = COUNT_PER_ENTRY - 1;
			public:
				typedef bool value_type;
				typedef typename int_by_size<COUNT_BITS>::unsigned_t mapped_type;
				typedef std::vector<mapped_type> container_type;
			private:
				using typename container_type::allocator_type;
				using typename container_type::size_type;

				size_type m_bitSize = 0;

				size_type CLOAK_CALL_THIS dataSize() const { return container_type::size(); }
				mapped_type CLOAK_CALL_THIS getData(In size_t p) const { return container_type::at(p); }
				void CLOAK_CALL_THIS set(In size_t dataPos, In mapped_type mask, In bool value)
				{
					while (dataPos >= dataSize()) { container_type::push_back(0); }
					if (value == true) { container_type::at(dataPos) |= mask; }
					else { container_type::at(dataPos) &= ~mask; }
				}
				void CLOAK_CALL_THIS set(In size_t bitPos, In bool value)
				{
					m_bitSize = max(m_bitSize, bitPos + 1);
					set(bitPos / COUNT_PER_ENTRY, static_cast<mapped_type>(1ULL << (bitPos & COUNT_MASK)), value);
				}
				bool CLOAK_CALL_THIS get(In size_t dataPos, In mapped_type mask) const
				{
					if (dataPos >= dataSize()) { return false; }
					return (container_type::at(dataPos) & mask) != 0;
				}
				bool CLOAK_CALL_THIS get(In size_t bitPos) const
				{
					return get(bitPos / COUNT_PER_ENTRY, static_cast<mapped_type>(1ULL << (bitPos & COUNT_MASK)));
				}
				template<typename I, std::enable_if_t<std::_Is_iterator_v<I>>* = nullptr> void CLOAK_CALL_THIS insert(In I first, In I last)
				{
					size_type p = 0;
					for (; first != last; ++first, ++p)
					{
						if (*first == true)
						{
							const size_type ip = p >> (2 + COUNT_BITS);
							while (dataSize() <= ip) { container_type::push_back(0); }
							container_type::at(ip) |= static_cast<mapped_type>(1ULL << (p & COUNT_MASK));
						}
					}
					m_bitSize = p;
				}
			public:
				struct iterator;
				struct const_iterator;
				struct reference {
					friend class Basic_BitSet;
					friend struct iterator;
					friend struct const_iterator;
					private:
						Basic_BitSet* const m_parent;
						const size_t m_bitPos = 0;

						CLOAK_CALL reference(In Basic_BitSet* p, size_t bitPos) : m_parent(p), m_bitPos(bitPos) {}
						CLOAK_CALL reference(In const reference& o) : m_parent(o.m_parent), m_bitPos(o.m_bitPos) {}
					public:
						bool CLOAK_CALL_THIS operator=(In bool v) 
						{ 
							if (m_parent != nullptr) { m_parent->set(m_bitPos, v); }
							return v; 
						}
						operator bool() 
						{ 
							if (m_parent != nullptr) { return m_parent->get(m_bitPos); }
							return false;
						}
				};
				typedef reference* pointer;
				typedef const bool& const_reference;
				typedef const bool* const_pointer;

				struct iterator {
					friend class Basic_BitSet;
					friend struct const_iterator;
					public:
						typedef std::bidirectional_iterator_tag iterator_category;
						typedef Basic_BitSet::reference value_type;
						typedef ptrdiff_t difference_type;
						typedef value_type* pointer;
						typedef value_type& reference;
					private:
						static constexpr size_t INVALID_POS = static_cast<size_t>(-1);
						Basic_BitSet* m_parent;
						size_t m_bitPos;
						Basic_BitSet::reference m_ref;

						CLOAK_CALL iterator(In Basic_BitSet* parent, In size_t bitPos) : m_parent(parent), m_bitPos(bitPos), m_ref(bitPos == INVALID_POS ? nullptr : parent, bitPos) { }
					public:
						CLOAK_CALL iterator(In const iterator& o) : m_parent(o.m_parent), m_bitPos(o.m_bitPos), m_ref(o.m_ref) { }

						operator const_iterator() { return const_iterator(m_parent, m_bitPos); }
						iterator& CLOAK_CALL_THIS operator=(In nullptr_t) { m_bitPos = INVALID_POS; return *this; }
						iterator CLOAK_CALL_THIS operator++(In int)
						{
							iterator r = *this;
							if (m_bitPos != INVALID_POS) 
							{ 
								m_bitPos++; 
								if (m_bitPos >= m_parent->size()) { m_bitPos = INVALID_POS; m_ref = Basic_BitSet::reference(nullptr, m_bitPos); }
								else { m_ref = Basic_BitSet::reference(m_parent, m_bitPos); }
							}
							return r;
						}
						iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_bitPos != INVALID_POS)
							{
								m_bitPos++;
								if (m_bitPos >= m_parent->size()) { m_bitPos = INVALID_POS; m_ref = Basic_BitSet::reference(nullptr, m_bitPos); }
								else { m_ref = Basic_BitSet::reference(m_parent, m_bitPos); }
							}
							return *this;
						}
						iterator CLOAK_CALL_THIS operator--(In int)
						{
							iterator r = *this;
							if (m_bitPos != INVALID_POS)
							{
								if (m_bitPos == 0) { m_bitPos = INVALID_POS; m_ref = Basic_BitSet::reference(nullptr, m_bitPos); }
								else { m_bitPos--; m_ref = Basic_BitSet::reference(m_parent, m_bitPos); }
							}
							else
							{
								const size_type s = m_parent->size();
								if (s > 0) { m_bitPos = s - 1; m_ref = Basic_BitSet::reference(m_parent, m_bitPos); }
							}
							return r;
						}
						iterator& CLOAK_CALL_THIS operator--()
						{
							if (m_bitPos != INVALID_POS)
							{
								if (m_bitPos == 0) { m_bitPos = INVALID_POS; m_ref = Basic_BitSet::reference(nullptr, m_bitPos); }
								else { m_bitPos--; m_ref = Basic_BitSet::reference(m_parent, m_bitPos); }
							}
							else
							{
								const size_type s = m_parent->size();
								if (s > 0) { m_bitPos = s - 1; m_ref = Basic_BitSet::reference(m_parent, m_bitPos); }
							}
							return *this;
						}
						reference CLOAK_CALL_THIS operator*() const { return m_ref; }
						pointer CLOAK_CALL_THIS operator->() const { return &m_ref; }
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_parent == o.m_parent && m_bitPos == o.m_bitPos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_parent != o.m_parent || m_bitPos != o.m_bitPos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_parent == o.m_parent && m_bitPos == o.m_bitPos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_parent != o.m_parent || m_bitPos != o.m_bitPos; }
				};
				struct const_iterator {
					friend class Basic_BitSet;
					friend struct iterator;
					public:
						typedef std::bidirectional_iterator_tag iterator_category;
						typedef bool value_type;
						typedef ptrdiff_t difference_type;
						typedef const bool* pointer;
						typedef const bool& reference;
					private:
						static constexpr size_t INVALID_POS = static_cast<size_t>(-1);
						const Basic_BitSet* m_parent;
						size_t m_bitPos;
						bool m_ref;

						CLOAK_CALL const_iterator(In const Basic_BitSet* parent, In size_t bitPos) : m_parent(parent), m_bitPos(bitPos), m_ref(bitPos == INVALID_POS ? false : parent->get(bitPos)) {}
					public:
						CLOAK_CALL const_iterator(In const const_iterator& o) : m_parent(o.m_parent), m_bitPos(o.m_bitPos), m_ref(o.m_ref) {}

						const_iterator& CLOAK_CALL_THIS operator=(In nullptr_t) { m_bitPos = INVALID_POS; return *this; }
						const_iterator CLOAK_CALL_THIS operator++(In int)
						{
							const_iterator r = *this;
							if (m_bitPos != INVALID_POS)
							{
								m_bitPos++;
								if (m_bitPos >= m_parent->size()) { m_bitPos = INVALID_POS; m_ref = false; }
								else { m_ref = m_parent->get(m_bitPos); }
							}
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_bitPos != INVALID_POS)
							{
								m_bitPos++;
								if (m_bitPos >= m_parent->size()) { m_bitPos = INVALID_POS; m_ref = false; }
								else { m_ref = m_parent->get(m_bitPos); }
							}
							return *this;
						}
						const_iterator CLOAK_CALL_THIS operator--(In int)
						{
							const_iterator r = *this;
							if (m_bitPos != INVALID_POS)
							{
								if (m_bitPos == 0) { m_bitPos = INVALID_POS; m_ref = false; }
								else { m_bitPos--; m_ref = m_parent->get(m_bitPos); }
							}
							else
							{
								const size_type s = m_parent->size();
								if (s > 0) { m_bitPos = s - 1; m_ref = m_parent->get(m_bitPos); }
							}
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator--()
						{
							if (m_bitPos != INVALID_POS)
							{
								if (m_bitPos == 0) { m_bitPos = INVALID_POS; m_ref = false; }
								else { m_bitPos--; m_ref = m_parent->get(m_bitPos); }
							}
							else
							{
								const size_type s = m_parent->size();
								if (s > 0) { m_bitPos = s - 1; m_ref = m_parent->get(m_bitPos); }
							}
							return *this;
						}
						const reference CLOAK_CALL_THIS operator*() const { return m_ref; }
						const pointer CLOAK_CALL_THIS operator->() const { return &m_ref; }
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_parent == o.m_parent && m_bitPos == o.m_bitPos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_parent != o.m_parent || m_bitPos != o.m_bitPos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_parent == o.m_parent && m_bitPos == o.m_bitPos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_parent != o.m_parent || m_bitPos != o.m_bitPos; }
				};
				typedef std::reverse_iterator<iterator> reverse_iterator;
				typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

				CLOAK_CALL Basic_BitSet() : m_bitSize(0) {}
				explicit CLOAK_CALL Basic_BitSet(In const allocator_type& al) : container_type(al) {}
				explicit CLOAK_CALL Basic_BitSet(In const size_type count, In_opt const allocator_type& al = allocator_type()) : container_type((count + COUNT_MASK) / COUNT_PER_ENTRY, al), m_bitSize(count) {}
				CLOAK_CALL Basic_BitSet(In const size_type count, In const bool val, In_opt const allocator_type& al = allocator_type()) : container_type((count + COUNT_MASK) / COUNT_PER_ENTRY, al), m_bitSize(count)
				{
					if (val == true)
					{
						for (size_type a = 0; a < count; a++) { set(a, true); }
					}
				}
				template<typename I, std::enable_if_t<std::_Is_iterator_v<I>>* = nullptr> CLOAK_CALL Basic_BitSet(In I first, In I last, In_opt const allocator_type& al = allocator_type()) : container_type(al), m_bitSize(0)
				{
					insert(first, last);
				}
				CLOAK_CALL Basic_BitSet(In std::initializer_list<value_type> il, In_opt const allocator_type& al = allocator_type()) : container_type(al), m_bitSize(0)
				{
					insert(il.begin(), il.end());
				}
				CLOAK_CALL Basic_BitSet(In const Basic_BitSet& o) : m_bitSize(0)
				{
					insert(o.begin(), o.end());
				}
				template<size_t E, typename N> CLOAK_CALL Basic_BitSet(In const Basic_BitSet<E, N>& o, In_opt const allocator_type& al) : container_type(al), m_bitSize(0)
				{
					insert(o.begin(), o.end());
				}
				CLOAK_CALL Basic_BitSet(In Basic_BitSet&& o) : container_type(std::forward(o)), m_bitSize(o.m_bitSize) {}
				CLOAK_CALL Basic_BitSet(In Basic_BitSet&& o, In const allocator_type& al) : container_type(std::forward(o), al), m_bitSize(o.m_bitSize) {}

				template<size_t E, typename N> Basic_BitSet& CLOAK_CALL_THIS operator=(In const Basic_BitSet<E, N>& o)
				{
					container_type::clear();
					m_bitSize = o.m_bitSize;
					insert(o.begin(), o.end());
					return *this;
				}
				Basic_BitSet& CLOAK_CALL_THIS operator=(In Basic_BitSet&& o)
				{
					m_bitSize = o.m_bitSize;
					container_type::operator=(std::forward<Basic_BitSet>(o));
					return *this;
				}
				reference CLOAK_CALL_THIS operator[](In size_type pos)
				{
					return reference(this, pos);
				}
				bool CLOAK_CALL_THIS operator[](In size_type pos) const
				{
					return get(pos);
				}
				bool CLOAK_CALL_THIS operator==(In const Basic_BitSet& o) const
				{
					const size_t s = min(dataSize(), o.dataSize());
					for (size_t a = s; a < o.dataSize(); a++) { if (o.getData(a) != 0) { return false; } }
					for (size_t a = 0; a < s; a++)
					{
						const mapped_type b = getData(a);
						const mapped_type c = o.getData(a);
						if (b != c) { return false; }
					}
					return true;
				}
				bool CLOAK_CALL_THIS operator!=(In const Basic_BitSet& o) const
				{
					const size_t s = min(dataSize(), o.dataSize());
					for (size_t a = s; a < o.dataSize(); a++) { if (o.getData(a) != 0) { return true; } }
					for (size_t a = 0; a < s; a++)
					{
						const mapped_type b = getData(a);
						const mapped_type c = o.getData(a);
						if (b != c) { return true; }
					}
					return false;
				}
				bool CLOAK_CALL_THIS operator>(In const Basic_BitSet& o) const
				{
					const size_t s1 = dataSize();
					const size_t s2 = o.dataSize();
					if (s1 != s2) { return s1 > s2; }
					for (size_t a = s1; a > 0; a--)
					{
						const mapped_type b = getData(a - 1);
						const mapped_type c = o.getData(a - 1);
						if (b != c) { return b > c; }
					}
					return false;
				}
				bool CLOAK_CALL_THIS operator<(In const Basic_BitSet& o) const
				{
					const size_t s1 = dataSize();
					const size_t s2 = o.dataSize();
					if (s1 != s2) { return s1 < s2; }
					for (size_t a = s1; a > 0; a--)
					{
						const mapped_type b = getData(a - 1);
						const mapped_type c = o.getData(a - 1);
						if (b != c) { return b < c; }
					}
					return false;
				}
				bool CLOAK_CALL_THIS operator>=(In const Basic_BitSet& o) const
				{
					const size_t s1 = dataSize();
					const size_t s2 = o.dataSize();
					if (s1 != s2) { return s1 > s2; }
					for (size_t a = s1; a > 0; a--)
					{
						const mapped_type b = getData(a - 1);
						const mapped_type c = o.getData(a - 1);
						if (b != c) { return b > c; }
					}
					return true;
				}
				bool CLOAK_CALL_THIS operator<=(In const Basic_BitSet& o) const
				{
					const size_t s1 = dataSize();
					const size_t s2 = o.dataSize();
					if (s1 != s2) { return s1 < s2; }
					for (size_t a = s1; a > 0; a--)
					{
						const mapped_type b = getData(a - 1);
						const mapped_type c = o.getData(a - 1);
						if (b != c) { return b < c; }
					}
					return true;
				}
				Basic_BitSet& CLOAK_CALL_THIS operator&=(In const Basic_BitSet& o)
				{
					const size_t s = min(dataSize(), o.dataSize());
					for (size_t a = s; a < dataSize(); a++) { container_type::at(a) = 0; }
					for (size_t a = 0; a < s; a++) { container_type::at(a) &= o.getData(a); }
					return *this;
				}
				Basic_BitSet& CLOAK_CALL_THIS operator|=(In const Basic_BitSet& o)
				{
					size_t s = min(dataSize(), o.dataSize());
					while (s < o.dataSize()) { container_type::push_back(0); s++; }
					for (size_t a = 0; a < s; a++) { container_type::at(a) |= o.getData(a); }
					m_bitSize = max(m_bitSize, o.m_bitSize);
					return *this;
				}
				Basic_BitSet CLOAK_CALL_THIS operator|(In const Basic_BitSet& o) const
				{
					Basic_BitSet r = *this;
					return r |= o;
				}
				Basic_BitSet CLOAK_CALL_THIS operator&(In const Basic_BitSet& o) const
				{
					Basic_BitSet r = *this;
					return r &= o;
				}

				size_type CLOAK_CALL_THIS size() const { return m_bitSize; }
				void CLOAK_CALL_THIS clear() { container_type::clear(); m_bitSize = 0; }
				reference CLOAK_CALL_THIS back() { return m_bitSize > 0 ? reference(this, m_bitSize - 1) : reference(nullptr, 0); }
				bool CLOAK_CALL_THIS back() const { return m_bitSize > 0 ? get(m_bitSize - 1) : false; }

				iterator CLOAK_CALL_THIS begin() 
				{
					return iterator(this, m_bitSize > 0 ? 0 : static_cast<size_t>(-1));
				}
				const_iterator CLOAK_CALL_THIS begin() const 
				{ 
					return const_iterator(this, m_bitSize > 0 ? 0 : static_cast<size_t>(-1));
				}
				const_iterator CLOAK_CALL_THIS cbegin() const 
				{ 
					return const_iterator(this, m_bitSize > 0 ? 0 : static_cast<size_t>(-1));
				}
				reverse_iterator CLOAK_CALL_THIS rbegin() 
				{ 
					return reverse_iterator(iterator(this, m_bitSize > 0 ? m_bitSize - 1 : static_cast<size_t>(-1)));
				}
				const_reverse_iterator CLOAK_CALL_THIS rbegin() const
				{
					return reverse_iterator(const_iterator(this, m_bitSize > 0 ? m_bitSize - 1 : static_cast<size_t>(-1)));
				}
				const_reverse_iterator CLOAK_CALL_THIS crbegin() const
				{
					return reverse_iterator(const_iterator(this, m_bitSize > 0 ? m_bitSize - 1 : static_cast<size_t>(-1)));
				}
				iterator CLOAK_CALL_THIS end() { return iterator(this, static_cast<size_t>(-1)); }
				const_iterator CLOAK_CALL_THIS end() const { return const_iterator(this, static_cast<size_t>(-1)); }
				const_iterator CLOAK_CALL_THIS cend() const { return const_iterator(this, static_cast<size_t>(-1)); }
				reverse_iterator CLOAK_CALL_THIS rend() { return reverse_iterator(begin()); }
				const_reverse_iterator CLOAK_CALL_THIS rend() const { return reverse_iterator(begin()); }
				const_reverse_iterator CLOAK_CALL_THIS crend() const { return reverse_iterator(iterator(this, static_cast<size_t>(-1))); }

				bool CLOAK_CALL_THIS contains(In const Basic_BitSet& o) const
				{
					const size_t s = min(dataSize(), o.dataSize());
					for (size_t a = s; a < o.dataSize(); a++) { if (o.getData(a) != 0) { return false; } }
					for (size_t a = 0; a < s; a++)
					{
						const mapped_type b = getData(a);
						const mapped_type c = o.getData(a);
						if ((b & c) != c) { return false; }
					}
					return true;
				}
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		typedef Basic_BitSet<sizeof(void*) * 8> BitSet;
		template<typename T, typename M = Global::Memory::MemoryHeap> class DisjointSet : private std::vector<T, Global::Memory::Allocator<T, M>> {
			public:
				typedef Global::Memory::Allocator<T, M> Allocator;
				typedef std::vector<T, Allocator> container_type;
				typedef T value_type;

				using typename container_type::size_type;
				using typename container_type::difference_type;
				using typename container_type::allocator_type;
				using typename container_type::reference;
				using typename container_type::const_reference;
				using typename container_type::pointer;
				using typename container_type::const_pointer;

				using container_type::empty;
				using container_type::size;
				using container_type::max_size;
			private:
				static constexpr size_type INVALID_POS = static_cast<size_type>(-1);
				struct SetLeaf {
					size_type Parent;
					uint32_t Rank;
				};
				std::vector<SetLeaf> m_forest;

				size_type CLOAK_CALL_THIS findParent(In size_type leafPos) const
				{
					SetLeaf* a = &m_forest[leafPos];
					while (leafPos != a->Parent)
					{
						leafPos = a->Parent;
						SetLeaf* p = a;
						a = &m_forest[a->Parent];
						p->Parent = a->Parent;
					}
					return leafPos;
				}
			public:
				struct const_iterator;
				struct iterator {
					friend class DisjointSet<T, M>;
					friend struct const_iterator;
					public:
						typedef std::random_access_iterator_tag iterator_category;
						typedef DisjointSet::value_type value_type;
						typedef ptrdiff_t difference_type;
						typedef value_type* pointer;
						typedef value_type& reference;
					private:
						size_type m_pos;
						DisjointSet<T, M>* m_parent;

						CLOAK_CALL iterator(In DisjointSet<T, M>* par, In size_type pos) : m_parent(par), m_pos(pos) {}
					public:
						CLOAK_CALL iterator(In const iterator& o) : m_parent(o.m_parent), m_pos(o.m_pos) {}

						operator const_iterator() { return const_iterator(m_parent, m_pos); }
						iterator& CLOAK_CALL_THIS operator=(In const iterator& o) { m_parent = o.m_parent; m_pos = o.m_pos; return *this; }
						iterator& CLOAK_CALL_THIS operator=(In nullptr_t) { m_pos = INVALID_POS; return *this; }
						iterator CLOAK_CALL_THIS operator++(In int)
						{
							iterator r = *this;
							if (m_pos != INVALID_POS)
							{
								m_pos++;
								if (m_pos >= m_parent->size()) { m_pos = INVALID_POS; }
							}
							return r;
						}
						iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_pos != INVALID_POS)
							{
								m_pos++;
								if (m_pos >= m_parent->size()) { m_pos = INVALID_POS; }
							}
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator+=(In int a)
						{
							if (m_pos != INVALID_POS)
							{
								if (a >= INVALID_POS - m_pos) { m_pos = INVALID_POS; }
								else
								{
									m_pos += a;
									if (m_pos >= m_parent->size()) { m_pos = INVALID_POS; }
								}
							}
							return *this;
						}
						iterator CLOAK_CALL_THIS operator+(In int a) const { return iterator(*this) += a; }
						iterator CLOAK_CALL_THIS operator+(In const iterator& a) const { CLOAK_ASSUME(a.m_parent == m_parent); return iterator(*this) += a.m_pos; }
						friend inline iterator CLOAK_CALL operator+(In int a, In const iterator& b) { return b + a; }

						iterator CLOAK_CALL_THIS operator--(In int)
						{
							iterator r = *this;
							if (m_pos != INVALID_POS)
							{
								if (m_pos == 0) { m_pos = INVALID_POS; }
								else { m_pos--; }
							}
							return r;
						}
						iterator& CLOAK_CALL_THIS operator--()
						{
							if (m_pos != INVALID_POS)
							{
								if (m_pos == 0) { m_pos = INVALID_POS; }
								else { m_pos--; }
							}
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator-=(In int a)
						{
							if (m_pos != INVALID_POS)
							{
								if (m_pos < a) { m_pos = INVALID_POS; }
								else { m_pos -= a; }
							}
							return *this;
						}
						iterator CLOAK_CALL_THIS operator-(In int a) const { return iterator(*this) -= a; }
						iterator CLOAK_CALL_THIS operator-(In const iterator& a) const { CLOAK_ASSUME(a.m_parent == m_parent); return iterator(*this) -= a.m_pos; }

						reference CLOAK_CALL_THIS operator*() const { return m_parent->at(m_pos); }
						pointer CLOAK_CALL_THIS operator->() const { return &m_parent->at(m_pos); }
						reference CLOAK_CALL_THIS operator[](In size_type t) const { return m_parent->at(m_pos + t); }
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_parent == o.m_parent && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_parent != o.m_parent || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_parent != o.m_parent || m_pos != o.m_pos; }

						iterator CLOAK_CALL_THIS unite(In const iterator& o) const { return m_parent->unite(*this, o); }
						iterator CLOAK_CALL_THIS unite(In const const_iterator& o) const { return m_parent->unite(*this, o); }
						iterator CLOAK_CALL_THIS parent() const { return m_parent->parent(*this); }
				};
				struct const_iterator {
					friend class DisjointSet<T, M>;
					friend struct iterator;
					public:
						typedef std::random_access_iterator_tag iterator_category;
						typedef DisjointSet::value_type value_type;
						typedef ptrdiff_t difference_type;
						typedef const value_type* pointer;
						typedef const value_type& reference;
					private:
						size_type m_pos;
						const DisjointSet<T, M>* m_parent;

						CLOAK_CALL const_iterator(In const DisjointSet<T, M>* par, In size_type pos) : m_parent(par), m_pos(pos) {}
					public:
						CLOAK_CALL const_iterator(In const iterator& o) : m_parent(o.m_parent), m_pos(o.m_pos) {}

						const_iterator& CLOAK_CALL_THIS operator=(In const const_iterator& o) { m_parent = o.m_parent; m_pos = o.m_pos; return *this; }
						const_iterator& CLOAK_CALL_THIS operator=(In nullptr_t) { m_pos = INVALID_POS; return *this; }
						const_iterator CLOAK_CALL_THIS operator++(In int)
						{
							const_iterator r = *this;
							if (m_pos != INVALID_POS)
							{
								m_pos++;
								if (m_pos >= m_parent->size()) { m_pos = INVALID_POS; }
							}
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_pos != INVALID_POS)
							{
								m_pos++;
								if (m_pos >= m_parent->size()) { m_pos = INVALID_POS; }
							}
							return *this;
						}
						const_iterator& CLOAK_CALL_THIS operator+=(In int a)
						{
							if (m_pos != INVALID_POS)
							{
								if (a >= INVALID_POS - m_pos) { m_pos = INVALID_POS; }
								else
								{
									m_pos += a;
									if (m_pos >= m_parent->size()) { m_pos = INVALID_POS; }
								}
							}
							return *this;
						}
						const_iterator CLOAK_CALL_THIS operator+(In int a) const { return const_iterator(*this) += a; }
						const_iterator CLOAK_CALL_THIS operator+(In const const_iterator& a) const { CLOAK_ASSUME(a.m_parent == m_parent); return const_iterator(*this) += a.m_pos; }
						friend inline const_iterator CLOAK_CALL operator+(In int a, In const const_iterator& b) { return b + a; }

						const_iterator CLOAK_CALL_THIS operator--(In int)
						{
							const_iterator r = *this;
							if (m_pos != INVALID_POS)
							{
								if (m_pos == 0) { m_pos = INVALID_POS; }
								else { m_pos--; }
							}
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator--()
						{
							if (m_pos != INVALID_POS)
							{
								if (m_pos == 0) { m_pos = INVALID_POS; }
								else { m_pos--; }
							}
							return *this;
						}
						const_iterator& CLOAK_CALL_THIS operator-=(In int a)
						{
							if (m_pos != INVALID_POS)
							{
								if (m_pos < a) { m_pos = INVALID_POS; }
								else { m_pos -= a; }
							}
							return *this;
						}
						const_iterator CLOAK_CALL_THIS operator-(In int a) const { return const_iterator(*this) -= a; }
						const_iterator CLOAK_CALL_THIS operator-(In const const_iterator& a) const { CLOAK_ASSUME(a.m_parent == m_parent); return const_iterator(*this) -= a.m_pos; }

						const reference CLOAK_CALL_THIS operator*() const { return m_parent->at(m_pos); }
						const pointer CLOAK_CALL_THIS operator->() const { return &m_parent->at(m_pos); }
						const reference CLOAK_CALL_THIS operator[](In size_type t) const { return m_parent->at(m_pos + t); }
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_parent == o.m_parent && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_parent != o.m_parent || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_parent != o.m_parent || m_pos != o.m_pos; }

						const_iterator CLOAK_CALL_THIS parent() const { return m_parent->parent(*this); }
				};
				typedef std::reverse_iterator<iterator> reverse_iterator;
				typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

				CLOAK_CALL DisjointSet() {}
				CLOAK_CALL DisjointSet(In const DisjointSet& o) : m_forest(o.m_forest), container_type(o) {}
				template<typename I> CLOAK_CALL DisjointSet(In I first, In I last) { insert(first, last); }
				CLOAK_CALL DisjointSet(In DisjointSet&& o) : container_type(std::forward(o)), m_forest(std::forward(o.m_forest)) {}
				CLOAK_CALL DisjointSet(In std::initializer_list<value_type> l) { insert(l.begin(), l.end()); }

				DisjointSet& CLOAK_CALL_THIS operator=(In const DisjointSet& o)
				{
					container_type::operator=(o);
					m_forest = o.m_forest;
					return *this;
				}
				DisjointSet& CLOAK_CALL_THIS operator=(In DisjointSet&& o)
				{
					m_forest = std::move(o.m_forest);
					container_type::operator=(std::forward(o));
					return *this;
				}

				void CLOAK_CALL_THIS swap(In DisjointSet& o)
				{
					m_forest.swap(o.m_forest);
					container_type::swap(o);
					return *this;
				}
				void CLOAK_CALL_THIS reserve(In size_type ns) { m_forest.reserve(ns); }

				template<typename I> void CLOAK_CALL_THIS insert(In I first, In I last) { for (; first != last; ++first) { emplace(*first); } }
				iterator CLOAK_CALL_THIS insert(In const value_type& v) { return emplace(v); }
				iterator CLOAK_CALL_THIS insert(In value_type&& v) { return emplace(std::forward<value_type>(v)); }
				void CLOAK_CALL_THIS insert(In std::initializer_list<value_type> l) { insert(l.begin(), l.end()); }

				template<typename... Args> iterator CLOAK_CALL_THIS emplace(In Args&&... args)
				{
					const size_type p = container_type::size();
					container_type::emplace_back(std::forward<Args>(args)...);
					SetLeaf l;
					l.Parent = p;
					l.Rank = 0;
					m_forest.push_back(l);
					return iterator(this, p);
				}
				iterator CLOAK_CALL_THIS unite(In size_type a, In size_type b)
				{
					a = findParent(a);
					b = findParent(b);
					if (a != b)
					{
						SetLeaf* la = &m_forest[a];
						SetLeaf* lb = &m_forest[b];
						if (la->Rank < lb->Rank) 
						{
							SetLeaf* t = la;
							la = lb;
							lb = t;
						}
						lb->Parent = a;
						if (la->Rank == lb->Rank) { la->Rank++; }
					}
					return iterator(this, a);
				}
				iterator CLOAK_CALL_THIS unite(In const iterator& a, In const iterator& b) 
				{ 
					CLOAK_ASSUME(a.m_parent == this); 
					CLOAK_ASSUME(b.m_parent == this); 
					if (a.m_pos == INVALID_POS || b.m_pos == INVALID_POS) { return iterator(this, INVALID_POS); }
					return unite(a.m_pos, b.m_pos); 
				}
				iterator CLOAK_CALL_THIS unite(In const const_iterator& a, In const iterator& b)
				{
					CLOAK_ASSUME(a.m_parent == this);
					CLOAK_ASSUME(b.m_parent == this);
					if (a.m_pos == INVALID_POS || b.m_pos == INVALID_POS) { return iterator(this, INVALID_POS); }
					return unite(a.m_pos, b.m_pos);
				}
				iterator CLOAK_CALL_THIS unite(In const iterator& a, In const const_iterator& b)
				{
					CLOAK_ASSUME(a.m_parent == this);
					CLOAK_ASSUME(b.m_parent == this);
					if (a.m_pos == INVALID_POS || b.m_pos == INVALID_POS) { return iterator(this, INVALID_POS); }
					return unite(a.m_pos, b.m_pos);
				}
				iterator CLOAK_CALL_THIS unite(In const const_iterator& a, In const const_iterator& b)
				{
					CLOAK_ASSUME(a.m_parent == this);
					CLOAK_ASSUME(b.m_parent == this);
					if (a.m_pos == INVALID_POS || b.m_pos == INVALID_POS) { return iterator(this, INVALID_POS); }
					return unite(a.m_pos, b.m_pos);
				}

				iterator CLOAK_CALL_THIS parent(In const iterator& o)
				{
					CLOAK_ASSUME(o.m_parent == this);
					if (o.m_pos == INVALID_POS) { return iterator(this, INVALID_POS); }
					return iterator(this, findParent(o.m_pos));
				}
				iterator CLOAK_CALL_THIS parent(In const const_iterator& o)
				{
					CLOAK_ASSUME(o.m_parent == this);
					if (o.m_pos == INVALID_POS) { return iterator(this, INVALID_POS); }
					return iterator(this, findParent(o.m_pos));
				}
				const_iterator CLOAK_CALL_THIS parent(In const iterator& o) const
				{
					CLOAK_ASSUME(o.m_parent == this);
					if (o.m_pos == INVALID_POS) { return iterator(this, INVALID_POS); }
					return const_iterator(this, findParent(o.m_pos));
				}
				const_iterator CLOAK_CALL_THIS parent(In const const_iterator& o) const
				{
					CLOAK_ASSUME(o.m_parent == this);
					if (o.m_pos == INVALID_POS) { return iterator(this, INVALID_POS); }
					return const_iterator(this, findParent(o.m_pos));
				}

				iterator CLOAK_CALL_THIS begin() { return iterator(this, 0); }
				const_iterator CLOAK_CALL_THIS begin() const { return const_iterator(this, 0); }
				const_iterator CLOAK_CALL_THIS cbegin() const { return const_iterator(this, 0); }
				reverse_iterator CLOAK_CALL_THIS rbegin() { return reverse_iterator(iterator(this, container_type::size() - 1)); }
				const_reverse_iterator CLOAK_CALL_THIS rbegin() const { return const_reverse_iterator(const_iterator(this, container_type::size() - 1)); }
				const_reverse_iterator CLOAK_CALL_THIS crbegin() const { return const_reverse_iterator(const_iterator(this, container_type::size() - 1)); }

				iterator CLOAK_CALL_THIS end() { return iterator(this, INVALID_POS); }
				const_iterator CLOAK_CALL_THIS end() const { return const_iterator(this, INVALID_POS); }
				const_iterator CLOAK_CALL_THIS cend() const { return const_iterator(this, INVALID_POS); }
				reverse_iterator CLOAK_CALL_THIS rend() { return reverse_iterator(iterator(this, INVALID_POS)); }
				const_reverse_iterator CLOAK_CALL_THIS rend() const { return const_reverse_iterator(const_iterator(this, INVALID_POS)); }
				const_reverse_iterator CLOAK_CALL_THIS crend() const { return const_reverse_iterator(const_iterator(this, INVALID_POS)); }

		};
		template<typename T, size_t D = 2, typename Compare = std::less<T>, typename M = Global::Memory::MemoryHeap> class Heap : private List<T,M> {
			public:
				typedef Compare key_compare;
				typedef T value_type;
				typedef List<T, M> container_type;

				using typename container_type::size_type;
				using typename container_type::difference_type;
				using typename container_type::allocator_type;
				using typename container_type::reference;
				using typename container_type::const_reference;
				using typename container_type::pointer;
				using typename container_type::const_pointer;

				using container_type::empty;
				using container_type::size;
				using container_type::max_size;
				using container_type::clear;
			private:
				key_compare m_kc;

				inline bool CLOAK_CALL_THIS restoreUp(In size_t i)
				{
					if (i == 0) { return false; }
					size_t p = (i - 1) / D;
					if (m_kc(container_type::at(i), container_type::at(p)) == true)
					{
						value_type tmp = std::move(container_type::at(i));
						do {
							container_type::at(i) = std::move(container_type::at(p));
							i = p;
							if (i == 0)
							{
								container_type::at(i) = std::move(tmp);
								return true;
							}
							else { p = (i - 1) / D; }
						} while (m_kc(tmp, container_type::at(p)) == true);
						container_type::at(i) = std::move(tmp);
						return true;
					}
					return false;
				}
				inline bool CLOAK_CALL_THIS restoreDown(In size_t i)
				{
					const size_t iv = container_type::size();
					if (i >= iv) { return false; }
					size_t mc = iv;
					for (size_t a = 0; a < D; a++)
					{
						const size_t child = (D*i) + a + 1;
						if (child < iv && (mc == iv || m_kc(container_type::at(child), container_type::at(mc)) == true)) { mc = child; }
					}
					if (mc < iv && m_kc(container_type::at(mc), container_type::at(i)) == true)
					{
						value_type tmp = std::move(container_type::at(i));
						do {
							container_type::at(i) = std::move(container_type::at(mc));
							i = mc;
							mc = iv;
							for (size_t a = 0; a < D; a++)
							{
								const size_t child = (D*i) + a + 1;
								if (child < iv && (mc == iv || m_kc(container_type::at(child), container_type::at(mc)) == true)) { mc = child; }
							}
						} while (mc < iv && m_kc(container_type::at(mc), tmp) == true);
						container_type::at(i) = std::move(tmp);
						return true;
					}
					return false;
				}
				CLOAK_FORCEINLINE bool CLOAK_CALL_THIS updateElement(In size_t i)
				{
					if (restoreUp(i) == false) { return restoreDown(i); }
					return true;
				}
			public:
				template<typename I> void CLOAK_CALL insert(In I first, In I last)
				{
					for (; first != last; ++first)
					{
						container_type::push_back(*first);
						restoreUp(container_type::size() - 1);
					}
				}
				void CLOAK_CALL push(In const value_type& v)
				{
					container_type::push_back(v);
					restoreUp(container_type::size() - 1);
				}
				void CLOAK_CALL push(In value_type&& v)
				{
					container_type::push_back(std::move(v));
					restoreUp(container_type::size() - 1);
				}
				template<typename... Args> void CLOAK_CALL emplace(In Args&&... args)
				{
					container_type::emplace_back(std::forward(args)...);
					restoreUp(container_type::size() - 1);
				}

				void CLOAK_CALL_THIS pop()
				{
					container_type::at(0) = std::move(container_type::at(container_type::size() - 1));
					container_type::pop_back();
					restoreDown(0);
				}

				value_type& CLOAK_CALL_THIS front() { return container_type::at(0); }
				const value_type& CLOAK_CALL_THIS front() const { return container_type::at(0); }

				//Updates all entries, re-evaluating their priorites
				void CLOAK_CALL_THIS update() { for (size_t a = 1; a < container_type::size(); a++) { restoreUp(a); } }

				CLOAK_CALL Heap() : m_kc(key_compare()) {}
				explicit CLOAK_CALL Heap(In const key_compare& k) : m_kc(k) {}
				CLOAK_CALL Heap(In const Heap& o) : m_kc(o.m_kc), container_type(o) {}
				CLOAK_CALL Heap(In const Heap& o, In const allocator_type& al) : m_kc(o.m_kc), container_type(o, al) {}
				CLOAK_CALL Heap(In const key_compare& k, In const allocator_type& al) : m_kc(k), container_type(al) {}
				template<typename I> CLOAK_CALL Heap(In I first, In I last) : m_kc(key_compare()) { insert(first, last); }
				template<typename I> CLOAK_CALL Heap(In I first, In I last, In const key_compare& k, In const allocator_type& al) : m_kc(k), container_type(al) { insert(first, last); }
				CLOAK_CALL Heap(In Heap&& o) : container_type(std::move(o)), m_kc(std::move(o.m_kc)) {}
				CLOAK_CALL Heap(In Heap&& o, In const allocator_type& al) : container_type(std::move(o), al), m_kc(std::move(o.m_kc)) {}
				CLOAK_CALL Heap(In std::initializer_list<value_type> il) : m_kc(key_compare()) { insert(il.begin(), il.end()); }
				CLOAK_CALL Heap(In std::initializer_list<value_type> il, In const key_compare& k, In const allocator_type& al) : m_kc(k), container_type(al) { insert(il.begin(), il.end()); }
		};

		template<typename T> class PagedList {
			public:
				typedef T value_type;
				typedef T* pointer;
				typedef const T* const_pointer;
				typedef T& reference;
				typedef const T& const_reference;
				typedef size_t size_type;
			private:
				struct Page {
					Page* Next;
					Page* Prev;
					size_type Offset;
				};
				static constexpr size_t PAGE_MAX_SIZE = 2 MB;
				static constexpr size_t PAGE_HEAD_SIZE = (sizeof(Page) + std::alignment_of<T>::value - 1) & ~(std::alignment_of<T>::value - 1);
				static constexpr size_t PAGE_DATA_COUNT = (PAGE_MAX_SIZE - PAGE_HEAD_SIZE) / sizeof(T);
				static constexpr size_t PAGE_SIZE = PAGE_HEAD_SIZE + (sizeof(T) * PAGE_DATA_COUNT);

				Global::Memory::PageStackAllocator* const m_alloc;
				size_type m_size;
				size_type m_offset;
				Page* m_start;
				Page* m_end;

				Page* CLOAK_CALL_THIS AllocatePage(In Page* prev, In Page* next)
				{
					Page* p = reinterpret_cast<Page*>(m_alloc->Allocate(PAGE_SIZE));
					p->Prev = prev;
					p->Next = next;
					if (next != nullptr) { next->Prev = p; }
					if (prev != nullptr) { prev->Next = p; }
					p->Offset = 0;
					return p;
				}
			public:
				struct iterator;
				struct const_iterator;
				struct iterator {
					friend struct const_iterator;
					friend class PagedList;
					public:
						typedef std::random_access_iterator_tag iterator_category;
						typedef T value_type;
						typedef ptrdiff_t difference_type;
						typedef value_type* pointer;
						typedef value_type& reference;
					private:
						size_type m_pos;
						PagedList* m_parent;
						Page* m_page;
						size_type m_offset;

						CLOAK_CALL iterator(In PagedList* parent, In size_t p) : m_parent(parent), m_pos(p) 
						{
							Page* pp = m_parent->m_start;
							size_t os = m_parent->m_offset + m_pos;
							while (os > PAGE_DATA_COUNT)
							{
								pp = pp->Next;
								os -= PAGE_DATA_COUNT;
							}
							m_page = pp;
							m_offset = os;
						}
					public:
						CLOAK_CALL iterator() : m_parent(nullptr), m_pos(0), m_page(nullptr), m_offset(0) {}
						CLOAK_CALL iterator(In const iterator& o) : m_parent(o.m_parent), m_pos(o.m_pos), m_page(o.m_page), m_offset(o.m_offset) {}

						iterator& CLOAK_CALL_THIS operator=(In const iterator& o) { m_parent = o.m_parent; m_pos = o.m_pos; m_page = o.m_page; m_offset = o.m_offset; return *this; }

						iterator CLOAK_CALL_THIS operator++(In int) 
						{
							iterator r = *this;
							if (m_parent != nullptr && m_pos < m_parent->size()) 
							{ 
								m_pos++; 
								m_offset++;
								if (m_offset == PAGE_DATA_COUNT) { m_offset = 0; m_page = m_page->Next; }
							}
							return r;
						}
						iterator& CLOAK_CALL_THIS operator++() 
						{ 
							if (m_parent != nullptr && m_pos < m_parent->size())
							{
								m_pos++;
								m_offset++;
								if (m_offset == PAGE_DATA_COUNT) { m_offset = 0; m_page = m_page->Next; }
							}
							return *this; 
						}
						iterator CLOAK_CALL_THIS operator--(In int)
						{
							iterator r = *this;
							if (m_pos > 0) 
							{ 
								m_pos--; 
								if (m_offset == 0) { m_page = m_page->Prev; m_offset = PAGE_DATA_COUNT; }
								m_offset--;
							}
							else if(m_parent != nullptr) { m_pos = m_parent->size(); }
							return r;
						}
						iterator& CLOAK_CALL_THIS operator--()
						{
							if (m_pos > 0) 
							{ 
								m_pos--; 
								if (m_offset == 0) { m_page = m_page->Prev; m_offset = PAGE_DATA_COUNT; }
								m_offset--;
							}
							else if (m_parent != nullptr) { m_pos = m_parent->size(); }
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator+=(In size_type a)
						{
							if (m_parent != nullptr)
							{
								const size_t s = m_parent->size();
								if (m_pos + a < s) 
								{ 
									m_pos += a; 
									m_offset += a;
									while (m_offset >= PAGE_DATA_COUNT)
									{
										m_page = m_page->Next;
										m_offset -= PAGE_DATA_COUNT;
									}
								}
								else { m_pos = s; }
							}
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator-=(In size_type a)
						{
							if (m_pos >= a) 
							{ 
								m_pos -= a; 
								while (a > m_offset)
								{
									m_page = m_page->Prev;
									m_offset += PAGE_DATA_COUNT;
								}
								m_offset -= a;
							}
							else if (m_parent != nullptr) { m_pos = m_parent->size(); }
							return *this;
						}
						iterator CLOAK_CALL_THIS operator+(In size_type a) const { return iterator(*this) += a; }
						iterator CLOAK_CALL_THIS operator-(In size_type a) const { return iterator(*this) -= a; }

						reference CLOAK_CALL_THIS operator[](In size_type a) const 
						{
							Page* p = m_page;
							size_t o = m_offset + a;
							while (o >= PAGE_DATA_COUNT) { p = p->Next; o -= PAGE_DATA_COUNT; }
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) + PAGE_HEAD_SIZE);
							return d[o]; 
						}
						reference CLOAK_CALL_THIS operator*() const 
						{ 
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return d[m_offset];
						}
						pointer CLOAK_CALL_THIS operator->() const 
						{ 
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return &d[m_offset];
						}

						bool CLOAK_CALL_THIS operator<(In const iterator& o) const { return m_parent == o.m_parent && m_pos < o.m_pos; }
						bool CLOAK_CALL_THIS operator<(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos < o.m_pos; }
						bool CLOAK_CALL_THIS operator>(In const iterator& o) const { return m_parent == o.m_parent && m_pos > o.m_pos; }
						bool CLOAK_CALL_THIS operator>(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos > o.m_pos; }
						bool CLOAK_CALL_THIS operator<=(In const iterator& o) const { return m_parent == o.m_parent && m_pos <= o.m_pos; }
						bool CLOAK_CALL_THIS operator<=(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos <= o.m_pos; }
						bool CLOAK_CALL_THIS operator>=(In const iterator& o) const { return m_parent == o.m_parent && m_pos >= o.m_pos; }
						bool CLOAK_CALL_THIS operator>=(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos >= o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_parent == o.m_parent && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_parent != o.m_parent || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_parent != o.m_parent || m_pos != o.m_pos; }
				};
				struct const_iterator {
					friend struct iterator;
					friend class PagedList;
					public:
						typedef std::random_access_iterator_tag iterator_category;
						typedef T value_type;
						typedef ptrdiff_t difference_type;
						typedef const value_type* pointer;
						typedef const value_type& reference;
					private:
						size_type m_pos;
						const PagedList* m_parent;
						Page* m_page;
						size_type m_offset;

						CLOAK_CALL const_iterator(In const PagedList* parent, In size_t p) : m_parent(parent), m_pos(p) 
						{
							Page* pp = m_parent->m_start;
							size_t os = m_parent->m_offset + m_pos;
							while (os > PAGE_DATA_COUNT)
							{
								pp = pp->Next;
								os -= PAGE_DATA_COUNT;
							}
							m_page = pp;
							m_offset = os;
						}
					public:
						CLOAK_CALL const_iterator() : m_parent(nullptr), m_pos(0), m_page(nullptr), m_offset(0) {}
						CLOAK_CALL const_iterator(In const iterator& o) : m_parent(o.m_parent), m_pos(o.m_pos), m_page(o.m_page), m_offset(o.m_offset) {}
						CLOAK_CALL const_iterator(In const const_iterator& o) : m_parent(o.m_parent), m_pos(o.m_pos), m_page(o.m_page), m_offset(o.m_offset) {}

						const_iterator& CLOAK_CALL_THIS operator=(In const iterator& o) { m_parent = o.m_parent; m_pos = o.m_pos; m_page = o.m_page; m_offset = o.m_offset; return *this; }
						const_iterator& CLOAK_CALL_THIS operator=(In const const_iterator& o) { m_parent = o.m_parent; m_pos = o.m_pos; m_page = o.m_page; m_offset = o.m_offset; return *this; }

						const_iterator CLOAK_CALL_THIS operator++(In int)
						{
							const_iterator r = *this;
							if (m_parent != nullptr && m_pos < m_parent->size())
							{
								m_pos++;
								m_offset++;
								if (m_offset == PAGE_DATA_COUNT) { m_offset = 0; m_page = m_page->Next; }
							}
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_parent != nullptr && m_pos < m_parent->size())
							{
								m_pos++;
								m_offset++;
								if (m_offset == PAGE_DATA_COUNT) { m_offset = 0; m_page = m_page->Next; }
							}
							return *this;
						}
						const_iterator CLOAK_CALL_THIS operator--(In int)
						{
							const_iterator r = *this;
							if (m_pos > 0)
							{
								m_pos--;
								if (m_offset == 0) { m_page = m_page->Prev; m_offset = PAGE_DATA_COUNT; }
								m_offset--;
							}
							else if (m_parent != nullptr) { m_pos = m_parent->size(); }
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator--()
						{
							if (m_pos > 0)
							{
								m_pos--;
								if (m_offset == 0) { m_page = m_page->Prev; m_offset = PAGE_DATA_COUNT; }
								m_offset--;
							}
							else if (m_parent != nullptr) { m_pos = m_parent->size(); }
							return *this;
						}
						const_iterator& CLOAK_CALL_THIS operator+=(In size_type a)
						{
							if (m_parent != nullptr)
							{
								const size_t s = m_parent->size();
								if (m_pos + a < s)
								{
									m_pos += a;
									m_offset += a;
									while (m_offset >= PAGE_DATA_COUNT)
									{
										m_page = m_page->Next;
										m_offset -= PAGE_DATA_COUNT;
									}
								}
								else { m_pos = s; }
							}
							return *this;
						}
						const_iterator& CLOAK_CALL_THIS operator-=(In size_type a)
						{
							if (m_pos >= a)
							{
								m_pos -= a;
								while (a > m_offset)
								{
									m_page = m_page->Prev;
									m_offset += PAGE_DATA_COUNT;
								}
								m_offset -= a;
							}
							else if (m_parent != nullptr) { m_pos = m_parent->size(); }
							return *this;
						}
						const_iterator CLOAK_CALL_THIS operator+(In size_type a) const { return const_iterator(*this) += a; }
						const_iterator CLOAK_CALL_THIS operator-(In size_type a) const { return const_iterator(*this) -= a; }

						reference CLOAK_CALL_THIS operator[](In size_type a) const
						{
							Page* p = m_page;
							size_t o = m_offset + a;
							while (o >= PAGE_DATA_COUNT) { p = p->Next; o -= PAGE_DATA_COUNT; }
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) + PAGE_HEAD_SIZE);
							return d[o];
						}
						reference CLOAK_CALL_THIS operator*() const
						{
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return d[m_offset];
						}
						pointer CLOAK_CALL_THIS operator->() const
						{
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return &d[m_offset];
						}

						bool CLOAK_CALL_THIS operator<(In const iterator& o) const { return m_parent == o.m_parent && m_pos < o.m_pos; }
						bool CLOAK_CALL_THIS operator<(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos < o.m_pos; }
						bool CLOAK_CALL_THIS operator>(In const iterator& o) const { return m_parent == o.m_parent && m_pos > o.m_pos; }
						bool CLOAK_CALL_THIS operator>(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos > o.m_pos; }
						bool CLOAK_CALL_THIS operator<=(In const iterator& o) const { return m_parent == o.m_parent && m_pos <= o.m_pos; }
						bool CLOAK_CALL_THIS operator<=(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos <= o.m_pos; }
						bool CLOAK_CALL_THIS operator>=(In const iterator& o) const { return m_parent == o.m_parent && m_pos >= o.m_pos; }
						bool CLOAK_CALL_THIS operator>=(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos >= o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_parent == o.m_parent && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_parent == o.m_parent && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_parent != o.m_parent || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_parent != o.m_parent || m_pos != o.m_pos; }
				};
				typedef std::reverse_iterator<iterator> reverse_iterator;
				typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

				CLOAK_CALL PagedList(In Global::Memory::PageStackAllocator* allocator) : m_alloc(allocator), m_size(0), m_offset(0), m_start(nullptr), m_end(nullptr) {}
				CLOAK_CALL PagedList(In PagedList&& o) : m_alloc(o.m_alloc), m_size(o.m_size), m_offset(o.m_offset), m_start(o.m_start), m_end(o.m_end) 
				{
					o.m_start = nullptr;
					o.m_end = nullptr;
					o.m_size = 0;
					o.m_offset = 0;
				}
				CLOAK_CALL PagedList(In const PagedList& o) : m_alloc(o.m_alloc), m_size(o.m_size), m_offset(0), m_start(nullptr), m_end(nullptr)
				{
					Page* op = o.m_start;
					Page* lp = nullptr;
					const size_type os = o.m_size + o.m_offset;
					while (op != nullptr)
					{
						Page* np = AllocatePage(lp, nullptr);
						if (lp == nullptr) { m_start = np; }
						T* od = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(op) + PAGE_HEAD_SIZE);
						T* nd = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(np) + PAGE_HEAD_SIZE);
						const size_type ds = op->Next != nullptr ? PAGE_DATA_COUNT : (os % PAGE_DATA_COUNT);
						for (size_type a = op->Offset, b = 0; a < ds; a++, b++) { new(&nd[b])T(od[a]); }
						lp = np;
						op = op->Next;
					}
					m_end = lp;
				}
				CLOAK_CALL ~PagedList() { clear(); }

				PagedList& CLOAK_CALL_THIS operator=(In const PagedList& o)
				{
					if (&o != this)
					{
						clear();
						Page* op = o.m_start;
						Page* lp = nullptr;
						const size_type os = o.m_size + o.m_offset;
						while (op != nullptr)
						{
							Page* np = AllocatePage(lp, nullptr);
							if (lp == nullptr) { m_start = np; }
							T* od = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(op) + PAGE_HEAD_SIZE);
							T* nd = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(np) + PAGE_HEAD_SIZE);
							const size_type ds = op->Next != nullptr ? PAGE_DATA_COUNT : (os % PAGE_DATA_COUNT);
							for (size_type a = op->Offset, b = 0; a < ds; a++, b++) { new(&nd[b])T(od[a]); }
							lp = np;
							op = op->Next;
						}
						m_end = lp;
					}
					return *this;
				}

				iterator CLOAK_CALL_THIS push_back(In const T& o)
				{
					const size_t os = (m_size + m_offset) % PAGE_DATA_COUNT;
					if (os == 0)
					{
						m_end = AllocatePage(m_end, nullptr);
						if (m_start == nullptr) { m_start = m_end; }
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_end) + PAGE_HEAD_SIZE);
					d[os] = o;
					return iterator(this, m_size++);
				}
				iterator CLOAK_CALL_THIS push_front(In const T& o)
				{
					if (m_offset == 0)
					{
						m_start = AllocatePage(nullptr, m_start);
						if (m_end == nullptr) { m_end = m_start; }
						m_start->Offset = m_offset = PAGE_DATA_COUNT;
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_start) + PAGE_HEAD_SIZE);
					d[--m_offset] = o;
					m_start->Offset--;
					m_size++;
					return iterator(this, 0);
				}
				template<typename... Args> iterator CLOAK_CALL_THIS emplace_back(In Args&&... args)
				{
					const size_t os = (m_size + m_offset) % PAGE_DATA_COUNT;
					if (os == 0)
					{
						m_end = AllocatePage(m_end, nullptr);
						if (m_start == nullptr) { m_start = m_end; }
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_end) + PAGE_HEAD_SIZE);
					d[os] = T(std::forward<Args>(args)...);
					return iterator(this, m_size++);
				}
				template<typename... Args> iterator CLOAK_CALL_THIS emplace_front(In Args&&... args)
				{
					if (m_offset == 0)
					{
						m_start = AllocatePage(nullptr, m_start);
						if (m_end == nullptr) { m_end = m_start; }
						m_start->Offset = m_offset = PAGE_DATA_COUNT;
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_start) + PAGE_HEAD_SIZE);
					d[--m_offset] = T(std::forward<Args>(args)...);
					m_start->Offset--;
					m_size++;
					return iterator(this, 0);
				}
				void CLOAK_CALL_THIS pop_back()
				{
					CLOAK_ASSUME(m_size > 0);
					m_size--;
					const size_t os = (m_size + m_offset) % PAGE_DATA_COUNT;
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_end) + PAGE_HEAD_SIZE);
					d[os].~T();
					if (os == 0 || m_size == 0)
					{
						Page* l = m_end;
						m_end = m_end->Prev;
						m_alloc->Free(l);
						if (m_size == 0) { m_start = m_end; m_offset = 0; }
					}
				}
				void CLOAK_CALL_THIS pop_front()
				{
					CLOAK_ASSUME(m_size > 0);
					m_size--;
					const size_t os = m_offset++;
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_start) + PAGE_HEAD_SIZE);
					d[os].~T();
					if (m_offset == PAGE_DATA_COUNT || m_size == 0)
					{
						Page* l = m_start;
						m_start = m_start->Next;
						m_alloc->Free(l);
						if (m_size == 0) { m_end = m_start; m_offset = 0; }
					}
				}

				T& CLOAK_CALL_THIS at(In size_type p)
				{
					CLOAK_ASSUME(p < m_size);
					size_t os = p + m_offset;
					Page* l = m_start;
					while (os >= PAGE_DATA_COUNT)
					{
						l = l->Next;
						os -= PAGE_DATA_COUNT;
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(l) + PAGE_HEAD_SIZE);
					return d[os];
				}
				const T& CLOAK_CALL_THIS at(In size_type p) const
				{
					CLOAK_ASSUME(p < m_size);
					size_t os = p + m_offset;
					Page* l = m_start;
					while (os >= PAGE_DATA_COUNT)
					{
						l = l->Next;
						os -= PAGE_DATA_COUNT;
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(l) + PAGE_HEAD_SIZE);
					return d[os];
				}
				T& CLOAK_CALL_THIS operator[](In size_type p) { return at(p); }
				const T& CLOAK_CALL_THIS operator[](In size_type p) const { return at(p); }

				size_type CLOAK_CALL_THIS size() const { return m_size; }
				void CLOAK_CALL_THIS clear()
				{
					Page* p = m_end;
					const size_type s = m_size + m_offset;
					while (p != nullptr)
					{
						T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) + PAGE_HEAD_SIZE);
						const size_t ds = p->Next != nullptr ? PAGE_DATA_COUNT : (s % PAGE_DATA_COUNT);
						for (size_type a = p->Offset; a < ds; a++) { d[a].~T(); }

						Page* lp = p;
						p = p->Prev;
						m_alloc->Free(lp);
					}
					m_size = 0;
					m_offset = 0;
					m_start = nullptr;
					m_end = nullptr;
				}
				void CLOAK_CALL_THIS resize(In size_type count)
				{
					if (m_size == count) { return; }
					if (m_size < count) 
					{
						do {
							emplace_back();
						} while (m_size < count);
					}
					else
					{
						do {
							pop_back();
						} while (m_size > count);
					}
				}

				iterator CLOAK_CALL_THIS begin() { return iterator(this, 0); }
				const_iterator CLOAK_CALL_THIS begin() const { return const_iterator(this, 0); }
				const_iterator CLOAK_CALL_THIS cbegin() const { return const_iterator(this, 0); }
				reverse_iterator CLOAK_CALL_THIS rbegin() { return reverse_iterator(iterator(this, m_size > 0 ? m_size - 1 : m_size)); }
				const_reverse_iterator CLOAK_CALL_THIS rbegin() const { return const_reverse_iterator(const_iterator(this, m_size > 0 ? m_size - 1 : m_size)); }
				const_reverse_iterator CLOAK_CALL_THIS crbegin() const { return const_reverse_iterator(const_iterator(this, m_size > 0 ? m_size - 1 : m_size)); }

				iterator CLOAK_CALL_THIS end() { return iterator(this, m_size); }
				const_iterator CLOAK_CALL_THIS end() const { return const_iterator(this, m_size); }
				const_iterator CLOAK_CALL_THIS cend() const { return const_iterator(this, m_size); }
				reverse_iterator CLOAK_CALL_THIS rend() { return reverse_iterator(iterator(this, m_size)); }
				const_reverse_iterator CLOAK_CALL_THIS rend() const { return const_reverse_iterator(const_iterator(this, m_size)); }
				const_reverse_iterator CLOAK_CALL_THIS crend() const { return const_reverse_iterator(const_iterator(this, m_size)); }
		};
		template<typename T, size_t D = 2, typename Compare = std::less<T>> class PagedHeap {
			public:
				typedef Compare key_compare;
				typedef T value_type;
				typedef T* pointer;
				typedef const T* const_pointer;
				typedef T& reference;
				typedef const T& const_reference;
				typedef size_t size_type;
			private:
				struct Page {
					Page* Prev;
					Page* Next;
					size_type Offset;
					size_type Pos;
					inline T& CLOAK_CALL_THIS get(In size_t i)
					{
						CLOAK_ASSUME(i >= Offset && i < Pos + Offset);
						T* d = reinterpret_cast<uintptr_t>(this) + sizeof(Page);
						return d[i - Offset];
					}
					inline const T& CLOAK_CALL_THIS get(In size_t i) const
					{
						CLOAK_ASSUME(i >= Offset && i < Pos + Offset);
						T* d = reinterpret_cast<uintptr_t>(this) + sizeof(Page);
						return d[i - Offset];
					}
				};
				static constexpr size_t PAGE_MAX_SIZE = 2 MB;
				static constexpr size_t PAGE_HEAD_SIZE = (sizeof(Page) + std::alignment_of<T>::value - 1) & ~(std::alignment_of<T>::value - 1);
				static constexpr size_t PAGE_DATA_COUNT = (PAGE_MAX_SIZE - PAGE_HEAD_SIZE) / sizeof(T);
				static constexpr size_t PAGE_SIZE = PAGE_HEAD_SIZE + (sizeof(T)*PAGE_DATA_COUNT);

				Global::Memory::PageStackAllocator* const m_alloc;
				size_type m_size;
				Page* m_first;
				Page* m_last;
				key_compare m_kc;
			private:
				Page* CLOAK_CALL_THIS AllocatePage(In Page* last)
				{
					Page* p = reinterpret_cast<Page*>(m_alloc->Allocate(PAGE_SIZE));
					p->Prev = last;
					p->Pos = 0;
					if (last != nullptr) { last->Next = p; }
					return p;
				}
				inline void CLOAK_CALL_THIS restoreUp(In size_t i)
				{
					if (i == 0) { return; }
					size_t p = (i - 1) / D;
					Page* cp = m_last;
					while (cp->Offset > i) { cp = cp->Prev; }
					Page* np = cp;
					while (np->Offset > p) { np = np->Prev; }
					if (m_kc(cp->get(i), np->get(p)) == true)
					{
						value_type tmp = std::move(cp->get(i));
						do {
							cp->get(i) = std::move(np->get(p));
							i = p;
							cp = np;
							if (i == 0)
							{
								CLOAK_ASSUME(cp == m_first);
								cp->get(i) = std::move(tmp);
								return;
							}
							else 
							{ 
								p = (i - 1) / D; 
								while (np->Offset > p) { np = np->Prev; }
							}
						} while (m_kc(tmp, np->get(p)) == true);
						cp->get(i) = std::move(tmp);
					}
				}
				inline void CLOAK_CALL_THIS restoreDown(In size_t i)
				{
					if (i >= m_size) { return; }
					size_t mc = m_size;
					Page* cp = m_first;
					while (cp->Offset + cp->Pos <= i) { cp = cp->Next; }
					Page* np = cp;
					Page* lp = np;
					for (size_t a = 0; a < D; a++)
					{
						const size_t child = (D*i) + a + 1;
						if (child < m_size)
						{
							while (np->Offset + np->Pos <= child) { np = np->Next; }
							if (mc == m_size || m_kc(np->get(child), lp->get(mc)) == true) 
							{ 
								mc = child; 
								lp = np;
							}
						}
					}
					if (mc < m_size && m_kc(lp->get(mc), cp->get(i)) == true)
					{
						value_type tmp = std::move(cp->get(i));
						do {
							cp->get(i) = std::move(lp->get(mc));
							i = mc;
							cp = lp;
							np = lp;
							mc = m_size;
							for (size_t a = 0; a < D; a++)
							{
								const size_t child = (D*i) + a + 1;
								if (child < m_size)
								{
									while (np->Offset + np->Pos <= child) { np = np->Next; }
									if (mc == m_size || m_kc(np->get(child), lp->get(mc)) == true)
									{
										mc = child;
										lp = np;
									}
								}
							}
						} while (mc < m_size && m_kc(lp->get(mc), tmp) == true);
						cp->get(i) = std::move(tmp);
					}
				}
			public:
				bool CLOAK_CALL_THIS empty() const { return m_size == 0; }
				size_type CLOAK_CALL_THIS size() const { return m_size; }

				void CLOAK_CALL_THIS push(In const T& t)
				{
					if (m_last == nullptr || m_last->Pos >= PAGE_DATA_COUNT) 
					{
						m_last = AllocatePage(m_last);
						if (m_first == nullptr) { m_first = m_last; }
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_last) + PAGE_HEAD_SIZE);
					new(&d[m_last->Pos++])T(t);
					m_size++;
					restoreUp(m_size - 1);
				}
				void CLOAK_CALL_THIS push(In T&& t)
				{
					if (m_last == nullptr || m_last->Pos >= PAGE_DATA_COUNT)
					{
						m_last = AllocatePage(m_last);
						if (m_first == nullptr) { m_first = m_last; }
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_last) + PAGE_HEAD_SIZE);
					new(&d[m_last->Pos++])T(std::move(t));
					m_size++;
					restoreUp(m_size - 1);
				}
				template<typename... Args> void CLOAK_CALL_THIS emplace(In Args&&... args)
				{
					if (m_last == nullptr || m_last->Pos >= PAGE_DATA_COUNT)
					{
						m_last = AllocatePage(m_last);
						if (m_first == nullptr) { m_first = m_last; }
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_last) + PAGE_HEAD_SIZE);
					new(&d[m_last->Pos++])T(std::forward<Args>(args)...);
					m_size++;
					restoreUp(m_size - 1);
				}

				template<typename I> void CLOAK_CALL_THIS insert(In I first, In I last)
				{
					for (; first != last; ++first) { push(*first); }
				}

				void CLOAK_CALL_THIS pop()
				{
					CLOAK_ASSUME(m_size > 0 && m_first != nullptr && m_last != nullptr);
					m_first->get(0) = std::move(m_last->get(m_last->Pos - 1));
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_last) + PAGE_HEAD_SIZE);
					d[--m_last->Pos].~T();
					m_size--;
					if (m_last->Pos == 0)
					{
						Page* p = m_last;
						m_last = m_last->Prev;
						m_alloc->Free(p);
					}
					restoreDown(0);
				}

				void CLOAK_CALL_THIS clear()
				{
					Page* p = m_last;
					m_first = nullptr;
					m_last = nullptr;
					m_size = 0;
					while (p != nullptr)
					{
						T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) + PAGE_HEAD_SIZE);
						for (size_type a = 0; a < p->Pos; a++) { d[a].~T(); }
						Page* lp = p;
						p = p->Prev;
						m_alloc->Free(lp);
					}
				}

				value_type& CLOAK_CALL_THIS front() { CLOAK_ASSUME(m_first != nullptr); return m_first->get(0); }
				const value_type& CLOAK_CALL_THIS front() const { CLOAK_ASSUME(m_first != nullptr); return m_first->get(0); }

				CLOAK_CALL PagedHeap(In Global::Memory::PageStackAllocator* allocator) : m_alloc(allocator), m_size(0), m_first(nullptr), m_last(nullptr), m_kc(key_compare()) {}
				CLOAK_CALL PagedHeap(In Global::Memory::PageStackAllocator* allocator, In const key_compare& compare) : m_alloc(allocator), m_size(0), m_first(nullptr), m_last(nullptr), m_kc(compare) {}
				template<typename I> CLOAK_CALL PagedHeap(In Global::Memory::PageStackAllocator* allocator, In I first, In I last) : m_alloc(allocator), m_size(0), m_first(nullptr), m_last(nullptr), m_kc(key_compare()) { insert(first, last); }
				template<typename I> CLOAK_CALL PagedHeap(In Global::Memory::PageStackAllocator* allocator, In I first, In I last, In const key_compare& compare) : m_alloc(allocator), m_size(0), m_first(nullptr), m_last(nullptr), m_kc(compare) { insert(first, last); }
				CLOAK_CALL PagedHeap(In const PagedHeap<T,D,Compare>& o) : m_alloc(o.m_alloc), m_size(o.m_size), m_first(nullptr), m_last(nullptr), m_kc(o.m_kc)
				{
					Page* op = o.m_first;
					Page* lp = nullptr;
					while (op != nullptr)
					{
						Page* np = AllocatePage(lp);
						if (m_first != nullptr) { m_first = np; }
						T* od = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(op) + PAGE_HEAD_SIZE);
						T* nd = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(np) + PAGE_HEAD_SIZE);
						for (size_type a = 0; a < op->Pos; a++)
						{
							new(&nd[a])T(od[a]);
						}
						np->Pos = op->Pos;
						lp = np;
						op = op->Next;
					}
					m_last = lp;
				}
				CLOAK_CALL ~PagedHeap() { clear(); }

				PagedHeap<T, D, Compare>& CLOAK_CALL_THIS operator=(In const PagedHeap<T, D, Compare>& p)
				{
					if (&p != this)
					{
						clear();
						m_size = p.m_size;
						Page* op = p.m_first;
						Page* lp = nullptr;
						m_first = nullptr;
						while (op != nullptr)
						{
							Page* np = AllocatePage(lp);
							if (m_first != nullptr) { m_first = np; }
							T* od = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(op) + PAGE_HEAD_SIZE);
							T* nd = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(np) + PAGE_HEAD_SIZE);
							for (size_type a = 0; a < op->Pos; a++)
							{
								new(&nd[a])T(od[a]);
							}
							np->Pos = op->Pos;
							lp = np;
							op = op->Next;
						}
						m_last = lp;
					}
					return *this;
				}
		};
		template<typename T> class PagedStack {
			public:
				typedef T value_type;
				typedef T* pointer;
				typedef const T* const_pointer;
				typedef T& reference;
				typedef const T& const_reference;
				typedef size_t size_type;
			private:
				struct Page {
					Page* Next;
					size_type Pos;
				};
				static constexpr size_t PAGE_MAX_SIZE = 2 MB;
				static constexpr size_t PAGE_HEAD_SIZE = (sizeof(Page) + std::alignment_of<T>::value - 1) & ~(std::alignment_of<T>::value - 1);
				static constexpr size_t PAGE_DATA_COUNT = (PAGE_MAX_SIZE - PAGE_HEAD_SIZE) / sizeof(T);
				static constexpr size_t PAGE_SIZE = PAGE_HEAD_SIZE + (sizeof(T)*PAGE_DATA_COUNT);

				Global::Memory::PageStackAllocator* const m_alloc;
				size_type m_size;
				Page* m_curPage;

				Page* CLOAK_CALL_THIS AllocatePage(In Page* last)
				{
					Page* p = reinterpret_cast<Page*>(m_alloc->Allocate(PAGE_SIZE));
					p->Next = last;
					p->Pos = 0;
					return p;
				}
			public:
				class iterator;
				class const_iterator;
				class iterator {
					friend class PagedStack;
					friend class const_iterator;
					public:
						typedef std::forward_iterator_tag iterator_category;
						typedef T value_type;
						typedef ptrdiff_t difference_type;
						typedef value_type* pointer;
						typedef value_type& reference;
					private:
						Page * m_page;
						size_type m_pos;

						CLOAK_CALL iterator(In Page* p, In size_type pos) : m_page(p), m_pos(pos) {}
					public:
						CLOAK_CALL iterator(In_opt nullptr_t = nullptr) : m_page(nullptr), m_pos(0) {}
						CLOAK_CALL iterator(In const iterator& o) : m_page(o.m_page), m_pos(o.m_pos) {}

						CLOAK_CALL_THIS operator const_iterator() { return const_iterator(m_page, m_pos); }
						iterator& CLOAK_CALL_THIS operator=(In const iterator& o) { m_page = o.m_page; m_pos = o.m_pos; return *this; }
						iterator CLOAK_CALL_THIS operator++(In int)
						{
							iterator r = *this;
							if (m_pos > 0) { m_pos--; }
							else if (m_page != nullptr)
							{
								m_page = m_page->Next;
								if (m_page != nullptr) { m_pos = m_page->Pos - 1; }
								else { m_pos = 0; }
							}
							return r;
						}
						iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_pos > 0) { m_pos--; }
							else if (m_page != nullptr) 
							{ 
								m_page = m_page->Next; 
								if (m_page != nullptr) { m_pos = m_page->Pos - 1; }
								else { m_pos = 0; }
							}
							return *this;
						}
						reference CLOAK_CALL_THIS operator*() const
						{
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return d[m_pos];
						}
						pointer CLOAK_CALL_THIS operator->() const
						{
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return &d[m_pos];
						}
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_page == o.m_page && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_page != o.m_page || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_page == o.m_page && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_page != o.m_page || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In nullptr_t) const { return m_page == nullptr && m_pos == 0; }
						bool CLOAK_CALL_THIS operator!=(In nullptr_t) const { return m_page != nullptr || m_pos != 0; }
				};
				class const_iterator {
					friend class PagedStack;
					friend class iterator;
					public:
						typedef std::forward_iterator_tag iterator_category;
						typedef T value_type;
						typedef ptrdiff_t difference_type;
						typedef const value_type* pointer;
						typedef const value_type& reference;
					private:
						const Page* m_page;
						size_type m_pos;

						CLOAK_CALL const_iterator(In const Page* page, In size_type pos) : m_page(p), m_pos(pos) {}
					public:
						CLOAK_CALL const_iterator(In_opt nullptr_t = nullptr) : m_page(nullptr), m_pos(0) {}
						CLOAK_CALL const_iterator(In const const_iterator& o) : m_page(o.m_page), m_pos(o.m_pos) {}
						CLOAK_CALL const_iterator(In const iterator& o) : m_page(o.m_page), m_pos(o.m_pos) {}

						const_iterator& CLOAK_CALL_THIS operator=(In const iterator& o) { m_page = o.m_page; m_pos = o.m_pos; return *this; }
						const_iterator& CLOAK_CALL_THIS operator=(In const const_iterator& o) { m_page = o.m_page; m_pos = o.m_pos; return *this; }
						const_iterator CLOAK_CALL_THIS operator++(In int)
						{
							const_iterator r = *this;
							if (m_pos > 0) { m_pos--; }
							else if (m_page != nullptr)
							{
								m_page = m_page->Next;
								if (m_page != nullptr) { m_pos = m_page->Pos - 1; }
								else { m_pos = 0; }
							}
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_pos > 0) { m_pos--; }
							else if (m_page != nullptr)
							{
								m_page = m_page->Next;
								if (m_page != nullptr) { m_pos = m_page->Pos - 1; }
								else { m_pos = 0; }
							}
							return *this;
						}
						const_reference CLOAK_CALL_THIS operator*() const
						{
							const T* d = reinterpret_cast<const T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return d[m_pos];
						}
						const_pointer CLOAK_CALL_THIS operator->() const
						{
							const T* d = reinterpret_cast<const T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return &d[m_pos];
						}
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_page == o.m_page && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_page != o.m_page || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_page == o.m_page && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_page != o.m_page || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In nullptr_t) const { return m_page == nullptr && m_pos == 0; }
						bool CLOAK_CALL_THIS operator!=(In nullptr_t) const { return m_page != nullptr || m_pos != 0; }
				};

				CLOAK_CALL PagedStack(In Global::Memory::PageStackAllocator* allocator) : m_alloc(allocator), m_size(0), m_curPage(nullptr) {}
				CLOAK_CALL PagedStack(In const PagedStack<T>& p) : m_alloc(p.m_alloc), m_size(p.m_size)
				{
					Page* op = p.m_curPage;
					Page* lp = nullptr;
					while (op != nullptr)
					{
						Page* np = AllocatePage(lp);
						T* od = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(op) + PAGE_HEAD_SIZE);
						T* nd = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(np) + PAGE_HEAD_SIZE);
						for (size_type a = 0; a < op->Pos; a++)
						{
							new(&nd[a])T(od[a]);
						}
						np->Pos = op->Pos;
						lp = np;
						op = op->Next;
					}
					m_curPage = lp;
				}
				CLOAK_CALL ~PagedStack() { clear(); }

				PagedStack<T>& CLOAK_CALL_THIS operator=(In const PagedStack<T>& p)
				{
					if (&p != this)
					{
						clear();
						m_size = p.m_size;
						Page* op = p.m_curPage;
						Page* lp = nullptr;
						while (op != nullptr)
						{
							Page* np = AllocatePage(lp);
							T* od = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(op) + PAGE_HEAD_SIZE);
							T* nd = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(np) + PAGE_HEAD_SIZE);
							for (size_type a = 0; a < op->Pos; a++)
							{
								new(&nd[a])T(od[a]);
							}
							np->Pos = op->Pos;
							lp = np;
							op = op->Next;
						}
						m_curPage = lp;
					}
					return *this;
				}
				void CLOAK_CALL_THIS push(In const T& t)
				{
					if (m_curPage == nullptr || m_curPage->Pos >= PAGE_DATA_COUNT) { m_curPage = AllocatePage(m_curPage); }
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_curPage) + PAGE_HEAD_SIZE);
					new(&d[m_curPage->Pos++])T(t);
					m_size++;
				}
				void CLOAK_CALL_THIS push(In T&& t)
				{
					if (m_curPage == nullptr || m_curPage->Pos >= PAGE_DATA_COUNT) { m_curPage = AllocatePage(m_curPage); }
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_curPage) + PAGE_HEAD_SIZE);
					new(&d[m_curPage->Pos++])T(std::move(t));
					m_size++;
				}
				template<typename... Args> void CLOAK_CALL_THIS emplace(In Args&&... args)
				{
					if (m_curPage == nullptr || m_curPage->Pos >= PAGE_DATA_COUNT) { m_curPage = AllocatePage(m_curPage); }
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_curPage) + PAGE_HEAD_SIZE);
					new(&d[m_curPage->Pos++])T(std::forward<Args>(args)...);
					m_size++;
				}
				void CLOAK_CALL_THIS pop()
				{
					if (m_curPage != nullptr)
					{
						T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_curPage) + PAGE_HEAD_SIZE);
						d[--m_curPage->Pos].~T();
						m_size--;
						if (m_curPage->Pos == 0)
						{
							Page* p = m_curPage;
							m_curPage = m_curPage->Next;
							m_alloc->Free(p);
						}
					}
				}
				void CLOAK_CALL_THIS clear()
				{
					Page* p = m_curPage;
					m_curPage = nullptr;
					m_size = 0;
					while (p != nullptr)
					{
						T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) + PAGE_HEAD_SIZE);
						for (size_type a = 0; a < p->Pos; a++) { d[a].~T(); }
						Page* lp = p;
						p = p->Next;
						m_alloc->Free(lp);
					}
				}
				size_type CLOAK_CALL_THIS size() const { return m_size; }
				bool CLOAK_CALL_THIS empty() const { return m_size == 0; }
				reference CLOAK_CALL_THIS top()
				{
					assert(m_size > 0);
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_curPage) + PAGE_HEAD_SIZE);
					return d[m_curPage->Pos - 1];
				}
				const_reference CLOAK_CALL_THIS top() const
				{
					assert(m_size > 0);
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_curPage) + PAGE_HEAD_SIZE);
					return d[m_curPage->Pos - 1];
				}

				iterator CLOAK_CALL_THIS begin()
				{
					if (m_curPage == nullptr) { return iterator(nullptr); }
					return iterator(m_curPage, m_curPage->Pos - 1);
				}
				const_iterator CLOAK_CALL_THIS begin() const
				{
					if (m_curPage == nullptr) { return const_iterator(nullptr); }
					return const_iterator(m_curPage, m_curPage->Pos - 1);
				}
				const_iterator CLOAK_CALL_THIS cbegin() const
				{
					if (m_curPage == nullptr) { return const_iterator(nullptr); }
					return const_iterator(m_curPage, m_curPage->Pos - 1);
				}
				iterator CLOAK_CALL_THIS end() { return iterator(nullptr); }
				const_iterator CLOAK_CALL_THIS end() const { return const_iterator(nullptr); }
				const_iterator CLOAK_CALL_THIS cend() const { return const_iterator(nullptr); }
		};
		template<typename T> class PagedQueue {
			public:
				typedef T value_type;
				typedef T* pointer;
				typedef const T* const_pointer;
				typedef T& reference;
				typedef const T& const_reference;
				typedef size_t size_type;
			private:
				struct Page {
					Page* Next;
					size_type Pos;
				};
				static constexpr size_t PAGE_MAX_SIZE = 256 KB;
				static constexpr size_t PAGE_HEAD_SIZE = (sizeof(Page) + std::alignment_of<T>::value - 1) & ~(std::alignment_of<T>::value - 1);
				static constexpr size_t PAGE_DATA_COUNT = (PAGE_MAX_SIZE - PAGE_HEAD_SIZE) / sizeof(T);
				static constexpr size_t PAGE_SIZE = PAGE_HEAD_SIZE + (sizeof(T)*PAGE_DATA_COUNT);

				Global::Memory::PageStackAllocator* const m_alloc;
				size_type m_size;
				Page* m_firstPage;
				Page* m_lastPage;
				Page* m_unusedPages;
				size_t m_pageOffset;

				Page* CLOAK_CALL_THIS AllocatePage()
				{
					Page* p = m_unusedPages;
					if (p != nullptr)
					{
						m_unusedPages = p->Next;
					}
					else
					{
						p = reinterpret_cast<Page*>(m_alloc->Allocate(PAGE_SIZE));
					}
					p->Next = nullptr;
					p->Pos = 0;
					return p;
				}
			public:
				class iterator;
				class const_iterator;
				class iterator {
					friend class PagedQueue;
					friend class const_iterator;
					public:
						typedef std::forward_iterator_tag iterator_category;
						typedef T value_type;
						typedef ptrdiff_t difference_type;
						typedef value_type* pointer;
						typedef value_type& reference;
					private:
						Page * m_page;
						size_type m_pos;

						CLOAK_CALL iterator(In Page* p, In size_type pos) : m_page(p), m_pos(pos) {}
					public:
						CLOAK_CALL iterator(In_opt nullptr_t = nullptr) : m_page(nullptr), m_pos(0) {}
						CLOAK_CALL iterator(In const iterator& o) : m_page(o.m_page), m_pos(o.m_pos) {}

						CLOAK_CALL_THIS operator const_iterator() { return const_iterator(m_page, m_pos); }
						iterator& CLOAK_CALL_THIS operator=(In const iterator& o) { m_page = o.m_page; m_pos = o.m_pos; return *this; }
						iterator CLOAK_CALL_THIS operator++(In int)
						{
							iterator r = *this;
							if (m_page != nullptr)
							{
								if (m_pos + 1 == m_page->Pos) { m_pos = 0; m_page = m_page->Next; }
								else { m_pos++; }
							}
							return r;
						}
						iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_page != nullptr)
							{
								if (m_pos + 1 == m_page->Pos) { m_pos = 0; m_page = m_page->Next; }
								else { m_pos++; }
							}
							return *this;
						}
						reference CLOAK_CALL_THIS operator*() const
						{
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return d[m_pos];
						}
						pointer CLOAK_CALL_THIS operator->() const
						{
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return &d[m_pos];
						}
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_page == o.m_page && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_page != o.m_page || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_page == o.m_page && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_page != o.m_page || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In nullptr_t) const { return m_page == nullptr && m_pos == 0; }
						bool CLOAK_CALL_THIS operator!=(In nullptr_t) const { return m_page != nullptr || m_pos != 0; }
				};
				class const_iterator {
					friend class PagedQueue;
					friend class iterator;
					public:
						typedef std::forward_iterator_tag iterator_category;
						typedef T value_type;
						typedef ptrdiff_t difference_type;
						typedef const value_type* pointer;
						typedef const value_type& reference;
					private:
						Page * m_page;
						size_type m_pos;

						CLOAK_CALL const_iterator(In Page* p, In size_type pos) : m_page(p), m_pos(pos) {}
					public:
						CLOAK_CALL const_iterator(In_opt nullptr_t = nullptr) : m_page(nullptr), m_pos(0) {}
						CLOAK_CALL const_iterator(In const iterator& o) : m_page(o.m_page), m_pos(o.m_pos) {}
						CLOAK_CALL const_iterator(In const const_iterator& o) : m_page(o.m_page), m_pos(o.m_pos) {}

						const_iterator& CLOAK_CALL_THIS operator=(In const iterator& o) { m_page = o.m_page; m_pos = o.m_pos; return *this; }
						const_iterator& CLOAK_CALL_THIS operator=(In const const_iterator& o) { m_page = o.m_page; m_pos = o.m_pos; return *this; }
						const_iterator CLOAK_CALL_THIS operator++(In int)
						{
							const_iterator r = *this;
							if (m_page != nullptr)
							{
								if (m_pos + 1 == m_page->Pos) { m_pos = 0; m_page = m_page->Next; }
								else { m_pos++; }
							}
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator++()
						{
							if (m_page != nullptr)
							{
								if (m_pos + 1 == m_page->Pos) { m_pos = 0; m_page = m_page->Next; }
								else { m_pos++; }
							}
							return *this;
						}
						const_reference CLOAK_CALL_THIS operator*() const
						{
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return d[m_pos];
						}
						const_pointer CLOAK_CALL_THIS operator->() const
						{
							T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_page) + PAGE_HEAD_SIZE);
							return &d[m_pos];
						}
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_page == o.m_page && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_page != o.m_page || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_page == o.m_page && m_pos == o.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_page != o.m_page || m_pos != o.m_pos; }
						bool CLOAK_CALL_THIS operator==(In nullptr_t) const { return m_page == nullptr && m_pos == 0; }
						bool CLOAK_CALL_THIS operator!=(In nullptr_t) const { return m_page != nullptr || m_pos != 0; }
				};

				CLOAK_CALL PagedQueue(In Global::Memory::PageStackAllocator* allocator) : m_alloc(allocator), m_size(0), m_firstPage(nullptr), m_lastPage(nullptr), m_unusedPages(nullptr), m_pageOffset(0) {}
				CLOAK_CALL PagedQueue(In const PagedQueue<T>& p) : m_alloc(p.m_alloc), m_size(p.m_size)
				{
					m_pageOffset = p.m_pageOffset;
					Page* op = p.m_firstPage;
					Page* lp = nullptr;
					while (op != nullptr)
					{
						Page* np = AllocatePage();
						if (lp == nullptr) { m_firstPage = np; }
						else { lp->Next = np; }
						T* on = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(op) + PAGE_HEAD_SIZE);
						T* nd = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(np) + PAGE_HEAD_SIZE);
						for (size_type a = 0; a < op->Pos; a++)
						{
							new(&nd[a])T(od[a]);
						}
						lp = np;
						op = op->Next;
					}
					m_lastPage = lp;
				}
				CLOAK_CALL ~PagedQueue() { clear(); }
				PagedQueue<T>& CLOAK_CALL_THIS operator=(In const PagedQueue<T>& p)
				{
					if (&p != this)
					{
						clear();
						m_size = p.m_size;
						m_pageOffset = p.m_pageOffset;
						Page* op = p.m_firstPage;
						Page* lp = nullptr;
						while (op != nullptr)
						{
							Page* np = AllocatePage();
							if (lp == nullptr) { m_firstPage = np; }
							else { lp->Next = np; }
							T* on = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(op) + PAGE_HEAD_SIZE);
							T* nd = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(np) + PAGE_HEAD_SIZE);
							for (size_type a = 0; a < op->Pos; a++)
							{
								new(&nd[a])T(od[a]);
							}
							lp = np;
							op = op->Next;
						}
						m_lastPage = lp;
					}
					return *this;
				}
				void CLOAK_CALL_THIS push(In const T& t)
				{
					if (m_lastPage == nullptr || m_lastPage->Pos >= PAGE_DATA_COUNT) 
					{ 
						Page* np = AllocatePage(); 
						if (m_lastPage != nullptr) { m_lastPage->Next = np; }
						if (m_firstPage == nullptr) { m_firstPage = np; }
						m_lastPage = np;
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_lastPage) + PAGE_HEAD_SIZE);
					new(&d[m_lastPage->Pos++])T(t);
					m_size++;
				}
				void CLOAK_CALL_THIS push(In T&& t)
				{
					if (m_lastPage == nullptr || m_lastPage->Pos >= PAGE_DATA_COUNT)
					{
						Page* np = AllocatePage();
						if (m_lastPage != nullptr) { m_lastPage->Next = np; }
						if (m_firstPage == nullptr) { m_firstPage = np; }
						m_lastPage = np;
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_lastPage) + PAGE_HEAD_SIZE);
					new(&d[m_lastPage->Pos++])T(t);
					m_size++;
				}
				template<typename... Args> void CLOAK_CALL_THIS emplace(In Args&&... args)
				{
					if (m_lastPage == nullptr || m_lastPage->Pos >= PAGE_DATA_COUNT)
					{
						Page* np = AllocatePage();
						if (m_lastPage != nullptr) { m_lastPage->Next = np; }
						if (m_firstPage == nullptr) { m_firstPage = np; }
						m_lastPage = np;
					}
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_lastPage) + PAGE_HEAD_SIZE);
					new(&d[m_lastPage->Pos++])T(std::forward<Args>(args)...);
					m_size++;
				}
				void CLOAK_CALL_THIS pop()
				{
					if (m_firstPage != nullptr && m_pageOffset < m_firstPage->Pos)
					{
						T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_firstPage) + PAGE_HEAD_SIZE);
						d[m_pageOffset++].~T();
						m_size--;
						if (m_pageOffset == m_firstPage->Pos)
						{
							m_pageOffset = 0;
							Page* p = m_firstPage;
							m_firstPage = m_firstPage->Next;
							p->Next = m_unusedPages;
							m_unusedPages = p;
						}
					}
				}
				void CLOAK_CALL_THIS clear()
				{
					Page* p = m_firstPage;
					while (p != nullptr)
					{
						T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) + PAGE_HEAD_SIZE);
						for (size_type a = 0; a < p->Pos; a++) { d[a].~T(); }
						Page* lp = p;
						p = p->Next;
						lp->Next = m_unusedPages;
						m_unusedPages = lp;
					}
					m_firstPage = nullptr;
					m_lastPage = nullptr;
					m_size = 0;
					m_pageOffset = 0;
				}
				size_type CLOAK_CALL_THIS size() const { return m_size; }
				bool CLOAK_CALL_THIS empty()const { return m_size == 0; }
				reference CLOAK_CALL_THIS front()
				{
					assert(m_size > 0);
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_firstPage) + PAGE_HEAD_SIZE);
					return d[m_pageOffset];
				}
				const_reference CLOAK_CALL_THIS front() const
				{
					assert(m_size > 0);
					T* d = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(m_firstPage) + PAGE_HEAD_SIZE);
					return d[m_pageOffset];
				}
				iterator CLOAK_CALL_THIS begin()
				{
					return iterator(m_firstPage, m_pageOffset);
				}
				const_iterator CLOAK_CALL_THIS begin() const
				{
					return const_iterator(m_firstPage, m_pageOffset);
				}
				const_iterator CLOAK_CALL_THIS cbegin() const
				{
					return const_iterator(m_firstPage, m_pageOffset);
				}
				iterator CLOAK_CALL_THIS end() { return iterator(nullptr); }
				const_iterator CLOAK_CALL_THIS end() const { return const_iterator(nullptr); }
				const_iterator CLOAK_CALL_THIS cend() const { return const_iterator(nullptr); }
		};
#endif
		template<typename T, typename D = List<T>> class Stack : public std::stack<T, D>
		{
			using std::stack<T, D>::stack;
			public:
				typedef typename D::iterator iterator;
				typedef typename D::const_iterator const_iterator;
				typedef typename D::reverse_iterator reverse_iterator;
				typedef typename D::const_reverse_iterator const_reverse_iterator;

				iterator CLOAK_CALL_THIS begin() { return c.begin(); }
				iterator CLOAK_CALL_THIS end() { return c.end(); }
				const_iterator CLOAK_CALL_THIS begin() const { return c.begin(); }
				const_iterator CLOAK_CALL_THIS end() const { return c.end(); }
				const_iterator CLOAK_CALL_THIS cbegin() const { return c.begin(); }
				const_iterator CLOAK_CALL_THIS cend() const { return c.end(); }
				reverse_iterator CLOAK_CALL_THIS rbegin() { return c.rbegin(); }
				reverse_iterator CLOAK_CALL_THIS rend() { return c.rend(); }
				const_reverse_iterator CLOAK_CALL_THIS crbegin() const { return c.crbegin(); }
				const_reverse_iterator CLOAK_CALL_THIS crend() const { return c.crend(); }

				void CLOAK_CALL_THIS clear() { while (size() > 0) { pop(); } }
				void* CLOAK_CALL operator new(In size_t s) { return Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { Global::Memory::MemoryHeap::Free(ptr); }
		};
		template<typename A> class RefPointer {
			private:
				A* m_ptr;
			public:
				CLOAK_CALL RefPointer() : m_ptr(nullptr) {}
				CLOAK_CALL RefPointer(In A* ptr) : m_ptr(ptr) { if (ptr != nullptr) { ptr->AddRef(); } }
				CLOAK_CALL RefPointer(In const RefPointer<A>& o) : RefPointer(o.m_ptr) {}
				CLOAK_CALL RefPointer(In RefPointer<A>&& o) : m_ptr(o.m_ptr) { o.m_ptr = nullptr; }
				CLOAK_CALL ~RefPointer() { if (m_ptr != nullptr) { m_ptr->Release(); } }

				//Access
				A* CLOAK_CALL_THIS Get() const { return m_ptr; }
				template<typename B, std::enable_if_t<CloakEngine::Predicate::is_safely_castable_v<A*, B*>>* = nullptr> inline CLOAK_CALL_THIS operator RefPointer<B>() const
				{
					return RefPointer<B>(static_cast<B*>(m_ptr));
				}
				inline CLOAK_CALL_THIS operator A*() const
				{
					if (m_ptr != nullptr) { m_ptr->AddRef(); }
					return m_ptr;
				}
				inline A* CLOAK_CALL_THIS operator->() const { return m_ptr; }
				inline A** CLOAK_CALL_THIS operator&() { CLOAK_ASSUME(m_ptr == nullptr); return &m_ptr; }

				//Compare
				inline bool CLOAK_CALL_THIS operator==(In nullptr_t) const { return m_ptr == nullptr; }
				inline bool CLOAK_CALL_THIS operator!=(In nullptr_t) const { return m_ptr != nullptr; }
				inline bool CLOAK_CALL_THIS operator==(In A* ptr) const { return m_ptr == ptr; }
				inline bool CLOAK_CALL_THIS operator!=(In A* ptr) const { return m_ptr != ptr; }
				inline bool CLOAK_CALL_THIS operator==(In const A* ptr) const { return m_ptr == ptr; }
				inline bool CLOAK_CALL_THIS operator!=(In const A* ptr) const { return m_ptr != ptr; }
				inline bool CLOAK_CALL_THIS operator==(In volatile A* ptr) const { return m_ptr == ptr; }
				inline bool CLOAK_CALL_THIS operator!=(In volatile A* ptr) const { return m_ptr != ptr; }
				inline bool CLOAK_CALL_THIS operator==(In const volatile A* ptr) const { return m_ptr == ptr; }
				inline bool CLOAK_CALL_THIS operator!=(In const volatile A* ptr) const { return m_ptr != ptr; }
				inline bool CLOAK_CALL_THIS operator==(In const RefPointer& o) const { return m_ptr == o.m_ptr; }
				inline bool CLOAK_CALL_THIS operator!=(In const RefPointer& o) const { return m_ptr != o.m_ptr; }

				friend inline bool CLOAK_CALL operator==(In nullptr_t, In const RefPointer& o) { return o == nullptr; }
				friend inline bool CLOAK_CALL operator!=(In nullptr_t, In const RefPointer& o) { return o != nullptr; }
				friend inline bool CLOAK_CALL operator==(In A* ptr, In const RefPointer& o) { return o == ptr; }
				friend inline bool CLOAK_CALL operator!=(In A* ptr, In const RefPointer& o) { return o != ptr; }
				friend inline bool CLOAK_CALL operator==(In const A* ptr, In const RefPointer& o) { return o == ptr; }
				friend inline bool CLOAK_CALL operator!=(In const A* ptr, In const RefPointer& o) { return o != ptr; }
				friend inline bool CLOAK_CALL operator==(In volatile A* ptr, In const RefPointer& o) { return o == ptr; }
				friend inline bool CLOAK_CALL operator!=(In volatile A* ptr, In const RefPointer& o) { return o != ptr; }
				friend inline bool CLOAK_CALL operator==(In const volatile A* ptr, In const RefPointer& o) { return o == ptr; }
				friend inline bool CLOAK_CALL operator!=(In const volatile A* ptr, In const RefPointer& o) { return o != ptr; }

				//Set
				void CLOAK_CALL_THIS Free() 
				{ 
					if (m_ptr != nullptr) { m_ptr->Release(); } 
					m_ptr = nullptr;
				}
				RefPointer& CLOAK_CALL_THIS operator=(In nullptr_t)
				{
					if (m_ptr != nullptr) { m_ptr->Release(); }
					m_ptr = nullptr;
					return *this;
				}
				RefPointer& CLOAK_CALL_THIS operator=(In A* ptr)
				{
					if (ptr != nullptr) { ptr->AddRef(); }
					if (m_ptr != nullptr) { m_ptr->Release(); }
					m_ptr = ptr;
					return *this;
				}
				RefPointer& CLOAK_CALL_THIS operator=(In const RefPointer<A>& o)
				{
					if (o.m_ptr != nullptr) { o.m_ptr->AddRef(); }
					if (m_ptr != nullptr) { m_ptr->Release(); }
					m_ptr = o.m_ptr;
					return *this;
				}
				RefPointer& CLOAK_CALL_THIS operator=(In RefPointer<A>&& o)
				{
					if (m_ptr != nullptr) { m_ptr->Release(); }
					m_ptr = o.m_ptr;
					o.m_ptr = nullptr;
					return *this;
				}

				//Utility:
				template<typename T, typename... Args> static RefPointer CLOAK_CALL_THIS Construct(In Args&& ... args)
				{ 
					RefPointer r = nullptr;
					r.m_ptr = new T(std::forward<Args>(args)...);
					return r;
				}
				template<typename T, typename... Args> static RefPointer CLOAK_CALL_THIS ConstructAt(In void* ptr, In Args&& ... args)
				{ 
					RefPointer r = nullptr;
					r.m_ptr = ::new(ptr)T(std::forward<Args>(args)...);
					return r;
				}
		};
	}
}

namespace std {
	typedef basic_stringstream<char16_t> u16stringstream;
	typedef basic_stringstream<char32_t> u32stringstream;
}

#endif