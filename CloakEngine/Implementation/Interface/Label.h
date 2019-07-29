#pragma once
#ifndef CE_IMPL_INTERFACE_LABEL_H
#define CE_IMPL_INTERFACE_LABEL_H

#include "CloakEngine/Interface/Label.h"
#include "Implementation/Interface/BasicGUI.h"

#pragma warning(push)
#pragma warning(disable:4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace Label_v1 {
				class CLOAK_UUID("{E4C9DC8D-ABC6-458C-9676-E23A2C32DB48}") Label : public virtual API::Interface::Label_v1::ILabel, public virtual BasicGUI_v1::BasicGUI {
					public:
						CLOAK_CALL_THIS Label();
						virtual CLOAK_CALL_THIS ~Label();
						virtual void CLOAK_CALL_THIS SetFont(In API::Files::IFont* font) override;
						virtual void CLOAK_CALL_THIS SetFontSize(In API::Files::FontSize size) override;
						virtual void CLOAK_CALL_THIS SetFontColor(In const API::Interface::FontColor& color) override;
						virtual void CLOAK_CALL_THIS SetJustify(In API::Files::Justify hJust) override;
						virtual void CLOAK_CALL_THIS SetText(In API::Files::IString* text) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::u16string& text) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::string& text) override;
						virtual void CLOAK_CALL_THIS SetJustify(In API::Files::Justify hJust, In API::Files::AnchorPoint vJust) override;
						virtual void CLOAK_CALL_THIS SetJustify(In API::Files::AnchorPoint vJust) override;
						virtual void CLOAK_CALL_THIS SetLetterSpace(In float space) override;
						virtual void CLOAK_CALL_THIS SetLineSpace(In float space) override;
						virtual API::Files::FontSize CLOAK_CALL_THIS GetFontSize() const override;
						virtual const API::Interface::FontColor& CLOAK_CALL_THIS GetFontColor() const override;
						virtual API::Files::AnchorPoint CLOAK_CALL_THIS GetVerticalJustify() const override;
						virtual API::Files::Justify CLOAK_CALL_THIS GetHorizontalJustify() const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;
						virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;

						API::Files::IFont* m_font;
						API::Files::FontSize m_fsize;
						API::Interface::FontColor m_fcolor;
						API::Files::Justify m_hJustify;
						API::Files::AnchorPoint m_vJustify;
						float m_lineSpace;
						float m_letterSpace;
						bool m_ownText;
						API::Files::IString* m_text;

						API::Files::IFont* m_d_font;
						API::Files::IString* m_d_text;
						API::Files::TextDesc m_d_textDesc;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif