#pragma once
#ifndef CE_API_RENDERING_COMMANDSIGNATURE_H
#define CE_API_RENDERING_COMMANDSIGNATURE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Rendering/PipelineState.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace CommandSignature_v1 {
				class IndirectCommandHint;
				//ICommandSignature:
				//	The Command Signature is required for indirect executing, and defines format and
				//	size of each command in the indirect command buffer. Notice that the use of 
				//	indirect executing might not be supported by the current used graphic API, in 
				//	which case the creation of command signatures will fail!
				CLOAK_INTERFACE_ID("{39FB4F75-7BD8-4B92-B1DD-95E6632EFC4D}") ICommandSignature : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual void CLOAK_CALL_THIS Resize(In size_t argCount, In IPipelineState* pso) = 0;

						virtual void CLOAK_CALL_THIS SetDraw(In size_t argID) = 0;
						virtual void CLOAK_CALL_THIS SetDrawIndexed(In size_t argID) = 0;
						virtual void CLOAK_CALL_THIS SetDispatch(In size_t argID) = 0;
						virtual void CLOAK_CALL_THIS SetVertexBuffer(In size_t argID, In_opt UINT slot = 0) = 0;
						virtual void CLOAK_CALL_THIS SetIndexBuffer(In size_t argID) = 0;
						virtual void CLOAK_CALL_THIS SetConstants(In size_t argID, In UINT rootID, In UINT numConstants, In_opt UINT firstConstantIndex = 0) = 0;
						virtual void CLOAK_CALL_THIS SetCBV(In size_t argID, In UINT rootID) = 0;
						virtual void CLOAK_CALL_THIS SetSRV(In size_t argID, In UINT rootID) = 0;
						virtual void CLOAK_CALL_THIS SetUAV(In size_t argID, In UINT rootID) = 0;
						virtual IndirectCommandHint CLOAK_CALL_THIS At(In size_t argID) = 0;
				};
				class CLOAKENGINE_API IndirectCommandHint {
					public:
						CLOAK_CALL IndirectCommandHint(In ICommandSignature* sig, In UINT argID);
						void CLOAK_CALL_THIS Draw();
						void CLOAK_CALL_THIS DrawIndexed();
						void CLOAK_CALL_THIS Dispatch();
						void CLOAK_CALL_THIS VertexBuffer(In_opt UINT slot = 0);
						void CLOAK_CALL_THIS IndexBuffer();
						void CLOAK_CALL_THIS Constants(In UINT rootID, In UINT numConstants, In_opt UINT firstConstantIndex = 0);
						void CLOAK_CALL_THIS CBV(In UINT rootID);
						void CLOAK_CALL_THIS SRV(In UINT rootID);
						void CLOAK_CALL_THIS UAV(In UINT rootID);
					private:
						ICommandSignature* const m_sig;
						const UINT m_argID;
				};
			}
		}
	}
}

#endif