#include "stdafx.h"
#include "Implementation/Files/Scene.h"
#include "CloakEngine/World/Entity.h"
#include "Implementation/Components/Position.h"
#include "Implementation/Components/GraphicUpdate.h"

#define SNAP_TO_AXIS_ADJ(cur, Name) cur->GetAdjacentWorldNode(API::Files::Scene_v1::AdjacentPos::Name)
#define SNAP_TO_AXIS_H_1(scene,p,Axis,halfDist,fullDist,cur,Comp,Neg,Name) if(p.Axis Comp Neg halfDist){do{auto* n=SNAP_TO_AXIS_ADJ(cur,Name); if(n!=nullptr){cur=n; p.Axis-= Neg fullDist;}else{break;}} while(p.Axis Comp Neg halfDist);}
#define SNAP_TO_AXIS_H_2(scene,p,Axis,halfDist,fullDist,cur,Comp,Neg,Name) if(p.Axis Comp Neg halfDist){do{auto* n=SNAP_TO_AXIS_ADJ(cur,Name); if(n==nullptr){scene->Resize(API::Files::Scene_v1::AdjacentPos::Name);} else {cur=n; p.Axis-= Neg fullDist;}} while(p.Axis Comp Neg halfDist);}
#define SNAP_TO_AXIS(scene,p,Axis,halfDist,fullDist,cur, NamePos, NameNeg,variant) SNAP_TO_AXIS_H_##variant(scene,p,Axis,halfDist,fullDist,cur,>,,NamePos) else SNAP_TO_AXIS_H_##variant(scene,p,Axis,halfDist,fullDist,cur,<,-,NameNeg)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Scene_v1 {
				constexpr API::Files::FileType g_fileType{ "Scene","CESB",1000 };

				CLOAK_CALL WorldNode::WorldNode(In Scene* par, In const API::Global::Math::IntVector& pos) : m_pos(pos), m_parent(par)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_drawSync));
					m_inStream = false;
					m_loadCount = 0;
					m_floodFill = 0;
					m_lod = API::Files::LOD::LOW;
					m_NTW = API::Global::Math::Matrix::Identity;
					for (size_t a = 0; a < ARRAYSIZE(m_adjNodes); a++) { m_adjNodes[a] = nullptr; }
				}
				CLOAK_CALL WorldNode::~WorldNode()
				{
					const bool isl = isLoaded();
					m_objects.for_each([=](Impl::Components::Position_v1::Position* pc)
					{
						CLOAK_ASSUME(pc != nullptr);
						Impl::World::Entity* o = nullptr;
						if (pc->GetEntity(CE_QUERY_ARGS(&o)) == API::Global::Debug::Error::OK)
						{
							if (isl) { o->unload(); }
							SAVE_RELEASE(o);
						}
						pc->RemoveFromNode(this);
					});
					SAVE_RELEASE(m_sync);
					SAVE_RELEASE(m_drawSync);
				}

				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr) 
						{ 
							Add(pc, pos, rotation, scale); 
							pc->Release();
						}
						else { m_parent->AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In_opt float scale)
				{
					Add(object, pos, rotation, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pos, rotation, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In const API::Global::Math::Vector& scale)
				{
					Add(object, pr.GetTranslationPart(), pr.GetRotationPart(), scale);
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In_opt float scale)
				{
					Add(object, pr.GetTranslationPart(), pr.GetRotationPart(), API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pr.GetTranslationPart(), pr.GetRotationPart(), API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Vector& scale)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr) 
						{ 
							Add(pc, pos, pc->GetRotation(), scale); 
							pc->Release();
						}
						else { m_parent->AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr)
						{
							const API::Global::Math::WorldPosition wp = pc->GetPosition();
							Add(pc, wp.Position, wp.Node, pc->GetRotation(), pc->GetScale());
							pc->Release();
						}
						else { m_parent->AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In_opt float scale)
				{
					Add(object, pos, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pos, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Vector& scale)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), scale);
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In_opt float scale)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), rotation, scale);
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In_opt float scale)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), rotation, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), rotation, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In_opt float scale)
				{
					Add(object, pos, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pos, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Vector& scale)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr)
						{
							Add(pc, pos.Position, pos.Node, pc->GetRotation(), scale);
							pc->Release();
						}
						else { m_parent->AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In_opt float scale)
				{
					Add(object, pos, rotation, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pos, rotation, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS WorldNode::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr)
						{
							Add(pc, pos.Position, pos.Node, rotation, scale);
							pc->Release();
						}
						else { m_parent->AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS WorldNode::Remove(In API::World::IEntity* object)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr) { pc->RemoveFromNode(this); }
						else { m_parent->RemoveFromScene(object); }
					}
				}

				API::Files::Scene_v1::IWorldNode* CLOAK_CALL_THIS WorldNode::GetAdjacentNode(In API::Files::Scene_v1::AdjacentPos pos) const
				{
					API::Helper::Lock lock(m_sync);
					return m_adjNodes[static_cast<size_t>(pos)];
				}
				API::Files::Scene_v1::IScene* CLOAK_CALL_THIS WorldNode::GetParentScene() const { return m_parent; }
				void CLOAK_CALL_THIS WorldNode::AddPortal(In IWorldNode* oth, In float dist)
				{
					WorldNode* n = nullptr;
					if (oth != nullptr && SUCCEEDED(oth->QueryInterface(CE_QUERY_ARGS(&n))) && CloakDebugCheckOK(dist > 0, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true))
					{
						const float d = dist*dist;
						API::Helper::Lock lock(m_sync);
						for (size_t a = 0; a < m_portals.size(); a++)
						{
							if (m_portals[a].Node == n)
							{
								m_portals[a].Distance = min(m_portals[a].Distance, d);
								goto add_portal_found;
							}
						}
						PortalInfo pi;
						pi.Node = n;
						pi.Distance = dist;
						m_portals.push_back(pi);
					add_portal_found:;

					}
					SAVE_RELEASE(n);
				}
				const API::Global::Math::IntVector& CLOAK_CALL_THIS WorldNode::GetPosition() const { return m_pos; }

				void CLOAK_CALL_THIS WorldNode::RequestLOD(In API::Files::LOD lod)
				{
					API::Files::LOD lL = m_lod;
					API::Files::LOD nL;
					do {
						nL = lL;
						if (static_cast<size_t>(nL) > static_cast<size_t>(lod)) { nL = lod; }
					} while (!m_lod.compare_exchange_strong(lL, nL));
					if (nL != lL)
					{
						API::Helper::Lock lock(m_sync);
						m_objects.for_each([=](Impl::Components::Position_v1::Position* pc)
						{ 
							Impl::World::Entity* o = nullptr;
							if (pc->GetEntity(CE_QUERY_ARGS(&o)) == API::Global::Debug::Error::OK)
							{
								o->RequestLOD(nL);
								SAVE_RELEASE(o);
							}
						});
					}
				}

				uint64_t CLOAK_CALL_THIS WorldNode::GetRefCount() { return m_parent->GetRefCount(); }
				API::Global::Debug::Error CLOAK_CALL_THIS WorldNode::Delete() { return API::Global::Debug::Error::OK; }
				Success(return == S_OK) Post_satisfies(*ppvObject != nullptr) HRESULT STDMETHODCALLTYPE WorldNode::QueryInterface(In REFIID riid, Outptr void** ptr)
				{
					HRESULT hRet = E_NOINTERFACE;
					if (riid == __uuidof(WorldNode)) { *ptr = (WorldNode*)this; hRet = S_OK; }
					else if (riid == __uuidof(API::Files::Scene_v1::IWorldNode)) { *ptr = (API::Files::Scene_v1::IWorldNode*)this; hRet = S_OK; }
					else if (riid == __uuidof(API::Files::FileHandler_v1::ILODFile)) { *ptr = (API::Files::FileHandler_v1::ILODFile*)this; hRet = S_OK; }
					else if (riid == __uuidof(API::Files::Scene_v1::IWorldContainer)) { *ptr = (API::Files::Scene_v1::IWorldContainer*)this; hRet = S_OK; }
					else if (riid == __uuidof(API::Helper::SavePtr_v1::ISavePtr)) { *ptr = (API::Helper::SavePtr_v1::ISavePtr*)this; hRet = S_OK; }
					if (SUCCEEDED(hRet)) { AddRef(); }
					return hRet;
				}
				uint64_t CLOAK_CALL_THIS WorldNode::AddRef() { return m_parent->AddRef(); }
				uint64_t CLOAK_CALL_THIS WorldNode::Release() { return m_parent->Release(); }
				void CLOAK_CALL_THIS WorldNode::SetDebugName(In const std::string& s) {}
				void CLOAK_CALL_THIS WorldNode::SetSourceFile(In const std::string& s) {}
				std::string CLOAK_CALL_THIS WorldNode::GetDebugName() const { return ""; }
				std::string CLOAK_CALL_THIS WorldNode::GetSourceFile() const { return ""; }
				void* CLOAK_CALL WorldNode::operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
				void CLOAK_CALL WorldNode::operator delete(In void* ptr) { API::Global::Memory::MemoryHeap::Free(ptr); }

				void CLOAK_CALL_THIS WorldNode::SetAdjacentNodes(In WorldNode* adj[6])
				{
					API::Helper::Lock lock(m_sync);
					for (size_t a = 0; a < ARRAYSIZE(m_adjNodes); a++) { m_adjNodes[a] = adj[a]; }
				}
				WorldNode* CLOAK_CALL_THIS WorldNode::GetAdjacentWorldNode(In API::Files::Scene_v1::AdjacentPos pos) const
				{
					API::Helper::Lock lock(m_sync);
					return m_adjNodes[static_cast<uint8_t>(pos)];
				}

				bool CLOAK_CALL_THIS WorldNode::isLoaded() const { return m_loadCount > 0; }
				void CLOAK_CALL_THIS WorldNode::load() 
				{
					ULONG r = m_loadCount.fetch_add(1) + 1;
					if (r == 1)
					{
						API::Helper::Lock lock(m_sync);
						m_objects.for_each([=](Impl::Components::Position_v1::Position* pc)
						{
							Impl::World::Entity* o = nullptr;
							if (pc->GetEntity(CE_QUERY_ARGS(&o)) == API::Global::Debug::Error::OK)
							{
								o->load();
								SAVE_RELEASE(o);
							}
						});
						m_parent->load();
					}
				}
				void CLOAK_CALL_THIS WorldNode::unload()
				{
					ULONG r = m_loadCount.fetch_sub(1);
					if (r == 1)
					{
						API::Helper::Lock lock(m_sync);
						m_lod = API::Files::LOD::LOW;
						m_objects.for_each([=](Impl::Components::Position_v1::Position* pc)
						{
							Impl::World::Entity* o = nullptr;
							if (pc->GetEntity(CE_QUERY_ARGS(&o)) == API::Global::Debug::Error::OK)
							{
								o->unload();
								SAVE_RELEASE(o);
							}
						});
						m_parent->unload();
						m_floodFill = 0;
					}
				}

				bool CLOAK_CALL_THIS WorldNode::UpdateFloodFillCounter(In uint32_t count) { return m_floodFill.exchange(count) != count; }
				uint32_t CLOAK_CALL_THIS WorldNode::GetFloodFillCounter() { return m_floodFill; }
				void CLOAK_CALL_THIS WorldNode::GetPortals(Out API::List<PortalInfo>* ports)
				{
					API::Helper::Lock lock(m_sync);
					ports->reserve(ports->size() + m_portals.size());
					for (size_t a = 0; a < m_portals.size(); a++) { ports->push_back(m_portals[a]); }
				}
				bool CLOAK_CALL_THIS WorldNode::setStreamFlag(In bool f) { return m_inStream.exchange(f); }

				void CLOAK_CALL_THIS WorldNode::Add(In Impl::Components::Position_v1::Position* pc, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rot, In const API::Global::Math::Vector& scale)
				{
					WorldNode* n = this;
					API::Global::Math::Vector p = pos;
					SnapToNode(&n, &p);
					pc->SetNode(n, p, rot, scale);
				}
				void CLOAK_CALL_THIS WorldNode::Add(In Impl::Components::Position_v1::Position* pc, In const API::Global::Math::Vector& pos, In const API::Global::Math::IntVector& npos, In const API::Global::Math::Quaternion& rot, In const API::Global::Math::Vector& scale)
				{
					WorldNode* n = this;
					API::Global::Math::Vector p = pos;
					SnapToNode(&n, &p, npos - m_pos);
					pc->SetNode(n, p, rot, scale);
				}
				void CLOAK_CALL_THIS WorldNode::SnapToNode(Out WorldNode** node, Inout API::Global::Math::Vector* pos)
				{
					WorldNode* cur = this;
					API::Global::Math::Point p(*pos);
					SNAP_TO_AXIS(m_parent, p, X, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Right, Left, 1);
					SNAP_TO_AXIS(m_parent, p, Y, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Top, Bottom, 1);
					SNAP_TO_AXIS(m_parent, p, Z, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Front, Back, 1);
					*node = cur;
					*pos = p;
				}
				void CLOAK_CALL_THIS WorldNode::SnapToNode(Out WorldNode** node, Inout API::Global::Math::Vector* pos, In const API::Global::Math::IntVector& npos)
				{
					WorldNode* cur = this;
					API::Global::Math::IntPoint p = static_cast<API::Global::Math::IntPoint>(npos);
					SNAP_TO_AXIS(m_parent, p, X, 0, 1, cur, Right, Left, 1);
					SNAP_TO_AXIS(m_parent, p, Y, 0, 1, cur, Top, Bottom, 1);
					SNAP_TO_AXIS(m_parent, p, Z, 0, 1, cur, Front, Back, 1);

					API::Global::Math::Point q(*pos);
					SNAP_TO_AXIS(m_parent, q, X, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Right, Left, 1);
					SNAP_TO_AXIS(m_parent, q, Y, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Top, Bottom, 1);
					SNAP_TO_AXIS(m_parent, q, Z, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Front, Back, 1);
					*node = cur;
					*pos = q;
				}
				void CLOAK_CALL_THIS WorldNode::Remove(In const ObjectRef& objRef)
				{
					API::Helper::Lock lock(m_sync);
					Impl::Components::Position_v1::Position* pc = *objRef;
					Impl::World::Entity* o = nullptr;
					if (pc->GetEntity(CE_QUERY_ARGS(&o)) == API::Global::Debug::Error::OK)
					{
						o->unload();
						SAVE_RELEASE(o);
					}
					pc->Release();
					m_objects.remove(objRef);
				}
				void CLOAK_CALL_THIS WorldNode::Add(In Impl::Components::Position_v1::Position* obj, Out ObjectRef* objRef)
				{
					API::Helper::Lock lock(m_sync);
					if (obj != nullptr)
					{
						obj->AddRef();
						Impl::World::Entity* o = nullptr;
						if (obj->GetEntity(CE_QUERY_ARGS(&o)) == API::Global::Debug::Error::OK)
						{
							o->RequestLOD(m_lod);
							if (isLoaded()) { o->load(); }
							SAVE_RELEASE(o);
						}
						*objRef = m_objects.push(obj);
					}
				}
				float CLOAK_CALL_THIS WorldNode::GetRadius(In API::Global::Memory::PageStackAllocator* alloc)
				{
					API::Helper::Lock lock(m_sync);
					const size_t s = m_objects.size();
					Impl::Components::Position** pc = reinterpret_cast<Impl::Components::Position**>(alloc->Allocate(sizeof(Impl::Components::Position*) * s));
					size_t p = 0;
					m_objects.for_each([&](Impl::Components::Position* pos) {
						CLOAK_ASSUME(p < s);
						pc[p++] = pos;
						pos->AddRef();
					});
					CLOAK_ASSUME(p == s);
					lock.unlock();
					
					float r = 0;
					for (size_t a = 0; a < s; a++)
					{
						API::Global::Math::WorldPosition p = pc[a]->GetPosition();
						p.Node -= m_pos;
						float nr = p.Length();
						Impl::Components::IGraphicUpdatePosition* gup = pc[a]->GetComponent<Impl::Components::IGraphicUpdatePosition>();
						if (gup != nullptr)
						{
							nr += gup->GetBoundingRadius();
							gup->Release();
						}
						r = max(r, nr);
						pc[a]->Release();
					}
					alloc->Free(pc);
					return r;
				}
				const API::Global::Math::Matrix& CLOAK_CALL_THIS WorldNode::GetNodeToWorld(Out_opt API::Rendering::WorldID* worldID)
				{
					API::Helper::Lock lock(m_drawSync);
					if (worldID != nullptr) { *worldID = m_worldID; }
					return m_NTW;
				}
				void CLOAK_CALL_THIS WorldNode::SetNodeToWorld(In const API::Global::Math::Matrix& NTW, In API::Rendering::WorldID worldID)
				{
					API::Helper::Lock lock(m_drawSync);
					m_NTW = NTW;
					m_worldID = worldID;
				}

				CLOAK_CALL GeneratedWorldNode::GeneratedWorldNode(In GeneratedScene* par, In const API::Global::Math::IntVector& pos) : WorldNode(par, pos) {}
				CLOAK_CALL GeneratedWorldNode::~GeneratedWorldNode() {}
				void CLOAK_CALL_THIS GeneratedWorldNode::SnapToNode(Out WorldNode** node, Inout API::Global::Math::Vector* pos)
				{
					GeneratedScene* s = nullptr;
					HRESULT hRet = m_parent->QueryInterface(CE_QUERY_ARGS(&s));
					if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true))
					{
						WorldNode* cur = this;
						API::Global::Math::Point p(*pos);
						SNAP_TO_AXIS(s, p, X, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Right, Left, 2);
						SNAP_TO_AXIS(s, p, Y, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Top, Bottom, 2);
						SNAP_TO_AXIS(s, p, Z, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Front, Back, 2);
						*node = cur;
						*pos = p;
					}
					SAVE_RELEASE(s);
				}
				void CLOAK_CALL_THIS GeneratedWorldNode::SnapToNode(Out WorldNode** node, Inout API::Global::Math::Vector* pos, In const API::Global::Math::IntVector& npos)
				{
					GeneratedScene* s = nullptr;
					HRESULT hRet = m_parent->QueryInterface(CE_QUERY_ARGS(&s));
					if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true))
					{
						WorldNode* cur = this;
						API::Global::Math::IntPoint p = static_cast<API::Global::Math::IntPoint>(npos);
						SNAP_TO_AXIS(s, p, X, 0, 1, cur, Right, Left, 2);
						SNAP_TO_AXIS(s, p, Y, 0, 1, cur, Top, Bottom, 2);
						SNAP_TO_AXIS(s, p, Z, 0, 1, cur, Front, Back, 2);

						API::Global::Math::Point q(*pos);
						SNAP_TO_AXIS(s, q, X, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Right, Left, 2);
						SNAP_TO_AXIS(s, q, Y, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Top, Bottom, 2);
						SNAP_TO_AXIS(s, q, Z, WORLDNODE_HALFSIZE, WORLDNODE_SIZE, cur, Front, Back, 2);
						*node = cur;
						*pos = q;
					}
					SAVE_RELEASE(s);
				}
				void CLOAK_CALL_THIS GeneratedWorldNode::SetAdjacentNode(In WorldNode* node, In API::Files::Scene_v1::AdjacentPos dir)
				{
					API::Helper::Lock lock(m_sync);
					m_adjNodes[static_cast<uint8_t>(dir)] = node;
				}

				CLOAK_CALL Scene::Scene()
				{
					DEBUG_NAME(Scene);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncDraw));
					m_skybox = nullptr;
					m_skyIntensity = 0;
					m_ambient = API::Helper::Color::Black;
					m_root = nullptr;
				}
				CLOAK_CALL Scene::~Scene()
				{
					WorldNode* nodes = reinterpret_cast<WorldNode*>(m_nodes);
					for (size_t a = 0; a < m_nodeCount; a++) { nodes[a].~WorldNode(); }
					API::Global::Memory::MemoryHeap::Free(m_nodes);
					m_nodeCount = 0;
					SAVE_RELEASE(m_syncDraw);
					if (isLoaded())
					{
						if (m_skybox != nullptr) { m_skybox->unload(); }
						for (const auto& a : m_entities) { a->unload(); }
					}
					SAVE_RELEASE(m_skybox);
					for (const auto& a : m_entities) { SAVE_RELEASE(a); }
				}
				void CLOAK_CALL_THIS Scene::SetSkybox(In API::Files::ICubeMap* skybox, In float lightIntensity)
				{
					API::Helper::WriteLock lock(m_sync);
					if (m_skybox != skybox)
					{
						if (m_skybox != nullptr)
						{
							if (isLoaded()) { m_skybox->unload(); }
							m_skybox->Release();
						}
						m_skybox = skybox;
						if (m_skybox != nullptr)
						{
							m_skybox->RequestLOD(API::Files::LOD::ULTRA);
							if (isLoaded()) { m_skybox->load(); }
							m_skybox->AddRef();
						}
					}
					m_skyIntensity = lightIntensity;
				}
				void CLOAK_CALL_THIS Scene::SetAmbientLight(In const API::Helper::Color::RGBX& col)
				{
					API::Helper::WriteLock lock(m_sync);
					m_ambient.R = col.R;
					m_ambient.G = col.G;
					m_ambient.B = col.B;
				}
				void CLOAK_CALL_THIS Scene::SetAmbientLight(In float R, In float G, In float B)
				{
					API::Helper::WriteLock lock(m_sync);
					m_ambient.R = R;
					m_ambient.G = G;
					m_ambient.B = B;
				}
				void CLOAK_CALL_THIS Scene::SetAmbientLight(In float R, In float G, In float B, In float intensity)
				{
					API::Helper::WriteLock lock(m_sync);
					m_ambient.R = R;
					m_ambient.G = G;
					m_ambient.B = B;
					m_ambient.A = intensity;
				}
				void CLOAK_CALL_THIS Scene::SetAmbientLight(In const API::Helper::Color::RGBX& col, In float intensity)
				{
					API::Helper::WriteLock lock(m_sync);
					m_ambient.R = col.R;
					m_ambient.G = col.G;
					m_ambient.B = col.B;
					m_ambient.A = intensity;
				}
				void CLOAK_CALL_THIS Scene::SetAmbientLight(In const API::Helper::Color::RGBA& col)
				{
					API::Helper::WriteLock lock(m_sync);
					m_ambient = col;
				}
				void CLOAK_CALL_THIS Scene::SetAmbientLightIntensity(In float intensity)
				{
					API::Helper::WriteLock lock(m_sync);
					m_ambient.A = intensity;
				}
				API::Files::Scene_v1::IWorldNode* CLOAK_CALL_THIS Scene::GetWorldNode(In int X, In int Y, In int Z)
				{
					API::Helper::ReadLock lock(m_sync);
					WorldNode* r = m_root;
					API::Global::Math::IntVector p(X, Y, Z);
					API::Global::Math::Vector xp(0, 0, 0);
					r->SnapToNode(&r, &xp, p);
					return r;
				}
				void CLOAK_CALL_THIS Scene::SetGravity(In const API::Global::Math::Vector& dir)
				{
					//TODO
				}
				void CLOAK_CALL_THIS Scene::EnableGravity()
				{
					//TODO
				}
				void CLOAK_CALL_THIS Scene::DisableGravity()
				{
					//TODO
				}

				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr)
						{
							API::Helper::WriteLock lock(m_sync);
							const API::Global::Math::WorldPosition wp = pc->GetPosition();
							m_root->Add(pc, wp.Position, wp.Node, pc->GetRotation(), pc->GetScale());
							pc->Release();
						}
						else { AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In_opt float scale)
				{
					Add(object, pos, rotation, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pos, rotation, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr) 
						{
							API::Helper::WriteLock lock(m_sync);
							m_root->Add(pc, pos, rotation, scale);
							pc->Release();
						}
						else { AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In_opt float scale)
				{
					Add(object, pr.GetTranslationPart(), pr.GetRotationPart(), API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pr.GetTranslationPart(), pr.GetRotationPart(), API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In const API::Global::Math::Vector& scale)
				{
					Add(object, pr.GetTranslationPart(), pr.GetRotationPart(), scale);
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In_opt float scale)
				{
					Add(object, pos, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pos, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Vector& scale)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr)
						{
							API::Helper::WriteLock lock(m_sync);
							m_root->Add(pc, pos, pc->GetRotation(), scale);
							pc->Release();
						}
						else { AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In_opt float scale)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Vector& scale)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), scale);
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In_opt float scale)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), rotation, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), rotation, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale)
				{
					Add(object, API::Global::Math::Vector(X, Y, Z), rotation, scale);
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In_opt float scale)
				{
					Add(object, pos, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pos, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Vector& scale)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr)
						{
							API::Helper::WriteLock lock(m_sync);
							m_root->Add(pc, pos.Position, pos.Node, pc->GetRotation(), scale);
							pc->Release();
						}
						else { AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In_opt float scale)
				{
					Add(object, pos, rotation, API::Global::Math::Vector(scale, scale, scale));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ)
				{
					Add(object, pos, rotation, API::Global::Math::Vector(scaleX, scaleY, scaleZ));
				}
				void CLOAK_CALL_THIS Scene::Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr)
						{
							API::Helper::WriteLock lock(m_sync);
							m_root->Add(pc, pos.Position, pos.Node, rotation, scale);
							pc->Release();
						}
						else { AddToScene(object); }
					}
				}
				void CLOAK_CALL_THIS Scene::Remove(In API::World::IEntity* object)
				{
					if (object != nullptr)
					{
						Impl::Components::Position_v1::Position* pc = object->GetComponent<Impl::Components::Position_v1::Position>();
						if (pc != nullptr) { pc->RemoveFromNode(); }
						else { RemoveFromScene(object); }
					}
				}
				
				void CLOAK_CALL_THIS Scene::AddToScene(In API::World::IEntity* object)
				{
					if (object != nullptr)
					{
						Impl::World::Entity* e = nullptr;
						HRESULT hRet = object->QueryInterface(CE_QUERY_ARGS(&e));
						if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							API::Helper::Lock lock(m_sync);
							auto f = m_entities.find(e);
							if (f == m_entities.end())
							{
								e->AddRef();
								m_entities.insert(e);
								e->SetScene(this);
								if (isLoaded()) { e->load(); }
							}
							e->Release();
						}
					}
				}
				void CLOAK_CALL_THIS Scene::RemoveFromScene(In API::World::IEntity* object)
				{
					if (object != nullptr)
					{
						Impl::World::Entity* e = nullptr;
						HRESULT hRet = object->QueryInterface(CE_QUERY_ARGS(&e));
						if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							API::Helper::Lock lock(m_sync);
							auto f = m_entities.find(e);
							if (f != m_entities.end())
							{
								m_entities.erase(f);
								e->SetScene(nullptr);
								e->Release();
							}
							e->Release();
						}
					}
				}
				const API::Helper::Color::RGBA& CLOAK_CALL_THIS Scene::GetAmbientLight() const
				{
					API::Helper::ReadLock lock(m_sync);
					return m_ambient;
				}
				bool CLOAK_CALL_THIS Scene::GetSkybox(Out float* lightIntensity, In REFIID riid, Outptr void** ppvObject) const
				{
					API::Helper::ReadLock lock(m_sync);
					*lightIntensity = m_skyIntensity;
					if (m_skybox != nullptr) { return SUCCEEDED(m_skybox->QueryInterface(riid, ppvObject)); }
					return false;
				}

				Success(return == true) bool CLOAK_CALL_THIS Scene::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, RessourceHandler, Files::Scene_v1, Scene);
				}
				bool CLOAK_CALL_THIS Scene::iCheckSetting(In const API::Global::Graphic::Settings& nset) const { return false; }
				LoadResult CLOAK_CALL_THIS Scene::iLoad(In API::Files::IReader* read, In API::Files::FileVersion version)
				{
					if (m_skybox != nullptr) { m_skybox->load(); }
					for (const auto& a : m_entities) { a->load(); }
					//TODO
					return LoadResult::Finished;
				}
				void CLOAK_CALL_THIS Scene::iUnload()
				{
					if (m_skybox != nullptr) { m_skybox->unload(); }
					WorldNode* nodes = reinterpret_cast<WorldNode*>(m_nodes);
					for (size_t a = 0; a < m_nodeCount; a++) { nodes[a].~WorldNode(); }
					API::Global::Memory::MemoryHeap::Free(m_nodes);
					m_nodeCount = 0;
					m_nodes = nullptr;
					m_root = nullptr;
					for (const auto& a : m_entities) { a->unload(); }
					//TODO
				}

				CLOAK_CALL GeneratedScene::GeneratedScene()
				{
					API::List<GeneratedWorldNode*>* nodes = new API::List<GeneratedWorldNode*>();
					m_root = m_bottomLeftBack = m_topRightFront = new GeneratedWorldNode(this, { 0,0,0 });
					nodes->push_back(m_bottomLeftBack);
					m_nodes = nodes;
				}
				CLOAK_CALL GeneratedScene::~GeneratedScene()
				{
					API::List<GeneratedWorldNode*>* nodes = reinterpret_cast<API::List<GeneratedWorldNode*>*>(m_nodes);
					for (size_t a = 0; a < nodes->size(); a++) { delete nodes->at(a); }
					delete nodes;
					m_nodes = nullptr;
					m_nodeCount = 0;
				}
				void CLOAK_CALL GeneratedScene::Resize(In API::Files::Scene_v1::AdjacentPos dir)
				{
					API::Helper::WriteLock lock(m_sync);
					CloakDebugLog("Resize Generated Scene");
					API::List<GeneratedWorldNode*>* nodes = reinterpret_cast<API::List<GeneratedWorldNode*>*>(m_nodes);
					const API::Global::Math::IntVector& trf = m_topRightFront->GetPosition();
					const API::Global::Math::IntVector& blb = m_bottomLeftBack->GetPosition();
					const API::Global::Math::IntPoint size = static_cast<API::Global::Math::IntPoint>(API::Global::Math::IntVector(1, 1, 1) + trf - blb);
					const uint8_t dim = static_cast<uint8_t>(dir) >> 1;
					const bool neg = (static_cast<uint8_t>(dir) & 0x1) != 0;
					GeneratedWorldNode* t = neg ? m_bottomLeftBack : m_topRightFront;
					API::Global::Math::IntPoint p = static_cast<API::Global::Math::IntPoint>(t->GetPosition());
					API::Global::Math::IntVector pd = API::Global::Math::IntVector(dim == 0 ? 1 : 0, dim == 1 ? 1 : 0, dim == 2 ? 1 : 0);
					API::Global::Math::IntPoint pos = neg ? (p - pd) : (p + pd);

					const uint8_t lineD = (dim + 1) % 3;
					const uint8_t colD = (dim + 2) % 3;
					const size_t lineS = static_cast<size_t>(size[lineD]);
					const size_t colS = static_cast<size_t>(size[colD]);
					GeneratedWorldNode** line = NewArray(GeneratedWorldNode*, lineS);
					WorldNode* adj[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
					const uint8_t nextDim = (dim << 1) | (neg ? 0 : 1);
					const uint8_t prevDim = nextDim ^ static_cast<uint8_t>(0x1);
					const uint8_t nextLin = (lineD << 1) | (neg ? 0 : 1);
					const uint8_t prevLin = nextLin ^ static_cast<uint8_t>(0x1);
					const uint8_t nextCol = (colD << 1) | (neg ? 0 : 1);
					const uint8_t prevCol = nextCol ^ static_cast<uint8_t>(0x1);
					for (size_t a = 0; a < colS; a++)
					{
						GeneratedWorldNode* c = t;
						const int lLp = pos[lineD];
						for (size_t b = 0; b < lineS; b++)
						{
							p.X = pos[0];
							p.Y = pos[1];
							p.Z = pos[2];

							adj[nextDim] = c;
							if (b > 0) { adj[prevLin] = line[b - 1]; }
							else { adj[prevLin] = nullptr; }
							if (a > 0) { adj[prevCol] = line[b]; }
							else { adj[prevCol] = nullptr; }
							GeneratedWorldNode* nNode = new GeneratedWorldNode(this, p);
							nNode->SetAdjacentNodes(adj);
							nodes->push_back(nNode);

							c->SetAdjacentNode(nNode, static_cast<API::Files::Scene_v1::AdjacentPos>(prevDim));
							if (b > 0) { line[b - 1]->SetAdjacentNode(nNode, static_cast<API::Files::Scene_v1::AdjacentPos>(nextLin)); }
							if (a > 0) { line[b]->SetAdjacentNode(nNode, static_cast<API::Files::Scene_v1::AdjacentPos>(nextCol)); }
							line[b] = nNode;
							pos[lineD] += neg ? 1 : -1;
							c = dynamic_cast<GeneratedWorldNode*>(c->GetAdjacentWorldNode(static_cast<API::Files::Scene_v1::AdjacentPos>(nextLin)));
							CLOAK_ASSUME(b + 1 == lineS || c != nullptr);
						}
						pos[lineD] = lLp;
						pos[colD] += neg ? 1 : -1;
						t = dynamic_cast<GeneratedWorldNode*>(t->GetAdjacentWorldNode(static_cast<API::Files::Scene_v1::AdjacentPos>(nextCol)));
						CLOAK_ASSUME(a + 1 == colS || t != nullptr);
					}
					if (neg) { m_bottomLeftBack = dynamic_cast<GeneratedWorldNode*>(m_bottomLeftBack->GetAdjacentWorldNode(dir)); }
					else { m_topRightFront = dynamic_cast<GeneratedWorldNode*>(m_topRightFront->GetAdjacentWorldNode(dir)); }

					DeleteArray(line);
				}

				Success(return == true) bool CLOAK_CALL_THIS GeneratedScene::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, Scene, Files::Scene_v1, GeneratedScene);
				}
				LoadResult CLOAK_CALL_THIS GeneratedScene::iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) { return LoadResult::Finished; }
				void CLOAK_CALL_THIS GeneratedScene::iUnload() {}
				bool CLOAK_CALL_THIS GeneratedScene::iLoadFile(Out bool* keepLoadingBackground) 
				{ 
					*keepLoadingBackground = false;
					API::Helper::WriteLock lock(m_sync);
					if (m_skybox != nullptr) { m_skybox->load(); }
					for (const auto& a : m_entities) { a->load(); }

					return true; 
				}
				void CLOAK_CALL_THIS GeneratedScene::iUnloadFile() 
				{
					API::Helper::WriteLock lock(m_sync);
					if (m_skybox != nullptr) { m_skybox->unload(); }
					for (const auto& a : m_entities) { a->unload(); }
				}
				void CLOAK_CALL_THIS GeneratedScene::iDelayedLoadFile(Out bool* keepLoadingBackground) { *keepLoadingBackground = false; }

				IMPLEMENT_FILE_HANDLER(Scene);
			}
		}
	}
}