#include "stdafx.h"
#include "CloakEngine/Rendering/Defines.h"
#include "CloakEngine/Global/Debug.h"
#include "Implementation/Rendering/Deserialization.h"

namespace CloakEngine {
	namespace API {
		namespace Rendering {
			CLOAK_CALL DWParam::DWParam() : UIntVal(0) {}
			CLOAK_CALL DWParam::DWParam(In FLOAT f) : FloatVal(f) {}
			CLOAK_CALL DWParam::DWParam(In UINT f) : UIntVal(f) {}
			CLOAK_CALL DWParam::DWParam(In INT f) : IntVal(f) {}

			void CLOAK_CALL_THIS DWParam::operator=(In FLOAT f) { FloatVal = f; }
			void CLOAK_CALL_THIS DWParam::operator=(In UINT f) { UIntVal = f; }
			void CLOAK_CALL_THIS DWParam::operator=(In INT f) { IntVal = f; }
		}
	}
}