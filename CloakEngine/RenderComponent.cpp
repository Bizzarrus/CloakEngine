#include "stdafx.h"
#include "Implementation/Components/Render.h"
#include "Implementation/Rendering/Manager.h"

#pragma warning(push)
#pragma warning(disable: 4436)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			namespace Render_v1 {
				CLOAK_CALL Render::Render(In API::World::IEntity* parent, In type_id typeID) : IRender(parent, typeID), Component(parent, typeID), IGraphicUpdatePosition(parent, typeID)
				{
					m_mats = nullptr;
					m_matCount = 0;
					m_meshMatCount = 0;
					m_mesh = nullptr;
					m_bounding = nullptr;
					m_localBuffer = nullptr;
					m_pos = API::Global::Math::Vector(0, 0, 0);
					m_loaded = false;
					m_lod = API::Files::LOD::LOW;
				}
				CLOAK_CALL Render::Render(In API::World::IEntity* parent, In API::World::Component* src) : Render(parent, src->TypeID)
				{
					Render* o = dynamic_cast<Render*>(src);
					if (o != nullptr)
					{
						m_mesh = o->m_mesh;
						if (m_mesh != nullptr) { m_mesh->AddRef(); }
						m_matCount = o->m_matCount;
						m_mats = reinterpret_cast<API::Files::IMaterial**>(API::Global::Memory::MemoryHeap::Allocate(sizeof(API::Files::IMaterial*)*m_matCount));
						for (size_t a = 0; a < m_matCount; a++)
						{
							if (o->m_mats[a] != nullptr)
							{
								m_mats[a] = o->m_mats[a];
								o->m_mats[a]->AddRef();
							}
						}
					}
				}
				CLOAK_CALL Render::~Render()
				{
					if (m_loaded)
					{
						if (m_mesh != nullptr) { m_mesh->unload(); }
						for (size_t a = 0; a < m_matCount; a++)
						{
							if (m_mats[a] != nullptr) { m_mats[a]->unload(); }
						}
					}
					for (size_t a = 0; a < m_matCount; a++) { SAVE_RELEASE(m_mats[a]); }
					SAVE_RELEASE(m_mesh);
					SAVE_RELEASE(m_bounding);
					SAVE_RELEASE(m_localBuffer);
					API::Global::Memory::MemoryHeap::Free(m_mats);
				}

				void CLOAK_CALL_THIS Render::SetMesh(In API::Files::IMesh* mesh)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_mesh != mesh)
					{
						if (m_mesh != nullptr)
						{
							if (m_loaded) { m_mesh->unload(); }
							m_mesh->Release();
						}
						m_mesh = mesh;
						SAVE_RELEASE(m_bounding);
						if (m_mesh != nullptr)
						{
							m_mesh->AddRef();
							m_mesh->RequestLOD(m_lod);
							if (m_loaded) { m_mesh->load(); }
						}
					}
				}
				void CLOAK_CALL_THIS Render::SetMaterials(In_reads(size) API::Files::IMaterial** mat, In size_t size)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					for (size_t a = 0; a < size; a++)
					{
						if (mat[a] != nullptr)
						{
							mat[a]->AddRef();
							mat[a]->RequestLOD(m_lod);
							if (m_loaded) { mat[a]->load(); }
						}
					}
					for (size_t a = 0; a < m_matCount; a++)
					{
						if (m_mats[a] != nullptr) 
						{
							if (m_loaded) { m_mats[a]->unload(); }
							m_mats[a]->Release(); 
						}
					}
					if (m_matCount != size)
					{
						API::Global::Memory::MemoryHeap::Free(m_mats);
						m_mats = reinterpret_cast<API::Files::IMaterial**>(API::Global::Memory::MemoryHeap::Allocate(sizeof(API::Files::IMaterial*)*size));
						m_matCount = size;
					}
					for (size_t a = 0; a < m_matCount; a++) { m_mats[a] = mat[a]; }
				}
				void CLOAK_CALL_THIS Render::SetMaterials(In const API::List<API::Files::IMaterial*>& mats, In_opt size_t size)
				{
					if (size > mats.size()) { size = mats.size(); }
					API::Helper::WriteLock lock = m_parent->LockWrite();
					for (size_t a = 0; a < size; a++)
					{
						API::Files::IMaterial* mat = mats[a];
						if (mat != nullptr)
						{
							mat->AddRef();
							mat->RequestLOD(m_lod);
							if (m_loaded) { mat->load(); }
						}
					}
					for (size_t a = 0; a < m_matCount; a++)
					{
						if (m_mats[a] != nullptr) 
						{
							if (m_loaded) { m_mats[a]->unload(); }
							m_mats[a]->Release(); 
						}
					}
					if (m_matCount != size)
					{
						API::Global::Memory::MemoryHeap::Free(m_mats);
						m_mats = reinterpret_cast<API::Files::IMaterial**>(API::Global::Memory::MemoryHeap::Allocate(sizeof(API::Files::IMaterial*)*size));
						m_matCount = size;
					}
					for (size_t a = 0; a < m_matCount; a++) { m_mats[a] = mats[a]; }
				}
				void CLOAK_CALL_THIS Render::SetMaterialCount(In size_t size)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_matCount != size)
					{
						API::Files::IMaterial** nm = reinterpret_cast<API::Files::IMaterial**>(API::Global::Memory::MemoryHeap::Allocate(sizeof(API::Files::IMaterial*)*size));
						const size_t mis = min(m_matCount, size);
						for (size_t a = 0; a < mis; a++) { nm[a] = m_mats[a]; }
						if (size > m_matCount)
						{
							for (size_t a = mis; a < size; a++) { nm[a] = nullptr; }
						}
						else
						{
							for (size_t a = mis; a < m_matCount; a++)
							{
								if (m_mats[a] != nullptr)
								{
									if (m_loaded) { m_mats[a]->unload(); }
									m_mats[a]->Release();
								}
							}
						}
						API::Global::Memory::MemoryHeap::Free(m_mats);
						m_mats = nm;
						m_matCount = size;
					}
				}
				void CLOAK_CALL_THIS Render::SetMaterial(In size_t id, In API::Files::IMaterial* mat)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (CloakDebugCheckOK(id < m_matCount, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						if (mat != nullptr)
						{
							mat->AddRef();
							mat->RequestLOD(m_lod);
							if (m_loaded) { mat->load(); }
						}
						if (m_mats[id] != nullptr)
						{
							if (m_loaded) { m_mats[id]->unload(); }
							m_mats[id]->Release();
						}
						m_mats[id] = mat;
					}
				}
				void CLOAK_CALL_THIS Render::SetMaterial(In size_t id, In const API::RefPointer<API::Files::IMaterial>& mat)
				{
					API::Files::IMaterial* m = mat;
					SetMaterial(id, m);
					m->Release();
				}

				void CLOAK_CALL_THIS Render::OnLoad(In API::Files::Priority prio, In API::Files::LOD lod)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_mesh != nullptr)
					{
						m_mesh->RequestLOD(lod);
						if (m_loaded == false) { m_mesh->load(); }
					}
					for (size_t a = 0; a < m_matCount; a++)
					{
						if (m_mats[a] != nullptr)
						{
							m_mats[a]->RequestLOD(lod);
							if (m_loaded == false) { m_mats[a]->load(); }
						}
					}
					m_lod = lod;
					m_loaded = true;
				}
				void CLOAK_CALL_THIS Render::OnUnload()
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_loaded == true)
					{
						if (m_mesh != nullptr)
						{
							m_mesh->unload();
						}
						for (size_t a = 0; a < m_matCount; a++)
						{
							if (m_mats[a] != nullptr) { m_mats[a]->unload(); }
						}
						m_loaded = false;
					}
				}

				void CLOAK_CALL_THIS Render::Update(In API::Global::Time time, In Impl::Components::Position* position, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					Impl::Files::WorldNode* node = nullptr;
					const API::Global::Math::Matrix& LTN = position->GetLocalToNode(time, &node);
					
					if (m_mesh != nullptr && node != nullptr && m_mesh->isLoaded(API::Files::Priority::NORMAL) && IsActive())
					{
						if (m_bounding == nullptr) { m_bounding = m_mesh->CreateBoundingVolume(); }
						if (m_localBuffer == nullptr)
						{
							m_localBuffer = manager->CreateConstantBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, sizeof(Impl::Rendering::MESH_DATA));
						}
						API::Global::Math::Matrix LTW = LTN * node->GetNodeToWorld(&m_worldID);
						API::Global::Math::Matrix WTL = API::Global::Math::Matrix::Inverse(LTW);
						m_bounding->Update();
						m_bounding->SetLocalToWorld(LTW);
						Impl::Rendering::MESH_DATA data;
						data.LocalToWorld = LTW;
						data.WorldToLocal = WTL;
						data.Wetness = 0; //TODO
						context->WriteBuffer(m_localBuffer, 0, &data, sizeof(Impl::Rendering::MESH_DATA));

						m_meshMatCount = m_mesh->GetMaterialInfoCount();
						m_pos = LTW.GetTranslation();
					}
					else { m_meshMatCount = 0; }
				}
				float CLOAK_CALL_THIS Render::GetBoundingRadius() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					if (m_bounding != nullptr) { return m_bounding->GetRadius(); }
					return 0;
				}
				void CLOAK_CALL_THIS Render::Draw(In API::Rendering::IContext3D* context, In_opt API::Files::LOD lod)
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					if (m_meshMatCount > 0)
					{
						const API::Files::LOD rl = context->CalculateLOD(m_pos, lod);
						if (context->SetMeshData(m_worldID, m_bounding, m_localBuffer, m_mesh, rl) == true)
						{
							for (size_t a = 0; a < m_meshMatCount; a++)
							{
								API::Files::MeshMaterialInfo mi = m_mesh->GetMaterialInfo(a, rl);
								if (mi.MaterialID < m_matCount) { context->Draw(this, m_mats[mi.MaterialID], mi); }
							}
						}
					}
				}
			}
		}
	}
}

#pragma warning(pop)