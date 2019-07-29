#include "stdafx.h"
#include "Implementation/Interface/EditBox.h"
#include "Implementation/Global/Localization.h"
#include "Implementation/Global/Mouse.h"
#include "Implementation/Files/Font.h"

#include "CloakEngine/Helper/StringConvert.h"

#include <sstream>

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace EditBox_v1 {
				CLOAK_CALL_THIS EditBox::EditBox()
				{
					m_ZSize = 2;
					m_curString = "";
					m_lastString = "";
					m_maxLetter = 0;
					m_numeric = false;
					m_numericFloat = true;
					m_cursorPos = 0;
					m_hookPos = 0;
					m_justifyLeft = true;
					m_textOffset.X = 0;
					m_textOffset.Y = 0;
					m_maxWidth = -1;
					m_gotSurrogate = false;
					m_lastSurrogate = u'\0';
					m_cursorColor = API::Helper::Color::White;
					m_cursorWidth = 0.1f;
					m_cursorImg = nullptr;
					m_cursorX = 0;

					INIT_EVENT(onClick, m_sync);
					INIT_EVENT(onEnterKey, m_sync);
					INIT_EVENT(onEnterChar, m_sync);
					INIT_EVENT(onScroll, m_sync);
					DEBUG_NAME(EditBox);
				}
				CLOAK_CALL_THIS EditBox::~EditBox()
				{
					SAVE_RELEASE(m_cursorImg);
				}
				void CLOAK_CALL_THIS EditBox::SetCursorColor(In const API::Helper::Color::RGBX& color)
				{
					API::Helper::Lock lock(m_sync);
					m_cursorColor = color;
				}
				void CLOAK_CALL_THIS EditBox::SetCursorPos(In size_t pos)
				{
					API::Helper::Lock lock(m_sync);
					if (m_ownText == false)
					{
						if (m_text != nullptr) { SetText(m_text->GetAsString()); }
						else { SetText(""); }
					}
					m_cursorPos = max(pos, m_curString.length());
					while (m_cursorPos < m_curString.length() && (m_curString[m_cursorPos] & 0xC0) == 0x80) { m_cursorPos++; }
					iCheckPos();
				}
				void CLOAK_CALL_THIS EditBox::SetCursor(In API::Files::ITexture* img)
				{
					API::Helper::Lock lock(m_sync);
					GUI_SET_TEXTURE(img, m_cursorImg);
				}
				void CLOAK_CALL_THIS EditBox::SetCursorWidth(In float w)
				{
					API::Helper::Lock lock(m_sync);
					m_cursorWidth = w;
				}
				void CLOAK_CALL_THIS EditBox::SetNumeric(In bool numeric, In_opt bool allowFloat)
				{
					API::Helper::Lock lock(m_sync);
					m_numeric = numeric;
					m_numericFloat = allowFloat;
				}
				void CLOAK_CALL_THIS EditBox::SetMaxLetter(In size_t length)
				{
					API::Helper::Lock lock(m_sync);
					m_maxLetter = length;
				}
				void CLOAK_CALL_THIS EditBox::SetTextPos(In const API::Global::Math::Space2D::Vector& offset, In_opt float maxWidth)
				{
					API::Helper::Lock lock(m_sync);
					m_textOffset = offset;
					m_maxWidth = maxWidth;
				}
				void CLOAK_CALL_THIS EditBox::SetText(In API::Files::IString* text)
				{
					API::Helper::Lock lock(m_sync);
					if (IsFocused()) { SetText(text->GetAsString()); }
					else { Label::SetText(text); }
				}
				void CLOAK_CALL_THIS EditBox::SetText(In const std::u16string& str)
				{
					API::Helper::Lock lock(m_sync);
					Label::SetText(str);
					m_lastString = API::Helper::StringConvert::ConvertToU8(str);
					m_curString = m_lastString;
					m_cursorPos = 0;
					m_hookPos = 0;
					m_justifyLeft = true;
				}
				void CLOAK_CALL_THIS EditBox::SetText(In const std::string& str)
				{
					API::Helper::Lock lock(m_sync);
					Label::SetText(str);
					m_lastString = str;
					m_curString = m_lastString;
					m_cursorPos = 0;
					m_hookPos = 0;
					m_justifyLeft = true;
				}
				bool CLOAK_CALL_THIS EditBox::IsNumeric() const
				{
					API::Helper::Lock lock(m_sync);
					return m_numeric;
				}
				bool CLOAK_CALL_THIS EditBox::IsClickable() const { return true; }
				size_t CLOAK_CALL_THIS EditBox::GetMaxLetter() const
				{
					API::Helper::Lock lock(m_sync);
					return m_maxLetter;
				}
				size_t CLOAK_CALL_THIS EditBox::GetCursorPos() const
				{
					API::Helper::Lock lock(m_sync);
					return m_cursorPos;
				}
				std::string CLOAK_CALL_THIS EditBox::GetText() const
				{
					API::Helper::Lock lock(m_sync);
					if (m_ownText == true) { return m_lastString; }
					else if (m_text != nullptr) { return API::Helper::StringConvert::ConvertToU8(m_text->GetAsString()); }
					return "";
				}
				void CLOAK_CALL_THIS EditBox::SetFont(In API::Files::IFont* font)
				{
					API::Helper::Lock lock(m_sync);
					Label::SetFont(font);
					iCheckPos();
				}
				void CLOAK_CALL_THIS EditBox::SetFontSize(In API::Files::FontSize size)
				{
					API::Helper::Lock lock(m_sync);
					Label::SetFontSize(size);
					iCheckPos();
				}

				void CLOAK_CALL_THIS EditBox::Initialize()
				{
					Frame::Initialize();
					registerListener([=](const API::Global::Events::onEnterChar& key) {OnEnterKey(key); });
					registerListener([=](const API::Global::Events::onEnterKey& key) {OnEnterKey(key); });
					registerListener([=](const API::Global::Events::onGainFocus& ev) {OnGainFocus(ev); });
					registerListener([=](const API::Global::Events::onLooseFocus& ev) {OnLooseFocus(ev); });
					registerListener([=](const API::Global::Events::onClick& ev) {OnClick(ev); });
				}

				void CLOAK_CALL_THIS EditBox::OnEnterKey(In const API::Global::Events::onEnterChar& ev)
				{
					API::Helper::Lock lock(m_sync);
					if (ev.c >= 0x20 || m_gotSurrogate)
					{
						if (m_gotSurrogate == false && ev.c >= 0xD800 && ev.c <= 0xDFFF)
						{
							m_gotSurrogate = true;
							m_lastSurrogate = ev.c;
						}
						else
						{
							std::u16string s = u"";
							if (m_gotSurrogate)
							{
								char16_t t[2] = { m_lastSurrogate,ev.c };
								s = std::u16string(t);
							}
							else { s = std::u16string(1, ev.c); }
							std::string s8 = API::Helper::StringConvert::ConvertToU8(s);
							if ((m_maxLetter <= 0 || API::Helper::StringConvert::UTF8Length(m_curString + s8) < m_maxLetter) && (m_numeric == false || (s8[0] == '-' && m_curString.length() == 0) || (s8[0] >= '0' && s8[0] <= '9') || (m_numericFloat == true && s8[0] == '.' && m_curString.find('.') == m_curString.npos))) 
							{
								const std::string beg = m_cursorPos > 0 ? m_curString.substr(0, m_cursorPos) : "";
								const std::string end = m_cursorPos < m_curString.length() ? m_curString.substr(m_cursorPos) : "";
								m_curString = beg + s8 + end;
								m_cursorPos += API::Helper::StringConvert::UTF8Length(s8);
								iCheckPos();
								API::Interface::registerForUpdate(this);
							}
						}
					}
				}
				void CLOAK_CALL_THIS EditBox::OnEnterKey(In const API::Global::Events::onEnterKey& ev)
				{
					if (ev.pressTpye != API::Global::Input::KEY_UP)
					{
						API::Helper::Lock lock(m_sync);
						if (ev.key == API::Global::Input::Keys::Keyboard::Arrow::LEFT)
						{
							if (m_cursorPos > 0)
							{
								while (m_cursorPos > 1 && (m_curString[m_cursorPos - 1] & 0xC0) == 0x80) { m_cursorPos--; }
								m_cursorPos--;
								iCheckPos();
							}
						}
						else if (ev.key == API::Global::Input::Keys::Keyboard::Arrow::RIGHT)
						{
							if (m_cursorPos < m_curString.length())
							{
								m_cursorPos++;
								while (m_cursorPos < m_curString.length() && (m_curString[m_cursorPos] & 0xC0) == 0x80) { m_cursorPos++; }
								iCheckPos();
							}
						}
						else if (ev.key == API::Global::Input::Keys::Keyboard::BACKSPACE)
						{
							if (m_cursorPos > 0)
							{
								size_t npos = m_cursorPos;
								while (npos > 1 && (m_curString[npos - 1] & 0xC0) == 0x80) { npos--; }
								npos--;

								if (npos > 0)
								{
									m_curString = m_curString.substr(0, npos) + m_curString.substr(m_cursorPos);
								}
								else
								{
									m_curString = m_curString.substr(m_cursorPos);
								}
								m_cursorPos = npos;
								iCheckPos();
							}
						}
						else if (ev.key == API::Global::Input::Keys::Keyboard::DELETE)
						{
							if (m_cursorPos < m_curString.length())
							{
								size_t npos = m_cursorPos + 1;
								while (npos < m_curString.length() && (m_curString[npos] & 0xC0) == 0x80) { npos++; }
								if (npos < m_curString.length())
								{
									m_curString = m_curString.substr(0, m_cursorPos) + m_curString.substr(npos);
								}
								else
								{
									m_curString = m_curString.substr(0, m_cursorPos);
								}
							}
						}
						else if (ev.key == API::Global::Input::Keys::Keyboard::ESCAPE)
						{
							m_curString = m_lastString;
							LooseFocus();
						}
						else if (ev.key == API::Global::Input::Keys::Keyboard::RETURN)
						{
							LooseFocus();
						}
						else if (ev.key == API::Global::Input::Keys::Keyboard::END)
						{
							m_cursorPos = m_curString.length();
							iCheckPos();
						}
						else if (ev.key == API::Global::Input::Keys::Keyboard::HOME)
						{
							m_cursorPos = 0;
							iCheckPos();
						}
					}
				}
				void CLOAK_CALL_THIS EditBox::OnLooseFocus(In const API::Global::Events::onLooseFocus& ev)
				{
					API::Helper::Lock lock(m_sync);
					m_lastString = m_curString;
					m_cursorPos = 0;
					m_hookPos = 0;
					m_justifyLeft = true;
					API::Interface::registerForUpdate(this);
					Impl::Global::Mouse::Hide();
				}
				void CLOAK_CALL_THIS EditBox::OnGainFocus(In const API::Global::Events::onGainFocus& ev)
				{
					API::Helper::Lock lock(m_sync);
					if (m_ownText == false) 
					{ 
						if (m_text != nullptr) { SetText(m_text->GetAsString()); }
						else { SetText(""); }
					}
					Impl::Global::Mouse::Show();
				}
				void CLOAK_CALL_THIS EditBox::OnClick(const API::Global::Events::onClick& ev)
				{
					if (ev.pressType == API::Global::Input::MOUSE_DOWN && ev.key == API::Global::Input::Keys::Mouse::LEFT) { SetFocus(); }
					else if (ev.pressType==API::Global::Input::MOUSE_UP && ev.previousType == API::Global::Input::MOUSE_DOWN && ev.key == API::Global::Input::Keys::Mouse::LEFT)
					{
						API::Helper::Lock lock(m_sync);
						if (m_font != nullptr && m_font->isLoaded(API::Files::Priority::NORMAL))
						{
							if (m_justifyLeft)
							{
								const std::string ts8 = m_curString.substr(m_hookPos);
								const std::u32string ts = API::Helper::StringConvert::ConvertToU32(ts8);
								const float tpX = ev.clickPos.X - (m_textOffset.X + m_pos.X);
								const float ls = GET_FONT_SIZE(m_fsize);
								size_t nextI = 0;
								float nextX = 0;
								float lastX = 0;

								for (size_t a = 0; a < ts.length() && nextX < tpX; a++)
								{
									lastX = nextX;
									nextI = a;
									const API::Files::LetterInfo& i = m_font->GetLetterInfo(ts[a]);
									float wi = i.Width;									
									if (a > 0) { wi += m_letterSpace; }
									nextX += wi*ls;
								}
								if (nextX < tpX || nextX - tpX < tpX - lastX) 
								{ 
									nextI++; 
									m_cursorX = nextX;
								}
								else { m_cursorX = lastX; }
								size_t tarPos = 0;
								for (size_t a = 0; tarPos < ts8.length() && a < nextI; a++)
								{
									do {
										tarPos++;
									} while ((ts8[tarPos] & 0xC0) == 0x80);
								}
								m_cursorPos = tarPos + m_hookPos;
							}
							else
							{
								const std::string ts8 = m_curString.substr(0, m_hookPos);
								const std::u32string ts = API::Helper::StringConvert::ConvertToU32(ts8);
								const float maxW = m_maxWidth < 0 ? m_Width : m_maxWidth;
								const float tpX = (maxW + m_textOffset.X + m_pos.X) - ev.clickPos.X;
								const float ls = GET_FONT_SIZE(m_fsize);
								size_t nextI = 0;
								float nextX = 0;
								float lastX = 0;
								for (size_t a = ts.length(), b = 0; a > 0 && nextX < tpX; a++, b++)
								{
									lastX = nextX;
									nextI = b;
									const API::Files::LetterInfo& i = m_font->GetLetterInfo(ts[a - 1]);
									float wi = i.Width;
									if (b > 0) { wi += m_letterSpace; }
									nextX += wi*ls;
								}
								if (nextX < tpX || nextX - tpX < tpX - lastX) 
								{ 
									nextI++; 
									m_cursorX = maxW - nextX;
								}
								else { m_cursorX = maxW - lastX; }
								nextI = ts.length() - nextI;
								size_t tarPos = 0;
								for (size_t a = 0; tarPos < ts8.length() && a < nextI; a++)
								{
									do {
										tarPos++;
									} while ((ts8[tarPos] & 0xC0) == 0x80);
								}
								m_cursorPos = tarPos + m_hookPos;
							}
						}
					}
				}
				
				Success(return == true) bool CLOAK_CALL_THIS EditBox::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Interface::EditBox_v1::IEditBox)) { *ptr = (API::Interface::EditBox_v1::IEditBox*)this; return true; }
					else if (riid == __uuidof(EditBox)) { *ptr = (EditBox*)this; return true; }
					else if (Frame::iQueryInterface(riid, ptr)) { return true; }
					return Label::iQueryInterface(riid, ptr);
				}
				bool CLOAK_CALL_THIS EditBox::iUpdateDrawInfo(In unsigned long long time)
				{
					Label::iUpdateDrawInfo(time);
					Frame::iUpdateDrawInfo(time);
					m_d_textDesc.Anchor = API::Files::AnchorPoint::TOP;
					m_d_textDesc.TopLeft.X += m_textOffset.X;
					m_d_textDesc.TopLeft.Y += m_textOffset.Y;
					m_d_textDesc.BottomRight.X = m_maxWidth < 0 ? (m_d_textDesc.BottomRight.X - m_textOffset.X) : (m_d_textDesc.TopLeft.X + m_maxWidth);
					m_d_textDesc.BottomRight.Y = m_d_textDesc.TopLeft.Y + GET_FONT_SIZE(m_d_textDesc.FontSize);
					m_d_textDesc.Justify = m_justifyLeft ? API::Files::Justify::LEFT : API::Files::Justify::RIGHT;
					if (m_ownText == true) 
					{ 
						if (m_justifyLeft) { m_text->Set(0, m_curString.substr(m_hookPos)); }
						else { m_text->Set(0, m_curString.substr(0, m_hookPos)); }
					}

					float H = GET_FONT_SIZE(m_fsize);
					float W = m_cursorWidth*H;
					float X = m_cursorX + m_textOffset.X + m_pos.X;
					float Y = m_textOffset.Y + m_pos.Y;
					m_d_cursorVertex.Color[0] = m_cursorColor;
					m_d_cursorVertex.TexTopLeft.Y = 0;
					m_d_cursorVertex.TexBottomRight.Y = 1;
					m_d_cursorVertex.TexBottomRight.X = 1;
					m_d_cursorVertex.TexTopLeft.X = 0;
					m_d_cursorVertex.Size.X = W;
					m_d_cursorVertex.Size.Y = H;
					m_d_cursorVertex.Position.X = X;
					m_d_cursorVertex.Position.Y = Y;
					m_d_cursorVertex.Rotation.Sinus = m_rotation.sin_a;
					m_d_cursorVertex.Rotation.Cosinus = m_rotation.cos_a;
					m_d_cursorVertex.Flags = API::Rendering::Vertex2DFlags::NONE;

					return IsFocused();
				}
				void CLOAK_CALL_THIS EditBox::iDraw(In API::Rendering::Context_v1::IContext2D* context)
				{
					Frame::iDraw(context);
					m_text->Draw(context, this, m_font, m_d_textDesc, 1, true);
					context->Draw(this, m_cursorImg, m_d_cursorVertex, 2);
				}
				void CLOAK_CALL_THIS EditBox::iCheckPos()
				{
					if (m_font != nullptr && m_font->isLoaded(API::Files::Priority::NORMAL))
					{
						if (m_justifyLeft == true)
						{
							if (m_cursorPos < m_hookPos) { m_hookPos = m_cursorPos; }
							else
							{
								const std::string ts8 = m_curString.substr(m_hookPos);
								const std::u32string ts = API::Helper::StringConvert::ConvertToU32(ts8);
								size_t tarPos = 0;
								const float ls = GET_FONT_SIZE(m_fsize);
								const float maxW = m_maxWidth < 0 ? (m_Width - m_textOffset.X) : m_maxWidth;
								float curW = 0;
								for (size_t a = 0; a < ts.length() && curW < maxW; a++)
								{
									API::Files::LetterInfo i = m_font->GetLetterInfo(ts[a]);
									float w = i.Width;
									if (a > 0) { w += m_letterSpace; }
									w = w*ls;
									if (curW + w < maxW) { tarPos++; }
									m_cursorX = curW;
									curW += w;
								}
								size_t tarPos8 = 0;
								for (size_t a = 0; tarPos8 < ts8.length() && a < tarPos; a++)
								{
									do {
										tarPos8++;
									} while ((ts8[tarPos8] & 0xC0) == 0x80);
								}
								if (m_cursorPos > tarPos8 + m_hookPos) { m_hookPos = m_cursorPos; m_justifyLeft = false; }
							}
						}
						else
						{
							if (m_cursorPos + 1 > m_hookPos) { m_hookPos = m_cursorPos; }
							else
							{
								const std::string ts8 = m_curString.substr(0, m_hookPos);
								const std::u32string ts = API::Helper::StringConvert::ConvertToU32(ts8);
								const float ls = GET_FONT_SIZE(m_fsize);
								const float maxW = m_maxWidth < 0 ? (m_Width - m_textOffset.X) : m_maxWidth;
								float curW = 0;
								size_t tarPos = ts.length();
								for (size_t a = ts.length(); a > 0 && curW < maxW; a--)
								{
									API::Files::LetterInfo i = m_font->GetLetterInfo(ts[a - 1]);
									float w = i.Width;
									if (a < ts.length()) { w += m_letterSpace; }
									w = w*ls;
									if (curW + w < maxW) { tarPos--; }
									m_cursorX = curW;
									curW += w;
								}
								size_t tarPos8 = 0;
								for (size_t a = 0; tarPos8 < ts8.length() && a < tarPos; a++)
								{
									do {
										tarPos8++;
									} while ((ts8[tarPos8] & 0xC0) == 0x80);
								}
								if (m_cursorPos < tarPos8) { m_hookPos = m_cursorPos; m_justifyLeft = true; }
								else if (curW < maxW) { m_hookPos = 0; m_justifyLeft = true; }
							}
						}
					}
				}
			}
		}
	}
}