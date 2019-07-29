#pragma once
#ifndef CE_IMPL_RENDEIRNG_BASICBUFFER_H
#define CE_IMPL_RENDEIRNG_BASICBUFFER_H

#include "CloakEngine/Rendering/BasicBuffer.h"
#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/Resource.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace BasicBuffer_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{EF858335-6B3F-458E-A98E-83FA25C16148}") IPixelBuffer : public virtual Impl::Rendering::IResource, public virtual API::Rendering::IPixelBuffer {
					public:

				};
				RENDERING_INTERFACE CLOAK_UUID("{D95B9F3D-2409-48D4-8D9E-78951F80E95E}") IGraphicBuffer : public virtual Impl::Rendering::IResource, public virtual API::Rendering::IGraphicBuffer {
					public:

				};
			}
		}
	}
}

#pragma warning(pop)

#endif