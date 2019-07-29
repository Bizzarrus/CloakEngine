#pragma once
#ifndef CE_E_GRAPHIC_GRAPHICLOCK_H
#define CE_E_GRAPHIC_GRAPHICLOCK_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Graphic.h"

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			namespace Lock {
				enum class SettingsFlag { None = 0, Shader = 1 };
				DEFINE_ENUM_FLAG_OPERATORS(SettingsFlag);
				bool CLOAK_CALL CheckSettingsUpdateMaster();
				void CLOAK_CALL UpdateSettings(In const API::Global::Graphic::Settings& oldSet, In_opt SettingsFlag flag = SettingsFlag::None);
				void CLOAK_CALL RegisterResizeBuffers(In UINT width, In UINT height);
			}
		}
	}
}

#endif
