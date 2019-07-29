#pragma once
#ifndef CE_API_INTERFACE_LISTMENU_H
#define CE_API_INTERFACE_LISTMENU_H

#include "CloakEngine/Interface/Menu.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace ListMenu_v1 {
				CLOAK_INTERFACE_ID("{25E7726F-E132-4C7B-90A8-961E659FCA76}") IListMenu : public virtual Menu_v1::IMenu, public virtual Helper::IEventCollector<Global::Events::onEnterKey, Global::Events::onClick, Global::Events::onScroll>{
					public:
						virtual void CLOAK_CALL_THIS SetLevelStepSize(In float s) = 0;
						virtual void CLOAK_CALL_THIS SetSliderWidth(In float w) = 0;
						virtual void CLOAK_CALL_THIS SetSliderTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetSliderThumbTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetSliderLeftArrowTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetSliderRightArrowTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetSliderArrowHeight(In float w) = 0;
						virtual void CLOAK_CALL_THIS SetSliderThumbHeight(In float w) = 0;
						virtual void CLOAK_CALL_THIS SetSliderStepSize(In float ss) = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif