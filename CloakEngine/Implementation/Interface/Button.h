#pragma once
#ifndef CE_IMPL_INTERFACE_BUTTON_H
#define CE_IMPL_INTERFACE_BUTTON_H

#include "CloakEngine/Interface/Button.h"
#include "Implementation/Interface/Frame.h"
#include "Implementation/Interface/Label.h"

#pragma warning(push)
#pragma warning(disable:4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace Button_v1 {
				class CLOAK_UUID("{6BF5CF10-7C3E-46ED-B336-33E0504A0369}") Button : public virtual API::Interface::Button_v1::IButton, public Frame_v1::Frame, public Label_v1::Label, IMPL_EVENT(onClick), IMPL_EVENT(onEnterKey), IMPL_EVENT(onScroll){
					public:
						CLOAK_CALL_THIS Button();
						virtual CLOAK_CALL_THIS ~Button();
						virtual void CLOAK_CALL_THIS SetHighlightTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight = API::Interface::Frame_v1::HighlightType::OVERRIDE) override;
						virtual void CLOAK_CALL_THIS SetPressedTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight = API::Interface::Frame_v1::HighlightType::OVERRIDE) override;
						virtual void CLOAK_CALL_THIS SetText(In API::Files::IString* text, In const API::Global::Math::Space2D::Vector& offset, In_opt float maxWidth = -1.0f) override;
						virtual void CLOAK_CALL_THIS SetText(In API::Files::IString* text) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::u16string& text, In const API::Global::Math::Space2D::Vector& offset, In_opt float maxWidth = -1.0f) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::u16string& text) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::string& text, In const API::Global::Math::Space2D::Vector& offset, In_opt float maxWidth = -1.0f) override;
						virtual void CLOAK_CALL_THIS SetText(In const std::string& text) override;
						virtual void CLOAK_CALL_THIS SetTextMaxWidth(In float maxWidth) override;
						virtual bool CLOAK_CALL_THIS IsClickable() const override;

						virtual void CLOAK_CALL_THIS Initialize() override;

						virtual void CLOAK_CALL_THIS setMouseOver(In bool mouseOver);
						virtual void CLOAK_CALL_THIS OnClick(const API::Global::Events::onClick& ev);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;
						virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;

						API::Global::Math::Space2D::Vector m_textOffset;
						API::Files::ITexture* m_imgHighlight;
						API::Interface::HighlightType m_highlight;
						API::Files::ITexture* m_imgPressed;
						API::Interface::HighlightType m_pressed;
						float m_maxTextW;
						bool m_isPressed;
						bool m_isHighlight;
						ImageDrawInfo m_d_highlight;
						ImageDrawInfo m_d_pressed;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif