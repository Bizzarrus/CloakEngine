#pragma once
#ifndef CE_API_RENDERING_CONTEXT_H
#define CE_API_RENDERING_CONTEXT_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Helper/Color.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Rendering/Resource.h"
#include "CloakEngine/Rendering/BasicBuffer.h"
#include "CloakEngine/Rendering/ColorBuffer.h"
#include "CloakEngine/Rendering/DepthBuffer.h"
#include "CloakEngine/Rendering/StructuredBuffer.h"
#include "CloakEngine/Rendering/Defines.h"
#include "CloakEngine/Rendering/PipelineState.h"
#include "CloakEngine/Rendering/DataTypes.h"
#include "CloakEngine/Rendering/Manager.h"
#include "CloakEngine/Rendering/Query.h"
#include "CloakEngine/Files/Image.h"
#include "CloakEngine/Files/Mesh.h"

#define INVALID_ROOT_INDEX static_cast<UINT>(-1)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace Context_v1 {
				enum class Vertex2DFlags { NONE = 0x0, MULTICOLOR = 0x1, GRAYSCALE = 0x2 };
				DEFINE_ENUM_FLAG_OPERATORS(Vertex2DFlags);
				struct VERTEX_2D {
					API::Rendering::FLOAT2 Position;
					API::Rendering::FLOAT2 Size;
					API::Rendering::FLOAT2 TexTopLeft;
					API::Rendering::FLOAT2 TexBottomRight;
					struct {
						float Sinus;
						float Cosinus;
					} Rotation;
					API::Helper::Color::RGBA Color[3];
					Vertex2DFlags Flags = Vertex2DFlags::NONE;
				};
				struct PassData3D {
					bool CullNearPlane;
					struct {
						UINT LocalBuffer;
						UINT Material;
						UINT DisplacementMap;
						UINT UseTessellationConstant;
					} RootIndex;
				};
				struct RENDER_PASS_RTV {
					CE::RefPointer<API::Rendering::IColorBuffer> Buffer;
					RENDER_PASS_ACCESS BeginAccess;
					RENDER_PASS_ACCESS EndAccess;
				};
				struct RENDER_PASS_DSV {
					CE::RefPointer<API::Rendering::IDepthBuffer> Buffer;
					struct {
						RENDER_PASS_ACCESS BeginAccess;
						RENDER_PASS_ACCESS EndAccess;
					} Depth;
					struct {
						RENDER_PASS_ACCESS BeginAccess;
						RENDER_PASS_ACCESS EndAccess;
					} Stencil;
					DSV_ACCESS DSVAccess;
				};
				typedef size_t WorldID;
				constexpr WorldID INVALID_WORLD_ID = 0;

				class IDrawable2D;
				class IDrawable3D;
				CLOAK_INTERFACE IContext2D {
					public:
						virtual void CLOAK_CALL_THIS Draw(In IDrawable2D* drawing, In Files::ITexture* img, In const VERTEX_2D& vertex, In UINT Z, In_opt size_t bufferNum = 0) = 0;
						virtual void CLOAK_CALL_THIS Draw(In IDrawable2D* drawing, In IColorBuffer* img, In const VERTEX_2D& vertex, In UINT Z) = 0;
						virtual void CLOAK_CALL_THIS StartNewPatch(In UINT Z) = 0;
						virtual void CLOAK_CALL_THIS EndPatch() = 0;
						virtual void CLOAK_CALL_THIS SetScissor(In const API::Rendering::SCALED_SCISSOR_RECT& rct) = 0;
						virtual void CLOAK_CALL_THIS ResetScissor() = 0;
						virtual void CLOAK_CALL_THIS Execute() = 0;
				};
				CLOAK_INTERFACE IContext3D {
					public:
						virtual API::Files::LOD CLOAK_CALL_THIS CalculateLOD(In const API::Global::Math::Vector& worldPos, In_opt API::Files::LOD prefered = API::Files::LOD::ULTRA) = 0;
						virtual bool CLOAK_CALL_THIS SetMeshData(In API::Rendering::WorldID worldID, In Files::IBoundingVolume* bounding, In IConstantBuffer* localBuffer, In IVertexBuffer* posBuffer, In_opt IVertexBuffer* vertexBuffer = nullptr, In_opt IIndexBuffer* indexBuffer = nullptr) = 0;
						virtual bool CLOAK_CALL_THIS SetMeshData(In API::Rendering::WorldID worldID, In Files::IBoundingVolume* bounding, In IConstantBuffer* localBuffer, In Files::IMesh* mesh, In Files::LOD lod) = 0;
						virtual void CLOAK_CALL_THIS Draw(In IDrawable3D* drawing, In Files::IMaterial* material, In const Files::MeshMaterialInfo& meshInfo) = 0;
						virtual void CLOAK_CALL_THIS Execute() = 0;
				};
				CLOAK_INTERFACE_ID("{94F72994-06F4-4EB3-B289-26FD555826A5}") ICopyContext : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS TransitionResource(In API::Rendering::IResource* rsc, In RESOURCE_STATE state, In_opt bool flush = false) = 0;
						virtual void CLOAK_CALL_THIS InsertUAVBarrier(In API::Rendering::IResource* resource, In_opt bool flush = false) = 0;
						virtual void CLOAK_CALL_THIS BeginTransitionResource(In API::Rendering::IResource* rsc, In RESOURCE_STATE state, In_opt bool flush = false) = 0;
						virtual void CLOAK_CALL_THIS WaitForAdapter(In API::Rendering::CONTEXT_TYPE context, In uint32_t nodeID, In uint64_t fenceState) = 0;
						virtual uint64_t CLOAK_CALL_THIS CloseAndExecute(In_opt bool wait = false, In_opt bool executeDirect = true) = 0;

						virtual void CLOAK_CALL_THIS CopyBuffer(Inout IResource* dst, In IResource* src) = 0;
						virtual void CLOAK_CALL_THIS CopyBufferRegion(Inout IResource* dst, In size_t dstOffset, In IResource* src, In size_t srcOffset, size_t In numBytes) = 0;
						virtual void CLOAK_CALL_THIS CopySubresource(Inout IResource* dst, In UINT dstSubIndex, In IResource* src, In UINT srcSubIndex) = 0;
						virtual void CLOAK_CALL_THIS CopyCounter(Inout IResource* dst, In size_t dstOffset, In IStructuredBuffer* src) = 0;
						virtual void CLOAK_CALL_THIS ResetCounter(Inout IStructuredBuffer* buffer, In_opt uint32_t val = 0) = 0;
						virtual void CLOAK_CALL_THIS WriteBuffer(Inout IResource* dst, In size_t dstOffset, In_reads_bytes(numBytes) const void* data, In size_t numBytes) = 0;
						virtual void CLOAK_CALL_THIS WriteBuffer(Inout IResource* dst, In UINT firstSubresource, In UINT numSubresources, In_reads(numSubresources) const API::Rendering::SUBRESOURCE_DATA* srds) = 0;
						virtual void CLOAK_CALL_THIS FillBuffer(Inout IResource* dst, In size_t dstOffset, In const DWParam& val, In size_t numBytes) = 0;
						virtual API::Rendering::ReadBack CLOAK_CALL_THIS ReadBack(In IPixelBuffer* src, In_opt UINT subresource = 0) = 0;
						virtual API::Rendering::ReadBack CLOAK_CALL_THIS ReadBack(In IGraphicBuffer* src) = 0;

						virtual void CLOAK_CALL_THIS BeginQuery(In IQuery* query) = 0;
						virtual void CLOAK_CALL_THIS EndQuery(In IQuery* query) = 0;
				};
				CLOAK_INTERFACE_ID("{CA0D3725-E701-4E72-AC41-FD734786E437}") IContext : public virtual ICopyContext{
					public:
						virtual void CLOAK_CALL_THIS ClearUAV(In IStructuredBuffer* target) = 0;
						virtual void CLOAK_CALL_THIS ClearUAV(In IByteAddressBuffer* target) = 0;
						virtual void CLOAK_CALL_THIS ClearUAV(In IColorBuffer* target) = 0;
						virtual void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In UINT numConstants, In_reads(numConstants) const DWParam* constants) = 0;
						virtual void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In const DWParam& X) = 0;
						virtual void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In const DWParam& X, In const DWParam& Y) = 0;
						virtual void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In const DWParam& X, In const DWParam& Y, In const DWParam& Z) = 0;
						virtual void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In const DWParam& X, In const DWParam& Y, In const DWParam& Z, In const DWParam& W) = 0;
						virtual void CLOAK_CALL_THIS SetConstantsByPos(In UINT rootIndex, In const DWParam& X, In UINT pos) = 0;
						virtual void CLOAK_CALL_THIS SetConstantBuffer(In UINT rootIndex, In IGraphicBuffer* cbv) = 0;
						virtual void CLOAK_CALL_THIS SetDynamicCBV(In UINT rootIndex, In size_t bufferSize, In_reads_bytes(bufferSize) const void* data) = 0;
						virtual void CLOAK_CALL_THIS SetDynamicSRV(In UINT rootIndex, In size_t size, In_reads_bytes(size) const void* buffer) = 0;
						virtual void CLOAK_CALL_THIS SetBufferSRV(In UINT rootIndex, In IGraphicBuffer* srv) = 0;
						virtual void CLOAK_CALL_THIS SetBufferUAV(In UINT rootIndex, In IGraphicBuffer* uav) = 0;
						virtual void CLOAK_CALL_THIS SetSRV(In UINT rootIndex, In const ResourceView& srv) = 0;
						virtual void CLOAK_CALL_THIS SetUAV(In UINT rootIndex, In const ResourceView& uav) = 0;
						virtual void CLOAK_CALL_THIS SetCBV(In UINT rootIndex, In const ResourceView& cbv) = 0;
						virtual void CLOAK_CALL_THIS SetDescriptorTable(In UINT rootIndex, In UINT offset, In_reads(count) const ResourceView* views, In_opt UINT count = 1) = 0;
						virtual void CLOAK_CALL_THIS SetPipelineState(In IPipelineState* pso) = 0;
						virtual void CLOAK_CALL_THIS SetPredication(In IQuery* query, In bool value) = 0;
						virtual void CLOAK_CALL_THIS ResetPredication() = 0;
						virtual void CLOAK_CALL_THIS ResolveMultiSample(In IColorBuffer* dst, In IColorBuffer* src) = 0;
						virtual void CLOAK_CALL_THIS ResolveMultiSample(In IColorBuffer* dst, In UINT dstSubResource, In IColorBuffer* src, In UINT srcSubResource) = 0;
						virtual void CLOAK_CALL_THIS ResolveMultiSample(In IColorBuffer* dst, In IColorBuffer* src, In UINT dstX, In UINT dstY) = 0;
						virtual void CLOAK_CALL_THIS ResolveMultiSample(In IColorBuffer* dst, In UINT dstSubResource, In IColorBuffer* src, In UINT srcSubResource, In UINT dstX, In UINT dstY) = 0;
						virtual void CLOAK_CALL_THIS ResolveMultiSample(In IColorBuffer* dst, In IColorBuffer* src, In UINT dstX, In UINT dstY, In UINT srcX, In UINT srcY, In UINT W, In UINT H) = 0;
						virtual void CLOAK_CALL_THIS ResolveMultiSample(In IColorBuffer* dst, In UINT dstSubResource, In IColorBuffer* src, In UINT srcSubResource, In UINT dstX, In UINT dstY, In UINT srcX, In UINT srcY, In UINT W, In UINT H) = 0;
						virtual void CLOAK_CALL_THIS SetDepthBounds(In float min, In float max) = 0;

						virtual void CLOAK_CALL_THIS ClearColor(In IColorBuffer* target) = 0;
						virtual void CLOAK_CALL_THIS ClearDepth(In IDepthBuffer* target) = 0;
						virtual void CLOAK_CALL_THIS ClearStencil(In IDepthBuffer* target) = 0;
						virtual void CLOAK_CALL_THIS ClearDepthAndStencil(In IDepthBuffer* target) = 0;
						virtual void CLOAK_CALL_THIS SetIndexBuffer(In IIndexBuffer* buffer) = 0;
						virtual void CLOAK_CALL_THIS SetVertexBuffer(In IVertexBuffer* buffer, In_opt UINT slot = 0) = 0;
						virtual void CLOAK_CALL_THIS SetVertexBuffer(In UINT count, In_reads(count) IVertexBuffer** buffer, In_opt UINT slot = 0) = 0;
						virtual void CLOAK_CALL_THIS SetDynamicVertexBuffer(In UINT slot, In size_t numVertices, In size_t vertexStride, In_reads_bytes(numVertices*vertexStride) const void* data) = 0;
						virtual void CLOAK_CALL_THIS SetDynamicIndexBuffer(In size_t count, In_reads(count) const uint16_t* data) = 0;
						virtual void CLOAK_CALL_THIS SetDepthStencilRefValue(In UINT val) = 0;
						virtual void CLOAK_CALL_THIS SetBlendFactor(In const API::Helper::Color::RGBA& blendColor) = 0;
						virtual void CLOAK_CALL_THIS SetViewInstanceMask(In UINT mask) = 0;
						virtual void CLOAK_CALL_THIS Draw(In UINT count, In_opt UINT offset = 0) = 0;
						virtual void CLOAK_CALL_THIS DrawIndexed(In UINT count, In_opt UINT indexOffset = 0, In_opt INT vertexOffset = 0) = 0;
						virtual void CLOAK_CALL_THIS DrawInstanced(In UINT countPerInstance, In UINT instanceCount, In_opt UINT vertexOffset = 0, In_opt UINT instanceOffset = 0) = 0;
						virtual void CLOAK_CALL_THIS DrawIndexedInstanced(In UINT countPerInstance, In UINT instanceCount, In_opt UINT indexOffset = 0, In_opt INT vertexOffset = 0, In_opt UINT instanceOffset = 0) = 0;
						virtual void CLOAK_CALL_THIS ExecuteIndirect(In IStructuredBuffer* argBuffer, In size_t argOffset = 0) = 0;
						virtual void CLOAK_CALL_THIS SetRenderTargets(In UINT numTargets, In_reads(numTargets) API::Rendering::IColorBuffer** RTVs, In_opt API::Rendering::IDepthBuffer* DSV = nullptr, In_opt DSV_ACCESS access = DSV_ACCESS::READ_ONLY_STENCIL) = 0;
						virtual void CLOAK_CALL_THIS BeginRenderPass(In UINT numTargets, In_reads(numTargets) const RENDER_PASS_RTV* RTVs, In const RENDER_PASS_DSV& DSV, In_opt RENDER_PASS_FLAG flags = RENDER_PASS_FLAG_NONE) = 0;
						virtual void CLOAK_CALL_THIS BeginRenderPass(In UINT numTargets, In_reads(numTargets) const RENDER_PASS_RTV* RTVs, In_opt RENDER_PASS_FLAG flags = RENDER_PASS_FLAG_NONE) = 0;
						virtual void CLOAK_CALL_THIS BeginRenderPass(In const RENDER_PASS_DSV& DSV, In_opt RENDER_PASS_FLAG flags = RENDER_PASS_FLAG_NONE) = 0;
						virtual void CLOAK_CALL_THIS EndRenderPass() = 0;
						virtual void CLOAK_CALL_THIS SetViewport(In const VIEWPORT& vp) = 0;
						virtual void CLOAK_CALL_THIS SetViewport(In size_t count, In_reads(count) const VIEWPORT* vp) = 0;
						virtual void CLOAK_CALL_THIS SetViewport(In FLOAT x, In FLOAT y, In FLOAT width, In FLOAT height, In_opt FLOAT minDepth = 0, In_opt FLOAT maxDepth = 1) = 0;
						virtual void CLOAK_CALL_THIS SetScissor(In const SCISSOR_RECT& scissor) = 0;
						virtual void CLOAK_CALL_THIS SetScissor(In size_t count, In_reads(count) const SCISSOR_RECT* scissor) = 0;
						virtual void CLOAK_CALL_THIS SetScissor(In LONG x, In LONG y, In LONG width, In LONG height) = 0;
						virtual void CLOAK_CALL_THIS SetViewportAndScissor(In const VIEWPORT& vp) = 0;
						virtual void CLOAK_CALL_THIS SetViewportAndScissor(In const SCISSOR_RECT& vp) = 0;
						virtual void CLOAK_CALL_THIS SetViewportAndScissor(In const VIEWPORT& vp, In const SCISSOR_RECT& scissor) = 0;
						virtual void CLOAK_CALL_THIS SetViewportAndScissor(In size_t count, In_reads(count) const VIEWPORT* vp) = 0;
						virtual void CLOAK_CALL_THIS SetViewportAndScissor(In size_t count, In_reads(count) const SCISSOR_RECT* vp) = 0;
						virtual void CLOAK_CALL_THIS SetViewportAndScissor(In size_t countVP, In_reads(countVP) const VIEWPORT* vp, In size_t countSC, In_reads(countSC) const SCISSOR_RECT* scissor) = 0;
						virtual void CLOAK_CALL_THIS SetViewportAndScissor(In FLOAT x, In FLOAT y, In FLOAT width, In FLOAT height, In_opt FLOAT minDepth = 0, In_opt FLOAT maxDepth = 1) = 0;
						virtual void CLOAK_CALL_THIS SetPrimitiveTopology(In PRIMITIVE_TOPOLOGY topology) = 0;
						virtual API::Rendering::IContext2D* CLOAK_CALL_THIS Enable2D(In UINT textureSlot) = 0;
						virtual API::Rendering::IContext3D* CLOAK_CALL_THIS Enable3D(In const API::Global::Math::Matrix& WorldToProjection, In_opt bool cullNear = true, In_opt bool useIndirectBuffer = false, In_opt UINT rootIndexMaterial = INVALID_ROOT_INDEX, In_opt UINT rootIndexLocalBuffer = INVALID_ROOT_INDEX, In_opt UINT rootIndexDisplacement = INVALID_ROOT_INDEX, In_opt UINT rootIndexMetaConstant = INVALID_ROOT_INDEX) = 0;
						virtual API::Rendering::IContext3D* CLOAK_CALL_THIS Enable3D(In_range(0, API::Rendering::MAX_VIEW_INSTANCES) size_t camCount, In_reads(camCount) const API::Global::Math::Matrix* WorldToProjection, In_opt bool cullNear = true, In_opt bool useIndirectBuffer = false, In_opt UINT rootIndexMaterial = INVALID_ROOT_INDEX, In_opt UINT rootIndexLocalBuffer = INVALID_ROOT_INDEX, In_opt UINT rootIndexDisplacement = INVALID_ROOT_INDEX, In_opt UINT rootIndexMetaConstant = INVALID_ROOT_INDEX) = 0;
						virtual void CLOAK_CALL_THIS SetShadingRate(In API::Rendering::SHADING_RATE xAxis, In API::Rendering::SHADING_RATE yAxis) = 0;

						virtual void CLOAK_CALL_THIS Dispatch(In_opt size_t groupCountX = 1, In_opt size_t groupCountY = 1, In_opt size_t groupCountZ = 1) = 0;
						virtual void CLOAK_CALL_THIS Dispatch1D(In size_t threadCount, In_opt size_t size = 64) = 0;
						virtual void CLOAK_CALL_THIS Dispatch2D(In size_t threadCountX, In size_t threadCountY, In_opt size_t sizeX = 8, In_opt size_t sizeY = 8) = 0;
						virtual void CLOAK_CALL_THIS Dispatch3D(In size_t threadCountX, In size_t threadCountY, In size_t threadCountZ, In size_t sizeX, In size_t sizeY, In size_t sizeZ) = 0;
				};
				CLOAK_INTERFACE IDrawable2D : public virtual API::Helper::SavePtr_v1::IBasicPtr {
					public:
						virtual void CLOAK_CALL_THIS Draw(In IContext2D* context) = 0;
						virtual bool CLOAK_CALL_THIS SetUpdateFlag() = 0;
						virtual void CLOAK_CALL_THIS UpdateDrawInformations(In API::Global::Time time, In ICopyContext* context, In IManager* manager) = 0;
				};
				CLOAK_INTERFACE IDrawable3D : public virtual API::Helper::SavePtr_v1::IBasicPtr{
					public:
						virtual void CLOAK_CALL_THIS Draw(In IContext3D* context, In_opt API::Files::LOD lod = API::Files::LOD::ULTRA) = 0;
				};
			}
		}
	}
}

#endif