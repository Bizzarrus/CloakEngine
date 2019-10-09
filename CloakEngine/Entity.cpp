#include "stdafx.h"
#include "Implementation/World/Entity.h"
#include "Implementation/Rendering/Manager.h"
#include "Implementation/World/ComponentManager.h"
#include "Implementation/Global/Memory.h"

#include "CloakEngine/Helper/SyncSection.h"

#include "Implementation/Components/Render.h"
#include "Implementation/Components/Position.h"
#include "Implementation/Components/Moveable.h"
#include "Implementation/Components/Light.h"
#include "Implementation/Components/Camera.h"


namespace CloakEngine {
	namespace Impl {
		namespace World {
			namespace Entity_v1 {
				CLOAK_CALL Entity::Entity()
				{
					DEBUG_NAME(Entity);
					m_lod = API::Files::LOD::LOW;
					m_loadPrio = API::Files::Priority::LOW;
					m_loaded = 0;
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));

					INIT_EVENT(onUnload, m_sync);
					INIT_EVENT(onLoad, m_sync);
				}
				CLOAK_CALL Entity::~Entity()
				{
					SAVE_RELEASE(m_sync);
					Impl::World::ComponentManager::UpdateEntityCache(this, true);
					for (const auto& a : m_components)
					{
						if (std::get<2>(a.second) != nullptr)
						{
							std::get<1>(a.second)->~Component();
							Impl::World::ComponentManager::Free(a.first, std::get<2>(a.second));
						}
					}
					for (const auto& a : m_toRemove)
					{
						a.first->~Component();
						Impl::World::ComponentManager::Free(a.second.second, a.second.first);
					}
				}

				ULONG CLOAK_CALL_THIS Entity::load(In_opt API::Files::Priority prio)
				{
					const ULONG r = m_loaded.fetch_add(1) + 1;
					if (r == 1)
					{
						API::Helper::Lock lock(m_sync);
						m_loadPrio = prio;
						for (const auto& a : m_components) { std::get<1>(a.second)->OnLoad(prio, m_lod); }
						OnComponentUpdated();
					}
					else
					{
						API::Helper::Lock lock(m_sync);
						if (static_cast<size_t>(prio) > static_cast<size_t>(m_loadPrio))
						{
							m_loadPrio = prio;
							for (const auto& a : m_components) { std::get<1>(a.second)->OnSetLoadPriority(prio); }
						}
					}
					return r;
				}
				ULONG CLOAK_CALL_THIS Entity::unload()
				{
					const ULONG r = m_loaded.fetch_sub(1);
					if (r == 1)
					{
						API::Helper::Lock lock(m_sync);
						m_lod = API::Files::LOD::LOW;
						m_loadPrio = API::Files::Priority::LOW;
						for (const auto& a : m_components) { std::get<1>(a.second)->OnUnload(); }
						OnComponentUpdated();
					}
					else if (r == 0) { m_loaded = 0; }
					return r;
				}
				bool CLOAK_CALL_THIS Entity::isLoaded(In API::Files::Priority prio)
				{
					if (m_loaded == 0) { return false; }
					API::Helper::Lock lock(m_sync);
					if (static_cast<size_t>(prio) > static_cast<size_t>(m_loadPrio))
					{
						m_loadPrio = prio;
						for (const auto& a : m_components) { std::get<1>(a.second)->OnSetLoadPriority(prio); }
					}
					return true;
				}
				bool CLOAK_CALL_THIS Entity::isLoaded() const { return m_loaded > 0; }
				void CLOAK_CALL_THIS Entity::RequestLOD(In API::Files::LOD lod)
				{
					API::Helper::Lock lock(m_sync);
					if (static_cast<size_t>(lod) < static_cast<size_t>(m_lod))
					{
						m_lod = lod;
						if (m_loaded > 0)
						{
							for (const auto& a : m_components) { std::get<1>(a.second)->OnSetLoadLOD(lod); }
						}
					}
				}

				void* CLOAK_CALL_THIS Entity::AllocateComponent(In const type_id& guid, In size_t size, In size_t alignment, In ptrdiff_t offset, Inout API::Helper::Lock* lock)
				{
					CLOAK_ASSUME(lock != nullptr);
					lock->unlock();
					lock->lock(m_sync);

					if (m_components.find(guid) == m_components.end())
					{
						void* r = Impl::World::ComponentManager::Allocate(guid, size, alignment);
						API::World::Component* comp = reinterpret_cast<API::World::Component*>(reinterpret_cast<uintptr_t>(r) + offset);
						m_components[guid] = std::make_tuple(r, comp, r);
						m_componentBits[Impl::World::ComponentManager::GetID(guid)] = true;
					}
					return nullptr;
				}
				void CLOAK_CALL_THIS Entity::SetAlias(In const type_id& guid, In API::World::Component* src, In void* castedPtr)
				{
					auto f = m_components.find(guid);
					if (f == m_components.end())
					{
						m_components.insert(std::make_pair(guid, std::make_tuple(castedPtr, src, nullptr)));
						m_componentBits[Impl::World::ComponentManager::GetID(guid)] = true;
					}
					else
					{
						CloakDebugCheckOK(std::get<0>(f->second) == castedPtr, API::Global::Debug::Error::COMPONENT_ALREADY_EXISTS, false);
					}
				}
				void CLOAK_CALL_THIS Entity::FreeComponent(In API::World::Component* co)
				{
					if (co == nullptr) { return; }
					API::Helper::Lock lock(m_sync);
					if (m_toRemove.empty()) { return; }
					auto f = m_toRemove.find(co);
					if (f == m_toRemove.end()) { return; }
					API::World::Component* c = f->first;
					if (c->IsActive()) { c->OnDisable(); }
					c->~Component();
					Impl::World::ComponentManager::Free(f->second.second, f->second.first);
					m_toRemove.erase(f);
				}
				void* CLOAK_CALL_THIS Entity::ObtainComponent(In const type_id& guid) const
				{
					API::Helper::Lock lock(m_sync);
					auto f = m_components.find(guid);
					if (f == m_components.end()) { return nullptr; }
					return std::get<0>(f->second);
				}
				bool CLOAK_CALL_THIS Entity::ContainsComponent(In const type_id& guid) const
				{
					API::Helper::Lock lock(m_sync);
					auto f = m_components.find(guid);
					return f != m_components.end();
				}
				void CLOAK_CALL_THIS Entity::ReleaseComponent(In const type_id& guid)
				{
					API::Helper::Lock lock(m_sync);
					auto f = m_components.find(guid);
					if (f != m_components.end())
					{
						API::List<decltype(f)> removes;
						for (auto a = m_components.begin(); a != m_components.end(); ++a)
						{
							if (std::get<1>(a->second) == std::get<1>(f->second))
							{
								if (std::get<2>(a->second) != nullptr && std::get<2>(f->second) == nullptr) { f = a; }
								m_componentBits[Impl::World::ComponentManager::GetID(a->first)] = false;
								removes.push_back(a);
							}
						}
						CLOAK_ASSUME(std::get<2>(f->second) != nullptr);
						API::World::Component* c = std::get<1>(f->second);
						c->AddRef();
						m_toRemove.insert(std::make_pair(c, std::make_pair(std::get<2>(f->second), f->first)));
						for (const auto& a : removes) { m_components.erase(a); }
						c->Release();
					}
				}
				void CLOAK_CALL_THIS Entity::OnComponentUpdated() { Impl::World::ComponentManager::UpdateEntityCache(this, false); }
				void CLOAK_CALL_THIS Entity::InitComponent(In API::World::Component* co)
				{
					API::Helper::Lock lock(m_sync);
					if (co->IsActive()) { co->OnEnable(); }
					if (isLoaded()) { co->OnLoad(m_loadPrio, m_lod); }
				}

				API::Helper::WriteLock CLOAK_CALL_THIS Entity::LockWrite() const { return API::Helper::WriteLock(m_sync); }
				API::Helper::ReadLock CLOAK_CALL_THIS Entity::LockRead() const { return API::Helper::ReadLock(m_sync); }

				void CLOAK_CALL_THIS Entity::SetScene(In API::Files::IScene* scene, In_opt API::Files::IWorldNode* node)
				{
					API::Helper::WriteLock lock(m_sync);
					for (const auto& a : m_components) { std::get<1>(a.second)->OnNodeChange(scene, node); }
				}
				bool CLOAK_CALL_THIS Entity::HasComponents(In const API::BitSet& set, In size_t num, In_reads(num) const type_id* ids, Out_writes(num) std::pair<void*, API::World::Component*>* components)
				{
					API::Helper::ReadLock lock(m_sync);
					if (m_componentBits.contains(set))
					{
						for (size_t a = 0; a < num; a++) 
						{ 
							const auto& b = m_components[ids[a]];
							if (std::get<1>(b)->IsActive() == false) { return false; }
							components[a] = std::make_pair(std::get<0>(b), std::get<1>(b));
						}
						return true;
					}
					return false;
				}
				
				void* CLOAK_CALL Entity::operator new(In size_t size) { return Impl::World::ComponentManager::AllocateEntity(size); }
				void CLOAK_CALL Entity::operator delete(In void* ptr) { Impl::World::ComponentManager::FreeEntity(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS Entity::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SavePtr, World::Entity_v1, Entity);
				}

#define CHECK_COMPONENT_BEGIN(uuid, dstComp, dstPtr, cs) { const type_id& _guid = (uuid); API::World::Component** _dst = &(dstComp); void** _dstP = &(dstPtr); API::World::Component* const _cs = (cs);
#define CHECK_COMPONENT_DETAIL(path, nameAPI, nameImpl)																												\
if(_guid == id_of_type<API::Components::path::nameAPI>())																											\
{																																									\
	const type_id _id = id_of_type<Impl::Components::path::nameImpl>();																								\
	const uint32_t _nid = Impl::World::ComponentManager::GetID(_id);																								\
	if(CloakDebugCheckOK(m_components.find(_id)==m_components.end() && m_componentBits[_nid] == false,API::Global::Debug::Error::COMPONENT_ALREADY_EXISTS, false))	\
	{																																								\
		void* _pos = Impl::World::ComponentManager::Allocate(_id, sizeof(Impl::Components::path::nameImpl),std::alignment_of_v<Impl::Components::path::nameImpl>);	\
		Impl::Components::path::nameImpl* _rd = nullptr;																											\
		if(_cs == nullptr) { _rd = new(reinterpret_cast<Impl::Components::path::nameImpl*>(_pos))Impl::Components::path::nameImpl(this, _id); }						\
		else { _rd = new(reinterpret_cast<Impl::Components::path::nameImpl*>(_pos))Impl::Components::path::nameImpl(this, _cs); }									\
		*_dst = static_cast<API::World::Component*>(_rd);																											\
		*_dstP = reinterpret_cast<void*>(static_cast<API::Components::path::nameAPI*>(_rd));																		\
		CLOAK_ASSUME(*_dst != nullptr && *_dstP != nullptr);																										\
		RegisterAlias<Impl::Components::path::nameImpl>(*_dst, _rd);																								\
		m_components.insert(std::make_pair(_id, std::make_tuple(_rd,*_dst,_pos)));																					\
		m_componentBits[_nid] = true;																																\
	}																																								\
	else {*_dst = nullptr; *_dstP = nullptr;}																														\
} else																																								
#define CHECK_COMPONENT(path, name) CHECK_COMPONENT_DETAIL(path, I##name, name)
#define CHECK_COMPONENT_END() {*_dst = nullptr; *_dstP = nullptr;}}

				void* CLOAK_CALL_THIS Entity::CreateComponent(In const type_id& guid, In_opt API::World::Component* src)
				{
					API::Helper::Lock lock(m_sync);
					API::World::Component* c = nullptr;
					void* r = nullptr;
					CHECK_COMPONENT_BEGIN(guid, c, r, src)
						CHECK_COMPONENT(Render_v1, Render)
						CHECK_COMPONENT(Position_v1, Position)
						CHECK_COMPONENT(Moveable_v1, Moveable)
						CHECK_COMPONENT(Light_v1, PointLight)
						CHECK_COMPONENT(Light_v1, SpotLight)
						CHECK_COMPONENT(Light_v1, DirectionalLight)
						CHECK_COMPONENT(Camera_v1, Camera)
					CHECK_COMPONENT_END()

					CLOAK_ASSUME(c != nullptr && r != nullptr);
					InitComponent(c);
					OnComponentUpdated();
					return r;
				}

#undef CHECK_COMPONENT_BEGIN
#undef CHECK_COMPONENT_DETAIL
#undef CHECK_COMPONENT
#undef CHECK_COMPONENT_END

			}
			namespace Entity_v2 {
				typedef Impl::Global::Memory::TypedPool<Entity> EntityPool;
				//EntityID is 64 bit wide, so we don't need to worry about ID overflowing. In fact, even if we would create 1mil Entities per Frame, with
				//120 FPS, it would still require nearly 5000 years of active game time to overflow this variable!
				std::atomic<EntityID> g_id = 0;
				CE::RefPointer<API::Helper::ISyncSection> g_syncPool = nullptr;
				EntityPool* g_pool = nullptr;

				CLOAK_CALL Entity::Entity() : m_id(g_id.fetch_add(1, std::memory_order_acq_rel))
				{
					m_ref = 1;
					m_cref = 0;
					m_blocked = false;
				}
				CLOAK_CALL Entity::~Entity() {}

				uint64_t CLOAK_CALL_THIS Entity::AddRef()
				{
					return m_ref.fetch_add(1, std::memory_order_acq_rel) + 1;
				}
				uint64_t CLOAK_CALL_THIS Entity::Release()
				{
					uint64_t r = m_ref.fetch_sub(1, std::memory_order_acq_rel);
					if (r == 1)
					{
						if (m_cref.load(std::memory_order_acquire) == 0) { delete this; }
						else if (m_blocked.exchange(true) == false) { Impl::World::ComponentManager_v2::ComponentAllocator::RemoveAll(m_id); }
					}
					else if (r == 0) { CLOAK_ASSUME(false); m_ref = 0; return 0; }
					return r - 1;
				}

				void* CLOAK_CALL Entity::operator new(In size_t size)
				{
					API::Helper::Lock lock(g_syncPool);
					return g_pool->Allocate();
				}
				void CLOAK_CALL Entity::operator delete(In void* ptr)
				{
					API::Helper::Lock lock(g_syncPool);
					g_pool->Free(ptr);
				}
			
				void CLOAK_CALL_THIS Entity::SetAlias(In const type_name& aliasID, In const type_name& realID, In size_t offsetToAlias, In size_t size, In size_t alignment, In size_t offsetToBase)
				{
					Impl::World::ComponentManager_v2::ComponentAllocator* alloc = Impl::World::ComponentManager_v2::ComponentAllocator::GetOrCreateAllocator(aliasID, size, alignment, offsetToBase);
					alloc->AllocateComponentLink(m_id, realID, offsetToAlias);
				}
				void* CLOAK_CALL_THIS Entity::GetComponent(In const type_name& id) const
				{
					Impl::World::ComponentManager_v2::ComponentAllocator* alloc = Impl::World::ComponentManager_v2::ComponentAllocator::GetAllocator(id);
					if (alloc != nullptr) { return alloc->GetComponent(m_id); }
					return nullptr;
				}
				void CLOAK_CALL_THIS Entity::RemoveComponent(In const type_name& id)
				{
					Impl::World::ComponentManager_v2::ComponentAllocator* alloc = Impl::World::ComponentManager_v2::ComponentAllocator::GetAllocator(id);
					if (alloc != nullptr) { alloc->RemoveComponent(m_id); }
				}
				void* CLOAK_CALL_THIS Entity::AllocateComponent(In const type_name& id, In size_t size, In size_t alignment, In size_t offset, Out void** type)
				{
					*type = nullptr;
					if (m_blocked.load(std::memory_order_acquire) == true) { return nullptr; }
					Impl::World::ComponentManager_v2::ComponentAllocator* alloc = Impl::World::ComponentManager_v2::ComponentAllocator::GetOrCreateAllocator(id, size, alignment, offset);
					*type = alloc;
					return alloc->AllocateComponent(m_id);
				}

#define CHECK_COMPONENT_TYPE(InterfaceType, RealType) \
static_assert(std::is_abstract_v<RealType> == false, #RealType " is abstract!"); \
static_assert(std::is_base_of_v<InterfaceType, RealType> == true, #RealType " does not inherit from " #InterfaceType); \
if(id == name_of_type<InterfaceType>()) \
{ \
	if(src == nullptr) { return static_cast<InterfaceType*>(AddComponent<RealType>().Get()); }\
	return static_cast<InterfaceType*>(AddComponent<RealType>(*reinterpret_cast<RealType>(reinterpret_cast<uintptr_t>(src) - offset_of<InterfaceType, RealType>())).Get()); \
}

				void* CLOAK_CALL_THIS Entity::CreateComponent(In const type_name& id, In void* src)
				{
					//TODO
					return nullptr;
				}

#undef CHECK_COMPONENT_TYPE

				void CLOAK_CALL_THIS Entity::ComponentAddRef()
				{
					m_cref.fetch_add(1, std::memory_order_acq_rel);
				}
				void CLOAK_CALL_THIS Entity::ComponentRelease()
				{
					uint64_t r = m_cref.fetch_sub(1, std::memory_order_acq_rel);
					if (r == 1) 
					{
						if (m_ref.load(std::memory_order_acquire) == 0) { delete this; }
					}
					else if (r == 0) { CLOAK_ASSUME(false); m_ref = 0; }
				}

				void CLOAK_CALL Initialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncPool));
					g_pool = new EntityPool();
				}
				void CLOAK_CALL ReleaseSyncs()
				{
					g_syncPool.Free();
				}
				void CLOAK_CALL Terminate()
				{
					delete g_pool;
				}
			}
		}
	}
	namespace API {
		namespace World {
			namespace Entity_v2 {
				void CLOAK_CALL_THIS Component::SetParent(In IEntity* entity, In void* type)
				{
					m_parent = entity;
					m_type = type;
					CLOAK_ASSUME(m_parent != nullptr);
					m_parent->ComponentAddRef();
				}

				CLOAK_CALL Component::Component()
				{
					m_parent = nullptr;
					m_type = nullptr;
					m_active = true;
				}
				CLOAK_CALL Component::Component(In const Component& o) : Component() {} //Yep, the copy constructor does not actually copy anything at all

				CLOAK_CALL Component::~Component()
				{
					if (m_parent != nullptr) { m_parent->ComponentRelease(); }
				}
				uint64_t CLOAK_CALL_THIS Component::AddRef()
				{
					Impl::World::ComponentManager_v2::ComponentAllocator* alloc = reinterpret_cast<Impl::World::ComponentManager_v2::ComponentAllocator*>(m_type);
					alloc->AddRef();
					return m_parent->AddRef();
				}
				uint64_t CLOAK_CALL_THIS Component::Release()
				{
					Impl::World::ComponentManager_v2::ComponentAllocator* alloc = reinterpret_cast<Impl::World::ComponentManager_v2::ComponentAllocator*>(m_type);
					alloc->Release();
					return m_parent->Release();
				}

				const type_name& CLOAK_CALL_THIS Component::GetType() const 
				{
					Impl::World::ComponentManager_v2::ComponentAllocator* alloc = reinterpret_cast<Impl::World::ComponentManager_v2::ComponentAllocator*>(m_type);
					return alloc->GetTypeName();
				}
			}
		}
	}
}