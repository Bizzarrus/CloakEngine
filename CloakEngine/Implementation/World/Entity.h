#pragma once
#ifndef CE_IMPL_WORLD_ENTITY_H
#define CE_IMPL_WORLD_ENTITY_H

#include "CloakEngine/World/Entity.h"

#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Helper/EventBase.h"

#include <tuple>

#pragma warning(push)
#pragma warning(disable: 4250)
#pragma warning(disable: 4996)

namespace CloakEngine {
	namespace Impl {
		namespace World {
			inline namespace Entity_v1 {
				class CLOAK_UUID("{CDE99A8A-1D93-4E7D-A88D-CF35D568E25F}") Entity : public virtual API::World::Entity_v1::IEntity, public Impl::Helper::SavePtr_v1::SavePtr, IMPL_EVENT(onLoad), IMPL_EVENT(onUnload) {
					public:
						CLOAK_CALL Entity();
						virtual CLOAK_CALL ~Entity();

						virtual ULONG CLOAK_CALL_THIS load(In_opt API::Files::Priority prio = API::Files::Priority::LOW) override;
						virtual ULONG CLOAK_CALL_THIS unload() override;
						virtual bool CLOAK_CALL_THIS isLoaded(In API::Files::Priority prio) override;
						virtual bool CLOAK_CALL_THIS isLoaded() const override;
						virtual void CLOAK_CALL_THIS RequestLOD(In API::Files::LOD lod) override;

						virtual API::Helper::WriteLock CLOAK_CALL_THIS LockWrite() const override;
						virtual API::Helper::ReadLock CLOAK_CALL_THIS LockRead() const override;

						virtual bool CLOAK_CALL_THIS HasComponents(In const API::BitSet& set, In size_t num, In_reads(num) const type_id* ids, Out_writes(num) std::pair<void*, API::World::Component*>* components);
						virtual void CLOAK_CALL_THIS Entity::SetScene(In API::Files::IScene* scene, In_opt API::Files::IWorldNode* node = nullptr);

						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						virtual void* CLOAK_CALL_THIS CreateComponent(In const type_id& guid, In_opt API::World::Component* src = nullptr) override;
						virtual void CLOAK_CALL_THIS FreeComponent(In API::World::Component* co) override;
						virtual void* CLOAK_CALL_THIS ObtainComponent(In const type_id& guid) const override;
						virtual bool CLOAK_CALL_THIS ContainsComponent(In const type_id& guid) const override;
						virtual void CLOAK_CALL_THIS ReleaseComponent(In const type_id& guid) override;
						virtual void* CLOAK_CALL_THIS AllocateComponent(In const type_id& guid, In size_t size, In size_t alignment, In ptrdiff_t offset, Inout API::Helper::Lock* lock) override;
						virtual void CLOAK_CALL_THIS SetAlias(In const type_id& guid, In API::World::Component* src, In void* castedPtr) override;
						virtual void CLOAK_CALL_THIS OnComponentUpdated() override;
						virtual void CLOAK_CALL_THIS InitComponent(In API::World::Component* co) override;

						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						API::Helper::ISyncSection* m_sync = nullptr;
						API::BitSet m_componentBits;
						std::atomic<ULONG> m_loaded;
						API::Files::LOD m_lod;
						API::Files::Priority m_loadPrio;
						//Maps ID -> [Casted Pointer | Component | Memory Pointer]
						API::FlatMap<type_id, std::tuple<void*, API::World::Component*, void*>> m_components;
						//Maps Component -> [Memory Pointer | ID]
						API::FlatMap<API::World::Component*, std::pair<void*, type_id>> m_toRemove;
				};
			}
			namespace Entity_v2 {
				typedef uint64_t EntityID;
				class CLOAK_UUID("{89F09229-0DA2-482E-B354-EF891B97510C}") Entity : public virtual API::World::Entity_v2::IEntity {
					public:
						CLOAK_CALL Entity();
						CLOAK_CALL ~Entity();

						uint64_t CLOAK_CALL_THIS AddRef() override;
						uint64_t CLOAK_CALL_THIS Release() override;

						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						void CLOAK_CALL_THIS SetAlias(In const type_name & aliasID, In const type_name & realID, In size_t offsetToAlias, In size_t size, In size_t alignment, In size_t offsetToBase) override;
						void* CLOAK_CALL_THIS GetComponent(In const type_name & id) const override;
						void CLOAK_CALL_THIS RemoveComponent(In const type_name & id) override;
						void* CLOAK_CALL_THIS AllocateComponent(In const type_name & id, In size_t size, In size_t alignment, In size_t offset, Out void** type) override;
						void* CLOAK_CALL_THIS CreateComponent(In const type_name & id, In void* src) override;
						void CLOAK_CALL_THIS ComponentAddRef() override;
						void CLOAK_CALL_THIS ComponentRelease() override;
					
						const EntityID m_id;
						std::atomic<uint64_t> m_ref;
						std::atomic<uint64_t> m_cref;
						std::atomic<bool> m_blocked;
				};

				void CLOAK_CALL Initialize();
				void CLOAK_CALL ReleaseSyncs();
				void CLOAK_CALL Terminate();
			}
		}
	}
}

#pragma warning(pop)

#endif