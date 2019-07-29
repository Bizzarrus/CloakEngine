#pragma once
#ifndef CE_API_INTERFACE_FRAME_H
#define CE_API_INTERFACE_FRAME_H

#include "CloakEngine/Interface/BasicGUI.h"
#include "CloakEngine/Files/Image.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace Frame_v1 {
				enum class HighlightType { ADD, OVERRIDE };
				enum class BorderType { SIDE, TOP, BOTTOM };
				CLOAK_INTERFACE_ID("{0AA73F69-4714-4C44-B5F3-265A9FFDE347}") IFrame : public virtual BasicGUI_v1::IBasicGUI {
					public:
						virtual void CLOAK_CALL_THIS Enable() = 0;
						virtual void CLOAK_CALL_THIS Disable() = 0;
						virtual void CLOAK_CALL_THIS SetBackgroundTexture(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetBorderSize(In BorderType Border, In float size) = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif