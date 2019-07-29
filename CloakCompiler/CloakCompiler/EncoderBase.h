#pragma once
#ifndef CC_API_ENCODERBASE_H
#define CC_API_ENCODERBASE_H

#include "Defines.h"
#include "CloakEngine/CloakEngine.h"

#include <string>

namespace CloakCompiler {
	CLOAKCOMPILER_API_NAMESPACE namespace API {
		enum class EncodeFlags {
			NONE = 0x0,
			NO_TEMP_READ = 0x1,
			NO_TEMP_WRITE = 0x2,
		};
		DEFINE_ENUM_FLAG_OPERATORS(EncodeFlags);
		struct EncodeDesc {
			std::u16string tempPath = u"";
			EncodeFlags flags = EncodeFlags::NONE;
			CloakEngine::Files::UGID targetGameID;
		};
		enum class LibGenerator {
			VisualStudio_2019,
			VisualStudio_2017,
			VisualStudio_2015,
			Ninja,
		};
	}
}

#endif