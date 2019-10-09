#pragma once
#ifndef CE_API_WORLD_ENTITY_H
#define CE_API_WORLD_ENTITY_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/FileHandler.h"
#include "CloakEngine/Helper/Predicate.h"
#include "CloakEngine/Helper/Types.h"
#include "CloakEngine/Files/Scene.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Helper/Reflection.h"

#include <atomic>

#define CLOAK_COMPONENT(ID, Name) class CLOAK_UUID(ID) Name : public virtual CloakEngine::API::World::Component

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace World {
			inline namespace Entity_v1 {
				class Component;
				//Entities are objects in the scene with support of a component system
				class CLOAK_UUID("{99EB73EA-7104-4BD3-9D4A-98DD4CF90650}") IEntity : public virtual API::Files::IFileHandler, public virtual API::Files::ILODFile {
					friend class Component;
					protected:
						virtual CLOAKENGINE_API void* CLOAK_CALL_THIS AllocateComponent(In const type_id& guid, In size_t size, In size_t alignment, In ptrdiff_t offset, Inout API::Helper::Lock* lock) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetAlias(In const type_id& guid, In Component* src, In void* castedPtr) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS FreeComponent(In Component* co) = 0;
						virtual CLOAKENGINE_API void* CLOAK_CALL_THIS CreateComponent(In const type_id& guid, In_opt Component* src = nullptr) = 0;
						virtual CLOAKENGINE_API void* CLOAK_CALL_THIS ObtainComponent(In const type_id& guid) const = 0;
						virtual CLOAKENGINE_API bool CLOAK_CALL_THIS ContainsComponent(In const type_id& guid) const = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS ReleaseComponent(In const type_id& guid) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS OnComponentUpdated() = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS InitComponent(In Component* co) = 0;
					private:
						//There's no T::Super
						template<typename T, typename B, typename = void> struct Alias {
							CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller, In Component* c, In B* base) {}
						};
						//There is a T::Super
						template<typename T, typename B> struct Alias<T, B, typename type_if<false, typename T::Super>::type> {
							//T::Super is a single type:
							template<typename S, typename = std::enable_if_t<std::is_convertible_v<T*, const volatile Component*>>> struct RegisterAll {
								CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller, In Component* c, In B* base)
								{
									caller->SetAlias(id_of_type<S>(), c, static_cast<S*>(base));
									Alias<S, B>::Register(caller, c, base);
								}
							};
							template<> struct RegisterAll<type_tuple<>, void> {
								CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller, In Component* c, In B* base) {}
							};
							//T::Super is a type tuple:
							template<typename S, typename... L> struct RegisterAll<type_tuple<S, L...>, void> {
								CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller, In Component* c, In B* base)
								{
									RegisterAll<S>::Register(caller, c, base);
									RegisterAll<type_tuple<L...>>::Register(caller, c, base);
								}
							};

							CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller, In Component* c, In B* base)
							{
								RegisterAll<typename T::Super>::Register(caller, c, base);
							}
						};
					protected:
						template<typename T, typename B> void CLOAK_CALL_THIS RegisterAlias(In Component* c, In B* base) 
						{ 
							Alias<T, B>::Register(this, c, base); 
						}
					public:
						virtual CLOAKENGINE_API API::Helper::WriteLock CLOAK_CALL_THIS LockWrite() const = 0;
						virtual CLOAKENGINE_API API::Helper::ReadLock CLOAK_CALL_THIS LockRead() const = 0;
						template<typename T, std::enable_if_t<std::is_convertible_v<T*, const volatile Component*>>* = nullptr> void CLOAK_CALL_THIS RemoveComponent() { ReleaseComponent(id_of_type<T>()); }
						template<typename T, std::enable_if_t<std::is_convertible_v<T*, const volatile Component*>>* = nullptr> API::RefPointer<T> CLOAK_CALL_THIS GetComponent() const { return reinterpret_cast<T*>(ObtainComponent(id_of_type<T>())); }
						template<typename T, std::enable_if_t<std::is_convertible_v<T*, const volatile Component*>>* = nullptr> bool CLOAK_CALL_THIS HasComponent() const { return ContainsComponent(id_of_type<T>()); }

						template<typename T, std::enable_if_t<std::is_convertible_v<T*, const volatile Component*> && !std::is_same_v<T,Component>>* = nullptr, std::enable_if_t<std::is_abstract_v<T>>* = nullptr> API::RefPointer<T> CLOAK_CALL_THIS AddComponent(In_opt T* src = nullptr)
						{
							return reinterpret_cast<T*>(CreateComponent(id_of_type<T>(), src));
						}

						template<typename T, typename... Args> std::enable_if_t<std::is_convertible_v<T*, const volatile Component*> && !std::is_abstract_v<T>, API::RefPointer<T>> __cdecl AddComponent(In Args&&... args)
						{							
							const ptrdiff_t off = reinterpret_cast<uintptr_t>(static_cast<Component*>(reinterpret_cast<T*>(static_cast<uintptr_t>(1)))) - static_cast<uintptr_t>(1);
							API::Helper::Lock lock;
							uint32_t typeID = 0;
							void* dst = AllocateComponent(id_of_type<T>(), sizeof(T), std::alignment_of<T>::value, off, &lock, &typeID);
							if (CloakDebugCheckError(dst != nullptr, API::Global::Debug::Error::COMPONENT_ALREADY_EXISTS, false))
							{
								return nullptr;
							}
							T* r = new(reinterpret_cast<T*>(dst))T(this, typeID, std::forward<Args>(args)...);
							RegisterAlias<T>(static_cast<Component*>(r), r);
							CLOAK_ASSUME(reinterpret_cast<uintptr_t>(static_cast<Component*>(r)) - off == reinterpret_cast<uintptr_t>(r));
							InitComponent(r);
							OnComponentUpdated();
							return r;
						}
				};

				class Component : public virtual API::Helper::IBasicPtr {
					public:
						CLOAKENGINE_API API::Global::Debug::Error CLOAK_CALL_THIS GetEntity(In REFIID riid, Outptr void** ppvObject) const { return SUCCEEDED(m_parent->QueryInterface(riid, ppvObject)) ? API::Global::Debug::Error::OK : API::Global::Debug::Error::NO_INTERFACE; }
						CLOAKENGINE_API uint64_t CLOAK_CALL_THIS AddRef() override
						{
							m_parent->AddRef();
							return static_cast<uint64_t>(m_ref.fetch_add(1) + 1); 
						}
						CLOAKENGINE_API uint64_t CLOAK_CALL_THIS Release() override
						{
							IEntity* p = m_parent;
							const int64_t r = m_ref.fetch_sub(1);
							if (r == 0) { m_ref = 0; }
							else if (r == 1) 
							{
								if (m_switchActive.exchange(false) == true)
								{
									API::Helper::WriteLock lock = m_parent->LockWrite();
									if (m_active == false) { OnDisable(); }
									else { OnEnable(); }
									m_parent->OnComponentUpdated();
								}
								m_parent->FreeComponent(this); 
							}
							p->Release();
							return static_cast<uint64_t>(r > 0 ? r - 1 : 0);
						}
						CLOAKENGINE_API void CLOAK_CALL_THIS SetActive(In bool active) 
						{ 
							if (m_active.exchange(active) != active)
							{
								if (m_ref == 0)
								{
									API::Helper::WriteLock lock = m_parent->LockWrite();
									if (active == false) { OnDisable(); }
									else { OnEnable(); }
									m_parent->OnComponentUpdated();
								}
								else { m_switchActive = true; }
							}
						}
						CLOAKENGINE_API bool CLOAK_CALL_THIS IsActive() const { return m_active; }

						template<typename T, std::enable_if_t<std::is_convertible_v<T*, const volatile Component*>>* = nullptr> API::RefPointer<T> CLOAK_CALL_THIS AddComponent() { return m_parent->AddComponent<T>(); }
						template<typename T, std::enable_if_t<std::is_convertible_v<T*, const volatile Component*>>* = nullptr> API::RefPointer<T> CLOAK_CALL_THIS GetComponent() { return m_parent->GetComponent<T>(); }

						virtual void CLOAK_CALL_THIS OnLoad(In API::Files::Priority prio, In API::Files::LOD lod) {}
						virtual void CLOAK_CALL_THIS OnSetLoadPriority(In API::Files::Priority prio) {}
						virtual void CLOAK_CALL_THIS OnSetLoadLOD(In API::Files::LOD lod) {}
						virtual void CLOAK_CALL_THIS OnUnload() {}
						virtual void CLOAK_CALL_THIS OnEnable() {}
						virtual void CLOAK_CALL_THIS OnDisable() {}
						virtual void CLOAK_CALL_THIS OnNodeChange(In API::Files::IScene* scene, In API::Files::IWorldNode* node) {}

						virtual CLOAK_CALL ~Component() {}

						const type_id TypeID;
					protected:
						CLOAKENGINE_API CLOAK_CALL Component(In IEntity* entity, In type_id typeID) : m_parent(entity), TypeID(typeID) {}

						IEntity* const m_parent;
					private:
						CLOAKENGINE_API CLOAK_CALL Component(In const Component& o) : m_parent(o.m_parent), m_active(o.m_active.load()), TypeID(o.TypeID) {}
						std::atomic<int64_t> m_ref = 0;
						std::atomic<bool> m_active = true;
						std::atomic<bool> m_switchActive = false;
				};
			}
			namespace Entity_v2 {
				class Component;
				class CLOAK_UUID("{D80139E9-C6F6-4B02-8597-159F76052E94}") IEntity : public virtual API::Helper::IBasicPtr {
					friend class Component;
					protected:
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetAlias(In const type_name& aliasID, In const type_name& realID, In size_t offsetToAlias, In size_t size, In size_t alignment, In size_t offsetToBase) = 0;
						virtual CLOAKENGINE_API void* CLOAK_CALL_THIS GetComponent(In const type_name& id) const = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS RemoveComponent(In const type_name& id) = 0;
						virtual CLOAKENGINE_API void* CLOAK_CALL_THIS AllocateComponent(In const type_name& id, In size_t size, In size_t alignment, In size_t offset, Out void** type) = 0;
						virtual CLOAKENGINE_API void* CLOAK_CALL_THIS CreateComponent(In const type_name& id, In void* src) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS ComponentAddRef() = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS ComponentRelease() = 0;
					private:
						//There is no T::Super
						template<typename T, typename B, typename = void> struct Alias {
							CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller) {}
						};
						//There is a T::Super
						template<typename T, typename B> struct Alias<T, B, typename type_if<false, typename T::Super>::type> {
							//T::Super is unknown
							template<typename S, typename = void> struct RegisterAll {
								CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller) {}
							};
							//T::Super is a single component
							template<typename S> struct RegisterAll<S, std::enable_if_t<std::is_base_of_v<Component, S> && std::is_base_of_v<S, T> && !std::is_same_v<Component, S>>> {
								CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller)
								{
									caller->SetAlias(name_of_type<S>(), name_of_type<B>(), offset_of<S, B>(), sizeof(S), std::alignment_of_v<S>, offset_of<Component, S>());
									Alias<S, B>::Register(caller);
								} 
							};
							//T::Super is a empty type tuple
							template<> struct RegisterAll<type_tuple<>, void> {
								CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller) {}
							};
							//T::Super is a filled type tuple
							template<typename S, typename... L> struct RegisterAll<type_tuple<S, L...>, void> {
								CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller)
								{
									RegisterAll<S>::Register(caller);
									RegisterAll<type_tuple<L...>>::Register(caller);
								}
							};
							CLOAK_FORCEINLINE static void CLOAK_CALL Register(In IEntity* caller) { RegisterAll<typename T::Super>::Register(caller); }
						};
					public:
						//Removes a component of the given type from the entity, if existent. Notice that removal does not happen immediatily, so a component of
						//the same type can not be re-added in the same frame, and GetComponent as well as Groups might still find this component within the frame
						//it was removed! Components will not be removed as long as there is at least one living reference to that component!
						template<typename T> inline void CLOAK_CALL_THIS RemoveComponent() 
						{ 
							static_assert(std::is_base_of_v<Component, T> && !std::is_same_v<Component, T>, "T is not a component type");
							RemoveComponent(name_of_type<T>()); 
						}
						//Returns a component of the given type from the entity, or nullptr if no such component exists. This function might be slow and shouldn't be
						//called often. Notice that GetComponent might return a component that was marked as to-be-deleted during the current frame.
						template<typename T> inline API::RefPointer<T> CLOAK_CALL_THIS GetComponent() const 
						{ 
							static_assert(std::is_base_of_v<Component, T> && !std::is_same_v<Component, T>, "T is not a component type");
							return reinterpret_cast<T*>(GetComponent(name_of_type<T>())); 
						}
						//Creates and returns a component of the given type for this entity. Notice that AddComponent is not thread-safe and may not be called by two threads
						//simultaniously on the same entity! Adding components may be slow, and may even fail if the entity is to be deleted. If another component of the same type is given, 
						//that component will be copied into the current entry.
						template<typename T> inline std::enable_if_t<std::is_abstract_v<T>, API::RefPointer<T>> CLOAK_CALL_THIS AddComponent(In_opt const API::RefPointer<T>& src = nullptr) 
						{ 
							static_assert(std::is_base_of_v<Component, T> && !std::is_same_v<Component, T>, "T is not a component type");
							return reinterpret_cast<T*>(CreateComponent(name_of_type<T>(), src.Get())); 
						}
						//Creates and returns a component of the given type for this entity. Notice that AddComponent is not thread-safe and may not be called by two threads
						//simultaniously on the same entity! Adding components may be slow, and may even fail if the entity is to be deleted.
						template<typename T, typename... Args> std::enable_if_t<!std::is_abstract_v<T>, API::RefPointer<T>> CLOAK_CALL_THIS AddComponent(In Args&&... args)
						{
							static_assert(std::is_base_of_v<Component, T> && !std::is_same_v<Component, T>, "T is not a component type");
							CE::RefPointer<T> c = GetComponent<T>();
							if (c != nullptr) { return c; }
							void* type = nullptr;
							void* dst = AllocateComponent(name_of_type<T>(), sizeof(T), std::alignment_of_v<T>, offset_of<Component, T>(), &type);
							if (dst != nullptr)
							{
								T* r = ::new(dst)T(std::forward<Args>(args)...);
								r->SetParent(this, type);
								Alias<T, T>::Register(this);
								CLOAK_ASSUME(reinterpret_cast<uintptr_t>(static_cast<Component*>(r)) - offset == reinterpret_cast<uintptr_t>(r));
								return r;
							}
							return nullptr;
						}
						//Creates and returns a component of the given type for this entity. Notice that AddComponent is not thread-safe and may not be called by two threads
						//simultaniously on the same entity! Adding components may be slow, and may even fail if the entity is to be deleted. If another component of the same type is given, 
						//that component will be copied into the current entry.
						template<typename T> inline std::enable_if_t<!std::is_abstract_v<T>, API::RefPointer<T>> CLOAK_CALL_THIS AddComponent(In const API::RefPointer<T>& src) 
						{
							if (src == nullptr) { return AddComponent<T>(); }
							return AddComponent<T>(*src.Get()); 
						}
				};
				class CLOAKENGINE_API Component : public virtual API::Helper::IBasicPtr {
					friend class IEntity;
					private:
						IEntity* m_parent;
						void* m_type;
						bool m_active;

						void CLOAK_CALL_THIS SetParent(In IEntity* entity, In void* type);
					protected:
						CLOAK_CALL Component();
						CLOAK_CALL Component(In const Component& o);
					public:
						virtual CLOAK_CALL ~Component();
						uint64_t CLOAK_CALL_THIS AddRef() override;
						uint64_t CLOAK_CALL_THIS Release() override;

						const type_name& CLOAK_CALL_THIS GetType() const;

						inline CE::RefPointer<IEntity> CLOAK_CALL_THIS GetEntity() const { return m_parent; }
						inline bool CLOAK_CALL_THIS IsActive() const { return m_active; }
						inline void CLOAK_CALL_THIS SetActive(In bool active) { m_active = active; }
				};
			}
		}
	}
}

#endif