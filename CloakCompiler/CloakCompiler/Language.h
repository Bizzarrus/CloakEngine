#pragma once
#ifndef CC_API_LANGUAGE_H
#define CC_API_LANGUAGE_H

#include "CloakCompiler/Defines.h"
#include "CloakCompiler/EncoderBase.h"
#include "CloakEngine/CloakEngine.h"

#include <unordered_map>

namespace CloakCompiler {
	CLOAKCOMPILER_API_NAMESPACE namespace API {
		namespace Language {
			namespace v1000 {
				constexpr CloakEngine::Files::FileType FileType{ "TextTranslation","CETT",1003 };
				enum class RespondeType { LANGUAGE, TEXT, SAVE, HEADER };
				typedef size_t Language;
				typedef size_t TextID;
				struct LanguageDesc {
					std::unordered_map<TextID, std::u16string> Text;
				};
				struct Desc {
					std::unordered_map<TextID, std::string> TextNames;
					std::unordered_map<Language, LanguageDesc> Translation;
					bool CreateHeader = true;
					std::string HeaderName = "";
					std::u16string HeaderPath = u"";
				};
				struct RespondeInfo {
					Language Lan;
					TextID Text;
					RespondeType Type;
				};
				CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In std::function<void(const RespondeInfo&)> response = false);
			}
			namespace v1002 {
				constexpr CloakEngine::Files::FileType FileType{ "TextTranslation","CETT",1002 };
				typedef size_t Language;
				typedef size_t TextID;
				struct LanguageDesc {
					CloakEngine::HashMap<TextID, std::u16string> Text;
				};
				struct Desc {
					CloakEngine::HashMap<TextID, std::string> TextNames;
					CloakEngine::HashMap<Language, LanguageDesc> Translation;
				};
				struct HeaderDesc {
					bool CreateHeader = true;
					std::string HeaderName;
					std::u16string Path = u"";
				};
				enum class ResponseCode {
					READ_TEMP,
					WRITE_TEMP,
					WRITE_HEADER,
					FINISH_TEXT,
					FINISH_LANGUAGE
				};
				struct RespondeInfo {
					ResponseCode Code;
					TextID Text;
					Language Language;
				};
				typedef std::function<void(const RespondeInfo&)> ResponseFunc;
				CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In_opt const HeaderDesc& header, In ResponseFunc response);
			}
		}
	}
}

#endif