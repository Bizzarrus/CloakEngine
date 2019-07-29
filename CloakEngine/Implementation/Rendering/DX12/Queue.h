#pragma once
#ifndef CE_IMPL_RENDERING_DX12_QUEUE_H
#define CE_IMPL_RENDERING_DX12_QUEUE_H

#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/Rendering/Queue.h"
#include "Implementation/Rendering/Context.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/Device.h"
#include "Implementation/Rendering/DX12/Allocator.h"
#include "Implementation/Rendering/DX12/Query.h"
#include "Implementation/Rendering/Manager.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace Queue_v1 {
					struct GraphicsCommandList {
						CE::RefPointer<ID3D12GraphicsCommandList> V0 = nullptr;
						CE::RefPointer<ID3D12GraphicsCommandList1> V1 = nullptr;
						CE::RefPointer<ID3D12GraphicsCommandList2> V2 = nullptr;
						CE::RefPointer<ID3D12GraphicsCommandList3> V3 = nullptr;
						CE::RefPointer<ID3D12GraphicsCommandList4> V4 = nullptr;
						CE::RefPointer<ID3D12GraphicsCommandList5> V5 = nullptr;
					};

					class Queue;
					class SingleQueue : public virtual ISingleQueue {
						friend class Queue;
						public:
							CLOAK_CALL SingleQueue(In Queue* parent, In QUEUE_TYPE qtype, In Device* device, In UINT node, In D3D12_COMMAND_LIST_TYPE type, In D3D12_COMMAND_QUEUE_PRIORITY prio, In IManager* manager, In API::Rendering::CROSS_ADAPTER_SUPPORT support);
							CLOAK_CALL ~SingleQueue();

							QUEUE_TYPE CLOAK_CALL_THIS GetType() const override;
							IQueue* CLOAK_CALL_THIS GetParent() const override;
							size_t CLOAK_CALL_THIS GetNodeID() const override;
							UINT CLOAK_CALL_THIS GetNodeMask() const override;
							void CLOAK_CALL_THIS CreateContext(In REFIID riid, Outptr void** ptr) override;
							API::Rendering::IVertexBuffer* CLOAK_CALL_THIS CreateVertexBuffer(In UINT vertexCount, In UINT vertexSize, In_reads(vertexCount*vertexSize) const void* data = nullptr) const override;
							API::Rendering::IIndexBuffer* CLOAK_CALL_THIS CreateIndexBuffer(In UINT size, In_reads(size) const uint16_t* data) const override;
							API::Rendering::IIndexBuffer* CLOAK_CALL_THIS CreateIndexBuffer(In UINT size, In_reads(size) const uint32_t* data) const override;
							API::Rendering::IConstantBuffer* CLOAK_CALL_THIS CreateConstantBuffer(In UINT elementSize, In_reads_opt(elementSize) const void* data = nullptr) const override;
							API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS CreateStructuredBuffer(In UINT elementCount, In UINT elementSize, In_reads(elementSize) const void* data, In_opt bool counterBuffer = true) const override;
							API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS CreateStructuredBuffer(In UINT elementCount, In UINT elementSize, In_opt bool counterBuffer, In_reads_opt(elementSize) const void* data = nullptr) const override;
							
							void CLOAK_CALL_THIS ReleaseOnFence(In API::Helper::ISavePtr* ptr, In uint64_t fence) override;
							bool CLOAK_CALL_THIS IsFenceComplete(In uint64_t val) override;
							bool CLOAK_CALL_THIS WaitForFence(In uint64_t val) override;
							void CLOAK_CALL_THIS WaitForAdapter(In IAdapter* q, In uint64_t fence) override;
							uint64_t CLOAK_CALL_THIS FlushExecutions(In API::Global::Memory::PageStackAllocator* alloc, In_opt bool wait = false) override;
							void CLOAK_CALL_THIS FreeContext(In Impl::Rendering::ICopyContext* ptr);							
							void CLOAK_CALL_THIS ExecuteListDeffered(In const GraphicsCommandList& cmdList, Impl::Rendering::ICopyContext* context);
							void CLOAK_CALL_THIS CreateCommandList(Out GraphicsCommandList* list, Outptr ID3D12CommandAllocator** alloc);
							void CLOAK_CALL_THIS DiscardAllocator(In ID3D12CommandAllocator* alloc, In uint64_t fence);
							void CLOAK_CALL_THIS Update();

							uint64_t CLOAK_CALL_THIS IncrementFence();
							uint64_t CLOAK_CALL_THIS ExecuteList(In const GraphicsCommandList& cmdList);
							ID3D12CommandAllocator* CLOAK_CALL_THIS RequestAllocator();
							DescriptorHeap* CLOAK_CALL_THIS GetDescriptorHeap(In HEAP_TYPE heapType) const;
						protected:
							const D3D12_COMMAND_LIST_TYPE m_type;
							const UINT m_node;
							const QUEUE_TYPE m_qtype;
							Device* const m_device;
							IManager* const m_manager;
							Queue* const m_parent;

							enum LIST_SUPPORT {
								LIST_V0 = 0,
								LIST_V1 = 1,
								LIST_V2 = 2,
								LIST_V3 = 3,
								LIST_V4 = 4,
								LIST_V5 = 5,
							};

							API::Helper::ISyncSection* m_syncFence;
							API::Helper::ISyncSection* m_syncContext;
							API::Helper::ISyncSection* m_syncExecute;
							API::Helper::ISyncSection* m_syncEvent;
							API::Helper::ISyncSection* m_syncAlloc;
							API::Helper::ISyncSection* m_syncFreeAlloc;
							API::Helper::ISyncSection* m_syncToRelease;
							uint64_t m_lastFenceCount;
							uint64_t m_nextFenceCount;
							HANDLE m_fenceEvent;
							ID3D12Fence* m_fence;
							ID3D12CommandQueue* m_queue;
							API::List<Impl::Rendering::ICopyContext*> m_contextPool;
							API::List<std::pair<ID3D12CommandList*, Impl::Rendering::ICopyContext*>> m_toExecute;
							API::Queue<Impl::Rendering::ICopyContext*> m_aviableContext;
							API::List<ID3D12CommandAllocator*> m_allocPool;
							API::Heap<std::pair<uint64_t, ID3D12CommandAllocator*>, 4> m_waitAlloc;
							API::Queue<std::pair<uint64_t, API::Helper::ISavePtr*>> m_toRelease;
							API::Queue<ID3D12CommandAllocator*> m_freeAlloc;
							API::FlatMap<IAdapter*, uint64_t> m_lastFenceWait;
							std::atomic<LIST_SUPPORT> m_supportedListVersion;
					};
					class Queue : public virtual Impl::Rendering::IQueue {
						friend class SingleQueue;
						public:
							CLOAK_CALL Queue(In Device* device, In UINT node, In size_t id, In IManager* manager, In API::Rendering::CROSS_ADAPTER_SUPPORT support);
							CLOAK_CALL ~Queue();
							void CLOAK_CALL_THIS Shutdown();
							ID3D12CommandQueue* CLOAK_CALL_THIS GetCommandQueue(In QUEUE_TYPE type);

							uint64_t CLOAK_CALL_THIS IncrementFence(In QUEUE_TYPE type) override;
							bool CLOAK_CALL_THIS IsFenceComplete(In QUEUE_TYPE type, In uint64_t val) override;
							bool CLOAK_CALL_THIS WaitForFence(In QUEUE_TYPE type, In uint64_t val) override;
							void CLOAK_CALL_THIS CreateContext(In REFIID riid, Outptr void** ptr, In QUEUE_TYPE type) override;
							uint64_t CLOAK_CALL_THIS FlushExecutions(In API::Global::Memory::PageStackAllocator* alloc, In bool wait, In QUEUE_TYPE type) override;
							UINT CLOAK_CALL_THIS GetNodeMask() const override;
							size_t CLOAK_CALL_THIS GetNodeID() const override;
							ISingleQueue* CLOAK_CALL_THIS GetSingleQueue(In QUEUE_TYPE type) override;
							IDynamicAllocatorPage* CLOAK_CALL_THIS RequestDynamicAllocatorPage(In DynamicAllocatorType type) override;
							void CLOAK_CALL_THIS CleanDynamicAllocatorPage(In IDynamicAllocatorPage* page, In bool aviable, In DynamicAllocatorType type) override;
							IDynamicAllocatorPage* CLOAK_CALL_THIS RequestCustomDynamicAllocatorPage(In DynamicAllocatorType type, In size_t size) override;
							void CLOAK_CALL_THIS CleanCustomDynamicAllocatorPage(In IDynamicAllocatorPage* page) override;
							IDynamicDescriptorHeapPage* CLOAK_CALL_THIS RequestDynamicDescriptorHeapPage() override;
							void CLOAK_CALL_THIS CleanDynamicDescriptorHeapPage(In IDynamicDescriptorHeapPage* page) override;
							IDescriptorHeap* CLOAK_CALL_THIS GetDescriptorHeap(In HEAP_TYPE heapType) const override;
							size_t CLOAK_CALL_THIS GetDynamicAllocatorPageSize(In DynamicAllocatorType type) const override;
							IReadBackPage* CLOAK_CALL_THIS RequestReadBackPage(In size_t AlignedSize) override;
							void CLOAK_CALL_THIS CleanReadBackPage(In IReadBackPage* page) override;
							void CLOAK_CALL_THIS Update() override;
							IDevice* CLOAK_CALL_THIS GetDevice() const override;

							QueryPool* CLOAK_CALL_THIS GetQueryPool();
						private:
							const UINT m_node;
							const size_t m_nodeID;
							Device* const m_device;
							SingleQueue m_graphicQueue;
							SingleQueue m_copyQueue;
							QueryPool m_queryPool;

							API::Helper::ISyncSection* m_syncFreeAllocPages;
							API::Helper::ISyncSection* m_syncWaitAllocPages;
							API::Helper::ISyncSection* m_syncHeapPages;
							API::Helper::ISyncSection* m_syncAllocPagePool;
							API::Helper::ISyncSection* m_syncHeapPagePool;
							API::Helper::ISyncSection* m_syncCustomPagePool;
							API::Helper::ISyncSection* m_syncReadBackPagePool;
							DescriptorHeap* m_descHeaps[HEAP_NUM_TYPES];
							API::List<IDynamicAllocatorPage*> m_dynAllocPagePool;
							API::List<IDynamicDescriptorHeapPage*> m_dynHeapPagePool;
							API::List<IReadBackPage*> m_readBackPagePool;
							API::Queue<IDynamicAllocatorPage*> m_freeDynAllocPages[2];
							API::Queue<IDynamicAllocatorPage*> m_waitDynAllocPages[2];
							API::Queue<IDynamicDescriptorHeapPage*> m_dynHeapPages;
							API::Queue<IDynamicAllocatorPage*> m_customPages;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif