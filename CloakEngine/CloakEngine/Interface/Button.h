#pragma once
#ifndef CE_API_INTERFACE_BUTTON_H
#define CE_API_INTERFACE_BUTTON_H

#include "CloakEngine/Interface/Frame.h"
#include "CloakEngine/Interface/Label.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace Button_v1 {
				CLOAK_INTERFACE_ID("{31E97730-FFDA-410E-95F2-1F1BE353FA46}") IButton : public virtual Frame_v1::IFrame, public virtual Label_v1::ILabel, public virtual Helper::IEventCollector<Global::Events::onEnterKey, Global::Events::onClick, Global::Events::onScroll> {
					public:
						virtual void CLOAK_CALL_THIS SetHighlightTexture(In Files::ITexture* img, In_opt Frame_v1::HighlightType highlight = Frame_v1::HighlightType::OVERRIDE) = 0;
						virtual void CLOAK_CALL_THIS SetPressedTexture(In Files::ITexture* img, In_opt Frame_v1::HighlightType highlight = Frame_v1::HighlightType::OVERRIDE) = 0;
						virtual void CLOAK_CALL_THIS SetText(In API::Files::IString* text, In const Global::Math::Space2D::Vector& offset, In_opt float maxWidth = -1.0f) = 0;
						virtual void CLOAK_CALL_THIS SetText(In API::Files::IString* text) = 0;
						virtual void CLOAK_CALL_THIS SetText(In const std::u16string& text, In const Global::Math::Space2D::Vector& offset, In_opt float maxWidth = -1.0f) = 0;
						virtual void CLOAK_CALL_THIS SetText(In const std::u16string& text) = 0;
						virtual void CLOAK_CALL_THIS SetText(In const std::string& text, In const Global::Math::Space2D::Vector& offset, In_opt float maxWidth = -1.0f) = 0;
						virtual void CLOAK_CALL_THIS SetText(In const std::string& text) = 0;
						virtual void CLOAK_CALL_THIS SetTextMaxWidth(In float maxWidth) = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif