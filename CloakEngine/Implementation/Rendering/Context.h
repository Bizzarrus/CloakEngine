#pragma once
#ifndef CE_IMPL_RENDEIRNG_CONTEXT_H
#define CE_IMPL_RENDEIRNG_CONTEXT_H

#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Rendering/Context.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/RootSignature.h"
#include "Implementation/Rendering/PipelineState.h"
#include "Implementation/Rendering/Resource.h"
#include "Implementation/Rendering/Queue.h"
#include "Implementation/Rendering/VertexData.h"
#include "Implementation/Rendering/RenderWorker.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Context_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{6126A294-EFD2-45D1-9E75-0F957E4D0A59}") ICopyContext : public virtual API::Rendering::ICopyContext{
					public:
						virtual void CLOAK_CALL_THIS InsertAliasBarrier(In API::Rendering::IResource* before, In API::Rendering::IResource* after, In_opt bool flush = false) = 0;
						virtual void CLOAK_CALL_THIS FlushResourceBarriers() = 0;
						virtual uint64_t CLOAK_CALL_THIS GetLastFence() const = 0;
						virtual ISingleQueue* CLOAK_CALL_THIS GetQueue() const = 0;
						virtual void CLOAK_CALL_THIS WaitForAdapter(In IAdapter* queue, In uint64_t value) = 0;
						virtual void CLOAK_CALL_THIS Reset() = 0;
						virtual void CLOAK_CALL_THIS CleanAndDiscard(In uint64_t fence) = 0;
						virtual void CLOAK_CALL_THIS BlockTransitions(In bool block) = 0;
				};
				RENDERING_INTERFACE CLOAK_UUID("{08733669-AD87-457A-95A3-84F96CC20638}") IContext : public virtual API::Rendering::IContext, public virtual ICopyContext {
					public:
						virtual void CLOAK_CALL_THIS SetDescriptorTable(In UINT rootIndex, In const API::Rendering::GPU_DESCRIPTOR_HANDLE& gpuAddress) = 0;
						virtual void CLOAK_CALL_THIS SetPipelineState(In API::Rendering::IPipelineState* pso, In_opt bool setRoot = true) = 0;
						virtual void CLOAK_CALL_THIS SetRootSignature(In IRootSignature* sig) = 0;
						virtual void CLOAK_CALL_THIS SetDynamicDescriptors(In UINT rootIndex, In UINT offest, In_reads(count) const API::Rendering::CPU_DESCRIPTOR_HANDLE* handles, In_opt UINT count = 1) = 0;
						virtual void CLOAK_CALL_THIS SetWorker(In IRenderWorker* worker, In size_t passID, In API::Files::MaterialType matType, In API::Rendering::DRAW_TYPE drawType, In const API::Global::Math::Vector& camPos, In float camViewDist, In size_t camID, In API::Files::LOD maxLOD) = 0;

						virtual API::Rendering::IPipelineState* CLOAK_CALL_THIS GetCurrentPipelineState() const = 0;
						virtual IRootSignature* CLOAK_CALL_THIS GetCurrentRootSignature() const = 0;
				};

				class Context2D : public virtual API::Rendering::IContext2D {
					public:
						CLOAK_CALL Context2D(In IContext* parent, API::Global::Memory::PageStackAllocator* pageAlloc);

						void CLOAK_CALL_THIS Draw(In API::Rendering::IDrawable2D* drawing, In API::Files::ITexture* img, In const API::Rendering::VERTEX_2D& vertex, In UINT Z, In_opt size_t bufferNum = 0) override;
						void CLOAK_CALL_THIS Draw(In API::Rendering::IDrawable2D* drawing, In API::Rendering::IColorBuffer* img, In const API::Rendering::VERTEX_2D& vertex, In UINT Z) override;
						void CLOAK_CALL_THIS StartNewPatch(In UINT Z) override;
						void CLOAK_CALL_THIS EndPatch() override;
						void CLOAK_CALL_THIS SetScissor(In const API::Rendering::SCALED_SCISSOR_RECT& rct) override;
						void CLOAK_CALL_THIS ResetScissor() override;
						void CLOAK_CALL_THIS Execute() override;

						void CLOAK_CALL_THIS Initialize(In UINT textureSlot);
					protected:
						struct ZBuffer {
							CE::RefPointer<API::Rendering::IColorBuffer> Buffer;
							UINT Z;
							VERTEX_2D Vertex;
						};

						IContext* const m_parent;
						API::Global::Memory::PageStackAllocator* const m_pageAlloc;
						API::List<ZBuffer> m_vertexBuffer;
						API::PagedStack<UINT> m_ZPatch;
						API::PagedStack<API::Rendering::SCALED_SCISSOR_RECT> m_scissor;
						UINT m_MaxZ;
						UINT m_texSlot;
				};
				class Context3D : public virtual API::Rendering::IContext3D {
					public:
						CLOAK_CALL Context3D(In IContext* parent, API::Global::Memory::PageStackAllocator* pageAlloc);

						API::Files::LOD CLOAK_CALL_THIS CalculateLOD(In const API::Global::Math::Vector& worldPos, In_opt API::Files::LOD prefered = API::Files::LOD::ULTRA) override;
						bool CLOAK_CALL_THIS SetMeshData(In API::Rendering::WorldID worldID, In API::Files::IBoundingVolume* bounding, In API::Rendering::IConstantBuffer* localBuffer, In API::Rendering::IVertexBuffer* posBuffer, In_opt API::Rendering::IVertexBuffer* vertexBuffer = nullptr, In_opt API::Rendering::IIndexBuffer* indexBuffer = nullptr) override;
						bool CLOAK_CALL_THIS SetMeshData(In API::Rendering::WorldID worldID, In API::Files::IBoundingVolume* bounding, In API::Rendering::IConstantBuffer* localBuffer, In API::Files::IMesh* mesh, In API::Files::LOD lod) override;
						void CLOAK_CALL_THIS Draw(In API::Rendering::IDrawable3D* drawing, In API::Files::IMaterial* material, In const API::Files::MeshMaterialInfo& meshInfo) override;
						void CLOAK_CALL_THIS Execute() override;

						void CLOAK_CALL_THIS SetWorker(In IRenderWorker* worker, In size_t passID, In API::Files::MaterialType matType, In API::Rendering::DRAW_TYPE drawType, In const API::Global::Math::Vector& camPos, In float camViewDist, In size_t camID, In API::Files::LOD maxLOD);
						void CLOAK_CALL_THIS Initialize(In const API::Rendering::Hardware& hardware, In bool allowIndirect, In size_t camCount, In_reads(camCount) const API::Global::Math::Matrix* WorldToProjection, In UINT rootIndexLocalBuffer, In UINT rootIndexMaterial, In UINT rootIndexDisplacement, In UINT rootIndexMetaConstant, In bool useTessellation, In_opt bool cullNearPlane = true);
					protected:
						void CLOAK_CALL_THIS Draw(In API::Rendering::PRIMITIVE_TOPOLOGY topology, In API::Files::IMaterial* material, In size_t offset, In size_t count, In bool matAviable);

						IContext * const m_parent;
						API::Global::Memory::PageStackAllocator* const m_pageAlloc;

						//Worker Settings:
						IRenderWorker* m_worker;
						size_t m_passID;
						API::Files::MaterialType m_matType;
						API::Rendering::DRAW_TYPE m_drawType;

						//Initial Settings:
						struct {
							API::Global::Math::OctantFrustum Frustrum;
							API::Global::Math::Matrix WorldToProjection;
						} m_camInstance[API::Rendering::MAX_VIEW_INSTANCES];
						size_t m_camInstanceCount;
						API::Global::Math::Vector m_camPos;
						float m_camViewDist;
						size_t m_camID;
						API::Files::LOD m_maxLOD;
						bool m_cullNear;
						bool m_useTessellation;
						API::Rendering::RESOURCE_BINDING_TIER m_bindingTier;
						bool m_allowExecuteIndirect;
						struct {
							UINT LocalBuffer;
							UINT Material;
							UINT DisplacementMap;
							UINT MetaConstant;
						} m_rootIndex;

						//Calculated Data:
						UINT m_doDraw;
						UINT m_drawMask;
						bool m_setDraw;
						bool m_gpuCulling;
						struct {
							API::Global::Math::AABB AABB;
						} m_drawView[API::Rendering::MAX_VIEW_INSTANCES];

						//Per Entity Data:
						struct {
							API::Rendering::IVertexBuffer* VertexBuffer[2];
							API::Rendering::IIndexBuffer* IndexBuffer;
							API::Rendering::IConstantBuffer* LocalBuffer;
						} m_meshData;

						//ExecuteIndirect Data:
						size_t m_indirectTriangles;
						API::PagedStack<DRAW3D_COMMAND> m_commands;
						API::PagedStack<DRAW3D_INDEXED_COMMAND> m_commandsIndexed;
				};
			}
		}
	}
}

#endif