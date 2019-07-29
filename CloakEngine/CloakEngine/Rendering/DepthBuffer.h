#pragma once
#ifndef CE_API_RENDERING_DEPTHBUFFER_H
#define CE_API_RENDERING_DEPTHBUFFER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/BasicBuffer.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace DepthBuffer_v1 {
				CLOAK_INTERFACE_ID("{2D0359A1-4F0B-4BC6-809F-7782D947422D}") IDepthBuffer : public virtual IPixelBuffer{
					public:
						virtual ResourceView CLOAK_CALL_THIS GetDSV(In_opt DSV_ACCESS access = DSV_ACCESS::NONE) const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetDepthSRV() const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetStencilSRV() const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetDynamicDepthSRV() const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetDynamicStencilSRV() const = 0;
						virtual float CLOAK_CALL_THIS GetClearDepth() const = 0;
						virtual uint32_t CLOAK_CALL_THIS GetClearStencil() const = 0;
						virtual void CLOAK_CALL_THIS Recreate(In const DEPTH_DESC& desc) = 0;
				};
			}
		}
	}
}

#endif