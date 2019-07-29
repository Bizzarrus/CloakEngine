#include "stdafx.h"
#include "Implementation/World/Entity.h"
#include "Implementation/Rendering/Manager.h"
#include "Implementation/World/ComponentManager.h"

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
					API::Helper::Lock lock(m_sync);
					for (const auto& a : m_components) { std::get<1>(a.second)->OnNodeChange(scene, node); }
				}
				bool CLOAK_CALL_THIS Entity::HasComponents(In const API::BitSet& set, In size_t num, In_reads(num) const type_id* ids, Out_writes(num) std::pair<void*, API::World::Component*>* components)
				{
					API::Helper::Lock lock(m_sync);
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
		}
	}
}