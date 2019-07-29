#include "stdafx.h"
#include "Implementation/Files/Mesh.h"
#include "Implementation/Rendering/VertexData.h"
#include "Implementation/Global/Graphic.h"

#include "CloakEngine/Global/Memory.h"
#include "Engine/Graphic/Core.h"

#include <assert.h>

#define V1000_READ_BONE_S(id,w,bc,n,read,L) if(bc>n){id.L = static_cast<API::Rendering::UINT1>(read->ReadBits(32)); w.L=static_cast<API::Rendering::FLOAT1>(read->ReadDouble(32));} else {id.L=0; w.L=0;}
#define v1000_READ_BONE(res,read,bc) V1000_READ_BONE_S(res->BoneID,res->BoneWeight,bc,3,read,W) V1000_READ_BONE_S(res->BoneID,res->BoneWeight,bc,2,read,Z) V1000_READ_BONE_S(res->BoneID,res->BoneWeight,bc,1,read,Y) V1000_READ_BONE_S(res->BoneID,res->BoneWeight,bc,0,read,X)

//#define PRINT_VERTEX_BUFFER

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Mesh_v1 {
				constexpr API::Files::FileType g_fileType{ "MeshFile","CEMF",1003 };

#if (defined(_DEBUG) && defined(PRINT_VERTEX_BUFFER))
#define PrintVB(vb, vbp, s) __DBG_PRINT_VB(vb, vbp, s)
#define PrintIB(ib, s) __DBG_PRINT_IB(ib, s)
#define PrintMats(mt, s) __DBG_PRINT_MAT(mt, s)
				inline void __DBG_PRINT_VB(In Impl::Rendering::VERTEX_3D* vb, In Impl::Rendering::VERTEX_3D_POSITION* vbp, In size_t size)
				{
					for (size_t a = 0; a < size; a++)
					{
						CloakDebugLog("Read vertex " + std::to_string(a));
						const Impl::Rendering::VERTEX_3D& v = vb[a];
						const Impl::Rendering::VERTEX_3D_POSITION& vp = vbp[a];
						CloakDebugLog("\t- Position = [" + std::to_string(vp.Position.X) + "|" + std::to_string(vp.Position.Y) + "|" + std::to_string(vp.Position.Z) + "]");
						CloakDebugLog("\t- Normal = [" + std::to_string(v.Normal.X) + "|" + std::to_string(v.Normal.Y) + "|" + std::to_string(v.Normal.Z) + "]");
						CloakDebugLog("\t- Binormal = [" + std::to_string(v.Binormal.X) + "|" + std::to_string(v.Binormal.Y) + "|" + std::to_string(v.Binormal.Z) + "]");
						CloakDebugLog("\t- Tangent = [" + std::to_string(v.Tangent.X) + "|" + std::to_string(v.Tangent.Y) + "|" + std::to_string(v.Tangent.Z) + "]");
						CloakDebugLog("\t- TexCoord = [" + std::to_string(v.TexCoord.X) + "|" + std::to_string(v.TexCoord.Y) + "]");
					}
				}
				inline void __DBG_PRINT_MAT(In API::Files::MeshMaterialInfo* mats, In size_t size)
				{
					for (size_t a = 0; a < size; a++)
					{
						CloakDebugLog("Read Material[" + std::to_string(a) + "]: Start at " + std::to_string(mats[a].Offset) + " End at " + std::to_string(mats[a].Offset + mats[a].Size));
					}
				}
				template<typename T> inline void __DBG_PRINT_IB(In T* ib, In size_t size)
				{
					for (size_t a = 0; a < size; a++)
					{
						CloakDebugLog("Read Index[" + std::to_string(a) + "]: " + std::to_string(ib[a]));
					}
				}
#else
#define PrintVB(vb, vbp, s)
#define PrintIB(ib, s)
#define PrintMats(mt, s)
#endif

				namespace v1000 {
					constexpr uint16_t STRIP_CUT_IB16 = 0xFFFF;
					constexpr uint32_t STRIP_CUT_IB32 = 0xFFFFFFFF;

					template<typename T> inline void CLOAK_CALL StripToList(Inout T** ib, Inout API::Files::MeshMaterialInfo* matInfo, Inout size_t* matInfoSize, Out uint32_t* ibs)
					{
						constexpr T cutValue = ~static_cast<T>(0);
						//Calculate required size:
						uint32_t triangles = 0;
						uint32_t listSize = 0;
						for (size_t a = 0; a < *matInfoSize; a++)
						{
							T lastIB[3] = { cutValue,cutValue,cutValue };
							for (size_t b = 0; b < matInfo[a].SizeStrip;)
							{
								do {
									lastIB[0] = lastIB[1];
									lastIB[1] = lastIB[2];
									lastIB[2] = (*ib)[matInfo[a].Offset + b];
									b++;
								} while ((lastIB[0] == cutValue || lastIB[1] == cutValue || lastIB[2] == cutValue) && b < matInfo[a].SizeStrip);
								if (lastIB[0] != lastIB[1] && lastIB[0] != lastIB[2] && lastIB[1] != lastIB[2])
								{
									CLOAK_ASSUME(lastIB[0] != cutValue);
									CLOAK_ASSUME(lastIB[1] != cutValue);
									CLOAK_ASSUME(lastIB[2] != cutValue);
									triangles++;
								}
							}
							listSize += static_cast<uint32_t>(matInfo[a].SizeList);
						}
						listSize += triangles * 3;
						//Transform:
						if (triangles > 0)
						{
							T* nib = reinterpret_cast<T*>(NewArray(uint8_t, listSize * sizeof(T)));
							size_t p = 0;
							for (size_t a = 0; a < *matInfoSize; a++)
							{
								const size_t off = matInfo[a].Offset;
								matInfo[a].Offset = p;
								T lastIB[3] = { cutValue,cutValue,cutValue };
								bool winding = false;
								for (size_t b = 0; b < matInfo[a].SizeStrip;)
								{
									bool cut = false;
									do {
										lastIB[0] = lastIB[1];
										lastIB[1] = lastIB[2];
										lastIB[2] = (*ib)[off + b];
										b++;
										cut = (lastIB[0] == cutValue || lastIB[1] == cutValue || lastIB[2] == cutValue);
										if (cut) { winding = false; }
									} while (cut == true && b < matInfo[a].SizeStrip);
									if (lastIB[0] != lastIB[1] && lastIB[0] != lastIB[2] && lastIB[1] != lastIB[2])
									{
										if (winding)
										{
											nib[p + 0] = lastIB[1];
											nib[p + 1] = lastIB[0];
											nib[p + 2] = lastIB[2];
										}
										else
										{
											nib[p + 0] = lastIB[0];
											nib[p + 1] = lastIB[1];
											nib[p + 2] = lastIB[2];
										}
										p += 3;
										CLOAK_ASSUME(p <= listSize);
										winding = !winding;
									}
								}
								if (matInfo[a].SizeList > 0)
								{
									memcpy(&nib[p], &(*ib)[matInfo[a].Offset + matInfo[a].SizeStrip], matInfo[a].SizeList * sizeof(T));
									p += matInfo[a].SizeList;
								}
								matInfo[a].SizeList = p - matInfo[a].Offset;
								CLOAK_ASSUME(p <= listSize);
							}
							assert(p == listSize);
							DeleteArray(reinterpret_cast<uint8_t*>(*ib));
							*ib = nib;
							*ibs = listSize;
						}
						//Combine:
						size_t resInfoSize = 0;
						for (size_t a = 0; a < *matInfoSize; a++)
						{
							if (resInfoSize == 0 || matInfo[resInfoSize - 1].Offset + matInfo[resInfoSize - 1].SizeList + matInfo[resInfoSize - 1].SizeStrip != matInfo[a].Offset || matInfo[resInfoSize - 1].MaterialID != matInfo[a].MaterialID)
							{
								if (a != resInfoSize) { matInfo[resInfoSize] = matInfo[a]; }
								resInfoSize++;
							}
						}
						*matInfoSize = resInfoSize;
					}

					inline API::Global::Math::Vector CLOAK_CALL ReadVector(In API::Files::IReader* read)
					{
						const float X = static_cast<float>(read->ReadDouble(32));
						const float Y = static_cast<float>(read->ReadDouble(32));
						const float Z = static_cast<float>(read->ReadDouble(32));
						return API::Global::Math::Vector(X, Y, Z);
					}
					inline void CLOAK_CALL ReadSingleVertex(In API::Files::IReader* read, In API::Files::FileVersion version, In bool useBones, Out Impl::Rendering::VERTEX_3D* vertex, Out Impl::Rendering::VERTEX_3D_POSITION* pos)
					{
						pos->Position = (API::Global::Math::Point)ReadVector(read);
						vertex->Normal = (API::Global::Math::Point)ReadVector(read);
						vertex->Binormal = (API::Global::Math::Point)ReadVector(read);
						vertex->Tangent = (API::Global::Math::Point)ReadVector(read);
						vertex->TexCoord.X = static_cast<API::Rendering::FLOAT1>(read->ReadDouble(32));
						vertex->TexCoord.Y = static_cast<API::Rendering::FLOAT1>(read->ReadDouble(32));
						uint8_t bc = 0;
						if (useBones) { bc = static_cast<uint8_t>(read->ReadBits(2)); }
						v1000_READ_BONE(vertex, read, bc);
					}
					inline void CLOAK_CALL ReadBoneUsage(In API::Files::IReader* read, In API::Files::FileVersion version, In size_t boneCount)
					{
						for (size_t a = 0; a < boneCount; a++) { read->ReadBits(1); }
						//TODO: Bone Usage is currently not used, may be discarded in future file version (?)
					}
					inline void CLOAK_CALL ReadRawVertexBuffer(In API::Files::IReader* read, In API::Files::FileVersion version, In size_t vbs, In size_t boneCount, Out Impl::Rendering::VERTEX_3D* vb, Out Impl::Rendering::VERTEX_3D_POSITION* vbp, Out API::Files::MeshMaterialInfo* matInfo)
					{
						size_t cs = 0;
						while (cs < vbs)
						{
							const size_t s = static_cast<size_t>(read->ReadBits(32));
							const size_t m = static_cast<size_t>(read->ReadBits(32));
							matInfo[m].Offset = cs;
							matInfo[m].SizeList = s;
							matInfo[m].SizeStrip = 0;
							matInfo[m].MaterialID = m;
							ReadBoneUsage(read, version, boneCount);
							for (size_t a = 0; a < s; a++) 
							{ 
								ReadSingleVertex(read, version, boneCount > 0, &vb[cs + a], &vbp[cs + a]);
							}
							cs += s;
						}
					}
					inline void CLOAK_CALL ReadIndexVertexBuffer(In API::Files::IReader* read, In API::Files::FileVersion version, In size_t vbs, In size_t boneCount, Out Impl::Rendering::VERTEX_3D* vb, Out Impl::Rendering::VERTEX_3D_POSITION* vbp)
					{
						for (size_t a = 0; a < vbs; a++) { ReadSingleVertex(read, version, boneCount > 0, &vb[a], &vbp[a]); }
					}
					inline bool CLOAK_CALL ReadIndexBuffer(In API::Files::IReader* read, In API::Files::FileVersion version, In size_t ibs, In bool i32, In bool shortened, In size_t boneCount, Out uint8_t* ib, Out API::Files::MeshMaterialInfo* matInfo)
					{
						uint32_t* ib32 = reinterpret_cast<uint32_t*>(ib);
						uint16_t* ib16 = reinterpret_cast<uint16_t*>(ib);
						size_t cs = 0;
						bool res = false;
						size_t m = 0;
						while (cs < ibs)
						{
							if (version >= 1003)
							{
								const size_t cS = static_cast<size_t>(read->ReadBits(32));
								const size_t cL = static_cast<size_t>(read->ReadBits(32));
								const size_t mid = static_cast<size_t>(read->ReadBits(32));
								CLOAK_ASSUME(cS > 0 || cL > 0);
								matInfo[m].Offset = cs;
								matInfo[m].SizeStrip = cS;
								matInfo[m].SizeList = cL;
								matInfo[m].MaterialID = mid;
								if (matInfo[m].SizeStrip > 0) { res = true; }
								ReadBoneUsage(read, version, boneCount);
								for (size_t a = 0; a < cS; a++)
								{
									if (i32 == true && shortened == false) { ib32[cs + a] = static_cast<uint32_t>(read->ReadBits(32)); }
									else
									{
										const uint16_t r = static_cast<uint16_t>(read->ReadBits(16));
										if (i32 == true) { ib32[cs + a] = r == STRIP_CUT_IB16 ? STRIP_CUT_IB32 : r; }
										else { ib16[cs + a] = r; }
									}
								}
								cs += cS;
								for (size_t a = 0; a < cL; a++)
								{
									if (i32 == true && shortened == false) { ib32[cs + a] = static_cast<uint32_t>(read->ReadBits(32)); }
									else
									{
										const uint16_t r = static_cast<uint16_t>(read->ReadBits(16));
										if (i32 == true) { ib32[cs + a] = r; }
										else { ib16[cs + a] = r; }
									}
								}
								cs += cL;
								m++;
							}
							else
							{
								const size_t s = static_cast<size_t>(read->ReadBits(32));
								const size_t mid = static_cast<size_t>(read->ReadBits(32));
								CLOAK_ASSUME(s > 0);
								bool strip = false;
								if (version == 1000) { m = mid; }
								else if (version >= 1001) { strip = read->ReadBits(1) == 1; }
								matInfo[m].Offset = cs;
								if (strip == true) { matInfo[m].SizeList = 0; matInfo[m].SizeStrip = s; }
								else { matInfo[m].SizeList = s; matInfo[m].SizeStrip = 0; }
								matInfo[m].MaterialID = mid;
								if (matInfo[m].SizeStrip > 0) { res = true; }
								ReadBoneUsage(read, version, boneCount);
								for (size_t a = 0; a < s; a++)
								{
									if (i32 == true && shortened == false) { ib32[cs + a] = static_cast<uint32_t>(read->ReadBits(32)); }
									else
									{
										const uint16_t r = static_cast<uint16_t>(read->ReadBits(16));
										if (i32 == true) { ib32[cs + a] = r == STRIP_CUT_IB16 ? STRIP_CUT_IB32 : r; }
										else { ib16[cs + a] = r; }
									}
								}
								cs += s;
								m++;
							}
						}
						return res;
					}
				}

				CLOAK_CALL Mesh::Mesh()
				{
					m_mats = nullptr;
					m_index = nullptr;
					for (size_t a = 0; a < ARRAYSIZE(m_vertex); a++) { m_vertex[a] = nullptr; }
					m_matCount = 0;
					m_matIDCount = 0;
					m_boneCount = 0;
					DEBUG_NAME(Mesh);
				}
				CLOAK_CALL Mesh::~Mesh()
				{
					SAVE_RELEASE(m_index);
					for (size_t a = 0; a < ARRAYSIZE(m_vertex); a++) { SAVE_RELEASE(m_vertex[a]); }
					DeleteArray(m_mats);
				}
				API::Files::IBoundingVolume* CLOAK_CALL_THIS Mesh::CreateBoundingVolume() const
				{
					API::Helper::ReadLock lock(m_sync);
					if (isLoaded()) { return API::Files::CreateBoundingVolume(m_boundDesc); }
					return nullptr;
				}
				API::Rendering::IVertexBuffer* CLOAK_CALL_THIS Mesh::GetVertexBuffer(In size_t slot, In API::Files::LOD lod) const
				{
					API::Helper::ReadLock lock(m_sync);
					if (isLoaded() && slot < ARRAYSIZE(m_vertex)) { return m_vertex[slot]; }
					return nullptr;
				}
				Ret_maybenull API::Rendering::IIndexBuffer* CLOAK_CALL_THIS Mesh::GetIndexBuffer(In API::Files::LOD lod) const
				{
					API::Helper::ReadLock lock(m_sync);
					if (isLoaded()) { return m_index; }
					return nullptr;
				}
				size_t CLOAK_CALL_THIS Mesh::GetMaterialCount() const
				{
					API::Helper::ReadLock lock(m_sync);
					return m_matIDCount;
				}
				size_t CLOAK_CALL_THIS Mesh::GetMaterialInfoCount() const
				{
					API::Helper::ReadLock lock(m_sync);
					return m_matCount;
				}
				API::Files::MeshMaterialInfo CLOAK_CALL_THIS Mesh::GetMaterialInfo(In size_t mat, In API::Files::LOD lod) const
				{
					API::Helper::ReadLock lock(m_sync);
					if (isLoaded() && mat < m_matCount) { return m_mats[mat]; }
					return { 0,0,0,API::Rendering::TOPOLOGY_TRIANGLELIST };
				}

				void CLOAK_CALL_THIS Mesh::RequestLOD(In API::Files::LOD lod)
				{
					//TODO
				}

				Success(return == true) bool CLOAK_CALL_THIS Mesh::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, RessourceHandler, Files::Mesh_v1, Mesh);
				}
				bool CLOAK_CALL_THIS Mesh::iCheckSetting(In const API::Global::Graphic::Settings& nset) const
				{
					API::Helper::ReadLock lock(m_sync);
					return m_tessellation != nset.Tessellation && m_includeStrips;
				}
				LoadResult CLOAK_CALL_THIS Mesh::iLoad(In API::Files::IReader* read, In API::Files::FileVersion version)
				{
					API::Helper::WriteLock lock(m_sync);
					if (version >= 1000)
					{
						API::Global::Graphic::Settings gset;
						Impl::Global::Graphic::GetModifedSettings(&gset);
						m_tessellation = gset.Tessellation;
						m_boneCount = static_cast<uint32_t>(read->ReadBits(32));
						if (m_boneCount == 0)
						{
							const uint8_t bvt = static_cast<uint8_t>(read->ReadBits(2));
							if (bvt == 0) //No bounding volume
							{
								m_boundDesc.Type = API::Files::BoundingType::NONE;
							}
							else if (bvt == 1) //Bounding Box
							{
								m_boundDesc.Type = API::Files::BoundingType::BOX;
								m_boundDesc.Box.Center = v1000::ReadVector(read);
								for (size_t a = 0; a < 3; a++) { m_boundDesc.Box.ScaledAxis[a] = v1000::ReadVector(read); }
								for (size_t a = 0; a < 3; a++) { m_boundDesc.Box.ScaledAxis[a] = m_boundDesc.Box.ScaledAxis[a].Normalize()*static_cast<float>(read->ReadDouble(32)); }
							}
							else if (bvt == 2) //Bounding Sphere
							{
								m_boundDesc.Type = API::Files::BoundingType::SPHERE;
								m_boundDesc.Sphere.Center = v1000::ReadVector(read);
								m_boundDesc.Sphere.Radius = static_cast<float>(read->ReadDouble(32));
							}
						}
						else
						{
							//TODO
						}

						m_matCount = static_cast<size_t>(read->ReadBits(16)) + 1;
						m_mats = NewArray(API::Files::Mesh_v1::MeshMaterialInfo,  m_matCount);
						const uint32_t vbs = static_cast<uint32_t>(read->ReadBits(32));
						Impl::Rendering::VERTEX_3D* vb = new Impl::Rendering::VERTEX_3D[vbs];
						Impl::Rendering::VERTEX_3D_POSITION* vbp = new Impl::Rendering::VERTEX_3D_POSITION[vbs];

						Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();

						if (read->ReadBits(1) == 1) //Use Index Buffer
						{
							bool i32 = read->ReadBits(1) == 1;
							bool ibsc = false;
							if (version >= 1002 && i32 == false) 
							{ 
								ibsc = read->ReadBits(1) == 1; 
								i32 = ibsc; //index buffer is encoded in 16 bit, but we actualy need 32 bit to store strip cut values
							}
							uint32_t ibs = static_cast<uint32_t>(read->ReadBits(32));
							uint8_t* ib = NewArray(uint8_t, (i32 ? 4 : 2)*ibs);

							v1000::ReadIndexVertexBuffer(read, version, vbs, m_boneCount, vb, vbp);
							m_includeStrips = v1000::ReadIndexBuffer(read, version, ibs, i32, ibsc, m_boneCount, ib, m_mats);
							if (i32)
							{
								uint32_t* i = reinterpret_cast<uint32_t*>(ib);
								if (gset.Tessellation)
								{ 
									v1000::StripToList(&i, m_mats, &m_matCount, &ibs); 
									ib = reinterpret_cast<uint8_t*>(i);
								}
								m_index = man->CreateIndexBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, ibs, i);
								PrintIB(i, ibs);
							}
							else
							{
								uint16_t* i = reinterpret_cast<uint16_t*>(ib);
								if (gset.Tessellation)
								{ 
									v1000::StripToList(&i, m_mats, &m_matCount, &ibs); 
									ib = reinterpret_cast<uint8_t*>(i);
								}
								m_index = man->CreateIndexBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, ibs, i);
								PrintIB(i, ibs);
							}
							DeleteArray(ib);
						}
						else //Use raw vertex buffer
						{
							v1000::ReadRawVertexBuffer(read, version, vbs, m_boneCount, vb, vbp, m_mats);
							m_index = nullptr;
							m_includeStrips = false;
						}
						//Removimg empty mats:
						size_t nmc = 0;
						m_matIDCount = 0;
						for (size_t a = 0, b = 0; a < m_matCount; a++)
						{
							if (m_mats[a].SizeList > 0 || m_mats[a].SizeStrip > 0)
							{
								if (a != b) { m_mats[b] = m_mats[a]; }
								b++;
								nmc++;
								m_matIDCount = max(m_matIDCount, m_mats[a].MaterialID + 1);
							}
						}
						m_matCount = nmc;
						PrintVB(vb, vbp, vbs);
						PrintMats(m_mats, m_matCount);
						m_vertex[0] = man->CreateVertexBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, vbs, sizeof(Impl::Rendering::VERTEX_3D_POSITION), vbp);
						m_vertex[1] = man->CreateVertexBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, vbs, sizeof(Impl::Rendering::VERTEX_3D), vb);
						delete[] vb;
						delete[] vbp;
					}
					return LoadResult::Finished;
				}
				void CLOAK_CALL_THIS Mesh::iUnload()
				{
					SAVE_RELEASE(m_index);
					for (size_t a = 0; a < ARRAYSIZE(m_vertex); a++) { SAVE_RELEASE(m_vertex[a]); }
					DeleteArray(m_mats);
					m_mats = nullptr;
					m_matCount = 0;
				}

				IMPLEMENT_FILE_HANDLER(Mesh);
			}
		}
	}
}