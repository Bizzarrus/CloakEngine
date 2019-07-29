#pragma once
#ifndef CE_IMPL_GLOBAL_MOUSE_H
#define CE_IMPL_GLOBAL_MOUSE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Mouse.h"
#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Global/Math.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Mouse {
				void CLOAK_CALL Initiate();
				void CLOAK_CALL Update();
				void CLOAK_CALL Release();
				void CLOAK_CALL CheckClipping();
				void CLOAK_CALL LooseFocus();
				void CLOAK_CALL Move(In LONG X, In LONG Y);
				void CLOAK_CALL SetPos(In LONG X, In LONG Y);
				void CLOAK_CALL SetGraphicSettings(In const API::Global::Graphic::Settings& set);
				void CLOAK_CALL UpdateCursor();
				bool CLOAK_CALL IsInClip();
				uint64_t CLOAK_CALL Show();
				uint64_t CLOAK_CALL Hide();
			}
		}
	}
}

#endif