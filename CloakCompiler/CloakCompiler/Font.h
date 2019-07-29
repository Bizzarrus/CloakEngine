#pragma once
#ifndef CC_API_FONT_H
#define CC_API_FONT_H

#include "CloakCompiler/Defines.h"
#include "CloakCompiler/EncoderBase.h"
#include "CloakEngine/CloakEngine.h"

#include <string>
#include <unordered_map>
#include <functional>

namespace CloakCompiler {
	CLOAKCOMPILER_API_NAMESPACE namespace API {
		namespace Font {
			constexpr CloakEngine::Files::FileType FileType{ "BitFont","CEBF",1003 };
			enum class LetterColorType { SINGLECOLOR, MULTICOLOR, TRUECOLOR };
			enum class RespondeType { STARTED, COMPLETED,FAIL_NO_IMG };
			struct Letter {
				std::u16string FilePath;
				LetterColorType Colored = LetterColorType::SINGLECOLOR;
			};
			struct Desc {
				std::unordered_map<char32_t,Letter> Alphabet;
				unsigned int SpaceWidth = 0;
				unsigned int BitLen = 0;
			};
			struct RespondeInfo {
				char32_t Letter;
				RespondeType Type;
			};
			CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In std::function<void(const RespondeInfo&)> response = false);
		}
	}
}

#endif