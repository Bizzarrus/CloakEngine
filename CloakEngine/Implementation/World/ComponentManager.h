#pragma once
#ifndef CE_IMPL_WORLD_COMPONENTMANAGER_H
#define CE_IMPL_WORLD_COMPONENTMANAGER_H

#include "CloakEngine/World/ComponentManager.h"
#include "Implementation/World/Entity.h"

namespace CloakEngine {
	namespace Impl {
		namespace World {
			namespace ComponentManager {
				uint32_t CLOAK_CALL GetID(In const type_id& guid);
				void* CLOAK_CALL Allocate(In const type_id& id, In size_t size, In size_t alignment);
				void CLOAK_CALL Free(In const type_id& id, In void* ptr);
				void CLOAK_CALL Initialize();
				void CLOAK_CALL ReleaseSyncs();
				void CLOAK_CALL Release();
				void* CLOAK_CALL AllocateEntity(In size_t size);
				void CLOAK_CALL FreeEntity(In void* ptr);
				void CLOAK_CALL UpdateEntityCache(Entity* e, In bool remove);
			}
			namespace ComponentManager_v2 {
				class ComponentAllocator {
					public:
						static void CLOAK_CALL Initialize();
						static void CLOAK_CALL Shutdown();
						static void CLOAK_CALL ReleaseSyncs();
						static void CLOAK_CALL Terminate();
						static ComponentAllocator* CLOAK_CALL GetAllocator(In const type_name& type);
						static ComponentAllocator* CLOAK_CALL GetOrCreateAllocator(In const type_name& type, In size_t size, In size_t alignment, In size_t offsetToBase);
						static void CLOAK_CALL FillAllocators(In size_t count, In_reads(count) const type_name* ids, Out_writes(count) ComponentAllocator** result);
						static void CLOAK_CALL RemoveAll(In Entity_v2::EntityID id);
						static void CLOAK_CALL FixAll(In size_t threadID, In size_t followCount, In_reads(followCount) API::Global::Task* followTasks);
						static void CLOAK_CALL LockComponentAccess(In size_t threadID);
						static void CLOAK_CALL UnlockComponentAccess(In size_t threadID);
					private:
						struct PageHeader {
							Entity_v2::EntityID HighestEntity = 0;
							std::atomic<uint64_t> LinkMask = 0; // 0 = Component is real data, 1 = Component is Alias
							std::atomic<uint64_t> DeleteMask = 0; // 0 = Component is fine, 1 = Component is to be deleted
							std::atomic<PageHeader*> Next = nullptr;
						};
						struct ComponentHeader {
							const Entity_v2::EntityID Entity;
							CLOAK_CALL ComponentHeader(In Entity_v2::EntityID id);
						};
						struct ComponentLink {
							const type_name Type;
							const size_t Offset;
							CLOAK_CALL ComponentLink(In const type_name& type, In size_t offset);
						};
						struct PageData {
							static constexpr size_t MAX_PAGE_SIZE = 512 KB;
							size_t ComponentSize; // Full component size, including component header
							size_t ComponentHeaderSize;
							size_t ComponentCount;
							size_t PageHeaderSize;
							size_t PageSize; // Full page size, including header

							CLOAK_CALL PageData(In size_t size, In size_t alignment);
						};
						struct SearchResult {
							bool Found;
							size_t GlobalIndex;
							size_t PageIndex;
							PageHeader* Page;
							CLOAK_CALL SearchResult(In bool found, In size_t globalIndex, In size_t pageIndex, In PageHeader* page);
						};
						struct SortData {
							Entity_v2::EntityID Entity;
							PageHeader* Page;
							struct {
								uint8_t PageIndex : 6;
								uint8_t IsLink : 1;
							};
						};
					public:
						class iterator {
							friend class ComponentAllocator;
							public:
								typedef std::forward_iterator_tag iterator_category;
								typedef size_t difference_type;
								typedef size_t size_type;
								typedef std::pair<Entity_v2::EntityID, void*> value_type;
								typedef value_type reference;
							private:
								const ComponentAllocator* m_alloc;
								PageHeader* m_page;
								size_t m_pos;
								size_t m_pi;

								CLOAK_CALL iterator(In const ComponentAllocator* alloc, In PageHeader* page, In size_t position);
							public:
								CLOAK_CALL iterator();
								CLOAK_CALL iterator(In const iterator& o);
								CLOAK_CALL iterator(In iterator&& o);

								iterator& CLOAK_CALL_THIS operator=(In const iterator& o);
								iterator& CLOAK_CALL_THIS operator=(In iterator&& o);

								iterator& CLOAK_CALL_THIS operator+=(In difference_type n);
								iterator& CLOAK_CALL_THIS operator++();
								iterator CLOAK_CALL_THIS operator++(In int);
								iterator CLOAK_CALL_THIS operator+(In difference_type n) const;
								friend iterator CLOAK_CALL operator+(In difference_type n, In const iterator& o);
								inline reference CLOAK_CALL_THIS operator[](In difference_type n) const;
								inline reference CLOAK_CALL_THIS operator*() const;

								bool CLOAK_CALL_THIS operator<(In const iterator& i) const;
								bool CLOAK_CALL_THIS operator<=(In const iterator& i) const;
								bool CLOAK_CALL_THIS operator>(In const iterator& i) const;
								bool CLOAK_CALL_THIS operator>=(In const iterator& i) const;
								bool CLOAK_CALL_THIS operator==(In const iterator& i) const;
								bool CLOAK_CALL_THIS operator!=(In const iterator& i) const;
						};
					private:
						const size_t m_baseOffset;
						const PageData m_data;
						const type_name m_type;
						size_t m_validSize;
						PageHeader* m_page;
						std::atomic<size_t> m_size;
						std::atomic<size_t> m_toDelete;
						std::atomic<uint64_t> m_ref;

						inline SearchResult CLOAK_CALL_THIS FindComponent(In Entity_v2::EntityID id);
						inline SearchResult CLOAK_CALL_THIS FindComponentUnsorted(In Entity_v2::EntityID id);
						inline void CLOAK_CALL_THIS RemoveAll();
					public:
						bool CLOAK_CALL_THIS RequiresFix() const;
						void CLOAK_CALL_THIS RemoveDeprecatedLinks();
						void CLOAK_CALL_THIS Fix();
					private:
						CLOAK_CALL ComponentAllocator(In const type_name& name, In size_t size, In size_t alignment, In size_t offsetToBase);
					public:
						CLOAK_CALL ~ComponentAllocator();
						void* CLOAK_CALL_THIS AllocateComponentLink(In Entity_v2::EntityID id, In const type_name& realType, In size_t offset);
						void* CLOAK_CALL_THIS AllocateComponent(In Entity_v2::EntityID id);
						void CLOAK_CALL_THIS RemoveComponent(In Entity_v2::EntityID id);
						void* CLOAK_CALL_THIS GetComponent(In Entity_v2::EntityID id);

						const type_name& CLOAK_CALL_THIS GetTypeName() const;

						void CLOAK_CALL_THIS AddRef();
						void CLOAK_CALL_THIS Release();

						iterator CLOAK_CALL_THIS begin() const;
						iterator CLOAK_CALL_THIS cbegin() const;
						iterator CLOAK_CALL_THIS end() const;
						iterator CLOAK_CALL_THIS cend() const;

						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
				};
			}
		}
	}
}

#endif