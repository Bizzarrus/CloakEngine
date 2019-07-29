#pragma once
#ifndef CE_API_INTERFACE_EDITBOX_H
#define CE_API_INTERFACE_EDITBOX_H

#include "CloakEngine/Interface/Label.h"
#include "CloakEngine/Interface/Frame.h"
#include "CloakEngine/Helper/Color.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Files/Font.h"

#include <string>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Interface {
			inline namespace EditBox_v1 {
				CLOAK_INTERFACE_ID("{9EA4A6FE-BA4B-4C50-B5AC-81FC6F2A6027}") IEditBox : public virtual Label_v1::ILabel, public virtual Frame_v1::IFrame, public virtual Helper::IEventCollector<Global::Events::onEnterKey, Global::Events::onEnterChar, Global::Events::onClick, Global::Events::onScroll>{
					public:
						virtual void CLOAK_CALL_THIS SetCursorColor(In const Helper::Color::RGBX& color) = 0;
						virtual void CLOAK_CALL_THIS SetCursorPos(In size_t pos) = 0;
						virtual void CLOAK_CALL_THIS SetCursor(In Files::ITexture* img) = 0;
						virtual void CLOAK_CALL_THIS SetCursorWidth(In float w) = 0;
						virtual void CLOAK_CALL_THIS SetNumeric(In bool numeric, In_opt bool allowFloat = true) = 0;
						virtual void CLOAK_CALL_THIS SetMaxLetter(In size_t length = 0) = 0;
						virtual void CLOAK_CALL_THIS SetTextPos(In const Global::Math::Space2D::Vector& offset, In_opt float maxWidth = -1.0f) = 0;
						virtual bool CLOAK_CALL_THIS IsNumeric() const = 0;
						virtual size_t CLOAK_CALL_THIS GetMaxLetter() const = 0;
						virtual size_t CLOAK_CALL_THIS GetCursorPos() const = 0;
						virtual std::string CLOAK_CALL_THIS GetText() const = 0;
				};
				CLOAK_INTERFACE_ID("{F6495D43-D3C0-4EC5-A3EB-A90D8A51EA82}") IMultiLineEditBox : public virtual IEditBox{
					public:

				};
			}
		}
	}
}

#pragma warning(pop)

#endif
