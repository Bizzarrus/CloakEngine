#pragma once
#ifndef CE_IMPL_RENDEIRNG_ALLOCATOR_H
#define CE_IMPL_RENDEIRNG_ALLOCATOR_H

#include "CloakEngine/Rendering/Allocator.h"
#include "CloakEngine/Helper/SavePtr.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/Device.h"
#include "Implementation/Rendering/RootSignature.h"
#include "Implementation/Rendering/Resource.h"
#include "Implementation/Rendering/DescriptorHandle.h"

#include <memory>

#define DEFAULT_ALIGN 256

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Allocator_v1 {
				template<typename T> class AlignedAllocator {
					private:
						BYTE* m_data = nullptr;
						T* m_d = nullptr;
						size_t m_c = 0;
						size_t m_os = 0;
						size_t m_bs = 0;
					public:
						CLOAK_CALL AlignedAllocator(In_reads(count) const T* data, In size_t count, In size_t align)
						{
							size_t m = align - 1;
							m_os = (sizeof(T) + m) & (~m);
							m_bs = (m_os*count) + m;
							m_data = NewArray(BYTE, m_bs);
							void* p = m_data;
							if (std::align(align, m_os, p, m_bs))
							{
								m_d = reinterpret_cast<T*>(p);
								m_c = count;
								for (size_t a = 0; a < count; a++)
								{
									m_d[a] = data[a];
								}
							}
						}
						CLOAK_CALL ~AlignedAllocator() { DeleteArray(m_data); }
						size_t CLOAK_CALL_THIS GetByteSize() const { return m_bs; }
						T* CLOAK_CALL_THIS Get() const { return m_d; }
						T* CLOAK_CALL_THIS Get(In size_t pos) const
						{
							if (pos < m_c) { return (T*)(((BYTE*)m_d) + (pos*m_os)); }
							return nullptr;
						}
				};
				enum class DynamicAllocatorType { GPU = 0, CPU = 1 };
				enum class LinearAllocatorType { INVALID = -1, GPU_ONLY = 0, CPU_WRITEABLE = 1 };

				RENDERING_INTERFACE CLOAK_UUID("{FFB1E6D6-2A89-4659-BBC1-5E0CE06D46A6}") IAllocator : public virtual API::Rendering::IAllocator{
					public:

				};
				RENDERING_INTERFACE CLOAK_UUID("{0B6C6E24-562D-4A27-8064-46E1C65574EE}") IDescriptorHeap : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual size_t CLOAK_CALL_THIS Allocate(Inout_updates(numHandles) API::Rendering::ResourceView Handles[], In UINT numHandles, In_opt API::Rendering::DescriptorPageType type = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS RegisterUsage(In const API::Rendering::ResourceView& handle, In Impl::Rendering::IResource* rsc, In size_t count) = 0;
						virtual void CLOAK_CALL_THIS Free(In const API::Rendering::ResourceView& Handle) = 0;
						virtual void CLOAK_CALL_THIS Free(In_reads(numHandles) API::Rendering::ResourceView Handles[], In UINT numHandles) = 0;
						virtual void CLOAK_CALL_THIS Shutdown() = 0;
						virtual API::Global::Debug::Error CLOAK_CALL_THIS GetOwner(In const API::Rendering::ResourceView& handle, In REFIID riid, Outptr void** ppvObject, In_opt UINT offset = 0) = 0;
						virtual void CLOAK_CALL_THIS GetOwner(In const API::Rendering::ResourceView& handle, In size_t count, Out_writes(count) Impl::Rendering::IResource** resources, In_opt UINT offset = 0) = 0;
				};
				RENDERING_INTERFACE CLOAK_UUID("{1C312A3B-E73E-40B5-B7EA-44D52ECEFF3B}") IDynamicDescriptorHeapPage : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual const DescriptorHandle& CLOAK_CALL_THIS GetHandle() const = 0;
						virtual void CLOAK_CALL_THIS Retire(In uint64_t fenceValue, In QUEUE_TYPE type) = 0;
						virtual bool CLOAK_CALL_THIS IsAviable() = 0;
				};
				RENDERING_INTERFACE CLOAK_UUID("{E799FD38-83A0-46BF-A278-65C84D061E45}") IDynamicDescriptorHeap : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS CleanUsedHeaps(In uint64_t fenceVal, In QUEUE_TYPE queue) = 0;
						virtual void CLOAK_CALL_THIS SetGraphicDescriptorHandles(In UINT rootIndex, In UINT offset, In UINT numHandles, In_reads(numHandles) const API::Rendering::CPU_DESCRIPTOR_HANDLE Handles[]) = 0;
						virtual void CLOAK_CALL_THIS SetComputeDescriptorHandles(In UINT rootIndex, In UINT offset, In UINT numHandles, In_reads(numHandles) const API::Rendering::CPU_DESCRIPTOR_HANDLE Handles[]) = 0;
						virtual void CLOAK_CALL_THIS ParseGraphicRootSignature(In IRootSignature* sig) = 0;
						virtual void CLOAK_CALL_THIS ParseComputeRootSignature(In IRootSignature* sig) = 0;
				};
				struct DynAlloc;
				RENDERING_INTERFACE CLOAK_UUID("{CBDAC037-31FB-4928-A40E-1F19E1B443A0}") IDynamicAllocatorPage : public virtual IResource{
					public:
						virtual bool CLOAK_CALL_THIS IsAviable() = 0;
						virtual bool CLOAK_CALL_THIS TryToAllocate(Inout DynAlloc* alloc, In size_t SizeAligned, In_opt size_t Alignment = DEFAULT_ALIGN) = 0;
						virtual void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) = 0;
						virtual void CLOAK_CALL_THIS Map(In void** data) = 0;
						virtual void CLOAK_CALL_THIS Unmap() = 0;
				};
				struct DynAlloc {
					CLOAK_CALL DynAlloc();
					CLOAK_CALL ~DynAlloc();
					void CLOAK_CALL_THIS Initialize(In IDynamicAllocatorPage* base, In size_t offset, In size_t size);
					IDynamicAllocatorPage* Buffer;
					size_t Offset;
					size_t Size;
					void* Data;
					API::Rendering::GPU_ADDRESS GPUAddress;
				};
				RENDERING_INTERFACE CLOAK_UUID("{D1995CE7-EB22-4867-8A3A-411DE1157193}") IDynamicAllocator : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS Allocate(Inout DynAlloc* alloc, In size_t SizeInByte, In_opt size_t Alignment = DEFAULT_ALIGN) = 0;
						virtual void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) = 0;
				};

				RENDERING_INTERFACE CLOAK_UUID("{CEA5FDEB-FC2D-4AA1-A82C-5676D36C0297}") IReadBackPage : public virtual API::Rendering::Allocator_v1::IReadBackPage {
					public:
						virtual size_t CLOAK_CALL_THIS GetFreeSize() = 0;
						virtual bool CLOAK_CALL_THIS TryToAllocate(In size_t SizeAligned, In UINT rowSize, In UINT rowPadding, Out size_t* Offset, In_opt size_t Alignment = DEFAULT_ALIGN) = 0;
						virtual void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) = 0;
				};
				RENDERING_INTERFACE CLOAK_UUID("{28A9A848-73BB-4850-8BBF-C4C22C69FBE7}") IReadBackAllocator : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual API::Rendering::ReadBack CLOAK_CALL_THIS Allocate(In size_t SizeInByte, In UINT rowSize, In UINT rowPadding, In_opt size_t Alignment = DEFAULT_ALIGN) = 0;
						virtual void CLOAK_CALL_THIS Clean(In uint64_t fenceID, In QUEUE_TYPE queue) = 0;
				};
			}
		}
	}
}

#endif