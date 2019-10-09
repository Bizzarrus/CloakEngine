#include "stdafx.h"
#include "Implementation/Global/Task.h"
#include "Implementation/Rendering/DX12/Queue.h"
#include "Implementation/Rendering/DX12/Context.h"
#include "Implementation/Rendering/DX12/StructuredBuffer.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Engine/WindowHandler.h"

//#define NO_COPY_QUEUE

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace Queue_v1 {
					constexpr DWORD MAX_WAIT = 125;
					constexpr size_t DYNALLOC_PAGE_SIZE[] = { 64 KB, 2 MB };
					constexpr size_t READBACK_PAGE_SIZE = 2 MB;


					//TODO: It seems that laptop dedicated gpu can't handle COPY queue corectly (or illegal command on queue?). Crashes on execute command list on copy queues.
					CLOAK_CALL Queue::Queue(In Device* device, In UINT node, In size_t id, In IManager* manager, In API::Rendering::CROSS_ADAPTER_SUPPORT support) 
						: m_graphicQueue(this, QUEUE_TYPE_DIRECT, device, node, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, manager, support), 
						m_copyQueue(this, QUEUE_TYPE_COPY, device, node, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, manager, support), 
						m_node(node), m_device(device), m_nodeID(id), m_queryPool(this)
					{
						for (size_t a = 0; a < HEAP_NUM_TYPES; a++)
						{
							m_descHeaps[a] = new DescriptorHeap(static_cast<HEAP_TYPE>(a), device, node);
						}
						m_syncFreeAllocPages = nullptr;
						m_syncWaitAllocPages = nullptr;
						m_syncAllocPagePool = nullptr;
						m_syncHeapPagePool = nullptr;
						m_syncHeapPages = nullptr;
						m_syncCustomPagePool = nullptr;
						m_syncReadBackPagePool = nullptr;

						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncFreeAllocPages));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncWaitAllocPages));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncAllocPagePool));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncHeapPagePool));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncHeapPages));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncCustomPagePool));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncReadBackPagePool));
					}
					CLOAK_CALL Queue::~Queue()
					{
						SAVE_RELEASE(m_syncFreeAllocPages);
						SAVE_RELEASE(m_syncWaitAllocPages);
						SAVE_RELEASE(m_syncAllocPagePool);
						SAVE_RELEASE(m_syncHeapPagePool);
						SAVE_RELEASE(m_syncHeapPages);
						SAVE_RELEASE(m_syncCustomPagePool);
						SAVE_RELEASE(m_syncReadBackPagePool);
						for (size_t a = 0; a < m_dynHeapPagePool.size(); a++) { SAVE_RELEASE(m_dynHeapPagePool[a]); }
						for (size_t a = 0; a < m_dynAllocPagePool.size(); a++) { SAVE_RELEASE(m_dynAllocPagePool[a]); }
						for (size_t a = 0; a < m_readBackPagePool.size(); a++) { SAVE_RELEASE(m_readBackPagePool[a]); }
						while (m_customPages.empty() == false) 
						{
							IDynamicAllocatorPage* a = m_customPages.front();
							m_customPages.pop();
							SAVE_RELEASE(a);
						}
						for (size_t a = 0; a < HEAP_NUM_TYPES; a++) { SAVE_RELEASE(m_descHeaps[a]); }
					}
					void CLOAK_CALL_THIS Queue::Shutdown()
					{
						for (size_t a = 0; a < HEAP_NUM_TYPES; a++) { m_descHeaps[a]->Shutdown(); }
					}
					ID3D12CommandQueue* CLOAK_CALL_THIS Queue::GetCommandQueue(In QUEUE_TYPE type)
					{
						switch (type)
						{
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_COPY:
#ifndef NO_COPY_QUEUE
								return m_copyQueue.m_queue;
#endif
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_DIRECT:
								return m_graphicQueue.m_queue;
							default:
								return nullptr;
						}
					}

					uint64_t CLOAK_CALL_THIS Queue::IncrementFence(In QUEUE_TYPE type)
					{
						switch (type)
						{
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_COPY:
#ifndef NO_COPY_QUEUE
								return m_copyQueue.IncrementFence();
#endif
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_DIRECT:
								return m_graphicQueue.IncrementFence();					
							default:
								return 0;
						}
					}
					bool CLOAK_CALL_THIS Queue::IsFenceComplete(In QUEUE_TYPE type, In uint64_t val)
					{
						switch (type)
						{
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_COPY:
#ifndef NO_COPY_QUEUE
								return m_copyQueue.IsFenceComplete(val);
#endif
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_DIRECT:
								return m_graphicQueue.IsFenceComplete(val);
							default:
								break;
						}
						return true;
					}
					bool CLOAK_CALL_THIS Queue::WaitForFence(In QUEUE_TYPE type, In uint64_t val)
					{
						switch (type)
						{
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_COPY:
#ifndef NO_COPY_QUEUE
								return m_copyQueue.WaitForFence(val);
#endif
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_DIRECT:
								return m_graphicQueue.WaitForFence(val);
							default:
								break;
						}
						return true;
					}
					void CLOAK_CALL_THIS Queue::CreateContext(In REFIID riid, Outptr void** ptr, In QUEUE_TYPE type)
					{
						*ptr = nullptr;
						switch (type)
						{
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_COPY:
#ifndef NO_COPY_QUEUE
								m_copyQueue.CreateContext(riid, ptr);
								break;
#endif
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_DIRECT:
								m_graphicQueue.CreateContext(riid, ptr);
								break;
							default:
								break;
						}
					}
					uint64_t CLOAK_CALL_THIS Queue::FlushExecutions(In API::Global::Memory::PageStackAllocator* alloc, In bool wait, In QUEUE_TYPE type)
					{
						switch (type)
						{
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_COPY:
#ifndef NO_COPY_QUEUE
								return m_copyQueue.FlushExecutions(alloc, wait);
#endif
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_DIRECT:
								return m_graphicQueue.FlushExecutions(alloc, wait);
							default:
								return 0;
						}
					}
					UINT CLOAK_CALL_THIS Queue::GetNodeMask() const { return m_node; }
					size_t CLOAK_CALL_THIS Queue::GetNodeID() const { return m_nodeID; }
					ISingleQueue* CLOAK_CALL_THIS Queue::GetSingleQueue(In QUEUE_TYPE type)
					{
						switch (type)
						{
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_COPY:
#ifndef NO_COPY_QUEUE
								return &m_copyQueue;
#endif
							case CloakEngine::Impl::Rendering::QUEUE_TYPE_DIRECT:
								return &m_graphicQueue;
							
							default:
								return nullptr;
						}
					}
					IDynamicAllocatorPage* CLOAK_CALL_THIS Queue::RequestDynamicAllocatorPage(In DynamicAllocatorType type)
					{
						const size_t t = static_cast<size_t>(type);
						CLOAK_ASSUME(t < ARRAYSIZE(m_freeDynAllocPages));
						API::Helper::Lock lock(m_syncCustomPagePool);
						while (m_customPages.empty() == false)
						{
							IDynamicAllocatorPage* p = m_customPages.front();
							if (p->IsAviable()) { SAVE_RELEASE(p); m_customPages.pop(); }
							else { break; }
						}
						lock.unlock();
						lock.lock(m_syncFreeAllocPages);
						if (m_freeDynAllocPages[t].empty() == false)
						{
							IDynamicAllocatorPage* res = m_freeDynAllocPages[t].front();
							m_freeDynAllocPages[t].pop();
							return res;
						}
						lock.unlock();
						lock.lock(m_syncWaitAllocPages);
						if (m_waitDynAllocPages[t].empty() == false)
						{
							IDynamicAllocatorPage* res = m_waitDynAllocPages[t].front();
							if (res->IsAviable()) { m_waitDynAllocPages[t].pop(); return res; }
						}
						lock.unlock();

						CloakDebugLog("Create Dynamic Allocator Page");

						D3D12_HEAP_PROPERTIES props;
						props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						props.CreationNodeMask = m_node;
						props.VisibleNodeMask = m_node;

						D3D12_RESOURCE_DESC desc;
						desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
						desc.Alignment = 0;
						desc.Height = 1;
						desc.Width = DYNALLOC_PAGE_SIZE[t];
						desc.DepthOrArraySize = 1;
						desc.MipLevels = 1;
						desc.Format = DXGI_FORMAT_UNKNOWN;
						desc.SampleDesc.Count = 1;
						desc.SampleDesc.Quality = 0;
						desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

						API::Rendering::RESOURCE_STATE state;
						bool canTransition = true;
						switch (type)
						{
							case CloakEngine::Impl::Rendering::Allocator_v1::DynamicAllocatorType::GPU:
								props.Type = D3D12_HEAP_TYPE_DEFAULT;
								desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
								state = API::Rendering::STATE_UNORDERED_ACCESS;
								break;
							case CloakEngine::Impl::Rendering::Allocator_v1::DynamicAllocatorType::CPU:
								props.Type = D3D12_HEAP_TYPE_UPLOAD;
								desc.Flags = D3D12_RESOURCE_FLAG_NONE;
								state = API::Rendering::STATE_GENERIC_READ;
								canTransition = false;
								break;
							default:
								assert(false);
						}
						ID3D12Resource* buffer = nullptr;
						HRESULT hRet = m_device->CreateCommittedResource(props, D3D12_HEAP_FLAG_NONE, desc, Casting::CastForward(state), nullptr, CE_QUERY_ARGS(&buffer));
						if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_RESSOURCES, true)) { return nullptr; }
						IDynamicAllocatorPage* res = new DynamicAllocatorPage(buffer, type, this, state, canTransition, DYNALLOC_PAGE_SIZE[t]);
						lock.lock(m_syncAllocPagePool);
						m_dynAllocPagePool.push_back(res);
						RELEASE(buffer);
						return res;
					}
					void CLOAK_CALL_THIS Queue::CleanDynamicAllocatorPage(In IDynamicAllocatorPage* page, In bool aviable, In DynamicAllocatorType type)
					{
						CLOAK_ASSUME(static_cast<size_t>(type) < ARRAYSIZE(m_freeDynAllocPages) && static_cast<size_t>(type) < ARRAYSIZE(m_waitDynAllocPages));
						if (aviable)
						{
							API::Helper::Lock lock(m_syncFreeAllocPages);
							m_freeDynAllocPages[static_cast<size_t>(type)].push(page);
						}
						else
						{
							API::Helper::Lock lock(m_syncWaitAllocPages);
							m_waitDynAllocPages[static_cast<size_t>(type)].push(page);
						}
					}
					IDynamicAllocatorPage* CLOAK_CALL_THIS Queue::RequestCustomDynamicAllocatorPage(In DynamicAllocatorType type, In size_t size)
					{
						API::Helper::Lock lock(m_syncCustomPagePool);
						while (m_customPages.empty() == false)
						{
							IDynamicAllocatorPage* p = m_customPages.front();
							if (p->IsAviable()) { SAVE_RELEASE(p); m_customPages.pop(); }
							else { break; }
						}
						lock.unlock();

						CloakDebugLog("Create Custom Dynamic Allocator Page");

						D3D12_HEAP_PROPERTIES props;
						props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						props.CreationNodeMask = m_node;
						props.VisibleNodeMask = m_node;

						D3D12_RESOURCE_DESC desc;
						desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
						desc.Alignment = 0;
						desc.Height = 1;
						desc.Width = size;
						desc.DepthOrArraySize = 1;
						desc.MipLevels = 1;
						desc.Format = DXGI_FORMAT_UNKNOWN;
						desc.SampleDesc.Count = 1;
						desc.SampleDesc.Quality = 0;
						desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

						API::Rendering::RESOURCE_STATE state;
						bool canTransition = true;
						switch (type)
						{
							case CloakEngine::Impl::Rendering::Allocator_v1::DynamicAllocatorType::GPU:
								props.Type = D3D12_HEAP_TYPE_DEFAULT;
								desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
								state = API::Rendering::STATE_UNORDERED_ACCESS;
								break;
							case CloakEngine::Impl::Rendering::Allocator_v1::DynamicAllocatorType::CPU:
								props.Type = D3D12_HEAP_TYPE_UPLOAD;
								desc.Flags = D3D12_RESOURCE_FLAG_NONE;
								state = API::Rendering::STATE_GENERIC_READ;
								canTransition = false;
								break;
							default:
								assert(false);
						}
						ID3D12Resource* buffer = nullptr;
						HRESULT hRet = m_device->CreateCommittedResource(props, D3D12_HEAP_FLAG_NONE, desc, Casting::CastForward(state), nullptr, CE_QUERY_ARGS(&buffer));
						if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_RESSOURCES, true)) { return nullptr; }
						IDynamicAllocatorPage* res = new CustomDynamicAllocatorPage(buffer, type, this, state, canTransition, size);
						RELEASE(buffer);
						return res;
					}
					void CLOAK_CALL_THIS Queue::CleanCustomDynamicAllocatorPage(In IDynamicAllocatorPage* page)
					{
						API::Helper::Lock lock(m_syncCustomPagePool);
						m_customPages.push(page);
					}
					IDynamicDescriptorHeapPage* CLOAK_CALL_THIS Queue::RequestDynamicDescriptorHeapPage()
					{
						API::Helper::Lock lock(m_syncHeapPages);
						if (m_dynHeapPages.empty() == false)
						{
							IDynamicDescriptorHeapPage* p = m_dynHeapPages.front();
							if (p->IsAviable()) { m_dynHeapPages.pop(); return p; }
						}
						lock.unlock();
						lock.lock(m_syncHeapPagePool);

						CloakDebugLog("Create Dynamic Descriptor Heap");

						DynamicDescriptorHeapPage* p = new DynamicDescriptorHeapPage(m_device, this);
						m_dynHeapPagePool.push_back(p);
						return p;
					}
					void CLOAK_CALL_THIS Queue::CleanDynamicDescriptorHeapPage(In IDynamicDescriptorHeapPage* page)
					{
						API::Helper::Lock lock(m_syncHeapPages);
						m_dynHeapPages.push(page);
					}
					IDescriptorHeap* CLOAK_CALL_THIS Queue::GetDescriptorHeap(In HEAP_TYPE heapType) const { return m_descHeaps[heapType]; }
					size_t CLOAK_CALL_THIS Queue::GetDynamicAllocatorPageSize(In DynamicAllocatorType type) const 
					{
						const size_t t = static_cast<size_t>(type);
						return DYNALLOC_PAGE_SIZE[t]; 
					}
					IReadBackPage* CLOAK_CALL_THIS Queue::RequestReadBackPage(In size_t AlignedSize)
					{
						API::Helper::Lock lock(m_syncReadBackPagePool);
						if (AlignedSize > READBACK_PAGE_SIZE)
						{
							CloakDebugLog("Create Custom Readback Page");

							D3D12_HEAP_PROPERTIES props;
							props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
							props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
							props.CreationNodeMask = m_node;
							props.VisibleNodeMask = m_node;
							props.Type = D3D12_HEAP_TYPE_READBACK;

							D3D12_RESOURCE_DESC desc;
							desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
							desc.Alignment = 0;
							desc.Height = 1;
							desc.Width = AlignedSize;
							desc.DepthOrArraySize = 1;
							desc.MipLevels = 1;
							desc.Format = DXGI_FORMAT_UNKNOWN;
							desc.SampleDesc.Count = 1;
							desc.SampleDesc.Quality = 0;
							desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
							desc.Flags = D3D12_RESOURCE_FLAG_NONE;

							ID3D12Resource* buffer = nullptr;
							HRESULT hRet = m_device->CreateCommittedResource(props, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, CE_QUERY_ARGS(&buffer));
							if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_RESSOURCES, true)) { return nullptr; }
							IReadBackPage* res = new CustomReadBackPage(buffer, AlignedSize, this);
							RELEASE(buffer);
							return res;
						}
						else
						{
							for (size_t a = 0; a < m_readBackPagePool.size(); a++)
							{
								IReadBackPage* p = m_readBackPagePool[a];
								if (p->GetFreeSize() >= AlignedSize) 
								{
									m_readBackPagePool[a] = m_readBackPagePool[m_readBackPagePool.size() - 1];
									m_readBackPagePool.pop_back();
									return p;
								}
							}

							CloakDebugLog("Create Readback Page");

							D3D12_HEAP_PROPERTIES props;
							props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
							props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
							props.CreationNodeMask = m_node;
							props.VisibleNodeMask = m_node;
							props.Type = D3D12_HEAP_TYPE_READBACK;

							D3D12_RESOURCE_DESC desc;
							desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
							desc.Alignment = 0;
							desc.Height = 1;
							desc.Width = READBACK_PAGE_SIZE;
							desc.DepthOrArraySize = 1;
							desc.MipLevels = 1;
							desc.Format = DXGI_FORMAT_UNKNOWN;
							desc.SampleDesc.Count = 1;
							desc.SampleDesc.Quality = 0;
							desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
							desc.Flags = D3D12_RESOURCE_FLAG_NONE;

							ID3D12Resource* buffer = nullptr;
							HRESULT hRet = m_device->CreateCommittedResource(props, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, CE_QUERY_ARGS(&buffer));
							if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_RESSOURCES, true)) { return nullptr; }
							IReadBackPage* res = new ReadBackPage(buffer, READBACK_PAGE_SIZE, this);
							RELEASE(buffer);
							return res;
						}
					}
					void CLOAK_CALL_THIS Queue::CleanReadBackPage(In IReadBackPage* page)
					{
						API::Helper::Lock lock(m_syncReadBackPagePool);
						m_readBackPagePool.push_back(page);
						page->AddRef();
					}
					void CLOAK_CALL_THIS Queue::Update()
					{
#ifndef NO_COPY_QUEUE
						m_copyQueue.Update();
#endif
						m_graphicQueue.Update();
					}
					IDevice* CLOAK_CALL_THIS Queue::GetDevice() const
					{
						m_device->AddRef();
						return m_device;
					}

					QueryPool* CLOAK_CALL_THIS Queue::GetQueryPool() { return &m_queryPool; }

					CLOAK_CALL SingleQueue::SingleQueue(In Queue* parent, In QUEUE_TYPE qtype, In Device* device, In UINT node, In D3D12_COMMAND_LIST_TYPE type, In D3D12_COMMAND_QUEUE_PRIORITY prio, In IManager* manager, In API::Rendering::CROSS_ADAPTER_SUPPORT support) : m_type(type), m_node(node), m_device(device), m_manager(manager), m_parent(parent), m_qtype(qtype)
					{
						m_supportedListVersion = static_cast<LIST_SUPPORT>(~LIST_V0);
						HRESULT hRet = S_OK;
						m_fenceEvent = INVALID_HANDLE_VALUE;
						m_syncContext = nullptr;
						m_syncFence = nullptr;
						m_syncExecute = nullptr;
						m_syncAlloc = nullptr;
						m_syncEvent = nullptr;
						m_syncFreeAlloc = nullptr;
						m_syncToRelease = nullptr;
						m_fence = nullptr;
						m_queue = nullptr;

						D3D12_COMMAND_QUEUE_DESC desc;
						desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
						desc.NodeMask = node;
						desc.Priority = prio;
						desc.Type = type;
						hRet = device->CreateCommandQueue(desc, CE_QUERY_ARGS(&m_queue));
						CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_QUEUE, true);

						m_lastFenceCount = 0;
						m_nextFenceCount = m_lastFenceCount + 1;
						D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_SHARED;
						if (support != API::Rendering::CROSS_ADAPTER_NOT_SUPPORTED) { flags |= D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER; }
						hRet = device->CreateFence(m_lastFenceCount, flags, CE_QUERY_ARGS(&m_fence));
						CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_FENCE, true);

						m_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
						CloakCheckError(m_fenceEvent != INVALID_HANDLE_VALUE && m_fenceEvent != NULL, API::Global::Debug::Error::GRAPHIC_FENCE, true);

						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncContext));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncFence));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncExecute));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncAlloc));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncEvent));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncFreeAlloc));
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncToRelease));
					}
					CLOAK_CALL SingleQueue::~SingleQueue()
					{
						SAVE_RELEASE(m_syncFence);
						SAVE_RELEASE(m_syncContext);
						SAVE_RELEASE(m_syncExecute);
						SAVE_RELEASE(m_syncAlloc);
						SAVE_RELEASE(m_syncEvent);
						SAVE_RELEASE(m_syncFreeAlloc);
						SAVE_RELEASE(m_syncToRelease);
						if (m_fenceEvent != INVALID_HANDLE_VALUE) { CloseHandle(m_fenceEvent); }
						RELEASE(m_fence);
						RELEASE(m_queue);
						for (size_t a = 0; a < m_contextPool.size(); a++) { SAVE_RELEASE(m_contextPool[a]); }
						for (size_t a = 0; a < m_allocPool.size(); a++) { m_allocPool[a]->Release(); }
						for (const auto& a : m_toRelease) { SAVE_RELEASE(a.second); }
					}

					QUEUE_TYPE CLOAK_CALL_THIS SingleQueue::GetType() const { return m_qtype; }
					IQueue* CLOAK_CALL_THIS SingleQueue::GetParent() const { return m_parent; }
					size_t CLOAK_CALL_THIS SingleQueue::GetNodeID() const { return m_parent->GetNodeID(); }
					UINT CLOAK_CALL_THIS SingleQueue::GetNodeMask() const { return m_parent->GetNodeMask(); }

					bool CLOAK_CALL_THIS SingleQueue::IsFenceComplete(In uint64_t val)
					{
						API::Helper::ReadLock lock(m_syncFence);
						if (val > m_lastFenceCount)
						{
							API::Helper::WriteLock wlock(m_syncFence);
							const uint64_t v = m_fence->GetCompletedValue();
							m_lastFenceCount = max(m_lastFenceCount, v);
						}
						return val <= m_lastFenceCount;
					}
					bool CLOAK_CALL_THIS SingleQueue::WaitForFence(In uint64_t val)
					{
						API::Helper::Lock elock(m_syncEvent);
						API::Helper::Lock lock(m_syncFence);
						if (IsFenceComplete(val) == false)
						{
							m_fence->SetEventOnCompletion(val, m_fenceEvent);
							lock.unlock();
							Impl::Global::Task::IHelpWorker* worker = Impl::Global::Task::GetCurrentWorker();
							if (worker != nullptr)
							{
								worker->HelpWorkUntil([val, this]() { return IsFenceComplete(val); }, API::Global::Threading::Flag::None);
							}
							else
							{
								DWORD e = WAIT_TIMEOUT;
								do {
									e = WaitForSingleObject(m_fenceEvent, MAX_WAIT);
								} while (IsFenceComplete(val) == false && e == WAIT_TIMEOUT);
							}
						}
						return true;
					}
					void CLOAK_CALL_THIS SingleQueue::CreateContext(In REFIID riid, Outptr void** ptr)
					{
						API::Helper::Lock lock(m_syncContext);
						Impl::Rendering::ICopyContext* con = nullptr;
						if (m_aviableContext.empty())
						{
							Impl::Rendering::ICopyContext* c = nullptr;
							if (m_type == D3D12_COMMAND_LIST_TYPE_COPY) { c = new CopyContext(this, m_manager); }
							else { c = new Context(this, m_manager); }
							con = c;
							m_contextPool.push_back(con);
						}
						else
						{
							con = m_aviableContext.front();
							m_aviableContext.pop();
						}
						con->Reset();
						HRESULT hRet = con->QueryInterface(riid, ptr);
						assert(SUCCEEDED(hRet));
						CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true);
					}
					API::Rendering::IVertexBuffer* CLOAK_CALL_THIS SingleQueue::CreateVertexBuffer(In UINT vertexCount, In UINT vertexSize, In_reads(vertexCount*vertexSize) const void* data) const
					{
						return VertexBuffer::Create(m_parent->GetNodeMask(), m_parent->GetNodeID() - 1, vertexCount, vertexSize, data);
					}
					API::Rendering::IIndexBuffer* CLOAK_CALL_THIS SingleQueue::CreateIndexBuffer(In UINT size, In_reads(size) const uint16_t* data) const
					{
						return IndexBuffer::Create(m_parent->GetNodeMask(), m_parent->GetNodeID() - 1, size, false, data);
					}
					API::Rendering::IIndexBuffer* CLOAK_CALL_THIS SingleQueue::CreateIndexBuffer(In UINT size, In_reads(size) const uint32_t* data) const
					{
						return IndexBuffer::Create(m_parent->GetNodeMask(), m_parent->GetNodeID() - 1, size, true, data);
					}
					API::Rendering::IConstantBuffer* CLOAK_CALL_THIS SingleQueue::CreateConstantBuffer(In UINT elementSize, In_reads_opt(elementSize) const void* data) const
					{
						return ConstantBuffer::Create(m_parent->GetNodeMask(), m_parent->GetNodeID() - 1, elementSize, data);
					}
					API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS SingleQueue::CreateStructuredBuffer(In UINT elementCount, In UINT elementSize, In_reads(elementSize) const void* data, In_opt bool counterBuffer) const
					{
						return StructuredBuffer::Create(m_parent->GetNodeMask(), m_parent->GetNodeID() - 1, counterBuffer, elementCount, elementSize, data);
					}
					API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS SingleQueue::CreateStructuredBuffer(In UINT elementCount, In UINT elementSize, In_opt bool counterBuffer, In_reads_opt(elementSize) const void* data) const
					{
						return StructuredBuffer::Create(m_parent->GetNodeMask(), m_parent->GetNodeID() - 1, counterBuffer, elementCount, elementSize, data);
					}
					void CLOAK_CALL_THIS SingleQueue::ReleaseOnFence(In API::Helper::ISavePtr* ptr, In uint64_t fence)
					{
						API::Helper::Lock lock(m_syncToRelease);
						m_toRelease.push(std::make_pair(fence, ptr));
					}
					void CLOAK_CALL_THIS SingleQueue::FreeContext(In Impl::Rendering::ICopyContext* ptr)
					{
						API::Helper::Lock lock(m_syncContext);
						m_aviableContext.push(ptr);
					}
					
					void CLOAK_CALL_THIS SingleQueue::ExecuteListDeffered(In const GraphicsCommandList& cmdList, Impl::Rendering::ICopyContext* context)
					{
						API::Helper::Lock lock(m_syncExecute);
						m_toExecute.push_back(std::make_pair(cmdList.V0.Get(), context));
					}
#define CREATE_AND_POPULATE(Name, Prev) if(SUCCEEDED(hRet) && list->Prev != nullptr) { list->Name = list->Prev; } else { list->Prev = nullptr; hRet = m_device->CreateCommandList(m_node, m_type, *alloc, CE_QUERY_ARGS(&list->Name)); ns = LIST_##Name; }
					void CLOAK_CALL_THIS SingleQueue::CreateCommandList(Out GraphicsCommandList* list, Outptr ID3D12CommandAllocator** alloc)
					{
						*alloc = RequestAllocator();
						HRESULT hRet = E_NOINTERFACE;
						LIST_SUPPORT os = m_supportedListVersion.load();
						LIST_SUPPORT ns = os;
						switch (os)
						{
							default:
							case LIST_V5:
								CREATE_AND_POPULATE(V5, V5); //Highest supported version, so Name == Prev
								//Fall through
							case LIST_V4:
								CREATE_AND_POPULATE(V4, V5);
								//Fall through
							case LIST_V3:
								CREATE_AND_POPULATE(V3, V4);
								//Fall through
							case LIST_V2:
								CREATE_AND_POPULATE(V2, V3);
								//Fall through
							case LIST_V1:
								CREATE_AND_POPULATE(V1, V2);
								//Fall through
							case LIST_V0:
								CREATE_AND_POPULATE(V0, V1);
								break;
						}
						m_supportedListVersion.compare_exchange_strong(os, ns);
						CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_NO_LIST, true);
					}
#undef CREATE_AND_POPULATE
					void CLOAK_CALL_THIS SingleQueue::DiscardAllocator(In ID3D12CommandAllocator* alloc, In uint64_t fence)
					{
						API::Helper::Lock lock(m_syncAlloc);
						m_waitAlloc.push(std::make_pair(fence, alloc));
					}
					void CLOAK_CALL_THIS SingleQueue::Update()
					{
						API::Helper::Lock lock(m_syncFence);
						const uint64_t v = m_fence->GetCompletedValue();
						lock.lock(m_syncToRelease);
						while (m_toRelease.empty() == false)
						{
							const auto a = m_toRelease.front();
							if (a.first <= v) { SAVE_RELEASE(a.second); m_toRelease.pop(); }
							else { break; }
						}
					}
					void CLOAK_CALL_THIS SingleQueue::WaitForAdapter(In IAdapter* q, In uint64_t fence)
					{
						SingleQueue* sq = dynamic_cast<SingleQueue*>(q);
						if (sq != nullptr)
						{
							API::Helper::Lock lock(sq->m_syncFence);
							if (fence > sq->m_lastFenceCount)
							{
								ID3D12Fence* sf = sq->m_fence;
								sf->AddRef();
								lock.lock(m_syncFence);
								const auto& f = m_lastFenceWait.find(q);
								if (f != m_lastFenceWait.end() && f->second >= fence) { sf->Release(); return; }
								m_lastFenceWait[q] = fence;
								m_queue->Wait(sf, fence);
								sf->Release();
							}
						}
					}

					uint64_t CLOAK_CALL_THIS SingleQueue::IncrementFence()
					{
						API::Helper::Lock lock(m_syncFence);
						const uint64_t lv = m_nextFenceCount;
						m_nextFenceCount++;
						m_queue->Signal(m_fence, lv);
						return lv;
					}
					uint64_t CLOAK_CALL_THIS SingleQueue::ExecuteList(In const GraphicsCommandList& cmdList)
					{
						API::Helper::Lock lock(m_syncFence);
						ID3D12CommandList* L[] = { cmdList.V0.Get() };
						m_queue->ExecuteCommandLists(ARRAYSIZE(L), L);
						return IncrementFence();
					}
					uint64_t CLOAK_CALL_THIS SingleQueue::FlushExecutions(In API::Global::Memory::PageStackAllocator* alloc, In_opt bool wait)
					{
						API::Helper::Lock lock(m_syncExecute);
						const size_t exc = m_toExecute.size();
						if (exc > 0)
						{
							ID3D12CommandList** lists = nullptr;
							if (alloc != nullptr) { lists = reinterpret_cast<ID3D12CommandList**>(alloc->Allocate(sizeof(ID3D12CommandList*) * exc)); }
							else { lists = reinterpret_cast<ID3D12CommandList**>(API::Global::Memory::MemoryPool::Allocate(sizeof(ID3D12CommandList*)*exc)); }
							CLOAK_ASSUME(lists != nullptr);
							for (size_t a = 0; a < exc; a++) { lists[a] = m_toExecute[a].first; }
							API::Helper::Lock flock(m_syncFence);
							m_queue->ExecuteCommandLists(static_cast<UINT>(exc), lists);
							const uint64_t r = IncrementFence();
							flock.unlock();
							if (alloc != nullptr) { alloc->Free(lists); }
							else { API::Global::Memory::MemoryPool::Free(lists); }
							for (size_t a = 0; a < exc; a++)
							{
								m_toExecute[a].second->CleanAndDiscard(r);
								FreeContext(m_toExecute[a].second);
							}
							m_toExecute.clear();
							lock.unlock();
							if (wait) { WaitForFence(r); }
							return r;
						}
						lock.unlock();
						lock.lock(m_syncFence);
						return m_lastFenceCount;
					}
					ID3D12CommandAllocator* CLOAK_CALL_THIS SingleQueue::RequestAllocator()
					{
						API::Helper::Lock lock(m_syncFreeAlloc);
						if (m_freeAlloc.empty())
						{
							API::Helper::Lock wlock(m_syncAlloc);
							API::Helper::Lock flock(m_syncFence);
							bool s = m_waitAlloc.empty() == false;
							uint64_t v = m_fence->GetCompletedValue();
							const uint64_t fv = m_lastFenceCount = max(m_lastFenceCount, v);
							flock.unlock();
							while (s)
							{
								const auto& cur = m_waitAlloc.front();
								if (cur.first <= fv)
								{
									m_freeAlloc.push(cur.second);
									m_waitAlloc.pop();
									s = m_waitAlloc.empty() == false;
								}
								else { s = false; }
							}
						}
						if (m_freeAlloc.empty() == false)
						{
							ID3D12CommandAllocator* alloc = m_freeAlloc.front();
							m_freeAlloc.pop();
							HRESULT hRet = alloc->Reset();
							CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_RESET, true);
							return alloc;
						}

						CloakDebugLog("Create Command Allocator");

						ID3D12CommandAllocator* alloc = nullptr;
						HRESULT hRet = m_device->CreateCommandAllocator(m_type, CE_QUERY_ARGS(&alloc));
						if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_ALLOCATOR, true)) { return nullptr; }
						m_allocPool.push_back(alloc);
						return alloc;
					}
					DescriptorHeap* CLOAK_CALL_THIS SingleQueue::GetDescriptorHeap(In HEAP_TYPE heapType) const { return m_parent->m_descHeaps[heapType]; }
				}
			}
		}
	}
}