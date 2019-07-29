#pragma once
#ifndef CE_API_GLOBAL_MOUSE_H
#define CE_API_GLOBAL_MOUSE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Files/Image.h"
#include "CloakEngine/Interface/BasicGUI.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Mouse {
				CLOAKENGINE_API void CLOAK_CALL SetImage(In Files::ITexture* img);
				CLOAKENGINE_API void CLOAK_CALL SetImage(In Files::ITexture* img, In float offsetX, In float offsetY);
				CLOAKENGINE_API void CLOAK_CALL SetImage(In Files::ITexture* img, In const Math::Space2D::Point& offset);
				CLOAKENGINE_API void CLOAK_CALL SetPos(In const Math::Space2D::Point& p);
				CLOAKENGINE_API void CLOAK_CALL SetPos(In float X, In float Y);
				CLOAKENGINE_API void CLOAK_CALL GetPos(Out Math::Space2D::Point* p);
				CLOAKENGINE_API void CLOAK_CALL Hide();
				CLOAKENGINE_API void CLOAK_CALL Show();
				CLOAKENGINE_API void CLOAK_CALL SetUseSoftwareMouse(In bool use);
				CLOAKENGINE_API void CLOAK_CALL SetKeepInScreen(In bool keep);
				CLOAKENGINE_API void CLOAK_CALL SetMouseSpeed(In float speed);
				CLOAKENGINE_API bool CLOAK_CALL IsVisible();
				CLOAKENGINE_API bool CLOAK_CALL IsSoftwareMouse();
				CLOAKENGINE_API Interface::IBasicGUI* CLOAK_CALL GetSoftwareMouse();
			}
		}
	}
}

#endif