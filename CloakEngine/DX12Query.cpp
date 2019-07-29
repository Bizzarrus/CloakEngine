#include "stdafx.h"
#include "Implementation/Rendering/DX12/Query.h"
#include "Implementation/Rendering/DX12/Casting.h"

#include <d3d12video.h>

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Query_v1 {
					constexpr size_t NUM_QUERIES[] =
					{
						/*QUERY_TYPE_OCCLUSION*/			1,
						/*QUERY_TYPE_OCCLUSION_COUNT*/		1,
						/*QUERY_TYPE_TIMESTAMP*/			1,
						/*QUERY_TYPE_PIPELINE_STATISTICS*/	1,
						/*QUERY_TYPE_STREAM_OUTPUT_ALL*/	4,
						/*QUERY_TYPE_STREAM_OUTPUT_0*/		1,
						/*QUERY_TYPE_STREAM_OUTPUT_1*/		1,
						/*QUERY_TYPE_STREAM_OUTPUT_2*/		1,
						/*QUERY_TYPE_STREAM_OUTPUT_3*/		1,
					};
					constexpr size_t ELEMENT_SIZE[] = 
					{
						/*QUERY_TYPE_OCCLUSION*/			sizeof(UINT64),
						/*QUERY_TYPE_OCCLUSION_COUNT*/		sizeof(UINT64),
						/*QUERY_TYPE_TIMESTAMP*/			sizeof(UINT64),
						/*QUERY_TYPE_PIPELINE_STATISTICS*/	sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS),
						/*QUERY_TYPE_STREAM_OUTPUT_ALL*/	sizeof(D3D12_QUERY_DATA_SO_STATISTICS)*4,
						/*QUERY_TYPE_STREAM_OUTPUT_0*/		sizeof(D3D12_QUERY_DATA_SO_STATISTICS),
						/*QUERY_TYPE_STREAM_OUTPUT_1*/		sizeof(D3D12_QUERY_DATA_SO_STATISTICS),
						/*QUERY_TYPE_STREAM_OUTPUT_2*/		sizeof(D3D12_QUERY_DATA_SO_STATISTICS),
						/*QUERY_TYPE_STREAM_OUTPUT_3*/		sizeof(D3D12_QUERY_DATA_SO_STATISTICS),
					};
					constexpr size_t HEAP_SIZE[] = 
					{
						/*D3D12_QUERY_HEAP_TYPE_OCCLUSION*/					sizeof(UINT64),
						/*D3D12_QUERY_HEAP_TYPE_TIMESTAMP*/					sizeof(UINT64),
						/*D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS*/		sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS),
						/*D3D12_QUERY_HEAP_TYPE_SO_STATISTICS*/				sizeof(D3D12_QUERY_DATA_SO_STATISTICS),
						/*D3D12_QUERY_HEAP_TYPE_VIDEO_DECODE_STATISTICS*/	sizeof(D3D12_QUERY_DATA_VIDEO_DECODE_STATISTICS),
						/*D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP*/		sizeof(UINT64),
					};

					inline constexpr size_t CLOAK_CALL MAX_ELEMENT_SIZE() 
					{
						size_t r = 0;
						for (size_t a = 0; a < ARRAYSIZE(ELEMENT_SIZE); a++) { r = max(r, ELEMENT_SIZE[a]); }
						return r;
					}

					CLOAK_CALL QueryPoolPage::QueryPoolPage(In size_t elementSize, In ID3D12QueryHeap* heap, In ID3D12Resource* base, In IQueue* queue) : Resource(base, API::Rendering::STATE_COPY_DEST, queue->GetNodeID(), false), m_heap(heap), m_elementSize(elementSize), m_queue(queue)
					{
						if (m_heap != nullptr) { m_heap->AddRef(); }
						for (size_t a = 0; a < ELEMENT_COUNT; a++) { m_usage[a].Flags = 0; }
					}
					CLOAK_CALL QueryPoolPage::~QueryPoolPage()
					{
						if (m_heap != nullptr) { m_heap->Release(); }
					}

					bool CLOAK_CALL_THIS QueryPoolPage::TryToAllocate(In QUEUE_TYPE queue, Out UINT* offset, Out UINT* index)
					{
						CLOAK_ASSUME(offset != nullptr && index != nullptr);
						API::Helper::Lock lock(m_sync);
						for (size_t a = 0; a < ELEMENT_COUNT; a++)
						{
							if (m_usage[a].Flags == 0)
							{
								m_usage[a].Flags = 0x01;
								m_usage[a].Queue = queue;
								m_usage[a].Fence = 0;
								m_usage[a].LastFence = 0;
								m_usage[a].Update = 0;
								*offset = static_cast<UINT>(a*m_elementSize);
								*index = static_cast<UINT>(a);
								return true;
							}
						}
						return false;
					}
					void CLOAK_CALL_THIS QueryPoolPage::Free(In UINT index)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(index < ELEMENT_COUNT);
						m_usage[index].Flags &= static_cast<uint8_t>(~0x01);
					}
					bool CLOAK_CALL_THIS QueryPoolPage::TryReadback(In UINT index, Out void* dst, Inout uint64_t* UpdateFlag)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(index < ELEMENT_COUNT);
						if (m_usage[index].Fence != 0 && m_usage[index].LastFence != m_usage[index].Fence && m_queue->IsFenceComplete(m_usage[index].Queue, m_usage[index].Fence))
						{
							m_usage[index].LastFence = m_usage[index].Fence;
							m_usage[index].Update++;
						}
						if (m_usage[index].Update != *UpdateFlag)
						{
							*UpdateFlag = m_usage[index].Update;
							D3D12_RANGE r;
							r.Begin = (index*m_elementSize);
							r.End = r.Begin + m_elementSize;
							void* src = nullptr;
							HRESULT hRet = m_data->Map(0, &r, &src);
							assert(SUCCEEDED(hRet));
							memcpy(dst, src, m_elementSize);
							m_data->Unmap(0, nullptr);
							return true;
						}
						return false;
					}
					void CLOAK_CALL_THIS QueryPoolPage::Clear(In UINT index, In uint64_t fence)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(index < ELEMENT_COUNT);
						if (m_usage[index].Fence != 0 && m_usage[index].LastFence != m_usage[index].Fence && m_queue->IsFenceComplete(m_usage[index].Queue, m_usage[index].Fence))
						{
							m_usage[index].LastFence = m_usage[index].Fence;
							m_usage[index].Update++;
						}
						m_usage[index].Fence = fence;
					}
					ID3D12QueryHeap* CLOAK_CALL_THIS QueryPoolPage::GetHeap() const { return m_heap; }

					Success(return == true) bool CLOAK_CALL_THIS QueryPoolPage::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(QueryPoolPage)) { *ptr = (QueryPoolPage*)this; return true; }
						return Resource::iQueryInterface(riid, ptr);
					}


					CLOAK_CALL QueryPool::QueryPool(In IQueue* queue) : m_poolOcclusion(D3D12_QUERY_HEAP_TYPE_OCCLUSION, queue, 1), m_poolTimestamp(D3D12_QUERY_HEAP_TYPE_TIMESTAMP, queue, 1), m_poolPipelineStatistics(D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS, queue, 1), m_poolStreamOutputSingle(D3D12_QUERY_HEAP_TYPE_SO_STATISTICS, queue, 1), m_poolStreamOutputAll(D3D12_QUERY_HEAP_TYPE_SO_STATISTICS, queue, 4)
					{

					}
					CLOAK_CALL QueryPool::~QueryPool()
					{

					}
					void CLOAK_CALL_THIS QueryPool::Allocate(In API::Rendering::QUERY_TYPE type, In QUEUE_TYPE queue, Out QueryPoolPage** page, Out UINT* offset, Out UINT* index)
					{
						switch (type)
						{
							case CloakEngine::API::Rendering::QUERY_TYPE_OCCLUSION:
							case CloakEngine::API::Rendering::QUERY_TYPE_OCCLUSION_COUNT:
								m_poolOcclusion.Allocate(page, queue, offset, index);
								break;
							case CloakEngine::API::Rendering::QUERY_TYPE_TIMESTAMP:
								m_poolTimestamp.Allocate(page, queue, offset, index);
								break;
							case CloakEngine::API::Rendering::QUERY_TYPE_PIPELINE_STATISTICS:
								m_poolPipelineStatistics.Allocate(page, queue, offset, index);
								break;
							case CloakEngine::API::Rendering::QUERY_TYPE_STREAM_OUTPUT_ALL:
								m_poolStreamOutputAll.Allocate(page, queue, offset, index);
								break;
							case CloakEngine::API::Rendering::QUERY_TYPE_STREAM_OUTPUT_0:
							case CloakEngine::API::Rendering::QUERY_TYPE_STREAM_OUTPUT_1:
							case CloakEngine::API::Rendering::QUERY_TYPE_STREAM_OUTPUT_2:
							case CloakEngine::API::Rendering::QUERY_TYPE_STREAM_OUTPUT_3:
								m_poolStreamOutputSingle.Allocate(page, queue, offset, index);
								break;
							default:
								CLOAK_ASSUME(false);
								break;
						}
					}

					CLOAK_CALL QueryPool::Pool::Pool(In D3D12_QUERY_HEAP_TYPE type, In IQueue* queue, In size_t numQueries) : m_type(type), m_numQueries(numQueries), m_queue(queue)
					{
						m_sync = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					}
					CLOAK_CALL QueryPool::Pool::~Pool()
					{
						SAVE_RELEASE(m_sync);
						for (size_t a = 0; a < m_heap.size(); a++) { m_heap[a]->Release(); }
					}

					void CLOAK_CALL_THIS QueryPool::Pool::Allocate(Out QueryPoolPage** page, In QUEUE_TYPE queue, Out UINT* offset, Out UINT* index)
					{
						API::Helper::Lock lock(m_sync);
						for (size_t a = 0; a < m_heap.size(); a++)
						{
							if (m_heap[a]->TryToAllocate(queue, offset, index) == true) { *page = m_heap[a]; return; }
						}
						QueryPoolPage* np = CreateNewPage();
						CLOAK_ASSUME(np != nullptr);
						const bool r = np->TryToAllocate(queue, offset, index);
						CLOAK_ASSUME(r == true);
						*page = np;
					}

					QueryPoolPage* CLOAK_CALL_THIS QueryPool::Pool::CreateNewPage()
					{
						QueryPoolPage* r = nullptr;
						const UINT nodeMask = m_queue->GetNodeMask();

						CloakDebugLog("Create Query Pool Page");

						D3D12_QUERY_HEAP_DESC hd;
						ZeroMemory(&hd, sizeof(hd));
						hd.Type = m_type;
						hd.NodeMask = nodeMask;
						hd.Count = static_cast<UINT>(QueryPoolPage::ELEMENT_COUNT * m_numQueries);

						D3D12_RESOURCE_DESC rd;
						ZeroMemory(&rd, sizeof(rd));
						rd.Alignment = 0;
						rd.DepthOrArraySize = 1;
						rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
						rd.Height = 1;
						rd.Width = m_numQueries * HEAP_SIZE[static_cast<size_t>(m_type)] * QueryPoolPage::ELEMENT_COUNT;
						rd.MipLevels = 1;
						rd.Format = DXGI_FORMAT_UNKNOWN;
						rd.SampleDesc.Count = 1;
						rd.SampleDesc.Quality = 0;
						rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
						rd.Flags = D3D12_RESOURCE_FLAG_NONE;

						D3D12_HEAP_PROPERTIES hp;
						ZeroMemory(&hp, sizeof(hp));
						hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						hp.CreationNodeMask = nodeMask;
						hp.VisibleNodeMask = nodeMask;
						hp.Type = D3D12_HEAP_TYPE_READBACK;

						IDevice* device = m_queue->GetDevice();
						Device* dxdev = nullptr;
						if (SUCCEEDED(device->QueryInterface(CE_QUERY_ARGS(&dxdev))))
						{
							ID3D12Resource* buffer = nullptr;
							HRESULT hRet = dxdev->CreateCommittedResource(hp, D3D12_HEAP_FLAG_NONE, rd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, CE_QUERY_ARGS(&buffer));
							if (SUCCEEDED(hRet))
							{
								ID3D12QueryHeap* heap = nullptr;
								hRet = dxdev->CreateQueryHeap(hd, CE_QUERY_ARGS(&heap));
								if (SUCCEEDED(hRet))
								{
									r = new QueryPoolPage(HEAP_SIZE[static_cast<size_t>(m_type)], heap, buffer, m_queue);
								}
							}
							dxdev->Release();
						}
						device->Release();
						return r;
					}

					CLOAK_CALL Query::Query(In QueryPool* pool, In QUEUE_TYPE queue, In API::Rendering::QUERY_TYPE type)
					{
						m_sync = nullptr;
						pool->Allocate(type, queue, &m_page, &m_offset, &m_index);
						m_data.Type = type;
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
						m_beginCount = 0;
						m_usageCount = 0;
						m_maxFence = 0;
						m_updateFlag = 0;
					}
					CLOAK_CALL Query::~Query()
					{
						m_page->Free(m_index);
						SAVE_RELEASE(m_sync);
					}

					API::Rendering::QUERY_TYPE CLOAK_CALL_THIS Query::GetType() const { return m_data.Type; }
					const API::Rendering::QUERY_DATA& CLOAK_CALL_THIS Query::GetData()
					{
						API::Helper::Lock lock(m_sync);
						uint8_t tmpData[MAX_ELEMENT_SIZE()];
						if (m_page->TryReadback(m_index, tmpData, &m_updateFlag))
						{
							switch (m_data.Type)
							{
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_OCCLUSION:
								{
									const UINT64& t = *reinterpret_cast<UINT64*>(tmpData);
									m_data.Occlusion = t > 0;
									break;
								}
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_OCCLUSION_COUNT:
								{
									const UINT64& t = *reinterpret_cast<UINT64*>(tmpData);
									m_data.OcclusionCount = static_cast<uint64_t>(t);
									break;
								}
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_TIMESTAMP:
								{
									const UINT64& t = *reinterpret_cast<UINT64*>(tmpData);
									m_data.Timestamp = static_cast<uint64_t>(t);
									break;
								}
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_PIPELINE_STATISTICS:
								{
									const D3D12_QUERY_DATA_PIPELINE_STATISTICS& t = *reinterpret_cast<D3D12_QUERY_DATA_PIPELINE_STATISTICS*>(tmpData);
									m_data.PipelineStatistics.CS.Invocations = t.CSInvocations;
									m_data.PipelineStatistics.DS.Invocations = t.DSInvocations;
									m_data.PipelineStatistics.GS.Invocations = t.GSInvocations;
									m_data.PipelineStatistics.GS.PrimitiveCount = t.GSPrimitives;
									m_data.PipelineStatistics.HS.Invocations = t.HSInvocations;
									m_data.PipelineStatistics.PS.Invocations = t.PSInvocations;
									m_data.PipelineStatistics.VS.Invocations = t.VSInvocations;
									m_data.PipelineStatistics.VS.PrimitiveCount = t.IAPrimitives;
									m_data.PipelineStatistics.VS.VertexCount = t.IAVertices;
									m_data.PipelineStatistics.Rasterizer.Invocations = t.CInvocations;
									m_data.PipelineStatistics.Rasterizer.PrimitiveCount = t.CPrimitives;
									break;
								}
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_STREAM_OUTPUT_ALL:
								{
									const D3D12_QUERY_DATA_SO_STATISTICS* t = reinterpret_cast<D3D12_QUERY_DATA_SO_STATISTICS*>(tmpData);
									for (size_t a = 0; a < 4; a++)
									{
										m_data.StreamOutput[a].OverflowSize = t[a].PrimitivesStorageNeeded;
										m_data.StreamOutput[a].WrittenPrimitives = t[a].NumPrimitivesWritten;
									}
									break;
								}
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_STREAM_OUTPUT_0:
								{
									const D3D12_QUERY_DATA_SO_STATISTICS& t = *reinterpret_cast<D3D12_QUERY_DATA_SO_STATISTICS*>(tmpData);
									m_data.StreamOutput[0].OverflowSize = t.PrimitivesStorageNeeded;
									m_data.StreamOutput[0].WrittenPrimitives = t.NumPrimitivesWritten;
									break;
								}
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_STREAM_OUTPUT_1:
								{
									const D3D12_QUERY_DATA_SO_STATISTICS& t = *reinterpret_cast<D3D12_QUERY_DATA_SO_STATISTICS*>(tmpData);
									m_data.StreamOutput[1].OverflowSize = t.PrimitivesStorageNeeded;
									m_data.StreamOutput[1].WrittenPrimitives = t.NumPrimitivesWritten;
									break;
								}
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_STREAM_OUTPUT_2:
								{
									const D3D12_QUERY_DATA_SO_STATISTICS& t = *reinterpret_cast<D3D12_QUERY_DATA_SO_STATISTICS*>(tmpData);
									m_data.StreamOutput[2].OverflowSize = t.PrimitivesStorageNeeded;
									m_data.StreamOutput[2].WrittenPrimitives = t.NumPrimitivesWritten;
									break;
								}
								case API::Rendering::QUERY_TYPE::QUERY_TYPE_STREAM_OUTPUT_3:
								{
									const D3D12_QUERY_DATA_SO_STATISTICS& t = *reinterpret_cast<D3D12_QUERY_DATA_SO_STATISTICS*>(tmpData);
									m_data.StreamOutput[3].OverflowSize = t.PrimitivesStorageNeeded;
									m_data.StreamOutput[3].WrittenPrimitives = t.NumPrimitivesWritten;
									break;
								}
								default:
									break;
							}
						}
						return m_data;
					}

					void CLOAK_CALL_THIS Query::OnBegin(In ID3D12GraphicsCommandList* cmdList, In Impl::Rendering::IAdapter* queue)
					{
						API::Helper::Lock lock(m_sync);
						if (CloakDebugCheckOK(m_data.Type != API::Rendering::QUERY_TYPE_TIMESTAMP, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false))
						{
							m_page->RegisterUsage(queue);
							m_beginCount++;
							for (size_t a = 0; a < NUM_QUERIES[static_cast<size_t>(m_data.Type)]; a++)
							{
								cmdList->BeginQuery(m_page->GetHeap(), static_cast<D3D12_QUERY_TYPE>(static_cast<size_t>(Casting::CastForward(m_data.Type)) + a), static_cast<UINT>((m_index / NUM_QUERIES[static_cast<size_t>(m_data.Type)]) + a));
							}
						}
					}
					void CLOAK_CALL_THIS Query::OnEnd(In ID3D12GraphicsCommandList* cmdList, In Impl::Rendering::IAdapter* queue)
					{
						API::Helper::Lock lock(m_sync);
						m_usageCount++;
						if (m_data.Type != API::Rendering::QUERY_TYPE_TIMESTAMP)
						{
							if (CloakDebugCheckOK(m_beginCount > 0, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false))
							{
								m_beginCount--;
							}
						}
						else
						{
							m_page->RegisterUsage(queue);
						}
						for (size_t a = 0; a < NUM_QUERIES[static_cast<size_t>(m_data.Type)]; a++)
						{
							cmdList->EndQuery(m_page->GetHeap(), static_cast<D3D12_QUERY_TYPE>(static_cast<size_t>(Casting::CastForward(m_data.Type)) + a), static_cast<UINT>((m_index / NUM_QUERIES[static_cast<size_t>(m_data.Type)]) + a));
						}
					}
					void CLOAK_CALL_THIS Query::Clear(In uint64_t fence)
					{
						API::Helper::Lock lock(m_sync);
						CLOAK_ASSUME(m_usageCount > 0);
						CLOAK_ASSUME(m_beginCount == 0);
						m_usageCount--;
						m_maxFence = max(m_maxFence, fence);
						m_page->UnregisterUsage(fence);
						if (m_usageCount == 0)
						{
							m_page->Clear(m_index, m_maxFence);
							m_maxFence = 0;
						}
					}

					Success(return == true) bool CLOAK_CALL_THIS Query::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, Query, Query_v1, SavePtr);
					}
				}
			}
		}
	}
}