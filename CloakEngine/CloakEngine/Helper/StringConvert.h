#pragma once
#ifndef CE_API_HELPER_STRINGCONVERT_H
#define CE_API_HELPER_STRINGCONVERT_H

#include "CloakEngine/Defines.h"

#include <string>

namespace std {
#ifdef UNICODE
	typedef wstring tstring;
#else
	typedef string tstring;
#endif
}

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Helper {
			namespace StringConvert {
				CLOAKENGINE_API std::string CLOAK_CALL ConvertToU8(In const std::u32string& str);
				CLOAKENGINE_API std::string CLOAK_CALL ConvertToU8(In const std::u16string& str);
				CLOAKENGINE_API std::string CLOAK_CALL ConvertToU8(In const std::string& str);
				CLOAKENGINE_API std::string CLOAK_CALL ConvertToU8(In const std::wstring& str);
				CLOAKENGINE_API std::u16string CLOAK_CALL ConvertToU16(In const std::string& str);
				CLOAKENGINE_API std::u16string CLOAK_CALL ConvertToU16(In const std::wstring& str);
				CLOAKENGINE_API std::u16string CLOAK_CALL ConvertToU16(In const std::u16string& str);
				CLOAKENGINE_API std::u16string CLOAK_CALL ConvertToU16(In const std::u32string& str);
				CLOAKENGINE_API std::wstring CLOAK_CALL ConvertToW(In const std::string& str);
				CLOAKENGINE_API std::wstring CLOAK_CALL ConvertToW(In const std::wstring& str);
				CLOAKENGINE_API std::wstring CLOAK_CALL ConvertToW(In const std::u16string& str);
				CLOAKENGINE_API std::wstring CLOAK_CALL ConvertToW(In const std::u32string& str);
				CLOAKENGINE_API std::u32string CLOAK_CALL ConvertToU32(In const std::string& str);
				CLOAKENGINE_API std::u32string CLOAK_CALL ConvertToU32(In const std::wstring& str);
				CLOAKENGINE_API std::u32string CLOAK_CALL ConvertToU32(In const std::u16string& str);
				CLOAKENGINE_API std::u32string CLOAK_CALL ConvertToU32(In const std::u32string& str);
				CLOAKENGINE_API std::tstring CLOAK_CALL ConvertToT(In const std::string& str);
				CLOAKENGINE_API std::tstring CLOAK_CALL ConvertToT(In const std::wstring& str);
				CLOAKENGINE_API std::tstring CLOAK_CALL ConvertToT(In const std::u16string& str);
				CLOAKENGINE_API std::tstring CLOAK_CALL ConvertToT(In const std::u32string& str);
				CLOAKENGINE_API char32_t CLOAK_CALL ConvertToU32(In const char16_t& str);
				CLOAKENGINE_API char32_t CLOAK_CALL ConvertToU32(In const char32_t& str);
				CLOAKENGINE_API char32_t CLOAK_CALL ConvertToU32(In const wchar_t& str);
				CLOAKENGINE_API char16_t CLOAK_CALL ConvertToU16(In const wchar_t& c);
				CLOAKENGINE_API char16_t CLOAK_CALL ConvertToU16(In const char16_t& c);
				CLOAKENGINE_API char16_t CLOAK_CALL ConvertToU16(In const char32_t& c);
				CLOAKENGINE_API size_t CLOAK_CALL UTF8Length(In const std::string& str);
			}
		}
	}
}

#endif