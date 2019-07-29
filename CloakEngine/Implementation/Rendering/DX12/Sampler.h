#pragma once
#ifndef CE_IMPL_RENDERING_DX12_SAMPLER_H
#define CE_IMPL_RENDERING_DX12_SAMPLER_H

#include "Implementation/Rendering/Sampler.h"
#include "Implementation/Rendering/Device.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Rendering/DX12/Allocator.h"

#include <d3d12.h>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				class CLOAK_UUID("{AC6392D2-6CB3-4544-AE8F-E4F4880C7C0E}") Sampler : public virtual Impl::Rendering::ISampler, public Impl::Helper::SavePtr {
					public:	
						CLOAK_CALL Sampler(In IDescriptorHeap* heap, In API::Rendering::ResourceView handles[SAMPLER_MAX_MIP_CLAMP], In const API::Rendering::SAMPLER_DESC& desc, In UINT rootID, In IDevice* device);
						CLOAK_CALL ~Sampler();
						virtual UINT CLOAK_CALL_THIS GetRootIndex() const override;
						virtual const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS GetGPUAddress(In_opt UINT minMipLevel = 0) const override;
					protected:
						API::Rendering::ResourceView m_handles[SAMPLER_MAX_MIP_CLAMP];
						IDescriptorHeap* m_heap;
						UINT m_rootID;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif