#include "stdafx.h"
#include "Engine/Thread.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Log.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Helper/StringConvert.h"

#include "Implementation/Global/Debug.h"
#include "Implementation/Global/Task.h"

#include "Engine/WindowHandler.h"
#include "Engine/FileLoader.h"

#define IS_THREAD_FLAG_SET(flags, flagName) (((flags) & (flagName)) == (flagName))

namespace CloakEngine {
	namespace Engine {
		namespace Thread {
			constexpr float MICROSEC_PER_SEC = 1000000.0f;
#ifdef _DEBUG
			CLOAK_CALL Thread::Thread(In float targetFPS, In uint32_t id, In_opt bool lockFPS, In_opt std::string label) : m_id(id), m_lockFPS(lockFPS), m_label(label)
#else
			CLOAK_CALL Thread::Thread(In float targetFPS, In uint32_t id, In_opt bool lockFPS) : m_id(id), m_lockFPS(lockFPS)
#endif
			{
				m_run = true;
				m_running = false;
				m_paused = false;
				m_finished = false;
				m_inactive = false;
				m_fps = 0;
				m_lastUpdate = 0;
				m_sleepTime = targetFPS <= 0.0f ? 0.0f : (MICROSEC_PER_SEC / targetFPS);
				Impl::Global::Game::registerThread();
			}
			CLOAK_CALL Thread::~Thread() {}
			void CLOAK_CALL_THIS Thread::Stop() { m_run = false; Awake(); }
			void CLOAK_CALL_THIS Thread::SetFPS(In float fps) 
			{ 
				if (m_lockFPS == false) { m_sleepTime = fps <= 0.0f ? 0.0f : (MICROSEC_PER_SEC / fps); } 
			}

			bool CLOAK_CALL_THIS Thread::IsPaused() const { return m_paused; }
			bool CLOAK_CALL_THIS Thread::IsRunning() const { return m_run && m_running; }
			uint32_t CLOAK_CALL_THIS Thread::GetID() const { return m_id; }
			float CLOAK_CALL_THIS Thread::GetFPS() const { return m_fps; }

#ifdef _DEBUG
			CLOAK_CALL CustomThread::CustomThread(In const ThreadDesc& desc, In uint32_t id, In_opt std::string label) : Thread(desc.TargetFPS, id, IS_THREAD_FLAG_SET(desc.Flags, FLAG_LOCK_FPS), label), m_flags(desc.Flags), m_fpsFactor(desc.FPSFactor), m_important(IS_THREAD_FLAG_SET(desc.Flags, FLAG_IMPORTANT))
#else
			CLOAK_CALL CustomThread::CustomThread(In const ThreadDesc& desc, In uint32_t id) : Thread(desc.TargetFPS, id, IS_THREAD_FLAG_SET(desc.Flags, FLAG_LOCK_FPS)), m_flags(desc.Flags), m_fpsFactor(desc.FPSFactor)
#endif
			{
				for (size_t a = 0; a < START_FUNC_COUNT; a++) { m_start[a] = desc.Functions.Start[a]; }
				for (size_t a = 0; a < STOP_FUNC_COUNT; a++) { m_stop[a] = desc.Functions.Stop[a]; }
				m_update = desc.Functions.Update;
			}
			CLOAK_CALL CustomThread::~CustomThread()
			{
				runStop(0);
				if (m_stop[1]) { m_stop[1](0); }
			}
			void CLOAK_CALL_THIS CustomThread::Awake()
			{
				if (Impl::Global::Game::canThreadRun() == false || m_run == false)
				{
					if (m_inactive.exchange(false) == true)
					{
						m_curTask = API::Global::PushTask([this](In size_t threadID) { runStop(threadID); });
						m_curTask.Schedule(CE::Global::Threading::Priority::High);
					}
				}
				else if (m_update && (IS_THREAD_FLAG_SET(m_flags, FLAG_IGNORE_PAUSE) || WindowHandler::isFocused() == true) && (IS_THREAD_FLAG_SET(m_flags, FLAG_PAUSE_ON_LOADING) || FileLoader::isLoading(API::Files::Priority::NORMAL) == false))
				{
					if (m_inactive.exchange(false) == true)
					{
						const API::Global::Time ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
						Impl::Global::Game::threadFPS(ctm, &m_tfi);
						m_lastUpdate = ctm;
						Impl::Global::Game::threadSleep(ctm, m_sleepTime, &m_tsi);
#ifdef _DEBUG
						CloakDebugLog("Awaken thread " + m_label);
						if (IS_THREAD_FLAG_SET(m_flags, FLAG_NO_RESPONSE_CHECK) == false) { Impl::Global::Game::threadRespond(m_id, ctm / 1000); }
#endif
						m_curTask = API::Global::PushTask([this](In size_t threadID) { runUpdate(threadID); });
						m_curTask.Schedule(m_important == true ? CE::Global::Threading::Priority::High : CE::Global::Threading::Priority::Normal);
					}
				}
			}
			void CLOAK_CALL_THIS CustomThread::Start()
			{
				CLOAK_ASSUME(m_curTask == nullptr);
				m_curTask = API::Global::PushTask([this](In size_t threadID) { runInitialize(threadID); });
				m_curTask.Schedule(CE::Global::Threading::Priority::High);
			}

			void CLOAK_CALL_THIS CustomThread::runInitialize(In size_t threadID)
			{
				if (Impl::Global::Game::canThreadRun() && m_run == true)
				{
					m_running = true;
					const API::Global::Time ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
					const API::Global::Time ct = ctm / 1000;
#ifdef _DEBUG
					if (IS_THREAD_FLAG_SET(m_flags, FLAG_NO_RESPONSE_CHECK) == false) { Impl::Global::Game::threadRespond(m_id, ct); }
#endif
					const Impl::Global::Game::ThreadCanComRes canComRes = Impl::Global::Game::checkThreadCommunicateLevel(&m_canCom);
					if (canComRes == Impl::Global::Game::ThreadCanComRes::UPDATED)
					{
						const BYTE nCC = Impl::Global::Game::getThreadCommunicateLevel(m_canCom);
#ifdef _DEBUG
						if (nCC == START_FUNC_COUNT && IS_THREAD_FLAG_SET(m_flags, FLAG_NO_RESPONSE_CHECK)) { Impl::Global::Game::disableThreadRespondCheck(m_id); }
#endif
						if (nCC > 0 && m_start[START_FUNC_COUNT - nCC]) { m_start[START_FUNC_COUNT - nCC](threadID); }
#ifdef _DEBUG
						if (nCC == 1) {
							CloakDebugLog("Starting thread " + m_label);
						}
#endif
						Impl::Global::Game::updateCommunicateLevel(&m_canCom);
					}
					else if (canComRes == Impl::Global::Game::ThreadCanComRes::FINISHED)
					{
						m_fps = Impl::Global::Game::threadFPS(ctm, &m_tfi);
						m_lastUpdate = ctm;
						if (m_update)
						{
							m_curTask = Impl::Global::Task::Sleep(Impl::Global::Game::threadSleep(ctm, m_sleepTime / m_fpsFactor, &m_tsi), [this](In size_t threadID) { runUpdate(threadID); });
							m_curTask.Schedule(m_important == true ? CE::Global::Threading::Priority::High : CE::Global::Threading::Priority::Normal, threadID);
						}
						else
						{
							if (m_paused.exchange(true) == false) { Impl::Global::Game::updateThreadPause(true); }
							m_inactive = true;
#ifdef _DEBUG
							CloakDebugLog("Sent thread " + m_label + " to inactive state");
#endif
						}
						return;
					}
					m_curTask = API::Global::PushTask([this](In size_t threadID) { runInitialize(threadID); });
					m_curTask.Schedule(CE::Global::Threading::Priority::High, threadID);
					return;
				}
				runStop(threadID);
			}
			void CLOAK_CALL_THIS CustomThread::runUpdate(In size_t threadID)
			{
				if (Impl::Global::Game::canThreadRun() && m_run == true)
				{
					const API::Global::Time ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
					const API::Global::Time ct = ctm / 1000;
#ifdef _DEBUG
					if (IS_THREAD_FLAG_SET(m_flags, FLAG_NO_RESPONSE_CHECK) == false) { Impl::Global::Game::threadRespond(m_id, ct); }
#endif
					if ((IS_THREAD_FLAG_SET(m_flags, FLAG_IGNORE_PAUSE) || WindowHandler::isFocused() == true) && (IS_THREAD_FLAG_SET(m_flags, FLAG_PAUSE_ON_LOADING) || FileLoader::isLoading(API::Files::Priority::NORMAL) == false))
					{
						if (m_paused.exchange(false) == true) { Impl::Global::Game::updateThreadPause(false); }
						const API::Global::Time etime = (m_lastUpdate < ctm ? ctm - m_lastUpdate : 0) / 1000;
						m_update(threadID, etime);
						m_fps = Impl::Global::Game::threadFPS(ctm, &m_tfi);
						m_lastUpdate = ctm;
						m_curTask = Impl::Global::Task::Sleep(Impl::Global::Game::threadSleep(ctm, m_sleepTime / m_fpsFactor, &m_tsi), [this](In size_t threadID) { runUpdate(threadID); });
						m_curTask.Schedule(m_important == true ? CE::Global::Threading::Priority::High : CE::Global::Threading::Priority::Normal, threadID);
					}
					else
					{
						if (m_paused.exchange(true) == false) { Impl::Global::Game::updateThreadPause(true); }
						m_inactive = true;
#ifdef _DEBUG
						CloakDebugLog("Sent thread " + m_label + " to inactive state");
#endif
						Awake(); //Just call awake to be sure that being inactive is still the right move
					}
					return;
				}
				runStop(threadID);
			}
			void CLOAK_CALL_THIS CustomThread::runStop(In size_t threadID)
			{
				if (m_finished.exchange(true) == false)
				{
#ifdef _DEBUG
					CloakDebugLog("Stopping thread " + m_label);
#endif
					if (m_stop[0]) { m_stop[0](threadID); }
					Impl::Global::Game::registerThreadStop();
					m_running = false;
				}
			}

			namespace ThreadLoops {
#ifdef _DEBUG
				CLOAK_CALL RenderLoopThread::RenderLoopThread(In uint32_t id, In float targetFPS) : Thread(targetFPS, id, false, "Render Loop")
#else
				CLOAK_CALL RenderLoopThread::RenderLoopThread(In uint32_t id, In float targetFPS) : Thread(targetFPS, id)
#endif
				{
					m_lastTime = 0;
					m_curTime = 0;
				}
				CLOAK_CALL RenderLoopThread::~RenderLoopThread()
				{
					runStop(0);
				}

				void CLOAK_CALL_THIS RenderLoopThread::Awake()
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true && WindowHandler::isFocused() == true)
					{
						if (m_inactive.exchange(true) == false)
						{
							m_readInput.Schedule(CE::Global::Threading::Priority::High);
							m_updateAI.Schedule(CE::Global::Threading::Priority::High);
						}
					}
				}
				void CLOAK_CALL_THIS RenderLoopThread::Start()
				{
					CLOAK_ASSUME(m_readInput == nullptr);
					m_readInput = API::Global::PushTask([this](In size_t threadID) { runInitialize(threadID); });
					m_readInput.Schedule(CE::Global::Threading::Priority::High);
				}

				void CLOAK_CALL_THIS RenderLoopThread::runInitialize(In size_t threadID)
				{
					if (Impl::Global::Game::canThreadRun() && m_run == true)
					{
						m_running = true;
						const API::Global::Time ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
						const API::Global::Time ct = ctm / 1000;
#ifdef _DEBUG
						Impl::Global::Game::threadRespond(m_id, ct);
#endif
						const Impl::Global::Game::ThreadCanComRes canComRes = Impl::Global::Game::checkThreadCommunicateLevel(&m_canCom);
						if (canComRes == Impl::Global::Game::ThreadCanComRes::UPDATED)
						{
							//TODO: Enable initialisation calls
							const BYTE nCC = Impl::Global::Game::getThreadCommunicateLevel(m_canCom);
							switch (START_FUNC_COUNT - nCC)
							{
								case 0:
									//Engine::Graphic::Core::InitPipeline();
									break;
								case 1:
									//Impl::Global::Game::onGameInit(0);
									//Engine::Graphic::Core::Load();
									//Engine::Physics::Start();
									break;
								case 2:

									break;
								case 3:
									//Engine::Graphic::Core::Start();
									//Impl::Global::Input::onLoad();
									break;
								case 4:
									//Impl::Global::Game::onGameStart();
									break;
								case START_FUNC_COUNT:
								default:
									break;
							}
#ifdef _DEBUG
							if (nCC == 1) {
								CloakDebugLog("Starting thread " + m_label);
							}
#endif
							Impl::Global::Game::updateCommunicateLevel(&m_canCom);
						}
						else if (canComRes == Impl::Global::Game::ThreadCanComRes::FINISHED)
						{
							m_fps = Impl::Global::Game::threadFPS(ctm, &m_tfi);
							m_lastUpdate = ctm;

							m_readInput = [this](In size_t threadID) { runReadInput(threadID); };

							m_updateAI = [this](In size_t threadID) { runUpdateAI(threadID); };

							m_updateGame = [this](In size_t threadID) { runUpdateGame(threadID); };
							m_updateGame.AddDependency(m_readInput);
							m_updateGame.AddDependency(m_updateAI);

							m_processAI = [this](In size_t threadID) { runProcessAI(threadID); };
							m_processAI.AddDependency(m_updateAI);

							m_readAnimation = [this](In size_t threadID) {runReadAnimation(threadID); };
							m_readAnimation.AddDependency(m_processAI);
							m_readAnimation.AddDependency(m_updateGame);

							m_updatePosition = [this](In size_t threadID) { runUpdatePosition(threadID); };
							m_updatePosition.AddDependency(m_updateGame);
							m_updatePosition.AddDependency(m_processAI);

							m_updateRootMotion = [this](In size_t threadID) { runUpdateRootMotion(threadID); };
							m_updateRootMotion.AddDependency(m_updatePosition);
							m_updateRootMotion.AddDependency(m_readAnimation);

							m_updateVisibility = [this](In size_t threadID) { runUpdateVisibility(threadID); };
							m_updateVisibility.AddDependency(m_updateRootMotion);

							m_updateAnimation = [this](In size_t threadID) { runUpdateAnimation(threadID); };
							m_updateAnimation.AddDependency(m_updateVisibility);

							m_mirrorGameState = [this](In size_t threadID) { runMirrorGameState(threadID); };
							m_mirrorGameState.AddDependency(m_updateAnimation);

							m_renderSubmit = [this](In size_t threadID) { runRenderSubmit(threadID); };
							m_renderSubmit.AddDependency(m_mirrorGameState);

							m_renderPresent = [this](In size_t threadID) { runRenderPresent(threadID); };
							m_renderPresent.AddDependency(m_renderSubmit);

							m_readInput.Schedule(SIMULATION_IMPORTANT, threadID);
							m_updateAI.Schedule(SIMULATION_IMPORTANT, threadID);
							m_updateGame.Schedule(SIMULATION_IMPORTANT, threadID);
							m_processAI.Schedule(SIMULATION_IMPORTANT, threadID);
							m_readAnimation.Schedule(SIMULATION_IMPORTANT, threadID);
							m_updatePosition.Schedule(SIMULATION_IMPORTANT, threadID);
							m_updateRootMotion.Schedule(SIMULATION_IMPORTANT, threadID);
							m_updateVisibility.Schedule(SIMULATION_IMPORTANT, threadID);
							m_updateAnimation.Schedule(SIMULATION_IMPORTANT, threadID);
							m_mirrorGameState.Schedule(SIMULATION_IMPORTANT, threadID);
							m_renderSubmit.Schedule(SIMULATION_IMPORTANT, threadID);
							m_renderPresent.Schedule(SIMULATION_IMPORTANT, threadID);
							return;
						}
						m_readInput = API::Global::PushTask([this](In size_t threadID) { runInitialize(threadID); });
						m_readInput.Schedule(CE::Global::Threading::Priority::High, threadID);
						return;
					}
					runStop(0);
				}
				void CLOAK_CALL_THIS RenderLoopThread::runStop(In size_t threadID)
				{
					if (m_finished.exchange(true) == false)
					{
#ifdef _DEBUG
						CloakDebugLog("Stopping thread " + m_label);
#endif
						//TODO: Enable stop calls

						//Impl::Global::Game::onGameStop();
						//Impl::Global::Input::onStop();
						//Engine::Physics::Release();

						m_running = false;
						m_readInput = nullptr;
						m_updateAI = nullptr;

						Impl::Global::Game::registerThreadStop();
					}
				}
				void CLOAK_CALL_THIS RenderLoopThread::runReadInput(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO
						//Impl::Global::Input::ReadQueue();
						m_readInput = [this](In size_t threadID) { runReadInput(threadID); };
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runUpdateAI(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO
						//AI::UpdateDecissions();
						m_updateAI = [this](In size_t threadID) { runUpdateAI(threadID); };
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runUpdateGame(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						m_lastTime = m_curTime;
						m_curTime = API::Global::Game::GetGameTimeStamp();
						const API::Global::Time etime = (m_curTime - m_lastTime) / 1000;
						//TODO
						//Impl::Global::Input::Update();
						//Impl::Global::Game::Update(etime);
						m_updateGame = [this](In size_t threadID) { runUpdateGame(threadID); };
						m_updateGame.AddDependency(m_readInput);
						m_updateGame.AddDependency(m_updateAI);
						m_updateGame.Schedule(SIMULATION_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runProcessAI(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO
						//API::World::Group<Impl::Components::AI, Impl::Components::Position> ai;
						//for (const auto& a : ai)
						//{
						//	Impl::Components::AI* i = a.Get<Impl::Components::AI>();
						//	Impl::Components::Position* p = a.Get<Impl::Components::Position>();
						//	API::Global::Task x = [i, p](In size_t threadID) {i->Update(p); i->Release(); p->Release(); };
						//	m_readAnimation.AddDependency(x);
						//	m_updatePosition.AddDependency(x);
						//	x.Schedule(SIMULATION_IMPORTANT, threadID);
						//}
						m_processAI = [this](In size_t threadID) { runProcessAI(threadID); };
						m_processAI.AddDependency(m_updateAI);
						m_processAI.Schedule(SIMULATION_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runUpdatePosition(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO
						//API::World::Group<Impl::Components::Position> group;
						//for (const auto& a : group)
						//{
						//	Impl::Components::Position* p = a.Get<Impl::Components::Position>();
						//	API::Global::Task x = [p](In size_t threadID) { p->Update(); p->UpdateWorldPosition(); p->Release(); };
						//	m_updateRootMotion.AddDependency(x);
						//	x.Schedule(SIMULATION_IMPORTANT, threadID);
						//}

						m_updatePosition = [this](In size_t threadID) { runUpdatePosition(threadID); };
						m_updatePosition.AddDependency(m_updateGame);
						m_updatePosition.AddDependency(m_processAI);
						//Next position update should wait for last (current) mirroring, as mirroring might still access data
						m_updatePosition.AddDependency(m_mirrorGameState);
						m_updatePosition.Schedule(SIMULATION_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runUpdateAnimation(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO
						//API::World::Group<Impl::Components::Animation> group;
						//for (const auto& a : group)
						//{
						//	Impl::Components::Animation* p = a.Get<Impl::Components::Animation>();
						//	API::Global::Task x = [p](In size_t threadID) { p->Update(); };
						//	m_updateGame.AddDependency(x);
						//	m_updateAI.AddDependency(x);
						//	m_mirrorGameState.AddDependency(x);
						//	x.Schedule(RENDERING_IMPORTANT, threadID);
						//}

						m_updateAnimation = [this](In size_t threadID) { runUpdateAnimation(threadID); };
						m_updateAnimation.AddDependency(m_updateVisibility);
						m_updateAnimation.Schedule(RENDERING_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runReadAnimation(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO
						//API::World::Group<Impl::Components::Animation> group;
						//for (const auto& a : group)
						//{
						//	Impl::Components::Animation* p = a.Get<Impl::Components::Animation>();
						//	API::Global::Task x = [p](In size_t threadID) { p->UpdateState(m_curTime); p->Release(); };
						//	m_updateRootMotion.AddDependency(x);
						//	x.Schedule(SIMULATION_IMPORTANT, threadID);
						//}

						m_readAnimation = [this](In size_t threadID) {runReadAnimation(threadID); };
						m_readAnimation.AddDependency(m_processAI);
						m_readAnimation.AddDependency(m_updateGame);
						//Next position update should wait for last (current) mirroring, as mirroring might still access data
						m_readAnimation.AddDependency(m_mirrorGameState);
						m_readAnimation.Schedule(SIMULATION_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runUpdateRootMotion(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						const API::Global::Time ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
						if (m_lastUpdate + MAX_PHYSICS_DELAY < ctm) { m_lastUpdate = ctm - MAX_PHYSICS_DELAY; }

						API::Global::Task st[2];
						API::Global::Task nt[1];
						size_t ss = 0;
						size_t ns = 0;
						if (m_lastUpdate + PHYSICS_UPDATE_DURATION < ctm)
						{
							st[ss++] = [this, ctm](In size_t threadID)
							{
								//TODO
								//Engine::Physics::UpdateBegin();
								runUpdatePhysics(threadID, ctm);
							};
							m_updateVisibility.AddDependency(st[ss - 1]);
						}
						else
						{
							st[ss++] = m_readInput;
							st[ss++] = m_updateAI;
							nt[ns++] = m_updateVisibility;

							//Since the tasks are copied first, their copies need to be scheduled as well to actually schedule the jobs
							//We simulate a pause/shutdown by not scheduling these two jobs, resulting in stopping exactly after one frame
							if (WindowHandler::isFocused() == true)
							{
								m_readInput.Schedule(SIMULATION_IMPORTANT, threadID);
								m_updateAI.Schedule(SIMULATION_IMPORTANT, threadID);
							}
							else
							{
								m_paused = true;
								m_inactive = true;
								Awake();
							}
						}
						CLOAK_ASSUME(ss <= ARRAYSIZE(st));
						CLOAK_ASSUME(ns <= ARRAYSIZE(nt));

						//TODO
						//API::World::Group<Impl::Components::Animation, Impl::Components::Position> group;
						//for (const auto& a : group)
						//{
						//	Impl::Components::Animation* n = a.Get<Impl::Components::Animation>();
						//	Impl::Components::Position* p = a.Get<Impl::Components::Position>();
						//	API::Global::Task x = [p](In size_t threadID) { p->ApplyRootMotion(n); p->Release(); n->Release(); };
						//	for (size_t b = 0; b < ss; b++) { st[b].AddDependency(x); }
						//	for (size_t b = 0; b < ns; b++) { nt[b].AddDependency(x); }
						//	x.Schedule(SIMULATION_IMPORTANT, threadID);
						//}

						for (size_t a = 0; a < ss; a++) { st[a].Schedule(SIMULATION_IMPORTANT, threadID); }

						m_updateRootMotion = [this](In size_t threadID) { runUpdateRootMotion(threadID); };
						m_updateRootMotion.AddDependency(m_updatePosition);
						m_updateRootMotion.AddDependency(m_readAnimation);
						m_updateRootMotion.Schedule(SIMULATION_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runUpdatePhysics(In size_t threadID, In API::Global::Time startTime)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//Engine::Physics::Update();
						API::Global::Task st[2];
						API::Global::Task nt[1];
						size_t ss = 0;
						size_t ns = 0;
						if (m_lastUpdate + PHYSICS_UPDATE_DURATION < startTime)
						{
							st[ss++] = [this, startTime](In size_t threadID) { runUpdatePhysics(threadID, startTime); };
							m_updateVisibility.AddDependency(st[ss - 1]);
						}
						else
						{
							st[ss++] = m_readInput;
							st[ss++] = m_updateAI;
							nt[ns++] = m_updateVisibility;

							//Since the tasks are copied first, their copies need to be scheduled as well to actually schedule the jobs
							//We simulate a pause/shutdown by not scheduling these two jobs, resulting in stopping exactly after one frame
							if (WindowHandler::isFocused() == true)
							{
								m_readInput.Schedule(SIMULATION_IMPORTANT, threadID);
								m_updateAI.Schedule(SIMULATION_IMPORTANT, threadID);
							}
							else
							{
								m_paused = true;
								m_inactive = true;
								Awake();
							}
						}
						CLOAK_ASSUME(ss <= ARRAYSIZE(st));
						CLOAK_ASSUME(ns <= ARRAYSIZE(nt));

						if (m_lastUpdate < startTime)
						{
							m_lastUpdate += PHYSICS_UPDATE_DURATION;
							//TODO
							//API::World::Group<Impl::Components::Physic, Impl::Components::Position> group;
							//for (const auto& a : group)
							//{
							//	Impl::Components::Physic* ph = a.Get<Impl::Components::Physic>();
							//	Impl::Components::Position* p = a.Get<Impl::Components::Position>();
							//	API::Global::Task x = [ph, p](In size_t threadID) { ph->Update(); p->UpdateWorldPosition(); ph->Release(); p->Release(); };
							//	for (size_t b = 0; b < ss; b++) { st[b].AddDependency(x); }
							//	for (size_t b = 0; b < ns; b++) { nt[b].AddDependency(x); }
							//	x.Schedule(SIMULATION_IMPORTANT, threadID);
							//}
						}
						for (size_t a = 0; a < ss; a++) { st[a].Schedule(SIMULATION_IMPORTANT, threadID); }
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runUpdateVisibility(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO: Check visibility of all objects in scene with frustrum culling
						//API::World::Group<Impl::Components::Rendering, Impl::Components::Position> group;
						//for (const auto& a : group)
						//{
						//	Impl::Components::Rendering* r = a.Get<Impl::Components::Rendering>();
						//	Impl::Components::Position* p = a.Get<Impl::Components::Position>();
						//	API::Global::Task x = [r, p](In size_t threadID) { r->CheckVisibility(p); r->Release(); p->Release(); };
						//	m_mirrorGameState.AddDependency(x);
						//	x.Schedule(RENDERING_IMPORTANT, threadID);
						//}

						m_updateVisibility = [this](In size_t threadID) { runUpdateVisibility(threadID); };
						m_updateVisibility.AddDependency(m_updateRootMotion);
						m_updateVisibility.Schedule(RENDERING_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runMirrorGameState(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO: Copy game state of all visible objects for rendering
						//API::World::Group<Impl::Components::Rendering, Impl::Components::Position, Impl::Components::Animation> group;
						//for (const auto& a : group)
						//{
						//	Impl::Components::Rendering* r = a.Get<Impl::Components::Rendering>();
						//	Impl::Components::Position* p = a.Get<Impl::Components::Position>();
						//	Impl::Components::Animation* n = a.Get<Impl::Components::Animation>();
						//	if (r->IsVisible())
						//	{
						//		API::Global::Task x = [r, p](In size_t threadID) { r->CopyGameState(p, n); r->Release(); p->Release(); n->Release(); };
						//		m_renderSubmit.AddDependency(x);
						//		x.Schedule(RENDERING_IMPORTANT, threadID);
						//	}
						//}

						m_mirrorGameState = [this](In size_t threadID) { runMirrorGameState(threadID); };
						m_mirrorGameState.AddDependency(m_updateAnimation);
						//Next mirror update needs to wait for last (current) rendering since it needs to reset and overwrite
						//the frame-specific memory
						m_mirrorGameState.AddDependency(m_renderSubmit);
						m_mirrorGameState.Schedule(RENDERING_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runRenderSubmit(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO
						//Engine::Graphic::Core::Submit();

						m_renderSubmit = [this](In size_t threadID) { runRenderSubmit(threadID); };
						m_renderSubmit.AddDependency(m_mirrorGameState);
						//Next render submit needs to wait for last (current) render present to prevent rendering calls overlapping
						m_renderSubmit.AddDependency(m_renderPresent);
						m_renderSubmit.Schedule(RENDERING_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
				void CLOAK_CALL_THIS RenderLoopThread::runRenderPresent(In size_t threadID)
				{
					if (m_run == true && Impl::Global::Game::canThreadRun() == true)
					{
						//TODO
						//Engine::Graphic::Core::Present();

						API::Global::Time ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
						m_fps = Impl::Global::Game::threadFPS(ctm, &m_tfi);
						m_renderPresent = Impl::Global::Task::Sleep(Impl::Global::Game::threadSleep(ctm, m_sleepTime, &m_tsi), [this](In size_t threadID) { runRenderPresent(threadID); });
						m_renderPresent.AddDependency(m_renderSubmit);
						m_renderPresent.Schedule(RENDERING_IMPORTANT, threadID);
					}
					else { runStop(threadID); }
				}
			}
		}
	}
}