#pragma once
#ifndef CE_EDIT_CLOAKCALLBACK_CEGAME_H
#define CE_EDIT_CLOAKCALLBACK_CEGAME_H

#include "CloakEngine/CloakEngine.h"

namespace Editor {
	class CEGame : public CloakEngine::Global::IGameEvent {
		public:
			virtual void CLOAK_CALL_THIS Delete() override;
			virtual void CLOAK_CALL_THIS OnInit() override;
			virtual void CLOAK_CALL_THIS OnStart() override;
			virtual void CLOAK_CALL_THIS OnStop() override;
			virtual void CLOAK_CALL_THIS OnUpdate(unsigned long long elapsedTime) override;
			virtual void CLOAK_CALL_THIS OnPause() override;
			virtual void CLOAK_CALL_THIS OnResume() override;
			virtual void CLOAK_CALL_THIS OnCrash(CloakEngine::Global::Debug::Error err) override;
			virtual void CLOAK_CALL_THIS OnUpdateLanguage() override;
	};
}

#endif