#pragma once
#ifndef CE_E_LINKEDLIST_H
#define CE_E_LINKEDLIST_H

#include "CloakEngine/Defines.h"

#include "CloakEngine/Global/Memory.h"
#include "CloakEngine/Helper/Predicate.h"

#include <assert.h>
#include <utility>
#include <functional>

namespace CloakEngine {
	namespace Engine {
		template<typename F, typename M = API::Global::Memory::MemoryPool, size_t A = alignof(F)> class LinkedList {
			private:
				struct Header {
					Header* Next;
				};
				struct Block {
					bool Used;
					Block* PreviousLocal;
					Block* NextLocal;
					Block* Previous;
					Block* Next;
				};

				Header* m_header;
				Block* m_free;
				Block* m_first;
				Block* m_lastBlock;
				size_t m_size;
				size_t m_lastAlloc;

				static constexpr size_t ALIGNMENT = A;
				static constexpr size_t MASK = ~static_cast<size_t>(ALIGNMENT - 1);
				static constexpr size_t BLOCK_SIZE = (sizeof(Block) + ALIGNMENT - 1) & MASK;
				static constexpr size_t ELEMENT_SIZE = (BLOCK_SIZE + sizeof(F) + ALIGNMENT - 1) & MASK;
				static constexpr size_t HEADER_SIZE = (sizeof(Header) + ALIGNMENT - 1) & MASK;
				static constexpr size_t LINEAR_THRESHOLD = ((1 MB) - HEADER_SIZE) / ELEMENT_SIZE;

				static F* CLOAK_CALL GetUsedData(In Block* b)
				{
					return reinterpret_cast<F*>(reinterpret_cast<uintptr_t>(b) + BLOCK_SIZE);
				}

				inline Header* CLOAK_CALL_THIS Allocate(In size_t count, Out Block** firstBlock, Out Block** lastBlock)
				{
					const size_t size = HEADER_SIZE + (count * ELEMENT_SIZE);
					const uintptr_t h = reinterpret_cast<uintptr_t>(M::Allocate(size));
					uintptr_t bp = h + HEADER_SIZE;
					Block* last = nullptr;
					*firstBlock = reinterpret_cast<Block*>(bp);
					for (size_t a = 0; a < count; a++, bp += ELEMENT_SIZE)
					{
						Block* b = reinterpret_cast<Block*>(bp);
						b->Used = false;
						b->Previous = last;
						b->PreviousLocal = last;
						if (last != nullptr) { last->Next = b; last->NextLocal = b; }
						last = b;
					}
					last->Next = nullptr;
					last->NextLocal = nullptr;
					*lastBlock = last;
					Header* r = reinterpret_cast<Header*>(h);
					r->Next = nullptr;
					return r;
				}
				inline Block* CLOAK_CALL_THIS GetNextFree()
				{
					if (m_free == nullptr)
					{
						m_lastAlloc = min(m_lastAlloc << 1, LINEAR_THRESHOLD);
#ifdef _DEBUG
						if (m_lastAlloc == LINEAR_THRESHOLD) { CloakDebugLog("LinkedList records high memory consumption (" + std::to_string(m_size) + " Elements). May use other data structure!"); }
#endif
						Block* first = nullptr;
						Block* last = nullptr;
						Header* h = Allocate(m_lastAlloc, &first, &last);
						h->Next = m_header;
						m_header = h;
						first->Previous = m_lastBlock;
						first->PreviousLocal = m_lastBlock;
						m_lastBlock->NextLocal = first;
						m_lastBlock = last;
						m_free = first;
					}

					m_size++;
					Block* r = m_free;
					assert(r->PreviousLocal == nullptr || r->PreviousLocal->Used == true);
					r->Used = true;
					m_free = r->Next;
					if (m_free != nullptr) { m_free->Previous = nullptr; }
					if (r->PreviousLocal == nullptr)
					{
						if (m_first != nullptr) { m_first->Previous = r; }
						r->Next = m_first;
						m_first = r; 
					}
					else 
					{ 
						r->Previous = r->PreviousLocal;
						r->Next = r->Previous->Next;
						if (r->Next != nullptr) { r->Next->Previous = r; }
						r->Previous->Next = r;
					}
					assert(r->Next == nullptr || r->Next->Used == true);
					assert(r->Previous == nullptr || r->Previous->Used == true);
					assert(r->Next == nullptr || r->Next->Previous == r);
					assert(r->Previous == nullptr || r->Previous->Next == r);
					assert(m_first != nullptr && m_first->Previous == nullptr);
					return r;
				}
				inline void CLOAK_CALL_THIS FreeBlock(In Block* b)
				{
					if (b != nullptr)
					{
						assert(b->Used == true);
						F* f = GetUsedData(b);
						f->~F();
						m_size--;

						if (b->Previous != nullptr) { b->Previous->Next = b->Next; }
						if (b->Next != nullptr) { b->Next->Previous = b->Previous; }
						if (b == m_first) { m_first = b->Next; }
						assert(m_size > 0 || m_first == nullptr);
						assert(m_first == nullptr || m_first->Used == true);
						assert(m_first == nullptr || m_first->Previous == nullptr);
						assert(b->Next == nullptr || b->Next->Used == true);
						assert(b->Previous == nullptr || b->Previous->Used == true);
						b->Used = false;

						Block* p = b->PreviousLocal;
						if (p == b->Previous) //Previous block is used
						{
							if (b->NextLocal != nullptr && b->NextLocal->Used == false) //Next block is not used
							{
								b->Next = b->NextLocal;
								Block* np = b->NextLocal->Previous;
								if (np != nullptr) { np->Next = b; }
								else { m_free = b; }
								b->Previous = np;
								b->NextLocal->Previous = b;
								assert(b->Next == nullptr || b->Next->Used == false);
								assert(b->Previous == nullptr || b->Previous->Used == false);
								assert(b->Next == nullptr || b->Next->Previous == b);
								assert(b->Previous == nullptr || b->Previous->Next == b);
							}
							else
							{
								b->Next = m_free;
								if (m_free != nullptr) { m_free->Previous = b; }
								m_free = b;
								b->Previous = nullptr;
								assert(b->Next == nullptr || b->Next->Used == false);
								assert(b->Previous == nullptr || b->Previous->Used == false);
								assert(b->Next == nullptr || b->Next->Previous == b);
								assert(b->Previous == nullptr || b->Previous->Next == b);
							}
						}
						else //Previous block is not used
						{
							b->Previous = p;
							b->Next = p->Next;
							p->Next = b;
							if (b->Next != nullptr) { b->Next->Previous = b; }
							if (b->NextLocal != nullptr && b->NextLocal != b->Next && b->NextLocal->Used == false)//Next block is also not used (worst case)
							{
								Block* sr = b->NextLocal->Previous;
								Block* el = b->Next;
								Block* er = b->NextLocal;
								b->Next = b->NextLocal;
								b->NextLocal->Previous = b;
								while (er->NextLocal != nullptr && er->NextLocal->Used == false) { er = er->NextLocal; }
								if (sr == nullptr)
								{
									m_free = er->Next;
									if (er->Next != nullptr) { er->Next->Previous = nullptr; }
								}
								else
								{
									sr->Next = er->Next;
									if (er->Next != nullptr) { er->Next->Previous = sr; }
								}
								if (el != nullptr) { el->Previous = er; }
								er->Next = el;
							}
							assert(b->Next == nullptr || b->Next->Used == false);
							assert(b->Previous == nullptr || b->Previous->Used == false);
							assert(b->Next == nullptr || b->Next->Previous == b);
							assert(b->Previous == nullptr || b->Previous->Next == b);
						}
					}
				}

				//Not copyable
				CLOAK_CALL LinkedList(In const LinkedList<F, M>& o) {}
				CLOAK_CALL LinkedList(In LinkedList<F, M>&& o) {}
				LinkedList<F, M>& CLOAK_CALL_THIS operator=(In const LinkedList<F, M>& o) { return *this; }
				LinkedList<F, M>& CLOAK_CALL_THIS operator=(In LinkedList<F, M>&& o) { return *this; }
			public:
				class const_iterator;
				class iterator {
					friend class LinkedList<F, M>;
					friend class const_iterator;
					public:
						typedef std::bidirectional_iterator_tag iterator_category;
						typedef F value_type;
						typedef ptrdiff_t difference_type;
						typedef value_type* pointer;
						typedef value_type& reference;
					private:
						Block* m_ptr;
					public:
						CLOAK_CALL iterator(In_opt nullptr_t t = nullptr) { m_ptr = nullptr; }
						CLOAK_CALL iterator(In Block* ptr) { m_ptr = ptr; }

						operator const_iterator() { return const_iterator(m_ptr); }
						iterator& CLOAK_CALL_THIS operator=(In nullptr_t) { m_ptr = nullptr; return *this; }
						iterator CLOAK_CALL_THIS operator++(In int)
						{
							iterator r = *this;
							assert(m_ptr == nullptr || m_ptr->Used == true);
							if (m_ptr != nullptr) { m_ptr = m_ptr->Next; }
							assert(m_ptr == nullptr || m_ptr->Used == true);
							return r;
						}
						iterator& CLOAK_CALL_THIS operator++()
						{
							assert(m_ptr == nullptr || m_ptr->Used == true);
							if (m_ptr != nullptr) { m_ptr = m_ptr->Next; }
							assert(m_ptr == nullptr || m_ptr->Used == true);
							return *this;
						}
						iterator CLOAK_CALL_THIS operator--(In int)
						{
							iterator r = *this;
							assert(m_ptr == nullptr || m_ptr->Used == true);
							if (m_ptr != nullptr) { m_ptr = m_ptr->Previous; }
							assert(m_ptr == nullptr || m_ptr->Used == true);
							return r;
						}
						iterator& CLOAK_CALL_THIS operator--()
						{
							assert(m_ptr == nullptr || m_ptr->Used == true);
							if (m_ptr != nullptr) { m_ptr = m_ptr->Previous; }
							assert(m_ptr == nullptr || m_ptr->Used == true);
							return *this;
						}
						reference CLOAK_CALL_THIS operator*() const { return *GetUsedData(m_ptr); }
						pointer CLOAK_CALL_THIS operator->() const { return GetUsedData(m_ptr); }
						bool CLOAK_CALL_THIS operator==(In const iterator& o) const { return m_ptr == o.m_ptr; }
						bool CLOAK_CALL_THIS operator!=(In const iterator& o) const { return m_ptr != o.m_ptr; }
						bool CLOAK_CALL_THIS operator==(In nullptr_t) const { return m_ptr == nullptr; }
						bool CLOAK_CALL_THIS operator!=(In nullptr_t) const { return m_ptr != nullptr; }
				};
				class const_iterator {
					friend class LinkedList<F, M>;
					public:
						typedef std::bidirectional_iterator_tag iterator_category;
						typedef F value_type;
						typedef ptrdiff_t difference_type;
						typedef const value_type* pointer;
						typedef const value_type& reference;
					private:
						Block* m_ptr;
					public:
						CLOAK_CALL const_iterator(In_opt nullptr_t t = nullptr) { m_ptr = nullptr; }
						CLOAK_CALL const_iterator(In const iterator& o) { m_ptr = o.m_ptr; }
						CLOAK_CALL const_iterator(In Block* ptr) { m_ptr = ptr; }

						const_iterator& CLOAK_CALL_THIS operator=(In const iterator& o) { m_ptr = o.m_ptr; return *this; }
						const_iterator& CLOAK_CALL_THIS operator=(In nullptr_t) { m_ptr = nullptr; return *this; }
						const_iterator CLOAK_CALL_THIS operator++(In int)
						{
							const_iterator r = *this;
							assert(m_ptr == nullptr || m_ptr->Used == true);
							if (m_ptr != nullptr) { m_ptr = m_ptr->Next; }
							assert(m_ptr == nullptr || m_ptr->Used == true);
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator++()
						{
							assert(m_ptr == nullptr || m_ptr->Used == true);
							if (m_ptr != nullptr) { m_ptr = m_ptr->Next; }
							assert(m_ptr == nullptr || m_ptr->Used == true);
							return *this;
						}
						const_iterator CLOAK_CALL_THIS operator--(In int)
						{
							const_iterator r = *this;
							assert(m_ptr == nullptr || m_ptr->Used == true);
							if (m_ptr != nullptr) { m_ptr = m_ptr->Previous; }
							assert(m_ptr == nullptr || m_ptr->Used == true);
							return r;
						}
						const_iterator& CLOAK_CALL_THIS operator--()
						{
							assert(m_ptr == nullptr || m_ptr->Used == true);
							if (m_ptr != nullptr) { m_ptr = m_ptr->Previous; }
							assert(m_ptr == nullptr || m_ptr->Used == true);
							return *this;
						}
						reference CLOAK_CALL_THIS operator*() const { return *GetUsedData(m_ptr); }
						pointer CLOAK_CALL_THIS operator->() const { return GetUsedData(m_ptr); }
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const { return m_ptr == o.m_ptr; }
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const { return m_ptr != o.m_ptr; }
						bool CLOAK_CALL_THIS operator==(In nullptr_t) const { return m_ptr == nullptr; }
						bool CLOAK_CALL_THIS operator!=(In nullptr_t) const { return m_ptr != nullptr; }
				};

				CLOAK_CALL LinkedList(In_opt size_t startSize = 8)
				{
					m_lastAlloc = max(1, startSize);
					m_header = Allocate(m_lastAlloc, &m_free, &m_lastBlock);
					m_first = nullptr;
					m_size = 0;
				}
				CLOAK_CALL ~LinkedList()
				{
					Header* h = m_header;
					do {
						Header* n = h->Next;
						M::Free(h);
						h = n;
					} while (h != nullptr);
				}
				inline void CLOAK_CALL_THIS clear()
				{
					Block* b = m_first;
					while (b != nullptr) 
					{
						Block* n = b->Next;
						FreeBlock(b);
						b = n;
					}
				}
				inline size_t CLOAK_CALL_THIS size() const { return m_size; }
				inline bool CLOAK_CALL_THIS isEmptry() const { return m_size == 0; }
				inline iterator CLOAK_CALL_THIS push(In const F& f)
				{
					Block* free = GetNextFree();
					F* r = GetUsedData(free);
					new(r)F(f);
					return iterator(free);
				}
				inline iterator CLOAK_CALL_THIS insert(In const F& f) { return push(f); }
				template<typename... Args> inline iterator CLOAK_CALL_THIS emplace(In Args&&... args)
				{
					Block* free = GetNextFree();
					F* r = GetUsedData(free);
					new(r)F(std::forward<Args>(args)...);
					return iterator(free);
				}
				inline void CLOAK_CALL_THIS pop(In const iterator& e) { FreeBlock(e.m_ptr); }
				inline void CLOAK_CALL_THIS pop(In const const_iterator& e) { FreeBlock(e.m_ptr); }
				inline void CLOAK_CALL_THIS remove(In const iterator& e) { FreeBlock(e.m_ptr); }
				inline void CLOAK_CALL_THIS remove(In const const_iterator& e) { FreeBlock(e.m_ptr); }
				inline void CLOAK_CALL_THIS erase(In const iterator& e) { FreeBlock(e.m_ptr); }
				inline void CLOAK_CALL_THIS erase(In const const_iterator& e) { FreeBlock(e.m_ptr); }
				inline void CLOAK_CALL_THIS for_each(In std::function<void(F&)> func, In_opt std::function<bool()> check = nullptr) const
				{
					Block* b = m_first;
					while (b != nullptr && (check == false || check()))
					{
						Block* n = b->Next;
						func(*GetUsedData(b));
						b = n;
					}
				}
				inline iterator CLOAK_CALL_THIS begin() { return iterator(m_first); }
				inline iterator CLOAK_CALL_THIS end() { return iterator(nullptr); }
				inline const_iterator CLOAK_CALL_THIS begin() const { return const_iterator(m_first); }
				inline const_iterator CLOAK_CALL_THIS end() const { return const_iterator(nullptr); }
				inline F* CLOAK_CALL_THIS get(In const iterator& i) { return i.m_ptr == nullptr ? nullptr : GetUsedData(i.m_ptr); }
				inline F* CLOAK_CALL_THIS get(In const const_iterator& i) { return i.m_ptr == nullptr ? nullptr : GetUsedData(i.m_ptr); }
				inline iterator CLOAK_CALL_THIS find(In const F& s)
				{
					Block* b = m_first;
					while (b != nullptr)
					{
						if (s == *GetUsedData(b)) { return iterator(b); }
						b = b->Next;
					}
					return iterator(nullptr);
				}
				inline const_iterator CLOAK_CALL_THIS find(In const F& s) const
				{
					Block* b = m_first;
					while (b != nullptr)
					{
						if (s == *GetUsedData(b)) { return const_iterator(b); }
						b = b->Next;
					}
					return const_iterator(nullptr);
				}
				inline bool CLOAK_CALL_THIS contains(In const F& s) const { return find(s) != nullptr; }
				void* CLOAK_CALL operator new(In size_t s) { return API::Global::Memory::MemoryHeap::Allocate(s); }
				void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
		};
	}
}

#endif
