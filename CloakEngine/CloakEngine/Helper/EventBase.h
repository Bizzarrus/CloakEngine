#pragma once
#ifndef CE_API_HELPER_EVENTBASE_H
#define CE_API_HELPER_EVENTBASE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Debug.h"

#include <functional>

#define CLOAK_FUNC_TO_EVENT(func,ev) [=](const ev& e){func(e);}

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Helper {
			inline namespace EventBase_v1 {
				class CLOAKENGINE_API IEventHandlerBase {
					private:
						template<typename A> struct traits : public traits<decltype(&A::operator())> {};
						template<typename A, typename B> struct traits<A(const B&)> { typedef B type; };
						template<typename A, typename B, typename C> struct traits<B(A::*)(const C&)const> : public traits<B(const C&)> {};
						template<typename A, typename B> struct traits<A(*)(const B&)> : public traits<A(const B&)> {};
						template<typename A> struct traits<A&> : public traits<A> {};
						template<typename A> struct traits<A&&> : public traits<A> {};
					public:
						template<class Event> void CLOAK_CALL_THIS triggerEvent(In const Event& ev, In_opt bool throwError = true)
						{
							typedef IEventHandler<Event> Handler;
							Handler* t = dynamic_cast<Handler*>(this);
							if (t == nullptr && throwError == true) { CloakCheckOK(false, API::Global::Debug::Error::EVENT_NOT_SUPPORTED, false); }
							if (t != nullptr) { t->_tE(ev); }
						}
						template<typename Listener> void CLOAK_CALL_THIS registerListener(Inout Listener&& list)
						{
							typedef typename traits<Listener>::type Event;
							typedef IEventHandler<Event> Handler;
							Handler* t = dynamic_cast<Handler*>(this);
							if (CloakCheckOK(t != nullptr, API::Global::Debug::Error::EVENT_NOT_SUPPORTED, false)) { t->_rL(list); }
						}
						virtual CLOAK_CALL ~IEventHandlerBase() {}
					protected:
						CLOAK_CALL IEventHandlerBase() {}
				};
				template<class Event> CLOAK_INTERFACE IEventHandler : public virtual IEventHandlerBase {
					friend class IEventHandlerBase;
					protected:
						typedef typename std::function<void(const Event&)> Listener;
						virtual void CLOAK_CALL_THIS _rL(Inout Listener&& list) = 0;
						virtual void CLOAK_CALL_THIS _tE(In const Event& ev) = 0;
				};
				template<class... Events> class CLOAKENGINE_API IEventCollector : public virtual IEventHandler<Events>... {

				};
			}
		}
	}
}

#endif