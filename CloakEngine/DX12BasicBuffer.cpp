#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/BasicBuffer.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/Device.h"
#include "Implementation/Rendering/Manager.h"

#include "Engine/Graphic/Core.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace BasicBuffer_v1 {
					void CLOAK_CALL_THIS PixelBuffer::CreateTexture(In IDevice* device, In UINT nodeMask, In size_t nodeID, In DXGI_FORMAT format, In const D3D12_RESOURCE_DESC& desc, In const D3D12_CLEAR_VALUE& clearVal)
					{
						API::Helper::Lock lock(m_sync);

						CloakDebugLog("Create Pixel Buffer");

						D3D12_HEAP_PROPERTIES heap;
						ZeroMemory(&heap, sizeof(heap));
						heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						heap.CreationNodeMask = nodeMask;
						heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						heap.Type = D3D12_HEAP_TYPE_DEFAULT;
						heap.VisibleNodeMask = nodeMask;

						m_width = desc.Width;
						m_height = desc.Height;
						m_format = desc.Format;
						m_realFormat = format;
						m_size = desc.DepthOrArraySize;
						m_curState = API::Rendering::STATE_COMMON;
						m_gpuHandle = API::Rendering::GPU_HANDLE_INVALID;
						m_numMipMaps = desc.MipLevels;
						m_sampleCount = desc.SampleDesc.Count;

						const D3D12_CLEAR_VALUE* cv = &clearVal;
						if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER && (desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)) == 0) { cv = nullptr; }

						RELEASE(m_data);
						Device* dxDev = nullptr;
						HRESULT hRet = E_FAIL;
						if (SUCCEEDED(device->QueryInterface(CE_QUERY_ARGS(&dxDev))))
						{
							hRet = dxDev->CreateCommittedResource(heap, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_COMMON, cv, CE_QUERY_ARGS(&m_data));
							RELEASE(dxDev);
						}
						if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_NO_RESSOURCES, true))
						{
							m_nodeID = nodeID + 1;
							iUpdateDecay(desc.Dimension, desc.Flags);
						}
					}
					void CLOAK_CALL_THIS PixelBuffer::UseTexture(In_opt ID3D12Resource* resource, In D3D12_RESOURCE_STATES state)
					{
						API::Helper::Lock lock(m_sync);
						if (m_data != nullptr) { m_data->Release(); }
						m_data = resource;

						m_curState = Casting::CastBackward(state);

						if (m_data != nullptr)
						{
							const D3D12_RESOURCE_DESC desc = m_data->GetDesc();
							m_width = desc.Width;
							m_height = desc.Height;
							m_size = desc.DepthOrArraySize;
							m_format = desc.Format;
							iUpdateDecay(desc.Dimension, desc.Flags);
						}
						else
						{
							m_width = 0;
							m_height = 0;
							m_size = 0;
							m_format = DXGI_FORMAT_UNKNOWN;
						}
					}
					CLOAK_CALL PixelBuffer::PixelBuffer() 
					{
						DEBUG_NAME(PixelBuffer);
						m_sampleCount = 0;
					}
					CLOAK_CALL PixelBuffer::~PixelBuffer() {}
					uint32_t CLOAK_CALL_THIS PixelBuffer::GetWidth() const { API::Helper::Lock lock(m_sync); return static_cast<uint32_t>(m_width); }
					uint32_t CLOAK_CALL_THIS PixelBuffer::GetHeight() const { API::Helper::Lock lock(m_sync); return m_height; }
					uint32_t CLOAK_CALL_THIS PixelBuffer::GetDepth() const { API::Helper::Lock lock(m_sync); return m_size; }
					uint32_t CLOAK_CALL_THIS PixelBuffer::GetSampleCount() const { API::Helper::Lock lock(m_sync); return m_sampleCount; }
					API::Rendering::Format CLOAK_CALL_THIS PixelBuffer::GetFormat() const { API::Helper::Lock lock(m_sync); return Casting::CastBackward(m_realFormat); }
					UINT CLOAK_CALL_THIS PixelBuffer::GetSubresourceIndex(In UINT mipMap, In UINT arrayIndex, In_opt UINT planeSlice) const { return min(m_numMipMaps, mipMap) + (min(arrayIndex, m_size)*m_numMipMaps) + (planeSlice*m_size*m_numMipMaps); }

					void CLOAK_CALL_THIS GraphicBuffer::InitBuffer(In UINT nodeMask, In size_t nodeID, In uint32_t numElements, In uint32_t elementSize, In_reads_bytes(numElements*elementSize) const void* data, In_opt API::Rendering::RESOURCE_STATE state)
					{
						API::Helper::Lock lock(m_sync);
						RELEASE(m_data);
						m_eCount = numElements;
						m_eSize = elementSize;
						m_size = numElements*elementSize;
						if (data != nullptr) { m_curState = API::Rendering::STATE_COPY_DEST; }
						else { m_curState = state; }
						m_nodeID = nodeID + 1;
						if (m_size > 0)
						{
							CloakDebugLog("Create Graphic Buffer");

							D3D12_RESOURCE_DESC desc;
							ZeroMemory(&desc, sizeof(desc));
							desc.Alignment = 0;
							desc.DepthOrArraySize = 1;
							desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
							desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
							desc.Format = DXGI_FORMAT_UNKNOWN;
							desc.Height = 1;
							desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
							desc.MipLevels = 1;
							desc.SampleDesc.Count = 1;
							desc.SampleDesc.Quality = 0;
							desc.Width = static_cast<UINT64>(m_size);

							D3D12_HEAP_PROPERTIES props;
							props.Type = D3D12_HEAP_TYPE_DEFAULT;
							props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
							props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
							props.CreationNodeMask = nodeMask;
							props.VisibleNodeMask = nodeMask;

							IManager* clm = Engine::Graphic::Core::GetCommandListManager();
							IDevice* dev = clm->GetDevice();
							Device* dxDev = nullptr;
							if (SUCCEEDED(dev->QueryInterface(CE_QUERY_ARGS(&dxDev))))
							{
								HRESULT hRet = dxDev->CreateCommittedResource(props, D3D12_HEAP_FLAG_NONE, desc, Casting::CastForward(m_curState), nullptr, CE_QUERY_ARGS(&m_data));
								if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_NO_RESSOURCES, true))
								{
									m_gpuHandle = m_data->GetGPUVirtualAddress();
									iUpdateDecay(desc.Dimension, desc.Flags);
									if (data != nullptr) { clm->InitializeBuffer(this, m_size, data, true); }
								}
								RELEASE(dxDev);
							}
							RELEASE(dev);
						}
					}
					void CLOAK_CALL_THIS GraphicBuffer::InitBuffer(In UINT nodeMask, In size_t nodeID, In uint32_t numElements, In uint32_t elementSize, In API::Rendering::RESOURCE_STATE state)
					{
						InitBuffer(nodeMask, nodeID, numElements, elementSize, nullptr, state);
					}
					CLOAK_CALL GraphicBuffer::GraphicBuffer()
					{
						DEBUG_NAME(GraphicBuffer);
					}
					CLOAK_CALL GraphicBuffer::~GraphicBuffer()
					{
						
					}
					const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS GraphicBuffer::GetGPUAddress() const { API::Helper::Lock lock(m_sync); return m_gpuHandle; }
					uint32_t CLOAK_CALL_THIS GraphicBuffer::GetSize() const { return m_size; }
					Success(return == true) bool CLOAK_CALL_THIS PixelBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, PixelBuffer, BasicBuffer_v1, Resource);
					}
					Success(return == true) bool CLOAK_CALL_THIS GraphicBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, GraphicBuffer, BasicBuffer_v1, Resource);
					}
				}
			}
		}
	}
}
#endif