#include "stdafx.h"
#include "Implementation/Interface/CheckButton.h"

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace CheckButton_v1 {
				CLOAK_CALL_THIS CheckButton::CheckButton()
				{
					m_ZSize = 3;
					m_isChecked = false;
					m_isMouseOver = false;
					m_isPressed = false;
					m_imgChecked = nullptr;
					m_imgPressed = nullptr;
					m_imgHighlight = nullptr;
					INIT_EVENT(onClick, m_sync);
					INIT_EVENT(onEnterKey, m_sync);
					INIT_EVENT(onCheck, m_sync);
					INIT_EVENT(onScroll, m_sync);
					DEBUG_NAME(CheckButton);
				}
				CLOAK_CALL_THIS CheckButton::~CheckButton()
				{
					SAVE_RELEASE(m_imgChecked);
					SAVE_RELEASE(m_imgPressed);
					SAVE_RELEASE(m_imgHighlight);
				}

				void CLOAK_CALL_THIS CheckButton::Initialize()
				{
					Frame::Initialize();
					registerListener([=](const API::Global::Events::onClick& click)
					{
						if (click.key == API::Global::Input::Keys::Mouse::LEFT)
						{
							if (click.pressType == API::Global::Input::KEY_UP)
							{
								if (click.previousType == API::Global::Input::KEY_DOWN) { SetChecked(!IsChecked()); }
								setPressed(false);
							}
							else if (click.pressType == API::Global::Input::KEY_DOWN)
							{
								setPressed(true);
							}
						}
					});
					registerListener([=](const API::Global::Events::onMouseEnter& enter) { setMouseOver(true); });
					registerListener([=](const API::Global::Events::onMouseLeave& leave) { setMouseOver(false); });
				}

				void CLOAK_CALL_THIS CheckButton::SetHighlightTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_imgHighlight);
					m_highlight = highlight;
				}
				void CLOAK_CALL_THIS CheckButton::SetPressedTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_imgPressed);
					m_pressed = highlight;
				}
				void CLOAK_CALL_THIS CheckButton::SetCheckedTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_imgChecked);
					m_checked = highlight;
				}
				void CLOAK_CALL_THIS CheckButton::SetChecked(In bool checked)
				{
					API::Helper::Lock lock(m_sync);
					const bool change = (m_isChecked != checked);
					m_isChecked = checked;
					API::Interface::registerForUpdate(this);
					if (change)
					{
						API::Global::Events::onCheck oce;
						oce.check = checked;
						triggerEvent(oce);
					}
				}
				bool CLOAK_CALL_THIS CheckButton::IsChecked() const { return m_isChecked; }

				Success(return == true) bool CLOAK_CALL_THIS CheckButton::iQueryInterface(In REFIID id, Outptr void** ptr)
				{
					CLOAK_QUERY(id, ptr, Frame, Interface::CheckButton_v1, CheckButton);
				}
				void CLOAK_CALL_THIS CheckButton::iDraw(In API::Rendering::Context_v1::IContext2D* context)
				{
					bool Override = false;
					if (m_d_checked.CanDraw())
					{
						iDrawImage(context, m_d_checked.GetImage(), 3);
						Override = (m_d_checked.Type == API::Interface::HighlightType::OVERRIDE);
					}
					if (m_d_pressed.CanDraw() && Override == false)
					{
						iDrawImage(context, m_d_pressed.GetImage(), 2);
						Override = (m_d_pressed.Type == API::Interface::HighlightType::OVERRIDE);
					}
					if (m_d_mouseOver.CanDraw() && Override == false)
					{
						iDrawImage(context, m_d_mouseOver.GetImage(), 1);
						Override = (m_d_mouseOver.Type == API::Interface::HighlightType::OVERRIDE);
					}
					if (m_d_img.CanDraw() && Override == false)
					{
						iDrawImage(context, m_d_img.GetImage(), 0);
					}
				}
				bool CLOAK_CALL_THIS CheckButton::iUpdateDrawInfo(In unsigned long long time)
				{
					m_d_checked.Update(m_imgChecked, m_isChecked, m_checked);
					m_d_pressed.Update(m_imgPressed, m_isPressed, m_pressed);
					m_d_mouseOver.Update(m_imgHighlight, m_isMouseOver, m_highlight);
					return Frame::iUpdateDrawInfo(time);
				}
				void CLOAK_CALL_THIS CheckButton::setPressed(In bool pressed)
				{
					API::Helper::Lock lock(m_sync);
					m_isPressed = pressed;
					API::Interface::registerForUpdate(this);
				}
				void CLOAK_CALL_THIS CheckButton::setMouseOver(In bool mouseOver)
				{
					API::Helper::Lock lock(m_sync);
					m_isMouseOver = mouseOver;
					API::Interface::registerForUpdate(this);
				}
				bool CLOAK_CALL_THIS CheckButton::IsClickable() const { return true; }
			}
		}
	}
}