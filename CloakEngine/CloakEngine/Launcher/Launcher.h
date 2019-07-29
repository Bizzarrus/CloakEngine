#pragma once
#ifndef CE_API_LAUNCHER_LAUNCHER_H
#define CE_API_LAUNCHER_LAUNCHER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"

#include <string>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Launcher {
			inline namespace Launcher_v1 {
				CLOAK_INTERFACE_ID("{193C164F-C23F-468B-A96D-BCBFF7BC4936}") ILauncher : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual void CLOAK_CALL_THIS SetWidth(In float w) = 0;
						virtual void CLOAK_CALL_THIS SetHeight(In float h) = 0;
						virtual void CLOAK_CALL_THIS SetName(In const std::u16string& name) = 0;
				};
			}
		}
	}
}

#endif