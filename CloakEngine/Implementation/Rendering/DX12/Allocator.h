#pragma once
#ifndef CE_IMPL_RENDERING_DX12_ALLOCATOR_H
#define CE_IMPL_RENDERING_DX12_ALLOCATOR_H

#include "CloakEngine/Rendering/Context.h"

#include "Implementation/Rendering/Allocator.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Rendering/DescriptorHandle.h"
#include "Implementation/Rendering/Queue.h"

#include "Implementation/Rendering/DX12/Resource.h"

#include "Engine/BitMap.h"

#include <d3d12.h>
#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace Allocator_v1 {
					class CLOAK_UUID("{B1D7699A-5F42-445B-95B4-CD377F13CFD8}") Allocator : public virtual Impl::Rendering::IAllocator, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL Allocator();
							virtual CLOAK_CALL ~Allocator();
							virtual void CLOAK_CALL_THIS PushStack() override;
							virtual void CLOAK_CALL_THIS PopStack() override;
							virtual API::Rendering::GPU_ADDRESS CLOAK_CALL_THIS Alloc(In size_t size, In size_t allign) override;
							virtual intptr_t CLOAK_CALL_THIS GetSizeOfFreeSpace() const override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
					};
					class CLOAK_UUID("{526B28C0-C5C4-4935-8DD9-EC2E48194409}") DescriptorHeap : public virtual Impl::Rendering::IDescriptorHeap, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL DescriptorHeap(In Impl::Rendering::HEAP_TYPE Type, In IDevice* device, In UINT node);
							virtual CLOAK_CALL ~DescriptorHeap();
							virtual size_t CLOAK_CALL_THIS Allocate(Inout_updates(numHandles) API::Rendering::ResourceView Handles[], In UINT numHandles, In_opt API::Rendering::DescriptorPageType type = API::Rendering::DescriptorPageType::UTILITY) override;
							virtual void CLOAK_CALL_THIS RegisterUsage(In const API::Rendering::ResourceView& handle, In Impl::Rendering::IResource* rsc, In size_t count) override;
							virtual void CLOAK_CALL_THIS Free(In const API::Rendering::ResourceView& Handle) override;
							virtual void CLOAK_CALL_THIS Free(In_reads(numHandles) API::Rendering::ResourceView Handles[], In UINT numHandles) override;
							virtual void CLOAK_CALL_THIS Shutdown() override;
							virtual API::Global::Debug::Error CLOAK_CALL_THIS GetOwner(In const API::Rendering::ResourceView& handle, In REFIID riid, Outptr void** ppvObject, In_opt UINT offset = 0) override;
							virtual void CLOAK_CALL_THIS GetOwner(In const API::Rendering::ResourceView& handle, In size_t count, Out_writes(count) Impl::Rendering::IResource** resources, In_opt UINT offset = 0) override;

							Ret_maybenull virtual ID3D12DescriptorHeap* CLOAK_CALL_THIS GetHeap(In size_t heap);
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							struct DescriptorView {
								CLOAK_CALL DescriptorView(In IDevice* dev, In Impl::Rendering::HEAP_TYPE type, In API::Rendering::DescriptorPageType pageType, In UINT node);
								CLOAK_CALL ~DescriptorView();
								void* CLOAK_CALL operator new(In size_t size);
								void CLOAK_CALL operator delete(In void* ptr);

								const API::Rendering::DescriptorPageType PageType;
								const size_t Size;
								ID3D12DescriptorHeap* Heap;
								Engine::BitMap Assigned;
								Impl::Rendering::IResource** Assignment;
								uint32_t Space;
							};
							const UINT m_node;
							API::List<DescriptorView*> m_descriptors;
							API::Helper::ISyncSection* m_sync;
							IDevice* m_dev;
							Impl::Rendering::HEAP_TYPE m_type;
							uint32_t m_typeSize;
					};
					class CLOAK_UUID("{5F567877-43E3-4384-A7A8-6203A1E4DCC8}") DynamicDescriptorHeapPage : public virtual Impl::Rendering::IDynamicDescriptorHeapPage, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL DynamicDescriptorHeapPage(In Device* device, In IQueue* queue);
							virtual CLOAK_CALL ~DynamicDescriptorHeapPage();

							const DescriptorHandle& CLOAK_CALL_THIS GetHandle() const override;
							void CLOAK_CALL_THIS Retire(In uint64_t fenceValue, In QUEUE_TYPE type) override;
							bool CLOAK_CALL_THIS IsAviable() override;
							ID3D12DescriptorHeap* CLOAK_CALL_THIS GetHeap() const;
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							API::Helper::ISyncSection* m_sync;
							IQueue* m_queue;
							QUEUE_TYPE m_retireType;
							uint64_t m_retireFence;
							std::atomic<bool> m_aviable;
							DescriptorHandle m_handle;
							ID3D12DescriptorHeap* m_heap;
					};
					class CLOAK_UUID("{D5A8517F-C1EA-4D6B-93A0-6B2CD609E2CB}") DynamicDescriptorHeap : public virtual Impl::Rendering::IDynamicDescriptorHeap, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							static uint32_t CLOAK_CALL GetDescriptorSize();

							CLOAK_CALL DynamicDescriptorHeap(In API::Rendering::IContext* context, In IQueue* queue);
							virtual CLOAK_CALL ~DynamicDescriptorHeap();
							void CLOAK_CALL_THIS CleanUsedHeaps(In uint64_t fenceVal, In QUEUE_TYPE queue) override;
							void CLOAK_CALL_THIS SetGraphicDescriptorHandles(In UINT rootIndex, In UINT offset, In UINT numHandles, In_reads(numHandles) const API::Rendering::CPU_DESCRIPTOR_HANDLE Handles[]) override;
							void CLOAK_CALL_THIS SetComputeDescriptorHandles(In UINT rootIndex, In UINT offset, In UINT numHandles, In_reads(numHandles) const API::Rendering::CPU_DESCRIPTOR_HANDLE Handles[]) override;
							void CLOAK_CALL_THIS ParseGraphicRootSignature(In IRootSignature* sig) override;
							void CLOAK_CALL_THIS ParseComputeRootSignature(In IRootSignature* sig) override;

							void CLOAK_CALL_THIS CommitGraphicDescriptorTables(In ID3D12GraphicsCommandList* cmdList);
							void CLOAK_CALL_THIS CommitComputeDescriptorTables(In ID3D12GraphicsCommandList* cmdList);
							D3D12_GPU_DESCRIPTOR_HANDLE CLOAK_CALL_THIS UploadDirect(In const D3D12_CPU_DESCRIPTOR_HANDLE& handle);
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							struct DescriptorTableCache {
								CLOAK_CALL DescriptorTableCache();
								uint32_t AssignedBitMap;
								API::Rendering::CPU_DESCRIPTOR_HANDLE* TableStart;
								uint32_t TableSize;
							};
							struct DescriptorHandleCache {
								static constexpr uint32_t MaxNumDescriptors = 256;
								static constexpr uint32_t MaxNumDescriptorTables = MAX_ROOT_PARAMETERS;
								CLOAK_CALL DescriptorHandleCache();
								void CLOAK_CALL_THIS ClearCache();
								void CLOAK_CALL_THIS CopyAndBindTables(In DescriptorHandle dstHandleStart, In ID3D12GraphicsCommandList* cmdList, In void(STDMETHODCALLTYPE ID3D12GraphicsCommandList::*func)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));
								void CLOAK_CALL_THIS UnbindAll();
								void CLOAK_CALL_THIS StageDescriptorHandles(In UINT rootIndex, In UINT offset, In UINT numHandles, In_reads(numHandles) const API::Rendering::CPU_DESCRIPTOR_HANDLE* handles);
								void CLOAK_CALL_THIS ParseRootSignature(In IRootSignature* sig);
								uint32_t CLOAK_CALL_THIS ComputeSize();
								uint32_t RootDescriptorBitMap;
								uint32_t RootParamsBitMap;
								uint32_t MaxCachedDescriptors;
								DescriptorTableCache DescriptorTables[MaxNumDescriptorTables];
								API::Rendering::CPU_DESCRIPTOR_HANDLE HandleCache[MaxNumDescriptors];
							};
							bool CLOAK_CALL_THIS HasSpace(In uint32_t count);
							void CLOAK_CALL_THIS RetireCurrentHeap();
							void CLOAK_CALL_THIS RetireUsedHeaps(In uint64_t fenceVal, In QUEUE_TYPE qtype);
							void CLOAK_CALL_THIS CopyAndBindStages(Inout DescriptorHandleCache& cache, In ID3D12GraphicsCommandList* cmdList, In void(STDMETHODCALLTYPE ID3D12GraphicsCommandList::*func)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));
							void CLOAK_CALL_THIS UnbindAll();
							Ret_notnull ID3D12DescriptorHeap* CLOAK_CALL_THIS GetHeap();
							DescriptorHandle CLOAK_CALL_THIS Allocate(In UINT count);

							IQueue* m_queue;
							API::Rendering::IContext* m_context;
							DynamicDescriptorHeapPage* m_curHeap;
							uint32_t m_curOffset;
							DescriptorHandle m_handle;
							API::List<DynamicDescriptorHeapPage*> m_heaps;
							DescriptorHandleCache m_GraphicHandleCache;
							DescriptorHandleCache m_ComputeHandleCache;
					};
					class CLOAK_UUID("{1D609104-0788-473C-8B5F-068A137EBCC9}") DynamicAllocatorPage : public Resource, public virtual IDynamicAllocatorPage {
						public:
							CLOAK_CALL DynamicAllocatorPage(In ID3D12Resource* base, In DynamicAllocatorType type, In IQueue* queue, In API::Rendering::RESOURCE_STATE states, In bool canTransition, In size_t size);
							virtual CLOAK_CALL ~DynamicAllocatorPage();
							bool CLOAK_CALL_THIS IsAviable() override;
							bool CLOAK_CALL_THIS TryToAllocate(Inout DynAlloc* alloc, In size_t SizeAligned, In_opt size_t Alignment = DEFAULT_ALIGN) override;
							void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) override;
							void CLOAK_CALL_THIS Map(In void** data) override;
							void CLOAK_CALL_THIS Unmap() override;
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							const size_t m_size;
							const DynamicAllocatorType m_type;
							IQueue* const m_queue;
							size_t m_offset;
							uint64_t m_fence[2];
							std::atomic<bool> m_aviable;
							bool m_reqClean;
					};
					class CLOAK_UUID("{465E9BE9-D27C-4806-ACDC-3EB920BC53D7}") CustomDynamicAllocatorPage : public Resource, public virtual IDynamicAllocatorPage {
						public:
							CLOAK_CALL CustomDynamicAllocatorPage(In ID3D12Resource* base, In DynamicAllocatorType type, In IQueue* queue, In API::Rendering::RESOURCE_STATE states, In bool canTransition, In size_t size);
							virtual CLOAK_CALL ~CustomDynamicAllocatorPage();
							bool CLOAK_CALL_THIS IsAviable() override;
							bool CLOAK_CALL_THIS TryToAllocate(Inout DynAlloc* alloc, In size_t SizeAligned, In_opt size_t Alignment = DEFAULT_ALIGN) override;
							void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) override;
							void CLOAK_CALL_THIS Map(In void** data) override;
							void CLOAK_CALL_THIS Unmap() override;
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							const size_t m_size;
							const DynamicAllocatorType m_type;
							IQueue* const m_queue;
							uint64_t m_fence;
							QUEUE_TYPE m_qt;
							std::atomic<bool> m_aviable;
					};
					class CLOAK_UUID("{AD08AC8A-9880-4535-B3F0-DCF129755AE2}") DynamicAllocator : public virtual IDynamicAllocator, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL DynamicAllocator(In DynamicAllocatorType type, In IQueue* queue);
							virtual CLOAK_CALL ~DynamicAllocator();
							void CLOAK_CALL_THIS Allocate(Inout DynAlloc* alloc, In size_t SizeInByte, In_opt size_t Alignment = DEFAULT_ALIGN) override;
							void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) override;
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							IQueue* const m_queue;
							const DynamicAllocatorType m_type;
							API::List<IDynamicAllocatorPage*> m_pages;
							IDynamicAllocatorPage* m_lastPage;
					};
					class CLOAK_UUID("{E61A4155-C38A-447F-A330-3492915A5AB3}") ReadBackPage : public Resource, public virtual IReadBackPage {
						public:
							CLOAK_CALL ReadBackPage(In ID3D12Resource* base, In size_t size, In IQueue* queue);
							virtual CLOAK_CALL ~ReadBackPage();

							size_t CLOAK_CALL_THIS GetFreeSize() override;
							bool CLOAK_CALL_THIS TryToAllocate(In size_t SizeAligned, In UINT rowSize, In UINT rowPadding, Out size_t* Offset, In_opt size_t Alignment = DEFAULT_ALIGN) override;
							void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) override;

							void CLOAK_CALL_THIS ReadData(In size_t offset, In size_t size, Out_writes_bytes(size) void* res) override;
							void CLOAK_CALL_THIS AddReadCount(In size_t offset) override;
							void CLOAK_CALL_THIS RemoveReadCount(In size_t offset) override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							enum class CleanState {
								AVIABLE,
								WAIT_FENCE,
								WAIT_READ,
							};
							struct Entry {
								size_t Size;
								size_t Offset;
								size_t FullSize;
								UINT RowSize;
								UINT RowPadding;
								uint64_t ReadCount;
								uint64_t Fence;
								QUEUE_TYPE Queue;
							};

							const size_t m_size;
							IQueue* const m_queue;
							size_t m_begin;
							size_t m_remSize;
							API::Queue<Entry> m_aviable;
							API::Helper::IConditionSyncSection* m_syncClean;
					};
					class CLOAK_UUID("{D599AFB6-2DF6-4A5D-B177-F9344645CD97}") CustomReadBackPage : public Resource, public virtual IReadBackPage {
						public:
							CLOAK_CALL CustomReadBackPage(In ID3D12Resource* base, In size_t size, In IQueue* queue);
							virtual CLOAK_CALL ~CustomReadBackPage();

							size_t CLOAK_CALL_THIS GetFreeSize() override;
							bool CLOAK_CALL_THIS TryToAllocate(In size_t SizeAligned, In UINT rowSize, In UINT rowPadding, Out size_t* Offset, In_opt size_t Alignment = DEFAULT_ALIGN) override;
							void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) override;

							void CLOAK_CALL_THIS ReadData(In size_t offset, In size_t size, Out_writes_bytes(size) void* res) override;
							void CLOAK_CALL_THIS AddReadCount(In size_t offset) override;
							void CLOAK_CALL_THIS RemoveReadCount(In size_t offset) override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							const size_t m_size;
							IQueue* const m_queue;
							bool m_aviable;
							UINT m_rowSize;
							UINT m_rowPadding;
							uint64_t m_fence;
							QUEUE_TYPE m_qt;
							API::Helper::IConditionSyncSection* m_syncClean;
					};
					class CLOAK_UUID("{B7D283E2-1DD0-432E-BA74-1CFEFF944B58}") ReadBackAllocator : public virtual IReadBackAllocator, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL ReadBackAllocator(In IQueue* queue);
							virtual CLOAK_CALL ~ReadBackAllocator();
							API::Rendering::ReadBack CLOAK_CALL_THIS Allocate(In size_t SizeInByte, In UINT rowSize, In UINT rowPadding, In_opt size_t Alignment = DEFAULT_ALIGN) override;
							void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) override;
						protected:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							IQueue* const m_queue;
							IReadBackPage* m_curPage;
							API::List<IReadBackPage*> m_lastPages;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif