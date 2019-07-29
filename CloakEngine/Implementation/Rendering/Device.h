#pragma once
#ifndef CE_IMPL_RENDERING_DEVICE_H
#define CE_IMPL_RENDERING_DEVICE_H

#include "CloakEngine/Helper/SavePtr.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/Resource.h"
#include "Implementation/Rendering/ColorBuffer.h"
#include "Implementation/Rendering/RootSignature.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Device_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{78AC56E0-E614-47FB-B7F7-54B0D0BC19AD}") IDevice : public virtual API::Helper::ISavePtr{
					public:
						virtual uint32_t CLOAK_CALL_THIS GetDescriptorHandleIncrementSize(In HEAP_TYPE type) const = 0;
						virtual void CLOAK_CALL_THIS CreateConstantBufferView(In const CBV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) const = 0;
						virtual void CLOAK_CALL_THIS CreateShaderResourceView(In IResource* rsc, In const SRV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) = 0;
						virtual void CLOAK_CALL_THIS CreateRenderTargetView(In IResource* rsc, In const RTV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) = 0;
						virtual void CLOAK_CALL_THIS CreateDepthStencilView(In IResource* rsc, In const DSV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) = 0;
						virtual void CLOAK_CALL_THIS CreateUnorderedAccessView(In IResource* rsc, In IResource* byteAddress, In const UAV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) = 0;
						virtual void CLOAK_CALL_THIS CreateSampler(In const API::Rendering::SAMPLER_DESC& desc, In API::Rendering::CPU_DESCRIPTOR_HANDLE handle) = 0;
						virtual void CLOAK_CALL_THIS CopyDescriptorsSimple(In UINT numDescriptors, In const API::Rendering::CPU_DESCRIPTOR_HANDLE& dstStart, In const API::Rendering::CPU_DESCRIPTOR_HANDLE& srcStart, In HEAP_TYPE heap) = 0;
						virtual void CLOAK_CALL_THIS CopyDescriptors(In UINT numDstRanges, In_reads(numDstRanges) const API::Rendering::CPU_DESCRIPTOR_HANDLE* dstRangeStarts, In_reads_opt(numDstRanges) const UINT* dstRangeSizes, In UINT numSrcRanges, In_reads(numSrcRanges) const API::Rendering::CPU_DESCRIPTOR_HANDLE* srcRangeStarts, In_reads_opt(numSrcRanges) const UINT* srcRangeSizes, In HEAP_TYPE heap) = 0;
						virtual HRESULT CLOAK_CALL_THIS CreateRootSignature(In const void* data, In size_t dataSize, In REFIID riid, Out void** ppvObject) = 0;
				};
			}
		}
	}
}

#endif