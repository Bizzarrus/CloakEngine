#include "stdafx.h"
#include "CloakEngine/World/ComponentManager.h"
#include "Implementation/World/ComponentManager.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/Global/Memory.h"
#include "Implementation/Components/Render.h"
#include "Implementation/World/Entity.h"

namespace CloakEngine {
	namespace Impl {
		namespace World {
			namespace ComponentManager {
				typedef Impl::World::Entity Entity;
				/*
				class EntityAllocator {
					private:
						static constexpr size_t MAX_PAGE_SIZE = 1 MB;
						static constexpr size_t ELEMENT_USAGE_COUNT = max(std::alignment_of_v<Entity>, sizeof(uint32_t)) / sizeof(uint32_t);
						static constexpr size_t ELEMENT_COUNT = ELEMENT_USAGE_COUNT * 32;
						
						struct PAGE_BLOCK {
							uint32_t Usage[ELEMENT_USAGE_COUNT];
						};
						struct PAGE {
							uint32_t FreeCount;
							PAGE* Next;
						};

						static constexpr size_t ELEMENT_SIZE = (sizeof(Entity) + std::alignment_of_v<Entity> - 1) & ~static_cast<size_t>(std::alignment_of_v<Entity> - 1);
						static constexpr size_t PAGE_BLOCK_SIZE = (sizeof(PAGE_BLOCK) + std::alignment_of_v<Entity> - 1) & ~static_cast<size_t>(std::alignment_of_v<Entity> - 1);
						static constexpr size_t BLOCK_SIZE = (ELEMENT_COUNT * ELEMENT_SIZE) + PAGE_BLOCK_SIZE;
						static constexpr size_t HEAD_SIZE = static_cast<size_t>(sizeof(PAGE) + std::alignment_of_v<Entity> - 1) & ~static_cast<size_t>(std::alignment_of_v<Entity> - 1);
						static constexpr size_t BLOCK_COUNT = (MAX_PAGE_SIZE - HEAD_SIZE) / BLOCK_SIZE;
						static constexpr size_t PAGE_SIZE = HEAD_SIZE + (BLOCK_COUNT * BLOCK_SIZE);

						PAGE* m_start;
						API::Helper::ISyncSection* m_sync;

						PAGE* CLOAK_CALL_THIS AllocatePage()
						{
							uintptr_t r = reinterpret_cast<uintptr_t>(Impl::Global::Memory::AllocateStaticMemory(PAGE_SIZE));
							PAGE* res = reinterpret_cast<PAGE*>(r);
							res->Next = m_start;
							res->FreeCount = BLOCK_COUNT * ELEMENT_COUNT;
							m_start = res;
							uintptr_t b = r + HEAD_SIZE;
							for (size_t a = 0; a < BLOCK_COUNT; a++, b += BLOCK_SIZE)
							{
								CLOAK_ASSUME(b + BLOCK_SIZE <= r + PAGE_SIZE);
								PAGE_BLOCK* block = reinterpret_cast<PAGE_BLOCK*>(b);
								for (size_t c = 0; c < ELEMENT_COUNT; c++) { block->Usage[c] = 0; }
							}
							return res;
						}
					public:
						struct iterator {
							friend class EntityAllocator;
							public:
								typedef std::forward_iterator_tag iterator_category;
								typedef Entity value_type;
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
										size_t uID = p / 32;
										size_t eID = p % 32;
										CLOAK_ASSUME(blockID < BLOCK_COUNT);
										CLOAK_ASSUME(uID < ELEMENT_USAGE_COUNT);
										PAGE_BLOCK* block = reinterpret_cast<PAGE_BLOCK*>(reinterpret_cast<uintptr_t>(m_page) + HEAD_SIZE + (blockID * BLOCK_SIZE));
										if ((block->Usage[uID] & (1U << eID)) != 0) { return; }
										DWORD i = 0;
										while ((i = API::Global::Math::bitScanForward(block->Usage[uID] & ~static_cast<uint32_t>((1U << eID) - 1))) == 0xFF)
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
										m_pos = (blockID * ELEMENT_COUNT) + (uID * 32) + eID;
										CLOAK_ASSUME(m_pos / ELEMENT_COUNT < BLOCK_COUNT);
									}
								}
								void* CLOAK_CALL_THIS get() const
								{
									if (m_page == nullptr) { return nullptr; }
									size_t blockID = m_pos / ELEMENT_COUNT;
									size_t p = m_pos % ELEMENT_COUNT;
									CLOAK_ASSUME(blockID < BLOCK_COUNT);
									uintptr_t block = reinterpret_cast<uintptr_t>(m_page) + HEAD_SIZE + (blockID * BLOCK_SIZE);
									return reinterpret_cast<void*>(block + PAGE_BLOCK_SIZE + (p * ELEMENT_SIZE));
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
								reference CLOAK_CALL_THIS operator*() const
								{
									return *reinterpret_cast<pointer>(get());
								}
								pointer CLOAK_CALL_THIS operator->() const { return reinterpret_cast<pointer>(get()); }
								bool CLOAK_CALL_THIS operator==(In const iterator& o) const 
								{
									return (m_page == nullptr && o.m_page == nullptr) || (m_page == o.m_page && m_pos == o.m_pos);
								}
								bool CLOAK_CALL_THIS operator!=(In const iterator& o) const 
								{
									return (m_page == nullptr) != (o.m_page == nullptr) || (m_page != nullptr && m_pos != o.m_pos);
								}
						};

						CLOAK_CALL EntityAllocator()
						{
							m_start = nullptr;
							m_sync = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
						}
						CLOAK_CALL ~EntityAllocator()
						{

						}
						void CLOAK_CALL_THIS Release() { SAVE_RELEASE(m_sync); }
						void* CLOAK_CALL_THIS Allocate()
						{
							API::Helper::Lock lock(m_sync);
							PAGE* p = m_start;
							while (p != nullptr && p->FreeCount == 0) { p = p->Next; }
							if (p == nullptr) { p = AllocatePage(); }
							uintptr_t bd = reinterpret_cast<uintptr_t>(p) + HEAD_SIZE;
							for (size_t a = 0; a < BLOCK_COUNT; a++, bd += BLOCK_SIZE)
							{
								PAGE_BLOCK* block = reinterpret_cast<PAGE_BLOCK*>(bd);
								for (size_t b = 0; b < ELEMENT_USAGE_COUNT; b++)
								{
									DWORD i = 0;
									if ((i = API::Global::Math::bitScanForward(~block->Usage[b])) < 32)
									{
										CLOAK_ASSUME((block->Usage[b] & (1U << i)) == 0);
										block->Usage[b] |= static_cast<uint32_t>(1U << i);
										p->FreeCount--;
										const uintptr_t res = bd + PAGE_BLOCK_SIZE + ((i + (b * 32)) * ELEMENT_SIZE);
										CLOAK_ASSUME((res & (std::alignment_of_v<Entity> - 1)) == 0);
										return reinterpret_cast<void*>(res);
									}
								}
							}
							CLOAK_ASSUME(false);
							return nullptr;
						}
						void CLOAK_CALL_THIS Free(In void* ptr)
						{
							if (ptr == nullptr) { return; }
							API::Helper::Lock lock(m_sync);
							PAGE* p = m_start;
							uintptr_t td = reinterpret_cast<uintptr_t>(ptr);
							CLOAK_ASSUME(p != nullptr);
							while (td < reinterpret_cast<uintptr_t>(p) + HEAD_SIZE || td >= reinterpret_cast<uintptr_t>(p) + PAGE_SIZE) 
							{ 
								p = p->Next; 
								CLOAK_ASSUME(p != nullptr);
							}
							CLOAK_ASSUME(p->FreeCount < BLOCK_COUNT * ELEMENT_COUNT);
							const size_t blockID = static_cast<size_t>((td - (reinterpret_cast<uintptr_t>(p) + HEAD_SIZE)) / BLOCK_SIZE);
							PAGE_BLOCK* block = reinterpret_cast<PAGE_BLOCK*>(reinterpret_cast<uintptr_t>(p) + HEAD_SIZE + (blockID * BLOCK_SIZE));
							CLOAK_ASSUME(td >= reinterpret_cast<uintptr_t>(block) + PAGE_BLOCK_SIZE && td < reinterpret_cast<uintptr_t>(block) + BLOCK_SIZE);
							const size_t elemID = static_cast<size_t>((td - (reinterpret_cast<uintptr_t>(block) + PAGE_BLOCK_SIZE)) / ELEMENT_SIZE);
							CLOAK_ASSUME(elemID < ELEMENT_COUNT);
							CLOAK_ASSUME((block->Usage[elemID / 32] & (1U << (elemID % 32))) != 0);
							block->Usage[elemID / 32] ^= 1U << (elemID % 32);
							p->FreeCount++;
						}
						API::Helper::Lock CLOAK_CALL_THIS Lock() const { return API::Helper::Lock(m_sync); }
						iterator CLOAK_CALL_THIS begin() { return iterator(m_start, 0); }
						iterator CLOAK_CALL_THIS end() { return iterator(nullptr); }
						void* CLOAK_CALL operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
				};
				*/
				typedef Impl::Global::Memory::TypedPool<Entity> EntityAllocator;
				class ExactSlotAllocator {
					private:
						struct PAGE {
							PAGE* Next;
						};
						struct SLOT {
							SLOT* Next;
						};

						static constexpr size_t MAX_PAGE_SIZE = 256 KB;

						PAGE* m_start;
						SLOT* m_free;
						API::Helper::ISyncSection* m_sync;
						const size_t ELEMENT_SIZE;
						const size_t HEAD_SIZE;
						const size_t PAGE_SIZE;
#ifdef _DEBUG
						size_t m_elemSize;
						size_t m_align;
#endif

						void CLOAK_CALL_THIS AllocatePage()
						{
							uintptr_t r = reinterpret_cast<uintptr_t>(Impl::Global::Memory::AllocateStaticMemory(PAGE_SIZE));
							PAGE* res = reinterpret_cast<PAGE*>(r);
							res->Next = m_start;
							m_start = res;
							SLOT* lastSlot = m_free;
							for (uintptr_t a = r + HEAD_SIZE; a < r + PAGE_SIZE; a += ELEMENT_SIZE)
							{
								SLOT* s = reinterpret_cast<SLOT*>(a);
								if (lastSlot != nullptr) { lastSlot->Next = s; }
								else { m_free = s; }
								lastSlot = s;
							}
							lastSlot->Next = nullptr;
						}
						constexpr inline size_t CLOAK_CALL CElementSize(In size_t elem, In size_t align) { return (max(elem, sizeof(SLOT)) + align - 1) & ~(align - 1); }
						constexpr inline size_t CLOAK_CALL CHeadSize(In size_t align) { return (sizeof(PAGE) + align - 1) & ~(align - 1); }
						constexpr inline size_t CLOAK_CALL CPageSize(In size_t elem, In size_t align)
						{
							const size_t hs = CHeadSize(align);
							const size_t es = CElementSize(elem, align);
							return hs + (((MAX_PAGE_SIZE - hs) / es) * es);
						}
					public:
						CLOAK_CALL ExactSlotAllocator(In size_t elemSize, In size_t alignment) : ELEMENT_SIZE(CElementSize(elemSize, alignment)), HEAD_SIZE(CHeadSize(alignment)), PAGE_SIZE(CPageSize(elemSize, alignment))
						{
#ifdef _DEBUG
							m_elemSize = elemSize;
							m_align = alignment;
#endif
							m_start = nullptr;
							m_free = nullptr;
							m_sync = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
						}
						CLOAK_CALL ~ExactSlotAllocator() {}
						void CLOAK_CALL Release() { SAVE_RELEASE(m_sync); }
						void* CLOAK_CALL_THIS Allocate()
						{
							API::Helper::Lock lock(m_sync);
							if (m_free == nullptr) { AllocatePage(); }
							CLOAK_ASSUME(m_free != nullptr);
							SLOT* s = m_free;
							m_free = m_free->Next;
							return reinterpret_cast<void*>(s);
						}
						void CLOAK_CALL_THIS Free(In void* ptr)
						{
							if (ptr == nullptr) { return; }
							API::Helper::Lock lock(m_sync);
							SLOT* s = reinterpret_cast<SLOT*>(ptr);
							s->Next = m_free;
							m_free = s;
						}
						void* CLOAK_CALL operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
#ifdef _DEBUG
						bool CLOAK_CALL_THIS Check(In size_t elemSize, In size_t alignment)
						{
							return elemSize <= m_elemSize && alignment <= m_align;
						}
#endif
				};
				class Cache : public virtual API::World::ComponentManager::ICache {
					private:
						static constexpr size_t MAX_CAPACITY = 1 << 14;

						const API::BitSet m_usage;
						const size_t m_usageSize;
						type_id* m_ids;
						API::Helper::ISyncSection* m_sync = nullptr;
						API::Helper::ISyncSection* m_syncUpdate = nullptr;
						std::atomic<uint64_t> m_ref = 0;
						const size_t ELEMENT_SIZE;
						void* m_data;
						size_t m_size;
						size_t m_capacity;
						API::FlatMap<Entity*, bool> m_toUpdate[2];
						size_t m_updID;

						void* CLOAK_CALL_THIS getNew();
					public:
						CLOAK_CALL Cache(In const API::BitSet& usage, In size_t size, In_reads(size) const type_id* types, In_reads(size) const size_t* typeOrder);
						CLOAK_CALL ~Cache();

						void CLOAK_CALL_THIS GetData(In size_t pos, In const size_t* order, Out API::World::ComponentManager::CacheData* data) override;
						size_t CLOAK_CALL_THIS Size() override;

						uint64_t CLOAK_CALL_THIS AddRef() override;
						uint64_t CLOAK_CALL_THIS Release() override;

						void CLOAK_CALL_THIS PushToUpdate(In Entity* e, In bool remove);

						bool CLOAK_CALL_THIS operator==(In const Cache& o) const { return m_usage == o.m_usage; }
						bool CLOAK_CALL_THIS operator!=(In const Cache& o) const { return m_usage != o.m_usage; }
						bool CLOAK_CALL_THIS operator<(In const Cache& o) const { return m_usage < o.m_usage; }
						bool CLOAK_CALL_THIS operator>(In const Cache& o) const { return m_usage > o.m_usage; }
						bool CLOAK_CALL_THIS operator<=(In const Cache& o) const { return m_usage >= o.m_usage; }
						bool CLOAK_CALL_THIS operator>=(In const Cache& o) const { return m_usage >= o.m_usage; }
						bool CLOAK_CALL_THIS operator==(In const API::BitSet& o) const { return m_usage == o; }
						bool CLOAK_CALL_THIS operator!=(In const API::BitSet& o) const { return m_usage != o; }
						bool CLOAK_CALL_THIS operator<(In const API::BitSet& o) const { return m_usage < o; }
						bool CLOAK_CALL_THIS operator>(In const API::BitSet& o) const { return m_usage > o; }
						bool CLOAK_CALL_THIS operator<=(In const API::BitSet& o) const { return m_usage >= o; }
						bool CLOAK_CALL_THIS operator>=(In const API::BitSet& o) const { return m_usage >= o; }

						void* CLOAK_CALL operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
						void CLOAK_CALL operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
				};
				struct CacheHandler {
					Cache* Cache;

					friend inline bool CLOAK_CALL operator<(In const CacheHandler& a, In const CacheHandler& b) { return a.Cache->operator<(*b.Cache); }
					friend inline bool CLOAK_CALL operator<(In const CacheHandler& a, In const CloakEngine::API::BitSet& b) { return a.Cache->operator<(b); }
					friend inline bool CLOAK_CALL operator<(In const CloakEngine::API::BitSet& a, In const CacheHandler& b) { return b.Cache->operator>(a); }
					friend inline bool CLOAK_CALL operator==(In const CacheHandler& a, In const CacheHandler& b) { return a.Cache->operator==(*b.Cache); }
					friend inline bool CLOAK_CALL operator==(In const CacheHandler& a, In const CloakEngine::API::BitSet& b) { return a.Cache->operator==(b); }
					friend inline bool CLOAK_CALL operator==(In const CloakEngine::API::BitSet& a, In const CacheHandler& b) { return b.Cache->operator==(a); }
				};
				struct CacheCompare {
					bool CLOAK_CALL_THIS operator()(In const CacheHandler& a, In const API::BitSet& b) { return a.Cache->operator<(b); }
					bool CLOAK_CALL_THIS operator()(In const CacheHandler& a, In const CacheHandler& b) { return a.Cache->operator<(*b.Cache); }
				};
			}
		}
	}
}

namespace CloakEngine {
	namespace Impl {
		namespace World {
			namespace ComponentManager {

				API::Helper::ISyncSection* g_sync = nullptr;
				API::Helper::ISyncSection* g_syncCache = nullptr;
				API::Helper::ISyncSection* g_syncEntityAllocator = nullptr;
				EntityAllocator* g_entityAllocator = nullptr;
				API::FlatMap<type_id, ExactSlotAllocator*>* g_allocs = nullptr;
				API::FlatMap<type_id, uint32_t>* g_ids = nullptr;
				API::FlatSet<CacheHandler, CacheCompare>* g_cache = nullptr;
				std::atomic<uint32_t> g_nextID = 0;

				uint32_t CLOAK_CALL GetID(In const type_id& guid)
				{
					API::Helper::Lock lock(g_sync);
					auto f = g_ids->find(guid);
					if (f == g_ids->end())
					{
						const uint32_t id = g_nextID.fetch_add(1);
						g_ids->insert(std::make_pair(guid, id));
						return id;
					}
					return f->second;
				}
				void* CLOAK_CALL Allocate(In const type_id& id, In size_t size, In size_t alignment)
				{
					API::Helper::Lock lock(g_sync);
					auto f = g_allocs->find(id);
					ExactSlotAllocator* al = nullptr;
					if (f == g_allocs->end())
					{
						al = new ExactSlotAllocator(size, alignment);
						g_allocs->insert(std::make_pair(id, al));
					}
					else 
					{ 
						al = f->second; 
#ifdef _DEBUG
						CLOAK_ASSUME(al != nullptr && al->Check(size, alignment));
#endif
					}
					lock.unlock();
					CLOAK_ASSUME(al != nullptr);
					return al->Allocate();
				}
				void CLOAK_CALL Free(In const type_id& id, In void* ptr)
				{
					API::Helper::Lock lock(g_sync);
					auto f = g_allocs->find(id);
					CLOAK_ASSUME(f != g_allocs->end());
					ExactSlotAllocator* al = f->second;
					lock.unlock();
					CLOAK_ASSUME(al != nullptr);
					al->Free(ptr);
				}
				void CLOAK_CALL Initialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync));
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncCache));
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncEntityAllocator));
					g_allocs = new API::FlatMap<type_id, ExactSlotAllocator*>();
					g_ids = new API::FlatMap<type_id, uint32_t>();
					g_cache = new API::FlatSet<CacheHandler, CacheCompare>();
					g_entityAllocator = new EntityAllocator();
				}
				void CLOAK_CALL ReleaseSyncs()
				{
					for (const auto& a : *g_allocs) { a.second->Release(); }
					for (const auto& a : *g_cache) { delete a.Cache; }
					g_cache->clear();
					SAVE_RELEASE(g_syncEntityAllocator);
					SAVE_RELEASE(g_sync);
					SAVE_RELEASE(g_syncCache);
				}
				void CLOAK_CALL Release()
				{
					for (const auto& a : *g_allocs) { delete a.second; }
					delete g_allocs;
					delete g_ids;
					delete g_cache;
					delete g_entityAllocator;
				}
				void* CLOAK_CALL AllocateEntity(In size_t size)
				{
					CLOAK_ASSUME(size == sizeof(Entity));
					API::Helper::Lock lock(g_syncEntityAllocator);
					return g_entityAllocator->Allocate();
				}
				void CLOAK_CALL FreeEntity(In void* ptr)
				{
					API::Helper::Lock lock(g_syncEntityAllocator);
					g_entityAllocator->Free(ptr);
				}
				void CLOAK_CALL UpdateEntityCache(Entity* e, In bool remove)
				{
					if (Impl::Global::Game::canThreadRun())
					{
						API::Helper::Lock lock(Impl::World::ComponentManager::g_syncCache);
						for (const auto& a : *g_cache) { a.Cache->PushToUpdate(e, remove); }
					}
				}

				CLOAK_CALL Cache::Cache(In const API::BitSet& usage, In size_t size, In_reads(size) const type_id* types, In_reads(size) const size_t* typeOrder) : m_usage(usage), m_usageSize(size), ELEMENT_SIZE(sizeof(Entity*) + (sizeof(API::World::ComponentManager::CacheData) * size))
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncUpdate));
					API::Helper::Lock lock(g_syncEntityAllocator);
					m_ids = NewArray(type_id, m_usageSize);
					m_data = nullptr;
					m_size = 0;
					m_capacity = 0;
					m_updID = 0;
					for (size_t a = 0; a < m_usageSize; a++) { m_ids[typeOrder[a]] = types[a]; }
					API::World::ComponentManager::CacheData* cp = NewArray(API::World::ComponentManager::CacheData, m_usageSize);
					for (auto a = g_entityAllocator->begin(); a != g_entityAllocator->end(); ++a)
					{
						if (a->isLoaded() && a->HasComponents(m_usage, m_usageSize, m_ids, cp))
						{
							void* t = getNew();
							*reinterpret_cast<Entity**>(t) = &(*a);
							memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(t) + sizeof(Entity*)), cp, sizeof(API::World::ComponentManager::CacheData) * m_usageSize);
						}
					}
					DeleteArray(cp);
				}
				CLOAK_CALL Cache::~Cache()
				{
					if (m_ref != 0) { CloakDebugLog("Warning: Component Cache is still in use!"); }
					for (size_t a = 0; a < ARRAYSIZE(m_toUpdate); a++)
					{
						for (const auto& b : m_toUpdate[a]) { b.first->Release(); }
					}
					API::Global::Memory::MemoryHeap::Free(m_data);
					DeleteArray(m_ids);
					SAVE_RELEASE(m_sync);
					SAVE_RELEASE(m_syncUpdate);
				}

				void* CLOAK_CALL_THIS Cache::getNew()
				{
					API::Helper::Lock lock(m_sync);
					if (m_size == m_capacity)
					{
						const size_t nc = m_capacity == 0 ? 16 : (m_capacity < MAX_CAPACITY ? (m_capacity + (m_capacity >> 1)) : (m_capacity + MAX_CAPACITY));
						void* nd = API::Global::Memory::MemoryHeap::Allocate(nc * ELEMENT_SIZE);
						if (m_capacity > 0) 
						{ 
							memcpy(nd, m_data, m_capacity * ELEMENT_SIZE); 
							API::Global::Memory::MemoryHeap::Free(m_data);
						}
						m_data = nd;
						m_capacity = nc;
					}
					void* r = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_data) + (m_size * ELEMENT_SIZE));
					CLOAK_ASSUME(r != nullptr);
					CLOAK_ASSUME(m_size < m_capacity);
					m_size++;
					return r;
				}

				void CLOAK_CALL_THIS Cache::GetData(In size_t pos, In const size_t* order, Out API::World::ComponentManager::CacheData* data)
				{
					for (size_t a = 0; a < m_usageSize; a++) 
					{ 
						data[a].first = nullptr;
						data[a].second = nullptr;
					}
					CLOAK_ASSUME(m_ref > 0);
					API::Helper::Lock lock(m_sync);
					if (pos < m_size)
					{
						API::World::ComponentManager::CacheData* t = reinterpret_cast<API::World::ComponentManager::CacheData*>(reinterpret_cast<uintptr_t>(m_data) + (pos * ELEMENT_SIZE) + sizeof(Entity*));
						for (size_t a = 0; a < m_usageSize; a++) { data[order[a]] = t[a]; }
					}
				}
				size_t CLOAK_CALL_THIS Cache::Size()
				{
					CLOAK_ASSUME(m_ref > 0);
					API::Helper::Lock lock(m_sync);
					return m_size;
				}

				uint64_t CLOAK_CALL_THIS Cache::AddRef()
				{
					API::Helper::Lock lock(m_sync);
					const uint64_t r = m_ref.fetch_add(1) + 1;
					if (r == 1)
					{
						API::Helper::Lock ulock(m_syncUpdate);
						const size_t uID = m_updID;
						m_updID = (m_updID + 1) % ARRAYSIZE(m_toUpdate);
						ulock.unlock();

						if (m_toUpdate[uID].size() > 0)
						{
							API::World::ComponentManager::CacheData* cp = NewArray(API::World::ComponentManager::CacheData, m_usageSize);
							for (const auto& a : m_toUpdate[uID])
							{
								bool f = false;
								size_t fp = 0;
								for (size_t b = 0; b < m_size; b++)
								{
									Entity* t = *reinterpret_cast<Entity**>(reinterpret_cast<uintptr_t>(m_data) + (b * ELEMENT_SIZE));
									if (t == a.first) { fp = b; f = true; break; }
								}
								const bool hc = a.second ? false : (a.first->isLoaded() && a.first->HasComponents(m_usage, m_usageSize, m_ids, cp));
								if (f != hc)
								{
									if (f == true)
									{
										if (fp + 1 != m_size)
										{
											memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_data) + (fp * ELEMENT_SIZE)), reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_data) + ((m_size - 1) * ELEMENT_SIZE)), ELEMENT_SIZE);
										}
										m_size--;
									}
									else
									{
										void* t = getNew();
										*reinterpret_cast<Entity**>(t) = a.first;
										memcpy(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(t) + sizeof(Entity*)), cp, sizeof(API::World::ComponentManager::CacheData) * m_usageSize);
									}
								}
							}
							DeleteArray(cp);
						}
						for (size_t a = 0; a < m_size; a++)
						{
							const uintptr_t t = reinterpret_cast<uintptr_t>(m_data) + (a*ELEMENT_SIZE);
							Entity* e = *reinterpret_cast<Entity**>(t);
							e->AddRef();
							API::World::ComponentManager::CacheData* cd = reinterpret_cast<API::World::ComponentManager::CacheData*>(t + sizeof(Entity*));
							for (size_t b = 0; b < m_usageSize; b++) { cd[b].second->AddRef(); }
						}
						for (const auto& a : m_toUpdate[uID])
						{
							if (a.second == false) { a.first->Release(); }
						}
						m_toUpdate[uID].clear();
					}
					return r;
				}
				uint64_t CLOAK_CALL_THIS Cache::Release()
				{
					API::Helper::Lock lock(m_sync);
					const uint64_t r = m_ref.fetch_sub(1);
					if (r == 1)
					{
						for (size_t a = 0; a < m_size; a++)
						{
							const uintptr_t t = reinterpret_cast<uintptr_t>(m_data) + (a*ELEMENT_SIZE);
							Entity* e = *reinterpret_cast<Entity**>(t);
							API::World::ComponentManager::CacheData* cd = reinterpret_cast<API::World::ComponentManager::CacheData*>(t + sizeof(Entity*));
							for (size_t b = 0; b < m_usageSize; b++) { cd[b].second->Release(); }
							e->Release();
						}
					}
					return r;
				}
				void CLOAK_CALL_THIS Cache::PushToUpdate(In Entity* e, In bool remove)
				{
					if (e == nullptr) { return; }
					API::Helper::Lock lock(m_syncUpdate);
					if (remove == false && m_toUpdate[m_updID].find(e) == m_toUpdate[m_updID].end()) { e->AddRef(); }
					m_toUpdate[m_updID][e] = remove;
				}
			}
		}
	}
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace World {
			CLOAKENGINE_API ComponentManager::ICache* CLOAK_CALL ComponentManager::RequestCache(In size_t size, In_reads(size) const type_id* guids, In_reads(size) const bool* readOnly, Out_writes(size) size_t* order)
			{
				API::BitSet mask;
				API::List<std::pair<uint32_t, size_t>> ids;
				ids.reserve(size);
				for (size_t a = 0; a < size; a++)
				{
					const uint32_t id = Impl::World::ComponentManager::GetID(guids[a]);
					mask[id] = true;

					ids.emplace_back(id, a);
					auto low = std::lower_bound(ids.begin(), ids.end(), ids.back(), [](In const std::pair<uint32_t, size_t>& a, In const std::pair<uint32_t, size_t>& b) {return a.first < b.first; });
					CLOAK_ASSUME(low != ids.end());
					CLOAK_ASSUME(low->first != id || low + 1 == ids.end());
					if (low->first != id) { std::rotate(ids.rbegin(), ids.rbegin() + 1, decltype(ids)::reverse_iterator(low)); }
				}
				for (size_t a = 0; a < size; a++)
				{
					const std::pair<uint32_t, size_t>& p = ids[a];
					order[p.second] = a;
				}

				API::Helper::Lock lock(Impl::World::ComponentManager::g_syncCache);
				CLOAK_ASSUME(Impl::World::ComponentManager::g_syncCache != nullptr);
				if (Impl::World::ComponentManager::g_cache->size() > 0)
				{
					auto low = std::lower_bound(Impl::World::ComponentManager::g_cache->begin(), Impl::World::ComponentManager::g_cache->end(), mask, Impl::World::ComponentManager::CacheCompare());
					if (low != Impl::World::ComponentManager::g_cache->end() && *low == mask)
					{
						Impl::World::ComponentManager::Cache* res = low->Cache;
						CLOAK_ASSUME(res != nullptr);
						res->AddRef();
						return res;
					}
				}
				Impl::World::ComponentManager::Cache* res = new Impl::World::ComponentManager::Cache(mask, size, guids, order);
				Impl::World::ComponentManager::g_cache->insert({ res });
				res->AddRef();
				return res;
			}
		}
	}	
}