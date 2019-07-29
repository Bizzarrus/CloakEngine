#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/Allocator.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/Context.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/Device.h"
#include "Implementation/Rendering/Context.h"
#include "Implementation/Rendering/Manager.h"

#include "Engine/Graphic/Core.h"

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/SyncSection.h"

#include <atomic>
#include <queue>

//#define DEBUG_SHOW_DESCRIPTORHEAPS
#if defined(_DEBUG) && defined(DEBUG_SHOW_DESCRIPTORHEAPS)
#include <sstream>
#endif

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace Allocator_v1 {
					//																		CBV/SRV/UAV		SAMPLER		RTV		DSV		CBV/SRV/UAV (DYNAMIC)	SAMPLER (DYNAMIC)
					constexpr uint32_t NUM_DESCRIPTORS[Impl::Rendering::HEAP_NUM_TYPES] = {	256,			32,			32,		32,		256,					32 };

					constexpr uint32_t DYN_DESCRIPTORS = 1024;
					constexpr uint32_t DYN_DESCRIPTORS_PER_COPY = 16;
					
					std::atomic<uint32_t> g_descriptorSize = 0;

					inline void CLOAK_CALL CopyData(In void* dst, In void* src, In size_t dstSize, In size_t srcSize, In UINT rowSize, In UINT rowPadding)
					{
						size_t dp = 0;
						size_t sp = 0;
						const UINT rspan = rowSize - rowPadding;
						while (sp + rowSize <= srcSize && dp + rspan <= dstSize)
						{
							void* d = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(dst) + dp);
							void* s = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(src) + sp);
							memcpy(d, s, rspan);
							dp += rspan;
							sp += rowSize;
						}
						size_t rem = dp < dstSize ? dstSize - dp : 0;
						if (rem > 0 && sp < srcSize) 
						{
							const size_t cs = min(srcSize - sp, rem);
							void* d = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(dst) + dp);
							void* s = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(src) + sp);
							memcpy(d, s, cs);
							rem -= cs;
							dp += cs;
						}
						if (rem > 0) 
						{
							void* d = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(dst) + dp);
							memset(d, 0, rem);
						}
					}

					CLOAK_CALL DescriptorHeap::DescriptorView::DescriptorView(In IDevice* dev, In Impl::Rendering::HEAP_TYPE type, In API::Rendering::DescriptorPageType ptype, In UINT node) : PageType(ptype), Assigned(NUM_DESCRIPTORS[static_cast<size_t>(type)]), Size(NUM_DESCRIPTORS[static_cast<size_t>(type)])
					{
						Assignment = reinterpret_cast<Impl::Rendering::IResource**>(API::Global::Memory::MemoryHeap::Allocate(Size * sizeof(Impl::Rendering::IResource*)));
						for (size_t a = 0; a < Size; a++) { Assignment[a] = nullptr; }
						CloakDebugLog("Create Descriptor Heap");
						D3D12_DESCRIPTOR_HEAP_DESC desc;
						desc.Type = Casting::CastForward(type);
						desc.NumDescriptors = static_cast<UINT>(Size);
						desc.Flags = (type == Impl::Rendering::HEAP_CBV_SRV_UAV || type == Impl::Rendering::HEAP_SAMPLER) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
						desc.NodeMask = node;
						Device* dxDev = nullptr;
						if (SUCCEEDED(dev->QueryInterface(CE_QUERY_ARGS(&dxDev))))
						{
							dxDev->CreateDescriptorHeap(desc, CE_QUERY_ARGS(&Heap));
							dxDev->Release();
						}
						Space = static_cast<uint32_t>(Size);
					}
					CLOAK_CALL DescriptorHeap::DescriptorView::~DescriptorView()
					{
						Heap->Release();
						API::Global::Memory::MemoryHeap::Free(Assignment);
					}
					void* CLOAK_CALL DescriptorHeap::DescriptorView::operator new(In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
					void CLOAK_CALL DescriptorHeap::DescriptorView::operator delete(In void* ptr) { return API::Global::Memory::MemoryHeap::Free(ptr); }

					CLOAK_CALL DescriptorHeap::DescriptorHeap(In Impl::Rendering::HEAP_TYPE Type, In IDevice* dev, In UINT node) : m_node(node)
					{
						m_sync = nullptr;
						m_dev = nullptr;
						m_type = Type;
						m_typeSize = 0;
						m_dev = dev;
						m_dev->AddRef();
						m_typeSize = m_dev->GetDescriptorHandleIncrementSize(Casting::CastBackward(Type));
						DEBUG_NAME(DescriptorHeap);
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					}
					CLOAK_CALL DescriptorHeap::~DescriptorHeap()
					{
						Shutdown();
					}
					size_t CLOAK_CALL_THIS DescriptorHeap::Allocate(Inout_updates(numHandles) API::Rendering::ResourceView handles[], In UINT numHandles, In_opt API::Rendering::DescriptorPageType type)
					{
						if (CloakDebugCheckOK(numHandles <= NUM_DESCRIPTORS[m_type], API::Global::Debug::Error::ILLEGAL_ARGUMENT, true))
						{
							API::Helper::Lock lock(m_sync);
							if (m_type != Impl::Rendering::HEAP_CBV_SRV_UAV) { type = API::Rendering::DescriptorPageType::UTILITY; }
							DescriptorView* dv = nullptr;
							size_t hn = 0;
							size_t sp = 0;
							for (size_t a = 0; a < m_descriptors.size(); a++)
							{
								sp = 0;
								DescriptorView* v = m_descriptors[a];
								if (v != nullptr && v->PageType == type && v->Space >= numHandles)
								{
									for (size_t b = 0; b < v->Assigned.size(); b++)
									{
										if (v->Assigned[b] == true) { sp = b + 1; }
										else if (b + 1 - sp >= numHandles)
										{
											dv = v;
											hn = a;
											goto descriptor_heap_alloc_found;
										}
									}
								}
							}
							CLOAK_ASSUME(dv == nullptr);
							m_descriptors.emplace_back(new DescriptorView(m_dev, m_type, type, m_node));
							hn = m_descriptors.size() - 1;
							dv = m_descriptors[hn];
							sp = 0;

						descriptor_heap_alloc_found:
							CLOAK_ASSUME(dv != nullptr);
							const API::Rendering::CPU_DESCRIPTOR_HANDLE begin = Casting::CastBackward(dv->Heap->GetCPUDescriptorHandleForHeapStart());
							const API::Rendering::GPU_DESCRIPTOR_HANDLE gpuBegin = Casting::CastBackward(dv->Heap->GetGPUDescriptorHandleForHeapStart());
							dv->Space -= numHandles;
							const size_t heapID = hn + 1;
							for (size_t a = 0; a < numHandles; a++)
							{
								CLOAK_ASSUME(a + sp < dv->Assigned.size());
								dv->Assigned[a + sp] = true;
								API::Rendering::ResourceView* h = &handles[a];
								h->HeapID = static_cast<uint32_t>(heapID);
								h->NodeMask = m_node;
								h->Position = static_cast<uint32_t>(a + sp);
								h->CPU = begin;
								h->CPU.ptr += (a + sp)*m_typeSize;
								h->GPU = gpuBegin;
								h->GPU.ptr += (static_cast<API::Rendering::GPU_ADDRESS>(a) + sp)*m_typeSize;
							}
#if defined(_DEBUG) && defined(DEBUG_SHOW_DESCRIPTORHEAPS)
							CloakDebugLog("Descriptor Heap[" + std::to_string(m_type) + "] Usage:");
							for (size_t a = 0; a < m_descriptors.size(); a++)
							{
								std::stringstream s;
								s << "\t" << a << " (Type: " << static_cast<size_t>(m_descriptors[a]->PageType) << "): ";
								for (size_t b = 0; b < m_descriptors[a]->Assigned.size(); b++)
								{
									s << "[" << (m_descriptors[a]->Assigned.isAt(b) == true ? "X" : " ") << "]";
								}
								CloakDebugLog(s.str());
							}
#endif
							return heapID;
						}
						return API::Rendering::HANDLEINFO_INVALID_HEAP;
					}
					void CLOAK_CALL_THIS DescriptorHeap::RegisterUsage(In const API::Rendering::ResourceView& handle, In Impl::Rendering::IResource* rsc, In size_t count)
					{
						CLOAK_ASSUME(rsc != nullptr);
						if (handle.HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP && handle.Position < NUM_DESCRIPTORS[static_cast<size_t>(m_type)] && count > 0)
						{
							CLOAK_ASSUME(handle.NodeMask == m_node);
							API::Helper::Lock lock(m_sync);
							if (handle.HeapID <= m_descriptors.size())
							{
								DescriptorView* dv = m_descriptors[handle.HeapID - 1];
								CLOAK_ASSUME(handle.Position + count <= dv->Assigned.size());
								for (size_t a = 0; a < count; a++)
								{
									CLOAK_ASSUME(dv->Assigned[handle.Position + a] == true);
									CLOAK_ASSUME(handle.Position + a < dv->Size);
									CLOAK_ASSUME(dv->Assignment[handle.Position + a] == nullptr);
									dv->Assignment[handle.Position + a] = rsc;
								}
							}
						}
					}
					void CLOAK_CALL_THIS DescriptorHeap::Free(In const API::Rendering::ResourceView& handle)
					{
						if (handle.HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP && handle.Position < NUM_DESCRIPTORS[static_cast<size_t>(m_type)])
						{
							CLOAK_ASSUME(handle.NodeMask == m_node);
							API::Helper::Lock lock(m_sync);
							if (handle.HeapID <= m_descriptors.size())
							{
								DescriptorView* dv = m_descriptors[handle.HeapID - 1];
								CLOAK_ASSUME(handle.Position < dv->Size);
								dv->Space++;
								dv->Assignment[handle.Position] = nullptr;
								dv->Assigned[handle.Position] = false;
							}
#if defined(_DEBUG) && defined(DEBUG_SHOW_DESCRIPTORHEAPS)
							CloakDebugLog("Descriptor Heap[" + std::to_string(m_type) + "] Usage:");
							for (size_t a = 0; a < m_descriptors.size(); a++)
							{
								std::stringstream s;
								s << "\t" << a << " (Type: " << static_cast<size_t>(m_descriptors[a]->PageType) << "): ";
								for (size_t b = 0; b < m_descriptors[a]->Assigned.size(); b++)
								{
									s << "[" << (m_descriptors[a]->Assigned.isAt(b) == true ? "X" : " ") << "]";
								}
								CloakDebugLog(s.str());
							}
#endif
						}
					}
					void CLOAK_CALL_THIS DescriptorHeap::Free(In_reads(numHandles) API::Rendering::ResourceView Handles[], In UINT numHandles)
					{
						API::Helper::Lock lock(m_sync);
						for (UINT a = 0; a < numHandles; a++) 
						{
							if (Handles[a].HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP && Handles[a].Position < NUM_DESCRIPTORS[static_cast<size_t>(m_type)])
							{
								CLOAK_ASSUME(Handles[a].NodeMask == m_node);
								if (Handles[a].HeapID <= m_descriptors.size())
								{
									DescriptorView* dv = m_descriptors[Handles[a].HeapID - 1];
									CLOAK_ASSUME(Handles[a].Position < dv->Size);
									dv->Space++;
									dv->Assignment[Handles[a].Position] = nullptr;
									dv->Assigned[Handles[a].Position] = false;
								}
							}
						}
#if defined(_DEBUG) && defined(DEBUG_SHOW_DESCRIPTORHEAPS)
						CloakDebugLog("Descriptor Heap[" + std::to_string(m_type) + "] Usage:");
						for (size_t a = 0; a < m_descriptors.size(); a++)
						{
							std::stringstream s;
							s << "\t" << a << " (Type: " << static_cast<size_t>(m_descriptors[a]->PageType) << "): ";
							for (size_t b = 0; b < m_descriptors[a]->Assigned.size(); b++)
							{
								s << "[" << (m_descriptors[a]->Assigned.isAt(b) == true ? "X" : " ") << "]";
							}
							CloakDebugLog(s.str());
						}
#endif
					}
					void CLOAK_CALL_THIS DescriptorHeap::Shutdown()
					{
						if (m_sync != nullptr)
						{
							API::Helper::Lock lock(m_sync);
							for (size_t a = 0; a < m_descriptors.size(); a++)
							{
								DescriptorView* dv = m_descriptors[a];
								delete dv;
							}
							m_descriptors.clear();
							lock.unlock();
							SAVE_RELEASE(m_sync);
						}
						RELEASE(m_dev);
					}
					API::Global::Debug::Error CLOAK_CALL_THIS DescriptorHeap::GetOwner(In const API::Rendering::ResourceView& handle, In REFIID riid, Outptr void** ppvObject, In_opt UINT offset)
					{
						const uint32_t pos = handle.Position + offset;
						if (handle.HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP && pos < NUM_DESCRIPTORS[static_cast<size_t>(m_type)])
						{
							CLOAK_ASSUME(handle.NodeMask == m_node);
							API::Helper::Lock lock(m_sync);
							if (handle.HeapID <= m_descriptors.size())
							{
								DescriptorView* dv = m_descriptors[handle.HeapID - 1];
								CLOAK_ASSUME(pos < dv->Size);
								if (dv->Assignment[pos] != nullptr)
								{
									HRESULT hRet = dv->Assignment[pos]->QueryInterface(riid, ppvObject);
									return SUCCEEDED(hRet) ? API::Global::Debug::Error::OK : API::Global::Debug::Error::NO_INTERFACE;
								}
							}
						}
						return API::Global::Debug::Error::NO_CLASS;
					}
					void CLOAK_CALL_THIS DescriptorHeap::GetOwner(In const API::Rendering::ResourceView& handle, In size_t count, Out_writes(count) Impl::Rendering::IResource** resources, In_opt UINT offset)
					{
						const uint32_t pos = handle.Position + offset;
						if (handle.HeapID != API::Rendering::HANDLEINFO_INVALID_HEAP && pos + count <= NUM_DESCRIPTORS[static_cast<size_t>(m_type)])
						{
							CLOAK_ASSUME(handle.NodeMask == m_node);
							API::Helper::Lock lock(m_sync);
							if (handle.HeapID <= m_descriptors.size())
							{
								DescriptorView* dv = m_descriptors[handle.HeapID - 1];
								CLOAK_ASSUME(pos + count <= dv->Size);
								for (size_t a = 0; a < count; a++) 
								{ 
									resources[a] = dv->Assignment[pos]; 
									if (resources[a] != nullptr) { resources[a]->AddRef(); }
								}
							}
						}
					}

					Ret_maybenull ID3D12DescriptorHeap* CLOAK_CALL_THIS DescriptorHeap::GetHeap(In size_t heap)
					{
						if (heap != API::Rendering::HANDLEINFO_INVALID_HEAP)
						{
							API::Helper::Lock lock(m_sync);
							if (heap <= m_descriptors.size()) { return m_descriptors[heap - 1]->Heap; }
						}
						return nullptr;
					}
					Success(return == true) bool CLOAK_CALL_THIS DescriptorHeap::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY_IMPL(ptr, riid, DescriptorHeap, Allocator_v1, SavePtr);
					}

					CLOAK_CALL DynamicDescriptorHeapPage::DynamicDescriptorHeapPage(In Device* device, In IQueue* queue)
					{
						DEBUG_NAME(DynamicDescriptorHeapPage);
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
						m_queue = queue;
						m_aviable = true;
						m_heap = nullptr;

						CloakDebugLog("Create Dynamic Descriptor Heap");
						D3D12_DESCRIPTOR_HEAP_DESC desc;
						desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
						desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
						desc.NodeMask = queue->GetNodeMask();
						desc.NumDescriptors = DYN_DESCRIPTORS;
						HRESULT hRet = device->CreateDescriptorHeap(desc, CE_QUERY_ARGS(&m_heap));
						CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_DESCRIPTOR, true);
						m_handle = DescriptorHandle(Casting::CastBackward(m_heap->GetCPUDescriptorHandleForHeapStart()), Casting::CastBackward(m_heap->GetGPUDescriptorHandleForHeapStart()));
					}
					CLOAK_CALL DynamicDescriptorHeapPage::~DynamicDescriptorHeapPage() 
					{
						RELEASE(m_heap); 
						SAVE_RELEASE(m_sync);
					}

					const DescriptorHandle& CLOAK_CALL_THIS DynamicDescriptorHeapPage::GetHandle() const { return m_handle; }
					ID3D12DescriptorHeap* CLOAK_CALL_THIS DynamicDescriptorHeapPage::GetHeap() const { return m_heap; }
					void CLOAK_CALL_THIS DynamicDescriptorHeapPage::Retire(In uint64_t fenceValue, In QUEUE_TYPE type)
					{
						API::Helper::Lock lock(m_sync);
						m_aviable = false;
						m_retireFence = fenceValue;
						m_retireType = type;
						m_queue->CleanDynamicDescriptorHeapPage(this);
					}
					bool CLOAK_CALL_THIS DynamicDescriptorHeapPage::IsAviable()
					{
						bool av = m_aviable;
						if (av == false)
						{
							API::Helper::Lock lock(m_sync);
							av = m_queue->IsFenceComplete(m_retireType, m_retireFence);
							m_aviable = av;
						}
						return av;
					}

					Success(return == true) bool CLOAK_CALL_THIS DynamicDescriptorHeapPage::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(DynamicDescriptorHeapPage)) { *ptr = (DynamicDescriptorHeapPage*)this; return true; }
						else if (riid == __uuidof(IDynamicDescriptorHeapPage)) { *ptr = (IDynamicDescriptorHeapPage*)this; return true; }
						return SavePtr::iQueryInterface(riid, ptr);
					}

					uint32_t CLOAK_CALL DynamicDescriptorHeap::GetDescriptorSize()
					{
						if (g_descriptorSize == 0)
						{
							IManager* clm = Engine::Graphic::Core::GetCommandListManager();
							IDevice* dev = clm->GetDevice();
							g_descriptorSize = dev->GetDescriptorHandleIncrementSize(HEAP_CBV_SRV_UAV);
							dev->Release();
						}
						return g_descriptorSize;
					}

					CLOAK_CALL DynamicDescriptorHeap::DynamicDescriptorHeap(In API::Rendering::IContext* context, In IQueue* queue)
					{
						m_queue = queue;
						m_context = context; //Don't hold ref count on context, since context already holds a ref count on this object!
						m_curHeap = nullptr;
						m_curOffset = 0;
						DEBUG_NAME(DynamicDescriptorHeap);
					}
					CLOAK_CALL DynamicDescriptorHeap::~DynamicDescriptorHeap()
					{
						SAVE_RELEASE(m_curHeap);
						for (size_t a = 0; a < m_heaps.size(); a++) { SAVE_RELEASE(m_heaps[a]); }
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::CleanUsedHeaps(In uint64_t fenceVal, In QUEUE_TYPE queue)
					{
						RetireCurrentHeap();
						RetireUsedHeaps(fenceVal, queue);
						m_GraphicHandleCache.ClearCache();
						m_ComputeHandleCache.ClearCache();
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::SetGraphicDescriptorHandles(In UINT rootIndex, In UINT offset, In UINT numHandles, In_reads(numHandles) const API::Rendering::CPU_DESCRIPTOR_HANDLE Handles[])
					{
						if (CloakDebugCheckOK(numHandles > 0, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							m_GraphicHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, Handles);
						}
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::SetComputeDescriptorHandles(In UINT rootIndex, In UINT offset, In UINT numHandles, In_reads(numHandles) const API::Rendering::CPU_DESCRIPTOR_HANDLE Handles[])
					{
						if (CloakDebugCheckOK(numHandles > 0, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							m_ComputeHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, Handles);
						}
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::ParseGraphicRootSignature(In IRootSignature* sig)
					{
						m_GraphicHandleCache.ParseRootSignature(sig);
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::ParseComputeRootSignature(In IRootSignature* sig)
					{
						m_ComputeHandleCache.ParseRootSignature(sig);
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::CommitGraphicDescriptorTables(In ID3D12GraphicsCommandList* cmdList)
					{
						if (m_GraphicHandleCache.RootParamsBitMap != 0) { CopyAndBindStages(m_GraphicHandleCache, cmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable); }
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::CommitComputeDescriptorTables(In ID3D12GraphicsCommandList* cmdList)
					{
						if (m_ComputeHandleCache.RootParamsBitMap != 0) { CopyAndBindStages(m_ComputeHandleCache, cmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable); }
					}
					D3D12_GPU_DESCRIPTOR_HANDLE CLOAK_CALL_THIS DynamicDescriptorHeap::UploadDirect(In const D3D12_CPU_DESCRIPTOR_HANDLE& handle)
					{
						if (!HasSpace(1))
						{
							RetireCurrentHeap();
							UnbindAll();
						}
						DX12::Context* context = nullptr;
						if (SUCCEEDED(m_context->QueryInterface(CE_QUERY_ARGS(&context))))
						{
							const D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
							ID3D12DescriptorHeap* const heap = GetHeap();
							context->SetDescriptorHeaps(&type, &heap);
							DescriptorHandle dstHandle = m_handle + m_curOffset*GetDescriptorSize();
							m_curOffset++;
							IManager* clm = Engine::Graphic::Core::GetCommandListManager();
							IDevice* dev = clm->GetDevice();
							dev->CopyDescriptorsSimple(1, dstHandle.GetCPUHandle(), Casting::CastBackward(handle), HEAP_CBV_SRV_UAV);
							dev->Release();
							context->Release();
							return Casting::CastForward(dstHandle.GetGPUHandle());
						}
						return{ 0 };
					}

					bool CLOAK_CALL_THIS DynamicDescriptorHeap::HasSpace(In uint32_t count) { return m_curHeap != nullptr && m_curOffset + count <= DYN_DESCRIPTORS; }
					void CLOAK_CALL_THIS DynamicDescriptorHeap::RetireCurrentHeap()
					{
						if (m_curOffset != 0 && m_curHeap != nullptr)
						{
							m_heaps.push_back(m_curHeap);
							RELEASE(m_curHeap);
							m_curOffset = 0;
						}
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::RetireUsedHeaps(In uint64_t fenceVal, In QUEUE_TYPE queue)
					{
						for (size_t a = 0; a < m_heaps.size(); a++) { m_heaps[a]->Retire(fenceVal, queue); }
						m_heaps.clear();
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::CopyAndBindStages(Inout DescriptorHandleCache& cache, In ID3D12GraphicsCommandList* cmdList, In void(STDMETHODCALLTYPE ID3D12GraphicsCommandList::*func)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
					{
						uint32_t size = cache.ComputeSize();
						if (!HasSpace(size))
						{
							RetireCurrentHeap();
							UnbindAll();
						}
						DX12::Context* context = nullptr;
						if (SUCCEEDED(m_context->QueryInterface(CE_QUERY_ARGS(&context))))
						{
							const D3D12_DESCRIPTOR_HEAP_TYPE type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
							ID3D12DescriptorHeap* const heap = GetHeap();
							context->SetDescriptorHeaps(&type, &heap);
							cache.CopyAndBindTables(Allocate(size), cmdList, func);
							context->Release();
						}
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::UnbindAll()
					{
						m_GraphicHandleCache.UnbindAll();
						m_ComputeHandleCache.UnbindAll();
					}
					Ret_notnull ID3D12DescriptorHeap* CLOAK_CALL_THIS DynamicDescriptorHeap::GetHeap()
					{
						if (m_curHeap == nullptr)
						{
							m_curOffset = 0;
							IDynamicDescriptorHeapPage* p = m_queue->RequestDynamicDescriptorHeapPage();
							HRESULT hRet = p->QueryInterface(CE_QUERY_ARGS(&m_curHeap));
							assert(SUCCEEDED(hRet));
							m_handle = m_curHeap->GetHandle();
						}
						return m_curHeap->GetHeap();
					}
					DescriptorHandle CLOAK_CALL_THIS DynamicDescriptorHeap::Allocate(In UINT count)
					{
						DescriptorHandle res = m_handle + (m_curOffset*GetDescriptorSize());
						m_curOffset += count;
						return res;
					}
					Success(return == true) bool CLOAK_CALL_THIS DynamicDescriptorHeap::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY_IMPL(ptr, riid, DynamicDescriptorHeap, Allocator_v1, SavePtr);
					}

					CLOAK_CALL DynamicDescriptorHeap::DescriptorTableCache::DescriptorTableCache()
					{
						AssignedBitMap = 0;
						TableStart = nullptr;
						TableSize = 0;
					}

					CLOAK_CALL DynamicDescriptorHeap::DescriptorHandleCache::DescriptorHandleCache()
					{
						ClearCache();
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::DescriptorHandleCache::ClearCache()
					{
						RootParamsBitMap = 0;
						MaxCachedDescriptors = 0;
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindTables(In DescriptorHandle dstHandleStart, In ID3D12GraphicsCommandList* cmdList, In void(STDMETHODCALLTYPE ID3D12GraphicsCommandList::*func)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
					{
						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();
						uint32_t paramCount = 0;
						uint32_t size[MaxNumDescriptorTables];
						DWORD indices[MaxNumDescriptorTables];
						uint32_t reqSpace = 0;
						DWORD index;

						DWORD params = RootParamsBitMap;
						//while (_BitScanForward(&index, params) == TRUE)
						while ((index = API::Global::Math::bitScanForward(params)) < 0xFF)
						{
							indices[paramCount] = index;
							params ^= 1 << index;
							DWORD maxHandle;
							if ((maxHandle = API::Global::Math::bitScanReverse(DescriptorTables[index].AssignedBitMap)) < 0xFF)
							{
								reqSpace += maxHandle + 1;
								size[paramCount] = maxHandle + 1;
							}
							paramCount++;
						}
						if (paramCount <= MaxNumDescriptorTables)
						{
							RootParamsBitMap = 0;
							UINT dstRanges = 0;
							API::Rendering::CPU_DESCRIPTOR_HANDLE dstRangeStarts[DYN_DESCRIPTORS_PER_COPY];
							UINT dstRangeSize[DYN_DESCRIPTORS_PER_COPY];

							UINT srcRanges = 0;
							API::Rendering::CPU_DESCRIPTOR_HANDLE srcRangeStarts[DYN_DESCRIPTORS_PER_COPY];
							UINT srcRangeSize[DYN_DESCRIPTORS_PER_COPY];

							const uint32_t descSize = GetDescriptorSize();
							for (uint32_t a = 0; a < paramCount; a++)
							{
								index = indices[a];

								const DescriptorTableCache& descTable = DescriptorTables[index];
								const API::Rendering::CPU_DESCRIPTOR_HANDLE* srcHandle = descTable.TableStart;
								uint32_t handles = descTable.AssignedBitMap;
								API::Rendering::CPU_DESCRIPTOR_HANDLE curDst = dstHandleStart.GetCPUHandle();
								(cmdList->*func)(index, Casting::CastForward(dstHandleStart.GetGPUHandle()));
								dstHandleStart += size[a] * descSize;
								DWORD skipCount;
								while ((skipCount = API::Global::Math::bitScanForward(handles)) < 0xFF)
								{
									handles >>= skipCount;
									srcHandle += skipCount;
									curDst.ptr += skipCount*descSize;
									DWORD descCount = API::Global::Math::bitScanForward(~handles);
									handles >>= descCount;
									if (srcRanges + descCount > DYN_DESCRIPTORS_PER_COPY)
									{
										dev->CopyDescriptors(dstRanges, dstRangeStarts, dstRangeSize, srcRanges, srcRangeStarts, srcRangeSize, HEAP_CBV_SRV_UAV);
										srcRanges = 0;
										dstRanges = 0;
									}
									dstRangeStarts[dstRanges] = curDst;
									dstRangeSize[dstRanges] = descCount;
									dstRanges++;

									for (UINT b = 0; b < descCount; b++)
									{
										srcRangeStarts[srcRanges] = srcHandle[b];
										srcRangeSize[srcRanges] = 1;
										CLOAK_ASSUME(srcHandle[b].ptr != 0);
										srcRanges++;
									}
									srcHandle += descCount;
									curDst.ptr += descCount*descSize;
								}
							}
							dev->CopyDescriptors(dstRanges, dstRangeStarts, dstRangeSize, srcRanges, srcRangeStarts, srcRangeSize, HEAP_CBV_SRV_UAV);
						}
						dev->Release();
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::DescriptorHandleCache::UnbindAll()
					{
						RootParamsBitMap = 0;
						DWORD params = RootDescriptorBitMap;
						DWORD index;
						while ((index = API::Global::Math::bitScanForward(params)) < 0xFF) 
						{
							params ^= 1 << index;
							if (DescriptorTables[index].AssignedBitMap != 0) { RootParamsBitMap |= 1 << index; }
						}
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(In UINT rootIndex, In UINT offset, In UINT numHandles, In_reads(numHandles) const API::Rendering::CPU_DESCRIPTOR_HANDLE* handles)
					{
						if (CloakDebugCheckOK(((1 << rootIndex) & RootDescriptorBitMap) != 0 && offset + numHandles <= DescriptorTables[rootIndex].TableSize, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							DescriptorTableCache& cache = DescriptorTables[rootIndex];
							API::Rendering::CPU_DESCRIPTOR_HANDLE* dst = cache.TableStart + offset;
							for (UINT a = 0; a < numHandles; a++) { dst[a] = handles[a]; }
							cache.AssignedBitMap |= ((1 << numHandles) - 1) << offset;
							RootParamsBitMap |= 1 << rootIndex;
						}
					}
					void CLOAK_CALL_THIS DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(In IRootSignature* sig)
					{
						UINT offset = 0;
						RootParamsBitMap = 0;
						RootDescriptorBitMap = sig->GetDescriptorBitMap();
						DWORD params = RootDescriptorBitMap;
						DWORD index;
						while ((index = API::Global::Math::bitScanForward(params)) < 0xFF)
						{
							params ^= 1 << index;
							UINT size = sig->GetTableSize(index);
							if (size > 0)
							{
								DescriptorTableCache& cache = DescriptorTables[index];
								cache.AssignedBitMap = 0;
								cache.TableStart = HandleCache + offset;
								cache.TableSize = size;
								offset += size;
							}
						}
						MaxCachedDescriptors = offset;
					}
					uint32_t CLOAK_CALL_THIS DynamicDescriptorHeap::DescriptorHandleCache::ComputeSize()
					{
						uint32_t req = 0;
						DWORD index;
						DWORD params = RootParamsBitMap;
						while ((index = API::Global::Math::bitScanForward(params)) < 0xFF)
						{
							params ^= 1 << index;
							DWORD handle = API::Global::Math::bitScanReverse(DescriptorTables[index].AssignedBitMap);
							req += static_cast<uint32_t>(handle) + 1;
						}
						return req;
					}
					//The Allocator class is just for ESRAM purpos (e.g. on XBox One) and therefore not used on Windows
					CLOAK_CALL Allocator::Allocator() {}
					CLOAK_CALL Allocator::~Allocator() {}
					void CLOAK_CALL_THIS Allocator::PushStack() {}
					void CLOAK_CALL_THIS Allocator::PopStack() {}
					API::Rendering::GPU_ADDRESS CLOAK_CALL_THIS Allocator::Alloc(In size_t size, In size_t allign) { return GPU_ADDRESS_UNKNOWN; }
					intptr_t CLOAK_CALL_THIS Allocator::GetSizeOfFreeSpace() const { return 0; }

					Success(return == true) bool CLOAK_CALL_THIS Allocator::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY(ptr, riid, Allocator, Allocator_v1, SavePtr);
					}

					const size_t g_dynamicAllocatorSize[2] = { 64 KB, 2 MB };

					CLOAK_CALL DynamicAllocatorPage::DynamicAllocatorPage(In ID3D12Resource* base, In DynamicAllocatorType type, In IQueue* queue, In API::Rendering::RESOURCE_STATE states, In bool canTransition, In size_t size) : Resource(base, states, queue->GetNodeID(), canTransition), m_size(size), m_queue(queue), m_type(type)
					{
						m_aviable = true;
						m_reqClean = false;
						for (size_t a = 0; a < ARRAYSIZE(m_fence); a++) { m_fence[a] = 0; }
						m_offset = 0;
						m_gpuHandle.ptr = base->GetGPUVirtualAddress();
						DEBUG_NAME(DynamicAllocatorPage);
					}
					CLOAK_CALL DynamicAllocatorPage::~DynamicAllocatorPage()
					{
						
					}
					bool CLOAK_CALL_THIS DynamicAllocatorPage::IsAviable()
					{
						bool av = m_aviable;
						if (av == false)
						{
							API::Helper::Lock lock(m_sync);
							//Check if a fence on any queue is not complete:
							for (size_t a = 0; a < ARRAYSIZE(m_fence); a++)
							{
								if (m_fence[a] > 0 && m_queue->IsFenceComplete(static_cast<QUEUE_TYPE>(a), m_fence[a]) == false) { return false; }
							}
							//All fences complete
							if (m_aviable.exchange(true) == false)
							{
								m_offset = 0;
								for (size_t a = 0; a < ARRAYSIZE(m_fence); a++) { m_fence[a] = 0; }
							}
							return true;
						}
						return av;
					}
					bool CLOAK_CALL_THIS DynamicAllocatorPage::TryToAllocate(Inout DynAlloc* alloc, In size_t SizeAligned, In_opt size_t Alignment)
					{
						if (alloc != nullptr && m_aviable == true)
						{
							API::Helper::Lock lock(m_sync);
							const size_t offset = AlignUp(m_offset, Alignment);
							if (offset + SizeAligned <= m_size)
							{
								alloc->Initialize(this, offset, SizeAligned);
								m_offset = offset + SizeAligned;
								return true;
							}
							m_reqClean = true;
						}
						return false;
					}
					void CLOAK_CALL_THIS DynamicAllocatorPage::Clean(In uint64_t fenceID, In QUEUE_TYPE queue)
					{
						const size_t q = static_cast<size_t>(queue);
						CLOAK_ASSUME(q < ARRAYSIZE(m_fence));
						API::Helper::Lock lock(m_sync);
						m_fence[q] = max(m_fence[q], fenceID);
						m_aviable = m_reqClean == false;
						m_reqClean = false;
						m_queue->CleanDynamicAllocatorPage(this, m_aviable, m_type);
					}
					void CLOAK_CALL_THIS DynamicAllocatorPage::Map(In void** data)
					{
						API::Helper::Lock lock(m_sync);
						if (m_data != nullptr) 
						{
							HRESULT hRet = m_data->Map(0, nullptr, data);
							assert(SUCCEEDED(hRet));
						}
					}
					void CLOAK_CALL_THIS DynamicAllocatorPage::Unmap()
					{
						API::Helper::Lock lock(m_sync);
						if (m_data != nullptr) 
						{
							if (m_type == DynamicAllocatorType::CPU)
							{
								D3D12_RANGE range;
								range.Begin = 0;
								range.End = m_size;
								m_data->Unmap(0, &range);
							}
							else
							{
								m_data->Unmap(0, nullptr);
							}
						}
					}
					Success(return == true) bool CLOAK_CALL_THIS DynamicAllocatorPage::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY_IMPL(ptr, riid, DynamicAllocatorPage, Allocator_v1, Resource);
					}

					CLOAK_CALL CustomDynamicAllocatorPage::CustomDynamicAllocatorPage(In ID3D12Resource* base, In DynamicAllocatorType type, In IQueue* queue, In API::Rendering::RESOURCE_STATE states, In bool canTransition, In size_t size) : Resource(base, states, queue->GetNodeID(), canTransition), m_queue(queue), m_size(size), m_type(type)
					{
						m_fence = 0;
						m_aviable = true;
					}
					CLOAK_CALL CustomDynamicAllocatorPage::~CustomDynamicAllocatorPage()
					{

					}
					bool CLOAK_CALL_THIS CustomDynamicAllocatorPage::IsAviable()
					{
						bool av = m_aviable;
						if (av == false)
						{
							API::Helper::Lock lock(m_sync);
							av = m_queue->IsFenceComplete(m_qt, m_fence);
							m_aviable = av;
						}
						return av;
					}
					bool CLOAK_CALL_THIS CustomDynamicAllocatorPage::TryToAllocate(Inout DynAlloc* alloc, In size_t SizeAligned, In_opt size_t Alignment)
					{
						if (alloc != nullptr && m_aviable == true)
						{
							API::Helper::Lock lock(m_sync);
							if (SizeAligned == m_size)
							{
								alloc->Initialize(this, 0, SizeAligned);
								return true;
							}
						}
						return false;
					}
					void CLOAK_CALL_THIS CustomDynamicAllocatorPage::Clean(In uint64_t fenceID, In QUEUE_TYPE queue)
					{
						API::Helper::Lock lock(m_sync);
						m_aviable = false;
						m_fence = fenceID;
						m_qt = queue;
						m_queue->CleanCustomDynamicAllocatorPage(this);
					}
					void CLOAK_CALL_THIS CustomDynamicAllocatorPage::Map(In void** data)
					{
						API::Helper::Lock lock(m_sync);
						if (m_data != nullptr)
						{
							HRESULT hRet = m_data->Map(0, nullptr, data);
							assert(SUCCEEDED(hRet));
						}
					}
					void CLOAK_CALL_THIS CustomDynamicAllocatorPage::Unmap()
					{
						API::Helper::Lock lock(m_sync);
						if (m_data != nullptr) 
						{
							if (m_type == DynamicAllocatorType::CPU)
							{
								D3D12_RANGE range;
								range.Begin = 0;
								range.End = m_size;
								m_data->Unmap(0, &range);
							}
							else
							{
								m_data->Unmap(0, nullptr);
							}
						}
					}
					
					bool CLOAK_CALL_THIS CustomDynamicAllocatorPage::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(Impl::Rendering::Allocator_v1::IDynamicAllocatorPage)) { *ptr = (Impl::Rendering::Allocator_v1::IDynamicAllocatorPage*)this; return true; }
						else if (riid == __uuidof(Impl::Rendering::DX12::Allocator_v1::CustomDynamicAllocatorPage*)) { *ptr = (Impl::Rendering::DX12::Allocator_v1::CustomDynamicAllocatorPage*)this; return true; } 
						return Resource::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL DynamicAllocator::DynamicAllocator(In DynamicAllocatorType type, In IQueue* queue) : m_type(type), m_queue(queue)
					{
						m_lastPage = nullptr;
						DEBUG_NAME(DynamicAllocator);
					}
					CLOAK_CALL DynamicAllocator::~DynamicAllocator()
					{
						for (size_t a = 0; a < m_pages.size(); a++) { SAVE_RELEASE(m_pages[a]); }
						SAVE_RELEASE(m_lastPage);
					}
					void CLOAK_CALL_THIS DynamicAllocator::Allocate(Inout DynAlloc* alloc, In size_t SizeInByte, In_opt size_t Alignment)
					{
						if (alloc != nullptr)
						{
							const size_t alignedSize = AlignUp(SizeInByte, Alignment);
							if (m_queue->GetDynamicAllocatorPageSize(m_type) < AlignUp(SizeInByte, Alignment))
							{
								IDynamicAllocatorPage* page = m_queue->RequestCustomDynamicAllocatorPage(m_type, alignedSize);
								const bool r = page->TryToAllocate(alloc, alignedSize, Alignment);
								assert(r == true);
								m_pages.push_back(page);
							}
							else
							{
								if (m_lastPage == nullptr || m_lastPage->TryToAllocate(alloc, alignedSize, Alignment) == false)
								{
									do {
										if (m_lastPage != nullptr) { m_pages.push_back(m_lastPage); }
										m_lastPage = m_queue->RequestDynamicAllocatorPage(m_type);
									} while (m_lastPage->TryToAllocate(alloc, alignedSize, Alignment) == false);
								}
							}
						}
					}
					void CLOAK_CALL_THIS DynamicAllocator::Clean(In uint64_t fenceID, In QUEUE_TYPE queue)
					{
						if (m_lastPage != nullptr) { m_lastPage->Clean(fenceID, queue); }
						for (size_t a = 0; a < m_pages.size(); a++) { m_pages[a]->Clean(fenceID, queue); }
						m_lastPage = nullptr;
						m_pages.clear();
					}

					Success(return == true) bool CLOAK_CALL_THIS DynamicAllocator::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY_IMPL(ptr, riid, DynamicAllocator, Allocator_v1, SavePtr);
					}

					CLOAK_CALL ReadBackPage::ReadBackPage(In ID3D12Resource* base, In size_t size, In IQueue* queue) : Resource(base, API::Rendering::STATE_COPY_DEST, queue->GetNodeID(), false), m_size(size), m_queue(queue)
					{
						m_syncClean = nullptr;
						m_begin = 0;
						m_remSize = m_size;
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncClean));
					}
					CLOAK_CALL ReadBackPage::~ReadBackPage()
					{
						SAVE_RELEASE(m_syncClean);
					}

					size_t CLOAK_CALL_THIS ReadBackPage::GetFreeSize()
					{
						API::Helper::Lock lock(m_sync);
						const size_t remEnd = m_size - m_begin;
						const size_t remBeg = m_remSize > remEnd ? (m_remSize - remEnd) : 0;
						size_t r = min(remEnd, m_remSize);
						return max(r, remBeg);
					}
					bool CLOAK_CALL_THIS ReadBackPage::TryToAllocate(In size_t SizeAligned, In UINT rowSize, In UINT rowPadding, Out size_t* Offset, In_opt size_t Alignment)
					{
						if (Offset != nullptr)
						{
							API::Helper::Lock lock(m_sync);
							if (SizeAligned == 0) { *Offset = m_begin; return true; }
							const size_t beg = AlignUp(m_begin, Alignment);
							const size_t remEnd = m_size - m_begin;
							const size_t remBeg = m_remSize > remEnd ? (m_remSize - remEnd) : 0;
							if (SizeAligned + beg <= m_size)
							{
								*Offset = beg;
								Entry e;
								e.Fence = 0;
								e.Queue = QUEUE_TYPE_DIRECT;
								e.Offset = beg;
								e.ReadCount = 1;
								e.Size = SizeAligned;
								e.FullSize = beg + SizeAligned - m_begin;
								e.RowPadding = rowPadding;
								e.RowSize = rowSize;
								m_aviable.push(e);
								m_remSize -= e.FullSize;
								m_begin = beg + SizeAligned;
								return true;
							}
							else if (remBeg >= SizeAligned)
							{
								*Offset = 0;
								Entry e;
								e.Fence = 0;
								e.Queue = QUEUE_TYPE_DIRECT;
								e.Offset = 0;
								e.ReadCount = 1;
								e.Size = SizeAligned;
								e.FullSize = SizeAligned + remEnd;
								e.RowPadding = rowPadding;
								e.RowSize = rowSize;
								m_aviable.push(e);
								m_remSize -= e.FullSize;
								m_begin = SizeAligned;
								return true;
							}
						}
						return false;
					}
					void CLOAK_CALL_THIS ReadBackPage::Clean(In uint64_t fenceID, In QUEUE_TYPE queue)
					{
						API::Helper::Lock lock(m_sync);
						bool upd = false;
						for (auto& e : m_aviable)
						{
							if (e.Fence == 0)
							{
								e.Fence = fenceID;
								e.Queue = queue;
								e.ReadCount--;
								if (e.ReadCount == 0) { upd = true; }
							}
						}
						if (upd == true)
						{
							while (m_aviable.empty() == false)
							{
								const Entry& e = m_aviable.front();
								if (e.ReadCount == 0)
								{
									m_remSize += e.FullSize;
									m_aviable.pop();
								}
								else { break; }
							}
							CLOAK_ASSUME(((m_aviable.empty() == false) ^ (m_remSize == m_size)) == true);
							if (m_remSize == m_size) { m_begin = 0; }
						}
						m_syncClean->signalAll();
						m_queue->CleanReadBackPage(this);
					}

					void CLOAK_CALL_THIS ReadBackPage::ReadData(In size_t offset, In size_t size, Out_writes_bytes(size) void* res)
					{
						uint64_t fence = 0;
						QUEUE_TYPE qt;
						UINT rPad = 0;
						UINT rSize = 0;
						size_t dSize = 0;
						API::Helper::Lock lock;
						do {
							lock.lock(m_sync);
							bool f = false;
							for (auto& e : m_aviable)
							{
								if (e.Offset <= offset && e.Offset + e.Size > offset)
								{
									fence = e.Fence;
									qt = e.Queue;
									rPad = e.RowPadding;
									rSize = e.RowSize;
									dSize = e.Size;
									f = true;
									break;
								}
							}
							CLOAK_ASSUME(f == true);
							lock.unlock();
							if (fence == 0)
							{
								lock.lock(m_syncClean, false); //Wait for cleanup and retry
								lock.unlock();
							}
							else
							{
								m_queue->WaitForFence(qt, fence);
								lock.lock(m_sync);
							}
						} while (fence == 0);
						if (m_data != nullptr)
						{
							D3D12_RANGE r;
							r.Begin = offset;
							r.End = offset + dSize;
							void* data = nullptr;
							HRESULT hRet = m_data->Map(0, &r, &data);
							assert(SUCCEEDED(hRet)); 
							CopyData(res, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + offset), size, dSize, rSize, rPad);
							m_data->Unmap(0, nullptr);
						}
					}
					void CLOAK_CALL_THIS ReadBackPage::AddReadCount(In size_t offset)
					{
						API::Helper::Lock lock(m_sync);
						AddRef();
						for (auto& e : m_aviable)
						{
							if (e.Offset <= offset && e.Offset + e.Size > offset) { e.ReadCount++; break; }
						}
					}
					void CLOAK_CALL_THIS ReadBackPage::RemoveReadCount(In size_t offset)
					{
						API::Helper::Lock lock(m_sync);
						bool upd = false;
						for (auto& e : m_aviable)
						{
							if (e.Offset <= offset && e.Offset + e.Size > offset) 
							{ 
								CLOAK_ASSUME(e.ReadCount > 0);
								e.ReadCount--;
								if (e.ReadCount == 0) { upd = true; }
								break;
							}
						}
						if (upd)
						{
							while (m_aviable.empty() == false)
							{
								const Entry& e = m_aviable.front();
								if (e.ReadCount == 0)
								{
									m_remSize += e.FullSize;
									m_aviable.pop();
								}
								else { break; }
							}
							CLOAK_ASSUME(((m_aviable.empty() == false) ^ (m_remSize == m_size)) == true);
							if (m_remSize == m_size) { m_begin = 0; }
						}
						Release();
					}
					
					Success(return == true) bool CLOAK_CALL_THIS ReadBackPage::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Rendering::Allocator_v1::IReadBackPage)) { *ptr = (API::Rendering::Allocator_v1::IReadBackPage*)this; return true; }
						else RENDERING_QUERY_IMPL(ptr, riid, ReadBackPage, Allocator_v1, Resource);
					}

					CLOAK_CALL CustomReadBackPage::CustomReadBackPage(In ID3D12Resource* base, In size_t size, In IQueue* queue) : Resource(base, API::Rendering::STATE_COPY_DEST, queue->GetNodeID(), false), m_size(size), m_queue(queue)
					{
						m_syncClean = nullptr;
						m_aviable = true;
						CREATE_INTERFACE(CE_QUERY_ARGS(&m_syncClean));
					}
					CLOAK_CALL CustomReadBackPage::~CustomReadBackPage()
					{
						SAVE_RELEASE(m_syncClean);
					}

					size_t CLOAK_CALL_THIS CustomReadBackPage::GetFreeSize()
					{
						API::Helper::Lock lock(m_sync);
						return m_aviable ? m_size : 0;
					}
					bool CLOAK_CALL_THIS CustomReadBackPage::TryToAllocate(In size_t SizeAligned, In UINT rowSize, In UINT rowPadding, Out size_t* Offset, In_opt size_t Alignment)
					{
						if (Offset != nullptr)
						{
							CLOAK_ASSUME(SizeAligned == m_size);
							API::Helper::Lock lock(m_sync);
							CLOAK_ASSUME(m_aviable);
							*Offset = 0;
							m_aviable = false;
							m_rowSize = rowSize;
							m_rowPadding = rowPadding;
							return true;
						}
						return false;
					}
					void CLOAK_CALL_THIS CustomReadBackPage::Clean(In uint64_t fenceID, In QUEUE_TYPE queue)
					{
						API::Helper::Lock lock(m_sync);
						m_fence = fenceID;
						m_qt = queue;
						m_syncClean->signalAll();
					}

					void CLOAK_CALL_THIS CustomReadBackPage::ReadData(In size_t offset, In size_t size, Out_writes_bytes(size) void* res)
					{
						CLOAK_ASSUME(offset == 0 && size <= m_size);
						API::Helper::Lock lock(m_sync);
						while (m_fence == 0)
						{
							lock.unlock();
							lock.lock(m_syncClean, false); //Wait for cleanup and retry
							lock.unlock();
							lock.lock(m_sync);
						}
						CLOAK_ASSUME(m_fence != 0);
						const uint64_t fence = m_fence;
						const QUEUE_TYPE qt = m_qt;
						lock.unlock();
						m_queue->WaitForFence(qt, fence);
						lock.lock(m_sync);
						if (m_data != nullptr)
						{
							D3D12_RANGE r;
							r.Begin = offset;
							r.End = offset + m_size;
							void* data = nullptr;
							HRESULT hRet = m_data->Map(0, &r, &data);
							assert(SUCCEEDED(hRet));
							CopyData(res, data, size, m_size, m_rowSize, m_rowPadding);
							m_data->Unmap(0, nullptr);
						}
					}
					void CLOAK_CALL_THIS CustomReadBackPage::AddReadCount(In size_t offset)
					{
						CLOAK_ASSUME(offset == 0);
						API::Helper::Lock lock(m_sync);
						AddRef();
					}
					void CLOAK_CALL_THIS CustomReadBackPage::RemoveReadCount(In size_t offset)
					{
						CLOAK_ASSUME(offset == 0);
						API::Helper::Lock lock(m_sync);
						Release();
					}

					Success(return == true) bool CLOAK_CALL_THIS CustomReadBackPage::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Rendering::Allocator_v1::IReadBackPage)) { *ptr = (API::Rendering::Allocator_v1::IReadBackPage*)this; return true; }
						else if (riid == __uuidof(Impl::Rendering::Allocator_v1::IReadBackPage)) { *ptr = (Impl::Rendering::Allocator_v1::IReadBackPage*)this; return true; }
						else if (riid == __uuidof(Impl::Rendering::DX12::Allocator_v1::CustomReadBackPage*)) { *ptr = (Impl::Rendering::DX12::Allocator_v1::CustomReadBackPage*)this; return true; }
						return Resource::iQueryInterface(riid, ptr);
					}

					CLOAK_CALL ReadBackAllocator::ReadBackAllocator(In IQueue* queue) : m_queue(queue)
					{
						m_curPage = nullptr;
					}
					CLOAK_CALL ReadBackAllocator::~ReadBackAllocator()
					{
						CLOAK_ASSUME(m_lastPages.size() == 0);
						CLOAK_ASSUME(m_curPage == nullptr);
					}
					API::Rendering::ReadBack CLOAK_CALL_THIS ReadBackAllocator::Allocate(In size_t SizeInByte, In UINT rowSize, In UINT rowPadding, In_opt size_t Alignment)
					{
						size_t offset = 0;
						if (m_curPage == nullptr) { m_curPage = m_queue->RequestReadBackPage(SizeInByte); }
						CLOAK_ASSUME(m_curPage != nullptr);
						do {
							if (m_curPage->TryToAllocate(SizeInByte, rowSize, rowPadding, &offset, Alignment)) { return API::Rendering::ReadBack(m_curPage, offset, (rowSize - rowPadding)*(SizeInByte / rowSize)); }
							m_lastPages.push_back(m_curPage); 
							m_curPage = m_queue->RequestReadBackPage(SizeInByte);
						} while (true);
						CLOAK_ASSUME(false); //We should never end up here!
						return API::Rendering::ReadBack(nullptr, 0, 0);
					}
					void CLOAK_CALL_THIS ReadBackAllocator::Clean(In uint64_t fenceID, In QUEUE_TYPE queue)
					{
						if (m_curPage != nullptr)
						{
							m_curPage->Clean(fenceID, queue);
							SAVE_RELEASE(m_curPage);
						}
						for (size_t a = 0; a < m_lastPages.size(); a++)
						{
							IReadBackPage* p = m_lastPages[a];
							p->Clean(fenceID, queue);
							p->Release();
						}
						m_lastPages.clear();
					}
					
					Success(return == true) bool CLOAK_CALL_THIS ReadBackAllocator::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY_IMPL(ptr, riid, ReadBackAllocator, Allocator_v1, SavePtr);
					}
				}
			}
		}
	}
}
#endif