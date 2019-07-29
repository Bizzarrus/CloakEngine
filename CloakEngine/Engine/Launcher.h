#pragma once
#ifndef CE_E_LAUNCHER_H
#define CE_E_LAUNCHER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/GameEvents.h"

#include "Implementation/Global/Game.h"

namespace CloakEngine {
	namespace Engine {
		namespace Launcher {
			bool CLOAK_CALL StartLauncher(In API::Global::ILauncherEvent* events, Inout Impl::Global::Game::ThreadSleepInfo* tsi);
			LRESULT CALLBACK LauncherCallback(In HWND hWnd, In UINT msg, In WPARAM wParam, In LPARAM lParam);
			void CLOAK_CALL SetLauncherResult(In bool res);
		}
	}
}

#endif
