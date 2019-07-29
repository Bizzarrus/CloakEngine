#pragma once
#ifndef CE_API_INTERFACE_CHECKBUTTON_H
#define CE_API_INTERFACE_CHECKBUTTON_H

#include "CloakEngine/Interface/Frame.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace CheckButton_v1 {
				CLOAK_INTERFACE_ID("{9C24DCE7-1CEE-4578-A6A2-0A6A0CBBC2DD}") ICheckButton : public virtual Frame_v1::IFrame, public virtual Helper::IEventCollector<Global::Events::onEnterKey, Global::Events::onClick, Global::Events::onCheck, Global::Events::onScroll>{
					public:	
						virtual void CLOAK_CALL_THIS SetHighlightTexture(In Files::ITexture* img, In_opt Frame_v1::HighlightType highlight = Frame_v1::HighlightType::OVERRIDE) = 0;
						virtual void CLOAK_CALL_THIS SetPressedTexture(In Files::ITexture* img, In_opt Frame_v1::HighlightType highlight = Frame_v1::HighlightType::OVERRIDE) = 0;
						virtual void CLOAK_CALL_THIS SetCheckedTexture(In Files::ITexture* img, In_opt Frame_v1::HighlightType highlight = Frame_v1::HighlightType::OVERRIDE) = 0;
						virtual void CLOAK_CALL_THIS SetChecked(In bool checked) = 0;
						virtual bool CLOAK_CALL_THIS IsChecked() const = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif