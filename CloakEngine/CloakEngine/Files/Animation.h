#pragma once
#ifndef CE_API_FILES_ANIMATION_H
#define CE_API_FILES_ANIMATION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Global/Game.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Animation_v1 {
				CLOAK_INTERFACE_ID("{9A8BB69B-29EA-45E0-A755-03F23F0D95E9}") IAnimation : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual API::Global::Time CLOAK_CALL_THIS GetLength() const = 0;
						virtual void CLOAK_CALL_THIS Start() = 0;
						virtual void CLOAK_CALL_THIS Pause() = 0;
						virtual void CLOAK_CALL_THIS Resume() = 0;
						virtual void CLOAK_CALL_THIS Stop() = 0;
						virtual void CLOAK_CALL_THIS SetAutoRepeat(In bool repeat) = 0;
						virtual bool CLOAK_CALL_THIS GetAutoRepeat() const = 0;
						virtual bool CLOAK_CALL_THIS IsPaused() = 0;
						virtual bool CLOAK_CALL_THIS IsRunning() = 0;
				};
			}
		}
	}
}

#endif