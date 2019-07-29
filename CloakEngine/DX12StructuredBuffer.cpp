#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/StructuredBuffer.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/Manager.h"

#include "Engine/Graphic/Core.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace StructuredBuffer_v1 {
					StructuredBuffer* CLOAK_CALL StructuredBuffer::Create(In UINT nodeMask, In size_t nodeID, In bool counterBuffer, In uint32_t numElements, In uint32_t elementSize, In_reads_opt(numElements*elementSize) const void* data)
					{
						StructuredBuffer* res = new StructuredBuffer(counterBuffer);
						res->InitBuffer(nodeMask, nodeID, numElements, elementSize, data);
						return res;
					}
					CLOAK_CALL StructuredBuffer::StructuredBuffer(In bool useByteAddressBuffer)
					{ 
						m_byteAddress = useByteAddressBuffer ? new ByteAddressBuffer() : nullptr;
						DEBUG_NAME(StructuredBuffer);
					}
					CLOAK_CALL StructuredBuffer::~StructuredBuffer() 
					{ 
						SAVE_RELEASE(m_byteAddress); 
						if (Impl::Global::Game::canThreadRun())
						{
							IDescriptorHeap* heap = Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_CBV_SRV_UAV);
							heap->Free(m_SRV);
							heap->Free(m_UAV);
						}
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS StructuredBuffer::GetUAV() const { API::Helper::Lock lock(m_sync); return m_UAV; }
					API::Rendering::ResourceView CLOAK_CALL_THIS StructuredBuffer::GetSRV() const { API::Helper::Lock lock(m_sync); return m_SRV; }
					void CLOAK_CALL_THIS StructuredBuffer::Recreate(In UINT elementCount, In UINT elementSize, In_reads_opt(elementSize) const void* data)
					{
						API::Helper::Lock lock(m_sync);
						API::Helper::Lock ulock = LockUsage();
						if (m_eSize != elementSize || m_eCount != elementCount)
						{
							const UINT nid = static_cast<UINT>(m_nodeID - 1);
							GraphicBuffer::InitBuffer(1 << nid, nid, elementCount, elementSize, data, m_curState);
							CreateResourceViews(1, &m_SRV, API::Rendering::VIEW_TYPE::SRV);
							CreateResourceViews(1, &m_UAV, API::Rendering::VIEW_TYPE::UAV);
						}
						else if (data != nullptr)
						{
							IManager* clm = Engine::Graphic::Core::GetCommandListManager();
							clm->InitializeBuffer(this, m_size, data, true);
						}
					}
					void CLOAK_CALL_THIS StructuredBuffer::InitBuffer(In UINT nodeMask, In size_t nodeID, In uint32_t numElements, In uint32_t elementSize, In_reads_bytes(numElements*elementSize) const void* data, In_opt API::Rendering::RESOURCE_STATE state)
					{
						GraphicBuffer::InitBuffer(nodeMask, nodeID, numElements, elementSize, data, state);
						if (m_byteAddress != nullptr) { m_byteAddress->InitBuffer(nodeMask, nodeID, 1, sizeof(uint32_t)); }
					}
					void CLOAK_CALL_THIS StructuredBuffer::InitBuffer(In UINT nodeMask, In size_t nodeID, In uint32_t numElements, In uint32_t elementSize, In API::Rendering::RESOURCE_STATE state)
					{
						InitBuffer(nodeMask, nodeID, numElements, elementSize, nullptr, state);
					}
					size_t CLOAK_CALL_THIS StructuredBuffer::GetResourceViewCount(In API::Rendering::VIEW_TYPE view)
					{
						API::Helper::Lock lock(m_sync);
						if (((API::Rendering::VIEW_TYPE::UAV | API::Rendering::VIEW_TYPE::SRV) & view) == API::Rendering::VIEW_TYPE::NONE) { return 0; }
						return 1 + (m_byteAddress != nullptr ? m_byteAddress->GetResourceViewCount(view) : 0);
					}
					void CLOAK_CALL_THIS StructuredBuffer::CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE type)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(count > 0);
						CLOAK_ASSUME(count >= GetResourceViewCount(type));
						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();
						switch (type)
						{
							case API::Rendering::VIEW_TYPE::UAV:
							{
								UAV_DESC uavDesc;
								ZeroMemory(&uavDesc, sizeof(uavDesc));
								uavDesc.ViewDimension = UAV_DIMENSION_BUFFER;
								uavDesc.Format = API::Rendering::Format::UNKNOWN;
								uavDesc.Buffer.CounterOffsetInBytes = 0;
								uavDesc.Buffer.NumElements = m_eCount;
								uavDesc.Buffer.StructureByteStride = m_eSize;
								uavDesc.Buffer.Flags = UAV_FLAG_NONE;
								m_UAV = handles[0];
								dev->CreateUnorderedAccessView(this, m_byteAddress, uavDesc, m_UAV.CPU);
								break;
							}
							case API::Rendering::VIEW_TYPE::SRV:
							{
								SRV_DESC srvDesc;
								ZeroMemory(&srvDesc, sizeof(srvDesc));
								srvDesc.ViewDimension = SRV_DIMENSION_BUFFER;
								srvDesc.Format = API::Rendering::Format::UNKNOWN;
								srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
								srvDesc.Buffer.NumElements = m_eCount;
								srvDesc.Buffer.StructureByteStride = m_eSize;
								srvDesc.Buffer.Flags = SRV_FLAG_NONE;
								m_SRV = handles[0];
								dev->CreateShaderResourceView(this, srvDesc, m_SRV.CPU);
								break;
							}
							default:
								CLOAK_ASSUME(count == 0);
								break;
						}
						dev->Release();
						if (m_byteAddress != nullptr && count > 1) { m_byteAddress->CreateResourceViews(count - 1, &handles[1], type); }
					}

					API::Rendering::IByteAddressBuffer* CLOAK_CALL_THIS StructuredBuffer::GetCounterBuffer() 
					{ 
						if (m_byteAddress == nullptr) { return nullptr; }
						m_byteAddress->AddRef(); 
						return m_byteAddress; 
					}

					ByteAddressBuffer* CLOAK_CALL ByteAddressBuffer::Create(In UINT nodeMask, In size_t nodeID, In uint32_t numElements)
					{
						ByteAddressBuffer* res = new ByteAddressBuffer();
						res->InitBuffer(nodeMask, nodeID, numElements, 4);
						return res;
					}
					CLOAK_CALL ByteAddressBuffer::ByteAddressBuffer()
					{
						DEBUG_NAME(ByteAddressBuffer);
					}
					CLOAK_CALL ByteAddressBuffer::~ByteAddressBuffer() 
					{
						if (Impl::Global::Game::canThreadRun())
						{
							IDescriptorHeap* heap = Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_CBV_SRV_UAV);
							heap->Free(m_SRV);
							heap->Free(m_UAV);
						}
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS ByteAddressBuffer::GetUAV() const { API::Helper::Lock lock(m_sync); return m_UAV; }
					API::Rendering::ResourceView CLOAK_CALL_THIS ByteAddressBuffer::GetSRV() const { API::Helper::Lock lock(m_sync); return m_SRV; }
					size_t CLOAK_CALL_THIS ByteAddressBuffer::GetResourceViewCount(In API::Rendering::VIEW_TYPE view)
					{
						API::Helper::Lock lock(m_sync);
						if (((API::Rendering::VIEW_TYPE::UAV | API::Rendering::VIEW_TYPE::SRV) & view) == API::Rendering::VIEW_TYPE::NONE) { return 0; }
						return 1;
					}
					void CLOAK_CALL_THIS ByteAddressBuffer::CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE type)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(count > 0);
						CLOAK_ASSUME(count >= GetResourceViewCount(type));
						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();
						switch (type)
						{
							case API::Rendering::VIEW_TYPE::UAV:
							{
								UAV_DESC uavDesc;
								ZeroMemory(&uavDesc, sizeof(uavDesc));
								uavDesc.ViewDimension = UAV_DIMENSION_BUFFER;
								uavDesc.Format = API::Rendering::Format::R32_TYPELESS;
								uavDesc.Buffer.CounterOffsetInBytes = 0;
								uavDesc.Buffer.NumElements = static_cast<UINT>(m_size >> 2);
								uavDesc.Buffer.StructureByteStride = 0;
								uavDesc.Buffer.Flags = UAV_FLAG_RAW;
								uavDesc.Buffer.FirstElement = 0;
								m_UAV = handles[0];
								dev->CreateUnorderedAccessView(this, nullptr, uavDesc, m_UAV.CPU);
								break;
							}
							case API::Rendering::VIEW_TYPE::SRV:
							{
								SRV_DESC srvDesc;
								ZeroMemory(&srvDesc, sizeof(srvDesc));
								srvDesc.ViewDimension = SRV_DIMENSION_BUFFER;
								srvDesc.Format = API::Rendering::Format::R32_TYPELESS;
								srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
								srvDesc.Buffer.NumElements = static_cast<UINT>(m_size >> 2);
								srvDesc.Buffer.StructureByteStride = 0;
								srvDesc.Buffer.Flags = SRV_FLAG_RAW;
								m_SRV = handles[0];
								dev->CreateShaderResourceView(this, srvDesc, m_SRV.CPU);
								break;
							}
							default:
								CLOAK_ASSUME(count == 0);
								break;
						}
						dev->Release();
					}

					Success(return == true) bool CLOAK_CALL_THIS ByteAddressBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, ByteAddressBuffer, StructuredBuffer_v1, GraphicBuffer);
					}
					Success(return == true) bool CLOAK_CALL_THIS StructuredBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, StructuredBuffer, StructuredBuffer_v1, GraphicBuffer);
					}

					VertexBuffer* CLOAK_CALL VertexBuffer::Create(In UINT nodeMask, In size_t nodeID, In UINT numElements, In UINT elementSize, In_reads_opt(numElements*elementSize) const void* data)
					{
						VertexBuffer* res = new VertexBuffer();
						res->InitBuffer(nodeMask, nodeID, numElements, elementSize, data, API::Rendering::STATE_COMMON);
						return res;
					}
					CLOAK_CALL VertexBuffer::~VertexBuffer()
					{

					}

					API::Rendering::VERTEX_BUFFER_VIEW CLOAK_CALL_THIS VertexBuffer::GetVBV(In_opt uint32_t offset) const
					{
						API::Rendering::VERTEX_BUFFER_VIEW res;
						res.Address = m_gpuHandle.ptr + (offset*m_eSize);
						res.SizeInBytes = m_eSize * (m_eCount - offset);
						res.StrideInBytes = m_eSize;
						return res;
					}

					Success(return == true) bool CLOAK_CALL_THIS VertexBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, VertexBuffer, StructuredBuffer_v1, StructuredBuffer);
					}
					CLOAK_CALL VertexBuffer::VertexBuffer() : StructuredBuffer(false)
					{

					}

					IndexBuffer* CLOAK_CALL IndexBuffer::Create(In UINT nodeMask, In size_t nodeID, In UINT numElements, In bool b32, In_reads_opt(numElements*(b32 ? 4 : 2)) const void* data)
					{
						IndexBuffer* res = new IndexBuffer(b32);
						res->InitBuffer(nodeMask, nodeID, numElements, b32 ? 4 : 2, data, API::Rendering::STATE_COMMON);
						return res;
					}
					CLOAK_CALL IndexBuffer::~IndexBuffer()
					{

					}
					bool CLOAK_CALL_THIS IndexBuffer::Is32Bit() const
					{
						return m_32Bit;
					}
					API::Rendering::INDEX_BUFFER_VIEW CLOAK_CALL_THIS IndexBuffer::GetIBV(In_opt uint32_t offset) const
					{
						API::Rendering::INDEX_BUFFER_VIEW res;
						res.Address = m_gpuHandle.ptr + (offset*m_eSize);
						res.Format = m_32Bit ? API::Rendering::Format::R32_UINT : API::Rendering::Format::R16_UINT;
						res.SizeInBytes = m_eSize * (m_eCount - offset);
						return res;
					}

					Success(return == true) bool CLOAK_CALL_THIS IndexBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, IndexBuffer, StructuredBuffer_v1, StructuredBuffer);
					}
					CLOAK_CALL IndexBuffer::IndexBuffer(In bool b32) : m_32Bit(b32), StructuredBuffer(false)
					{

					}

					ConstantBuffer* CLOAK_CALL ConstantBuffer::Create(In UINT nodeMask, In size_t nodeID, In UINT elementSize, In_reads_opt(elementSize) const void* data)
					{
						ConstantBuffer* res = new ConstantBuffer();
						res->InitBuffer(nodeMask, nodeID, 1, static_cast<UINT>(elementSize + 0xFF) & ~static_cast<UINT>(0xFF), data, API::Rendering::STATE_COMMON); //CBV requres size to be 256-byte-aligned
						return res;
					}
					CLOAK_CALL ConstantBuffer::~ConstantBuffer()
					{
						if (Impl::Global::Game::canThreadRun())
						{
							IDescriptorHeap* heap = Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(m_nodeID - 1)->GetDescriptorHeap(HEAP_CBV_SRV_UAV);
							heap->Free(m_CBV);
						}
					}
					API::Rendering::ResourceView CLOAK_CALL_THIS ConstantBuffer::GetCBV() const { API::Helper::Lock lock(m_sync); return m_CBV; }

					Success(return == true) bool CLOAK_CALL_THIS ConstantBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, ConstantBuffer, StructuredBuffer_v1, GraphicBuffer);
					}
					CLOAK_CALL ConstantBuffer::ConstantBuffer()
					{
						DEBUG_NAME(ConstantBuffer);
					}
					size_t CLOAK_CALL_THIS ConstantBuffer::GetResourceViewCount(In API::Rendering::VIEW_TYPE view)
					{
						API::Helper::Lock lock(m_sync);
						if (((API::Rendering::VIEW_TYPE::CBV) & view) == API::Rendering::VIEW_TYPE::NONE) { return 0; }
						return 1;
					}
					void CLOAK_CALL_THIS ConstantBuffer::CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE type)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(count > 0);
						CLOAK_ASSUME(count >= GetResourceViewCount(type));
						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();
						switch (type)
						{
							case API::Rendering::VIEW_TYPE::CBV:
							{
								CBV_DESC desc;
								ZeroMemory(&desc, sizeof(desc));
								desc.BufferLocation = m_gpuHandle.ptr;
								desc.SizeInBytes = static_cast<UINT>(m_size);
								m_CBV = handles[0];
								dev->CreateConstantBufferView(desc, m_CBV.CPU);
								break;
							}
							default:
								CLOAK_ASSUME(count == 0);
								break;
						}
						dev->Release();
					}
				}
			}
		}
	}
}
#endif