#pragma once
#ifndef CE_E_PHYSICS_CORE_H
#define CE_E_PHYSICS_CORE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SyncSection.h"

namespace CloakEngine {
	namespace Engine {
		namespace Physics {
			void CLOAK_CALL Start(In size_t threadID);
			void CLOAK_CALL Update(In size_t threadID, In API::Global::Time etime);
			void CLOAK_CALL Release(In size_t threadID);
			API::Helper::Lock CLOAK_CALL LockPositionUpdate();
		}
	}
}

#endif