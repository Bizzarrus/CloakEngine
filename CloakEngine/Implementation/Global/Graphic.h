#pragma once
#ifndef CE_IMPL_GLOBAL_GRAPHIC_H
#define CE_IMPL_GLOBAL_GRAPHIC_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Graphic.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Graphic {
				void CLOAK_CALL Initialize();
				void CLOAK_CALL Release();
				void CLOAK_CALL GetModifedSettings(Out API::Global::Graphic::Settings* gset);
			}
		}
	}
}

#endif