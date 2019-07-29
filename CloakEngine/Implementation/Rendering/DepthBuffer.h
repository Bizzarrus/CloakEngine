#pragma once
#ifndef CE_IMPL_RENDEIRNG_DEPTHBUFFER_H
#define CE_IMPL_RENDEIRNG_DEPTHBUFFER_H

#include "CloakEngine/Rendering/DepthBuffer.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/BasicBuffer.h"

#define VIEWTYPE_DEPTH_DEFAULT CloakEngine::API::Rendering::VIEW_TYPE::DSV

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace DepthBuffer_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{E1B003AE-C26A-48DD-A343-5C29931A9679}") IDepthBuffer : public virtual IPixelBuffer, public virtual API::Rendering::IDepthBuffer {
					public:
						
				};
			}
		}
	}
}

#endif