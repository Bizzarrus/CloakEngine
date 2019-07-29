#include "stdafx.h"
#include "CloakEngine/Helper/Logic.h"

namespace CloakEngine {
	namespace API {
		namespace Helper {
			namespace Logic {
				CLOAKENGINE_API unsigned long long CLOAK_CALL passGate(In Gate gate, In unsigned long long a, In unsigned long long b)
				{
					switch (gate)
					{
						case CloakEngine::API::Helper::Logic::Gate::AND:
							return a & b;
						case CloakEngine::API::Helper::Logic::Gate::OR:
							return a | b;
						case CloakEngine::API::Helper::Logic::Gate::XOR:
							return (a & ~b) | (~a & b);
						case CloakEngine::API::Helper::Logic::Gate::NAND:
							return ~(a & b);
						case CloakEngine::API::Helper::Logic::Gate::NOR:
							return ~(a | b);
						case CloakEngine::API::Helper::Logic::Gate::NXOR:
							return (a & b) | ~(a | b);
						default:
							break;
					}
					return a;
				}
			}
		}
	}
}