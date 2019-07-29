#pragma once
#ifndef CE_IMPL_GLOBAL_MEMORY_H
#define CE_IMPL_GLOBAL_MEMORY_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"

#include <iterator>
#include <functional>

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Memory {
				void CLOAK_CALL OnStart();
				void CLOAK_CALL OnStop();
				void* CLOAK_CALL AllocateStaticMemory(In size_t size);


				template<typename T> class TypedPool {
					private:
						typedef size_t usageMask_t;
						static constexpr size_t USAGE_BITS = sizeof(usageMask_t) << 3;
						static constexpr size_t MAX_PAGE_SIZE = 1 MB;
						static constexpr size_t ELEMENT_USAGE_COUNT = max(std::alignment_of<T>::value, sizeof(usageMask_t)) / sizeof(usageMask_t);
						static constexpr size_t ELEMENT_COUNT = ELEMENT_USAGE_COUNT * USAGE_BITS;
						static constexpr usageMask_t MAX_USAGE = ~0;

						struct PAGE_BLOCK {
							usageMask_t Usage[ELEMENT_USAGE_COUNT];
							PAGE_BLOCK* NextFree;
						};
						struct PAGE {
							PAGE* Next;
						};

						static constexpr size_t ELEMENT_SIZE = (sizeof(T) + std::alignment_of<T>::value - 1) & ~static_cast<size_t>(std::alignment_of<T>::value - 1);
						static constexpr size_t PAGE_BLOCK_SIZE = (sizeof(PAGE_BLOCK) + std::alignment_of<T>::value - 1) & ~static_cast<size_t>(std::alignment_of<T>::value - 1);
						static constexpr size_t BLOCK_SIZE = (ELEMENT_COUNT * ELEMENT_SIZE) + PAGE_BLOCK_SIZE;
						static constexpr size_t HEAD_SIZE = static_cast<size_t>(sizeof(PAGE) + std::alignment_of<T>::value - 1) & ~static_cast<size_t>(std::alignment_of<T>::value - 1);
						static constexpr size_t BLOCK_COUNT = max((MAX_PAGE_SIZE - HEAD_SIZE) / BLOCK_SIZE, 1);
						static constexpr size_t PAGE_SIZE = HEAD_SIZE + (BLOCK_COUNT * BLOCK_SIZE) + std::alignment_of<T>::value - 1;

						PAGE* m_start;
						PAGE_BLOCK* m_free;

						PAGE_BLOCK* CLOAK_CALL_THIS AllocatePage()
						{
							uintptr_t r = reinterpret_cast<uintptr_t>(AllocateStaticMemory(PAGE_SIZE));
							PAGE* res = reinterpret_cast<PAGE*>((r + std::alignment_of<T>::value - 1) & ~static_cast<uintptr_t>(std::alignment_of<T>::value - 1));
							CLOAK_ASSUME(r - reinterpret_cast<uintptr_t>(res) <= std::alignment_of<T>::value - 1);
							res->Next = m_start;
							m_start = res;
							uintptr_t b = r + HEAD_SIZE;
							PAGE_BLOCK* l = m_free;
							for (size_t a = 0; a < BLOCK_COUNT; a++, b += BLOCK_SIZE)
							{
								CLOAK_ASSUME(b + BLOCK_SIZE <= r + PAGE_SIZE);
								CLOAK_ASSUME((b & (std::alignment_of<T>::value - 1)) == 0);
								PAGE_BLOCK* block = reinterpret_cast<PAGE_BLOCK*>(b);
								block->NextFree = l;
								for (size_t c = 0; c < ELEMENT_USAGE_COUNT; c++) { block->Usage[c] = 0; }
								l = block;
							}
							m_free = l;
							return l;
						}
					public:
						struct iterator {
							friend class TypedPool<T>;
							public:
								typedef std::forward_iterator_tag iterator_category;
								typedef T value_type;
								typedef ptrdiff_t difference_type;
								typedef value_type* pointer;
								typedef value_type& reference;
							private:
								PAGE* m_page;
								size_t m_pos;

								void CLOAK_CALL_THIS checkPos()
								{
									while (m_page != nullptr)
									{
										size_t blockID = m_pos / ELEMENT_COUNT;
										size_t p = m_pos % ELEMENT_COUNT;
										size_t uID = p / USAGE_BITS;
										size_t eID = p % USAGE_BITS;
										CLOAK_ASSUME(blockID < BLOCK_COUNT);
										CLOAK_ASSUME(uID < ELEMENT_USAGE_COUNT);
										PAGE_BLOCK* block = reinterpret_cast<PAGE_BLOCK*>(reinterpret_cast<uintptr_t>(m_page) + HEAD_SIZE + (blockID * BLOCK_SIZE));
										if ((block->Usage[uID] & static_cast<usageMask_t>(1Ui64 << eID)) != 0) { return; }
										DWORD i = 0;
										while ((i = API::Global::Math::bitScanForward(block->Usage[uID] & ~static_cast<usageMask_t>((1Ui64 << eID) - 1))) == 0xFF)
										{
											uID++;
											eID = 0;
											if (uID >= ELEMENT_USAGE_COUNT)
											{
												uID = 0;
												blockID++;
												if (blockID >= BLOCK_COUNT)
												{
													m_page = m_page->Next;
													if (m_page == nullptr) { m_pos = 0; return; }
													blockID = 0;
												}
												block = reinterpret_cast<PAGE_BLOCK*>(reinterpret_cast<uintptr_t>(m_page) + HEAD_SIZE + (blockID * BLOCK_SIZE));
											}
										}
										eID = static_cast<size_t>(i);
										m_pos = (blockID * ELEMENT_COUNT) + (uID * USAGE_BITS) + eID;
										CLOAK_ASSUME(m_pos / ELEMENT_COUNT < BLOCK_COUNT);
									}
								}
								pointer CLOAK_CALL_THIS get() const
								{
									if (m_page == nullptr) { return nullptr; }
									size_t blockID = m_pos / ELEMENT_COUNT;
									size_t p = m_pos % ELEMENT_COUNT;
									CLOAK_ASSUME(blockID < BLOCK_COUNT);
									uintptr_t block = reinterpret_cast<uintptr_t>(m_page) + HEAD_SIZE + (blockID * BLOCK_SIZE);
									return reinterpret_cast<pointer>(block + PAGE_BLOCK_SIZE + (p * ELEMENT_SIZE));
								}
								CLOAK_CALL iterator(In PAGE* page, In size_t pos) : m_page(page), m_pos(pos) { checkPos(); }
							public:
								CLOAK_CALL iterator(In const iterator& o) : m_page(o.m_page), m_pos(o.m_pos) {}
								CLOAK_CALL iterator(In_opt nullptr_t = nullptr) : m_page(nullptr), m_pos(0) {}

								iterator& CLOAK_CALL_THIS operator=(In nullptr_t)
								{
									m_page = nullptr;
									m_pos = 0;
									return *this;
								}
								iterator& CLOAK_CALL_THIS operator=(In const iterator& o)
								{
									m_page = o.m_page;
									m_pos = o.m_pos;
									return *this;
								}
								iterator CLOAK_CALL_THIS operator++(In int)
								{
									iterator r = *this;
									if (m_page != nullptr) { m_pos++; checkPos(); }
									return r;
								}
								iterator& CLOAK_CALL_THIS operator++()
								{
									if (m_page != nullptr) { m_pos++; checkPos(); }
									return *this;
								}
								reference CLOAK_CALL_THIS operator*() const { return *get(); }
								pointer CLOAK_CALL_THIS operator->() const { return get(); }
								bool CLOAK_CALL_THIS operator==(In const iterator& o) const
								{
									return (m_page == nullptr && o.m_page == nullptr) || (m_page == o.m_page && m_pos == o.m_pos);
								}
								bool CLOAK_CALL_THIS operator!=(In const iterator& o) const
								{
									return (m_page == nullptr) != (o.m_page == nullptr) || (m_page != nullptr && m_pos != o.m_pos);
								}
						};

						CLOAK_CALL TypedPool() : m_start(nullptr), m_free(nullptr) {}
						CLOAK_CALL ~TypedPool() {}
						void* CLOAK_CALL_THIS Allocate()
						{
							PAGE_BLOCK* block = m_free;
							if (block == nullptr) { block = AllocatePage(); }
							for (size_t a = 0; a < ELEMENT_USAGE_COUNT; a++)
							{
								uint8_t i = 0;
								if ((i = API::Global::Math::bitScanForward(~block->Usage[a])) < USAGE_BITS)
								{
									CLOAK_ASSUME((block->Usage[a] & static_cast<usageMask_t>(1Ui64 << i)) == 0);
									block->Usage[a] |= static_cast<usageMask_t>(1Ui64 << i);
									if (block->Usage[a] == MAX_USAGE) { m_free = block->NextFree; }
									const uintptr_t res = reinterpret_cast<uintptr_t>(block) + PAGE_BLOCK_SIZE + ((i + (a * USAGE_BITS)) * ELEMENT_SIZE);
									CLOAK_ASSUME((res & (std::alignment_of<T>::value - 1)) == 0);
									return reinterpret_cast<void*>(res);
								}
							}
							CLOAK_ASSUME(false);
							return nullptr;
						}
						void CLOAK_CALL_THIS Free(In void* ptr)
						{
							if (ptr == nullptr) { return; }
							PAGE* p = m_start;
							if (p == nullptr) { return; }
							const uintptr_t td = reinterpret_cast<uintptr_t>(ptr);
							while (td < reinterpret_cast<uintptr_t>(p) + HEAD_SIZE || td >= reinterpret_cast<uintptr_t>(p) + PAGE_SIZE)
							{
								p = p->Next;
								if (p == nullptr) { return; }
							}
							const size_t blockID = static_cast<size_t>((td - (reinterpret_cast<uintptr_t>(p) + HEAD_SIZE)) / BLOCK_SIZE);
							PAGE_BLOCK* block = reinterpret_cast<PAGE_BLOCK*>(reinterpret_cast<uintptr_t>(p) + HEAD_SIZE + (blockID * BLOCK_SIZE));
							CLOAK_ASSUME(td >= reinterpret_cast<uintptr_t>(block) + PAGE_BLOCK_SIZE && td < reinterpret_cast<uintptr_t>(block) + BLOCK_SIZE);
							const size_t elemID = static_cast<size_t>((td - (reinterpret_cast<uintptr_t>(block) + PAGE_BLOCK_SIZE)) / ELEMENT_SIZE);
							CLOAK_ASSUME(elemID < ELEMENT_COUNT);
							CLOAK_ASSUME((block->Usage[elemID / USAGE_BITS] & static_cast<usageMask_t>(1Ui64 << (elemID % USAGE_BITS))) != 0);
							if (block->Usage[elemID / USAGE_BITS] == MAX_USAGE)
							{
								block->NextFree = m_free;
								m_free = block;
							}
							block->Usage[elemID / USAGE_BITS] ^= static_cast<usageMask_t>(1Ui64 << (elemID % USAGE_BITS));
						}
						iterator CLOAK_CALL_THIS begin() { return iterator(m_start, 0); }
						iterator CLOAK_CALL_THIS end() { return iterator(nullptr); }
						void* CLOAK_CALL operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
				};
			}
		}
	}
}

#endif