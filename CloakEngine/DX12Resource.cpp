#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/Resource.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/BasicBuffer.h"
#include "Implementation/Rendering/DX12/Allocator.h"
#include "Implementation/Rendering/Context.h"

#include "Engine/Graphic/Core.h"

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/StringConvert.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Resource_v1 {
					constexpr API::Rendering::VIEW_TYPE HEAP_VIEW_TYPES[HEAP_NUM_TYPES] = {
						API::Rendering::VIEW_TYPE::CBV | API::Rendering::VIEW_TYPE::SRV | API::Rendering::VIEW_TYPE::UAV | API::Rendering::VIEW_TYPE::CUBE,
						API::Rendering::VIEW_TYPE::NONE,
						API::Rendering::VIEW_TYPE::RTV,
						API::Rendering::VIEW_TYPE::DSV,
						API::Rendering::VIEW_TYPE::CBV_DYNAMIC | API::Rendering::VIEW_TYPE::SRV_DYNAMIC | API::Rendering::VIEW_TYPE::UAV_DYNAMIC | API::Rendering::VIEW_TYPE::CUBE_DYNAMIC,
						API::Rendering::VIEW_TYPE::NONE,
					};
					constexpr API::Rendering::VIEW_TYPE GENERATING_VIEW_TYPES[] = { API::Rendering::VIEW_TYPE::SRV, API::Rendering::VIEW_TYPE::CBV, API::Rendering::VIEW_TYPE::CUBE, API::Rendering::VIEW_TYPE::UAV, API::Rendering::VIEW_TYPE::RTV, API::Rendering::VIEW_TYPE::DSV, API::Rendering::VIEW_TYPE::CBV_DYNAMIC, API::Rendering::VIEW_TYPE::SRV_DYNAMIC, API::Rendering::VIEW_TYPE::UAV_DYNAMIC, API::Rendering::VIEW_TYPE::CUBE_DYNAMIC };

#ifdef _DEBUG
					constexpr bool __SA_CHECK_VIEW_TYPES()
					{
						size_t lp = 0;
						for (size_t a = 0; a < ARRAYSIZE(GENERATING_VIEW_TYPES); a++)
						{
							size_t p = ARRAYSIZE(HEAP_VIEW_TYPES);
							for (size_t b = 0; b < ARRAYSIZE(HEAP_VIEW_TYPES); b++)
							{
								if ((GENERATING_VIEW_TYPES[a] & HEAP_VIEW_TYPES[b]) != API::Rendering::VIEW_TYPE::NONE)
								{
									p = b;
								}
							}
							if (p >= ARRAYSIZE(HEAP_VIEW_TYPES) || p < lp) { return false; }
							lp = p;
						}
						return true;
					}
					static_assert(__SA_CHECK_VIEW_TYPES(), "GENERATING_VIEW_TYPES has wrong order");
#endif

					struct ViewHandleInfo {
						API::Rendering::VIEW_TYPE View;
						PixelBuffer* Pixel;
						GraphicBuffer* Graphic;
						Resource* Base;
					};
					void CLOAK_CALL Resource::GenerateViews(In_reads(bufferSize) Resource* buffer[], In size_t bufferSize, In API::Rendering::VIEW_TYPE Type, In_opt API::Rendering::DescriptorPageType Page)
					{
						size_t HeapSize[ARRAYSIZE(HEAP_VIEW_TYPES)];
						for (size_t a = 0; a < ARRAYSIZE(HeapSize); a++) { HeapSize[a] = 0; }
						size_t* NumHandles[ARRAYSIZE(GENERATING_VIEW_TYPES)];
						size_t NumHandlesSum[ARRAYSIZE(NumHandles)];
						for (size_t a = 0; a < ARRAYSIZE(NumHandles); a++) 
						{ 
							NumHandlesSum[a] = 0;
							NumHandles[a] = NewArray(size_t, bufferSize);
							for (size_t b = 0; b < bufferSize; b++) { NumHandles[a][b] = 0; }
						}
						size_t used = 0;
						size_t nodeID = 0;
						for (size_t a = 0; a < bufferSize; a++)
						{
							if (buffer[a] != nullptr)
							{
								API::Helper::Lock lock(buffer[a]->m_sync);
								if (CloakCheckOK(buffer[a]->m_nodeID != 0 && (nodeID == 0 || buffer[a]->m_nodeID == nodeID), API::Global::Debug::Error::GRAPHIC_WRONG_NODE, false)) { nodeID = buffer[a]->m_nodeID; }
								for (size_t b = 0; b < ARRAYSIZE(GENERATING_VIEW_TYPES); b++) 
								{ 
									NumHandles[b][a] = buffer[a]->GetResourceViewCount(GENERATING_VIEW_TYPES[b]); 
									NumHandlesSum[b] += NumHandles[b][a];
								}
							}
						}
						for (size_t a = 0; a < ARRAYSIZE(HeapSize); a++)
						{
							for (size_t b = 0; b < ARRAYSIZE(GENERATING_VIEW_TYPES); b++)
							{
								if ((HEAP_VIEW_TYPES[a] & GENERATING_VIEW_TYPES[b]) != API::Rendering::VIEW_TYPE::NONE)
								{
									HeapSize[a] += NumHandlesSum[b];
								}
							}
						}
						if (nodeID == 0) { return; }
						IQueue* queue = Engine::Graphic::Core::GetCommandListManager()->GetQueueByNodeID(nodeID - 1);
						size_t p[ARRAYSIZE(GENERATING_VIEW_TYPES)];
						for (size_t a = 0; a < ARRAYSIZE(p); a++)
						{
							p[a] = 0;
							if (a > 0) { p[a] = NumHandlesSum[a - 1]; }
						}
						for (size_t a = 1; a < ARRAYSIZE(p); a++) 
						{ 
							p[a] += p[a - 1]; 
						}
						size_t HeapSizePos[ARRAYSIZE(HeapSize)];
						for (size_t a = 0; a < ARRAYSIZE(HeapSizePos); a++) 
						{ 
							HeapSizePos[a] = 0; 
							if (a > 0) { HeapSizePos[a] = HeapSize[a - 1]; }
						}
						for (size_t a = 1; a < ARRAYSIZE(HeapSizePos); a++) { HeapSizePos[a] += HeapSizePos[a - 1]; }
						API::Rendering::ResourceView* handles = NewArray(API::Rendering::ResourceView, HeapSizePos[ARRAYSIZE(HeapSizePos) - 1] + HeapSize[ARRAYSIZE(HeapSize) - 1]);
						IDescriptorHeap* heaps[ARRAYSIZE(GENERATING_VIEW_TYPES)];
						for (size_t a = 0; a < ARRAYSIZE(HeapSize); a++)
						{
							if (HeapSize[a] > 0)
							{
								IDescriptorHeap* heap = queue->GetDescriptorHeap(static_cast<HEAP_TYPE>(a));
								heap->Allocate(&handles[HeapSizePos[a]], static_cast<UINT>(HeapSize[a]), Page);
								for (size_t b = 0; b < ARRAYSIZE(GENERATING_VIEW_TYPES); b++)
								{
									if ((GENERATING_VIEW_TYPES[b] & HEAP_VIEW_TYPES[a]) != API::Rendering::VIEW_TYPE::NONE)
									{
										heaps[b] = heap;
									}
								}
							}
						}
						for (size_t a = 0; a < bufferSize; a++)
						{
							if (buffer[a] != nullptr)
							{
								API::Helper::Lock lock(buffer[a]->m_sync);
								for (size_t b = 0; b < ARRAYSIZE(GENERATING_VIEW_TYPES); b++)
								{
									const size_t count = NumHandles[b][a];
									CLOAK_ASSUME(count == 0 || p[b] < HeapSizePos[ARRAYSIZE(HeapSizePos) - 1] + HeapSize[ARRAYSIZE(HeapSize) - 1]);
									CLOAK_ASSUME(count <= NumHandlesSum[b]);
									if (count > 0)
									{
										heaps[b]->RegisterUsage(handles[p[b]], buffer[a], count);
										buffer[a]->CreateResourceViews(count, &handles[p[b]], GENERATING_VIEW_TYPES[b]);
									}
									p[b] += count;
								}
							}
						}
						DeleteArray(handles);
						for (size_t a = 0; a < ARRAYSIZE(NumHandles); a++) { DeleteArray(NumHandles[a]); }
					}

					CLOAK_CALL Resource::Resource() : Resource(nullptr, API::Rendering::STATE_COMMON, 0, true) { }
					CLOAK_CALL Resource::Resource(In ID3D12Resource* base, In API::Rendering::RESOURCE_STATE curState, In size_t nodeID, In_opt bool canTransition) : m_canTransition(canTransition), m_gpuHandle(GPU_ADDRESS_UNKNOWN), m_nodeID(nodeID), m_resident(true)
					{
						m_statePromoted = false;
						m_syncUsage = nullptr;
						m_data = base;
						m_transState = RESOURCE_STATE_INVALID;
						m_curState = curState;
						m_lastQueue = nullptr;
						m_lastFence = 0;
						m_usageCount = 0;
						m_usageQueue = nullptr;
						m_canDecay = false;
						if (m_data != nullptr) { m_data->AddRef(); }
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncUsage));
						DEBUG_NAME(Resource);
						if (base != nullptr)
						{
							D3D12_RESOURCE_DESC desc = base->GetDesc();
							iUpdateDecay(desc.Dimension, desc.Flags);
						}
					}
					CLOAK_CALL Resource::~Resource() 
					{ 
						RELEASE(m_data);
						SAVE_RELEASE(m_sync); 
						SAVE_RELEASE(m_syncUsage);
					}
					const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS Resource::GetGPUAddress() const { return m_gpuHandle; }
					void* CLOAK_CALL Resource::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
					void CLOAK_CALL Resource::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

					void CLOAK_CALL_THIS Resource::RegisterUsage(In Impl::Rendering::IAdapter* queue)
					{
						API::Helper::Lock lock(m_syncUsage);
						CLOAK_ASSUME(queue != nullptr);
						if (m_resident == false) 
						{
							CLOAK_ASSUME(m_usageQueue == nullptr);
							MakeResident();
						}
						else
						{
							while (queue != m_usageQueue && m_usageQueue != nullptr)
							{
								lock.unlock();
								lock.lock(m_syncUsage, false);
							}
						}
						CLOAK_ASSUME(queue == m_usageQueue || m_usageQueue == nullptr);
						CLOAK_ASSUME(m_usageQueue != nullptr || m_usageCount == 0);
						if (queue == m_usageQueue) 
						{ 
							m_usageCount++; 
						}
						else if (m_usageQueue == nullptr) 
						{
							AddRef();
							if (m_lastQueue != nullptr)
							{
								//Implicit State Decay:
								if (m_statePromoted == true || m_canDecay == true || m_lastQueue->GetType() == QUEUE_TYPE_COPY)
								{
									m_curState = API::Rendering::STATE_COMMON;
								}
								//Queue switch:
								if (m_lastQueue != queue)
								{
									if (m_curState != API::Rendering::STATE_COMMON)
									{
										API::Rendering::ICopyContext* con = nullptr;
										m_lastQueue->CreateContext(CE_QUERY_ARGS(&con));
										con->TransitionResource(this, API::Rendering::STATE_COMMON);
										const uint64_t f = con->CloseAndExecute();
										con->Release();
										AddRef();
										m_lastQueue->ReleaseOnFence(this, f);
										queue->WaitForAdapter(m_lastQueue, f);
									}
									else
									{
										queue->WaitForAdapter(m_lastQueue, m_lastFence);
									}
								}
							}
							m_usageCount = 1;
							m_usageQueue = queue;
						}
					}
					void CLOAK_CALL_THIS Resource::UnregisterUsage(In uint64_t fence)
					{
						API::Helper::Lock lock(m_syncUsage);
						CLOAK_ASSUME(m_usageCount > 0);
						m_usageCount--;
						if (m_usageCount == 0)
						{
							m_usageQueue->ReleaseOnFence(this, fence);
							m_lastQueue = m_usageQueue;
							m_lastFence = fence;
							m_usageQueue = nullptr;

							//Set event
							m_syncUsage->signalOne();
						}
					}

					void CLOAK_CALL_THIS Resource::MakeResident()
					{
						API::Helper::Lock lock(m_syncUsage);
						if (m_resident == false)
						{
							//TODO: Load into active GPU memory
							//	This operation depends on type of resource:
							//	- Committed resources call Device->MakeResident()
							//	- Placed Resources may be copied from system memory? (Requires resource view regeneration)
							//	- Reserved/Tiled Resources can be remapped from system memory
							m_resident = true;
						}
					}
					void CLOAK_CALL_THIS Resource::Evict()
					{
						API::Helper::Lock lock(m_syncUsage);
						if (m_resident == true)
						{
							LockUsage();
							//TODO: Move outside of active GPU memory
							//	This operation depends on type of resource:
							//	- Commited resources call Device->Evict()
							//	- Placed Resources may be copied to system memory?
							//	- Reserved/Tiled Resources can be remapped to system memory
							m_resident = false;
						}
					}

					Ret_maybenull ID3D12Resource* CLOAK_CALL_THIS Resource::GetResource() { return m_data; }
					Ret_maybenull const ID3D12Resource* CLOAK_CALL_THIS Resource::GetResource() const { return m_data; }
					Ret_notnull API::Helper::ISyncSection* CLOAK_CALL_THIS Resource::GetSyncSection() { return m_sync; }
					void CLOAK_CALL_THIS Resource::SetResourceState(In API::Rendering::RESOURCE_STATE newState, In_opt bool autoPromote)
					{
						API::Helper::Lock lock(m_sync);
						m_curState = newState;
						m_statePromoted = autoPromote;
					}
					void CLOAK_CALL_THIS Resource::SetTransitionResourceState(In API::Rendering::RESOURCE_STATE newState)
					{
						API::Helper::Lock lock(m_sync);
						m_transState = newState;
					}
					bool CLOAK_CALL_THIS Resource::CanTransition() const { return m_canTransition; }
					bool CLOAK_CALL_THIS Resource::SupportStatePromotion() const { return m_canDecay; }
					API::Rendering::RESOURCE_STATE CLOAK_CALL_THIS Resource::GetResourceState() const
					{
						API::Helper::Lock lock(m_sync);
						return m_curState;
					}
					API::Rendering::RESOURCE_STATE CLOAK_CALL_THIS Resource::GetTransitionResourceState() const
					{
						API::Helper::Lock lock(m_sync);
						return m_transState;
					}
					uint8_t CLOAK_CALL_THIS Resource::GetDetailLevel() const { return 0; }
					size_t CLOAK_CALL_THIS Resource::GetNodeID() const { return m_nodeID; }
					Success(return == true) bool CLOAK_CALL_THIS Resource::iQueryInterface(In REFIID riid, Outptr void** ptr) 
					{ 
						RENDERING_QUERY(ptr, riid, Resource, Resource_v1, SavePtr);
					}
					API::Helper::Lock CLOAK_CALL_THIS Resource::LockUsage()
					{
						API::Helper::Lock lock(m_syncUsage);
						while (m_usageQueue != nullptr)
						{
							lock.unlock();
							lock.lock(m_syncUsage, false);
						}
						if (m_lastQueue != nullptr) 
						{ 
							m_lastQueue->WaitForFence(m_lastFence); 
							m_lastQueue = nullptr;
						}
						return lock;
					}
					size_t CLOAK_CALL_THIS Resource::GetResourceViewCount(In API::Rendering::VIEW_TYPE view) { return 0; }
					void CLOAK_CALL_THIS Resource::CreateResourceViews(In size_t count, In_reads(count) API::Rendering::ResourceView* handles, In API::Rendering::VIEW_TYPE) {}
					void CLOAK_CALL_THIS Resource::iUpdateDecay(In D3D12_RESOURCE_DIMENSION dimension, In D3D12_RESOURCE_FLAGS flags)
					{
						m_canDecay = (dimension == D3D12_RESOURCE_DIMENSION_BUFFER || ((flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) != 0 && (dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D || dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D || dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)));
					}
				}
			}
		}
	}
}
#endif