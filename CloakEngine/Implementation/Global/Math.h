#pragma once
#ifndef CE_IMPL_GLOBAL_MATH_H
#define CE_IMPL_GLOBAL_MATH_H

#include "CloakEngine/Defines.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Math {
				void CLOAK_CALL Initialize(In bool sse41, In bool avx);
			}
		}
	}
}

#endif