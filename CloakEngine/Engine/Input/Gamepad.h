#pragma once
#ifndef CE_E_INPUT_GAMEPAD_H
#define CE_E_INPUT_GAMEPAD_H

#include "CloakEngine/Defines.h"

namespace CloakEngine {
	namespace Engine {
		namespace Input {
			namespace Gamepad {
				void CLOAK_CALL Initialize();
				void CLOAK_CALL Release();
				void CLOAK_CALL Update(In unsigned long long etime);
				void CLOAK_CALL SetEnable(In bool enabled);
				void CLOAK_CALL SetVibrationLeft(In DWORD user, In float power);
				void CLOAK_CALL SetVibrationRight(In DWORD user, In float power);
				void CLOAK_CALL SetVibration(In DWORD user, In float left, In float right);
			}
		}
	}
}

#endif