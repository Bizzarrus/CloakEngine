#pragma once
#ifndef CE_API_INTERFACE_LABEL_H
#define CE_API_INTERFACE_LABEL_H

#include "CloakEngine/Interface/BasicGUI.h"
#include "CloakEngine/Files/Font.h"
#include "CloakEngine/Global/Localization.h"
#include "CloakEngine/Helper/Color.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace Label_v1 {
				struct FontColor {
					Helper::Color::RGBA Color[3];
				};
				CLOAK_INTERFACE_ID("{18BFEBF9-C58E-4902-86F1-10C6DAD4BBFB}") ILabel : public virtual BasicGUI_v1::IBasicGUI {
					public:
						virtual void CLOAK_CALL_THIS SetFont(In Files::IFont* font) = 0;
						virtual void CLOAK_CALL_THIS SetFontSize(In Files::FontSize size) = 0;
						virtual void CLOAK_CALL_THIS SetFontColor(In const FontColor& color) = 0;
						virtual void CLOAK_CALL_THIS SetJustify(In Files::Justify hJust) = 0;
						virtual void CLOAK_CALL_THIS SetJustify(In Files::Justify hJust, In Files::AnchorPoint vJust) = 0;
						virtual void CLOAK_CALL_THIS SetJustify(In Files::AnchorPoint vJust) = 0;
						virtual void CLOAK_CALL_THIS SetLetterSpace(In float space) = 0;
						virtual void CLOAK_CALL_THIS SetLineSpace(In float space) = 0;
						virtual void CLOAK_CALL_THIS SetText(In API::Files::IString* text) = 0;
						virtual void CLOAK_CALL_THIS SetText(In const std::u16string& text) = 0;
						virtual void CLOAK_CALL_THIS SetText(In const std::string& text) = 0;
						virtual Files::FontSize CLOAK_CALL_THIS GetFontSize() const = 0;
						virtual const FontColor& CLOAK_CALL_THIS GetFontColor() const = 0;
						virtual Files::AnchorPoint CLOAK_CALL_THIS GetVerticalJustify() const = 0;
						virtual Files::Justify CLOAK_CALL_THIS GetHorizontalJustify() const = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif
