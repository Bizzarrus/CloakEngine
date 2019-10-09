#pragma once
#ifndef CE_IMPL_RENDERING_DX12_MANAGER_H
#define CE_IMPL_RENDERING_DX12_MANAGER_H

#include "CloakEngine/Helper/SyncSection.h"

#include "Implementation/Rendering/Manager.h"
#include "Implementation/Helper/SavePtr.h"

#include "Implementation\Rendering\DX12\Defines.h"
#include "Implementation/Rendering/DX12/Context.h"
#include "Implementation/Rendering/DX12/Allocator.h"
#include "Implementation/Rendering/DX12/Queue.h"
#include "Implementation/Rendering/DX12/Device.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <queue>
#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace Manager_v1 {
					class CLOAK_UUID("{FD079353-662A-42F1-B39F-37A38A2DCFEB}") Manager : public virtual IManager, public Impl::Helper::SavePtr_v1::SavePtr {
						friend class Context;
						public:
							CLOAK_CALL Manager();
							virtual CLOAK_CALL ~Manager();
							HRESULT CLOAK_CALL_THIS Create(In UINT adapterID) override;
							HMONITOR CLOAK_CALL_THIS GetMonitor() const override;
							void CLOAK_CALL_THIS ShutdownAdapters() override;
							void CLOAK_CALL_THIS ShutdownMemory() override;
							void CLOAK_CALL_THIS ShutdownDevice() override;
							void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IResource* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) override;
							void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IPixelBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) override;
							void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IColorBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::WORLD) override;
							void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IDepthBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL) override;
							void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IGraphicBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) override;
							void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IStructuredBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) override;
							void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IConstantBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) override;
							void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IColorBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::WORLD) override;
#ifdef _DEBUG
							void CLOAK_CALL_THIS BeginContextFile(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In REFIID riid, Outptr void** ptr, In const std::string& file) const override;
#else
							void CLOAK_CALL_THIS BeginContext(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In REFIID riid, Outptr void** ptr) const override;
#endif
							void CLOAK_CALL_THIS InitializeBuffer(In API::Rendering::IResource* dst, In size_t size, In_reads_bytes(size) const void* data, In_opt bool wait = false) override;
							void CLOAK_CALL_THIS InitializeTexture(In API::Rendering::IResource* dst, In UINT numSubresources, In_reads(numSubresources) API::Rendering::SUBRESOURCE_DATA data[], In_opt bool wait = false) const override;
							void CLOAK_CALL_THIS UpdateTextureSubresource(In API::Rendering::IResource* dst, In UINT firstSubresource, In UINT numSubresources, In_reads(numSubresources) API::Rendering::SUBRESOURCE_DATA data[], In_opt bool wait = false) const override;
							uint64_t CLOAK_CALL_THIS FlushExecutions(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In_opt bool wait = false) override;
							API::Rendering::SHADER_MODEL CLOAK_CALL_THIS GetShaderModel() const override;
							uint64_t CLOAK_CALL_THIS IncrementFence(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID) override;
							bool CLOAK_CALL_THIS IsReady() const override;
							bool CLOAK_CALL_THIS IsFenceComplete(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In uint64_t val) override;
							bool CLOAK_CALL_THIS WaitForFence(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In uint64_t val) override;
							void CLOAK_CALL_THIS WaitForGPU() override;
							bool CLOAK_CALL_THIS SupportMultiThreadedRendering() const override;
							IDevice* CLOAK_CALL_THIS GetDevice() const override;
							API::Rendering::IPipelineState* CLOAK_CALL_THIS CreatePipelineState() const override;
							API::Rendering::IQuery* CLOAK_CALL_THIS CreateQuery(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In API::Rendering::QUERY_TYPE type) const override;
							IRootSignature* CLOAK_CALL_THIS CreateRootSignature() const override;
							IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In ISwapChain* base, In uint32_t textureNum) const override;
							IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In const API::Rendering::TEXTURE_DESC& desc, In size_t nodeID, In UINT nodeMask) const override;
							IDepthBuffer* CLOAK_CALL_THIS CreateDepthBuffer(In const API::Rendering::DEPTH_DESC& desc, In size_t nodeID, In UINT nodeMask) const override;
							CE::RefPointer<ISwapChain> CLOAK_CALL_THIS CreateSwapChain(In const API::Global::Graphic::Settings& gset, Out_opt HRESULT* hRet = nullptr) override;
							bool CLOAK_CALL_THIS EnumerateAdapter(In size_t id, Out std::string* name) const override;
							size_t CLOAK_CALL_THIS GetNodeCount() const override;
							const API::Rendering::Hardware& CLOAK_CALL_THIS GetHardwareSetting() const override;

							API::Rendering::IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::TEXTURE_DESC& desc, In const API::Helper::Color::RGBA* const* colorData) const override;
							API::Rendering::IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::TEXTURE_DESC& desc, In API::Helper::Color::RGBA** colorData) const override;
							API::Rendering::IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::TEXTURE_DESC& desc) const override;
							API::Rendering::IDepthBuffer* CLOAK_CALL_THIS CreateDepthBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::DEPTH_DESC& desc) const override;
							API::Rendering::IVertexBuffer* CLOAK_CALL_THIS CreateVertexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT vertexCount, In UINT vertexSize, In_reads(vertexCount*vertexSize) const void* data) const override;
							API::Rendering::IIndexBuffer* CLOAK_CALL_THIS CreateIndexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT size, In_reads(size) const uint16_t* data) const override;
							API::Rendering::IIndexBuffer* CLOAK_CALL_THIS CreateIndexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT size, In_reads(size) const uint32_t* data) const override;
							API::Rendering::IConstantBuffer* CLOAK_CALL_THIS CreateConstantBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementSize, In_reads_opt(elementSize) const void* data = nullptr) const override;
							API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS CreateStructuredBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementCount, In UINT elementSize, In_reads(elementSize) const void* data, In_opt bool counterBuffer = true) const override;
							API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS CreateStructuredBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementCount, In UINT elementSize, In_opt bool counterBuffer = true, In_reads_opt(elementSize) const void* data = nullptr) const override;
							API::Rendering::IByteAddressBuffer* CLOAK_CALL_THIS CreateByteAddressBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In_opt UINT count = 1) const override;
							size_t CLOAK_CALL_THIS GetDataPos(In const API::Rendering::TEXTURE_DESC& desc, In uint32_t MipMap, In_opt uint32_t Image = 0) const override;
							size_t CLOAK_CALL_THIS GetNodeID(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID) const override;
							IQueue* CLOAK_CALL_THIS GetQueueByNodeID(In size_t nodeID) const override;
							ISingleQueue* CLOAK_CALL_THIS GetQueue(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID) const override;
							virtual void CLOAK_CALL_THIS ReadBuffer(In size_t dstByteSize, Out_writes(dstByteSize) void* dst, In API::Rendering::IPixelBuffer* src, In_opt UINT subresource = 0) const override;
							virtual void CLOAK_CALL_THIS ReadBuffer(In size_t dstByteSize, Out_writes(dstByteSize) void* dst, In API::Rendering::IGraphicBuffer* src) const override;
							void CLOAK_CALL_THIS ReadBuffer(In size_t dstSize, Out_writes(dstSize) API::Helper::Color::RGBA* color, In API::Rendering::IColorBuffer* src, In_opt UINT subresource = 0) const override;
							void CLOAK_CALL_THIS Update() override;

							ID3D12CommandQueue* CLOAK_CALL_THIS GetCommandQueue() const;
							bool CLOAK_CALL_THIS GetFactory(In REFIID riid, Outptr void** ptr) const;
							bool CLOAK_CALL_THIS UpdateFactory();
						private:							
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							void CLOAK_CALL_THIS GetNodeInfo(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, Out QUEUE_TYPE* qtype, Out UINT* node) const;

							API::Rendering::Hardware m_hardwareSettings;
							D3D_FEATURE_LEVEL m_featureLevel;
							UINT m_adapterCount;
							API::List<std::string> m_adapterNames;
							HMONITOR m_monitor;
							Queue* m_queues;
							std::atomic<IDXGIFactory5*> m_factory;
							Device* m_device;
							std::atomic<bool> m_isReady;
							std::atomic<bool> m_init;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif