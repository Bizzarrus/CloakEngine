#pragma once
#ifndef CE_API_GLOBAL_GRAPHIC_H
#define CE_API_GLOBAL_GRAPHIC_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Image.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Graphic {
				struct Resolution {
					unsigned int Width;
					unsigned int Height;
					int XOffset;
					int YOffset;
				};
				enum class WindowMode { FULLSCREEN, WINDOW, FULLSCREEN_WINDOW };
				enum class TextureFilter { BILINEAR, TRILINIEAR, ANISOTROPx2, ANISOTROPx4, ANISOTROPx8, ANISOTROPx16 };
				enum class TextureQuality { LOW = 0x0, MEDIUM = 0x1, HIGH = 0x2, ULTRA = 0x3 };
				enum class RenderMode { DX12 };
				enum class BloomMode { DISABLED, NORMAL, HIGH_QUALITY };
				enum class MSAA {DISABLED = 1, MSAAx2 = 2, MSAAx4 = 4, MSAAx8 = 8};
				enum class InputDelayPrevention { Disabled, Level1, Level2, Level3 };
				struct Settings {
					uint32_t ShadowMapResolution;
					uint32_t AdapterID;
					uint32_t BackBufferCount;
					Resolution Resolution;
					WindowMode WindowMode;
					TextureFilter TextureFilter;
					TextureQuality TextureQuality;
					RenderMode RenderMode;
					BloomMode Bloom;
					MSAA MSAA;
					bool FXAA;
					bool VSync;
					bool Tessellation;
					bool MultiThreadedRendering;
					float FieldOfView;
					float Gamma;
				};

				CLOAKENGINE_API void CLOAK_CALL SetSettings(In const Settings& setting);
				CLOAKENGINE_API void CLOAK_CALL GetSettings(Out Settings* setting);
				Success(return==true) CLOAKENGINE_API bool CLOAK_CALL EnumerateResolution(In size_t num, Out Resolution* res);
				CLOAKENGINE_API void CLOAK_CALL ShowWindow();
				CLOAKENGINE_API void CLOAK_CALL HideWindow();
				CLOAKENGINE_API Global::Debug::Error CLOAK_CALL GetManager(In REFIID riid, Outptr void** ppvObject);
				CLOAKENGINE_API long double CLOAK_CALL GetAspectRatio();
			}
		}
	}
}

#endif