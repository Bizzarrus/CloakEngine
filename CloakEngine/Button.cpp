#include "stdafx.h"
#include "Implementation/Interface/Button.h"

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace Button_v1 {
				CLOAK_CALL_THIS Button::Button() : m_textOffset(0, 0, 0)
				{
					m_ZSize = 4;
					m_imgHighlight = nullptr;
					m_highlight = API::Interface::HighlightType::ADD;
					m_maxTextW = -1;
					m_imgPressed = nullptr;
					m_pressed = API::Interface::HighlightType::ADD;
					m_isPressed = false;
					m_isHighlight = false;
					INIT_EVENT(onClick, m_sync);
					INIT_EVENT(onEnterKey, m_sync);
					INIT_EVENT(onScroll, m_sync);
					DEBUG_NAME(Button);
				}
				CLOAK_CALL_THIS Button::~Button()
				{
					SAVE_RELEASE(m_imgHighlight);
				}

				void CLOAK_CALL_THIS Button::Initialize()
				{
					Frame::Initialize();
					registerListener([=](const API::Global::Events::onClick& click) { OnClick(click); });
					registerListener([=](const API::Global::Events::onMouseEnter& enter) { setMouseOver(true); });
					registerListener([=](const API::Global::Events::onMouseLeave& leave) { setMouseOver(false); });
				}

				void CLOAK_CALL_THIS Button::SetHighlightTexture(In API::Files::ITexture* img, In_opt API::Interface::HighlightType highlight)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_imgHighlight);
					m_highlight = highlight;
				}
				void CLOAK_CALL_THIS Button::SetPressedTexture(In API::Files::ITexture* img, In_opt API::Interface::HighlightType highlight)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_imgPressed);
					m_pressed = highlight;
				}
				void CLOAK_CALL_THIS Button::SetText(In API::Files::IString* text, In const API::Global::Math::Space2D::Vector& offset, In_opt float maxWidth)
				{
					API::Helper::Lock lock(m_sync);
					Label::SetText(text);
					m_textOffset = offset;
					m_maxTextW = maxWidth;
				}
				void CLOAK_CALL_THIS Button::SetText(In API::Files::IString* text)
				{
					Label::SetText(text);
				}
				void CLOAK_CALL_THIS Button::SetText(In const std::u16string& text, In const API::Global::Math::Space2D::Vector& offset, In_opt float maxWidth)
				{
					API::Helper::Lock lock(m_sync);
					if (m_ownText == false)
					{
						API::Files::IString* s = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&s));
						s->Set(0, text);
						SetText(s, offset, maxWidth);
						m_ownText = true;
					}
					else
					{
						m_text->Set(0, text);
						SetText(m_text, offset, maxWidth);
					}
				}
				void CLOAK_CALL_THIS Button::SetText(In const std::u16string& text)
				{
					Label::SetText(text);
				}
				void CLOAK_CALL_THIS Button::SetText(In const std::string& text, In const API::Global::Math::Space2D::Vector& offset, In_opt float maxWidth)
				{
					if (m_ownText == false)
					{
						API::Files::IString* s = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&s));
						s->Set(0, text);
						SetText(s, offset, maxWidth);
						m_ownText = true;
					}
					else
					{
						m_text->Set(0, text);
						SetText(m_text, offset, maxWidth);
					}
				}
				void CLOAK_CALL_THIS Button::SetText(In const std::string& text)
				{
					Label::SetText(text);
				}
				void CLOAK_CALL_THIS Button::SetTextMaxWidth(In float maxWidth)
				{
					API::Helper::Lock lock(m_sync);
					m_maxTextW = maxWidth;
				}
				bool CLOAK_CALL_THIS Button::IsClickable() const { return true; }

				Success(return == true) bool CLOAK_CALL_THIS Button::iQueryInterface(In REFIID id, Outptr void** ptr)
				{
					if (id == __uuidof(API::Interface::Button_v1::IButton)) { *ptr = (API::Interface::Button_v1::IButton*)this; return true; }
					else if (id == __uuidof(Button)) { *ptr = (Button*)this; return true; }
					else if (Frame::iQueryInterface(id, ptr)) { return true; }
					return Label::iQueryInterface(id, ptr);
				}
				void CLOAK_CALL_THIS Button::iDraw(In API::Rendering::Context_v1::IContext2D* context)
				{
					bool Override = false;
					if (m_d_pressed.CanDraw())
					{
						iDrawImage(context, m_d_pressed.GetImage(), 3);
						Override = (m_d_pressed.Type == API::Interface::HighlightType::OVERRIDE);
					}
					if (m_d_highlight.CanDraw() && Override == false)
					{
						iDrawImage(context, m_d_highlight.GetImage(), 2);
						Override = (m_d_highlight.Type == API::Interface::HighlightType::OVERRIDE);
					}
					if (m_d_img.CanDraw() && Override == false)
					{
						iDrawImage(context, m_d_img.GetImage(), 0);
					}
					if (m_d_text != nullptr && m_d_text->isLoaded(API::Files::Priority::NORMAL))
					{
						m_d_text->Draw(context, this, m_d_font, m_d_textDesc, Override ? 4 : 1);
					}
				}
				bool CLOAK_CALL_THIS Button::iUpdateDrawInfo(In unsigned long long time) 
				{
					const float W = m_Width - m_textOffset.X;
					const float H = m_Height - m_textOffset.Y;
					m_d_textDesc.Anchor = m_vJustify;
					m_d_textDesc.Color[0] = m_fcolor.Color[0];
					m_d_textDesc.Color[1] = m_fcolor.Color[1];
					m_d_textDesc.Color[2] = m_fcolor.Color[2];
					m_d_textDesc.Grayscale = false;
					m_d_textDesc.Justify = m_hJustify;
					m_d_textDesc.TopLeft = m_pos + m_textOffset;
					m_d_textDesc.BottomRight = m_pos + API::Global::Math::Space2D::Point(m_maxTextW < 0 ? W : min(W, m_maxTextW), H);
					m_d_textDesc.FontSize = m_fsize;
					m_d_textDesc.LetterSpace = m_letterSpace;
					m_d_textDesc.LineSpace = m_lineSpace;

					m_d_highlight.Update(m_imgHighlight, m_isHighlight, m_highlight);
					m_d_pressed.Update(m_imgPressed, m_isPressed, m_pressed);

					if (m_d_text != m_text) { SWAP(m_d_text, m_text); }
					if (m_d_font != m_font) { SWAP(m_d_font, m_font); }

					return Frame::iUpdateDrawInfo(time); 
				}
				void CLOAK_CALL_THIS Button::OnClick(const API::Global::Events::onClick& ev)
				{  
					API::Helper::Lock lock(m_sync);
					if (ev.key == API::Global::Input::Keys::Mouse::LEFT)
					{
						if (ev.pressType == API::Global::Input::MOUSE_UP) 
						{ 
							m_isPressed = false; 
						}
						else if (ev.pressType == API::Global::Input::MOUSE_DOWN || ev.pressType == API::Global::Input::MOUSE_DOUBLE) 
						{ 
							m_isPressed = true; 
							API::Interface::registerForUpdate(this);
						}
					}
				}
				void CLOAK_CALL_THIS Button::setMouseOver(In bool mouseOver) 
				{ 
					API::Helper::Lock lock(m_sync); 
					m_isHighlight = mouseOver; 
					API::Interface::registerForUpdate(this);
				}
			}
		}
	}
}
