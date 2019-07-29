#pragma once
#ifndef CE_IMPL_GLOBAL_LOCALIZATION_H
#define CE_IMPL_GLOBAL_LOCALIZATION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Localization.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Localization {
				void CLOAK_CALL Initialize();
				void CLOAK_CALL Release();
				bool CLOAK_CALL GetState(In API::Global::Localization::LanguageFileID fID);
			}
		}
	}
}

#endif