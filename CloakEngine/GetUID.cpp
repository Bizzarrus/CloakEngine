#include "stdafx.h"
#include "CloakEngine\Helper\GetUID.h"

namespace CloakEngine {
	namespace API {
		namespace Helper {
			namespace GetUID_v1 {
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(SavePtr_v1::ISavePtr*) { return IUID::SavePtr_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(SyncSection_v1::ISyncSection*) { return IUID::SyncSection_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Compressor_v1::ICompressor*) { return IUID::FileCompressor_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Compressor_v1::IReader*) { return IUID::FileReader_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Compressor_v1::IWriter*) { return IUID::FileWriter_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::FileHandler_v1::IFileHandler*) { return IUID::FileHandler_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Language_v1::ILanguage*) { return IUID::Language_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Files::Configuration_v1::IConfiguration*) { return IUID::Configuration_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::BasicGUI_v1::IBasicGUI*) { return IUID::BasicGUI_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Button_v1::IButton*) { return IUID::Button_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::CheckButton_v1::ICheckButton*) { return IUID::CheckButton_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::DropDownMenu_v1::IDropDownMenu*) { return IUID::DropDownMenu_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::EditBox_v1::IEditBox*) { return IUID::EditBox_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Frame_v1::IFrame*) { return IUID::Frame_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Label_v1::ILabel*) { return IUID::Label_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Slider_v1::IHSlider*) { return IUID::HSlider_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Slider_v1::ISlider*) { return IUID::Slider_v1; }
				CLOAKENGINE_API inline IUID CLOAK_CALL getUID(Interface::Slider_v1::IVSlider*) { return IUID::VSlider_v1; }
			}
		}
	}
}