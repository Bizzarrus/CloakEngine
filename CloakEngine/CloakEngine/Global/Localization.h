#pragma once
#ifndef CE_API_GLOBAL_LOCALIZATION_H
#define CE_API_GLOBAL_LOCALIZATION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Files/Language.h"

#include <string>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Localization {
				typedef unsigned int LanguageFileID;
				typedef Files::TextID TextID;
				typedef Files::LanguageID Language;

				CLOAKENGINE_API LanguageFileID CLOAK_CALL registerLanguageFile(In Files::ILanguage* file);
				CLOAKENGINE_API void CLOAK_CALL setLanguage(In Language lan);
				CLOAKENGINE_API void CLOAK_CALL setWindowText(In API::Files::IString* str);
				CLOAKENGINE_API API::Files::IString* CLOAK_CALL getText(In LanguageFileID fID, In TextID id);
				CLOAKENGINE_API Language CLOAK_CALL getCurrentLanguage();

				namespace Chars {
					namespace Gamepad {
						constexpr char16_t DPad_Up				= u'\uE000';
						constexpr char16_t DPad_Down			= u'\uE001';
						constexpr char16_t DPad_Left			= u'\uE002';
						constexpr char16_t DPad_Right			= u'\uE003';
						constexpr char16_t Start				= u'\uE004';
						constexpr char16_t Back					= u'\uE005';
						constexpr char16_t LeftThumb			= u'\uE006';
						constexpr char16_t RightThumb			= u'\uE007';
						constexpr char16_t LeftThumb_Up			= u'\uE008';
						constexpr char16_t RightThumb_Up		= u'\uE009';
						constexpr char16_t LeftThumb_Down		= u'\uE00A';
						constexpr char16_t RightThumb_Down		= u'\uE00B';
						constexpr char16_t LeftThumb_Left		= u'\uE00C';
						constexpr char16_t RightThumb_Left		= u'\uE00D';
						constexpr char16_t LeftThumb_Right		= u'\uE00E';
						constexpr char16_t RightThumb_Right		= u'\uE00F';
						constexpr char16_t LeftThumb_Pressed	= u'\uE010';
						constexpr char16_t RightThumb_Pressed	= u'\uE011';
						constexpr char16_t LeftTrigger			= u'\uE012';
						constexpr char16_t RightTrigger			= u'\uE013';
						constexpr char16_t LeftShoulder			= u'\uE014';
						constexpr char16_t RightShoulder		= u'\uE015';
						constexpr char16_t A					= u'\uE016';
						constexpr char16_t B					= u'\uE017';
						constexpr char16_t X					= u'\uE018';
						constexpr char16_t Y					= u'\uE019';
					}
				}
			}
		}
	}
}

#endif