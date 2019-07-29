#pragma once
#ifndef CE_IMPL_GLOBAL_STEAM_H
#define CE_IMPL_GLOBAL_STEAM_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Steam.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Steam {
				void CLOAK_CALL Initialize(In bool use);
				void CLOAK_CALL Update();
				void CLOAK_CALL RequestData();
				void CLOAK_CALL Release();
			}
		}
	}
}

#endif
