#pragma once
#ifndef CE_IMPL_GLOBAL_LOG_H
#define CE_IMPL_GLOBAL_LOG_H

#include "CloakEngine/Defines.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Log {
				void CLOAK_CALL onStart(In size_t threadID);
				void CLOAK_CALL onUpdate(In size_t threadID, In API::Global::Time etime);
				void CLOAK_CALL onStop(In size_t threadID);
				void CLOAK_CALL Release();
				void CLOAK_CALL Initialize();
			}
		}
	}
}

#endif