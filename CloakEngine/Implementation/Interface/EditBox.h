#pragma once
#ifndef CE_IMPL_INTERFACE_EDITBOX_H
#define CE_IMPL_INTERFACE_EDITBOX_H

#include "CloakEngine/Interface/EditBox.h"
#include "Implementation/Interface/Label.h"
#include "Implementation/Interface/Frame.h"

#include <atomic>
#include <string>

#pragma warning(push)
#pragma warning(disable:4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace EditBox_v1 {
				class CLOAK_UUID("{8C2F1F18-60B3-4254-B3CE-20EA4DA48F31}") EditBox : public virtual API::Interface::EditBox_v1::IEditBox, public Label_v1::Label, public Frame_v1::Frame, IMPL_EVENT(onClick), IMPL_EVENT(onEnterKey), IMPL_EVENT(onEnterChar), IMPL_EVENT(onScroll) {
					public:
						CLOAK_CALL_THIS EditBox();
						virtual CLOAK_CALL_THIS ~EditBox();
						virtual void CLOAK_CALL_THIS SetCursorColor(In const API::Helper::Color::RGBX& color) override;
						virtual void CLOAK_CALL_THIS SetCursorPos(In size_t pos) override;
						virtual void CLOAK_CALL_THIS SetCursor(In API::Files::ITexture* img) override;
						virtual void CLOAK_CALL_THIS SetCursorWidth(In float w) override;
						virtual void CLOAK_CALL_THIS SetNumeric(In bool numeric, In_opt bool allowFloat = true) override;
						virtual void CLOAK_CALL_THIS SetMaxLetter(In size_t length = 0) override;
						virtual void CLOAK_CALL_THIS SetTextPos(In const API::Global::Math::Space2D::Vector& offset, In_opt float maxWidth = -1.0f) override;
						virtual void CLOAK_CALL_THIS SetText(In API::Files::IString* text) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::u16string& str) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::string& str) override;
						virtual bool CLOAK_CALL_THIS IsNumeric() const override;
						virtual bool CLOAK_CALL_THIS IsClickable() const override;
						virtual size_t CLOAK_CALL_THIS GetMaxLetter() const override;
						virtual size_t CLOAK_CALL_THIS GetCursorPos() const override;
						virtual std::string CLOAK_CALL_THIS GetText() const override;
						virtual void CLOAK_CALL_THIS SetFont(In API::Files::IFont* font) override;
						virtual void CLOAK_CALL_THIS SetFontSize(In API::Files::FontSize size) override;

						virtual void CLOAK_CALL_THIS Initialize() override;

						virtual void CLOAK_CALL_THIS OnEnterKey(In const API::Global::Events::onEnterChar& ev);
						virtual void CLOAK_CALL_THIS OnEnterKey(In const API::Global::Events::onEnterKey& ev);
						virtual void CLOAK_CALL_THIS OnLooseFocus(In const API::Global::Events::onLooseFocus& ev);
						virtual void CLOAK_CALL_THIS OnGainFocus(In const API::Global::Events::onGainFocus& ev);
						virtual void CLOAK_CALL_THIS OnClick(const API::Global::Events::onClick& ev);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;
						virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;
						virtual void CLOAK_CALL_THIS iCheckPos();

						std::string m_curString;
						std::string m_lastString;
						size_t m_maxLetter;
						bool m_numeric;
						bool m_numericFloat;
						size_t m_cursorPos;
						size_t m_hookPos;
						bool m_justifyLeft;
						API::Global::Math::Space2D::Vector m_textOffset;
						float m_maxWidth;
						bool m_gotSurrogate;
						char16_t m_lastSurrogate;
						API::Helper::Color::RGBA m_cursorColor;
						float m_cursorWidth;
						float m_cursorX;
						API::Files::ITexture* m_cursorImg;

						API::Rendering::VERTEX_2D m_d_cursorVertex;
				};
#ifdef DISABLED
				class CLOAK_UUID("{F6C6128F-AB74-47BF-8B82-5C899FDA47B4}") MultiLineEditBox : public virtual API::Interface::IMultiLineEditBox, public EditBox {
					public:
						CLOAK_CALL_THIS MultiLineEditBox();
						virtual CLOAK_CALL_THIS ~MultiLineEditBox();

						virtual void CLOAK_CALL_THIS SetLineSpace(In float space) override;
						virtual void CLOAK_CALL_THIS OnEnterKey(In const API::Global::Events::onEnterChar& ev) override;
						virtual void CLOAK_CALL_THIS OnEnterKey(In const API::Global::Events::onEnterKey& ev) override;
						virtual void CLOAK_CALL_THIS OnClick(const API::Global::Events::onClick& ev) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::u16string& str) override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iCheckPos() override;
						virtual void CLOAK_CALL_THIS iDecreaseEnd() override;
						virtual void CLOAK_CALL_THIS iChangeLine(In size_t newLine);

						struct Line { size_t Begin; size_t End; };
						API::List<Line> m_lines;
						size_t m_curLine;
						size_t m_drawLineBegin;
						size_t m_drawLineEnd;
				};
#endif
			}
		}
	}
}

#pragma warning(pop)

#endif