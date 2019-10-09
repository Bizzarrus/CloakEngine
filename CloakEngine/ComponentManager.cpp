#include "stdafx.h"
#include "CloakEngine/World/ComponentManager.h"
#include "Implementation/World/ComponentManager.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/OS.h"
#include "Implementation/Global/Memory.h"
#include "Implementation/Global/Task.h"
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
						API::Helper::ReadLock lock(Impl::World::ComponentManager::g_syncCache);
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
			namespace ComponentManager_v2 {
				constexpr API::Global::Time MAX_FIX_WAIT_TIME = 1000; //Value in micro seconds

				struct AccessLock {
					uint16_t Read = 0;
					uint16_t Write = 0;
					constexpr CLOAK_CALL AccessLock() : Read(0), Write(0) {}
					constexpr CLOAK_CALL AccessLock(In uint16_t r, In uint16_t w) : Read(r), Write(w) {}
				};

				API::FlatMap<type_name, ComponentAllocator*>* g_allocList = nullptr;
				CE::RefPointer<API::Helper::ISyncSection> g_syncAlloc = nullptr;
				std::atomic<bool> g_blockAdd = false;
				std::atomic<AccessLock> g_access;
				static_assert(decltype(g_access)::is_always_lock_free == true, "AccessLock is too big!");

				template<typename A, typename B> CLOAK_FORCEINLINE void* CLOAK_CALL Move(In A* ptr, In B offset) { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(ptr) + offset); }
				class ComponentCache : public API::World::ComponentManager_v2::IComponentCache {
					private:
						typedef API::Global::Memory::MemoryLocal allocator_t;
						static constexpr size_t PAGE_SIZE = 256 KB;

						struct Page {
							Page* Next = nullptr;
						};
						struct PageData {
							size_t LineCount;
							size_t LineSize;
							size_t PageSize;
							size_t PageHeaderSize;
							CLOAK_CALL PageData(In size_t count)
							{
								PageHeaderSize = (sizeof(Page) + std::alignment_of<void*>::value - 1) & ~(std::alignment_of<void*>::value - 1);
								LineSize = count * sizeof(void*);
								LineCount = (PAGE_SIZE - PageHeaderSize) / LineSize;
								PageSize = PageHeaderSize + (LineCount * LineSize);
							}
						};

						const PageData m_data;
						const size_t m_count;
						ComponentAllocator** const m_allocs;
						size_t m_size;
						Page* m_page;
						Page* m_lp;
						size_t m_lpid;
						std::atomic<uint64_t> m_ref = 1;

					public:
						CLOAK_CALL ComponentCache(In size_t count, In_reads(count) ComponentAllocator** allocs) : m_count(count), m_allocs(allocs), m_size(0), m_data(count)
						{
							m_page = nullptr;
							//Creating the iterators:
							ComponentAllocator::iterator* it = reinterpret_cast<ComponentAllocator::iterator*>(STACK_ALLOCATE((sizeof(ComponentAllocator::iterator) + sizeof(void*)) * m_count));
							for (size_t a = 0; a < m_count; a++)
							{
								m_allocs[a]->AddRef();
								::new(&it[a])ComponentAllocator::iterator(m_allocs[a]->begin());
							}
							//Filling in the pages:
							void** nslot = reinterpret_cast<void**>(Move(it, sizeof(ComponentAllocator::iterator) * m_count));
							Page* curPage = nullptr;
							do {
								//Find furthest iterator:
								Entity_v2::EntityID me = 0;
								size_t mei = 0;
								size_t mc = 0;
								for (size_t a = 0; a < count; a++)
								{
									if (it[a] == m_allocs[a]->end()) { goto finished_pages; }
									const auto i = *it[a];
									nslot[a] = i.second;
									if (a == 0 || i.first > me)
									{
										me = i.first;
										mei = a;
										mc = 1;
									}
									else if (i.first == me)
									{
										mc++;
										if (mc == count)
										{
											CLOAK_ASSUME(a + 1 == count);
											//We found a full set!
											const size_t pp = m_size % m_data.LineCount;
											if (pp == 0)
											{
												Page* np = reinterpret_cast<Page*>(allocator_t::Allocate(m_data.PageSize));
												::new(np)Page();
												if (m_page != nullptr) { m_page->Next = np; }
												else { m_page = np; }
												curPage = np;
											}
											CLOAK_ASSUME(curPage != nullptr);
											void* dst = Move(curPage, m_data.PageHeaderSize + (pp * m_data.LineSize));
											memcpy(dst, nslot, m_data.LineSize);
											m_size++;
											for (size_t b = 0; b < count; b++) { ++it[b]; }
											goto next_iterator;
										}
									}
								}
								//Advance all iterators, except the one that was the furthest:
								for (size_t a = 0; a < count; a++) { if (a != mei) { ++it[a]; } }
							next_iterator:
								continue;
							} while (true);
						finished_pages:
							STACK_FREE(it);
							m_lpid = 0;
							m_lp = m_page;
						}
					private:
						CLOAK_CALL ~ComponentCache()
						{
							while (m_page != nullptr)
							{
								Page* n = m_page->Next;
								allocator_t::Free(m_page);
								m_page = n;
							}
							for (size_t a = 0; a < m_count; a++) { m_allocs[a]->Release(); }
						}
					public:
						static CE::RefPointer<API::World::ComponentManager_v2::IComponentCache> CLOAK_CALL CreateCache(In size_t count, In_reads(count) const type_name* ids)
						{
							void* mem = nullptr;
							size_t s = 0;
							if constexpr (std::alignment_of<ComponentCache>::value < std::alignment_of<ComponentAllocator*>::value) 
							{ 
								s = ((sizeof(ComponentCache) + std::alignment_of<ComponentAllocator*>::value - 1) & ~(std::alignment_of<ComponentAllocator*>::value - 1)); 
							}
							else { s = sizeof(ComponentCache); }
							mem = allocator_t::Allocate(s + (count * sizeof(ComponentAllocator*)));
							ComponentAllocator** allocs = reinterpret_cast<ComponentAllocator**>(Move(mem, s));
							ComponentAllocator::FillAllocators(count, ids, allocs);
							return CE::RefPointer<API::World::ComponentManager_v2::IComponentCache>::ConstructAt<ComponentCache>(mem, count, allocs);
						}

						size_t CLOAK_CALL_THIS GetSize() const override { return m_size; }
						void CLOAK_CALL_THIS Get(In size_t pos, Out void** data) override 
						{
							if (pos < m_size)
							{
								const size_t pid = pos / m_data.LineCount;
								const size_t pof = pos % m_data.LineCount;
								if (pid < m_lpid)
								{
									m_lpid = 0;
									m_lp = m_page;
								}
								for (; m_lpid < pid; m_lpid++) 
								{
									CLOAK_ASSUME(m_lp != nullptr);
									m_lp = m_lp->Next; 
								}
								CLOAK_ASSUME(m_lp != nullptr);
								memcpy(data, Move(m_lp, m_data.PageHeaderSize + (pof * m_data.LineSize)), m_data.LineSize);
							}
							else
							{
								for (size_t a = 0; a < m_count; a++) { data[a] = nullptr; }
							}
						}

						uint64_t CLOAK_CALL_THIS AddRef() override { return m_ref.fetch_add(1) + 1; }
						uint64_t CLOAK_CALL_THIS Release() override
						{
							uint64_t r = m_ref.fetch_sub(1);
							if (r == 1) 
							{ 
								this->~ComponentCache();
								allocator_t::Free(this);
							}
							else if (r == 0) { CLOAK_ASSUME(false); m_ref = 0; return 0; }
							return r - 1;
						}
				};

				void CLOAK_CALL ComponentAllocator::Initialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncAlloc));
					g_allocList = new API::FlatMap<type_name, ComponentAllocator*>();
				}
				void CLOAK_CALL ComponentAllocator::Shutdown()
				{
					API::Helper::ReadLock lock(g_syncAlloc);
					g_blockAdd = true;
					API::Global::Task t2 = [](In size_t threadID) {
						for (auto a = g_allocList->begin(); a != g_allocList->end(); ++a) { a->second->RemoveAll(); }
					};
					API::Global::Task t1 = [&t2](In size_t threadID) { FixAll(threadID, 1, &t2); };
					lock.unlock();
					t1.Schedule();
					t2.Schedule();
					t2.WaitForExecution(true);
				}
				void CLOAK_CALL ComponentAllocator::ReleaseSyncs()
				{
					g_syncAlloc.Free();
				}
				void CLOAK_CALL ComponentAllocator::Terminate()
				{
					for (auto a = g_allocList->begin(); a != g_allocList->end(); ++a) { delete a->second; }
					delete g_allocList;
				}
				inline ComponentAllocator* CLOAK_CALL ComponentAllocator::GetAllocator(In const type_name& type)
				{
					API::Helper::ReadLock lock(g_syncAlloc);
					const auto f = g_allocList->find(type);
					if (f != g_allocList->end()) { return f->second; }
					return nullptr;
				}
				ComponentAllocator* CLOAK_CALL ComponentAllocator::GetOrCreateAllocator(In const type_name& type, In size_t size, In size_t alignment, In size_t offsetToBase)
				{
					ComponentAllocator* r = GetAllocator(type);
					if (r != nullptr) { return r; }
					API::Helper::WriteLock lock(g_syncAlloc);
					const auto f = g_allocList->find(type); //We try another find, since the allocator might have been created by another thread while we were waiting in the lock
					if (f != g_allocList->end()) { return f->second; }
					r = new ComponentAllocator(type, size, alignment, offsetToBase);
					g_allocList->insert(type, r);
					return r;
				}
				inline void CLOAK_CALL ComponentAllocator::FillAllocators(In size_t count, In_reads(count) const type_name* ids, Out_writes(count) ComponentAllocator** result)
				{
					API::Helper::ReadLock lock(g_syncAlloc);
					for (size_t a = 0; a < count; a++)
					{
						const auto f = g_allocList->find(ids[a]);
						if (f != g_allocList->end()) { result[a] = f->second; }
						else { result[a] = nullptr; }
					}
				}
				void CLOAK_CALL ComponentAllocator::RemoveAll(In Entity_v2::EntityID id)
				{
					API::Helper::ReadLock lock(g_syncAlloc);
					for (auto a = g_allocList->begin(); a != g_allocList->end(); ++a) { a->second->RemoveComponent(id); }
				}
				void CLOAK_CALL ComponentAllocator::FixAll(In size_t threadID, In size_t followCount, In_reads(followCount) API::Global::Task* followTasks)
				{
					//Lock component access:
					AccessLock expL = AccessLock(0, 0);
					AccessLock newL = AccessLock(0, 1);
					if (g_access.compare_exchange_strong(expL, newL, std::memory_order_acq_rel) == false) { return; }
					CLOAK_ASSUME(expL.Read == 0);
					//Start fix:
					for (auto a = g_allocList->begin(); a != g_allocList->end(); ++a)
					{
						if (a->second->RequiresFix())
						{
							API::Global::Task fixing = [followCount, followTasks](In size_t threadID) {
								for (auto a = g_allocList->begin(); a != g_allocList->end(); ++a)
								{
									if (a->second->RequiresFix())
									{
										ComponentAllocator* c = a->second;
										API::Global::Task t = [c](In size_t threadID) 
										{ 
											c->Fix(); 
											//Unlock access:
											AccessLock e = g_access.load(std::memory_order_acquire);
											AccessLock v;
											do {
												v = e;
												CLOAK_ASSUME(v.Read == 0);
												CLOAK_ASSUME(v.Write > 0);
												v.Write--;
											} while (g_access.compare_exchange_weak(e, v, std::memory_order_acq_rel) == false);
										};
										for (size_t b = 0; b < followCount; b++) { followTasks[b].AddDependency(t); }
										t.Schedule(threadID);
									}
								}
							};
							for (size_t b = 0; b < followCount; b++) { followTasks[b].AddDependency(fixing); }
							a = g_allocList->begin();
							uint16_t wc = 0;
							for (; a != g_allocList->end(); ++a)
							{
								wc++;
								ComponentAllocator* c = a->second;
								API::Global::Task t = [c](In size_t threadID) { c->RemoveDeprecatedLinks(); };
								fixing.AddDependency(t);
								t.Schedule(threadID);
							}
							g_access.store(AccessLock(0, wc), std::memory_order_release);
							fixing.Schedule(threadID);
							return;
						}
					}
					//Unlock access:
					g_access.store(AccessLock(0, 0), std::memory_order_release);
				}
				void CLOAK_CALL ComponentAllocator::LockComponentAccess(In size_t threadID)
				{
					AccessLock e = g_access.load(std::memory_order_acquire);
					AccessLock n = e;
					n.Read++;
					if (e.Write > 0 || g_access.compare_exchange_strong(e, n, std::memory_order_acq_rel) == false)
					{
						Impl::Global::Task::IHelpWorker* worker = Impl::Global::Task::GetCurrentWorker(threadID);
						do {
							while (e.Write > 0) 
							{
								worker->HelpWork(~API::Global::Threading::Flag::Components);
								e = g_access.load(std::memory_order_acquire);
							}
							n = e;
							n.Read++;
						} while (g_access.compare_exchange_weak(e, n, std::memory_order_acq_rel) == false);
					}
					CLOAK_ASSUME(e.Write == 0);
				}
				void CLOAK_CALL ComponentAllocator::UnlockComponentAccess(In size_t threadID)
				{
					AccessLock e = g_access.load(std::memory_order_acquire);
					AccessLock n;
					do {
						n = e;
						CLOAK_ASSUME(n.Read > 0);
						CLOAK_ASSUME(n.Write == 0);
						n.Read--;
					} while (g_access.compare_exchange_weak(e, n, std::memory_order_acq_rel) == false);
				}

				CLOAK_CALL ComponentAllocator::ComponentHeader::ComponentHeader(In Entity_v2::EntityID id) : Entity(id) {}
				CLOAK_CALL ComponentAllocator::ComponentLink::ComponentLink(In const type_name& type, In size_t offset) : Type(type), Offset(offset) {}
				CLOAK_CALL ComponentAllocator::PageData::PageData(In size_t size, In size_t alignment)
				{
					const size_t alignMask = alignment - 1;
					ComponentHeaderSize = (sizeof(ComponentHeader) + alignMask) & ~alignMask;
					ComponentSize = ((max(size, sizeof(ComponentLink)) + alignMask) & ~alignMask) + ComponentHeaderSize;
					PageHeaderSize = (sizeof(PageHeader) + alignMask) & ~alignMask;
					ComponentCount = (MAX_PAGE_SIZE - PageHeaderSize) / ComponentSize;
					if (ComponentCount == 0) { ComponentCount = 1; }
					else if (ComponentCount > 64) { ComponentCount = 64; }
					PageSize = PageHeaderSize + (ComponentSize * ComponentCount);
					const Impl::OS::CACHE_DATA& cache = Impl::OS::GetCPUInformation().GetCache(Impl::OS::CacheType::Data, Impl::OS::CacheLevel::L1);
					if (cache.LineSize > alignment) { PageSize = (PageSize + cache.LineSize - 1) & ~(cache.LineSize - 1); }
				}
				CLOAK_CALL ComponentAllocator::SearchResult::SearchResult(In bool found, In size_t globalIndex, In size_t pageIndex, In PageHeader* page) : Found(found), GlobalIndex(globalIndex), PageIndex(pageIndex), Page(page) {}

				CLOAK_CALL ComponentAllocator::iterator::iterator(In const ComponentAllocator* alloc, In PageHeader* page, In size_t position) : m_alloc(alloc), m_page(page), m_pos(position), m_pi(position% alloc->m_data.ComponentCount) {}
				CLOAK_CALL ComponentAllocator::iterator::iterator() : m_alloc(nullptr), m_page(nullptr), m_pos(0), m_pi(0) {}
				CLOAK_CALL ComponentAllocator::iterator::iterator(In const iterator& o) : m_alloc(o.m_alloc), m_page(o.m_page), m_pos(o.m_pos), m_pi(o.m_pi) {}
				CLOAK_CALL ComponentAllocator::iterator::iterator(In iterator&& o) : m_alloc(o.m_alloc), m_page(o.m_page), m_pos(o.m_pos), m_pi(o.m_pi) {}

				ComponentAllocator::iterator& CLOAK_CALL_THIS ComponentAllocator::iterator::operator=(In const iterator& o)
				{
					m_alloc = o.m_alloc;
					m_page = o.m_page;
					m_pos = o.m_pos;
					m_pi = o.m_pi;
					return *this;
				}
				ComponentAllocator::iterator& CLOAK_CALL_THIS ComponentAllocator::iterator::operator=(In iterator&& o)
				{
					m_alloc = o.m_alloc;
					m_page = o.m_page;
					m_pos = o.m_pos;
					m_pi = o.m_pi;
					return *this;
				}

				ComponentAllocator::iterator& CLOAK_CALL_THIS ComponentAllocator::iterator::operator+=(In difference_type n)
				{
					if (m_alloc != nullptr)
					{
						m_pos += n;
						if (m_pos >= m_alloc->m_validSize)
						{
							m_pos = 0;
							m_pi = 0;
							m_alloc = nullptr;
							m_page = nullptr;
							return *this;
						}
						const size_t pi = m_pi + n;
						const size_t p = pi / m_alloc->m_data.ComponentCount;
						m_pi = pi % m_alloc->m_data.ComponentCount;
						for (size_t a = 0; a < p; a++) { m_page = m_page->Next; }
					}
					return *this;
				}
				ComponentAllocator::iterator& CLOAK_CALL_THIS ComponentAllocator::iterator::operator++() { return operator+=(1); }
				ComponentAllocator::iterator CLOAK_CALL_THIS ComponentAllocator::iterator::operator++(In int)
				{
					iterator r(*this);
					operator+=(1);
					return r;
				}
				ComponentAllocator::iterator CLOAK_CALL_THIS ComponentAllocator::iterator::operator+(In difference_type n) const { return iterator(*this) += n; }
				ComponentAllocator::iterator CLOAK_CALL operator+(In ComponentAllocator::iterator::difference_type n, In const ComponentAllocator::iterator& o) { return ComponentAllocator::iterator(o) += n; }
				inline ComponentAllocator::iterator::reference CLOAK_CALL_THIS ComponentAllocator::iterator::operator[](In difference_type n) const
				{
					if (m_page != nullptr)
					{
						CLOAK_ASSUME(m_alloc != nullptr);
						const size_t pi = m_pi + n;
						PageHeader* page = m_page;
						const size_t p = pi / m_alloc->m_data.ComponentCount;
						const size_t i = pi % m_alloc->m_data.ComponentCount;
						for (size_t a = 0; a < p; a++) { page = page->Next; }
						ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_alloc->m_data.PageHeaderSize + (i * m_alloc->m_data.ComponentSize)));
						return reference(h->Entity, Move(h, m_alloc->m_data.ComponentHeaderSize));
					}
					return reference(0, nullptr);
				}
				inline ComponentAllocator::iterator::reference CLOAK_CALL_THIS ComponentAllocator::iterator::operator*() const
				{
					if (m_page != nullptr)
					{
						CLOAK_ASSUME(m_alloc != nullptr);
						ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(m_page, m_alloc->m_data.PageHeaderSize + (m_pi * m_alloc->m_data.ComponentSize)));
						return reference(h->Entity, Move(h, m_alloc->m_data.ComponentHeaderSize));
					}
					return reference(0, nullptr);
				}

				bool CLOAK_CALL_THIS ComponentAllocator::iterator::operator<(In const iterator& i) const { return m_pos < i.m_pos; }
				bool CLOAK_CALL_THIS ComponentAllocator::iterator::operator<=(In const iterator& i) const { return m_pos <= i.m_pos; }
				bool CLOAK_CALL_THIS ComponentAllocator::iterator::operator>(In const iterator& i) const { return m_pos > i.m_pos; }
				bool CLOAK_CALL_THIS ComponentAllocator::iterator::operator>=(In const iterator& i) const { return m_pos >= i.m_pos; }
				bool CLOAK_CALL_THIS ComponentAllocator::iterator::operator==(In const iterator& i) const { return m_pos == i.m_pos; }
				bool CLOAK_CALL_THIS ComponentAllocator::iterator::operator!=(In const iterator& i) const { return m_pos != i.m_pos; }

				inline ComponentAllocator::SearchResult CLOAK_CALL_THIS ComponentAllocator::FindComponent(In Entity_v2::EntityID id)
				{
					PageHeader* page = m_page;
					size_t a = 0;
					if (m_validSize > 0)
					{
						while (true)
						{
							CLOAK_ASSUME(page != nullptr);
							if (page->HighestEntity >= id)
							{
								const size_t remaining = min(m_validSize - a, m_data.ComponentCount);
								size_t lb = 0;
								size_t hb = remaining;
								while (lb != hb)
								{
									const size_t m = (lb + hb) >> 1;
									const Entity_v2::EntityID mid = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (m * m_data.ComponentSize)))->Entity;
									if (mid == id) { return SearchResult(true, m, m - a, page); }
									else if (lb == m) { break; }
									else if (mid < id) { lb = m; }
									else { hb = m; }
								}
								return SearchResult(false, a, remaining, page);
							}
							a += m_data.ComponentCount;
							if (a >= m_validSize) { return SearchResult(false, m_validSize, m_validSize % m_data.ComponentCount, page); }
							page = page->Next.load(std::memory_order_relaxed);
						}
					}
					return SearchResult(false, 0, 0, page);
				}
				inline ComponentAllocator::SearchResult CLOAK_CALL_THIS ComponentAllocator::FindComponentUnsorted(In Entity_v2::EntityID id)
				{
					SearchResult sr = FindComponent(id);
					if (sr.Found == true) { return sr; }
					const size_t size = m_size.load(std::memory_order_acquire);
					size_t a = sr.GlobalIndex;
					PageHeader* page = sr.Page;
					if (size > m_validSize)
					{
						//Move to unsorted area:
						while (a < m_validSize)
						{
							a += m_data.ComponentCount;
							page = page->Next.load(std::memory_order_relaxed);
						}
						a = m_validSize;
						size_t index = a % m_data.ComponentCount;
						do {
							const size_t remaining = min(size - a, m_data.ComponentCount - index);
							ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (index * m_data.ComponentSize)));
							for (size_t b = 0; b < remaining; b++, h = reinterpret_cast<ComponentHeader*>(Move(h, m_data.ComponentSize)), index++)
							{
								if (h->Entity == id) { return SearchResult(true, a + b, index, page); }
							}
							a += remaining;
							if (a >= size)
							{
								CLOAK_ASSUME(a == size && index == size % m_data.ComponentCount); //Sanity check
								return SearchResult(false, a, index, page);
							}
							page = page->Next.load(std::memory_order_acquire);
							index = 0;
						} while (page != nullptr);
					}
					return SearchResult(false, 0, 0, page);
				}
				inline void CLOAK_CALL_THIS ComponentAllocator::RemoveAll()
				{
					const size_t size = m_size.load(std::memory_order_acquire);
					PageHeader* page = m_page;
					for (size_t a = 0; a < size; a += m_data.ComponentCount)
					{
						page->DeleteMask.store(~0Ui64, std::memory_order_release);
						page = page->Next;
					}
				}
				bool CLOAK_CALL_THIS ComponentAllocator::RequiresFix() const { return (m_toDelete.load(std::memory_order_consume) > 0 || m_validSize < m_size.load(std::memory_order_consume)) && m_ref.load(std::memory_order_consume) == 0; }
				void CLOAK_CALL_THIS ComponentAllocator::RemoveDeprecatedLinks()
				{
					const size_t size = m_size.load(std::memory_order_acquire);
					PageHeader* page = m_page;
					for (size_t a = 0; a < size; a += m_data.ComponentCount)
					{
						uint64_t linkMask = page->LinkMask.load(std::memory_order_acquire);
						uint8_t ni = 0;
						while ((ni = API::Global::Math::bitScanForward(linkMask)) != 0xFF)
						{
							const uint64_t deleteMask = 1Ui64 << ni;
							linkMask ^= deleteMask;
							if (IsFlagSet(page->DeleteMask.load(std::memory_order_relaxed), deleteMask) == false)
							{
								ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (ni * m_data.ComponentSize)));
								ComponentLink* link = reinterpret_cast<ComponentLink*>(Move(h, m_data.ComponentHeaderSize));
								ComponentAllocator* alloc = GetAllocator(link->Type);
								if (alloc != nullptr)
								{
									const SearchResult sr = alloc->FindComponent(h->Entity);
									if (sr.Found == false || IsFlagSet(sr.Page->DeleteMask.load(std::memory_order_acquire), 1Ui64 << sr.PageIndex))
									{
										page->DeleteMask.fetch_or(deleteMask);
										m_toDelete.fetch_add(1, std::memory_order_relaxed);
									}
								}
							}
						}
						page = page->Next.load(std::memory_order_acquire);
					}
				}
				void CLOAK_CALL_THIS ComponentAllocator::Fix()
				{
					if (m_ref.load(std::memory_order_consume) == 0)
					{
						const size_t size = m_size.load(std::memory_order_acquire);
						size_t validSize = m_validSize;
						const size_t deleteCount = m_toDelete.load(std::memory_order_acquire);
						CLOAK_ASSUME(size >= validSize);
						CLOAK_ASSUME(size >= deleteCount);
						const size_t finalSize = size - deleteCount;
						//Let's see if there are some entries already sorted, which would speed up the following algorithm. Also when we're at it, let's
						//call the destructors on any to-be-removed component or struct:
						if (validSize < size || deleteCount > 0)
						{
							PageHeader* page = m_page;
							Entity_v2::EntityID lID = 0;
							bool growSortedArea = validSize < size;
							for (size_t a = 0; a < size; a += m_data.ComponentCount)
							{
								const uint64_t delMask = page->DeleteMask.load(std::memory_order_relaxed);
								if (growSortedArea == true)
								{
									if (a + m_data.ComponentCount <= validSize)
									{
										const uint8_t f = API::Global::Math::bitScanReverse(~delMask);
										if (f != 0xFF && a + f < size)
										{
											ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (f * m_data.ComponentSize)));
											CLOAK_ASSUME(h->Entity >= lID);
											lID = h->Entity;
										}
									}
									else
									{
										uint8_t f = 0xFF;
										uint64_t del = delMask;
										while ((f = API::Global::Math::bitScanForward(~del)) != 0xFF && a + f < size)
										{
											del ^= 1Ui64 << f;
											ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (f * m_data.ComponentSize)));
											if (lID <= h->Entity)
											{
												lID = h->Entity;
												validSize = max(validSize, a + f);
											}
											else { growSortedArea = false; }
										}
									}
								}
								if (deleteCount > 0)
								{
									const uint64_t link = page->LinkMask.load(std::memory_order_relaxed);
									uint8_t f = 0xFF;
									uint64_t del = delMask;
									while ((f = API::Global::Math::bitScanForward(del) != 0xFF && a + f < size))
									{
										const uint64_t mask = 1Ui64 << f;
										del ^= mask;
										ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (f * m_data.ComponentSize)));
										if (IsFlagSet(link, mask))
										{
											ComponentLink* l = reinterpret_cast<ComponentLink*>(Move(h, m_data.ComponentHeaderSize));
											l->~ComponentLink();
										}
										else
										{
											API::World::Entity_v2::Component* c = reinterpret_cast<API::World::Entity_v2::Component*>(Move(h, m_data.ComponentHeaderSize + m_baseOffset));
											c->~Component();
										}
										h->~ComponentHeader();
									}
								}
								page = page->Next;
							}
						}

						if (finalSize == 0)
						{
							m_page->HighestEntity = 0;
							m_page->DeleteMask.store(0, std::memory_order_relaxed);
							m_page->LinkMask.store(0, std::memory_order_relaxed);
							//Free all other pages:
							PageHeader* p = m_page->Next.exchange(nullptr, std::memory_order_relaxed);
							while (p != nullptr)
							{
								PageHeader* n = p->Next;
								p->~PageHeader();
								API::Global::Memory::MemoryHeap::Free(p);
								p = n;
							}
						}
						else if (finalSize == 1)
						{
							//There's only one entry to keep, so we can skip any sorting right away
							PageHeader* page = m_page;
							for (size_t a = 0; a < size; a += m_data.ComponentCount)
							{
								const uint64_t del = page->DeleteMask.load(std::memory_order_acquire);
								const uint8_t f = API::Global::Math::bitScanForward(~del);
								if (f != 0xFF)
								{
									CLOAK_ASSUME(a + f < size);//If this failes, we got the deleteCount wrong!
									ComponentHeader* src = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (f * m_data.ComponentSize)));
									m_page->HighestEntity = src->Entity;
									m_page->LinkMask.store(IsFlagSet(page->LinkMask.load(std::memory_order_relaxed), 1Ui64 << f) ? 1 : 0, std::memory_order_relaxed);
									m_page->DeleteMask.store(0, std::memory_order_relaxed);
									if (a > 0 || f > 0)
									{
										void* dst = Move(m_page, m_data.PageHeaderSize);
										memcpy(dst, src, m_data.ComponentSize);
									}
									break;
								}
								page = page->Next;
							}
							//Free all other pages:
							page = m_page->Next.exchange(nullptr, std::memory_order_relaxed);
							while (page != nullptr)
							{
								PageHeader* n = page->Next;
								page->~PageHeader();
								API::Global::Memory::MemoryHeap::Free(page);
								page = n;
							}
						}
						else if (validSize < size)
						{
							//There are new entries, which are not yet sorted, so we need to sort all entries.
							//First, find all valid components that require sorting:
							SortData* data = reinterpret_cast<SortData*>(API::Global::Memory::MemoryLocal::Allocate(sizeof(SortData) * finalSize * 2));
							SortData* sorted[2] = { &data[0], &data[finalSize] };
							PageHeader* page = m_page;
							Entity_v2::EntityID maxEID = 0;
							Entity_v2::EntityID minEID = ~0;
							for (size_t a = 0, c = 0; a < size; a += m_data.ComponentCount)
							{
								const size_t m = min(size - a, m_data.ComponentCount);
								const uint64_t del = page->DeleteMask.load(std::memory_order_acquire);
								const uint64_t links = page->LinkMask.load(std::memory_order_acquire);
								for (size_t b = 0; b < m; b++)
								{
									const uint64_t mask = 1Ui64 << b;
									if (IsFlagSet(del, mask) == false)
									{
										ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (b * m_data.ComponentSize)));
										::new(&sorted[0][c])SortData();
										sorted[0][c].Page = page;
										sorted[0][c].PageIndex = b;
										sorted[0][c].IsLink = IsFlagSet(links, mask);
										sorted[0][c].Entity = h->Entity;
										maxEID = max(maxEID, h->Entity);
										minEID = min(minEID, h->Entity);
										c++;
									}
								}
								page = page->Next;
								CLOAK_ASSUME((a + m_data.ComponentCount >= size) == (c == finalSize)); //If this failes, we got the deleteCount wrong!
							}
							CLOAK_ASSUME(maxEID > minEID); //sanity check
							//Sorting using RADIX sort:
							uint32_t counting[16];
							const size_t iterations = API::Global::Math::bitScanReverse(maxEID - minEID) + 1;
							for (size_t a = 0; a < finalSize; a++) { sorted[0][a].Entity -= minEID; }
							for (size_t a = 0; a < iterations; a += 4)
							{
								memset(counting, 0, sizeof(uint32_t) * ARRAYSIZE(counting));
								for (size_t b = 0; b < finalSize; b++) { counting[(sorted[0][b].Entity >> a) & 0xF]++; }
								for (size_t b = 1; b < ARRAYSIZE(counting); b++) { counting[b] += counting[b - 1]; }
								for (size_t b = finalSize; b > 0; b--)
								{
									const uint32_t p = (sorted[0][b - 1].Entity >> a) & 0xF;
									sorted[1][--counting[p]] = sorted[0][b - 1];
								}
								std::swap(sorted[0], sorted[1]);
							}
							for (size_t a = 0; a < finalSize; a++) { sorted[0][a].Entity += minEID; }
							//Mark "empty" parts of a page as deleted for following algorithm:
							page = m_page;
							for (size_t a = 0; a < size; a += m_data.ComponentCount)
							{
								const size_t m = min(size - a, m_data.ComponentCount);
								if (m < 64) { page->DeleteMask.fetch_or((~0Ui64) << m, std::memory_order_acq_rel); }
								page = page->Next;
							}
							//Copy data:
							PageHeader* fop = nullptr;
							PageHeader* lop = nullptr;
							PageHeader* fnp = nullptr;
							PageHeader* lnp = nullptr;
							page = m_page;
							size_t wp = 0;
							size_t rp = 0;
							const size_t validPageSize = validSize - (validSize % m_data.ComponentCount);
							for (; rp < size; wp += m_data.ComponentCount)
							{
								CLOAK_ASSUME(page != nullptr);
								PageHeader* next = page->Next;
								rp += m_data.ComponentCount;
								const size_t nm = wp < finalSize ? min(finalSize - wp, m_data.ComponentCount) : 0;
								size_t wip = 0;
								uint64_t del = page->DeleteMask.load(std::memory_order_acquire);
								CLOAK_ASSUME(nm != 0 || del == ~0Ui64);
								uint8_t p = 0;
								bool foundOne = false;
								while ((p = API::Global::Math::bitScanForward(~del)) != 0xFF)
								{
									del ^= 1Ui64 << p;
									ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (p * m_data.ComponentSize)));
									if (rp < validPageSize)
									{
										//We are evaluating an already sorted page, so we can run over the sorted array in parallel -> O(n)
										for (size_t b = wip; b < nm; b++)
										{
											wip++;
											if (sorted[0][wp + b].Entity == h->Entity) { goto component_found; }
										}
									}
									else
									{
										//We are evaluating an unsorted page, so we need to search the whole area within the sorted array -> O(n²)
										for (size_t b = 0; b < nm; b++)
										{
											if (sorted[0][wp + b].Entity == h->Entity) { goto component_found; }
										}
									}
									//Page can not be reused, as there is at least one component that needs to be moved to a later page. So create a new page!
									PageHeader* np = nullptr;
									//First, check if we can use an older page:
									PageHeader* op = fop;
									PageHeader* lp = nullptr;
									while (op != nullptr)
									{
										if (op->DeleteMask.load(std::memory_order_acquire) == ~0Ui64)
										{
											np = op;
											PageHeader* lpn = op->Next.load(std::memory_order_acquire);
											if (lp != nullptr) { lp->Next = lpn; }
											else { fop = lpn; }
											if (lpn == nullptr) { lop = lp; }
											goto copy_to_new_page;
										}
										lp = op;
										op = op->Next;
									}
									np = ::new(API::Global::Memory::MemoryHeap::Allocate(m_data.PageSize))PageHeader();
								copy_to_new_page:
									uint64_t nlink = 0;
									for (size_t b = 0; b < nm; b++)
									{
										const SortData& s = sorted[0][b + wp];
										if (s.IsLink == true) { nlink |= 1Ui64 << b; }
										void* src = Move(s.Page, m_data.PageSize + (s.PageIndex * m_data.ComponentSize));
										void* dst = Move(np, m_data.PageSize + (b * m_data.ComponentSize));
										memcpy(dst, src, m_data.ComponentSize);
										s.Page->DeleteMask.fetch_or(1Ui64 << s.PageIndex, std::memory_order_acq_rel);
									}

									//Add new page to list of new pages:
									np->HighestEntity = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageSize + ((nm - 1) * m_data.ComponentSize)))->Entity;
									np->LinkMask = nlink;
									np->DeleteMask = 0;
									np->Next = nullptr;
									if (lnp == nullptr) { fnp = np; }
									else { lnp->Next = np; }
									lnp = np;
									if (foundOne == true)
									{
										//Add old page to list of old pages:
										if (lop == nullptr) { fop = page; }
										else { lop->Next = page; }
										lop = page;
										page->Next = nullptr;
									}
									else
									{
										//Page hasn't been used at all yet, so it may be used in the next iteration
										next = page;
										rp -= m_data.ComponentCount;
									}
									goto next_page;
								component_found:
									foundOne = true;
								}
								uint64_t nlink = 0;
								//First move to tmp page:
								if (nm > 0)
								{
									void* tmpPage = nullptr;
									if (page->DeleteMask.load(std::memory_order_acquire) != ~0Ui64)
									{
										tmpPage = API::Global::Memory::MemoryLocal::Allocate(m_data.PageSize - m_data.PageHeaderSize);
										memcpy(tmpPage, Move(page, m_data.PageHeaderSize), m_data.PageSize - m_data.PageHeaderSize);
									}
									//Then copy:
									for (size_t b = 0; b < nm; b++)
									{
										const SortData& s = sorted[0][b + wp];
										if (s.IsLink == true) { nlink |= 1Ui64 << b; }
										void* src = nullptr;
										void* dst = Move(page, m_data.PageHeaderSize + (b * m_data.ComponentSize));
										if (s.Page == page)
										{
											src = Move(tmpPage, s.PageIndex * m_data.ComponentSize);
										}
										else
										{
											src = Move(s.Page, m_data.PageHeaderSize + (s.PageIndex * m_data.ComponentSize));
											s.Page->DeleteMask.fetch_or(1Ui64 << s.PageIndex, std::memory_order_acq_rel);
										}
										memcpy(dst, src, m_data.ComponentSize);
									}
									API::Global::Memory::MemoryLocal::Free(tmpPage);

									//Add page to list of new pages:
									page->HighestEntity = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageSize + ((nm - 1) * m_data.ComponentSize)))->Entity;
									page->LinkMask = nlink;
									page->DeleteMask = 0;
									page->Next = nullptr;
									if (lnp == nullptr) { fnp = page; }
									else { lnp->Next = page; }
									lnp = page;
								}
								else
								{
									//Page is not required, let's add it to the list of old pages
									if (lop == nullptr) { fop = page; }
									else { lop->Next = page; }
									lop = page;
									page->Next = nullptr;
								}
							next_page:
								page = next;
							}

							//Free old pages:
							if (page != nullptr)
							{
								if (lop == nullptr) { fop = page; }
								else { lop->Next = page; }
							}
							while (fop != nullptr)
							{
								PageHeader* n = fop->Next;
								fop->~PageHeader();
								API::Global::Memory::MemoryHeap::Free(fop);
								fop = n;
							}
							m_page = fnp;
							API::Global::Memory::MemoryLocal::Free(data);
						}
						else
						{
							//We just need to remove deleted entries, no sorting is required. Therefore, it is save to do this inline:
							PageHeader* readPage = m_page;
							PageHeader* writePage = m_page;
							for (size_t read = 0, write = 0; read < size;)
							{
								CLOAK_ASSUME(write < finalSize);
								const size_t m = min(finalSize - write, m_data.ComponentCount);
								uint64_t nlink = 0;
								Entity_v2::EntityID mEID = 0;
								for (size_t wi = 0; wi < m; wi++, read++)
								{
									uint64_t del = readPage->DeleteMask.load(std::memory_order_acquire);
									uint64_t ri = read % m_data.ComponentCount;
									while (IsFlagSet(del, 1Ui64 << ri))
									{
										read++;
										ri++;
										if (ri == m_data.ComponentCount)
										{
											readPage = readPage->Next;
											del = readPage->DeleteMask.load(std::memory_order_acquire);
											ri = 0;
										}
									}
									CLOAK_ASSUME(read <= write + wi);
									CLOAK_ASSUME(read < size);
									if (IsFlagSet(readPage->LinkMask.load(std::memory_order_acquire), 1Ui64 << ri)) { nlink |= 1Ui64 << wi; }
									void* src = Move(readPage, m_data.PageHeaderSize + (ri * m_data.ComponentSize));
									if (wi + 1 == m) { mEID = reinterpret_cast<ComponentHeader*>(src)->Entity; }
									if (read != write + wi)
									{
										void* dst = Move(writePage, m_data.PageHeaderSize + (wi * m_data.ComponentSize));
										memcpy(dst, src, m_data.ComponentSize);
									}
								}
								writePage->LinkMask.store(nlink, std::memory_order_release);
								writePage->DeleteMask.store(0, std::memory_order_release);
								writePage->HighestEntity = mEID;
								writePage = writePage->Next;
								write += m;
								CLOAK_ASSUME((read == size) == (write == finalSize)); //If this failes, we got the deleteCount wrong!
							}
							//Delete all other pages:
							if (writePage != nullptr)
							{
								writePage = writePage->Next.exchange(nullptr, std::memory_order_release);
								while (writePage != nullptr)
								{
									PageHeader* n = writePage->Next;
									writePage->~PageHeader();
									API::Global::Memory::MemoryHeap::Free(writePage);
									writePage = n;
								}
							}
						}
						m_validSize = finalSize;
						m_toDelete.store(0, std::memory_order_release);
						m_size.store(finalSize, std::memory_order_release);
#ifdef _DEBUG
						//Final check to see if everything worked out:
						{
							PageHeader* page = m_page;
							Entity_v2::EntityID lid = 0;
							for (size_t a = 0; a < finalSize; a += m_data.ComponentCount)
							{
								CLOAK_ASSUME(page != nullptr);
								CLOAK_ASSUME(page->DeleteMask.load(std::memory_order_acquire) == 0);
								const size_t rem = min(finalSize - a, m_data.ComponentCount);
								for (size_t b = 0; b < rem; b++)
								{
									ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (b * m_data.ComponentSize)));
									CLOAK_ASSUME((a == 0 && b == 0 && lid == h->Entity) || lid < h->Entity);
									lid = h->Entity;
								}
								page = page->Next;
							}
						}
#endif
					}
				}

				CLOAK_CALL ComponentAllocator::ComponentAllocator(In const type_name& name, In size_t size, In size_t alignment, In size_t offsetToBase) : m_type(name), m_baseOffset(offsetToBase), m_data(size, alignment)
				{
					m_ref = 0;
					m_size = 0;
					m_validSize = 0;
					m_toDelete = 0;
					m_page = ::new(API::Global::Memory::MemoryHeap::Allocate(m_data.PageSize))PageHeader();
				}
				CLOAK_CALL ComponentAllocator::~ComponentAllocator()
				{
					PageHeader* page = m_page;
					size_t p = 0;
					const size_t s = m_size.load(std::memory_order_consume);
					do {
						PageHeader* np = page->Next;
						const size_t size = min(m_data.ComponentCount, s > p ? s - p : 0);
						ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(np, m_data.PageHeaderSize));
						for (size_t a = 0; a < size; a++)
						{
							if (IsFlagSet(np->LinkMask.load(std::memory_order_consume), 1Ui64 << a))
							{
								ComponentLink* l = reinterpret_cast<ComponentLink*>(Move(h, m_data.ComponentHeaderSize));
								l->~ComponentLink();
							}
							else
							{
								API::World::Entity_v2::Component* c = reinterpret_cast<API::World::Entity_v2::Component*>(Move(h, m_data.ComponentHeaderSize + m_baseOffset));
								c->~Component();
							}
							h->~ComponentHeader();
							h = reinterpret_cast<ComponentHeader*>(Move(h, m_data.ComponentSize));
						}
						np->~PageHeader();
						API::Global::Memory::MemoryHeap::Free(np);
						page = np;
						p += m_data.ComponentCount;
					} while (page != nullptr);
				}
				void* CLOAK_CALL_THIS ComponentAllocator::AllocateComponentLink(In Entity_v2::EntityID id, In const type_name& realType, In size_t offset)
				{
					if (g_blockAdd.load(std::memory_order_relaxed) == true) { return nullptr; }
					PageHeader* page = m_page;
					PageHeader* lp = nullptr;
					void* np = nullptr;
					const size_t size = m_size.fetch_add(1, std::memory_order_acq_rel);
					const size_t index = size % m_data.ComponentCount;
					for (size_t a = m_data.ComponentCount; a <= size; a += m_data.ComponentCount)
					{
						lp = page;
						page = page->Next.load(std::memory_order_acquire);
						if (page == nullptr)
						{
							if (np == nullptr) { np = API::Global::Memory::MemoryHeap::Allocate(m_data.PageSize); }
							page = ::new(np)PageHeader();
							PageHeader* ep = nullptr;
							if (lp->Next.compare_exchange_strong(ep, page, std::memory_order_acq_rel) == true) { np = nullptr; }
							else
							{
								page->~PageHeader();
								page = ep;
							}
						}
					}
					if (np != nullptr) { API::Global::Memory::MemoryHeap::Free(np); }
					ComponentHeader* h = reinterpret_cast<ComponentHeader*>(Move(page, m_data.PageHeaderSize + (index * m_data.ComponentSize)));
					h = ::new(h)ComponentHeader(id);
					void* res = Move(h, m_data.ComponentHeaderSize);
					if (realType != m_type)
					{
						page->LinkMask.fetch_or(1Ui64 << index, std::memory_order_relaxed);
						ComponentLink* link = reinterpret_cast<ComponentLink*>(res);
						link = ::new(link)ComponentLink(realType, offset);
					}
					return res;
				}
				void* CLOAK_CALL_THIS ComponentAllocator::AllocateComponent(In Entity_v2::EntityID id) { return AllocateComponentLink(id, m_type, 0); }
				void CLOAK_CALL_THIS ComponentAllocator::RemoveComponent(In Entity_v2::EntityID id)
				{
					const SearchResult sr = FindComponentUnsorted(id);
					if (sr.Found == false) { return; }
					const uint64_t mask = 1Ui64 << sr.PageIndex;
					if (IsFlagSet(sr.Page->DeleteMask.fetch_or(mask, std::memory_order_relaxed), mask) == false) { m_toDelete.fetch_add(1, std::memory_order_relaxed); }
					if (IsFlagSet(sr.Page->LinkMask.load(std::memory_order_relaxed), mask))
					{
						ComponentLink* l = reinterpret_cast<ComponentLink*>(Move(sr.Page, m_data.PageHeaderSize + (sr.PageIndex * m_data.ComponentSize) + m_data.ComponentHeaderSize));
						ComponentAllocator* alloc = GetAllocator(l->Type);
						if (alloc != nullptr) { alloc->RemoveComponent(id); }
					}
				}
				void* CLOAK_CALL_THIS ComponentAllocator::GetComponent(In Entity_v2::EntityID id)
				{
					const SearchResult sr = FindComponentUnsorted(id);
					if (sr.Found == true)
					{
						void* res = Move(sr.Page, m_data.PageHeaderSize + (sr.PageIndex * m_data.ComponentSize) + m_data.ComponentHeaderSize);
						const uint64_t mask = 1Ui64 << sr.PageIndex;
						if (IsFlagSet(sr.Page->LinkMask.load(std::memory_order_relaxed), mask))
						{
							ComponentLink* link = reinterpret_cast<ComponentLink*>(res);
							ComponentAllocator* o = GetAllocator(link->Type);
							if (o != nullptr)
							{
								void* nc = o->GetComponent(id);
								if (nc != nullptr) { return Move(nc, link->Offset); }
							}
							if (IsFlagSet(sr.Page->DeleteMask.fetch_or(mask, std::memory_order_relaxed), mask) == false) { m_toDelete.fetch_add(1, std::memory_order_relaxed); }
							return nullptr;
						}
						return res;
					}
					return nullptr;
				}

				const type_name& CLOAK_CALL_THIS ComponentAllocator::GetTypeName() const { return m_type; }

				void CLOAK_CALL_THIS ComponentAllocator::AddRef() { m_ref.fetch_add(1, std::memory_order_acq_rel); }
				void CLOAK_CALL_THIS ComponentAllocator::Release() { m_ref.fetch_sub(1, std::memory_order_acq_rel); }

				ComponentAllocator::iterator CLOAK_CALL_THIS ComponentAllocator::begin() const { return iterator(this, m_page, 0); }
				ComponentAllocator::iterator CLOAK_CALL_THIS ComponentAllocator::cbegin() const { return iterator(this, m_page, 0); }
				ComponentAllocator::iterator CLOAK_CALL_THIS ComponentAllocator::end() const { return iterator(); }
				ComponentAllocator::iterator CLOAK_CALL_THIS ComponentAllocator::cend() const { return iterator(); }

				void* CLOAK_CALL ComponentAllocator::operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
				void CLOAK_CALL ComponentAllocator::operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }
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

				API::Helper::ReadLock rlock(Impl::World::ComponentManager::g_syncCache);
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

				API::Helper::WriteLock wlock(Impl::World::ComponentManager::g_syncCache);
				Impl::World::ComponentManager::g_cache->insert({ res });
				res->AddRef();
				return res;
			}
			namespace ComponentManager_v2 {
				CLOAKENGINE_API CE::RefPointer<IComponentCache> CLOAK_CALL ComponentManager::RequestCache(In size_t count, In_reads(count) const type_name* ids)
				{
					return Impl::World::ComponentManager_v2::ComponentCache::CreateCache(count, ids);
				}
			}
		}
	}	
}