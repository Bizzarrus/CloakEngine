#pragma once
#ifndef CE_EDIT_CLOAKCALLBACK_CEINPUT_H
#define CE_EDIT_CLOAKCALLBACK_CEINPUT_H

#include "CloakEngine/CloakEngine.h"

namespace Editor {
	class CEInput : public CloakEngine::Global::IInputEvent {
		public:
			virtual void CLOAK_CALL_THIS Delete() override;
			virtual void CLOAK_CALL_THIS OnMouseMove(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Math::Space2D::Vector& direction, In const CloakEngine::Global::Math::Space2D::Point& resPos, In CloakEngine::Global::Time time) override;
			virtual void CLOAK_CALL_THIS OnGamepadConnect(In CloakEngine::Global::Input::User user, In CloakEngine::Global::Input::CONNECT_EVENT connect) override;
			virtual void CLOAK_CALL_THIS OnGamepadThumbMove(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Input::ThumbMoveData& data, In CloakEngine::Global::Time etime) override;
			virtual void CLOAK_CALL_THIS OnKeyEnter(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Input::KeyCode& down, In const CloakEngine::Global::Input::KeyCode& up) override;
			virtual void CLOAK_CALL_THIS OnScroll(In CloakEngine::Global::Input::User user, In CloakEngine::Global::Input::SCROLL_STATE button, In float delta) override;
			virtual void CLOAK_CALL_THIS OnWindowClose() override;
	};
}

#endif