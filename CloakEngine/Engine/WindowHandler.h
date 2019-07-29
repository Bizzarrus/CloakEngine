#pragma once
#ifndef CE_E_WINDOWHANDLER_H
#define CE_E_WINDOWHANDLER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/Logic.h"
#include "CloakEngine/Global/Graphic.h"

//Private windows messages (allowed Range: ]WM_APP;0xBFFF]
#define WM_VIDEO_EVENT WM_APP+1
#define WM_SOCKET WM_APP+2

namespace CloakEngine {
	namespace Engine {
		namespace WindowHandler {
			const int defaultW = 640;
			const int defaultH = 480;

			void CLOAK_CALL setExternWindow(In HWND wnd);
			void CLOAK_CALL onInit();
			void CLOAK_CALL onLoad();
			void CLOAK_CALL onStop();
			bool CLOAK_CALL UpdateOnce();
			bool CLOAK_CALL WaitForMessage(In API::Global::Time maxWait);
			HWND CLOAK_CALL getWindow();
			void CLOAK_CALL showWindow();
			void CLOAK_CALL hideWindow();
			void CLOAK_CALL setWindowName(In const std::u16string& name);
			bool CLOAK_CALL isFocused();
			bool CLOAK_CALL isVisible();
			void CLOAK_CALL updateSettings(In const API::Global::Graphic::Settings& newSet, In const API::Global::Graphic::Settings& oldSet);
			void CLOAK_CALL checkSettings(Inout API::Global::Graphic::Settings* gset);
		}
	}
}

#endif