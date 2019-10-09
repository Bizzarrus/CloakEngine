#include "stdafx.h"
#include "Engine/GameCoreLoop.h"

#include "CloakEngine/Global/Task.h"

#include "Implementation/Global/Game.h"
#include "Implementation/Global/Task.h"
#include "Implementation/World/ComponentManager.h"

#include "Engine/FileLoader.h"
#include "Engine/Graphic/Core.h"

//#define PRINT_STAGE_NAMES
//#define PRINT_RENDERING_STAGES
//#define INPUT_DELAY_PROBE

namespace CloakEngine {
	namespace Engine {
		namespace Game {
			constexpr float MICROSEC_PER_SEC = 1000000.0f;
			constexpr API::Global::Time DEFAULT_SLEEP_TIME = static_cast<API::Global::Time>(MICROSEC_PER_SEC / 30);

			struct TaskList {
				union {
					struct {
						API::Global::Task OnStart;
						API::Global::Task OnStop;
						API::Global::Task ReadInput;
						API::Global::Task ReadAI;
						API::Global::Task ProcessInput;
						API::Global::Task UpdateGame;
						API::Global::Task ProcessAI;
						API::Global::Task UpdateComponents;
						API::Global::Task UpdatePositions;
						API::Global::Task WriteAI;
						API::Global::Task ReadAnimation;
						API::Global::Task UpdateRootMotion;
						API::Global::Task UpdatePhysics;
						API::Global::Task UpdateVisibility;
						API::Global::Task UpdateAnimation;
						API::Global::Task UpdateAudio;
						API::Global::Task MirrorGameState;
						struct {
							API::Global::Task CopyMirror;
							API::Global::Task Submit;
							API::Global::Task Present;
						} Rendering;
					};
					API::Global::Task Task[20];
				};

				CLOAK_CALL TaskList()
				{
					for (size_t a = 0; a < ARRAYSIZE(Task); a++) { Task[a] = nullptr; }
				}
				CLOAK_CALL ~TaskList()
				{
					for (size_t a = 0; a < ARRAYSIZE(Task); a++) { Task[a].~Task(); }
				}
			} g_tasks;
			static_assert(sizeof(TaskList) == sizeof(API::Global::Task) * ARRAYSIZE(TaskList::Task), "Task array must be at least as large as the number of named tasks!");

			std::atomic<bool> g_run = true;
			std::atomic<float> g_frameSleep = 0;
			std::atomic<float> g_fps = 0;
			std::atomic<float> g_timeScale = 1;
			std::atomic<API::Global::Graphic::InputDelayPrevention> g_idpl = API::Global::Graphic::InputDelayPrevention::Disabled;
			struct {
				struct {
					API::Global::Time Time = 0;
					API::Global::Time Elapsed = 0;
				} Unscaled;
				struct {
					API::Global::Time Time = 0;
					API::Global::Time Elapsed = 0;
					float RoundingError = 0;
				} Scaled; //These times are used for AI decisions
				struct {
					API::Global::Time Time = 0;
					API::Global::Time Elapsed = 0;
				} Physics; //These times are used for physics and animation calculation
				struct {
					API::Global::Time Elapsed = 0;
				} Rendering; //These times are used for rendering
			} g_gameTime; 
			API::Global::Time g_lastFrame = 0;
			Impl::Global::Game::ThreadSleepState g_tss;
			Impl::Global::Game::ThreadComState g_tcs;
			Impl::Global::Game::ThreadFPSState g_tfs;
			API::Global::IGameEvent* g_game = nullptr;

			namespace Stages {
				enum class Stage {
					ReadInput,
					ReadAI,
					ProcessInput,
					UpdateGame,
					ProcessAI,
					UpdateComponents,
					UpdatePositions,
					WriteAI,
					ReadAnimation,
					UpdateRootMotion,
					UpdatePhysics,
					UpdateVisibility,
					UpdateAnimation,
					UpdateAudio,
					MirrorGameState,
				};
				enum class RenderingStage {
					CopyMirror,
					Submit,
					Present,
				};

				template<typename T, T> struct Dependency{};
				template<> struct Dependency<Stage, Stage::ReadInput> 
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t, In API::Global::Graphic::InputDelayPrevention idpl)
					{
						switch (idpl)
						{
							case API::Global::Graphic::InputDelayPrevention::Disabled:
								t.AddDependency(g_tasks.UpdateGame);
								break;
							case API::Global::Graphic::InputDelayPrevention::Level1:
							case API::Global::Graphic::InputDelayPrevention::Level2:
							case API::Global::Graphic::InputDelayPrevention::Level3:
								t.AddDependency(g_tasks.ReadAI);
								break;
							default:
								break;
						}
						
					}
				};
				template<> struct Dependency<Stage, Stage::ReadAI> 
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdateAudio);
					}
				};
				template<> struct Dependency<Stage, Stage::ProcessInput>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.ReadInput);
						t.AddDependency(g_tasks.ReadAI);
					}
				};
				template<> struct Dependency<Stage, Stage::UpdateGame>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.ProcessInput);
					}
				};
				template<> struct Dependency<Stage, Stage::ProcessAI>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.ReadAI);
					}
				};
				template<> struct Dependency<Stage, Stage::UpdateComponents>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdateGame);
						t.AddDependency(g_tasks.MirrorGameState);
						t.AddDependency(g_tasks.UpdateAnimation);
					}
				};
				template<> struct Dependency<Stage, Stage::UpdatePositions>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdateComponents);
						t.AddDependency(g_tasks.ProcessAI);
					}
				};
				template<> struct Dependency<Stage, Stage::WriteAI>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdateComponents);
						t.AddDependency(g_tasks.ProcessAI);
					}
				};
				template<> struct Dependency<Stage, Stage::ReadAnimation>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.WriteAI);
					}
				};
				template<> struct Dependency<Stage, Stage::UpdateRootMotion>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdatePositions);
						t.AddDependency(g_tasks.ReadAnimation);
					}
				};
				template<> struct Dependency<Stage, Stage::UpdatePhysics>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdateRootMotion);
					}
				};
				template<> struct Dependency<Stage, Stage::UpdateVisibility>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdatePhysics);
					}
				};
				template<> struct Dependency<Stage, Stage::UpdateAnimation>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdateVisibility);
					}
				};
				template<> struct Dependency<Stage, Stage::UpdateAudio>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t)
					{
						t.AddDependency(g_tasks.UpdatePhysics);
					}
				};
				template<> struct Dependency<Stage, Stage::MirrorGameState>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t, In API::Global::Graphic::InputDelayPrevention idpl)
					{
						switch (idpl)
						{
							case API::Global::Graphic::InputDelayPrevention::Disabled:
							case API::Global::Graphic::InputDelayPrevention::Level1:
								t.AddDependency(g_tasks.Rendering.CopyMirror);
								break;
							case API::Global::Graphic::InputDelayPrevention::Level2:
								t.AddDependency(g_tasks.Rendering.Submit);
								break;
							case API::Global::Graphic::InputDelayPrevention::Level3:
								t.AddDependency(g_tasks.Rendering.Present);
								break;
							default:
								break;
						}

						t.AddDependency(g_tasks.UpdateAnimation);
					}
				};
				
				template<> struct Dependency<RenderingStage, RenderingStage::CopyMirror>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t, In API::Global::Graphic::InputDelayPrevention idpl)
					{
						switch (idpl)
						{
							case API::Global::Graphic::InputDelayPrevention::Disabled:
							case API::Global::Graphic::InputDelayPrevention::Level1:
								t.AddDependency(g_tasks.Rendering.Submit);
								break;
							case API::Global::Graphic::InputDelayPrevention::Level2:
							case API::Global::Graphic::InputDelayPrevention::Level3:
								break;
							default:
								break;
						}
					}
				};
				template<> struct Dependency<RenderingStage, RenderingStage::Submit>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t, In API::Global::Graphic::InputDelayPrevention idpl)
					{
						switch (idpl)
						{
							case API::Global::Graphic::InputDelayPrevention::Disabled:
							case API::Global::Graphic::InputDelayPrevention::Level1:
							case API::Global::Graphic::InputDelayPrevention::Level2:
								t.AddDependency(g_tasks.Rendering.Present);
								break;
							case API::Global::Graphic::InputDelayPrevention::Level3:
								break;
							default:
								break;
						}

						t.AddDependency(g_tasks.Rendering.CopyMirror);
					}
				};
				template<> struct Dependency<RenderingStage, RenderingStage::Present>
				{
					static inline void CLOAK_CALL Set(Inout API::Global::Task& t, In API::Global::Graphic::InputDelayPrevention idpl)
					{
						t.AddDependency(g_tasks.Rendering.Submit);
					}
				};

#if (defined(INPUT_DELAY_PROBE) && defined(_DEBUG))
				struct IDP {
					bool InRenderer = false;
					uint32_t Stage;
					uint64_t StartFrame = 0;
				};
				std::atomic<IDP> g_curProbe;
				std::atomic<bool> g_hasProbe = false;
				std::atomic<uint64_t> g_curFrame = 0;

#define UPDATE_INPUT_DELAY_PROBE(cur, next) if(g_hasProbe == true) { IDP _e = g_curProbe.load(); IDP _n; do { _n = _e; if(_e.Stage != static_cast<uint32_t>(cur) || _e.InRenderer != std::is_same_v<decltype(cur), RenderingStage>){ break; } _n.Stage = static_cast<uint32_t>(next); _n.InRenderer = std::is_same_v<decltype(next), RenderingStage>;} while(g_curProbe.compare_exchange_weak(_e, _n) == false); }
#else
#define UPDATE_INPUT_DELAY_PROBE(cur, next)
#endif
				namespace Rendering {
#if (defined(PRINT_RENDERING_STAGES) && defined(_DEBUG))
					uint64_t dbgFID = 0;
					uint64_t dbgFIDM = 0;
					uint64_t dbgFIDS = 0;
#endif

					inline void CLOAK_CALL Present(In size_t threadID)
					{
						//Engine::Graphic::Core_v2::Present();
#if (defined(INPUT_DELAY_PROBE) && defined(_DEBUG))
						const uint64_t cf = g_curFrame.fetch_add(1) + 1;
						if (g_hasProbe == true)
						{
							IDP i = g_curProbe.load();
							if (i.InRenderer == true && i.Stage == static_cast<uint32_t>(RenderingStage::Present))
							{
								CloakDebugLog("Input Delay: " + std::to_string(cf - i.StartFrame) + " Frames");
								g_hasProbe = false;
							}
						}
#endif
						g_fps.store(g_tfs.Update(), std::memory_order_relaxed);
#if (defined(PRINT_RENDERING_STAGES) && defined(_DEBUG))
						CloakDebugLog("CGL: Present Frame: " + std::to_string(dbgFIDS));
#endif
					}
					inline void CLOAK_CALL Submit(In size_t threadID)
					{
						UPDATE_INPUT_DELAY_PROBE(RenderingStage::Submit, RenderingStage::Present);
						//Engine::Graphic::Core_v2::Submit(g_gameTime.Rendering.Elapsed);
#if (defined(PRINT_RENDERING_STAGES) && defined(_DEBUG))
						dbgFIDS = dbgFIDM;
						CloakDebugLog("CGL: Submit Frame: " + std::to_string(dbgFIDM));
#endif
					}
					inline void CLOAK_CALL CopyMirror(In size_t threadID)
					{
						UPDATE_INPUT_DELAY_PROBE(RenderingStage::CopyMirror, RenderingStage::Submit);
						//TODO: swap game state mirrors
#if (defined(PRINT_RENDERING_STAGES) && defined(_DEBUG))
						dbgFIDM = dbgFID;
						CloakDebugLog("CGL: Copy Frame Mirror: " + std::to_string(dbgFID));
#endif
					}

				}

				inline void CLOAK_CALL ReadInput(In size_t threadID)
				{
#if (defined(INPUT_DELAY_PROBE) && defined(_DEBUG))
					if (g_hasProbe == false)
					{
						IDP i;
						i.InRenderer = false;
						i.Stage = static_cast<uint32_t>(Stage::ProcessInput);
						i.StartFrame = g_curFrame.load();
						g_curProbe = i;
						g_hasProbe = true;
					}
#endif
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Read Input");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.ReadInput = [](In size_t threadID) { ReadInput(threadID); };
						Dependency<Stage, Stage::ReadInput>::Set(g_tasks.ReadInput, g_idpl.load(std::memory_order_consume));
						g_tasks.ReadInput.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.ReadInput); }
					}
				}
				inline void CLOAK_CALL ReadAI(In size_t threadID)
				{
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Read AI State");
#endif
					const API::Global::Time gameTime = API::Global::Game::GetGameTimeStamp();
					g_gameTime.Unscaled.Elapsed = g_gameTime.Unscaled.Time == 0 ? 0 : (g_gameTime.Unscaled.Time - gameTime);
					g_gameTime.Unscaled.Time = gameTime;
					const long double segt = g_gameTime.Scaled.RoundingError + (static_cast<long double>(g_gameTime.Unscaled.Elapsed) * g_timeScale.load(std::memory_order_consume));
					g_gameTime.Scaled.Elapsed = static_cast<API::Global::Time>(std::floor(segt));
					g_gameTime.Scaled.Time += g_gameTime.Scaled.Elapsed;
					g_gameTime.Scaled.RoundingError = static_cast<float>(segt - g_gameTime.Scaled.Elapsed);

					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.ReadAI = [](In size_t threadID) { ReadAI(threadID); };
						Dependency<Stage, Stage::ReadAI>::Set(g_tasks.ReadAI);
						g_tasks.ReadAI.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.ReadAI); }
					}
				}
				inline void CLOAK_CALL ProcessInput(In size_t threadID)
				{
					UPDATE_INPUT_DELAY_PROBE(Stage::ProcessInput, Stage::UpdateGame);
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Process Input");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.ProcessInput = [](In size_t threadID) { ProcessInput(threadID); };
						Dependency<Stage, Stage::ProcessInput>::Set(g_tasks.ProcessInput);
						g_tasks.ProcessInput.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.ProcessInput); }
					}
				}
				inline void CLOAK_CALL UpdateGame(In size_t threadID, In bool hasWindow)
				{
					UPDATE_INPUT_DELAY_PROBE(Stage::UpdateGame, Stage::UpdatePositions);
					const API::Global::Time cft = Impl::Global::Game::getCurrentTimeMicroSeconds();
					const API::Global::Time etime = g_lastFrame == 0 ? 0 : (cft - g_lastFrame);
					g_lastFrame = cft;
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Update Game");
#endif
					g_game->OnUpdate(etime, g_gameTime.Scaled.Time, Engine::FileLoader::isLoading(API::Files::Priority::NORMAL));

					if (g_run.load(std::memory_order_consume) == true)
					{
						if (hasWindow == true) 
						{ 
							g_tasks.UpdateGame = [](In size_t threadID) { UpdateGame(threadID, true); }; 
						}
						else 
						{
							g_fps.store(g_tfs.Update(cft), std::memory_order_relaxed);
							g_tasks.UpdateGame = Impl::Global::Task::Sleep(g_tss.GetSleepTime(cft, g_frameSleep.load(std::memory_order_relaxed)), [](In size_t threadID) { UpdateGame(threadID, false); });
						}
						Dependency<Stage, Stage::UpdateGame>::Set(g_tasks.UpdateGame);
						g_tasks.UpdateGame.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.UpdateGame); }
					}
				}
				inline void CLOAK_CALL ProcessAI(In size_t threadID)
				{
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Process AI");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.ProcessAI = [](In size_t threadID) { ProcessAI(threadID); };
						Dependency<Stage, Stage::ProcessAI>::Set(g_tasks.ProcessAI);
						g_tasks.ProcessAI.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.ProcessAI); }
					}
				}
				inline void CLOAK_CALL UpdateComponents(In size_t threadID)
				{
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Update Components");
#endif
					API::Global::Task follow[] = { g_tasks.UpdatePositions, g_tasks.WriteAI };
					Impl::World::ComponentManager_v2::ComponentAllocator::FixAll(threadID, ARRAYSIZE(follow), follow);

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.UpdateComponents = [](In size_t threadID) { UpdateComponents(threadID); };
						Dependency<Stage, Stage::UpdateComponents>::Set(g_tasks.UpdateComponents);
						g_tasks.UpdateComponents.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.UpdateComponents); }
					}
				}
				inline void CLOAK_CALL UpdatePositions(In size_t threadID)
				{
					UPDATE_INPUT_DELAY_PROBE(Stage::UpdatePositions, Stage::UpdatePhysics);
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Update Positions");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.UpdatePositions = [](In size_t threadID) { UpdatePositions(threadID); };
						Dependency<Stage, Stage::UpdatePositions>::Set(g_tasks.UpdatePositions);
						g_tasks.UpdatePositions.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.UpdatePositions); }
					}
				}
				inline void CLOAK_CALL WriteAI(In size_t threadID)
				{
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Write AI State");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.WriteAI = [](In size_t threadID) { WriteAI(threadID); };
						Dependency<Stage, Stage::WriteAI>::Set(g_tasks.WriteAI);
						g_tasks.WriteAI.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.WriteAI); }
					}
				}
				inline void CLOAK_CALL ReadAnimation(In size_t threadID)
				{
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Read Animation State");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.ReadAnimation = [](In size_t threadID) { ReadAnimation(threadID); };
						Dependency<Stage, Stage::ReadAnimation>::Set(g_tasks.ReadAnimation);
						g_tasks.ReadAnimation.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.ReadAnimation); }
					}
				}
				inline void CLOAK_CALL UpdateRootMotion(In size_t threadID)
				{
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Update Root Motion");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.UpdateRootMotion = [](In size_t threadID) { UpdateRootMotion(threadID); };
						Dependency<Stage, Stage::UpdateRootMotion>::Set(g_tasks.UpdateRootMotion);
						g_tasks.UpdateRootMotion.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.UpdateRootMotion); }
					}
				}
				inline void CLOAK_CALL UpdatePhysics(In size_t threadID)
				{
					UPDATE_INPUT_DELAY_PROBE(Stage::UpdatePhysics, Stage::UpdateVisibility);
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Update Physics");
#endif
					g_gameTime.Physics.Time = g_gameTime.Scaled.Time;
					g_gameTime.Physics.Elapsed = g_gameTime.Scaled.Elapsed;
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.UpdatePhysics = [](In size_t threadID) { UpdatePhysics(threadID); };
						Dependency<Stage, Stage::UpdatePhysics>::Set(g_tasks.UpdatePhysics);
						g_tasks.UpdatePhysics.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.UpdatePhysics); }
					}
				}
				inline void CLOAK_CALL UpdateVisibility(In size_t threadID)
				{
					UPDATE_INPUT_DELAY_PROBE(Stage::UpdateVisibility, Stage::UpdateAnimation);
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Update Visibility");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.UpdateVisibility = [](In size_t threadID) { UpdateVisibility(threadID); };
						Dependency<Stage, Stage::UpdateVisibility>::Set(g_tasks.UpdateVisibility);
						g_tasks.UpdateVisibility.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.UpdateVisibility); }
					}
				}
				inline void CLOAK_CALL UpdateAnimation(In size_t threadID)
				{
					UPDATE_INPUT_DELAY_PROBE(Stage::UpdateAnimation, Stage::MirrorGameState);
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Update Animation");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.UpdateAnimation = [](In size_t threadID) { UpdateAnimation(threadID); };
						Dependency<Stage, Stage::UpdateAnimation>::Set(g_tasks.UpdateAnimation);
						g_tasks.UpdateAnimation.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.UpdateAnimation); }
					}
				}
				inline void CLOAK_CALL UpdateAudio(In size_t threadID)
				{
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Update Audio");
#endif
					//TODO

					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.UpdateAudio = [](In size_t threadID) { UpdateAudio(threadID); };
						Dependency<Stage, Stage::UpdateAudio>::Set(g_tasks.UpdateAudio);
						g_tasks.UpdateAudio.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.UpdateAudio); }
					}
				}
				inline void CLOAK_CALL MirrorGameState(In size_t threadID)
				{
					UPDATE_INPUT_DELAY_PROBE(Stage::MirrorGameState, RenderingStage::CopyMirror);

					API::Global::Time cft = Impl::Global::Game::getCurrentTimeMicroSeconds();
					g_gameTime.Rendering.Elapsed = g_gameTime.Physics.Elapsed;
					const API::Global::Graphic::InputDelayPrevention idpl = g_idpl.load(std::memory_order_consume);

					g_tasks.Rendering.CopyMirror = [](In size_t threadID) { Rendering::CopyMirror(threadID); };
					Dependency<RenderingStage, RenderingStage::CopyMirror>::Set(g_tasks.Rendering.CopyMirror, idpl);

					g_tasks.Rendering.Submit = [](In size_t threadID) { Rendering::Submit(threadID); };
					Dependency<RenderingStage, RenderingStage::Submit>::Set(g_tasks.Rendering.Submit, idpl);
					g_tasks.Rendering.Submit.Schedule(threadID);

					g_tasks.Rendering.Present = [](In size_t threadID) { Rendering::Present(threadID); };
					Dependency<RenderingStage, RenderingStage::Present>::Set(g_tasks.Rendering.Present, idpl);
					g_tasks.Rendering.Present.Schedule(threadID);

#if (defined(PRINT_RENDERING_STAGES) && defined(_DEBUG))
					dbgFID++;
					CloakDebugLog("CGL: Create Frame Mirror: " + std::to_string(dbgFID));
#endif
					//Copy data into mirror:
					{
						API::Global::Task follow[] = { g_tasks.Rendering.CopyMirror, g_tasks.UpdateComponents };
						//TODO: Fill new mirror
					}

					g_tasks.Rendering.CopyMirror.Schedule(threadID);
					if (g_run.load(std::memory_order_consume) == true)
					{
						g_tasks.MirrorGameState = Impl::Global::Task::Sleep(g_tss.GetSleepTime(cft, g_frameSleep.load(std::memory_order_relaxed)), [](In size_t threadID) { MirrorGameState(threadID); });
						Dependency<Stage, Stage::MirrorGameState>::Set(g_tasks.MirrorGameState, idpl);
						g_tasks.MirrorGameState.Schedule(threadID);
						if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.MirrorGameState); }
					}
					if (g_run.load(std::memory_order_consume) == false) { g_tasks.OnStop.AddDependency(g_tasks.Rendering.Present); }
				}

				inline void CLOAK_CALL OnStart(In size_t threadID, In bool hasWindow)
				{
					if (g_run.load(std::memory_order_consume) == true)
					{
						const Impl::Global::Game::ThreadComState::State ccs = g_tcs.Check();
						if (ccs == Impl::Global::Game::ThreadComState::UPDATED)
						{
							const uint8_t nCC = g_tcs.GetCurrentLevel();
							CLOAK_ASSUME(nCC < Impl::Global::Game::ThreadComState::MAX_LEVEL);
							const uint8_t ccl = Impl::Global::Game::ThreadComState::MAX_LEVEL - (nCC + 1);
							switch (ccl)
							{
								case 0:
									//TODO
									break;
								case 1:
									//TODO
									g_game->OnInit();
									//Engine::Graphic::Core_v2::InitializePipeline();
									break;
								case 2:
									//TODO
									break;
								case 3:
									//TODO
									//if (hasWindow == true) { Engine::Graphic::Core_v2::InitializeWindow(); }
									break;
								case 4:
									//TODO
									g_game->OnStart();
									break;
								default:
									break;
							}
							if (nCC == 1) { CloakDebugLog("Starting game main loop"); }
							g_tcs.Update();
						}
						else if (ccs == Impl::Global::Game::ThreadComState::FINISHED)
						{
							const API::Global::Graphic::InputDelayPrevention idpl = g_idpl.load(std::memory_order_consume);
							//Start main loop (Notice: Order of operation is important here!):
							g_tasks.ReadAI = [](In size_t threadID) { ReadAI(threadID); };
							Dependency<Stage, Stage::ReadAI>::Set(g_tasks.ReadAI);
							g_tasks.ReadInput			= [](In size_t threadID) { ReadInput(threadID); };
							Dependency<Stage, Stage::ReadInput>::Set(g_tasks.ReadInput, idpl);
							g_tasks.ProcessInput		= [](In size_t threadID) { ProcessInput(threadID); };
							Dependency<Stage, Stage::ProcessInput>::Set(g_tasks.ProcessInput);
							g_tasks.UpdateGame			= [hasWindow](In size_t threadID) { UpdateGame(threadID, hasWindow); };
							Dependency<Stage, Stage::UpdateGame>::Set(g_tasks.UpdateGame);
							g_tasks.ProcessAI			= [](In size_t threadID) { ProcessAI(threadID); };
							Dependency<Stage, Stage::ProcessAI>::Set(g_tasks.ProcessAI);
							g_tasks.UpdateComponents	= [](In size_t threadID) { UpdateComponents(threadID); };
							Dependency<Stage, Stage::UpdateComponents>::Set(g_tasks.UpdateComponents);
							g_tasks.UpdatePositions		= [](In size_t threadID) { UpdatePositions(threadID); };
							Dependency<Stage, Stage::UpdatePositions>::Set(g_tasks.UpdatePositions);
							g_tasks.WriteAI				= [](In size_t threadID) { WriteAI(threadID); };
							Dependency<Stage, Stage::WriteAI>::Set(g_tasks.WriteAI);
							g_tasks.ReadAnimation		= [](In size_t threadID) { ReadAnimation(threadID); };
							Dependency<Stage, Stage::ReadAnimation>::Set(g_tasks.ReadAnimation);
							g_tasks.UpdateRootMotion	= [](In size_t threadID) { UpdateRootMotion(threadID); };
							Dependency<Stage, Stage::UpdateRootMotion>::Set(g_tasks.UpdateRootMotion);
							g_tasks.UpdatePhysics		= [](In size_t threadID) { UpdatePhysics(threadID); };
							Dependency<Stage, Stage::UpdatePhysics>::Set(g_tasks.UpdatePhysics);
							g_tasks.UpdateVisibility	= [](In size_t threadID) { UpdateVisibility(threadID); };
							Dependency<Stage, Stage::UpdateVisibility>::Set(g_tasks.UpdateVisibility);
							g_tasks.UpdateAnimation		= [](In size_t threadID) { UpdateAnimation(threadID); };
							Dependency<Stage, Stage::UpdateAnimation>::Set(g_tasks.UpdateAnimation);
							g_tasks.UpdateAudio			= [](In size_t threadID) { UpdateAudio(threadID); };
							Dependency<Stage, Stage::UpdateAudio>::Set(g_tasks.UpdateAudio);
							if (hasWindow == true)
							{
								g_tasks.MirrorGameState = [](In size_t threadID) { MirrorGameState(threadID); };
								Dependency<Stage, Stage::MirrorGameState>::Set(g_tasks.MirrorGameState, idpl);
							}
						
							for (size_t a = 0; a < ARRAYSIZE(g_tasks.Task); a++) { g_tasks.Task[a].Schedule(threadID); }
							if (g_run.load(std::memory_order_consume) == false) 
							{
								for (size_t a = 0; a < ARRAYSIZE(g_tasks.Task); a++) { g_tasks.OnStop.AddDependency(g_tasks.Task[a]); }
							}
							return;
						}
						g_tasks.OnStart = Impl::Global::Task::Sleep(DEFAULT_SLEEP_TIME, [hasWindow](In size_t threadID) { OnStart(threadID, hasWindow); });
						g_tasks.OnStart.Schedule(threadID);
					}
				}
				inline void CLOAK_CALL OnStop(In size_t threadID)
				{
#if (defined(PRINT_STAGE_NAMES) && defined(_DEBUG))
					CloakDebugLog("CGL: Stop");
#endif
					for (size_t a = 0; a < ARRAYSIZE(g_tasks.Task); a++)
					{
						CLOAK_ASSUME(g_tasks.Task[a].Finished() || g_tasks.Task[a] == g_tasks.OnStop);
						g_tasks.Task[a] = nullptr;
					}
					//Call shutdown functions:
					//TODO
					g_game->OnStop();

					Impl::Global::Game::registerThreadStop();
				}
			}

			void CLOAK_CALL Initialize(In bool hasWindow, In float targetFPS, In API::Global::IGameEvent* game)
			{
				g_game = game;
				g_frameSleep.store(targetFPS <= 0.0f ? 0.0f : (MICROSEC_PER_SEC / targetFPS), std::memory_order_relaxed);
				Impl::Global::Game::registerThread();
				g_tasks.OnStart = [hasWindow](In size_t threadID) { Stages::OnStart(threadID, hasWindow); };
				g_tasks.OnStart.Schedule();
			}
			void CLOAK_CALL SignalStop()
			{
				g_run.store(false, std::memory_order_release);
				g_tasks.OnStop = [](In size_t threadID) { Stages::OnStop(threadID); };
				for (size_t a = 0; a < ARRAYSIZE(TaskList::Task); a++) { g_tasks.OnStop.AddDependency(g_tasks.Task[a]); }
				g_tasks.OnStop.Schedule();
			}
			void CLOAK_CALL SetIDPL(In API::Global::Graphic::InputDelayPrevention idp) { g_idpl.store(idp, std::memory_order_release); }
			void CLOAK_CALL SetFPS(In float fps)
			{
				g_frameSleep.store(fps <= 0.0f ? 0.0f : (MICROSEC_PER_SEC / fps), std::memory_order_relaxed);
			}
			float CLOAK_CALL GetFPS() { return g_fps.load(std::memory_order_consume); }
		}
	}
}