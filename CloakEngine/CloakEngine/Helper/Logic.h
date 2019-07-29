#pragma once
#ifndef CE_API_HELPER_LOGIC_H
#define CE_API_HELPER_LOGIC_H

#include "CloakEngine/Defines.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Helper {
			namespace Logic {
				enum class Gate {
					AND,
					OR,
					XOR,
					NAND,
					NOR,
					NXOR,
				};
				enum class Compare {
					EQUAL = 0x01,
					SMALLER = 0x02,
					BIGGER = 0x04,
					SMALLER_EQUAL = 0x03,
					BIGGER_EQUAL = 0x05,
				};
				CLOAKENGINE_API unsigned long long CLOAK_CALL passGate(In Gate gate, In unsigned long long a, In unsigned long long b);
			}
		}
	}
}

#endif
