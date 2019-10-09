#include "stdafx.h"
#include "Implementation/Rendering/Context.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace Context_v1 {
				constexpr size_t DATA_ALIGNMENT_MASK = 15;
				constexpr float LOD_RANGE[] = { 0.125f, 0.25f, 0.5f };

#ifdef _DEBUG
				CLOAK_FORCEINLINE constexpr bool __CheckLODRange() { for (size_t a = 1; a < ARRAYSIZE(LOD_RANGE); a++) { if (LOD_RANGE[a - 1] >= LOD_RANGE[a]) { return false; } } return true; }
				static_assert(__CheckLODRange(), "LOD Ranges must be ordered from highest quality (lowest range) to lowest quality (highest range)");
#endif

				struct DrawBatch {
					API::Rendering::IColorBuffer* Buffer;
					size_t Count;
				};

				CLOAK_FORCEINLINE constexpr size_t CLOAK_CALL TriangleCount(In API::Rendering::PRIMITIVE_TOPOLOGY top, In size_t drawCount)
				{
					switch (top)
					{
						case API::Rendering::TOPOLOGY_TRIANGLESTRIP:
							return drawCount - 2;
						case API::Rendering::TOPOLOGY_TRIANGLESTRIP_ADJ:
							return (drawCount - 3) / 2;
						case API::Rendering::TOPOLOGY_TRIANGLELIST:
							return drawCount / 3;
						case API::Rendering::TOPOLOGY_TRIANGLELIST_ADJ:
							return drawCount / 6;
						default:
							break;
					}
					return 0;
				}

				CLOAK_CALL Context2D::Context2D(In IContext* parent, API::Global::Memory::PageStackAllocator* pageAlloc) : m_parent(parent), m_pageAlloc(pageAlloc), m_ZPatch(pageAlloc), m_scissor(pageAlloc)
				{
					m_MaxZ = 0;
				}

				void CLOAK_CALL_THIS Context2D::Draw(In API::Rendering::IDrawable2D* drawing, In API::Files::ITexture* img, In const API::Rendering::VERTEX_2D& vertex, In UINT Z, In_opt size_t bufferNum)
				{
					if (img != nullptr && img->isLoaded(API::Files::Priority::NORMAL))
					{
						Draw(drawing, img->GetBuffer(bufferNum), vertex, Z);
					}
				}
				void CLOAK_CALL_THIS Context2D::Draw(In API::Rendering::IDrawable2D* drawing, In API::Rendering::IColorBuffer* img, In const API::Rendering::VERTEX_2D& vertex, In UINT Z)
				{
					if (img != nullptr)
					{
						UINT ZBase = 0;
						if (m_ZPatch.empty() == false) { ZBase = m_ZPatch.top(); }
						const UINT ZPos = ZBase + Z;
						API::Rendering::SCALED_SCISSOR_RECT scissor;
						if (m_scissor.size() == 0)
						{
							scissor.TopLeftX = 0;
							scissor.TopLeftY = 0;
							scissor.Width = 1;
							scissor.Height = 1;
						}
						else { scissor = m_scissor.top(); }

						VERTEX_2D ver;
						ver.Size = vertex.Size;
						ver.TexTopLeft = vertex.TexTopLeft;
						ver.TexBottomRight = vertex.TexBottomRight;
						for (size_t a = 0; a < 3; a++)
						{
							ver.Color[a].X = vertex.Color[a].R;
							ver.Color[a].Y = vertex.Color[a].G;
							ver.Color[a].Z = vertex.Color[a].B;
							ver.Color[a].W = vertex.Color[a].A;
						}
						ver.Position = vertex.Position;
						ver.Flags = 0;
						ver.Rotation.X = vertex.Rotation.Sinus;
						ver.Rotation.Y = vertex.Rotation.Cosinus;
						ver.Scissor.X = scissor.TopLeftX;
						ver.Scissor.Y = scissor.TopLeftY;
						ver.Scissor.Z = scissor.Width;
						ver.Scissor.W = scissor.Height;
						if ((vertex.Flags & API::Rendering::Vertex2DFlags::MULTICOLOR) != API::Rendering::Vertex2DFlags::NONE) { ver.Flags |= 0x1; }
						if ((vertex.Flags & API::Rendering::Vertex2DFlags::GRAYSCALE) != API::Rendering::Vertex2DFlags::NONE) { ver.Flags |= 0x2; }

						if (ver.Size.X > 0 && ver.Size.Y > 0 && ver.Position.X <= 1 && ver.Position.Y <= 1 && ver.Position.X + ver.Size.X >= 0 && ver.Position.Y + ver.Size.Y >= 0)
						{
							m_vertexBuffer.push_back({ img, ZPos, ver });
							m_MaxZ = max(m_MaxZ, ZPos);
						}
					}
				}
				void CLOAK_CALL_THIS Context2D::StartNewPatch(In UINT Z)
				{
					UINT ZBase = 0;
					if (m_ZPatch.empty() == false) { ZBase = m_ZPatch.top(); }
					m_ZPatch.push(ZBase + Z);
				}
				void CLOAK_CALL_THIS Context2D::EndPatch()
				{
					m_ZPatch.pop();
				}
				void CLOAK_CALL_THIS Context2D::SetScissor(In const API::Rendering::SCALED_SCISSOR_RECT& rct)
				{
					API::Rendering::SCALED_SCISSOR_RECT last;
					if (m_scissor.size() == 0)
					{
						last.TopLeftX = 0;
						last.TopLeftY = 0;
						last.Width = 1;
						last.Height = 1;
					}
					else { last = m_scissor.top(); }
					API::Rendering::SCALED_SCISSOR_RECT next;
					float lR = last.TopLeftX + last.Width;
					float lB = last.TopLeftY + last.Height;
					float nR = rct.TopLeftX + rct.Width;
					float nB = rct.TopLeftY + rct.Height;
					next.TopLeftX = max(rct.TopLeftX, last.TopLeftX);
					next.TopLeftY = max(rct.TopLeftY, last.TopLeftY);
					next.Width = max(0, min(lR, nR) - next.TopLeftX);
					next.Height = max(0, min(lB, nB) - next.TopLeftY);
					m_scissor.push(next);
				}
				void CLOAK_CALL_THIS Context2D::ResetScissor()
				{
					if (m_scissor.empty() == false) { m_scissor.pop(); }
				}

				void CLOAK_CALL_THIS Context2D::Initialize(In UINT textureSlot)
				{
					CLOAK_ASSUME(m_vertexBuffer.size() == 0);
					m_texSlot = textureSlot;
				}
				void CLOAK_CALL_THIS Context2D::Execute()
				{
					if (m_vertexBuffer.size() > 0)
					{
						const size_t size = m_vertexBuffer.size();
						m_parent->SetPrimitiveTopology(API::Rendering::TOPOLOGY_TRIANGLESTRIP);

						//Sorting with RADIX-Sort:
						size_t* const sortHeap = reinterpret_cast<size_t*>(m_pageAlloc->Allocate(sizeof(size_t) * 2 * size));
						size_t* sorted[2] = { &sortHeap[0], &sortHeap[size] };
						uint32_t counting[16];
						for (size_t a = 0; a < size; a++) { sorted[0][a] = a; }

						//First: Sort by image to reduce image swapping and to transition images:
						uint32_t bits = sizeof(void*) << 3;
						for (uint32_t a = 0; a < bits; a += 4)
						{
							memset(counting, 0, sizeof(uint32_t) * ARRAYSIZE(counting));
							for (size_t b = 0; b < size; b++) { counting[(reinterpret_cast<uintptr_t>(m_vertexBuffer[sorted[0][b]].Buffer.Get()) >> a) & 0xF]++; }
							for (size_t b = 1; b < ARRAYSIZE(counting); b++) { counting[b] += counting[b - 1]; }
							for (size_t b = size; b > 0; b--)
							{
								const uint32_t p = (reinterpret_cast<uintptr_t>(m_vertexBuffer[sorted[0][b - 1]].Buffer.Get()) >> a) & 0xF;
								sorted[1][--counting[p]] = sorted[0][b - 1];
							}
							std::swap(sorted[0], sorted[1]);
						}

						//Transition image states:
						API::Rendering::IColorBuffer* limg = nullptr;
						for (size_t a = 0; a < size; a++)
						{
							API::Rendering::IColorBuffer* i = m_vertexBuffer[sorted[0][a]].Buffer.Get();
							if (i != limg) { m_parent->TransitionResource(i, API::Rendering::STATE_PIXEL_SHADER_RESOURCE); }
							limg = i;
						}
						m_parent->FlushResourceBarriers();

						//Next: Sort by Z-Order:
						bits = API::Global::Math::bitScanReverse(max(m_MaxZ, 1)) + 1;
						CLOAK_ASSUME(bits <= sizeof(UINT) << 3);
						for (uint32_t a = 0; a < bits; a += 4)
						{
							memset(counting, 0, sizeof(uint32_t) << 4);
							for (size_t b = 0; b < size; b++) { counting[(m_vertexBuffer[sorted[0][b]].Z >> a) & 0xF]++; }
							for (size_t b = 1; b < ARRAYSIZE(counting); b++) { counting[b] += counting[b - 1]; }
							for (size_t b = size; b > 0; b--)
							{
								const uint32_t p = (m_vertexBuffer[sorted[0][b - 1]].Z >> a) & 0xF;
								sorted[1][--counting[p]] = sorted[0][b - 1];
							}
							std::swap(sorted[0], sorted[1]);
						}

						//Drawing:
						limg = nullptr;
						VERTEX_2D* sortedData = reinterpret_cast<VERTEX_2D*>(m_pageAlloc->Allocate(sizeof(VERTEX_2D) * size));
						CLOAK_ASSUME((reinterpret_cast<uintptr_t>(sortedData) & 0xF) == 0);
						API::PagedQueue<DrawBatch> batch(m_pageAlloc);
						size_t lp = 0;
						if (m_texSlot != INVALID_ROOT_INDEX)
						{
							for (size_t a = 0; a < size; a++)
							{
								const ZBuffer& b = m_vertexBuffer[sorted[0][a]];
								sortedData[a] = b.Vertex;
								if (limg != b.Buffer && lp < a)
								{
									batch.push({ limg, a - lp });
									lp = a;
								}
								limg = b.Buffer.Get();
							}
						}
						if (lp < size) { batch.push({ limg, size - lp }); }
						m_parent->SetDynamicVertexBuffer(0, size, sizeof(VERTEX_2D), sortedData);
						lp = 0;
						while (batch.empty() == false)
						{
							const DrawBatch& b = batch.front();
							if (m_texSlot != INVALID_ROOT_INDEX) { m_parent->SetSRV(m_texSlot, b.Buffer->GetSRV()); }
							m_parent->DrawInstanced(4, static_cast<UINT>(b.Count), 0, static_cast<UINT>(lp));
							lp += b.Count;
							batch.pop();
						}

						//Clearing:
						m_vertexBuffer.clear();
						m_ZPatch.clear();
						m_scissor.clear();
						m_MaxZ = 0;

						m_pageAlloc->Free(sortedData);
						m_pageAlloc->Free(sortHeap);
					}
				}

				CLOAK_CALL Context3D::Context3D(In IContext* parent, API::Global::Memory::PageStackAllocator* pageAlloc) : m_parent(parent), m_pageAlloc(pageAlloc), m_commands(pageAlloc), m_commandsIndexed(pageAlloc)
				{
					m_worker = nullptr;
					m_meshData.IndexBuffer = nullptr;
					for (size_t a = 0; a < ARRAYSIZE(m_meshData.VertexBuffer); a++) { m_meshData.VertexBuffer[a] = nullptr; }
					m_meshData.LocalBuffer = nullptr;
					m_rootIndex.DisplacementMap = INVALID_ROOT_INDEX;
					m_rootIndex.LocalBuffer = INVALID_ROOT_INDEX;
					m_rootIndex.Material = INVALID_ROOT_INDEX;
					m_rootIndex.MetaConstant = INVALID_ROOT_INDEX;
					m_doDraw = 0;
					m_drawMask = 0;
					m_cullNear = true;
					m_passID = 0;
					m_indirectTriangles = 0;
					m_setDraw = false;
					m_gpuCulling = false;
					m_useTessellation = false;
				}

				API::Files::LOD CLOAK_CALL_THIS Context3D::CalculateLOD(In const API::Global::Math::Vector& worldPos, In_opt API::Files::LOD prefered)
				{
					const float d = (worldPos - m_camPos).Length();
					API::Files::LOD r = API::Files::LOD::LOW;
					for (size_t a = 0; a < ARRAYSIZE(LOD_RANGE); a++)
					{
						if (d <= m_camViewDist * LOD_RANGE[a])
						{
							r = static_cast<API::Files::LOD>(a);
							break;
						}
					}
					return static_cast<API::Files::LOD>(max(static_cast<uint32_t>(r), static_cast<uint32_t>(prefered)));
				}
				bool CLOAK_CALL_THIS Context3D::SetMeshData(In API::Rendering::WorldID worldID, In API::Files::IBoundingVolume* bounding, In API::Rendering::IConstantBuffer* localBuffer, In API::Rendering::IVertexBuffer* posBuffer, In_opt API::Rendering::IVertexBuffer* vertexBuffer, In_opt API::Rendering::IIndexBuffer* indexBuffer)
				{
					if (m_camID == worldID)
					{
						m_meshData.VertexBuffer[0] = posBuffer;
						m_meshData.VertexBuffer[1] = vertexBuffer;
						m_meshData.IndexBuffer = indexBuffer;
						m_meshData.LocalBuffer = localBuffer;

						m_doDraw = 0;
						m_drawMask = 0;
						if (m_meshData.VertexBuffer[0] != nullptr && m_meshData.LocalBuffer != nullptr)
						{
							for (size_t a = 0; a < m_camInstanceCount; a++)
							{
								if (bounding->Check(m_camInstance[a].Frustrum, m_passID, m_cullNear))
								{
									m_doDraw++;
									m_drawMask |= static_cast<UINT>(1 << a);
								}
							}
						}

						m_setDraw = false;
						m_gpuCulling = m_allowExecuteIndirect == true && m_meshData.IndexBuffer != nullptr && m_worker != nullptr;
						if (m_gpuCulling == true) 
						{ 
							for (size_t a = 0; a < m_camInstanceCount; a++)
							{
								if ((m_drawMask & static_cast<UINT>(1 << a)) != 0)
								{
									m_drawView[a].AABB = bounding->ToAABB(m_camInstance[a].WorldToProjection);
								}
							}
						}
					}
					else { m_doDraw = 0; }
					return m_doDraw > 0;
				}
				bool CLOAK_CALL_THIS Context3D::SetMeshData(In API::Rendering::WorldID worldID, In API::Files::IBoundingVolume* bounding, In API::Rendering::IConstantBuffer* localBuffer, In API::Files::IMesh* mesh, In API::Files::LOD lod)
				{
					if (m_camID == worldID)
					{
						for (size_t a = 0; a < ARRAYSIZE(m_meshData.VertexBuffer); a++) { m_meshData.VertexBuffer[a] = mesh->GetVertexBuffer(a, lod); }
						m_meshData.IndexBuffer = mesh->GetIndexBuffer(lod);
						m_meshData.LocalBuffer = localBuffer;

						m_doDraw = 0;
						m_drawMask = 0;
						if (m_meshData.VertexBuffer[0] != nullptr && m_meshData.LocalBuffer != nullptr)
						{
							for (size_t a = 0; a < m_camInstanceCount; a++)
							{
								if (bounding->Check(m_camInstance[a].Frustrum, m_passID, m_cullNear))
								{
									m_doDraw++;
									m_drawMask |= static_cast<UINT>(1 << a);
								}
							}
						}

						m_setDraw = false;
						m_gpuCulling = m_allowExecuteIndirect == true && m_meshData.IndexBuffer != nullptr && m_worker != nullptr;
						if (m_gpuCulling == true)
						{
							for (size_t a = 0; a < m_camInstanceCount; a++)
							{
								if ((m_drawMask & static_cast<UINT>(1 << a)) != 0)
								{
									m_drawView[a].AABB = bounding->ToAABB(m_camInstance[a].WorldToProjection);
								}
							}
						}
					}
					else { m_doDraw = 0; }
					return m_doDraw > 0;
				}
				void CLOAK_CALL_THIS Context3D::Draw(In API::Rendering::IDrawable3D* drawing, In API::Files::IMaterial* material, In const API::Files::MeshMaterialInfo& meshInfo)
				{
					const bool matAviable = material != nullptr && material->isLoaded(API::Files::Priority::NORMAL) && (material->GetType() & m_matType) != API::Files::MaterialType::Unknown;
					if (m_doDraw > 0 && (m_drawType == API::Rendering::DRAW_TYPE::DRAW_TYPE_GEOMETRY || matAviable))
					{
						if (meshInfo.SizeStrip > 0) { Draw(API::Rendering::TOPOLOGY_TRIANGLESTRIP, material, meshInfo.Offset, meshInfo.SizeStrip, matAviable); }
						if (meshInfo.SizeList > 0) { Draw(API::Rendering::TOPOLOGY_TRIANGLELIST, material, meshInfo.Offset + meshInfo.SizeStrip, meshInfo.SizeList, matAviable); }
					}
				}
				void CLOAK_CALL_THIS Context3D::Execute()
				{
					//TODO: Copy command/commandIndexed buffers to specific gpu buffer
					m_worker = nullptr;
					m_doDraw = false;
					m_indirectTriangles = 0;
					m_commands.clear();
					m_commandsIndexed.clear();
				}

				void CLOAK_CALL_THIS Context3D::SetWorker(In IRenderWorker* worker, In size_t passID, In API::Files::MaterialType matType, In API::Rendering::DRAW_TYPE drawType, In const API::Global::Math::Vector& camPos, In float camViewDist, In size_t camID, In API::Files::LOD maxLOD)
				{
					m_worker = worker;
					m_passID = passID;
					m_matType = matType;
					m_drawType = drawType;
					m_camID = camID;
					m_camPos = camPos;
					m_camViewDist = camViewDist;
					m_maxLOD = maxLOD;
				}
				void CLOAK_CALL_THIS Context3D::Initialize(In const API::Rendering::Hardware& hardware, In bool allowIndirect, In size_t camCount, In_reads(camCount) const API::Global::Math::Matrix* WorldToProjection, In UINT rootIndexLocalBuffer, In UINT rootIndexMaterial, In UINT rootIndexDisplacement, In UINT rootIndexMetaConstant, In bool useTessellation, In_opt bool cullNear)
				{
					CLOAK_ASSUME(camCount == 1 || hardware.ViewInstanceSupport != API::Rendering::VIEW_INSTANCE_NOT_SUPPORTED);
					for (size_t a = 0; a < camCount; a++)
					{
						m_camInstance[a].Frustrum = WorldToProjection[a];
						m_camInstance[a].WorldToProjection = WorldToProjection[a];
					}
					m_camInstanceCount = camCount;
					m_cullNear = cullNear;
					m_bindingTier = hardware.ResourceBindingTier;
					m_allowExecuteIndirect = allowIndirect;
					m_useTessellation = useTessellation;
					m_rootIndex.DisplacementMap = rootIndexDisplacement;
					m_rootIndex.LocalBuffer = rootIndexLocalBuffer;
					m_rootIndex.Material = rootIndexMaterial;
					m_rootIndex.MetaConstant = rootIndexMetaConstant;
				}

				void CLOAK_CALL_THIS Context3D::Draw(In API::Rendering::PRIMITIVE_TOPOLOGY topology, In API::Files::IMaterial* material, In size_t offset, In size_t count, In bool matAviable)
				{
					const size_t triCount = TriangleCount(topology, count);
					//Executing commands on the gpu causes some overhead. To compensate for that, we only use meshes with 128+ triangles with gpu culling, and use normal drawing for everything else
					if (m_gpuCulling && triCount >= 128 && (topology == API::Rendering::TOPOLOGY_TRIANGLELIST || topology == API::Rendering::TOPOLOGY_3_CONTROL_POINT_PATCHLIST || (topology == API::Rendering::TOPOLOGY_TRIANGLESTRIP && m_useTessellation == false)))
					{
						const bool useTes = m_useTessellation && material->SupportTesslation();
						if (topology == API::Rendering::TOPOLOGY_TRIANGLESTRIP)
						{
							DRAW3D_INDEXED_COMMAND cmd;
							cmd.Draw.InstanceCount = 1;
							cmd.Draw.StartInstanceOffset = static_cast<UINT>(offset);
							cmd.Draw.StartVertexOffset = 0;
							cmd.Draw.IndexCountPerInstance = static_cast<UINT>(count);
							cmd.Draw.StartIndexOffset = 0;

							cmd.LocalBuffer = m_meshData.LocalBuffer->GetGPUAddress().ptr;
							cmd.Material = material->GetSRVMaterial().GPU.ptr;
							cmd.Displacement = useTes ? material->GetSRVDisplacement().GPU.ptr : cmd.Material;
							cmd.VertexBufferPossition = m_meshData.VertexBuffer[0]->GetSRV().GPU.ptr;
							cmd.UseTessellation = useTes ? 1 : 0;

							API::Rendering::INDEX_BUFFER_VIEW ibv = m_meshData.IndexBuffer->GetIBV();
							cmd.IndexBuffer.Address = ibv.Address;
							cmd.IndexBuffer.Format = ibv.Format;
							cmd.IndexBuffer.SizeInBytes = ibv.SizeInBytes;

							for (size_t a = 0; a < min(ARRAYSIZE(m_meshData.VertexBuffer), ARRAYSIZE(cmd.VertexBuffer)); a++)
							{
								API::Rendering::VERTEX_BUFFER_VIEW vbv = m_meshData.VertexBuffer[a]->GetVBV();
								cmd.VertexBuffer[a].Address = vbv.Address;
								cmd.VertexBuffer[a].SizeInBytes = vbv.SizeInBytes;
								cmd.VertexBuffer[a].StrideInBytes = vbv.StrideInBytes;
							}

							//TODO: Init AABB and Depth
							m_commandsIndexed.push(cmd);
						}
						else
						{

							DRAW3D_COMMAND cmd;
							cmd.Draw.InstanceCount = 1;
							cmd.Draw.StartInstanceOffset = static_cast<UINT>(offset);
							cmd.Draw.StartVertexOffset = 0;
							cmd.Draw.VertexCountPerInstance = static_cast<UINT>(count);

							cmd.LocalBuffer = m_meshData.LocalBuffer->GetGPUAddress().ptr;
							cmd.Material = material->GetSRVMaterial().GPU.ptr;
							cmd.Displacement = useTes ? material->GetSRVDisplacement().GPU.ptr : cmd.Material;
							cmd.VertexBufferPossition = m_meshData.VertexBuffer[0]->GetSRV().GPU.ptr;
							cmd.UseTessellation = useTes ? 1 : 0;

							API::Rendering::INDEX_BUFFER_VIEW ibv = m_meshData.IndexBuffer->GetIBV();
							cmd.IndexBuffer.Address = ibv.Address;
							cmd.IndexBuffer.Format = ibv.Format;
							cmd.IndexBuffer.SizeInBytes = ibv.SizeInBytes;

							for (size_t a = 0; a < min(ARRAYSIZE(m_meshData.VertexBuffer), ARRAYSIZE(cmd.VertexBuffer)); a++)
							{
								API::Rendering::VERTEX_BUFFER_VIEW vbv = m_meshData.VertexBuffer[a]->GetVBV();
								cmd.VertexBuffer[a].Address = vbv.Address;
								cmd.VertexBuffer[a].SizeInBytes = vbv.SizeInBytes;
								cmd.VertexBuffer[a].StrideInBytes = vbv.StrideInBytes;
							}

							//TODO: Init AABB and Depth
							m_commands.push(cmd);
						}
						m_indirectTriangles += triCount;
					}
					else
					{
						if (m_setDraw == false)
						{
							m_setDraw = true;
							m_parent->SetIndexBuffer(m_meshData.IndexBuffer);
							if (m_doDraw > 1) { m_parent->SetViewInstanceMask(m_drawMask); }
							if (m_drawType == API::Rendering::DRAW_TYPE::DRAW_TYPE_GEOMETRY || (m_drawType == API::Rendering::DRAW_TYPE::DRAW_TYPE_GEOMETRY_TESSELLATION && m_useTessellation == false))
							{
								m_parent->SetVertexBuffer(m_meshData.VertexBuffer[0]);
							}
							else
							{
								m_parent->SetVertexBuffer(ARRAYSIZE(m_meshData.VertexBuffer), m_meshData.VertexBuffer);
							}
							if (m_rootIndex.LocalBuffer != INVALID_ROOT_INDEX) { m_parent->SetConstantBuffer(m_rootIndex.LocalBuffer, m_meshData.LocalBuffer); }
						}
						m_parent->SetPrimitiveTopology(topology);
						if (matAviable)
						{
							if (m_rootIndex.Material != INVALID_ROOT_INDEX) { m_parent->SetSRV(m_rootIndex.Material, material->GetSRVMaterial()); }
							if (m_useTessellation)
							{
								const bool useTes = material->SupportTesslation();
								if (m_rootIndex.DisplacementMap != INVALID_ROOT_INDEX)
								{
									if (useTes) { m_parent->SetSRV(m_rootIndex.DisplacementMap, material->GetSRVDisplacement()); }
									else if (m_bindingTier == API::Rendering::RESOURCE_BINDING_TIER_1) { m_parent->SetSRV(m_rootIndex.DisplacementMap, material->GetSRVMaterial()); }
								}
								if (m_rootIndex.MetaConstant != INVALID_ROOT_INDEX)
								{
									UINT c = 0;
									if (useTes) { c |= 0x1; }
									m_parent->SetConstants(m_rootIndex.MetaConstant, c);
								}
							}
						}
						if (m_meshData.IndexBuffer != nullptr) { m_parent->DrawIndexed(static_cast<UINT>(count), static_cast<UINT>(offset)); }
						else { m_parent->Draw(static_cast<UINT>(count), static_cast<UINT>(offset)); }
					}
				}
			}
		}
	}
}