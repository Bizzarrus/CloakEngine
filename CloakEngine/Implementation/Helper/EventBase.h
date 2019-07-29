#pragma once
#ifndef CE_IMPL_HELPER_EVENTBASE_H
#define CE_IMPL_HELPER_EVENTBASE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/EventBase.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Task.h"

#include <atomic>

#define IMPL_EVENT(ev) public Impl::Helper::EventHandler<API::Global::Events::ev>
#define INIT_EVENT(ev, sync) static_cast<EventHandler<API::Global::Events::ev>&>(*this).initializeEvent(sync)

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Helper {
			inline namespace EventBase_v1 {
				class EventHandlerTask {
					protected:
						API::Global::Task m_lastTask;
						std::atomic<bool> m_init;
						API::Helper::ISyncSection* m_syncEvent = nullptr;
					public:
						CLOAK_CALL EventHandlerTask() { m_init = false; }
						virtual CLOAK_CALL ~EventHandlerTask() { SAVE_RELEASE(m_syncEvent); }
				};
				template<class Event> class EventHandler : public virtual API::Helper::IEventHandler<Event>, public virtual EventHandlerTask {
					private:
						API::List<Listener> m_listeners;
					public:
						CLOAK_CALL EventHandler() { }
						virtual CLOAK_CALL ~EventHandler() { }
						virtual void CLOAK_CALL_THIS _rL(Inout Listener&& list) override
						{
							if (list && m_init)
							{
								API::Helper::Lock lock(m_syncEvent);
								m_listeners.push_back(list);
							}
						}
						virtual void CLOAK_CALL_THIS _tE(In const Event& ev) override
						{
							if (m_init)
							{
								API::Helper::Lock lock(m_syncEvent);
								for (size_t a = 0; a < m_listeners.size(); a++)
								{
									Listener l = m_listeners[a];
									API::Global::Task nt = [=](In size_t id) {l(ev); };
									nt.AddDependency(m_lastTask);
									nt.Schedule();
									m_lastTask = nt;
								}
							}
						}
						virtual void CLOAK_CALL_THIS initializeEvent(In API::Helper::ISyncSection* sync)
						{
							if (m_init.exchange(true) == false)
							{
								m_syncEvent = sync;
								if (m_syncEvent != nullptr) { m_syncEvent->AddRef(); }
							}
						}
				};
			}
		}
	}
}

#pragma warning(pop)

#endif
