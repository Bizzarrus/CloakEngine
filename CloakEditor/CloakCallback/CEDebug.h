#pragma once
#ifndef CE_EDIT_CLOAKCALLBACK_DEBUG_H
#define CE_EDIT_CLOAKCALLBACK_DEBUG_H

#include "CloakEngine/CloakEngine.h"

namespace Editor {
	class CEDebug : public CloakEngine::Global::IDebugEvent {
		public:
			virtual void CLOAK_CALL_THIS Delete() override;
			virtual void CLOAK_CALL_THIS OnLogWrite(In const std::string& msg) override;
	};
}

#endif