#include "stdafx.h"
#include "Engine/FileLoader.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Memory.h"
#include "CloakEngine/Global/Task.h"

#include "Implementation/Global/Game.h"
#include "Implementation/Global/Task.h"

#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace FileLoader {
			constexpr size_t PRIORITY_COUNT = 4;
			//Background-loading priority is hidden, as it is controled by the system automaticly
			//	Ressources with the priority BACKGROUND will be delayed-loaded if they are not yet fully loaded and there's nothing other to load
			constexpr API::Files::Priority PRIORITY_BACKGROUND = static_cast<API::Files::Priority>(0);
			//Maxmimum of time to spend pushing things to the loading pool
			constexpr API::Global::Time MAX_UPDATE_TIME = 50;

			std::atomic<unsigned int> g_lastUpdatedPriority = 0;

			inline void CLOAK_CALL IncreaseLoadCount(In unsigned int p);
			inline void CLOAK_CALL IncreaseLoadCount(In API::Files::Priority p);
			inline void CLOAK_CALL DecreaseLoadCount(In unsigned int p);
			inline void CLOAK_CALL DecreaseLoadCount(In API::Files::Priority p);

			API::Helper::ISyncSection* g_sync[PRIORITY_COUNT] = { nullptr,nullptr,nullptr,nullptr };
			API::Helper::ISyncSection* g_syncSet = nullptr;
			LinkedList<ILoad*>* g_loads;
			LinkedList<loadInfo>* g_toLoad[PRIORITY_COUNT];
			std::atomic<int64_t> g_loadCount[PRIORITY_COUNT] = { 0,0,0,0 };
			std::atomic<bool> g_checkedLC = false;
			std::atomic<bool> g_isLoading[PRIORITY_COUNT] = { false,false,false,false };
			std::atomic<uint64_t> g_remainLoad = 0;

			inline void CLOAK_CALL loadFindJob()
			{
				const uint64_t opCount = g_remainLoad.exchange(0);
				CloakDebugLog("Loading " + std::to_string(opCount) + " objects");
				if (opCount > 0)
				{
					uint64_t p = 0;
					for (size_t a = PRIORITY_COUNT; a > 0; a--)
					{
						API::Helper::Lock lock(g_sync[a - 1]);
						if (g_toLoad[a - 1]->size() > 0)
						{
							for (auto b = g_toLoad[a - 1]->begin(); b != g_toLoad[a - 1]->end();)
							{
								unsigned int lup = g_lastUpdatedPriority.exchange(0);
								if (lup >= a) { a = lup + 1; break; }
								loadInfo i = *b;
								i.obj->setWaitForLoad();
								i.obj->onStartLoadAction();
								i.obj->Release();
								g_toLoad[a - 1]->remove(b++);
								if (++p == opCount) { goto foundAll; }
							}
						}
					}
				foundAll:;
					CLOAK_ASSUME(p == opCount);
				}
			}
			inline void CLOAK_CALL IncreaseLoadCount(In unsigned int p)
			{
				if (g_loadCount[p].fetch_add(1) == 0) { g_checkedLC = false; }
			}
			inline void CLOAK_CALL IncreaseLoadCount(In API::Files::Priority p) { IncreaseLoadCount(static_cast<unsigned int>(p)); }
			inline void CLOAK_CALL DecreaseLoadCount(In unsigned int p)
			{
				if (g_loadCount[p].fetch_sub(1) == 1) { Impl::Global::Game::wakeThreads(); g_checkedLC = false; }
			}
			inline void CLOAK_CALL DecreaseLoadCount(In API::Files::Priority p) { DecreaseLoadCount(static_cast<unsigned int>(p)); }
			inline void CLOAK_CALL CheckLoadCount()
			{
				if (g_checkedLC.exchange(true) == false)
				{
					bool f = false;
					for (size_t a = PRIORITY_COUNT; a > 0; a--)
					{
						f = f || g_loadCount[a - 1] != 0;
						g_isLoading[a - 1] = f;
					}
				}
			}
			inline void CLOAK_CALL pushInLoadQueue(In ILoad* t, In API::Files::Priority prio)
			{
				if (t != nullptr && Impl::Global::Game::canThreadRun())
				{
					API::Helper::Lock lock(g_sync[static_cast<unsigned int>(prio)]);
					loadInfo a;
					a.obj = t;
					t->AddRef();
					IncreaseLoadCount(prio);
					t->setPriority(prio, g_toLoad[static_cast<unsigned int>(prio)]->push(a));
					if (g_remainLoad.fetch_add(1) == 0) { API::Global::PushTask([](In size_t id) {loadFindJob(); }); }
					unsigned int lp = g_lastUpdatedPriority;
					while (lp < static_cast<unsigned int>(prio) && !g_lastUpdatedPriority.compare_exchange_strong(lp, static_cast<unsigned int>(prio))) {}
				}
			}

			CLOAK_CALL_THIS ILoad::ILoad() : m_loadPtr(nullptr) 
			{ 
				m_loaded = false; 
				m_loading = false; 
				m_waitForLoad = false;
				m_priority = PRIORITY_BACKGROUND;
				m_loadAction = LoadType::NONE;
				API::Helper::Lock lock(g_syncSet);
				m_ptr = g_loads->push(this);
			}
			CLOAK_CALL_THIS ILoad::~ILoad() 
			{
				API::Helper::Lock lock(g_syncSet);
				g_loads->pop(m_ptr);
			}
			bool CLOAK_CALL_THIS ILoad::onLoad()
			{
				if (m_loaded == false)
				{
					CloakDebugLog("load: " + GetDebugName());
					bool r = false;
					m_loaded = iLoadFile(&r);
					CloakDebugLog("finished loading: " + GetDebugName());
					return r;
				}
				return false;
			}
			void CLOAK_CALL_THIS ILoad::onUnload()
			{
				if (m_loaded == true)
				{
					CloakDebugLog("unload: " + GetDebugName());
					iUnloadFile();
					CloakDebugLog("finished unloading: " + GetDebugName());
					m_loaded = false;
				}
			}
			bool CLOAK_CALL_THIS ILoad::onDelayedLoad()
			{
				if (m_loaded == false) { onLoad(); }
				if (m_loaded == true)
				{
					CloakDebugLog("delayed loaded: " + GetDebugName());
					bool r = false;
					iDelayedLoadFile(&r);
					CloakDebugLog("finished delayed loading: " + GetDebugName());
					return r;
				}
				return false;
			}
			void CLOAK_CALL_THIS ILoad::onStartLoadAction(In_opt bool keepLoading)
			{
				m_loading = true;
				m_loadPtr = nullptr;
				LoadType t = m_loadAction.exchange(LoadType::NONE);
				API::Files::Priority p = m_priority.exchange(PRIORITY_BACKGROUND);
				if (t != LoadType::NONE)
				{
					AddRef();
					API::Global::Threading::ScheduleHint hint = t == LoadType::UNLOAD || t == LoadType::RELOAD ? API::Global::Threading::ScheduleHint::None : API::Global::Threading::ScheduleHint::IO;
					API::Global::Threading::Priority priority = API::Global::Threading::Priority::Normal;
					switch (p)
					{
						case PRIORITY_BACKGROUND: priority = API::Global::Threading::Priority::Low; break;
						case API::Files::Priority::HIGH: priority = API::Global::Threading::Priority::High; break;
						default: break;
					}
					API::Global::PushTask([this, t](In size_t id) { this->onProcessLoadAction(t); }).Schedule(hint, priority);
					return;
				}
				m_waitForLoad = false;
				DecreaseLoadCount(p);
				m_loading = false;
				if (keepLoading == true) { iRequestDelayedLoad(PRIORITY_BACKGROUND); }
				m_isLoaded = m_loaded.load() == true;
			}
			void CLOAK_CALL_THIS ILoad::onProcessLoadAction(In LoadType loadType)
			{
				bool keepLoading = false;
				switch (loadType)
				{
					case CloakEngine::Engine::FileLoader::LoadType::LOAD:
						keepLoading = onLoad();
						break;
					case CloakEngine::Engine::FileLoader::LoadType::UNLOAD:
						onUnload();
						break;
					case CloakEngine::Engine::FileLoader::LoadType::RELOAD:
						onUnload();
						API::Global::PushTask([this](In size_t id) { this->onProcessLoadAction(LoadType::LOAD); }).Schedule(API::Global::Threading::ScheduleHint::IO, API::Global::Threading::Priority::High);
						return;
					case CloakEngine::Engine::FileLoader::LoadType::DELAYED:
						keepLoading = onDelayedLoad();
						break;
					default:
						CLOAK_ASSUME(false);
						break;
				}
				onStartLoadAction(keepLoading);
				Release();
			}
			void CLOAK_CALL_THIS ILoad::setPriority(In API::Files::Priority p, In Engine::LinkedList<loadInfo>::const_iterator ptr)
			{
				API::Helper::Lock lock(g_sync[static_cast<unsigned int>(p)]);
				m_loadPtr = ptr;
				m_priority = p;
			}
			void CLOAK_CALL_THIS ILoad::setWaitForLoad()
			{
				m_waitForLoad = true;
				m_loadPtr = nullptr;
			}
			void CLOAK_CALL_THIS ILoad::updateSetting(In const API::Global::Graphic::Settings& nset)
			{
				if (m_loaded && iCheckSetting(nset)) { iRequestReload(API::Files::Priority::NORMAL); }
			}
			bool CLOAK_CALL_THIS ILoad::iCheckSetting(In const API::Global::Graphic::Settings& nset) const
			{
				return false;
			}
			void CLOAK_CALL_THIS ILoad::iRequestLoad(In_opt API::Files::Priority prio)
			{
				CloakDebugLog("Request load: " + GetDebugName());
				LoadType exp = LoadType::NONE;
				if (m_loadAction.compare_exchange_strong(exp, LoadType::LOAD) == true && m_loading == false)
				{
					pushInLoadQueue(this, prio);
				}
			}
			void CLOAK_CALL_THIS ILoad::iRequestUnload(In_opt API::Files::Priority prio)
			{
				CloakDebugLog("Request unload: " + GetDebugName());
				if (m_loadAction.exchange(LoadType::UNLOAD) == LoadType::NONE && m_loading == false)
				{
					pushInLoadQueue(this, prio);
				}
			}
			void CLOAK_CALL_THIS ILoad::iRequestReload(In_opt API::Files::Priority prio)
			{
				CloakDebugLog("Request reload: " + GetDebugName());
				LoadType exp = LoadType::NONE;
				if (m_loadAction.compare_exchange_strong(exp, LoadType::RELOAD) == true && m_loading == false)
				{
					pushInLoadQueue(this, prio);
				}
			}
			void CLOAK_CALL_THIS ILoad::iRequestDelayedLoad(In_opt API::Files::Priority prio)
			{
				CloakDebugLog("Request delayed loading: " + GetDebugName());
				LoadType exp = LoadType::NONE;
				if (m_loadAction.compare_exchange_strong(exp, LoadType::DELAYED) == true && m_loading == false)
				{
					pushInLoadQueue(this, prio);
				}
			}
			void CLOAK_CALL_THIS ILoad::iSetPriority(In API::Files::Priority p)
			{
				if (m_loading == false)
				{
					bool updPr = false;
					API::Files::Priority op;
					API::Files::Priority np;
					do {
						op = m_priority.load();
						np = op;
						updPr = false;
						if (static_cast<unsigned int>(p) > static_cast<unsigned int>(np))
						{
							np = p;
							updPr = true;
						}
					} while (!m_priority.compare_exchange_strong(op, np));
					if (updPr)
					{
						API::Helper::Lock lock(g_sync[static_cast<unsigned int>(op)]);
						Engine::LinkedList<loadInfo>::const_iterator e = m_loadPtr.load();
						if (e != nullptr)
						{
							loadInfo li = *e;
							g_toLoad[static_cast<unsigned int>(op)]->remove(e);
							IncreaseLoadCount(np);
							DecreaseLoadCount(op);
							lock.unlock();
							lock.lock(g_sync[static_cast<unsigned int>(np)]);
							m_loadPtr = g_toLoad[static_cast<unsigned int>(np)]->push(li);
							unsigned int lp = g_lastUpdatedPriority;
							while (lp < static_cast<unsigned int>(np) && !g_lastUpdatedPriority.compare_exchange_strong(lp, static_cast<unsigned int>(np))) {}
						}
						else if (m_waitForLoad)
						{
							IncreaseLoadCount(np);
							DecreaseLoadCount(op);
						}
						else { m_priority = API::Files::Priority::LOW; }
					}
				}
				else
				{
					API::Files::Priority op;
					API::Files::Priority np;
					do {
						op = m_priority.load();
						np = op;
						if (static_cast<unsigned int>(p) > static_cast<unsigned int>(np)) { np = p; }
					} while (!m_priority.compare_exchange_strong(op, np));
					if (np != op)
					{
						IncreaseLoadCount(np);
						DecreaseLoadCount(op);
						if (m_loading == false && np != API::Files::Priority::LOW && m_loadPtr.load() == nullptr)
						{
							DecreaseLoadCount(m_priority.exchange(API::Files::Priority::LOW));
						}
					}
				}
			}

			void CLOAK_CALL onStart()
			{
				g_loads = new Engine::LinkedList<ILoad*>();
				for (size_t a = 0; a < ARRAYSIZE(g_toLoad); a++) { g_toLoad[a] = new Engine::LinkedList<loadInfo>(); }
				for (size_t a = 0; a < ARRAYSIZE(g_sync); a++) { CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync[a])); }
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncSet));
			}
			void CLOAK_CALL onStop()
			{
				for (size_t a = 0; a < ARRAYSIZE(g_sync); a++) { SAVE_RELEASE(g_sync[a]); }
				SAVE_RELEASE(g_syncSet);
				for (size_t a = 0; a < ARRAYSIZE(g_toLoad); a++)
				{
					g_toLoad[a]->for_each([=](loadInfo& info) {SAVE_RELEASE(info.obj); });
					delete g_toLoad[a];
				}
				delete g_loads;
			}
			void CLOAK_CALL updateSetting(In const API::Global::Graphic::Settings& nset)
			{
				API::Helper::Lock lock(g_syncSet);
				g_loads->for_each([=](ILoad* l) {
					l->updateSetting(nset);
				});
			}
			bool CLOAK_CALL isLoading(In_opt API::Files::Priority prio)
			{
				CheckLoadCount();
				return g_isLoading[static_cast<unsigned int>(prio)];
			}
		}
	}
}