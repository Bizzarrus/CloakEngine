#pragma once
#ifndef CE_IMPL_RENDERING_QUEUE_H
#define CE_IMPL_RENDERING_QUEUE_H

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/Allocator.h"
#include "Implementation/Rendering/Adapter.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Queue_v1 {
				class IQueue;
				RENDERING_INTERFACE ISingleQueue : public IAdapter {
					public:
						virtual IQueue* CLOAK_CALL_THIS GetParent() const = 0;
						virtual API::Rendering::IVertexBuffer* CLOAK_CALL_THIS CreateVertexBuffer(In UINT vertexCount, In UINT vertexSize, In_reads(vertexCount*vertexSize) const void* data = nullptr) const = 0;
						virtual API::Rendering::IIndexBuffer* CLOAK_CALL_THIS CreateIndexBuffer(In UINT size, In_reads(size) const uint16_t* data) const = 0;
						virtual API::Rendering::IIndexBuffer* CLOAK_CALL_THIS CreateIndexBuffer(In UINT size, In_reads(size) const uint32_t* data) const = 0;
						virtual API::Rendering::IConstantBuffer* CLOAK_CALL_THIS CreateConstantBuffer(In UINT elementSize, In_reads_opt(elementSize) const void* data = nullptr) const = 0;
						virtual API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS CreateStructuredBuffer(In UINT elementCount, In UINT elementSize, In_reads(elementSize) const void* data, In_opt bool counterBuffer = true) const = 0;
						virtual API::Rendering::IStructuredBuffer* CLOAK_CALL_THIS CreateStructuredBuffer(In UINT elementCount, In UINT elementSize, In_opt bool counterBuffer, In_reads_opt(elementSize) const void* data = nullptr) const = 0;
				};
				RENDERING_INTERFACE IQueue{
					public:
						virtual uint64_t CLOAK_CALL_THIS IncrementFence(In QUEUE_TYPE type) = 0;
						virtual bool CLOAK_CALL_THIS IsFenceComplete(In QUEUE_TYPE type, In uint64_t val) = 0;
						virtual bool CLOAK_CALL_THIS WaitForFence(In QUEUE_TYPE type, In uint64_t val) = 0;
						virtual void CLOAK_CALL_THIS CreateContext(In REFIID riid, Outptr void** ptr, In QUEUE_TYPE type) = 0;
						virtual uint64_t CLOAK_CALL_THIS FlushExecutions(In API::Global::Memory::PageStackAllocator* alloc, In bool wait, In QUEUE_TYPE type) = 0;
						virtual UINT CLOAK_CALL_THIS GetNodeMask() const = 0;
						virtual ISingleQueue* CLOAK_CALL_THIS GetSingleQueue(In QUEUE_TYPE type) = 0;
						virtual IDynamicAllocatorPage* CLOAK_CALL_THIS RequestDynamicAllocatorPage(In DynamicAllocatorType type) = 0;
						virtual void CLOAK_CALL_THIS CleanDynamicAllocatorPage(In IDynamicAllocatorPage* page, In bool aviable, In DynamicAllocatorType type) = 0;
						virtual IDynamicAllocatorPage* CLOAK_CALL_THIS RequestCustomDynamicAllocatorPage(In DynamicAllocatorType type, In size_t size) = 0;
						virtual void CLOAK_CALL_THIS CleanCustomDynamicAllocatorPage(In IDynamicAllocatorPage* page) = 0;
						virtual IDynamicDescriptorHeapPage* CLOAK_CALL_THIS RequestDynamicDescriptorHeapPage() = 0;
						virtual void CLOAK_CALL_THIS CleanDynamicDescriptorHeapPage(In IDynamicDescriptorHeapPage* page) = 0;
						virtual size_t CLOAK_CALL_THIS GetNodeID() const = 0;
						virtual IDescriptorHeap* CLOAK_CALL_THIS GetDescriptorHeap(In HEAP_TYPE heapType) const = 0;
						virtual size_t CLOAK_CALL_THIS GetDynamicAllocatorPageSize(In DynamicAllocatorType type) const = 0;
						virtual IReadBackPage* CLOAK_CALL_THIS RequestReadBackPage(In size_t AlignedSize) = 0;
						virtual void CLOAK_CALL_THIS CleanReadBackPage(In IReadBackPage* page) = 0;
						virtual void CLOAK_CALL_THIS Update() = 0;
						virtual IDevice* CLOAK_CALL_THIS GetDevice() const = 0;
				};
			}
		}
	}
}

#endif