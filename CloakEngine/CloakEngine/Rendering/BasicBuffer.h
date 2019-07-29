#pragma once
#ifndef CE_API_RENDERING_BASICBUFFER_H
#define CE_API_RENDERING_BASICBUFFER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/Resource.h"
#include "CloakEngine/Rendering/Allocator.h"
#include "CloakEngine/Rendering/Defines.h"
#include "CloakEngine/Helper/Color.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace BasicBuffer_v1 {
				CLOAK_INTERFACE_ID("{D20F9678-4620-41CE-9B92-77910F357831}") IPixelBuffer : public virtual IResource {
					public:
						virtual uint32_t CLOAK_CALL_THIS GetWidth() const = 0;
						virtual uint32_t CLOAK_CALL_THIS GetHeight() const = 0;
						virtual uint32_t CLOAK_CALL_THIS GetDepth() const = 0;
						virtual uint32_t CLOAK_CALL_THIS GetSampleCount() const = 0;
						virtual Format CLOAK_CALL_THIS GetFormat() const = 0;
						virtual UINT CLOAK_CALL_THIS GetSubresourceIndex(In UINT mipMap, In UINT arrayIndex = 0, In_opt UINT planeSlice = 0) const = 0;
				};
				CLOAK_INTERFACE_ID("{B4EE9800-691B-41B3-B66D-6AF37AF8A12E}") IGraphicBuffer : public virtual IResource {
					public:
						virtual const GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS GetGPUAddress() const = 0;
						virtual uint32_t CLOAK_CALL_THIS GetSize() const = 0;
				};
			}
		}
	}
}

#endif