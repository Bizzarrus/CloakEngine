#pragma once
#ifndef CE_IMPL_HELPER_STATEMACHINE_H
#define CE_IMPL_HELPER_STATEMACHINE_H

#include "CloakEngine/Helper/StateMachine.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/Helper/SavePtr.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Helper {
			inline namespace StateMachine_v1 {
				class CLOAK_UUID("{AC71B9A1-A336-470E-B699-4197D6D2E44E}") BasicStateEvent : public API::Helper::StateMachine_v1::IStateEvent {
					public:
						CLOAK_CALL BasicStateEvent(In API::Helper::StateMachine_v1::EventID id);
						virtual CLOAK_CALL ~BasicStateEvent();
						virtual API::Helper::StateMachine_v1::EventID CLOAK_CALL_THIS GetID() const override;
						virtual bool CLOAK_CALL_THIS GetAs(In REFIID riid, Outptr void** ppvObject) const override;
					protected:
						const API::Helper::StateMachine_v1::EventID m_ID;
				};
				class CLOAK_UUID("{DC064F52-3CF2-4C87-8C3B-D0611E809B47}") State : public virtual API::Helper::StateMachine_v1::IState, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL State(In API::Helper::StateMachine_v1::IStateMachine* par, In API::Helper::StateMachine_v1::StateID id);
						virtual CLOAK_CALL ~State();
						virtual API::Helper::StateMachine_v1::StateID CLOAK_CALL_THIS GetID() const override;
						virtual void CLOAK_CALL_THIS SetOnEvent(In API::Helper::StateMachine_v1::StateCallback callback) override;
						virtual API::Helper::StateMachine_v1::IStateMachine* CLOAK_CALL_THIS GetParent() const override;
						virtual API::Helper::StateMachine_v1::StateTransitionInfo CLOAK_CALL_THIS SendEvent(In API::Helper::StateMachine_v1::IStateEvent* ev);
						virtual void CLOAK_CALL_THIS OnLeave();
						virtual void CLOAK_CALL_THIS OnEnter();
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						API::Helper::ISyncSection* m_sync;
						API::Helper::StateMachine_v1::StateCallback m_callback;
						API::Helper::StateMachine_v1::StateID const m_ID;
						API::Helper::StateMachine_v1::IStateMachine* const m_par;
				};
				class CLOAK_UUID("{FB3ECD60-389F-4794-8EB2-00A20818BBC9}") StateMachine : public virtual API::Helper::StateMachine_v1::IStateMachine, public State {
					public:
						CLOAK_CALL StateMachine(In API::Helper::StateMachine_v1::IStateMachine* par, In API::Helper::StateMachine_v1::StateID id, In bool keepHistory);
						virtual CLOAK_CALL ~StateMachine();
						virtual API::Helper::StateMachine_v1::IState* CLOAK_CALL_THIS GetCurrentState() const override;
						virtual API::Helper::StateMachine_v1::IState* CLOAK_CALL_THIS CreateState(In API::Helper::StateMachine_v1::StateID id) override;
						virtual API::Helper::StateMachine_v1::IStateMachine* CLOAK_CALL_THIS CreateSubMachine(In API::Helper::StateMachine_v1::IState* beginState, In API::Helper::StateMachine_v1::StateID id, In bool keepHistory) override;
						virtual void CLOAK_CALL_THIS TriggerEvent(In API::Helper::StateMachine_v1::IStateEvent* ev) override;
						virtual void CLOAK_CALL_THIS SetBeginState(In API::Helper::StateMachine_v1::IState* state) override;
						virtual API::Helper::StateMachine_v1::StateTransitionInfo CLOAK_CALL_THIS SendEvent(In API::Helper::StateMachine_v1::IStateEvent* ev) override;
						virtual void CLOAK_CALL_THIS OnLeave() override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						void CLOAK_CALL_THIS iTransitionState(In State* next);
						API::Helper::ISyncSection* m_sync;
						API::List<State*> m_states;
						State* m_curState;
						State* m_beginState;
						const bool m_keepH;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif