#include "stdafx.h"
#include "CloakCallback/CEFactory.h"
#include "CloakCallback/CEGame.h"
#include "CloakCallback/CEInput.h"
#include "CloakCallback/CEDebug.h"

namespace Editor {
	void CLOAK_CALL_THIS CEFactory::Delete() const { delete this; }
	CloakEngine::Global::IGameEvent* CLOAK_CALL_THIS CEFactory::CreateGame() const { return new CEGame(); }
	CloakEngine::Global::ILauncherEvent* CLOAK_CALL_THIS CEFactory::CreateLauncher() const { return nullptr; }
	CloakEngine::Global::IInputEvent* CLOAK_CALL_THIS CEFactory::CreateInput() const { return new CEInput(); }
	CloakEngine::Global::ILobbyEvent* CLOAK_CALL_THIS CEFactory::CreateLobby() const { return nullptr; }
	CloakEngine::Global::IDebugEvent* CLOAK_CALL_THIS CEFactory::CreateDebug() const { return new CEDebug(); }
}