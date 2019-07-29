#pragma once
#ifndef CE_API_GLOBAL_CONTAINER_LIST_H
#define CE_API_GLOBAL_CONTAINER_LIST_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Container/Allocator.h"

#include <iterator>
#include <type_traits>
#include <initializer_list>
#include <memory>
#include <algorithm>

#ifdef USE_NEW_CONTAINER

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		template<typename T, typename Alloc = API::Global::Memory::Allocator<T, API::Global::Memory::MemoryHeap>> class List {
			protected:
				typedef allocator_traits<Alloc>::rebind_traits<T> alloc_traits;
			public:	
				typedef T value_type;
				typedef T& reference;
				typedef const T& const_reference;

				using allocator_type = typename alloc_traits::allocator_type;
				using pointer = typename alloc_traits::pointer;
				using const_pointer = typename alloc_traits::const_pointer;
				using size_type = typename alloc_traits::size_type;
				using difference_type = typename alloc_traits::difference_type;

				class const_iterator {
					friend class List;
					public:
						typedef std::random_access_iterator_tag iterator_category;
						typedef typename List::difference_type difference_type;
						typedef typename List::size_type size_type;
						typedef typename List::value_type value_type;
						typedef typename List::const_reference reference;
						typedef typename List::const_pointer pointer;
					private:
						difference_type m_pos;
						const List* m_parent;

						CLOAK_CALL const_iterator(In difference_type pos, In const List* parent) : m_parent(parent), m_pos(pos) {}
					public:
						CLOAK_CALL const_iterator() : m_pos(0), m_parent(nullptr) {}

						const_iterator& CLOAK_CALL_THIS operator+=(In difference_type n)
						{
							if (m_parent != nullptr) { m_pos += n; }
							return *this;
						}
						const_iterator& CLOAK_CALL_THIS operator-=(In difference_type n)
						{
							if (m_parent != nullptr) { m_pos -= n; }
							return *this;
						}
						const_iterator& CLOAK_CALL_THIS operator++() { return operator+=(1); }
						const_iterator CLOAK_CALL_THIS operator++(In int)
						{
							const_iterator r(*this);
							operator+=(1);
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator--() { return operator-=(1); }
						const_iterator CLOAK_CALL_THIS operator--(In int)
						{
							const_iterator r(*this);
							operator-=(1);
							return r;
						}
						const_iterator CLOAK_CALL_THIS operator+(In difference_type n) const { return const_iterator(*this) += n; }
						friend const_iterator CLOAK_CALL operator+(In difference_type n, In const const_iterator& i) { return i + n; }
						const_iterator CLOAK_CALL_THIS operator-(In difference_type n) const { return const_iterator(*this) -= n; }
						difference_type CLOAK_CALL_THIS operator-(In const const_iterator& i) const { return m_pos - i.m_pos; }

						reference CLOAK_CALL_THIS operator[](In difference_type p) const { return m_parent->m_data[m_pos + p]; }
						reference CLOAK_CALL_THIS operator*() const { return m_parent->m_data[m_pos]; }
						pointer CLOAK_CALL_THIS operator->() const { return &m_parent->m_data[m_pos]; }

						bool CLOAK_CALL_THIS operator<(In const const_iterator& i) const { return m_pos < i.m_pos; }
						bool CLOAK_CALL_THIS operator<=(In const const_iterator& i) const { return m_pos <= i.m_pos; }
						bool CLOAK_CALL_THIS operator>(In const const_iterator& i) const { return m_pos > i.m_pos; }
						bool CLOAK_CALL_THIS operator>=(In const const_iterator& i) const { return m_pos >= i.m_pos; }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& i) const { return m_pos == i.m_pos; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& i) const { return m_pos != i.m_pos; }
				};
				class iterator : public const_iterator {
					friend class List;
					public:
						typedef typename List::value_type value_type;
						typedef typename List::reference reference;
						typedef typename List::pointer pointer;
					private:
						CLOAK_CALL iterator(In difference_type pos, In const List* parent) : const_iterator(pos, parent) {}
					public:
						iterator& CLOAK_CALL_THIS operator+=(In difference_type n)
						{
							const_iterator::operator+=(n);
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator-=(In difference_type n)
						{
							const_iterator::operator-=(n);
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator++() { return operator+=(1); }
						iterator CLOAK_CALL_THIS operator++(In int)
						{
							iterator r(*this);
							operator+=(1);
							return r;
						}
						iterator& CLOAK_CALL_THIS operator--() { return operator-=(1); }
						iterator CLOAK_CALL_THIS operator--(In int)
						{
							iterator r(*this);
							operator-=(1);
							return r;
						}
						iterator CLOAK_CALL_THIS operator+(In difference_type n) const { return iterator(*this) += n; }
						friend iterator CLOAK_CALL operator+(In difference_type n, In const iterator& i) { return i + n; }
						iterator CLOAK_CALL_THIS operator-(In difference_type n) const { return iterator(*this) -= n; }
						difference_type CLOAK_CALL_THIS operator-(In const iterator& i) const { return m_pos - i.m_pos; }

						reference CLOAK_CALL_THIS operator[](In difference_type p) const { return m_parent->m_data[m_pos + p]; }
						reference CLOAK_CALL_THIS operator*() const { return m_parent->m_data[m_pos]; }
						pointer CLOAK_CALL_THIS operator->() const { return &m_parent->m_data[m_pos]; }
				};

				typedef std::reverse_iterator<iterator> reverse_iterator;
				typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
			protected:
				static constexpr size_type MIN_CAPACITY = 4; //capacities lower then that will require a reallocation on every resize

				size_type m_size; //Index one past the last element
				size_type m_capacity; //Size of array
				size_type m_offset; //Index of first valid element
				allocator_type m_alloc;
				T* m_data;

				static constexpr bool MOVE_ALLWAYS_EQUAL = alloc_traits::propagate_on_container_move_assignment::value == true || alloc_traits::is_always_equal::value == true;

				CLOAK_FORCEINLINE size_type CLOAK_CALL_THIS CalculateGrowth(In size_type newSize)
				{
					const size_type ch = m_capacity >> 1;
					const size_type nch = max(ch, MIN_CAPACITY);
					if (m_capacity > alloc_traits::max_size(m_alloc) - nch) { return newSize; } //Would overflow otherwise
					const size_type nc = m_capacity + nch;
					if (nc < newSize) { return newSize; } //Would be insufficient otherwise
					return nc;
				}

				CLOAK_FORCEINLINE void CLOAK_CALL_THIS CopyAllocator(In const allocator_type& alloc)
				{ 
					if constexpr (alloc_traits::propagate_on_container_copy_assignment::value == true) { m_alloc = alloc; }
				}
				CLOAK_FORCEINLINE void CLOAK_CALL_THIS MoveAllocator(In allocator_type&& alloc)
				{
					if constexpr (alloc_traits::propagate_on_container_move_assignment::value == true) { m_alloc = std::move(alloc); }
				}

				CLOAK_FORCEINLINE void CLOAK_CALL_THIS Destroy(In size_type first, In size_type last)
				{
					if constexpr (std::is_trivially_destructible<T>::value == false)
					{
						for (; first < last; first++) { alloc_traits::destroy(m_alloc, &m_data[first]); }
					}
				}

				template<typename Iterator> inline void CLOAK_CALL_THIS AssignRange(In Iterator first, In Iterator last, In std::input_iterator_tag)
				{
					size_type next = m_offset;
					for (; next < m_size && first != last; ++first, ++next) { m_data[next] = *first; }
					//Trim:
					Destroy(next, m_size);
					//Append:
					for (; first != last; ++first) { emplace_back(*first); }
				}
				template<typename Iterator> inline void CLOAK_CALL_THIS AssignRange(In Iterator first, In Iterator last, In std::forward_iterator_tag)
				{
					const size_type ns = static_cast<size_type>(std::distance(first, last));
					if (ns > m_capacity)
					{
						CLOAK_ASSUME(ns <= alloc_traits::max_size(m_alloc));
						const size_type nc = CalculateGrowth(ns);
						Destroy(m_offset, m_size);
						m_data = alloc_traits::reallocate(m_alloc, m_data, m_capacity, nc);
						m_capacity = nc;
						m_offset = 0;
						m_size = 0;
						for (; first != last; ++first, ++m_size) { alloc_traits::construct(m_alloc, &m_data[m_size], *first); }
					}
					else
					{
						const size_type of = m_offset;
						const size_type os = m_size;
						size_type p = 0;
						for (; p < of && first != last; ++first, ++p) { alloc_traits::construct(m_alloc, &m_data[p], *first); }
						for (; p < os && first != last; ++first, ++p) { m_data[p] = *first; }
						for (; first != last; ++first, ++p) { alloc_traits::construct(m_alloc, &m_data[p], *first); }
						m_size = p;
						m_offset = 0;
						if (p < of) { p = of; }
						Destroy(p, os);
					}
				}
				CLOAK_FORCEINLINE void CLOAK_CALL_THIS MoveFrom(In List&& other)
				{
					m_size = other.m_size;
					m_capacity = other.m_capacity;
					m_offset = other.m_offset;
					m_data = other.m_data;

					other.m_size = 0;
					other.m_capacity = 0;
					other.m_offset = 0;
					other.m_data = nullptr;
				}
				inline void CLOAK_CALL_THIS MoveAssign(In List&& other)
				{
					if constexpr (MOVE_ALLWAYS_EQUAL == true)
					{
						MoveFrom(std::move(other));
					}
					else
					{
						if (m_alloc == other.m_alloc) { return MoveFrom(std::move(other)); }
						const size_type ns = other.size();
						if (ns > m_capacity)
						{
							CLOAK_ASSUME(ns <= alloc_traits::max_size(m_alloc));
							const size_type nc = CalculateGrowth(ns);
							Destroy(m_offset, m_size);
							m_data = alloc_traits::reallocate(m_alloc, m_data, m_capacity, nc);
							m_capacity = nc;
							m_offset = 0;
							m_size = ns;
							for (size_type a = 0, b = other.m_offset; b < other.m_size; a++) { alloc_traits::construct(m_alloc, &m_data[a], other.m_data[b]); }
						}
						else
						{
							size_type a = 0;
							size_type b = other.m_offset;
							for (; a < m_offset && b < other.m_size; a++, b++) { alloc_traits::construct(m_alloc, &m_data[a], other.m_data[b]); }
							for (; a < m_size && b < other.m_size; a++, b++) { m_data[a] = other.m_data[b]; }
							for (; b < other.m_size; a++, b++) { alloc_traits::construct(m_alloc, &m_data[a], other.m_data[b]); }
							Destroy(max(a, m_offset), m_size);
							m_size = ns;
							m_offset = 0;
						}
					}
				}
				inline void CLOAK_CALL_THIS AssignValue(In size_type ns, In const value_type& value)
				{
					if (ns > m_capacity)
					{
						CLOAK_ASSUME(ns <= alloc_traits::max_size(m_alloc));
						const size_type nc = CalculateGrowth(ns);
						Destroy(m_offset, m_size);
						m_data = alloc_traits::reallocate(m_alloc, m_data, m_capacity, nc);
						m_capacity = nc;
						m_offset = 0;
						m_size = ns;
						for (size_type a = 0; a < ns; a++) { alloc_traits::construct(m_alloc, &m_data[a], value); }
					}
					else
					{
						size_type a = 0;
						for (; a < m_offset && a < ns; a++) { alloc_traits::construct(m_alloc, &m_data[a], value); }
						for (; a < m_size && a < ns; a++) { m_data[a] = value; }
						for (; a < ns; a++) { alloc_traits::construct(m_alloc, &m_data[a], value); }
						Destroy(max(a, m_offset), m_size);
						m_size = ns;
						m_offset = 0;
					}
				}
				inline void CLOAK_CALL_THIS CheckSizeIncrease(In size_type toInsert)
				{
					const size_type np = toInsert + m_size;
					const size_type ns = np - m_offset;
					if (ns > m_capacity) //Resize Array:
					{
						const size_type nc = CalculateGrowth(ns);
						m_data = alloc_traits::reallocate(m_alloc, m_data, m_capacity, nc, m_size - m_offset, m_offset, 0);
						m_size -= m_offset;
						m_offset = 0;
						m_capacity = nc;
					}
					else if (np > m_capacity) //Move to fill offset:
					{
						size_type a = 0;
						size_type b = m_offset;
						for (; a < m_offset && b < m_size; a++, b++) { alloc_traits::construct(m_alloc, &m_data[a], m_data[b]); }
						for (; b < m_size; a++, b++) { m_data[a] = m_data[b]; }
						Destroy(max(a, m_offset), m_size);
						m_size -= m_offset;
						m_offset = 0;
					}
				}
				inline size_type CLOAK_CALL_THIS MakeSpace(In size_type p, In size_type count)
				{
					if (count == 0) { return 0; }
					const size_type ns = size() + count;
					if (p < m_offset) { p = m_offset; }
					if (p > m_size) { p = m_size; }
					if (ns >= m_capacity)
					{
						const size_type nc = CalculateGrowth(ns);
						T* nd = alloc_traits::allocate(m_alloc, nc);
						size_type a = 0;
						size_type b = m_offset;
						for (; b < p; a++, b++)
						{
							alloc_traits::construct(m_alloc, &nd[a], m_data[b]);
							alloc_traits::destroy(m_alloc, &m_data[b]);
						}
						size_type c = a + count;
						for (; b < m_size; c++, b++)
						{
							alloc_traits::construct(m_alloc, &nd[c], m_data[b]);
							alloc_traits::destroy(m_alloc, &m_data[b]);
						}
						alloc_traits::deallocate(m_alloc, m_data, m_capacity);
						m_size = ns;
						m_capacity = nc;
						m_offset = 0;
						m_data = nd;
						return a;
					}
					else
					{
						const size_type fo = m_offset > count ? m_offset - count : 0;
						const size_type fs = ns + fo;
						size_type a = fo;
						size_type b = m_offset;
						size_type c = m_size;
						size_type d = fs;
						if (a != b)
						{
							for (; a < m_offset && b < p; a++, b++) { alloc_traits::construct(m_alloc, &m_data[a], m_data[b]); }
							for (; a < m_size && b < p; a++, b++) { m_data[a] = m_data[b]; }
						}
						if (c != d)
						{
							for (; d > m_size && c > p; d--, c--) { alloc_traits::construct(m_alloc, &m_data[d - 1], m_data[c - 1]); }
							for (; c > p; d--, c--) { m_data[d - 1] = m_data[c - 1]; }
						}
						CLOAK_ASSUME(d > a && d - a == count);
						Destroy(max(a, m_offset), min(d, m_size));
						m_offset = fo;
						m_size = fs;
						return a;
					}
				}
			public:
				//Constructors:
				CLOAK_CALL List() noexcept(std::is_nothrow_constructible<allocator_type>::value) : m_size(0), m_capacity(0), m_offset(0), m_alloc(allocator_type()), m_data(nullptr) {}
				explicit CLOAK_CALL List(In const allocator_type& alloc) noexcept(std::is_nothrow_constructible<allocator_type>::value) : m_size(0), m_capacity(0), m_offset(0), m_alloc(alloc), m_data(nullptr) {}
				explicit CLOAK_CALL List(In size_type count, In const T& value, In_opt const allocator_type& alloc = allocator_type()) : m_size(count), m_capacity(count), m_offset(0), m_alloc(alloc)
				{
					m_data = alloc_traits::allocate(m_alloc, m_capacity);
					for (size_type a = 0; a < count; a++) { alloc_traits::construct(m_alloc, &m_data[a], value); }
				}
				template<bool B = std::is_default_constructible_v<T>, typename = std::enable_if_t<B>>
				explicit CLOAK_CALL List(In size_type count, In_opt const allocator_type& alloc = allocator_type()) : m_size(count), m_capacity(count), m_offset(0), m_alloc(alloc)
				{
					m_data = alloc_traits::allocate(m_alloc, m_capacity);
					for (size_type a = 0; a < count; a++) { alloc_traits::construct(m_alloc, &m_data[a]); }
				}
				template<typename Iterator, typename = typename std::enable_if<!std::is_same<typename std::iterator_traits<Iterator>::value_type, void>::value>::type> 
				CLOAK_CALL List(In Iterator first, In Iterator last, In_opt const allocator_type& alloc = allocator_type()) : m_size(0), m_capacity(0), m_offset(0), m_alloc(alloc), m_data(nullptr)
				{
					auto s = std::distance(first, last);
					if (s > 0)
					{
						m_size = static_cast<size_type>(s);
						m_capacity = m_size;
						m_data = alloc.allocate(m_size);
						for (size_type a = 0; first != last; ++first, a++) { alloc_traits::construct(m_alloc, &m_data[a], *first); }
					}
				}
				CLOAK_CALL List(In const List& other) : m_size(other.m_size), m_capacity(other.m_capacity), m_offset(other.m_offset), m_alloc(alloc_traits::select_on_container_copy_construction(other.m_alloc))
				{
					m_data = alloc_traits::allocate(m_alloc, m_capacity);
					for (size_type a = m_offset; a < m_size; a++) { alloc_traits::construct(m_alloc, &m_data[a], other.m_data[a]); }
				}
				CLOAK_CALL List(In const List& other, In allocator_type alloc) : m_size(other.m_size), m_capacity(other.m_capacity), m_offset(other.m_offset), m_alloc(alloc)
				{
					m_data = alloc_traits::allocate(m_alloc, m_capacity);
					for (size_type a = m_offset; a < m_size; a++) { alloc_traits::construct(m_alloc, &m_data[a], other.m_data[a]); }
				}
				template<typename OA> CLOAK_CALL List(In const List<T, OA>& other, In_opt const allocator_type& alloc = allocator_type()) : m_size(other.size()), m_capacity(other.capacity()), m_offset(0), m_alloc(alloc)
				{
					m_data = alloc_traits::allocate(m_alloc, m_capacity);
					for (size_type a = 0; a < m_size; a++) { alloc_traits::construct(m_alloc, &m_data[a], other[a]); }
				}
				CLOAK_CALL List(In List&& other) : m_size(other.m_size), m_capacity(other.m_capacity), m_offset(other.m_offset), m_alloc(other.m_alloc), m_data(other.m_data)
				{
					other.m_size = 0;
					other.m_capacity = 0;
					other.m_offset = 0;
					other.m_data = nullptr;
				}
				CLOAK_CALL List(In List&& other, In const allocator_type& alloc) : m_size(other.m_size), m_capacity(other.m_capacity), m_offset(other.m_offset), m_alloc(alloc), m_data(other.m_data)
				{
					if constexpr (alloc_traits::is_always_equal::value == true) 
					{ 
						other.m_size = 0;
						other.m_capacity = 0;
						other.m_offset = 0;
						other.m_data = nullptr;
					}
					else
					{
						if (other.m_alloc == alloc)
						{
							other.m_size = 0;
							other.m_capacity = 0;
							other.m_offset = 0;
							other.m_data = nullptr;
						}
						else
						{
							//Need to copy data, as the new allocator can't deallocate the old memory
							m_data = alloc_traits::allocate(m_alloc, m_capacity);
							for (size_type a = 0, b = other.m_offset; b < other.m_size; a++) { alloc_traits::construct(m_alloc, &m_data[a], other.m_data[b]); }
							m_size -= m_offset;
							m_offset = 0;
						}
					}
				}
				CLOAK_CALL List(In std::initializer_list<T> init, In_opt const allocator_type& alloc = allocator_type()) : List(init.begin(), init.end(), alloc) {}

				//Destructor:
				virtual CLOAK_CALL ~List() 
				{
					Destroy(m_offset, m_size);
					alloc_traits::deallocate(m_alloc, m_data, m_capacity);
				}

				//Assignment operators:
				List& CLOAK_CALL_THIS operator=(In const List& other)
				{
					if (this != std::addressof(other))
					{
						if (alloc_traits::propagate_on_container_copy_assignment::value == true && m_alloc != other.m_alloc)
						{
							Destroy(m_offset, m_size);
							alloc_traits::deallocate(m_alloc, m_data, m_capacity);
							m_size = 0;
							m_offset = 0;
							m_capacity = 0;
							m_data = nullptr;
						}
						CopyAllocator(other.m_alloc);
						assign(other.begin(), other.end());
					}
					return *this;
				}
				List& CLOAK_CALL_THIS operator=(In List&& other)
				{
					if (this != std::addressof(other))
					{
						if (MOVE_ALLWAYS_EQUAL == true || m_alloc == other.m_alloc)
						{
							//We are going to steal other's memory, so we need to free our own first. 
							Destroy(m_offset, m_size);
							alloc_traits::deallocate(m_alloc, m_data, m_capacity);
							m_size = 0;
							m_offset = 0;
							m_capacity = 0;
							m_data = nullptr;
						}
						MoveAllocator(std::move(other.m_alloc));
						MoveAssign(std::move(other));
					}
					return *this;
				}
				List& CLOAK_CALL_THIS operator=(In std::initializer_list<T> list)
				{
					assign(list.begin(), list.end());
					return *this;
				}

				//Iterator acces:
				iterator CLOAK_CALL_THIS begin() { return iterator(m_offset, this); }
				const_iterator CLOAK_CALL_THIS begin() const { return const_iterator(m_offset, this); }
				const_iterator CLOAK_CALL_THIS cbegin() const { return const_iterator(m_offset, this); }
				reverse_iterator CLOAK_CALL_THIS rbegin() { return reverse_iterator(end()); }
				const_reverse_iterator CLOAK_CALL_THIS rbegin() const { return const_reverse_iterator(cend()); }
				const_reverse_iterator CLOAK_CALL_THIS crbegin() const { return const_reverse_iterator(cend()); }
				
				iterator CLOAK_CALL_THIS end() { return iterator(m_size, this); }
				const_iterator CLOAK_CALL_THIS end() const { return const_iterator(m_size, this); }
				const_iterator CLOAK_CALL_THIS cend() const { return const_iterator(m_size, this); }
				reverse_iterator CLOAK_CALL_THIS rend() { return reverse_iterator(begin()); }
				const_reverse_iterator CLOAK_CALL_THIS rend() const { return const_reverse_iterator(cbegin()); }
				const_reverse_iterator CLOAK_CALL_THIS crend() const { return const_reverse_iterator(cbegin()); }

				//Access:
				reference CLOAK_CALL_THIS operator[](In size_type pos) 
				{
					CLOAK_ASSUME(pos + m_offset < m_size);
					return m_data[pos + m_offset];
				}
				const_reference CLOAK_CALL_THIS operator[](In size_type pos) const 
				{
					CLOAK_ASSUME(pos + m_offset < m_size);
					return m_data[pos + m_offset];
				}
				reference CLOAK_CALL_THIS at(In size_type pos) { return operator[](pos); }
				const_reference CLOAK_CALL_THIS at(In size_type pos) const { return operator[](pos); }
				reference CLOAK_CALL_THIS front() { return at(0); }
				const_reference CLOAK_CALL_THIS front() const { return at(0); }
				reference CLOAK_CALL_THIS back()
				{
					CLOAK_ASSUME(m_size > 0);
					return m_data[m_size - 1];
				}
				const_reference CLOAK_CALL_THIS back() const
				{
					CLOAK_ASSUME(m_size > 0);
					return m_data[m_size - 1];
				}
				T* CLOAK_CALL_THIS data() noexcept { return &m_data[m_offset]; }
				const T* CLOAK_CALL_THIS data() const noexcept { return &m_data[m_offset]; }

				//Value insertion:
				void CLOAK_CALL_THIS assign(In size_type count, In const value_type& value) { AssignValue(count, value); }
				void CLOAK_CALL_THIS assign(In std::initializer_list<T> list) { assign(list.begin(), list.end()); }
				template<typename Iterator, typename = typename std::enable_if<!std::is_same<typename std::iterator_traits<Iterator>::value_type, void>::value>::type> 
				void CLOAK_CALL_THIS assign(In Iterator first, In Iterator last) { AssignRange(first, last, Iterator::iterator_category()); }
				void CLOAK_CALL_THIS push_back(In const value_type& value) { emplace_back(value); }
				void CLOAK_CALL_THIS push_back(In value_type&& value) { emplace_back(std::move(value)); }
				template<typename... Args> void CLOAK_CALL_THIS emplace_back(In Args&&... args)
				{
					CheckSizeIncrease(1);
					CLOAK_ASSUME(m_size < m_capacity);
					alloc_traits::construct(m_alloc, &m_data[m_size++], std::forward<Args>(args)...);
				}
				template<typename... Args> iterator CLOAK_CALL_THIS emplace(In const const_iterator& pos, Args&&... args)
				{
					const size_type p = MakeSpace(static_cast<size_type>(max(pos.m_pos, 0)), 1);
					alloc_traits::construct(m_alloc, &m_data[p], std::forward<Args>(args)...);
					return iterator(p, this);
				}
				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In const T& value) { return emplace(pos, value); }
				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In T&& value) { return emplace(pos, std::move(value)); }
				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In size_type count, In const T& value)
				{
					const size_type p = MakeSpace(static_cast<size_type>(max(pos.m_pos, 0)), count);
					for (size_type a = 0; a < count; a++) { alloc_traits::construct(m_alloc, &m_data[p + a], value); }
					return iterator(p, this);
				}
				template<typename Iterator, typename = typename std::enable_if<!std::is_same<typename std::iterator_traits<Iterator>::value_type, void>::value>::type>
				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In Iterator first, In Iterator last)
				{
					const size_type p = MakeSpace(static_cast<size_type>(max(pos.m_pos, 0)), std::distance(first, last));
					for (size_type a = p; first != last; ++first, ++a) { alloc_traits::construct(m_alloc, &m_data[a], *first); }
					return iterator(p, this);
				}
				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In std::initializer_list<T> list) { return insert(pos, list.begin(), list.end()); }

				//Value removal:
				void CLOAK_CALL_THIS clear()
				{
					Destroy(m_offset, m_size);
					m_offset = m_size = 0;
				}
				iterator CLOAK_CALL_THIS erase(In const const_iterator& pos)
				{
					CLOAK_ASSUME(pos.m_pos >= m_offset && pos.m_pos < m_size);
					const size_type m = (m_size + m_offset) >> 1;
					const size_type p = static_cast<size_type>(pos.m_pos);
					if (p < m)
					{
						for (size_type a = p; a > m_offset; a--)
						{
							m_data[a] = m_data[a - 1];
						}
						alloc_traits::destroy(m_alloc, &m_data[m_offset++]);
						return iterator(p + 1, this);
					}
					else
					{
						for (size_type a = p + 1; a < m_size; a++)
						{
							m_data[a - 1] = m_data[a];
						}
						alloc_traits::destroy(m_alloc, &m_data[--m_size]);
						return iterator(p, this);
					}
				}
				iterator CLOAK_CALL_THIS erase(In const const_iterator& first, In const const_iterator& last)
				{
					CLOAK_ASSUME(first.m_pos >= m_offset && first.m_pos < m_size);
					CLOAK_ASSUME(last.m_pos >= m_offset && last.m_pos <= m_size);
					CLOAK_ASSUME(first <= last);
					const size_type p = static_cast<size_type>(first.m_pos);
					const size_type s = static_cast<size_type>(std::distance(first, last));
					size_type a = p;
					size_type b = p + s;
					for (; b < m_size; a++, b++) { m_data[a] = m_data[b]; }
					Destroy(a, m_size);
					m_size = a;
				}
				void CLOAK_CALL_THIS pop_back()
				{
					CLOAK_ASSUME(m_size > m_offset);
					alloc_traits::destroy(m_alloc, &m_data[--m_size]);
				}
				void CLOAK_CALL_THIS resize(In size_type count)
				{
					const size_type s = size();
					if (count > s)
					{
						CheckSizeIncrease(count - s);
						for (size_type a = m_size, b = s; b < count; a++, b++) { alloc_traits::construct(m_alloc, &m_data[a]); }
					}
					else
					{
						const size_type ns = m_size + count - s;
						Destroy(ns, m_size);
						m_size = ns;
					}
				}
				void CLOAK_CALL_THIS resize(In size_type count, In const value_type& value)
				{
					const size_type s = size();
					if (count > s)
					{
						CheckSizeIncrease(count - s);
						for (size_type a = m_size, b = s; b < count; a++, b++) { alloc_traits::construct(m_alloc, &m_data[a], value); }
					}
					else
					{
						const size_type ns = m_size + count - s;
						Destroy(ns, m_size);
						m_size = ns;
					}
				}

				//Capacity:
				bool CLOAK_CALL_THIS empty() const noexcept { return m_offset == m_size; }
				size_type CLOAK_CALL_THIS size() const noexcept { return m_size - m_offset; }
				size_type CLOAK_CALL_THIS capacity() const noexcept { return m_capacity; }
				size_type CLOAK_CALL_THIS max_size() const noexcept { return alloc_traits::max_size(m_alloc); }
				void CLOAK_CALL_THIS reserve(In size_type cap)
				{
					if (cap > m_capacity)
					{
						const size_type nc = CalculateGrowth(cap);
						T* nd = alloc_traits::allocate(m_alloc, nc);
						for (size_type a = 0, b = m_offset; b < m_size; a++, b++)
						{
							alloc_traits::construct(m_alloc, &nd[a], m_data[b]);
							alloc_traits::destroy(m_alloc, &m_data[b]);
						}
						alloc_traits::deallocate(m_alloc, m_data, m_capacity);
						m_size -= m_offset;
						m_offset = 0;
						m_capacity = nc;
						m_data = nd;
					}
				}
				void CLOAK_CALL_THIS shrink_to_fit()
				{
					const size_type ns = size();
					if (m_capacity > ns)
					{
						T* nd = alloc_traits::allocate(m_alloc, ns);
						for (size_type a = 0, b = m_offset; b < m_size; a++, b++)
						{
							alloc_traits::construct(m_alloc, &nd[a], m_data[b]);
							alloc_traits::destroy(m_alloc, &m_data[b]);
						}
						alloc_traits::deallocate(m_alloc, m_data, m_capacity);
						m_size = ns;
						m_offset = 0;
						m_capacity = ns;
						m_data = nd;
					}
				}

				//Comparisson:
				bool CLOAK_CALL_THIS operator==(In const List& other) const noexcept
				{
					if (size() == other.size())
					{
						for (size_type a = m_offset, b = other.m_offset; a < m_size; a++, b++)
						{
							if (m_data[a] != other.m_data[b]) { return false; }
						}
						return true;
					}
					return false;
				}
				bool CLOAK_CALL_THIS operator!=(In const List& other) const noexcept { return !operator==(other); }
				bool CLOAK_CALL_THIS operator<(In const List& other) const noexcept
				{ 
					size_type a = m_offset;
					size_type b = other.m_offset;
					for (; a < m_size && b < other.m_size; a++, b++)
					{
						if (m_data[a] < other.m_data[b]) { return true; }
						if (other.m_data[b] < m_data[a]) { return false; }
					}
					return (a == m_size) && (b != other.m_size);
				}
				bool CLOAK_CALL_THIS operator<=(In const List& other) const noexcept
				{
					size_type a = m_offset;
					size_type b = other.m_offset;
					for (; a < m_size && b < other.m_size; a++, b++)
					{
						if (m_data[a] < other.m_data[b]) { return true; }
						if (other.m_data[b] < m_data[a]) { return false; }
					}
					return (a == m_size) && (b == other.m_size);
				}
				bool CLOAK_CALL_THIS operator>(In const List& other) const noexcept { return other < *this; }
				bool CLOAK_CALL_THIS operator>=(In const List& other) const noexcept { return other > *this; }

				//Other:
				void CLOAK_CALL_THIS swap(In List& other) noexcept(alloc_traits::propagate_on_container_swap::value || alloc_traits::is_always_equal::value)
				{
					if constexpr (alloc_traits::propagate_on_container_swap::value || alloc_traits::is_always_equal::value)
					{
						std::swap(m_alloc, other.m_alloc);
						std::swap(m_size, other.m_size);
						std::swap(m_capacity, other.m_capacity);
						std::swap(m_offset, other.m_offset);
						std::swap(m_data, other.m_data);
					}
					else
					{
						if (m_alloc != other.m_alloc) { throw std::exception("The allocator of this list don't allow swapping"); }
						std::swap(m_size, other.m_size);
						std::swap(m_capacity, other.m_capacity);
						std::swap(m_offset, other.m_offset);
						std::swap(m_data, other.m_data);
					}
				}
				allocator_type get_allocator() const { return m_alloc; }
		};

#ifdef LIST_BOOL_OPTIMIZATION
		template<typename Alloc> class List<bool, Alloc> {
			protected:
				typedef size_t base_type;
				static constexpr base_type VALUE_BITS = sizeof(base_type) << 3;
				static constexpr base_type VALUE_MASK = VALUE_BITS - 1;

				class value_wrapper {
					friend class List;
					private:
						base_type m_value = 0;
					public:
						constexpr CLOAK_CALL value_wrapper() noexcept = default;
						constexpr CLOAK_CALL value_wrapper(In const value_wrapper&) noexcept = default;
						constexpr CLOAK_CALL value_wrapper(In base_type v) noexcept : m_value(v) {}

						CLOAK_FORCEINLINE void CLOAK_CALL_THIS Set(In size_t pos, In bool v) noexcept
						{
							CLOAK_ASSUME(pos < VALUE_BITS);
							if (v == true)
							{
								m_value |= base_type(1) << pos;
							}
							else
							{
								m_value &= ~(base_type(1) << pos);
							}
						}
						CLOAK_FORCEINLINE bool CLOAK_CALL_THIS Get(In size_t pos) const noexcept
						{
							CLOAK_ASSUME(pos < VALUE_BITS);
							return (m_value & (base_type(1) << pos)) != 0;
						}
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS Flip(In size_t pos) noexcept
						{
							CLOAK_ASSUME(pos < VALUE_BITS);
							m_value ^= base_type(1) << pos;
						}

						CLOAK_FORCEINLINE value_wrapper CLOAK_CALL_THIS operator<<(In uint64_t pos) const noexcept { return value_wrapper(m_value << pos); }
						CLOAK_FORCEINLINE value_wrapper CLOAK_CALL_THIS operator>>(In uint64_t pos) const noexcept { return value_wrapper(m_value >> pos); }
						CLOAK_FORCEINLINE value_wrapper CLOAK_CALL_THIS operator|(In const value_wrapper& p) const noexcept { return value_wrapper(m_value | p.m_value); }
						CLOAK_FORCEINLINE value_wrapper CLOAK_CALL_THIS operator&(In const value_wrapper& p) const noexcept { return value_wrapper(m_value & p.m_value); }
						CLOAK_FORCEINLINE value_wrapper CLOAK_CALL_THIS operator~() const noexcept { return value_wrapper(~m_value); }

						CLOAK_FORCEINLINE value_wrapper& CLOAK_CALL_THIS operator<<=(In uint64_t pos) noexcept { m_value = m_value << pos; return *this; }
						CLOAK_FORCEINLINE value_wrapper& CLOAK_CALL_THIS operator>>=(In uint64_t pos) noexcept { m_value = m_value >> pos; return *this; }
						CLOAK_FORCEINLINE value_wrapper& CLOAK_CALL_THIS operator|=(In const value_wrapper& p) noexcept { m_value = m_value | p.m_value; return *this; }
						CLOAK_FORCEINLINE value_wrapper& CLOAK_CALL_THIS operator&=(In const value_wrapper& p) noexcept { m_value = m_value & p.m_value; return *this; }
				};

				static constexpr value_wrapper ALL_FALSE = 0;
				static constexpr value_wrapper ALL_TRUE = ~0;

				typedef List<value_wrapper, Alloc> parent_type;
			public:
				using allocator_type = typename allocator_traits<Alloc>::rebind_traits<bool>::allocator_type;
				using size_type = typename parent_type::size_type;
				using difference_type = typename parent_type::difference_type;
			protected:
				class ir_common {
					friend class List;
					private:
						const value_wrapper* m_parent;
						size_type m_offset;
					public:
						CLOAK_CALL ir_common() noexcept : m_parent(nullptr), m_offset(0) {}
						CLOAK_CALL ir_common(In const ir_common& o) = default;
						CLOAK_CALL ir_common(In ir_common&& o) = default;
						CLOAK_CALL ir_common(In const value_wrapper* vw, In size_type offset) noexcept : m_parent(vw), m_offset(offset) {}

						CLOAK_FORCEINLINE void CLOAK_CALL_THIS Advance(In difference_type pos) noexcept
						{
							if (pos < 0 && m_offset < 0 - static_cast<size_type>(pos))
							{
								m_offset += pos;
								m_parent -= 1 + (static_cast<size_type>(-1) - m_offset) / VALUE_BITS;
								m_offset %= VALUE_BITS;
							}
							else
							{
								m_offset += pos;
								m_parent += m_offset / VALUE_BITS;
								m_offset &= VALUE_MASK;
							}
						}
						CLOAK_FORCEINLINE ptrdiff_t CLOAK_CALL_THIS Difference(In const ir_common& o) const noexcept 
						{
							return ((o.m_parent - m_parent) * VALUE_BITS) + ((o.m_offset + VALUE_BITS - m_offset) & VALUE_MASK);
						}
						CLOAK_FORCEINLINE bool CLOAK_CALL_THIS Get() const noexcept
						{
							if (m_parent != nullptr) { return m_parent->Get(m_offset); }
							return false;
						}
						//I really dislike the fact that we use const_cast here. However, we guarantee that it will only be called in a non-const context, an this way we don't
						//need to write everything twice and can cast iterators to const_iterators without any additional code
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS Set(In bool v) noexcept
						{
							if (m_parent != nullptr) { const_cast<value_wrapper*>(m_parent)->Set(m_offset, v); }
						}
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS Flip() noexcept
						{
							if (m_parent != nullptr) { const_cast<value_wrapper*>(m_parent)->Flip(m_offset); }
						}
						
						bool CLOAK_CALL_THIS operator<(In const ir_common& o) const { return m_parent < o.m_parent || (m_parent == o.m_parent && m_offset < o.m_offset); }
						bool CLOAK_CALL_THIS operator<=(In const ir_common& o) const { return m_parent < o.m_parent || (m_parent == o.m_parent && m_offset <= o.m_offset); }
						bool CLOAK_CALL_THIS operator>(In const ir_common& o) const { return m_parent > o.m_parent || (m_parent == o.m_parent && m_offset > o.m_offset); }
						bool CLOAK_CALL_THIS operator>=(In const ir_common& o) const { return m_parent > o.m_parent || (m_parent == o.m_parent && m_offset >= o.m_offset); }
						bool CLOAK_CALL_THIS operator==(In const ir_common& o) const { return m_parent == o.m_parent && m_offset == o.m_offset; }
						bool CLOAK_CALL_THIS operator!=(In const ir_common& o) const { return !operator==(o); }
				};
			public:
				typedef bool value_type;
				typedef bool const_reference;
				class reference : private ir_common {
					friend class List;
					private:
						CLOAK_CALL reference() noexcept {}
					public:
						CLOAK_CALL reference(In const ir_common& o) : ir_common(o) {}
						CLOAK_CALL reference(In ir_common&& o) : ir_common(std::move(o)) {}

						CLOAK_CALL_THIS operator bool() const { return Get(); }

						reference& CLOAK_CALL_THIS operator=(In bool o) noexcept
						{
							Set(o);
							return *this;
						}
						reference& CLOAK_CALL_THIS operator=(In const reference& o) noexcept { return *this = bool(o); }

						void CLOAK_CALL_THIS flip() noexcept { Flip(); }
				};

				typedef reference* pointer;
				typedef const reference* const_pointer;

				class const_iterator : protected ir_common {
					friend class List;
					public:
						typedef std::random_access_iterator_tag iterator_category;
						typedef typename List::difference_type difference_type;
						typedef typename List::size_type size_type;
						typedef typename List::value_type value_type;
						typedef typename List::const_reference reference;
						typedef typename List::const_pointer pointer;
					protected:
						CLOAK_CALL const_iterator(In const ir_common& o) : ir_common(o) {}
						CLOAK_CALL const_iterator(In ir_common&& o) : ir_common(std::move(o)) {}
					public:
						CLOAK_CALL const_iterator() noexcept = default;
						CLOAK_CALL const_iterator(In const const_iterator& o) noexcept = default;
						CLOAK_CALL const_iterator(In const_iterator&& o) noexcept = default;

						const_iterator& CLOAK_CALL_THIS operator+=(In difference_type n)
						{
							Advance(static_cast<size_type>(n));
							return *this;
						}
						const_iterator& CLOAK_CALL_THIS operator-=(In difference_type n) { return operator+=(-n); }
						const_iterator& CLOAK_CALL_THIS operator++() { return operator+=(1); }
						const_iterator CLOAK_CALL_THIS operator++(In int)
						{
							const_iterator r(*this);
							operator+=(1);
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator--() { return operator-=(1); }
						const_iterator CLOAK_CALL_THIS operator--(In int)
						{
							const_iterator r(*this);
							operator-=(1);
							return r;
						}
						const_iterator CLOAK_CALL_THIS operator+(In difference_type n) const { return const_iterator(*this) += n; }
						friend const_iterator CLOAK_CALL operator+(In difference_type n, In const const_iterator& i) { return i + n; }
						const_iterator CLOAK_CALL_THIS operator-(In difference_type n) const { return const_iterator(*this) -= n; }
						difference_type CLOAK_CALL_THIS operator-(In const const_iterator& i) const { return static_cast<difference_type>(Difference(i)); }

						reference CLOAK_CALL_THIS operator*() const { return Get(); }
						reference CLOAK_CALL_THIS operator[](In difference_type p) const { return *(*this + p); }

						bool CLOAK_CALL_THIS operator<(In const const_iterator& i) const { return ir_common::operator<(i); }
						bool CLOAK_CALL_THIS operator<=(In const const_iterator& i) const { return ir_common::operator<=(i); }
						bool CLOAK_CALL_THIS operator>(In const const_iterator& i) const { return ir_common::operator>(i); }
						bool CLOAK_CALL_THIS operator>=(In const const_iterator& i) const { return ir_common::operator>=(i); }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& i) const { return ir_common::operator==(i); }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& i) const { return ir_common::operator!=(i); }
				};
				class iterator : public const_iterator {
					friend class List;
					public:
						typedef typename List::value_type value_type;
						typedef typename List::reference reference;
						typedef typename List::pointer pointer;
					protected:
						CLOAK_CALL iterator(In const ir_common& o) : const_iterator(o) {}
						CLOAK_CALL iterator(In ir_common&& o) : const_iterator(std::move(o)) {}
					public:
						CLOAK_CALL iterator() noexcept = default;
						CLOAK_CALL iterator(In const iterator& o) noexcept = default;
						CLOAK_CALL iterator(In iterator&& o) noexcept = default;

						iterator& CLOAK_CALL_THIS operator+=(In difference_type n)
						{
							const_iterator::operator+=(n);
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator-=(In difference_type n)
						{
							const_iterator::operator-=(n);
							return *this;
						}
						iterator& CLOAK_CALL_THIS operator++() { return operator+=(1); }
						iterator CLOAK_CALL_THIS operator++(In int)
						{
							iterator r(*this);
							operator+=(1);
							return r;
						}
						iterator& CLOAK_CALL_THIS operator--() { return operator-=(1); }
						iterator CLOAK_CALL_THIS operator--(In int)
						{
							iterator r(*this);
							operator-=(1);
							return r;
						}
						iterator CLOAK_CALL_THIS operator+(In difference_type n) const { return iterator(*this) += n; }
						friend iterator CLOAK_CALL operator+(In difference_type n, In const iterator& i) { return i + n; }
						iterator CLOAK_CALL_THIS operator-(In difference_type n) const { return iterator(*this) -= n; }
						difference_type CLOAK_CALL_THIS operator-(In const iterator& i) const { return const_iterator::operator-(i); }

						reference CLOAK_CALL_THIS operator*() const { return reference(*this); }
						reference CLOAK_CALL_THIS operator[](In difference_type p) const { return *(*this + p); }
				};

				typedef std::reverse_iterator<iterator> reverse_iterator;
				typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
			protected:
				parent_type m_data;
				uint64_t m_startBit; //first valid bit of first value
				uint64_t m_endBit; //one past last valid bit of last value
			public:
				//Constructors:
				CLOAK_CALL List() noexcept(std::is_nothrow_constructible<parent_type>::value) : m_startBit(0), m_endBit(0) {}
				explicit CLOAK_CALL List(In size_type count, In bool value) : m_data((count + VALUE_MASK) / VALUE_BITS, value == true ? ALL_TRUE : ALL_FALSE), m_startBit(0), m_endBit(count) {}
				explicit CLOAK_CALL List(In size_type count) : List(count, false) {}
				template<typename Iterator, typename = typename std::enable_if<!std::is_same<typename std::iterator_traits<Iterator>::value_type, void>::value>::type>
				CLOAK_CALL List(In Iterator first, In Iterator last) : List(std::distance(first, last), false)
				{
					for (size_type a = 0; first != last; ++first, ++a)
					{
						const size_type b = a & VALUE_MASK;
						const size_type c = a / VALUE_BITS;
						m_data[c].Set(b, *first);
					}
				}
				CLOAK_CALL List(In const List& other) : m_data(other.m_data), m_startBit(other.m_startBit), m_endBit(other.m_endBit) {}
				template<typename OA> CLOAK_CALL List(In const List<bool, OA>& other) : List(other.begin(), other.end()) {}
				CLOAK_CALL List(In List&& other) : m_data(std::move(other.m_data)), m_startBit(other.m_startBit), m_endBit(other.m_endBit) {}
				CLOAK_CALL List(In std::initializer_list<bool> init) : List(init.begin(), init.end()) {}

				//Assignment operators:
				List& CLOAK_CALL_THIS operator=(In const List& other)
				{
					m_data = other.m_data;
					m_startBit = other.m_startBit;
					m_endBit = other.m_endBit;
					return *this;
				}
				List& CLOAK_CALL_THIS operator=(In List&& other)
				{
					m_data = std::move(other.m_data);
					m_startBit = other.m_startBit;
					m_endBit = other.m_endBit;
					return *this;
				}
				List& CLOAK_CALL_THIS operator=(In std::initializer_list<bool> list)
				{
					assign(list.begin(), list.end());
					return *this;
				}

				//Iterator acces:
				iterator CLOAK_CALL_THIS begin() { return ir_common(&m_data.data()[m_startBit / VALUE_BITS], m_startBit & VALUE_MASK); }
				const_iterator CLOAK_CALL_THIS begin() const { return ir_common(&m_data.data()[m_startBit / VALUE_BITS], m_startBit & VALUE_MASK); }
				const_iterator CLOAK_CALL_THIS cbegin() const { return ir_common(&m_data.data()[m_startBit / VALUE_BITS], m_startBit & VALUE_MASK); }
				reverse_iterator CLOAK_CALL_THIS rbegin() { return reverse_iterator(end()); }
				const_reverse_iterator CLOAK_CALL_THIS rbegin() const { return const_reverse_iterator(cend()); }
				const_reverse_iterator CLOAK_CALL_THIS crbegin() const { return const_reverse_iterator(cend()); }

				iterator CLOAK_CALL_THIS end() { return ir_common(&m_data.data()[m_endBit / VALUE_BITS], m_endBit & VALUE_MASK); }
				const_iterator CLOAK_CALL_THIS end() const { return ir_common(&m_data.data()[m_endBit / VALUE_BITS], m_endBit & VALUE_MASK); }
				const_iterator CLOAK_CALL_THIS cend() const { return ir_common(&m_data.data()[m_endBit / VALUE_BITS], m_endBit & VALUE_MASK); }
				reverse_iterator CLOAK_CALL_THIS rend() { return reverse_iterator(begin()); }
				const_reverse_iterator CLOAK_CALL_THIS rend() const { return const_reverse_iterator(cbegin()); }
				const_reverse_iterator CLOAK_CALL_THIS crend() const { return const_reverse_iterator(cbegin()); }

				//Access:
				reference CLOAK_CALL_THIS operator[](In size_type pos)
				{
					CLOAK_ASSUME(pos < m_endBit - m_startBit);
					const size_type p = m_startBit + pos;
					return ir_common(&m_data.data()[p / VALUE_BITS], p & VALUE_MASK);
				}
				const_reference CLOAK_CALL_THIS operator[](In size_type pos) const
				{
					CLOAK_ASSUME(pos < m_endBit - m_startBit);
					const size_type p = m_startBit + pos;
					return m_data.data()[p / VALUE_BITS].Get(p & VALUE_MASK);
				}
				reference CLOAK_CALL_THIS at(In size_type pos) { return operator[](pos); }
				const_reference CLOAK_CALL_THIS at(In size_type pos) const { return operator[](pos); }
				reference CLOAK_CALL_THIS front() { return at(0); }
				const_reference CLOAK_CALL_THIS front() const { return at(0); }
				reference CLOAK_CALL_THIS back()
				{
					CLOAK_ASSUME(m_endBit > m_startBit);
					return at(m_endBit - (m_startBit + 1));
				}
				const_reference CLOAK_CALL_THIS back() const
				{
					CLOAK_ASSUME(m_endBit > m_startBit);
					return at(m_endBit - (m_startBit + 1));
				}

				//Value insertion:
				void CLOAK_CALL_THIS assign(In size_type count, In bool value)
				{
					m_data.assign((count + VALUE_MASK) / VALUE_BITS, value == true ? ALL_TRUE : ALL_FALSE);
					m_startBit = 0;
					m_endBit = count;
				}
				void CLOAK_CALL_THIS assign(In std::initializer_list<bool> list) { assign(list.begin(), list.end()); }
				template<typename Iterator, typename = typename std::enable_if<!std::is_same<typename std::iterator_traits<Iterator>::value_type, void>::value>::type>
				void CLOAK_CALL_THIS assign(In Iterator first, In Iterator last) 
				{ 
					assign(std::distance(first, last), false);
					for (size_type a = 0; first != last; ++first, ++a)
					{
						const size_type b = a & VALUE_MASK;
						const size_type c = a / VALUE_BITS;
						m_data[c].Set(b, *first);
					}
				}
				void CLOAK_CALL_THIS push_back(In bool value) { emplace_back(value); }
				void CLOAK_CALL_THIS emplace_back(In_opt bool args = false)
				{
					const size_type p = m_endBit / VALUE_BITS;
					while (p >= m_data.size()) { m_data.push_back(ALL_FALSE); }
					m_data[p].Set(m_endBit & VALUE_MASK, args);
					m_endBit++;
				}
			protected:
				size_type CLOAK_CALL_THIS MakeSpace(In const const_iterator& pos, In size_type size)
				{
					//Find position:
					for (size_type a = 0; a < m_data.size(); a++)
					{
						if (pos.m_parent == &m_data[a])
						{
							const uint64_t r = (VALUE_BITS - (m_endBit % VALUE_BITS)) % VALUE_BITS;
							const uint64_t blocks = size > r ? (((size + VALUE_BITS) - (r + 1)) / VALUE_BITS) : 0;
							for (uint64_t b = 0; b < blocks; b++) { m_data.push_back(ALL_FALSE); }
							//Move everything to the right:
							const uint64_t shift = size % VALUE_BITS;
							const uint64_t shiftBlocks = size / VALUE_BITS;
							CLOAK_ASSUME(a + shiftBlocks < m_data.size());
							if (m_data.size() > a + 1)
							{
								m_data[m_data.size() - 1] = m_data[m_data.size() - (shiftBlocks + 1)] << shift;
								if (m_data.size() > 2)
								{
									for (uint64_t b = m_data.size() - 2; b > a + shiftBlocks; b--)
									{
										if (shift > 0) { m_data[b + 1] |= m_data[b - shiftBlocks] >> (VALUE_BITS - shift); }
										m_data[b] = m_data[b - shiftBlocks] << shift;
									}
								}
								//Zero out shifted blocks:
								for (uint64_t b = 0; b < shiftBlocks; b++) { m_data[a + b + 1] = ALL_FALSE; }
								//Move last part:
								if (pos.m_offset < VALUE_BITS) 
								{ 
									m_data[a + shiftBlocks + 1] |= (m_data[a] & (ALL_TRUE << pos.m_offset)) >> (VALUE_BITS - shift); 
								}
							}
							m_data[a] = (m_data[a] & (ALL_TRUE >> (VALUE_BITS - pos.m_offset))) | ((m_data[a] & (ALL_TRUE << pos.m_offset)) << shift);
							m_endBit += size;
							return a;
						}
					}
					return m_data.size();
				}
			public:
				iterator CLOAK_CALL_THIS emplace(In const const_iterator& pos, In_opt bool args = false)
				{
					const size_type p = MakeSpace(pos, 1) + ((pos.m_offset + 1) / VALUE_BITS);
					if (p < m_data.size())
					{
						m_data[p].Set((pos.m_offset + 1) % VALUE_BITS, args);
						return ir_common(&m_data.data()[p], pos.m_offset + 1);
					}
					emplace_back(args);
					return end() - 1;
				}


				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In bool value) { return emplace(pos, value); }
				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In size_type count, In bool value)
				{
					const size_type p = MakeSpace(pos, count) + ((pos.m_offset + 1) / VALUE_BITS);
					if (p < m_data.size())
					{
						for (size_type a = 0, b = (pos.m_offset + 1) % VALUE_BITS, c = p; a < count; a++, b++)
						{
							if (b == VALUE_BITS) { b = 0; c++; }
							m_data[c].Set(b, value);
						}
						return ir_common(&m_data.data()[p], pos.m_offset + 1);
					}
					for (size_type a = 0; a < count; a++) { emplace_back(value); }
					return end() - count;
				}
				/*
				template<typename Iterator, typename = typename std::enable_if<!std::is_same<typename std::iterator_traits<Iterator>::value_type, void>::value>::type>
				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In Iterator first, In Iterator last)
				{
					const size_type p = MakeSpace(static_cast<size_type>(max(pos.m_pos, 0)), std::distance(first, last));
					for (size_type a = p; first != last; ++first, ++a) { alloc_traits::construct(m_alloc, &m_data[a], *first); }
					return iterator(p, this);
				}
				iterator CLOAK_CALL_THIS insert(In const const_iterator& pos, In std::initializer_list<T> list) { return insert(pos, list.begin(), list.end()); }

				//Value removal:
				void CLOAK_CALL_THIS clear()
				{
					Destroy(m_offset, m_size);
					m_offset = m_size = 0;
				}
				iterator CLOAK_CALL_THIS erase(In const const_iterator& pos)
				{
					CLOAK_ASSUME(pos.m_pos >= m_offset && pos.m_pos < m_size);
					const size_type m = (m_size + m_offset) >> 1;
					const size_type p = static_cast<size_type>(pos.m_pos);
					if (p < m)
					{
						for (size_type a = p; a > m_offset; a--)
						{
							m_data[a] = m_data[a - 1];
						}
						alloc_traits::destroy(m_alloc, &m_data[m_offset++]);
						return iterator(p + 1, this);
					}
					else
					{
						for (size_type a = p + 1; a < m_size; a++)
						{
							m_data[a - 1] = m_data[a];
						}
						alloc_traits::destroy(m_alloc, &m_data[--m_size]);
						return iterator(p, this);
					}
				}
				iterator CLOAK_CALL_THIS erase(In const const_iterator& first, In const const_iterator& last)
				{
					CLOAK_ASSUME(first.m_pos >= m_offset && first.m_pos < m_size);
					CLOAK_ASSUME(last.m_pos >= m_offset && last.m_pos <= m_size);
					CLOAK_ASSUME(first <= last);
					const size_type p = static_cast<size_type>(first.m_pos);
					const size_type s = static_cast<size_type>(std::distance(first, last));
					size_type a = p;
					size_type b = p + s;
					for (; b < m_size; a++, b++) { m_data[a] = m_data[b]; }
					Destroy(a, m_size);
					m_size = a;
				}
				void CLOAK_CALL_THIS pop_back()
				{
					CLOAK_ASSUME(m_size > m_offset);
					alloc_traits::destroy(m_alloc, &m_data[--m_size]);
				}
				void CLOAK_CALL_THIS resize(In size_type count)
				{
					const size_type s = size();
					if (count > s)
					{
						CheckSizeIncrease(count - s);
						for (size_type a = m_size, b = s; b < count; a++, b++) { alloc_traits::construct(m_alloc, &m_data[a]); }
					}
					else
					{
						const size_type ns = m_size + count - s;
						Destroy(ns, m_size);
						m_size = ns;
					}
				}
				void CLOAK_CALL_THIS resize(In size_type count, In const value_type& value)
				{
					const size_type s = size();
					if (count > s)
					{
						CheckSizeIncrease(count - s);
						for (size_type a = m_size, b = s; b < count; a++, b++) { alloc_traits::construct(m_alloc, &m_data[a], value); }
					}
					else
					{
						const size_type ns = m_size + count - s;
						Destroy(ns, m_size);
						m_size = ns;
					}
				}

				//Capacity:
				bool CLOAK_CALL_THIS empty() const noexcept { return m_offset == m_size; }
				size_type CLOAK_CALL_THIS size() const noexcept { return m_size - m_offset; }
				size_type CLOAK_CALL_THIS capacity() const noexcept { return m_capacity; }
				size_type CLOAK_CALL_THIS max_size() const noexcept { return alloc_traits::max_size(m_alloc); }
				void CLOAK_CALL_THIS reserve(In size_type cap)
				{
					if (cap > m_capacity)
					{
						const size_type nc = CalculateGrowth(cap);
						T* nd = alloc_traits::allocate(m_alloc, nc);
						for (size_type a = 0, b = m_offset; b < m_size; a++, b++)
						{
							alloc_traits::construct(m_alloc, &nd[a], m_data[b]);
							alloc_traits::destroy(m_alloc, &m_data[b]);
						}
						alloc_traits::deallocate(m_alloc, m_data, m_capacity);
						m_size -= m_offset;
						m_offset = 0;
						m_capacity = nc;
						m_data = nd;
					}
				}
				void CLOAK_CALL_THIS shrink_to_fit()
				{
					const size_type ns = size();
					if (m_capacity > ns)
					{
						T* nd = alloc_traits::allocate(m_alloc, ns);
						for (size_type a = 0, b = m_offset; b < m_size; a++, b++)
						{
							alloc_traits::construct(m_alloc, &nd[a], m_data[b]);
							alloc_traits::destroy(m_alloc, &m_data[b]);
						}
						alloc_traits::deallocate(m_alloc, m_data, m_capacity);
						m_size = ns;
						m_offset = 0;
						m_capacity = ns;
						m_data = nd;
					}
				}

				//Comparisson:
				bool CLOAK_CALL_THIS operator==(In const List& other) const noexcept
				{
					if (size() == other.size())
					{
						for (size_type a = m_offset, b = other.m_offset; a < m_size; a++, b++)
						{
							if (m_data[a] != other.m_data[b]) { return false; }
						}
						return true;
					}
					return false;
				}
				bool CLOAK_CALL_THIS operator!=(In const List& other) const noexcept { return !operator==(other); }
				bool CLOAK_CALL_THIS operator<(In const List& other) const noexcept
				{ 
					size_type a = m_offset;
					size_type b = other.m_offset;
					for (; a < m_size && b < other.m_size; a++, b++)
					{
						if (m_data[a] < other.m_data[b]) { return true; }
						if (other.m_data[b] < m_data[a]) { return false; }
					}
					return (a == m_size) && (b != other.m_size);
				}
				bool CLOAK_CALL_THIS operator<=(In const List& other) const noexcept
				{
					size_type a = m_offset;
					size_type b = other.m_offset;
					for (; a < m_size && b < other.m_size; a++, b++)
					{
						if (m_data[a] < other.m_data[b]) { return true; }
						if (other.m_data[b] < m_data[a]) { return false; }
					}
					return (a == m_size) && (b == other.m_size);
				}
				bool CLOAK_CALL_THIS operator>(In const List& other) const noexcept { return other < *this; }
				bool CLOAK_CALL_THIS operator>=(In const List& other) const noexcept { return other > *this; }

				//Other:
				void CLOAK_CALL_THIS swap(In List& other) noexcept(alloc_traits::propagate_on_container_swap::value || alloc_traits::is_always_equal::value)
				{
					if constexpr (alloc_traits::propagate_on_container_swap::value || alloc_traits::is_always_equal::value)
					{
						std::swap(m_alloc, other.m_alloc);
						std::swap(m_size, other.m_size);
						std::swap(m_capacity, other.m_capacity);
						std::swap(m_offset, other.m_offset);
						std::swap(m_data, other.m_data);
					}
					else
					{
						if (m_alloc != other.m_alloc) { throw std::exception("The allocator of this list don't allow swapping"); }
						std::swap(m_size, other.m_size);
						std::swap(m_capacity, other.m_capacity);
						std::swap(m_offset, other.m_offset);
						std::swap(m_data, other.m_data);
					}
				}
				allocator_type get_allocator() const { return m_alloc; }
				*/
		};
#endif
	}
}

namespace std {
	template<typename T, typename Alloc> void CLOAK_CALL_THIS swap(In const CloakEngine::API::List<T, Alloc>& a, In const CloakEngine::API::List<T, Alloc>& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }
	template<typename T, typename Alloc, typename U> void CLOAK_CALL_THIS erase(In CloakEngine::API::List<T, Alloc>& a, In const U& value) { a.erase(std::remove(a.begin(), a.end(), value), a.end()); }
	template<typename T, typename Alloc, typename Pred> void CLOAK_CALL_THIS erase_if(In CloakEngine::API::List<T, Alloc>& a, In Pred pred) { a.erase(std::remove_if(a.begin(), a.end(), pred), a.end()); }
}

#endif

#endif