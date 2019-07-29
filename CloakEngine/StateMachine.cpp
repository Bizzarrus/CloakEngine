#include "stdafx.h"
#include "Implementation/Helper/StateMachine.h"

namespace CloakEngine {
	namespace API {
		namespace Helper {
			namespace StateMachine_v1 {
				IStateMachine* CLOAK_CALL IStateMachine::CreateStateMachine(In StateID id) { return new Impl::Helper::StateMachine_v1::StateMachine(nullptr, id, false); }
			}
		}
	}
	namespace Impl {
		namespace Helper {
			namespace StateMachine_v1 {
				CLOAK_CALL BasicStateEvent::BasicStateEvent(In API::Helper::StateMachine_v1::EventID id) : m_ID(id) {}
				CLOAK_CALL BasicStateEvent::~BasicStateEvent() {}
				API::Helper::StateMachine_v1::EventID CLOAK_CALL_THIS BasicStateEvent::GetID() const { return m_ID; }
				bool CLOAK_CALL_THIS BasicStateEvent::GetAs(In REFIID riid, Outptr void** ppvObject) const
				{
					if (riid == __uuidof(API::Helper::StateMachine_v1::IStateEvent)) { *ppvObject = (API::Helper::StateMachine_v1::IStateEvent*)this; return true; }
					else if (riid == __uuidof(BasicStateEvent)) { *ppvObject = (BasicStateEvent*)this; return true; }
					*ppvObject = nullptr;
					return false;
				}

				CLOAK_CALL State::State(In API::Helper::StateMachine_v1::IStateMachine* par, In API::Helper::StateMachine_v1::StateID id) : m_par(par), m_ID(id) 
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
				}
				CLOAK_CALL State::~State() 
				{
					SAVE_RELEASE(m_sync);
				}
				API::Helper::StateMachine_v1::StateID CLOAK_CALL_THIS State::GetID() const { return m_ID; }
				void CLOAK_CALL_THIS State::SetOnEvent(In API::Helper::StateMachine_v1::StateCallback callback) 
				{
					API::Helper::Lock lock(m_sync);
					m_callback = callback; 
				}
				API::Helper::StateMachine_v1::IStateMachine* CLOAK_CALL_THIS State::GetParent() const { return m_par; }
				API::Helper::StateMachine_v1::StateTransitionInfo CLOAK_CALL_THIS State::SendEvent(In API::Helper::StateMachine_v1::IStateEvent* ev)
				{
					API::Helper::Lock lock(m_sync);
					if (m_callback) { return m_callback(ev, this); }
					lock.unlock();
					API::Helper::StateMachine_v1::StateTransitionInfo sti;
					sti.DoTransition = false;
					sti.Next = nullptr;
					return sti;
				}
				void CLOAK_CALL_THIS State::OnLeave()
				{
					BasicStateEvent bse(API::Helper::StateMachine_v1::EIDOnLeaveState);
					m_par->TriggerEvent(&bse);
				}
				void CLOAK_CALL_THIS State::OnEnter()
				{
					BasicStateEvent bse(API::Helper::StateMachine_v1::EIDOnEnterState);
					m_par->TriggerEvent(&bse);
				}

				Success(return == true) bool CLOAK_CALL_THIS State::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SavePtr, Helper::StateMachine_v1, State);
				}

				CLOAK_CALL StateMachine::StateMachine(In API::Helper::StateMachine_v1::IStateMachine* par, In API::Helper::StateMachine_v1::StateID id, In bool keepHistory) : State(par,id), m_keepH(keepHistory)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					m_curState = nullptr;
					m_beginState = nullptr;
				}
				CLOAK_CALL StateMachine::~StateMachine()
				{
					SAVE_RELEASE(m_sync);
					SAVE_RELEASE(m_curState);
					SAVE_RELEASE(m_beginState);
					for (size_t a = 0; a < m_states.size(); a++) { SAVE_RELEASE(m_states[a]); }
				}
				API::Helper::StateMachine_v1::IState* CLOAK_CALL_THIS StateMachine::GetCurrentState() const
				{
					API::Helper::Lock lock(m_sync);
					return m_curState;
				}
				API::Helper::StateMachine_v1::IState* CLOAK_CALL_THIS StateMachine::CreateState(In API::Helper::StateMachine_v1::StateID id)
				{
					API::Helper::Lock lock(m_sync);
					State* s = new State(this, id);
					m_states.push_back(s);
					s->AddRef();
					return s;
				}
				API::Helper::StateMachine_v1::IStateMachine* CLOAK_CALL_THIS StateMachine::CreateSubMachine(In API::Helper::StateMachine_v1::IState* beginState, In API::Helper::StateMachine_v1::StateID id, In bool keepHistory)
				{
					API::Helper::Lock lock(m_sync);
					StateMachine* s = new StateMachine(this, id, keepHistory);
					m_states.push_back(s);
					s->AddRef();
					return s;
				}
				void CLOAK_CALL_THIS StateMachine::TriggerEvent(In API::Helper::StateMachine_v1::IStateEvent* ev)
				{
					API::Helper::Lock lock(m_sync);
					if (m_curState != nullptr) 
					{ 
						API::Helper::StateMachine_v1::StateTransitionInfo sti = m_curState->SendEvent(ev);
						const API::Helper::StateMachine_v1::EventID eid = ev->GetID();
						if (sti.DoTransition && CloakCheckOK(eid != API::Helper::EIDOnEnterState, API::Global::Debug::Error::STATE_TRANSITION_ENTER, false) && CloakCheckOK(eid != API::Helper::EIDOnLeaveState, API::Global::Debug::Error::STATE_TRANSITION_LEAVE, false))
						{
							State* s = nullptr;
							if (CloakCheckOK(sti.Next != nullptr && sti.Next->GetParent() == this && SUCCEEDED(sti.Next->QueryInterface(CE_QUERY_ARGS(&s))), API::Global::Debug::Error::STATE_TRANSITION_ILLEGAL, false)) { iTransitionState(s); }
							SAVE_RELEASE(s);
						}
					}
				}
				void CLOAK_CALL_THIS StateMachine::SetBeginState(In API::Helper::StateMachine_v1::IState* state)
				{
					API::Helper::Lock lock(m_sync);
					if (CloakCheckOK(m_beginState == nullptr, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false))
					{
						State* s = nullptr;
						HRESULT hRet = state->QueryInterface(CE_QUERY_ARGS(&s));
						if (CloakDebugCheckOK(state != nullptr && SUCCEEDED(hRet) && s->GetParent() == this, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							m_beginState = s;
							m_beginState->AddRef();
							iTransitionState(s);
						}
						SAVE_RELEASE(s);
					}
				}
				API::Helper::StateMachine_v1::StateTransitionInfo CLOAK_CALL_THIS StateMachine::SendEvent(In API::Helper::StateMachine_v1::IStateEvent* ev)
				{
					TriggerEvent(ev);
					return State::SendEvent(ev);
				}
				void CLOAK_CALL_THIS StateMachine::OnLeave()
				{
					State::OnLeave();
					if (m_keepH == false) { iTransitionState(m_beginState); }
				}

				Success(return == true) bool CLOAK_CALL_THIS StateMachine::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, State, Helper::StateMachine_v1, StateMachine);
				}
				void CLOAK_CALL_THIS StateMachine::iTransitionState(In State* next)
				{
					API::Helper::Lock lock(m_sync);
					if (next != m_curState)
					{
						if (m_curState != nullptr)
						{
							m_curState->OnLeave();
							m_curState->Release();
						}
						m_curState = next;
						if (m_curState != nullptr)
						{
							m_curState->AddRef();
							m_curState->OnEnter();
						}
					}
				}
			}
		}
	}
}