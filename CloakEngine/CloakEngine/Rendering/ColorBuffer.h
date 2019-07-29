#pragma once
#ifndef CE_API_RENDERING_COLORBUFFER_H
#define CE_API_RENDERING_COLORBUFFER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/BasicBuffer.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace Context_v1 {
				CLOAK_INTERFACE IContext;
				CLOAK_INTERFACE IComputeContext;
			}
			inline namespace ColorBuffer_v1 {
				CLOAK_INTERFACE_ID("{E85004C9-A8F0-4771-9E6D-543097ADB77C}") IColorBuffer : public virtual IPixelBuffer{
					public:
						virtual ResourceView CLOAK_CALL_THIS GetSRV(In_opt bool cube = false) const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetUAV(In_opt UINT num = 0) const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetDynamicSRV(In_opt bool cube = false) const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetDynamicUAV(In_opt UINT num = 0) const = 0;
						virtual ResourceView CLOAK_CALL_THIS GetRTV(In_opt UINT num = 0) const = 0;
						virtual const Helper::Color::RGBA& CLOAK_CALL_THIS GetClearColor() const = 0;
						virtual void CLOAK_CALL_THIS Recreate(In const TEXTURE_DESC& desc) = 0;
				};
			}
		}
	}
}

#endif