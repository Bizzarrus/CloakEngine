#pragma once
#ifndef CE_API_GLOBAL_VIDEO_H
#define CE_API_GLOBAL_VIDEO_H

#include "CloakEngine/Defines.h"

#include <string>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Video {
				CLOAKENGINE_API void CLOAK_CALL StartVideo(std::u16string filePath);
				CLOAKENGINE_API void CLOAK_CALL StopVideo();
				CLOAKENGINE_API bool CLOAK_CALL IsVideoPlaying();
			}
		}
	}
}

#endif