#pragma once
#ifndef CE_API_HELPER_GETUID_H
#define CE_API_HELPER_GETUID_H

#include "CloakEngine\Defines.h"
#include "CloakEngine\Helper\UIDs.h"
#include "CloakEngine\Helper\SavePtr.h"
#include "CloakEngine\Helper\SyncSection.h"
#include "CloakEngine\Files\Compressor.h"
#include "CloakEngine\Files\FileHandler.h"
#include "CloakEngine\Files\Language.h"
#include "CloakEngine\Files\Configuration.h"
#include "CloakEngine\Interface\BasicGUI.h"
#include "CloakEngine\Interface\Button.h"
#include "CloakEngine\Interface\CheckButton.h"
#include "CloakEngine\Interface\DropDownMenu.h"
#include "CloakEngine\Interface\EditBox.h"
#include "CloakEngine\Interface\Frame.h"
#include "CloakEngine\Interface\Label.h"
#include "CloakEngine\Interface\Slider.h"

#define CLOAKENGINE_IUID_PTR(ptr) CloakEngine::API::Helper::getUID(*ptr),ptr

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Helper {
			inline namespace GetUID_v1 {
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(SavePtr_v1::ISavePtr*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(SyncSection_v1::ISyncSection*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Compressor_v1::ICompressor*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Compressor_v1::IReader*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Compressor_v1::IWriter*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::FileHandler_v1::IFileHandler*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Language_v1::ILanguage*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Configuration_v1::IConfiguration*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::BasicGUI_v1::IBasicGUI*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Button_v1::IButton*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::CheckButton_v1::ICheckButton*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::DropDownMenu_v1::IDropDownMenu*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::EditBox_v1::IEditBox*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Frame_v1::IFrame*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Label_v1::ILabel*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Slider_v1::IHSlider*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Slider_v1::ISlider*);
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Slider_v1::IVSlider*);
			}
		}
	}
}

#endif
