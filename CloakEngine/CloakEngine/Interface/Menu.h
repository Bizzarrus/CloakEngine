#pragma once
#ifndef CE_API_INTERFACE_MENU_H
#define CE_API_INTERFACE_MENU_H

#include "CloakEngine/Interface/Frame.h"
#include "CloakEngine/Global/Localization.h"
#include "CloakEngine/Interface/Label.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace Menu_v1 {
				typedef unsigned long long ButtonID;
			}
		}
		namespace Global {
			namespace Events {
				struct onMenuButtonClick : public onClick {
					Interface::ButtonID menuButtonID;
					bool isChecked;
				};
			}
		}
		namespace Interface {
			inline namespace Menu_v1 {
				struct MENU_ITEM {
					API::Files::IString* Text = nullptr;
					MENU_ITEM* SubMenu = nullptr;
					size_t SubMenuSize = 0;
					bool Checked = false;
					bool CanChecked = false;
					ButtonID ID = 0;
				};
				struct MENU_DESC {
					MENU_ITEM* Menu = nullptr;
					size_t MenuSize = 0;
					CLOAKENGINE_API bool CLOAK_CALL IsValid() const;
				};

				CLOAK_INTERFACE_ID("{72F2C3F9-59A3-492F-A8D9-6E82D4B5E1EB}") IMenu : public virtual Frame_v1::IFrame, public virtual Helper::IEventCollector<API::Global::Events::onMenuButtonClick> {
					public:
						virtual void CLOAK_CALL_THIS SetMenu(In const MENU_DESC& desc) = 0;
						virtual void CLOAK_CALL_THIS SetButtonTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetButtonHighlightTexture(In Files::ITexture* img, In_opt HighlightType highlight = API::Interface::Frame_v1::HighlightType::OVERRIDE) = 0;
						virtual void CLOAK_CALL_THIS SetButtonCheckTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetButtonSubMenuTexture(In Files::ITexture* open, In Files::ITexture* closed) = 0;
						virtual void CLOAK_CALL_THIS SetButtonWidth(In float W) = 0;
						virtual void CLOAK_CALL_THIS SetButtonHeight(In float H) = 0;
						virtual void CLOAK_CALL_THIS SetButtonSize(In float W, In float H) = 0;
						virtual void CLOAK_CALL_THIS SetButtonSymbolWidth(In float W) = 0;
						virtual void CLOAK_CALL_THIS SetButtonTextOffset(In const Global::Math::Space2D::Vector& offset) = 0;

						virtual void CLOAK_CALL_THIS SetButtonFont(In Files::IFont* font) = 0;
						virtual void CLOAK_CALL_THIS SetButtonFontSize(In Files::FontSize size) = 0;
						virtual void CLOAK_CALL_THIS SetButtonFontColor(In const FontColor& color) = 0;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In Files::Justify hJust) = 0;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In Files::Justify hJust, In Files::AnchorPoint vJust) = 0;
						virtual void CLOAK_CALL_THIS SetButtonJustify(In Files::AnchorPoint vJust) = 0;
						virtual void CLOAK_CALL_THIS SetButtonLetterSpace(In float space) = 0;
						virtual void CLOAK_CALL_THIS SetButtonLineSpace(In float space) = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif