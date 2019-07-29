#include "stdafx.h"
#include "CloakCallback/CEInput.h"

namespace Editor {
	void CLOAK_CALL_THIS CEInput::Delete() { delete this; }
	void CLOAK_CALL_THIS CEInput::OnMouseMove(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Math::Space2D::Vector& direction, In const CloakEngine::Global::Math::Space2D::Point& resPos, In CloakEngine::Global::Time time)
	{

	}
	void CLOAK_CALL_THIS CEInput::OnGamepadConnect(In CloakEngine::Global::Input::User user, In CloakEngine::Global::Input::CONNECT_EVENT connect)
	{

	}
	void CLOAK_CALL_THIS CEInput::OnGamepadThumbMove(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Input::ThumbMoveData& data, In CloakEngine::Global::Time etime)
	{

	}
	void CLOAK_CALL_THIS CEInput::OnKeyEnter(In CloakEngine::Global::Input::User user, In const CloakEngine::Global::Input::KeyCode& down, In const CloakEngine::Global::Input::KeyCode& up)
	{

	}
	void CLOAK_CALL_THIS CEInput::OnScroll(In CloakEngine::Global::Input::User user, In CloakEngine::Global::Input::SCROLL_STATE button, In float delta)
	{

	}
	void CLOAK_CALL_THIS CEInput::OnWindowClose()
	{
		CloakEngine::Global::Graphic::HideWindow();
	}
}