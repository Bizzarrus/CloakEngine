#pragma once
#ifndef CE_IMPL_RENDEIRNG_COLORBUFFER_H
#define CE_IMPL_RENDEIRNG_COLORBUFFER_H

#include "CloakEngine/Rendering/ColorBuffer.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/BasicBuffer.h"
#include "Implementation/Rendering/SwapChain.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace ColorBuffer_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{8D426885-CEE9-47A0-9C3A-431DB6D1CA00}") IColorBuffer : public virtual IPixelBuffer, public virtual API::Rendering::IColorBuffer {
					public:
						virtual void CLOAK_CALL_THIS SetDetailLevel(In uint32_t lvl) = 0;
				};
			}
		}
	}
}

#endif