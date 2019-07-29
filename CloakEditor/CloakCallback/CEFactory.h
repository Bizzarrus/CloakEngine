#pragma once
#ifndef CE_EDIT_CLOAKCALLBACK_FACTORY_H
#define CE_EDIT_CLOAKCALLBACK_FACTORY_H

#include "CloakEngine/CloakEngine.h"

namespace Editor {
	class CEFactory : public virtual CloakEngine::Global::IGameEventFactory {
		public:
			virtual void CLOAK_CALL_THIS Delete() const override;
			virtual CloakEngine::Global::IGameEvent* CLOAK_CALL_THIS CreateGame() const override;
			virtual CloakEngine::Global::ILauncherEvent* CLOAK_CALL_THIS CreateLauncher() const override;
			virtual CloakEngine::Global::IInputEvent* CLOAK_CALL_THIS CreateInput() const override;
			virtual CloakEngine::Global::ILobbyEvent* CLOAK_CALL_THIS CreateLobby() const override;
			virtual CloakEngine::Global::IDebugEvent* CLOAK_CALL_THIS CreateDebug() const override;
	};
}

#endif