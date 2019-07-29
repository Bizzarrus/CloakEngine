#pragma once
#ifndef CE_IMPL_INTERFACE_CHECKBUTTON_H
#define CE_IMPL_INTERFACE_CHECKBUTTON_H

#include "CloakEngine/Interface/CheckButton.h"
#include "Implementation/Interface/Frame.h"

#pragma warning(push)
#pragma warning(disable:4250)

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			inline namespace CheckButton_v1 {
				class CLOAK_UUID("{73FB5F26-73AF-419E-8185-2DB233987F04}") CheckButton : public virtual API::Interface::CheckButton_v1::ICheckButton, public Impl::Interface::Frame_v1::Frame, IMPL_EVENT(onClick), IMPL_EVENT(onEnterKey), IMPL_EVENT(onCheck), IMPL_EVENT(onScroll){
					public:
						CLOAK_CALL_THIS CheckButton();
						virtual CLOAK_CALL_THIS ~CheckButton();
						virtual void CLOAK_CALL_THIS SetHighlightTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight = API::Interface::Frame_v1::HighlightType::OVERRIDE) override;
						virtual void CLOAK_CALL_THIS SetPressedTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight = API::Interface::Frame_v1::HighlightType::OVERRIDE) override;
						virtual void CLOAK_CALL_THIS SetCheckedTexture(In API::Files::ITexture* img, In_opt API::Interface::Frame_v1::HighlightType highlight = API::Interface::Frame_v1::HighlightType::OVERRIDE) override;
						virtual void CLOAK_CALL_THIS SetChecked(In bool checked) override;
						virtual bool CLOAK_CALL_THIS IsChecked() const override;
						virtual void CLOAK_CALL_THIS setPressed(In bool pressed);
						virtual void CLOAK_CALL_THIS setMouseOver(In bool mouseOver);
						virtual bool CLOAK_CALL_THIS IsClickable() const override;

						virtual void CLOAK_CALL_THIS Initialize() override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iDraw(In API::Rendering::Context_v1::IContext2D* context) override;
						virtual bool CLOAK_CALL_THIS iUpdateDrawInfo(In unsigned long long time) override;

						bool m_isChecked;
						bool m_isPressed;
						bool m_isMouseOver;
						API::Files::ITexture* m_imgChecked;
						API::Files::ITexture* m_imgPressed;
						API::Files::ITexture* m_imgHighlight;
						API::Interface::Frame_v1::HighlightType m_checked;
						API::Interface::Frame_v1::HighlightType m_pressed;
						API::Interface::Frame_v1::HighlightType m_highlight;

						ImageDrawInfo m_d_checked;
						ImageDrawInfo m_d_pressed;
						ImageDrawInfo m_d_mouseOver;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif