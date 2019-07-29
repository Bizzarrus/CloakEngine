#pragma once
#ifndef CE_API_GLOBAL_CONTAINER_ALLOCATOR_H
#define CE_API_GLOBAL_CONTAINER_ALLOCATOR_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/Predicate.h"
#include "CloakEngine/Helper/Types.h"

#include <functional>

#define NewArray(T, S) CloakEngine::API::Global::Memory::AllocateArray<T>(S)
#define DeleteArray(P) CloakEngine::API::Global::Memory::FreeArray(P)

#if CHECK_COMPILER(MicrosoftVisualCpp)
#define CLOAKENGINE_ALLOCATOR __declspec(allocator)
#else
#define CLOAKENGINE_ALLOCATOR
#endif

//#define USE_NEW_CONTAINER

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Memory {
				struct MemoryStats {
					uint64_t Available = 0;
					uint64_t Used = 0;
				};
				class CLOAKENGINE_API MemoryPool {
					public:
						static void* CLOAK_CALL Allocate(In size_t size);
						static void* CLOAK_CALL Reallocate(In void* ptr, In size_t size, In_opt bool moveData = true);
						static void* CLOAK_CALL TryResizeBlock(In void* ptr, In size_t size);
						static void CLOAK_CALL Free(In void* ptr);
						static size_t CLOAK_CALL GetMaxAlloc();
					private:
						CLOAK_CALL MemoryPool() = delete;
				};
				class CLOAKENGINE_API MemoryHeap {
					public:
						static void* CLOAK_CALL Allocate(In size_t size);
						static void* CLOAK_CALL Reallocate(In void* ptr, In size_t size, In_opt bool moveData = true);
						static void* CLOAK_CALL TryResizeBlock(In void* ptr, In size_t size);
						static void CLOAK_CALL Free(In void* ptr);
						static size_t CLOAK_CALL GetMaxAlloc();
					private:
						CLOAK_CALL MemoryHeap() = delete;
				};
				class CLOAKENGINE_API ThreadLocal {
					public:
						typedef void* StorageID;
						static StorageID CLOAK_CALL Allocate();
						//Frees the given storage ID. No action is taken for all the stored per-thread pointers
						static void CLOAK_CALL Free(In StorageID id);
						//Frees the given storage ID. func is called once for each stored per-thread pointers, so you can
						//free/destruct anything stored there.
						static void CLOAK_CALL Free(In StorageID id, In std::function<void(void* ptr)> func);
						//Returns a pointer to a per-thread stored pointer value for the current thread. The per-thread
						//pointer values is always set to nullptr on first use in the current thread.
						static void** CLOAK_CALL Get(In StorageID id);
					private:
						CLOAK_CALL ThreadLocal() = delete;
				};
				//Fast, short-living memory. Will be freed after current task finishes
				class CLOAKENGINE_API MemoryLocal {
					public:
						static void* CLOAK_CALL Allocate(In size_t size);
						static void* CLOAK_CALL Reallocate(In void* ptr, In size_t size, In_opt bool moveData = true);
						static void* CLOAK_CALL TryResizeBlock(In void* ptr, In size_t size);
						static void CLOAK_CALL Free(In void* ptr);
						static size_t CLOAK_CALL GetMaxAlloc();
					private:
						CLOAK_CALL MemoryLocal() = delete;
				};
				class CLOAKENGINE_API PageStackAllocator {
					public:
						static size_t CLOAK_CALL GetMaxAlloc();

						CLOAK_CALL PageStackAllocator();
						CLOAK_CALL ~PageStackAllocator();
						void* CLOAK_CALL_THIS Allocate(In size_t size);
						void* CLOAK_CALL_THIS Reallocate(In void* ptr, In size_t size, In_opt bool moveData = true);
						void* CLOAK_CALL_THIS TryResizeBlock(In void* ptr, In size_t size);
						void CLOAK_CALL_THIS Free(In void* ptr);
						void CLOAK_CALL_THIS Clear();
					private:
						void* m_ptr;
						void* m_empty;
				};
				template<typename A, typename M = MemoryPool> type_if_satisfied<A*, decltype(is_default_constructible), A> CLOAK_CALL AllocateArray(In size_t s)
				{
					if (s == 0) { return nullptr; }
					static_assert(sizeof(size_t) <= 16, "size_t requires more than 16 bytes, which is not supported!");
					uintptr_t r = reinterpret_cast<uintptr_t>(M::Allocate((sizeof(A) * s) + 16));
					size_t* sp = reinterpret_cast<size_t*>(r + 16 - sizeof(size_t));
					*sp = s;
					A* res = reinterpret_cast<A*>(r + 16);
					memset(res, 0, sizeof(A) * s);
					for (size_t a = 0; a < s; a++) { ::new(&res[a])A(); }
					return res;
				}
				template<typename A, typename M = MemoryPool> void CLOAK_CALL FreeArray(In A * arr)
				{
					if (arr == nullptr) { return; }
					uintptr_t r = reinterpret_cast<uintptr_t>(arr) - 16;
					size_t* sp = reinterpret_cast<size_t*>(r + 16 - sizeof(size_t));
					const size_t s = *sp;
					for (size_t a = 0; a < s; a++) { (&arr[a])->~A(); }
					M::Free(reinterpret_cast<void*>(r));
				}
				template<typename A, typename M> class Allocator {
					public:
						typedef A value_type;
						using propagate_on_container_move_assignment = std::false_type;

						static constexpr bool reallocate_can_copy = std::is_trivially_copyable<A>::value;

						constexpr CLOAK_CALL Allocator() noexcept {}
						constexpr CLOAK_CALL Allocator(In const Allocator&) noexcept = default;
						template<typename U> constexpr CLOAK_CALL Allocator(In const Allocator<U, M>& a)noexcept {}

						[[nodiscard]] CLOAKENGINE_ALLOCATOR A* CLOAK_CALL allocate(In size_t count)
						{
							void* ptr = M::Allocate(count * sizeof(A));
							if (ptr == nullptr) { throw std::bad_alloc(); }
							return reinterpret_cast<A*>(ptr);
						}
						[[nodiscard]] A* CLOAK_CALL reallocate(In A* p, In size_t oldSize, In size_t newSize, In size_t count, In size_t oldOffset, In size_t newOffset)
						{
							CLOAK_ASSUME(count + newOffset <= newSize);
							CLOAK_ASSUME(count + oldOffset <= oldSize);

							const size_t newCap = newSize * sizeof(A);
							A* res = nullptr;
							if constexpr (std::is_trivially_copyable<A>::value == true)
							{
								res = reinterpret_cast<A*>(M::Reallocate(p, newCap, count > 0));
								if (res == nullptr) { throw std::bad_alloc(); }
							}
							else
							{
								res = reinterpret_cast<A*>(M::TryResizeBlock(p, newCap));
								if (res == nullptr)
								{
									if (count > 0)
									{
										res = reinterpret_cast<A*>(M::Allocate(newCap));
										if (res == nullptr) { throw std::bad_alloc(); }

										for (size_t a = 0; a < count; a++)
										{
											::new(&res[a + newOffset])A(p[a + oldOffset]);
											p[a + oldOffset].~A();
										}
										M::Free(p);
									}
									else
									{
										M::Free(p);
										res = reinterpret_cast<A*>(M::Allocate(newSize * sizeof(A)));
										if (res == nullptr) { throw std::bad_alloc(); }
									}
									return res; //Memory has already been moved, no further operations required
								}
							}
							//Move memory by offset if required:
							CLOAK_ASSUME(res != nullptr);
							if (count > 0 && oldOffset != newOffset)
							{
								if (oldOffset > newOffset)
								{
									for (size_t a = 0; a < count; a++)
									{
										if constexpr (std::is_trivially_copyable<A>::value == true)
										{
											memcpy(&res[a + newOffset], &res[a + oldOffset], sizeof(A));
										}
										else
										{
											::new(&res[a + newOffset])A(res[a + oldOffset]);
											res[a + oldOffset].~A();
										}
									}
								}
								else
								{
									for (size_t a = count; a > 0; a--)
									{
										if constexpr (std::is_trivially_copyable<A>::value == true)
										{
											memcpy(&res[a + newOffset - 1], &res[a + oldOffset - 1], sizeof(A));
										}
										else
										{
											::new(&res[a + newOffset - 1])A(res[a + oldOffset - 1]);
											res[a + oldOffset - 1].~A();
										}
									}
								}
							}
							return res;
						}
						void CLOAK_CALL deallocate(In A* p, In size_t n) noexcept { return M::Free(p); }

						size_t CLOAK_CALL max_size() const noexcept { return M::GetMaxAlloc() / sizeof(A); }
				};
				template<typename A, typename M> class Allocator<const A, M> : public Allocator<A, M> {};
				template<typename A, typename M, typename B, typename N> [[nodiscard]] inline bool CLOAK_CALL operator==(In const Allocator<A, M>& a, In const Allocator<B, N>& b) { return false; }
				template<typename A, typename M, typename B, typename N> [[nodiscard]] inline bool CLOAK_CALL operator!=(In const Allocator<A, M>& a, In const Allocator<B, N>& b) { return true; }
				template<typename A, typename M, typename B> [[nodiscard]] inline bool CLOAK_CALL operator==(In const Allocator<A, M>& a, In const Allocator<B, M>& b) { return true; }
				template<typename A, typename M, typename B> [[nodiscard]] inline bool CLOAK_CALL operator!=(In const Allocator<A, M>& a, In const Allocator<B, M>& b) { return false; }
			}
		}
		template<typename Alloc> class allocator_traits : public std::allocator_traits<Alloc> {
			private:
				template<typename U> static auto has_reallocate_impl(int) -> decltype(std::is_same<decltype(std::declval<U>().reallocate(pointer(), 0, 0, 0, 0, 0)), pointer>::value, std::true_type()) { return std::true_type(); }
				template<typename> static std::false_type has_reallocate_impl(...) { return std::false_type(); }
				using has_reallocate = typename decltype(has_reallocate_impl<Alloc>(0));
				typedef std::allocator_traits<Alloc> traits;
			public:
				using pointer = typename std::allocator_traits<Alloc>::pointer;
				using const_pointer = typename std::allocator_traits<Alloc>::const_pointer;
				using void_pointer = typename std::allocator_traits<Alloc>::void_pointer;
				using const_void_pointer = typename std::allocator_traits<Alloc>::const_void_pointer;

				using size_type = typename std::allocator_traits<Alloc>::size_type;
				using difference_type = typename std::allocator_traits<Alloc>::difference_type;

				using propagate_on_container_copy_assignment = typename std::allocator_traits<Alloc>::propagate_on_container_copy_assignment;
				using propagate_on_container_move_assignment = typename std::allocator_traits<Alloc>::propagate_on_container_move_assignment;
				using propagate_on_container_swap = typename std::allocator_traits<Alloc>::propagate_on_container_swap;
				using is_always_equal = typename std::allocator_traits<Alloc>::is_always_equal;

				template<typename O> using rebind_alloc = typename std::allocator_traits<Alloc>::template rebind_alloc<O>;
				template<typename O> using rebind_traits = typename allocator_traits<rebind_alloc<O>>;

				static pointer CLOAK_CALL reallocate(In Alloc& a, In pointer oldMem, In size_type oldSize, In size_type newSize, In size_type count, In size_type oldOffset, In size_type newOffset)
				{
					CLOAK_ASSUME(count + newOffset <= newSize);
					CLOAK_ASSUME(count + oldOffset <= oldSize);
					if constexpr (has_reallocate::value == true) { return a.reallocate(oldMem, oldSize, newSize, count, oldOffset, newOffset); }
					else
					{
						if (count > 0)
						{
							pointer newMem = allocate(a, newSize);
							for (size_type b = 0; b < count; b++)
							{
								std::allocator_traits<Alloc>::construct(a, &newMemb[b + newOffset], oldMem[b + oldOffset]);
								std::allocator_traits<Alloc>::destroy(&oldMem[b + oldOffset]);
							}
							deallocate(a, oldMem, oldSize);
							return newMem;
						}
						else
						{
							deallocate(a, oldMem, oldSize);
							return allocate(a, newSize);
						}
					}
				}
				static pointer CLOAK_CALL reallocate(In Alloc& a, In pointer oldMem, In size_type oldSize, In size_type newSize, In_opt size_type count = 0)
				{
					return reallocate(a, oldMem, oldSize, newSize, count, 0, 0);
				}
		};
	}
}

#endif