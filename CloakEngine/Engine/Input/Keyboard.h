#pragma once
#ifndef CE_E_INPUT_KEYBOARD_H
#define CE_E_INPUT_KEYBOARD_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Input.h"

namespace CloakEngine {
	namespace Engine {
		namespace Input {
			namespace Keyboard {
				void CLOAK_CALL Initialize();
				void CLOAK_CALL Release();
				void CLOAK_CALL InputEvent(In WPARAM wparam, In LPARAM lparam);
				void CLOAK_CALL DoubleClickEvent(In API::Global::Input::KeyCode key);
			}
		}
	}
}

#endif