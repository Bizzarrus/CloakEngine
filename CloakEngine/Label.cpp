#include "stdafx.h"
#include "Implementation/Interface/Label.h"


namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace Label_v1 {
				CLOAK_CALL_THIS Label::Label()
				{
					m_fcolor.Color[0] = API::Helper::Color::RGBA(0, 0, 0, 1);
					m_fcolor.Color[1] = API::Helper::Color::RGBA(0, 0, 0, 1);
					m_fcolor.Color[2] = API::Helper::Color::RGBA(0, 0, 0, 1);
					m_font = nullptr;
					m_d_font = nullptr;
					m_fsize = 0;
					m_hJustify = API::Files::Justify::LEFT;
					m_vJustify = API::Files::AnchorPoint::TOP;
					m_lineSpace = 0;
					m_letterSpace = 0;
					m_text = nullptr;
					m_ownText = false;
					m_d_text = nullptr;
					DEBUG_NAME(Label);
				}
				CLOAK_CALL_THIS Label::~Label()
				{
					SAVE_RELEASE(m_font);
					SAVE_RELEASE(m_text);
					SAVE_RELEASE(m_d_text);
					SAVE_RELEASE(m_d_font);
				}
				void CLOAK_CALL_THIS Label::SetFont(In API::Files::IFont* font)
				{
					API::Helper::Lock lock(m_sync);
					if (m_font != nullptr) { m_font->unload(); }
					SAVE_RELEASE(m_font);
					m_font = font;
					if (m_font != nullptr) 
					{ 
						m_font->AddRef(); 
						m_font->load();
					}
				}
				void CLOAK_CALL_THIS Label::SetFontSize(In API::Files::FontSize size)
				{
					API::Helper::Lock lock(m_sync);
					m_fsize = size;
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Label::SetFontColor(In const API::Interface::FontColor& color)
				{
					API::Helper::Lock lock(m_sync);
					for (unsigned int a = 0; a < 3; a++) { m_fcolor.Color[a] = color.Color[a]; }
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Label::SetJustify(In API::Files::Justify hJust)
				{
					API::Helper::Lock lock(m_sync);
					m_hJustify = hJust;
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Label::SetJustify(In API::Files::Justify hJust, In API::Files::AnchorPoint vJust)
				{
					API::Helper::Lock lock(m_sync);
					m_hJustify = hJust;
					m_vJustify = vJust;
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Label::SetJustify(In API::Files::AnchorPoint vJust)
				{
					API::Helper::Lock lock(m_sync);
					m_vJustify = vJust;
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Label::SetLetterSpace(In float space)
				{
					API::Helper::Lock lock(m_sync);
					m_letterSpace = space;
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Label::SetLineSpace(In float space)
				{
					API::Helper::Lock lock(m_sync);
					m_lineSpace = space;
					UpdatePosition();
				}
				void CLOAK_CALL_THIS Label::SetText(In API::Files::IString* text)
				{
					API::Helper::Lock lock(m_sync);
					if (m_text != text)
					{
						m_ownText = false;
						SWAP(m_text, text);
						API::Interface::registerForUpdate(this);
					}
				}
				void CLOAK_CALL_THIS Label::SetText(In const std::u16string& text)
				{
					API::Helper::Lock lock(m_sync);
					if (m_ownText == false)
					{
						API::Files::IString* s = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&s));
						s->Set(0, text);
						SetText(s);
						m_ownText = true;
						s->Release();
					}
					else { m_text->Set(0, text); }
				}
				void CLOAK_CALL_THIS Label::SetText(In const std::string& text)
				{
					API::Helper::Lock lock(m_sync);
					if (m_ownText == false)
					{
						API::Files::IString* s = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&s));
						s->Set(0, text);
						SetText(s);
						m_ownText = true;
						s->Release();
					}
					else { m_text->Set(0, text); }
				}
				API::Files::FontSize CLOAK_CALL_THIS Label::GetFontSize() const { API::Helper::Lock lock(m_sync); return m_fsize; }
				const API::Interface::FontColor& CLOAK_CALL_THIS Label::GetFontColor() const { API::Helper::Lock lock(m_sync); return m_fcolor; }
				API::Files::Justify CLOAK_CALL_THIS Label::GetHorizontalJustify() const { API::Helper::Lock lock(m_sync); return m_hJustify; }
				API::Files::AnchorPoint CLOAK_CALL_THIS Label::GetVerticalJustify() const { API::Helper::Lock lock(m_sync); return m_vJustify; }

				Success(return==true) bool CLOAK_CALL_THIS Label::iQueryInterface(In REFIID id, Outptr void** ptr)
				{
					CLOAK_QUERY(id, ptr, BasicGUI, Interface::Label_v1, Label);
				}
				void CLOAK_CALL_THIS Label::iDraw(In API::Rendering::Context_v1::IContext2D* context)
				{
					if (m_d_text != nullptr && m_d_text->isLoaded(API::Files::Priority::NORMAL))
					{
						m_d_text->Draw(context, this, m_d_font, m_d_textDesc);
					}
				}
				bool CLOAK_CALL_THIS Label::iUpdateDrawInfo(In unsigned long long time)
				{
					m_d_textDesc.Anchor = m_vJustify;
					m_d_textDesc.Color[0] = m_fcolor.Color[0];
					m_d_textDesc.Color[1] = m_fcolor.Color[1];
					m_d_textDesc.Color[2] = m_fcolor.Color[2];
					m_d_textDesc.Grayscale = false;
					m_d_textDesc.Justify = m_hJustify;
					m_d_textDesc.TopLeft = m_pos;
					m_d_textDesc.BottomRight = m_pos + API::Global::Math::Space2D::Point(m_Width, m_Height);
					m_d_textDesc.FontSize = m_fsize;
					m_d_textDesc.LetterSpace = m_letterSpace;
					m_d_textDesc.LineSpace = m_lineSpace;

					if (m_d_text != m_text) { SWAP(m_d_text, m_text); }
					if (m_d_font != m_font) { SWAP(m_d_font, m_font); }
					return false;
				}
			}
		}
	}
}
