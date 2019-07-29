#pragma once
#ifndef CE_API_HELPER_STATEMACHINE_H
#define CE_API_HELPER_STATEMACHINE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"

#include <functional>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Helper {
			inline namespace StateMachine_v1 {
				CLOAK_INTERFACE IState;
				CLOAK_INTERFACE IStateMachine;
				CLOAK_INTERFACE IStateEvent;
				struct StateTransitionInfo {
					bool DoTransition = false;
					IState* Next = nullptr;
				};

				typedef unsigned int EventID;
				typedef unsigned int StateID;
				typedef std::function<StateTransitionInfo(IStateEvent*, IState*)> StateCallback;

				constexpr EventID EIDOnEnterState = 1;
				constexpr EventID EIDOnLeaveState = 2;

				CLOAK_INTERFACE_ID("{5B8FA43B-F8C8-4359-B9A6-76BE1D86FEEF}") IStateEvent {
					public:
						virtual EventID CLOAK_CALL_THIS GetID() const = 0;
						virtual bool CLOAK_CALL_THIS GetAs(In REFIID riid, Outptr void** ppvObject) const = 0;
				};
				CLOAK_INTERFACE_ID("{368F6D54-3487-4B92-9170-0ACC441958EC}") IState : public virtual Helper::SavePtr_v1::ISavePtr{
					public:
						virtual StateID CLOAK_CALL_THIS GetID() const = 0;
						virtual void CLOAK_CALL_THIS SetOnEvent(In StateCallback callback) = 0;
						virtual IStateMachine* CLOAK_CALL_THIS GetParent() const = 0;
				};
				CLOAK_INTERFACE_ID("{3F4866BF-633B-47E5-9565-232DB56577BC}") IStateMachine : public virtual IState{
					public:
						static IStateMachine* CLOAK_CALL CreateStateMachine(In StateID id);
						virtual void CLOAK_CALL_THIS TriggerEvent(In IStateEvent* ev) = 0;
						virtual void CLOAK_CALL_THIS SetBeginState(In IState* state) = 0;
						virtual IState* CLOAK_CALL_THIS GetCurrentState() const = 0;
						virtual IState* CLOAK_CALL_THIS CreateState(In StateID id) = 0;
						virtual IStateMachine* CLOAK_CALL_THIS CreateSubMachine(In IState* beginState, In StateID id, In bool keepHistory) = 0;
				};
			}
		}
	}
}

#endif
