#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/Device.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/Resource.h"
#include "Implementation/Rendering/DX12/ColorBuffer.h"
#include "Implementation/Rendering/DX12/RootSignature.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Device_v1 {
					CLOAK_CALL Device::Device(In const GraphicDevice& dev12)
					{
						DEBUG_NAME(Device);
						m_dev12 = dev12;
					}
					CLOAK_CALL Device::~Device()
					{						

					}
					uint32_t CLOAK_CALL_THIS Device::GetDescriptorHandleIncrementSize(In HEAP_TYPE type) const
					{
						return static_cast<uint32_t>(m_dev12.V0->GetDescriptorHandleIncrementSize(Casting::CastForward(type)));
					}
					void CLOAK_CALL_THIS Device::CreateConstantBufferView(In const CBV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle) const
					{
						D3D12_CONSTANT_BUFFER_VIEW_DESC rd = Casting::CastForward(desc);
						m_dev12.V0->CreateConstantBufferView(&rd, Casting::CastForward(cpuHandle));
					}
					void CLOAK_CALL_THIS Device::CreateShaderResourceView(In IResource* rsc, In const SRV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle)
					{
						Resource* res = nullptr;
						if (rsc!=nullptr && SUCCEEDED(rsc->QueryInterface(CE_QUERY_ARGS(&res))))
						{
							D3D12_SHADER_RESOURCE_VIEW_DESC rd = Casting::CastForward(desc);
							m_dev12.V0->CreateShaderResourceView(res->m_data, &rd, Casting::CastForward(cpuHandle));
							res->Release();
						}
					}
					void CLOAK_CALL_THIS Device::CreateRenderTargetView(In IResource* rsc, In const RTV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle)
					{
						Resource* res = nullptr;
						if (rsc != nullptr && SUCCEEDED(rsc->QueryInterface(CE_QUERY_ARGS(&res))))
						{
							D3D12_RENDER_TARGET_VIEW_DESC rd = Casting::CastForward(desc);
							m_dev12.V0->CreateRenderTargetView(res->m_data, &rd, Casting::CastForward(cpuHandle));
							res->Release();
						}
					}
					void CLOAK_CALL_THIS Device::CreateUnorderedAccessView(In IResource* rsc, In_opt IResource* byteAddress, In const UAV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle)
					{
						Resource* res = nullptr;
						if (rsc != nullptr && SUCCEEDED(rsc->QueryInterface(CE_QUERY_ARGS(&res))))
						{
							D3D12_UNORDERED_ACCESS_VIEW_DESC rd = Casting::CastForward(desc);
							Resource* ba = nullptr;
							if (byteAddress != nullptr && SUCCEEDED(byteAddress->QueryInterface(CE_QUERY_ARGS(&ba))))
							{
								m_dev12.V0->CreateUnorderedAccessView(res->m_data, ba->m_data, &rd, Casting::CastForward(cpuHandle));
								ba->Release();
							}
							else
							{
								m_dev12.V0->CreateUnorderedAccessView(res->m_data, nullptr, &rd, Casting::CastForward(cpuHandle));
							}
							res->Release();
						}
					}
					void CLOAK_CALL_THIS Device::CreateDepthStencilView(In IResource* rsc, In const DSV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle)
					{
						Resource* res = nullptr;
						if (rsc != nullptr && SUCCEEDED(rsc->QueryInterface(CE_QUERY_ARGS(&res))))
						{
							D3D12_DEPTH_STENCIL_VIEW_DESC rd = Casting::CastForward(desc);
							m_dev12.V0->CreateDepthStencilView(res->m_data, &rd, Casting::CastForward(cpuHandle));
							res->Release();
						}
					}
					void CLOAK_CALL_THIS Device::CreateSampler(In const API::Rendering::SAMPLER_DESC& desc, In API::Rendering::CPU_DESCRIPTOR_HANDLE handle)
					{
						D3D12_SAMPLER_DESC d;
						d.Filter = Casting::CastForward(desc.Filter);
						d.AddressU = Casting::CastForward(desc.AddressU);
						d.AddressV = Casting::CastForward(desc.AddressV);
						d.AddressW = Casting::CastForward(desc.AddressW);
						d.MipLODBias = desc.MipLODBias;
						d.MaxAnisotropy = desc.MaxAnisotropy;
						d.ComparisonFunc = Casting::CastForward(desc.CompareFunction);
						for (size_t a = 0; a < 4; a++) { d.BorderColor[a] = desc.BorderColor[a]; }
						d.MinLOD = desc.MinLOD;
						d.MaxLOD = desc.MaxLOD;
						m_dev12.V0->CreateSampler(&d, Casting::CastForward(handle));
					}
					void CLOAK_CALL_THIS Device::CopyDescriptorsSimple(In UINT numDescriptors, In const API::Rendering::CPU_DESCRIPTOR_HANDLE& dstStart, In const API::Rendering::CPU_DESCRIPTOR_HANDLE& srcStart, In HEAP_TYPE heap)
					{
						m_dev12.V0->CopyDescriptorsSimple(numDescriptors, Casting::CastForward(dstStart), Casting::CastForward(srcStart), Casting::CastForward(heap));
					}
					void CLOAK_CALL_THIS Device::CopyDescriptors(In UINT numDstRanges, In_reads(numDstRanges) const API::Rendering::CPU_DESCRIPTOR_HANDLE* dstRangeStarts, In_reads_opt(numDstRanges) const UINT* dstRangeSizes, In UINT numSrcRanges, In_reads(numSrcRanges) const API::Rendering::CPU_DESCRIPTOR_HANDLE* srcRangeStarts, In_reads_opt(numSrcRanges) const UINT* srcRangeSizes, In HEAP_TYPE heap)
					{
						D3D12_CPU_DESCRIPTOR_HANDLE* dstRange = NewArray(D3D12_CPU_DESCRIPTOR_HANDLE, numDstRanges);
						D3D12_CPU_DESCRIPTOR_HANDLE* srcRange = NewArray(D3D12_CPU_DESCRIPTOR_HANDLE, numSrcRanges);
						for (UINT a = 0; a < numDstRanges; a++) { dstRange[a] = Casting::CastForward(dstRangeStarts[a]); }
						for (UINT a = 0; a < numSrcRanges; a++) { srcRange[a] = Casting::CastForward(srcRangeStarts[a]); }
						m_dev12.V0->CopyDescriptors(numDstRanges, dstRange, dstRangeSizes, numSrcRanges, srcRange, srcRangeSizes, Casting::CastForward(heap));
						DeleteArray(dstRange);
						DeleteArray(srcRange);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateRootSignature(In const void* data, In size_t dataSize, In REFIID riid, Out void** ppvObject)
					{
						const UINT nodeCount = m_dev12.V0->GetNodeCount();
						return m_dev12.V0->CreateRootSignature(nodeCount == 1 ? 0 : ((1 << nodeCount) - 1), data, dataSize, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateCommandQueue(In const D3D12_COMMAND_QUEUE_DESC& desc, In REFIID riid, Out void** ppvObject)
					{
						CLOAK_ASSUME((desc.NodeMask == 0) == (m_dev12.V0->GetNodeCount() == 1));
						return m_dev12.V0->CreateCommandQueue(&desc, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateFence(In uint64_t value, In D3D12_FENCE_FLAGS flags, In REFIID riid, Out void** ppvObject)
					{
						return m_dev12.V0->CreateFence(value, flags, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateCommandList(In UINT node, In D3D12_COMMAND_LIST_TYPE type, In ID3D12CommandAllocator* alloc, In REFIID riid, Out void** ppvObject)
					{
						return m_dev12.V0->CreateCommandList(node, type, alloc, nullptr, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateCommandAllocator(In D3D12_COMMAND_LIST_TYPE type, In REFIID riid, Out void** ppvObject)
					{
						return m_dev12.V0->CreateCommandAllocator(type, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateCommittedResource(In const D3D12_HEAP_PROPERTIES& heap, In D3D12_HEAP_FLAGS heapFlags, In const D3D12_RESOURCE_DESC& desc, In D3D12_RESOURCE_STATES state, In_opt const D3D12_CLEAR_VALUE* clearValue, In REFIID riid, Out void** ppvObject)
					{
						CLOAK_ASSUME((heap.CreationNodeMask == 0) == (m_dev12.V0->GetNodeCount() == 1) && heap.CreationNodeMask == heap.VisibleNodeMask);
						return m_dev12.V0->CreateCommittedResource(&heap, heapFlags, &desc, state, clearValue, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateDescriptorHeap(In const D3D12_DESCRIPTOR_HEAP_DESC& desc, REFIID riid, void** ppvObject)
					{
						CLOAK_ASSUME((desc.NodeMask == 0) == (m_dev12.V0->GetNodeCount() == 1));
						return m_dev12.V0->CreateDescriptorHeap(&desc, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateComputePipelineState(In const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, REFIID riid, void** ppvObject)
					{
						CLOAK_ASSUME((desc.NodeMask == 0) == (m_dev12.V0->GetNodeCount() == 1));
						return m_dev12.V0->CreateComputePipelineState(&desc, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreateGraphicsPipelineState(In const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, REFIID riid, void** ppvObject)
					{
						CLOAK_ASSUME((desc.NodeMask == 0) == (m_dev12.V0->GetNodeCount() == 1));
						return m_dev12.V0->CreateGraphicsPipelineState(&desc, riid, ppvObject);
					}
					HRESULT CLOAK_CALL_THIS Device::CreatePipelineState(In D3D12_PIPELINE_STATE_DESC& desc, REFIID riid, void** ppvObject)
					{
						if (m_dev12.V2 != nullptr)
						{
							D3D12_PIPELINE_STATE_STREAM_DESC d;
							d.pPipelineStateSubobjectStream = &desc;
							d.SizeInBytes = sizeof(desc);
							return m_dev12.V2->CreatePipelineState(&d, riid, ppvObject);
						}
						else
						{
							//TODO
						}
						return E_FAIL;
					}
					HRESULT CLOAK_CALL_THIS Device::CreateQueryHeap(In const D3D12_QUERY_HEAP_DESC& desc, REFIID riid, void** ppvObject)
					{
						CLOAK_ASSUME((desc.NodeMask == 0) == (m_dev12.V0->GetNodeCount() == 1));
						return m_dev12.V0->CreateQueryHeap(&desc, riid, ppvObject);
					}
					void CLOAK_CALL_THIS Device::GetCopyableFootprints(In const D3D12_RESOURCE_DESC& resourceDesc, In UINT firstSubresource, In UINT numSubresources, In UINT64 baseOffset, Out_writes(numSubresources) D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts, Out_writes(numSubresources) UINT* numRows, Out_writes(numSubresources) UINT64* rowSizeInBytes, Out_opt UINT64* totalBytes)
					{
						m_dev12.V0->GetCopyableFootprints(&resourceDesc, firstSubresource, numSubresources, baseOffset, layouts, numRows, rowSizeInBytes, totalBytes);
					}
					Use_annotations HRESULT STDMETHODCALLTYPE Device::QueryInterface(REFIID riid, void** ppvObject)
					{
						if (ppvObject == nullptr) { return E_INVALIDARG; }
						*ppvObject = nullptr;
						bool got = false;
						if (riid == __uuidof(ID3D12Device)) 
						{ 
							*ppvObject = static_cast<ID3D12Device*>(m_dev12.V0.Get());
							m_dev12.V0->AddRef();
							got = true;
						}
						else if (riid == __uuidof(Impl::Rendering::DX12::Device_v1::Device)) 
						{ 
							*ppvObject = static_cast<Impl::Rendering::DX12::Device_v1::Device*>(this);
							AddRef();
							got = true;
						}
						else if (riid == __uuidof(Impl::Rendering::Device_v1::IDevice)) 
						{ 
							*ppvObject = static_cast<Impl::Rendering::Device_v1::IDevice*>(this);
							AddRef();
							got = true;
						}
						else
						{
							got = SavePtr::iQueryInterface(riid, ppvObject);
							if (got) { AddRef(); }
						}
						return got ? S_OK : E_NOINTERFACE;
					}
				}
			}
		}
	}
}
#endif