#pragma once
#ifndef CE_IMPL_RENDERING_SAMPLER_H
#define CE_IMPL_RENDERING_SAMPLER_H

#include "Implementation/Rendering/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"

#define SAMPLER_MAX_MIP_CLAMP 4

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Sampler_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{C57A8684-8930-48C5-9057-DA3012CAFABE}") ISampler : public virtual API::Helper::ISavePtr {
					public:
						virtual UINT CLOAK_CALL_THIS GetRootIndex() const = 0;
						virtual const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS GetGPUAddress(In_opt UINT minMipLevel = 0) const = 0;
				};
			}
		}
	}
}

#endif