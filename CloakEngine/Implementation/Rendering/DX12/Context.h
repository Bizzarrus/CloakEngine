#pragma once
#ifndef CE_IMPL_RENDERING_DX12_CONTEXT_H
#define CE_IMPL_RENDERING_DX12_CONTEXT_H

#include "Implementation/Rendering/Context.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Rendering/Manager.h"
#include "Implementation/Rendering/ColorBuffer.h"
#include "Implementation/Rendering/VertexData.h"
#include "Implementation/Rendering/Allocator.h"
#include "Implementation/Rendering/DX12/Allocator.h"
#include "Implementation/Rendering/DX12/RootSignature.h"
#include "Implementation/Rendering/DX12/Queue.h"
#include "Implementation/Rendering/DX12/Query.h"

#include "Engine/LinkedList.h"

#include <d3d12.h>
#include <unordered_map>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace Context_v1 {
					class CLOAK_UUID("{A0EC4DBB-DF0C-41AF-839E-514624C6411A}") CopyContext : public virtual Impl::Rendering::ICopyContext, public Impl::Helper::SavePtr_v1::SavePtr {
						friend class Queue_v1::SingleQueue;
						public:
							virtual CLOAK_CALL ~CopyContext();
							void CLOAK_CALL_THIS CopyResource(In API::Rendering::IResource* dst, In ID3D12Resource* src);
							void CLOAK_CALL_THIS CopyTextureRegion(In ID3D12Resource* dst, In ID3D12Resource* src, In_reads(numSubresource) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts, In UINT firstSubresource, In UINT numSubresource);

							void CLOAK_CALL_THIS InsertAliasBarrier(In API::Rendering::IResource* before, In API::Rendering::IResource* after, In_opt bool flush = false) override;
							void CLOAK_CALL_THIS FlushResourceBarriers() override;
							uint64_t CLOAK_CALL_THIS GetLastFence() const override;
							ISingleQueue* CLOAK_CALL_THIS GetQueue() const override;
							void CLOAK_CALL_THIS WaitForAdapter(In IAdapter* queue, In uint64_t value) override;
							virtual void CLOAK_CALL_THIS Reset() override;
							virtual void CLOAK_CALL_THIS CleanAndDiscard(In uint64_t fence) override;
							void CLOAK_CALL_THIS BlockTransitions(In bool block) override;

							void CLOAK_CALL_THIS TransitionResource(In API::Rendering::IResource* rsc, In API::Rendering::RESOURCE_STATE state, In_opt bool flush = false) override;
							void CLOAK_CALL_THIS InsertUAVBarrier(In API::Rendering::IResource* resource, In_opt bool flush = false) override;
							void CLOAK_CALL_THIS BeginTransitionResource(In API::Rendering::IResource* rsc, In API::Rendering::RESOURCE_STATE state, In_opt bool flush = false) override;
							void CLOAK_CALL_THIS WaitForAdapter(In API::Rendering::CONTEXT_TYPE context, In uint32_t nodeID, In uint64_t fenceState) override;
							uint64_t CLOAK_CALL_THIS CloseAndExecute(In_opt bool wait = false, In_opt bool executeDirect = true) override;

							void CLOAK_CALL_THIS CopyBuffer(Inout API::Rendering::IResource* dst, In API::Rendering::IResource* src) override;
							void CLOAK_CALL_THIS CopyBufferRegion(Inout API::Rendering::IResource* dst, In size_t dstOffset, In API::Rendering::IResource* src, In size_t srcOffset, size_t In numBytes) override;
							void CLOAK_CALL_THIS CopySubresource(Inout API::Rendering::IResource* dst, In UINT dstSubIndex, In API::Rendering::IResource* src, In UINT srcSubIndex) override;
							void CLOAK_CALL_THIS CopyCounter(Inout API::Rendering::IResource* dst, In size_t dstOffset, In API::Rendering::IStructuredBuffer* src) override;
							void CLOAK_CALL_THIS ResetCounter(Inout API::Rendering::IStructuredBuffer* buffer, In_opt uint32_t val = 0) override;
							void CLOAK_CALL_THIS WriteBuffer(Inout API::Rendering::IResource* dst, In size_t dstOffset, In_reads_bytes(numBytes) const void* data, In size_t numBytes) override;
							void CLOAK_CALL_THIS WriteBuffer(Inout API::Rendering::IResource* dst, In UINT firstSubresource, In UINT numSubresources, In_reads(numSubresources) const API::Rendering::SUBRESOURCE_DATA* srds) override;
							void CLOAK_CALL_THIS FillBuffer(Inout API::Rendering::IResource* dst, In size_t dstOffset, In const API::Rendering::DWParam& val, In size_t numBytes) override;
							API::Rendering::ReadBack CLOAK_CALL_THIS ReadBack(In API::Rendering::IPixelBuffer* src, In_opt UINT subresource = 0) override;
							API::Rendering::ReadBack CLOAK_CALL_THIS ReadBack(In API::Rendering::IGraphicBuffer* src) override;

							void CLOAK_CALL_THIS BeginQuery(In API::Rendering::IQuery* query) override;
							void CLOAK_CALL_THIS EndQuery(In API::Rendering::IQuery* query) override;
						protected:
							Success(return == true) bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							virtual uint64_t CLOAK_CALL_THIS Finish(In_opt bool wait = false, In_opt bool executeDirect = true);
							CLOAK_CALL CopyContext(In SingleQueue* queue, In IManager* manager);

							SingleQueue* m_queue;
							IManager* m_manager;
							ID3D12CommandAllocator* m_curAlloc;
							GraphicsCommandList m_cmdList;
							API::Global::Memory::PageStackAllocator m_pageAlloc;
							API::PagedStack<IResource*> m_usedResources;
							API::PagedStack<Query*> m_usedQueries;
							IDynamicAllocator* m_dynAlloc[2];
							IReadBackAllocator* m_readBack;
							D3D12_RESOURCE_BARRIER m_barrierBuffer[16];
							UINT m_barriersToFlush;
							std::atomic<uint64_t> m_lastFence;
							API::Helper::ISyncSection* m_sync;
							API::HashMap<IAdapter*, uint64_t> m_waitingQueues;
							bool m_blockedTransition;
							bool m_inRenderPass;
					};
					class CLOAK_UUID("{8727E750-B206-40E3-95C4-461C76B3437B}") Context : public virtual IContext, public CopyContext {
						friend class Queue_v1::SingleQueue;
						public:
							virtual CLOAK_CALL ~Context();
							void CLOAK_CALL_THIS ClearUAV(In API::Rendering::IStructuredBuffer* target) override;
							void CLOAK_CALL_THIS ClearUAV(In API::Rendering::IByteAddressBuffer* target) override;
							void CLOAK_CALL_THIS ClearUAV(In API::Rendering::IColorBuffer* target) override;
							void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In UINT numConstants, In_reads(numConstants) const API::Rendering::DWParam* constants) override;
							void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In const API::Rendering::DWParam& X) override;
							void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In const API::Rendering::DWParam& X, In const API::Rendering::DWParam& Y) override;
							void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In const API::Rendering::DWParam& X, In const API::Rendering::DWParam& Y, In const API::Rendering::DWParam& Z) override;
							void CLOAK_CALL_THIS SetConstants(In UINT rootIndex, In const API::Rendering::DWParam& X, In const API::Rendering::DWParam& Y, In const API::Rendering::DWParam& Z, In const API::Rendering::DWParam& W) override;
							void CLOAK_CALL_THIS SetConstantsByPos(In UINT rootIndex, In const API::Rendering::DWParam& X, In UINT pos) override;
							void CLOAK_CALL_THIS SetConstantBuffer(In UINT rootIndex, In API::Rendering::IGraphicBuffer* cbv) override;
							void CLOAK_CALL_THIS SetDynamicCBV(In UINT rootIndex, In size_t bufferSize, In_reads_bytes(bufferSize) const void* data) override;
							void CLOAK_CALL_THIS SetDynamicSRV(In UINT rootIndex, In size_t size, In_reads_bytes(size) const void* buffer) override;
							void CLOAK_CALL_THIS SetBufferSRV(In UINT rootIndex, In API::Rendering::IGraphicBuffer* srv) override;
							void CLOAK_CALL_THIS SetBufferUAV(In UINT rootIndex, In API::Rendering::IGraphicBuffer* uav) override;
							void CLOAK_CALL_THIS SetSRV(In UINT rootIndex, In const API::Rendering::ResourceView& srv) override;
							void CLOAK_CALL_THIS SetUAV(In UINT rootIndex, In const API::Rendering::ResourceView& uav) override;
							void CLOAK_CALL_THIS SetCBV(In UINT rootIndex, In const API::Rendering::ResourceView& cbv) override;
							void CLOAK_CALL_THIS SetDescriptorTable(In UINT rootIndex, In const API::Rendering::GPU_DESCRIPTOR_HANDLE& gpuAddress) override;
							void CLOAK_CALL_THIS SetDescriptorTable(In UINT rootIndex, In UINT offset, In_reads(count) const API::Rendering::ResourceView* views, In UINT count = 1) override;
							void CLOAK_CALL_THIS ClearColor(In API::Rendering::IColorBuffer* target) override;
							void CLOAK_CALL_THIS ClearDepth(In API::Rendering::IDepthBuffer* target) override;
							void CLOAK_CALL_THIS ClearStencil(In API::Rendering::IDepthBuffer* target) override;
							void CLOAK_CALL_THIS ClearDepthAndStencil(In API::Rendering::IDepthBuffer* target) override;
							void CLOAK_CALL_THIS SetIndexBuffer(In API::Rendering::IIndexBuffer* buffer) override;
							void CLOAK_CALL_THIS SetVertexBuffer(In API::Rendering::IVertexBuffer* buffer, In_opt UINT slot = 0) override;
							void CLOAK_CALL_THIS SetVertexBuffer(In UINT count, In_reads(count) API::Rendering::IVertexBuffer** buffer, In_opt UINT slot = 0) override;
							void CLOAK_CALL_THIS SetDynamicVertexBuffer(In UINT slot, In size_t numVertices, In size_t vertexStride, In_reads_bytes(numVertices*vertexStride) const void* data) override;
							void CLOAK_CALL_THIS SetDynamicIndexBuffer(In size_t count, In_reads(count) const uint16_t* data) override;
							void CLOAK_CALL_THIS SetDepthStencilRefValue(In UINT val) override;
							void CLOAK_CALL_THIS SetBlendFactor(In const API::Helper::Color::RGBA& blendColor) override;
							void CLOAK_CALL_THIS SetViewInstanceMask(In UINT mask) override;
							void CLOAK_CALL_THIS Draw(In UINT count, In_opt UINT offset = 0) override;
							void CLOAK_CALL_THIS DrawIndexed(In UINT count, In_opt UINT indexOffset = 0, In_opt INT vertexOffset = 0) override;
							void CLOAK_CALL_THIS DrawInstanced(In UINT countPerInstance, In UINT instanceCount, In_opt UINT vertexOffset = 0, In_opt UINT instanceOffset = 0) override;
							void CLOAK_CALL_THIS DrawIndexedInstanced(In UINT countPerInstance, In UINT instanceCount, In_opt UINT indexOffset = 0, In_opt INT vertexOffset = 0, In_opt UINT instanceOffset = 0) override;
							void CLOAK_CALL_THIS ExecuteIndirect(In API::Rendering::IStructuredBuffer* argBuffer, In_opt size_t argOffset = 0) override;
							void CLOAK_CALL_THIS SetPredication(In API::Rendering::IQuery* query, In bool value) override;
							void CLOAK_CALL_THIS ResetPredication() override;
							void CLOAK_CALL_THIS ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In API::Rendering::IColorBuffer* src) override;
							void CLOAK_CALL_THIS ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In UINT dstSubResource, In API::Rendering::IColorBuffer* src, In UINT srcSubResource) override;
							void CLOAK_CALL_THIS ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In API::Rendering::IColorBuffer* src, In UINT dstX, In UINT dstY) override;
							void CLOAK_CALL_THIS ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In UINT dstSubResource, In API::Rendering::IColorBuffer* src, In UINT srcSubResource, In UINT dstX, In UINT dstY) override;
							void CLOAK_CALL_THIS ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In API::Rendering::IColorBuffer* src, In UINT dstX, In UINT dstY, In UINT srcX, In UINT srcY, In UINT W, In UINT H) override;
							void CLOAK_CALL_THIS ResolveMultiSample(In API::Rendering::IColorBuffer* dst, In UINT dstSubResource, In API::Rendering::IColorBuffer* src, In UINT srcSubResource, In UINT dstX, In UINT dstY, In UINT srcX, In UINT srcY, In UINT W, In UINT H) override;
							void CLOAK_CALL_THIS SetDepthBounds(In float min, In float max) override;
							void CLOAK_CALL_THIS SetShadingRate(In API::Rendering::SHADING_RATE xAxis, In API::Rendering::SHADING_RATE yAxis) override;

							void CLOAK_CALL_THIS Dispatch(In_opt size_t groupCountX = 1, In_opt size_t groupCountY = 1, In_opt size_t groupCountZ = 1) override;
							void CLOAK_CALL_THIS Dispatch1D(In size_t threadCount, In_opt size_t size = 64) override;
							void CLOAK_CALL_THIS Dispatch2D(In size_t threadCountX, In size_t threadCountY, In_opt size_t sizeX = 8, In_opt size_t sizeY = 8) override;
							void CLOAK_CALL_THIS Dispatch3D(In size_t threadCountX, In size_t threadCountY, In size_t threadCountZ, In size_t sizeX, In size_t sizeY, In size_t sizeZ) override;

							void CLOAK_CALL_THIS SetRootSignature(In IRootSignature* sig) override;
							void CLOAK_CALL_THIS SetPipelineState(In API::Rendering::IPipelineState* pso) override;
							void CLOAK_CALL_THIS SetPipelineState(In API::Rendering::IPipelineState* pso, In bool setRoot) override;
							void CLOAK_CALL_THIS SetDynamicDescriptors(In UINT rootIndex, In UINT offest, In_reads(count) const API::Rendering::CPU_DESCRIPTOR_HANDLE* handles, In_opt UINT count = 1) override;
							void CLOAK_CALL_THIS SetWorker(In IRenderWorker* worker, In size_t passID, In API::Files::MaterialType matType, In API::Rendering::DRAW_TYPE drawType, In const API::Global::Math::Vector& camPos, In float camViewDist, In size_t camID, In API::Files::LOD maxLOD) override;

							API::Rendering::IPipelineState* CLOAK_CALL_THIS GetCurrentPipelineState() const override;
							IRootSignature* CLOAK_CALL_THIS GetCurrentRootSignature() const override;

							void CLOAK_CALL_THIS SetRenderTargets(In UINT numTargets, In_reads(numTargets) API::Rendering::IColorBuffer** RTVs, In_opt API::Rendering::IDepthBuffer* DSV = nullptr, In_opt API::Rendering::DSV_ACCESS access = API::Rendering::DSV_ACCESS::READ_ONLY_STENCIL) override;
							void CLOAK_CALL_THIS BeginRenderPass(In UINT numTargets, In_reads(numTargets) const API::Rendering::RENDER_PASS_RTV * RTVs, In const API::Rendering::RENDER_PASS_DSV & DSV, In_opt API::Rendering::RENDER_PASS_FLAG flags = API::Rendering::RENDER_PASS_FLAG_NONE) override;
							void CLOAK_CALL_THIS BeginRenderPass(In UINT numTargets, In_reads(numTargets) const API::Rendering::RENDER_PASS_RTV * RTVs, In_opt API::Rendering::RENDER_PASS_FLAG flags = API::Rendering::RENDER_PASS_FLAG_NONE) override;
							void CLOAK_CALL_THIS BeginRenderPass(In const API::Rendering::RENDER_PASS_DSV & DSV, In_opt API::Rendering::RENDER_PASS_FLAG flags = API::Rendering::RENDER_PASS_FLAG_NONE) override;
							void CLOAK_CALL_THIS EndRenderPass() override;
							void CLOAK_CALL_THIS SetViewport(In const API::Rendering::VIEWPORT& vp) override;
							void CLOAK_CALL_THIS SetViewport(In size_t count, In_reads(count) const API::Rendering::VIEWPORT* vp) override;
							void CLOAK_CALL_THIS SetViewport(In FLOAT x, In FLOAT y, In FLOAT width, In FLOAT height, In_opt FLOAT minDepth = 0, In_opt FLOAT maxDepth = 1) override;
							void CLOAK_CALL_THIS SetScissor(In const API::Rendering::SCISSOR_RECT& scissor) override;
							void CLOAK_CALL_THIS SetScissor(In size_t count, In_reads(count) const API::Rendering::SCISSOR_RECT* scissor) override;
							void CLOAK_CALL_THIS SetScissor(In LONG x, In LONG y, In LONG width, In LONG height) override;
							void CLOAK_CALL_THIS SetViewportAndScissor(In const API::Rendering::VIEWPORT& vp) override;
							void CLOAK_CALL_THIS SetViewportAndScissor(In const API::Rendering::SCISSOR_RECT& vp) override;
							void CLOAK_CALL_THIS SetViewportAndScissor(In const API::Rendering::VIEWPORT& vp, In const API::Rendering::SCISSOR_RECT& scissor) override;
							void CLOAK_CALL_THIS SetViewportAndScissor(In size_t count, In_reads(count) const API::Rendering::VIEWPORT* vp) override;
							void CLOAK_CALL_THIS SetViewportAndScissor(In size_t count, In_reads(count) const API::Rendering::SCISSOR_RECT* vp) override;
							void CLOAK_CALL_THIS SetViewportAndScissor(In size_t countVP, In_reads(countVP) const API::Rendering::VIEWPORT* vp, In size_t countSC, In_reads(countSC) const API::Rendering::SCISSOR_RECT* scissor) override;
							void CLOAK_CALL_THIS SetViewportAndScissor(In FLOAT x, In FLOAT y, In FLOAT width, In FLOAT height, In_opt FLOAT minDepth = 0, In_opt FLOAT maxDepth = 1) override;
							void CLOAK_CALL_THIS SetPrimitiveTopology(In API::Rendering::PRIMITIVE_TOPOLOGY topology) override;
							API::Rendering::IContext2D* CLOAK_CALL_THIS Enable2D(In UINT textureSlot) override;
							API::Rendering::IContext3D* CLOAK_CALL_THIS Enable3D(In const API::Global::Math::Matrix& WorldToProjection, In_opt bool cullNear = true, In_opt bool useIndirectBuffer = false, In_opt UINT rootIndexMaterial = INVALID_ROOT_INDEX, In_opt UINT rootIndexLocalBuffer = INVALID_ROOT_INDEX, In_opt UINT rootIndexDisplacement = INVALID_ROOT_INDEX, In_opt UINT rootIndexMetaConstant = INVALID_ROOT_INDEX) override;
							API::Rendering::IContext3D* CLOAK_CALL_THIS Enable3D(In_range(0, API::Rendering::MAX_VIEW_INSTANCES) size_t camCount, In_reads(camCount) const API::Global::Math::Matrix* WorldToProjection, In_opt bool cullNear = true, In_opt bool useIndirectBuffer = false, In_opt UINT rootIndexMaterial = INVALID_ROOT_INDEX, In_opt UINT rootIndexLocalBuffer = INVALID_ROOT_INDEX, In_opt UINT rootIndexDisplacement = INVALID_ROOT_INDEX, In_opt UINT rootIndexMetaConstant = INVALID_ROOT_INDEX) override;

							void CLOAK_CALL_THIS Reset() override;
							void CLOAK_CALL_THIS CleanAndDiscard(In uint64_t fence) override;

							void CLOAK_CALL_THIS SetDescriptorHeaps(In_reads(count) const D3D12_DESCRIPTOR_HEAP_TYPE* Type, In_reads(count) ID3D12DescriptorHeap* const* Heaps, In_range(1, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES) UINT count = 1);
							void CLOAK_CALL_THIS SetConstantBufferByAddress(In UINT rootIndex, In API::Rendering::GPU_ADDRESS cbv);
							void CLOAK_CALL_THIS BeginRenderPass(In UINT numTargets, In_reads(numTargets) const API::Rendering::RENDER_PASS_RTV * RTVs, In const API::Rendering::RENDER_PASS_DSV* DSV, In API::Rendering::RENDER_PASS_FLAG flags);
						protected:
							Success(return == true) bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							uint64_t CLOAK_CALL_THIS Finish(In_opt bool wait = false, In_opt bool executeDirect = true) override;
							void CLOAK_CALL_THIS BindDescriptorHeaps();
							CLOAK_CALL Context(In SingleQueue* queue, In IManager* manager);
							
							enum class RVCacheType { NONE, SRV, UAV, CBV, ADDRESS };

							ID3D12RootSignature* m_curRoot;
							API::Rendering::IPipelineState* m_curPSO;
							D3D12_VIEWPORT m_curViewport[API::Rendering::MAX_RENDER_TARGET];
							D3D12_RECT m_curScissor[API::Rendering::MAX_RENDER_TARGET];
							API::Rendering::DSV_ACCESS m_curDSVAccess;
							bool m_updatedDynHeap;
							size_t m_curRTVCount;
							RootSignature* m_rootSig;
							UINT m_lastViewMask;
							//This value is only for caching and does not use reference counting
							API::Rendering::IIndexBuffer* m_curIndexBuffer;
							//This value is only for caching and does not use reference counting
							API::Rendering::IVertexBuffer* m_curVertexBuffer[8];
							//This value is only for caching and does not use reference counting
							API::Rendering::IColorBuffer* m_curRTV[API::Rendering::MAX_RENDER_TARGET];
							//This value is only for caching and does not use reference counting
							API::Rendering::IDepthBuffer* m_curDSV;
							struct {
								RVCacheType Type;
								API::Rendering::ResourceView View;
								API::Rendering::GPU_ADDRESS Address;
							} m_curRV[Impl::Rendering::MAX_ROOT_PARAMETERS];
							
							ID3D12DescriptorHeap* m_currentHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
							DynamicDescriptorHeap* m_dynHeap;
							enum class Usage { GRAPHIC, COMPUTE, NONE } m_usage;
							bool m_useTessellation;
							API::Rendering::PRIMITIVE_TOPOLOGY m_topology;
							UINT m_lastSamplerMipLevel;
							
							Context2D m_2DContext;
							Context3D m_3DContext;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif