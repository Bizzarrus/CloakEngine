#pragma once
#ifndef CE_IMPL_GLOBAL_VIDEO_H
#define CE_IMPL_GLOBAL_VIDEO_H

#include "CloakEngine/Defines.h"
#include "Implementation/Rendering/ColorBuffer.h"
#include "Implementation/Rendering/VideoEncoder.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Video {
				void CLOAK_CALL Initialize(In Impl::Rendering::IVideoEncoder* encoder);
				void CLOAK_CALL Release();
				void CLOAK_CALL OnVideoEvent();
				void CLOAK_CALL OnSizeChange(In const LPRECT rec);
			}
		}
	}
}

#endif