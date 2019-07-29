#pragma once
#ifndef CE_IMPL_RENDERING_VIDEOENCODER_H
#define CE_IMPL_RENDERING_VIDEOENCODER_H

#include "CloakEngine/Helper/SavePtr.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/ColorBuffer.h"
#include "Implementation/Rendering/Context.h"

#include <string>

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace VideoEncoder_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{9EFDAF9B-46BE-4B6D-AB3D-28D64F80797D}") IVideoEncoder : public virtual API::Helper::ISavePtr {
					public:
						virtual bool CLOAK_CALL_THIS StartPlayback(In const std::u16string& file) = 0;
						virtual void CLOAK_CALL_THIS StopPlayback() = 0;
						virtual bool CLOAK_CALL_THIS IsVideoPlaying() const = 0;
						virtual void CLOAK_CALL_THIS RenderVideo(In API::Rendering::IContext* context, In API::Rendering::IColorBuffer* dst) = 0;
						virtual void CLOAK_CALL_THIS OnVideoEvent() = 0;
						virtual void CLOAK_CALL_THIS OnWindowResize(In LPRECT rct) = 0;
				};
			}
		}
	}
}

#endif