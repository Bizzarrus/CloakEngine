#include "stdafx.h"
#include "CloakEngine/Global/Game.h"
#include "Implementation/Global/Game.h"
#include "Implementation/Global/Debug.h"
#include "Implementation/Global/DebugLayer.h"
#include "Implementation/Global/Log.h"
#include "Implementation/Global/Graphic.h"
#include "Implementation/Global/Localization.h"
#include "Implementation/Global/Input.h"
#include "Implementation/Global/Audio.h"
#include "Implementation/Global/Mouse.h"
#include "Implementation/Global/Steam.h"
#include "Implementation/Global/Video.h"
#include "Implementation/Global/Connection.h"
#include "Implementation/Global/Memory.h"
#include "Implementation/Global/Task.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Helper/SyncSection.h"
#include "Implementation/Files/Compressor.h"
#include "Implementation/Files/Language.h"
#include "Implementation/Files/Configuration.h"
#include "Implementation/Files/Image.h"
#include "Implementation/Files/Font.h"
#include "Implementation/Files/Shader.h"
#include "Implementation/Files/Buffer.h"
#include "Implementation/Files/FileHandler.h"
#include "Implementation/Files/Scene.h"
#include "Implementation/Files/FileIOBuffer.h"
#include "Implementation/Interface/BasicGUI.h"
#include "Implementation/Interface/Button.h"
#include "Implementation/Interface/CheckButton.h"
#include "Implementation/Interface/DropDownMenu.h"
#include "Implementation/Interface/Frame.h"
#include "Implementation/Interface/Label.h"
#include "Implementation/Interface/EditBox.h"
#include "Implementation/Interface/Slider.h"
#include "Implementation/Interface/ProgressBar.h"
#include "Implementation/Interface/LoadScreen.h"
#include "Implementation/Interface/ListMenu.h"
#include "Implementation/Interface/MovableFrame.h"
#include "Implementation/OS.h"
#include "Implementation/Rendering/Allocator.h"
#include "Implementation/Rendering/Context.h"
#include "Implementation/Rendering/RootSignature.h"
#include "Implementation/World/Entity.h"
#include "Implementation/World/ComponentManager.h"

#include "Engine/Launcher.h"
#include "Engine/Thread.h"
#include "Engine/GameCoreLoop.h"
#include "Engine/WindowHandler.h"
#include "Engine/Graphic/Core.h"
#include "Engine/Graphic/GraphicLock.h"
#include "Engine/FileLoader.h"
#include "Engine/FlipVector.h"
#include "Engine/WorldStreamer.h"
#include "Engine/Compress/NetBuffer/SecureNet.h"
#include "Engine/Physics/Core.h"
#include "Engine\Audio\Core.h"

#include "CloakEngine/Global/Log.h"
#include "CloakEngine/Helper/StringConvert.h"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include <thread>
#include <string>


#ifdef _DEBUG
#define CREATE_THREAD(desc,label) Impl::Global::Game::createThread(desc,label)
#else
#define CREATE_THREAD(desc,label) Impl::Global::Game::createThread(desc)
#endif

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Game {
				constexpr float DEFAULT_FPS = 60.0f;

				struct ThreadInfo {
					bool checkUpdate = true;
					bool checkChanged = false;
					unsigned long long lastUpdate = 0;
				};
				struct AsyncEngineInfo {
					API::Global::IGameEventFactory* Factory;
					API::Global::GameInfo Info;
				};
				struct TimeInfo {
					API::Global::Time Offset = 0;
					API::Global::Time LastPaused = 0;
					bool Paused = false;
				};

				API::Helper::ISyncSection* g_syncPtrs = nullptr;
				API::Helper::ISyncSection* g_syncModulPath = nullptr;
#ifdef _DEBUG
				API::Helper::ISyncSection* g_syncResponde = nullptr;
				Engine::LinkedList<API::Helper::ISavePtr*>* g_ptrs;
#endif
				std::atomic<bool> g_run = true;
				std::atomic<bool> g_running = false;
				std::atomic<bool> g_freqInit = false;
				std::atomic<bool> g_debugMode = false;
				std::atomic<bool> g_hasWindow = true;
				std::atomic<bool> g_canComCheck = false;
				std::atomic<int> g_threadPaused = 0;
				std::atomic<unsigned long long> g_tof = 0;
				std::atomic<LARGE_INTEGER> g_fof;
				std::atomic<int> g_canComCount = 0;
				std::atomic<int> g_threadStopCount = 0;
				std::atomic<LARGE_INTEGER> g_freq;				
				std::atomic<float> g_fps = 0;
				std::atomic<float> g_sleepTime = 1000000.0f / DEFAULT_FPS;
				std::atomic<Impl::Global::DebugLayer*> g_dbgLayer = nullptr;
				std::atomic<TimeInfo> g_pauseTime;
				std::atomic<TimeInfo> g_runTime;
				std::atomic<uint8_t> g_currentComLevel = Impl::Global::Game::ThreadComState::MAX_LEVEL;
				
				size_t g_threadCount = 0;
				Engine::Thread::Thread* g_threads[API::Global::THREAD_COUNT - 1];
#ifdef _DEBUG
				Engine::FlipVector<ThreadInfo>* g_threadRespond;
#endif
				std::string* g_modulPath;
				bool g_hasModulPath = false;
				HANDLE g_onStopEvent = nullptr;
#ifdef _DEBUG
				void CLOAK_CALL removePtr(In ptrAdr ptr)
				{
					if (API::Global::Game::IsDebugMode())
					{
						API::Helper::Lock lock(g_syncPtrs);
						g_ptrs->erase(ptr);
					}
				}
				ptrAdr CLOAK_CALL registerPtr(In API::Helper::ISavePtr* ptr)
				{
					if (API::Global::Game::IsDebugMode())
					{
						if (ptr != nullptr)
						{
							API::Helper::Lock lock(g_syncPtrs);
							return g_ptrs->insert(ptr);
						}
					}
					return ptrAdr();
				}
#endif
				void CLOAK_CALL registerThread() 
				{ 
					g_canComCount += Impl::Global::Game::ThreadComState::MAX_LEVEL - 1;
					g_threadStopCount++; 
				}
				void CLOAK_CALL registerThreadStop() 
				{ 
					g_threadStopCount--; 
				}
#ifdef _DEBUG
				void CLOAK_CALL threadRespond(In unsigned int tID, In API::Global::Time time)
				{
					API::Helper::Lock lock(g_syncResponde);
					if (tID < g_threadRespond->get()->size()) 
					{
						ThreadInfo* ti = &g_threadRespond->get()->at(tID);
						ti->lastUpdate = time;
					}
				}
				void CLOAK_CALL disableThreadRespondCheck(In unsigned int tID)
				{
					API::Helper::Lock lock(g_syncResponde);
					if (tID < g_threadRespond->get()->size())
					{
						ThreadInfo* ti = &g_threadRespond->get()->at(tID);
						if (ti->checkUpdate == true) 
						{ 
							ti->checkUpdate = false;
							ti->checkChanged = true;
						}
					}
				}
				void CLOAK_CALL checkResponse(In API::Global::Time ct)
				{
					API::Helper::Lock lock(Impl::Global::Game::g_syncResponde);
					API::List<Impl::Global::Game::ThreadInfo>& v = Impl::Global::Game::g_threadRespond->flip();
					lock.unlock();
					const size_t tn = v.size();
					for (size_t a = 0; a < tn; a++)
					{
						Impl::Global::Game::ThreadInfo* t = &v[a];
						if (t->checkChanged == true)
						{
							lock.lock(Impl::Global::Game::g_syncResponde);
							Impl::Global::Game::ThreadInfo* n = &Impl::Global::Game::g_threadRespond->get()->at(a);
							if (n->checkChanged == true && n->checkUpdate != t->checkUpdate) { t->checkUpdate = n->checkUpdate; n->checkChanged = false; }
							else { n->checkUpdate = t->checkUpdate; }
							lock.unlock();
							t->checkChanged = false;
						}
						if (g_threads[a]->IsPaused() == false && CloakCheckError(t->checkUpdate == false || ct < t->lastUpdate || ct - t->lastUpdate < 5000, API::Global::Debug::Error::THREAD_NO_RESPONDE, false))
						{
							API::Global::Log::WriteToLog("Thread #" + std::to_string(a) + " doesn't responde! (Last Responde: " + std::to_string(t->lastUpdate) + ")", API::Global::Log::Type::Warning);
							t->checkUpdate = false;
							lock.lock(Impl::Global::Game::g_syncResponde);
							Impl::Global::Game::g_threadRespond->get()->at(a).checkUpdate = false;
							lock.unlock();
						}
					}
				}
#endif
				bool CLOAK_CALL canThreadStop() { return g_run || g_threadStopCount <= 0; }
				bool CLOAK_CALL canThreadRun() { return g_run && g_running; }
				void CLOAK_CALL looseFocus()
				{
					TimeInfo lI = g_runTime;
					if (lI.Paused == false)
					{
						TimeInfo nI;
						do {
							nI = lI;
							if (nI.Paused == false)
							{
								nI.Paused = true;
								nI.LastPaused = getCurrentTimeMilliSeconds();
							}
						} while (!g_runTime.compare_exchange_strong(lI, nI));
					}
				}
				void CLOAK_CALL gainFocus()
				{
					TimeInfo lI = g_runTime;
					if (lI.Paused == true)
					{
						TimeInfo nI;
						do {
							nI = lI;
							if (nI.Paused == true)
							{
								nI.Paused = false;
								nI.Offset += getCurrentTimeMilliSeconds() - nI.LastPaused;
							}
						} while (!g_runTime.compare_exchange_strong(lI, nI));
					}
					wakeThreads();
				}
				void CLOAK_CALL wakeThreads()
				{
					if (GetCurrentComLevel() == 0 && canThreadRun())
					{
						const size_t tc = g_threadCount;
						for (size_t a = 0; a < tc; a++) { g_threads[a]->Awake(); }
					}
				}
				void CLOAK_CALL updateThreadPause(In bool mode)
				{
					if (mode == true) { g_threadPaused++; }
					else { g_threadPaused--; }
				}
				bool CLOAK_CALL checkThreadWaiting() { return g_threadPaused > 0; }
				float CLOAK_CALL_THIS ThreadFPSState::Update() { return Update(Impl::Global::Game::getCurrentTimeMicroSeconds()); }
				float CLOAK_CALL_THIS ThreadFPSState::Update(In API::Global::Time currentTime)
				{
					float res = m_lastFPS;
					if (currentTime > m_last)
					{
						const API::Global::Time etime = currentTime - m_last;
						m_turns++;
						if (etime >= 500000)
						{
							const API::Global::Time pet = currentTime - m_prev;
							res = ((m_turns + m_lastTurns) * 1000000.0f) / pet;
							m_prev = m_last;
							m_lastTurns = m_turns;
							m_last = currentTime;
							m_turns = 0;
							m_lastFPS = res;
						}
					}
					return res;
				}
				API::Global::Time CLOAK_CALL_THIS ThreadSleepState::GetSleepTime(In float targetFrameTime) 
				{
					const API::Global::Time cur = getCurrentTimeMicroSeconds();
					return GetSleepTime(cur, cur, targetFrameTime);
				}
				API::Global::Time CLOAK_CALL_THIS ThreadSleepState::GetSleepTime(In API::Global::Time frameStart, In float targetFrameTime) 
				{
					const API::Global::Time cur = getCurrentTimeMicroSeconds();
					return GetSleepTime(frameStart, cur, targetFrameTime);
				}
				API::Global::Time CLOAK_CALL_THIS ThreadSleepState::GetSleepTime(In API::Global::Time frameStart, In API::Global::Time currentTime, In float targetFrameTime) 
				{
					if (static_cast<long double>(currentTime) < static_cast<long double>(frameStart) + targetFrameTime)
					{
						long double st = targetFrameTime + m_round - static_cast<long double>(currentTime - (m_expectedAwake == 0 ? frameStart : m_expectedAwake));
						st = max(0, st);
						const API::Global::Time tts = static_cast<API::Global::Time>(std::floor(st));
						m_round = st - tts;
						m_expectedAwake = currentTime + tts;
						return tts;
					}
					m_round = 0;
					m_expectedAwake = currentTime;
					return 0;
				}
				ThreadComState::State CLOAK_CALL_THIS ThreadComState::Check()
				{
					if (m_level == 0) { return FINISHED; }
					if (g_canComCheck == false)
					{
						m_reduce = false;
						m_level = MAX_LEVEL;
					}
					else
					{
						const size_t tc = 2 + g_threadCount; //the +2 is for #1 current thread and #2 game core loop
						const uint8_t ncc = static_cast<uint8_t>((g_canComCount + tc - 1) / tc);
						if (ncc < m_level)
						{
							g_currentComLevel = ncc;
							m_level = ncc;
							m_reduce = true;
							return UPDATED;	
						}
					}
					return WAITING;
				}
				void CLOAK_CALL_THIS ThreadComState::Update()
				{
					if (m_reduce == true)
					{
						m_reduce = false;
						g_canComCount--;
					}
				}
				uint8_t CLOAK_CALL_THIS ThreadComState::GetCurrentLevel() const { return m_level; }
				unsigned long long CLOAK_CALL getCurrentTimeMilliSeconds()
				{
					if (g_freqInit == true)
					{
						LARGE_INTEGER now;
						LARGE_INTEGER freq = g_freq;
						QueryPerformanceCounter(&now);
						now.QuadPart -= g_fof.load().QuadPart;
						return (1000LL*now.QuadPart) / freq.QuadPart;
					}
					return GetTickCount64() - g_tof;
				}
				unsigned long long CLOAK_CALL getCurrentTimeMicroSeconds()
				{
					if (g_freqInit == true)
					{
						LARGE_INTEGER now;
						LARGE_INTEGER freq = g_freq;
						QueryPerformanceCounter(&now);
						now.QuadPart -= g_fof.load().QuadPart;
						return (1000000LL * now.QuadPart) / freq.QuadPart;
					}
					return (GetTickCount64() - g_tof) * 1000;
				}
#ifdef _DEBUG
				Ret_notnull Engine::Thread::Thread* CLOAK_CALL createThread(In const Engine::Thread::ThreadDesc& desc, In_opt std::string label = "")
				{
#else
				Ret_notnull Engine::Thread::Thread* CLOAK_CALL createThread(In const Engine::Thread::ThreadDesc& desc)
				{
#endif
					CLOAK_ASSUME(g_threadCount + 1 < API::Global::THREAD_COUNT);
#ifdef _DEBUG
					Engine::Thread::Thread* p = new Engine::Thread::CustomThread(desc, static_cast<unsigned int>(g_threadCount), label);
#else
					Engine::Thread::Thread* p = new Engine::Thread::CustomThread(desc, static_cast<unsigned int>(g_threadCount));
#endif
					if (p != nullptr)
					{
						g_threads[g_threadCount++] = p;
						p->Start();
					}
					return p;
				}

				std::string CLOAK_CALL getModulePath()
				{
					API::Helper::Lock lock(g_syncModulPath);
					if (g_hasModulPath == false)
					{
						size_t copied = 0;
						API::List<wchar_t> buf;
						do {
							buf.resize(buf.size() + MAX_PATH);
							copied = GetModuleFileName(NULL, &buf.at(0), (DWORD)buf.size());
						} while (copied >= buf.size());

						buf.resize(copied);
						std::string modName(API::Helper::StringConvert::ConvertToU8(std::wstring(buf.begin(), buf.end())));
						const size_t lbs = modName.find_last_of('\\');
						const size_t ls = modName.find_last_of('/');
						std::string modPath = modName.substr(0, lbs == modName.npos ? ls : (ls == modName.npos ? lbs : max(lbs, ls)));
						g_modulPath = new std::string(modPath);
						g_hasModulPath = true;
					}
					return *g_modulPath;
				}
				uint8_t CLOAK_CALL GetCurrentComLevel() { return g_currentComLevel; }

				DWORD WINAPI runEngineAsync(LPVOID p)
				{
					AsyncEngineInfo* info = reinterpret_cast<AsyncEngineInfo*>(p);
					API::Global::Game::StartEngine(info->Factory, info->Info);
					delete info;
					return 0;
				}
			}
		}
	}
	namespace API {
		namespace Global {
			namespace Game {
				constexpr uint32_t VERSION = CLOAKENGINE_VERSION;

				CLOAKENGINE_API bool CLOAK_CALL CheckVersion(In_opt uint32_t version) { return VERSION == version; }
				CLOAKENGINE_API void CLOAK_CALL StartEngine(In IGameEventFactory* factory, In const GameInfo& info)
				{
					if (factory == nullptr) { return; }
#ifdef _DEBUG
					int crtflag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
					_CrtSetDbgFlag((crtflag | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF) & ~(_CRTDBG_CHECK_ALWAYS_DF));
#endif
					SetErrorMode(SEM_NOOPENFILEERRORBOX);

					//Initialisation of managed memory:
					Impl::OS::Initialize();
					Impl::Global::Memory::OnStart();
					Impl::Global::Log::Initialize();
					Impl::Global::Debug::Initialize();
					Impl::OS::PrintCPUInformation();

					IGameEvent* game = factory->CreateGame();
					IInputEvent* input = factory->CreateInput();
					ILauncherEvent* launcher = factory->CreateLauncher();
					ILobbyEvent* lobby = factory->CreateLobby();
					IDebugEvent* dbg = nullptr;
					if (info.debugMode) { dbg = factory->CreateDebug(); }

					Impl::Global::DebugLayer* dbgLayer = nullptr;
#ifdef _DEBUG
					Impl::Global::Game::g_ptrs = new Engine::LinkedList<API::Helper::ISavePtr*>();
#endif
					if (CloakCheckOK(game != nullptr && input != nullptr, Debug::Error::GAME_NO_EVENT_HANDLER, true))
					{
						Impl::Global::Game::g_onStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

						Impl::Global::Input::SetInputHandler(input);
						Impl::Global::Lobby::SetLobbyEventHandler(lobby);
						Impl::Global::Debug::setDebugEvent(dbg);

						// VVV Initialize time
						LARGE_INTEGER freq;
						Impl::Global::Game::g_freqInit = (QueryPerformanceFrequency(&freq) == TRUE);
						Impl::Global::Game::g_freq = freq;
						if (Impl::Global::Game::g_freqInit)
						{
							LARGE_INTEGER now;
							QueryPerformanceCounter(&now);
							Impl::Global::Game::g_fof = now;
						}
						else { Impl::Global::Game::g_tof = GetTickCount64(); }

						// VVV Initialize
						Impl::Global::Game::g_running = true;
						Impl::Global::Game::g_debugMode = info.debugMode;
						Impl::Global::Game::g_hasWindow = info.useWindow;
						const bool useConsole = info.debugMode || info.useConsole || info.useWindow == false;
						Impl::Helper::Lock::Initiate();
						CREATE_INTERFACE(CE_QUERY_ARGS(&Impl::Global::Game::g_syncPtrs));
						CREATE_INTERFACE(CE_QUERY_ARGS(&Impl::Global::Game::g_syncModulPath));
#ifdef _DEBUG
						CREATE_INTERFACE(CE_QUERY_ARGS(&Impl::Global::Game::g_syncResponde));
						Impl::Global::Game::g_threadRespond = new Engine::FlipVector<Impl::Global::Game::ThreadInfo>();
#endif
						if (IsDebugMode()) 
						{ 
							CREATE_INTERFACE(CE_QUERY_ARGS(&dbgLayer)); 
							Impl::Global::Game::g_dbgLayer = dbgLayer;
						}
						Impl::Global::Task::IRunWorker* taskWorker = Impl::Global::Task::Initialize(info.useWindow);
						Impl::World::ComponentManager::Initialize();
						Impl::World::ComponentManager_v2::ComponentAllocator::Initialize();
						Impl::World::Entity_v2::Initialize();
						Impl::Files::Buffer_v1::Initialize();
						Impl::Files::FileIO_v1::Initialize();
						Impl::Files::Compressor_v2::Initialize();
						Impl::Files::InitializeFileHandler();
						Engine::Compress::NetBuffer::SecureNetBuffer::Initialize();
						Engine::WorldStreamer::Initialize();
						Engine::Audio::Core::Start();
						Impl::Files::Image::Initialize();
						Impl::Global::Game::ThreadSleepState tsi;

						if (Engine::Launcher::StartLauncher(launcher, &tsi) && Impl::Global::Game::g_run == true)
						{
							Impl::Global::Game::registerThread();
							//VVV Initialize and start all required threads
							//Log
							{
								Engine::Thread::ThreadDesc threadDesc;
								threadDesc.Functions.Start[0] = Impl::Global::Log::onStart;
								threadDesc.Functions.Start[2] = threadDesc.Functions.Start[3] = threadDesc.Functions.Start[4] = [=](In size_t threadID) {Impl::Global::Log::onUpdate(threadID, 0); };
								threadDesc.Functions.Update = Impl::Global::Log::onUpdate;
								threadDesc.Functions.Stop[1] = Impl::Global::Log::onStop;
								threadDesc.TargetFPS = 30;
								threadDesc.FPSFactor = 1;
								threadDesc.Flags = Engine::Thread::FLAG_LOCK_FPS | Engine::Thread::FLAG_IGNORE_PAUSE | Engine::Thread::FLAG_NO_RESPONSE_CHECK | Engine::Thread::FLAG_IMPORTANT;
								CREATE_THREAD(threadDesc, "Log");
							}
							if (useConsole)
							{
								Engine::Thread::ThreadDesc threadDesc;
								threadDesc.Functions.Start[0] = Impl::Global::Debug::enableConsole;
								threadDesc.Functions.Update = Impl::Global::Debug::updateConsole;
								threadDesc.Functions.Stop[1] = Impl::Global::Debug::releaseConsole;
								threadDesc.FPSFactor = 1;
								threadDesc.TargetFPS = 30;
								threadDesc.Flags = Engine::Thread::FLAG_LOCK_FPS | Engine::Thread::FLAG_IGNORE_PAUSE | Engine::Thread::FLAG_NO_RESPONSE_CHECK;
								CREATE_THREAD(threadDesc, "Console");
							}
							if (info.useWindow)
							{
								//Lobby
								{
									Engine::Thread::ThreadDesc threadDesc;
									threadDesc.Functions.Start[4] = Impl::Global::Lobby::OnStart;
									threadDesc.Functions.Update = Impl::Global::Lobby::OnUpdate;
									threadDesc.Functions.Stop[0] = Impl::Global::Lobby::OnStop;
									threadDesc.TargetFPS = Impl::Global::Game::DEFAULT_FPS;
									threadDesc.FPSFactor = 1;
									threadDesc.Flags = Engine::Thread::FLAG_IGNORE_PAUSE | Engine::Thread::FLAG_NO_RESPONSE_CHECK;
									CREATE_THREAD(threadDesc, "Lobby");
								}
								//Input
								{
									Engine::Thread::ThreadDesc threadDesc;
									threadDesc.Functions.Start[3] = Impl::Global::Input::onLoad;
									threadDesc.Functions.Update = Impl::Global::Input::onUpdate;
									threadDesc.Functions.Stop[0] = Impl::Global::Input::onStop;
									threadDesc.TargetFPS = Impl::Global::Game::DEFAULT_FPS;
									threadDesc.FPSFactor = 2;
									threadDesc.Flags = Engine::Thread::FLAG_NONE;
									CREATE_THREAD(threadDesc, "Input");
								}
							}
							//Physics
							{
								Engine::Thread::ThreadDesc threadDesc;
								threadDesc.Functions.Start[1] = Engine::Physics::Start;
								threadDesc.Functions.Update = Engine::Physics::Update;
								threadDesc.Functions.Stop[0] = Engine::Physics::Release;
								threadDesc.TargetFPS = Impl::Global::Game::DEFAULT_FPS;
								threadDesc.FPSFactor = 1;
								threadDesc.Flags = Engine::Thread::FLAG_PAUSE_ON_LOADING;
								CREATE_THREAD(threadDesc, "Physics");
							}
							Engine::Game::Initialize(info.useWindow, Impl::Global::Game::DEFAULT_FPS, game);
							//TODO: AAA Initialize and start all required threads
#ifdef _DEBUG
							{
								API::Helper::Lock rlock(Impl::Global::Game::g_syncResponde);
								Impl::Global::Game::g_threadRespond->resize(Impl::Global::Game::g_threadCount);
								Impl::Global::Game::ThreadInfo defInfo;
								defInfo.lastUpdate = Impl::Global::Game::getCurrentTimeMilliSeconds();
								defInfo.checkUpdate = true;
								Impl::Global::Game::g_threadRespond->setAll(defInfo);
							}
#endif
							Impl::Global::Game::g_canComCheck = true;
							Impl::Interface::Initialize();
							Impl::Global::Graphic::Initialize();
							Impl::Global::Localization::Initialize();
							Impl::Global::Mouse::Initiate();
							Impl::Global::Audio::Initialize();
							Impl::Global::Input::OnInitialize();
							Engine::FileLoader::onStart();
							Impl::Global::Game::ThreadComState canCom;
							unsigned long long lastUpdate = 0;
							Impl::Global::Game::ThreadFPSState tfi;
							Log::WriteToLog("START GAME", Log::Type::Info);
#ifdef _DEBUG
							bool canCheck = false;
#endif
							// Initializing
							while (Impl::Global::Game::g_run == true)
							{
								const unsigned long long ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
								const unsigned long long ct = ctm / 1000;
								const Impl::Global::Game::ThreadComState::State ccs = canCom.Check();
								if (ccs == Impl::Global::Game::ThreadComState::UPDATED)
								{
									const uint8_t nCC = canCom.GetCurrentLevel();
									const BYTE startCount = (Impl::Global::Game::ThreadComState::MAX_LEVEL - 1) - nCC;
									if (startCount == 0)
									{
#ifdef _DEBUG
										canCheck = true;
#endif
										Engine::Graphic::Core::InitPipeline();
										if (info.useWindow) { Engine::WindowHandler::onInit(); }
									}
									else if (startCount == 1)
									{
										Impl::Global::Steam::Initialize(info.useSteam);
										Engine::Graphic::Core::Load();
									}
									else if (startCount == 2)
									{
										Impl::Global::Steam::RequestData();
										if (info.useWindow) { Engine::WindowHandler::onLoad(); }
									}
									else if (startCount == 3)
									{
										if (info.useWindow) { Engine::Graphic::Core::Start(); }
									}
									canCom.Update();
								}
								else if (ccs == Impl::Global::Game::ThreadComState::FINISHED)
								{
									Impl::Global::Game::g_fps = tfi.Update(ctm);
									lastUpdate = ctm;
									Sleep(static_cast<DWORD>(tsi.GetSleepTime(ctm, Impl::Global::Game::g_sleepTime) / 1000));
									goto game_running;
								}
#ifdef _DEBUG
								if (canCheck) { Impl::Global::Game::checkResponse(ct); }
#endif
								Impl::Global::Game::g_fps = tfi.Update(ctm);
								lastUpdate = ctm;
								Sleep(static_cast<DWORD>(tsi.GetSleepTime(ctm, Impl::Global::Game::g_sleepTime) / 1000));
							}
							goto game_shutdown;

						game_running:
							Engine::FileLoader::onStartLoading();
							if (info.useWindow) { Engine::WindowHandler::showWindow(); }
							//Core loop:
							while (Impl::Global::Game::g_run == true)
							{
								const unsigned long long ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
								const unsigned long long ct = ctm / 1000;
								const unsigned long long etime = (lastUpdate < ctm ? ctm - lastUpdate : 0) / 1000;

#ifdef _DEBUG
								Impl::Global::Game::checkResponse(ct);
#endif
								Impl::Global::Steam::Update();
								if (info.useWindow) 
								{ 
									Engine::Graphic::Core::Update(etime);

									Impl::Global::Game::g_fps = tfi.Update(ctm);
									lastUpdate = ctm;
									const CE::Global::Time sleepTime = tsi.GetSleepTime(ctm, Impl::Global::Game::g_sleepTime);
									if (sleepTime > 0)
									{
										CE::Global::Time curTime = Impl::Global::Game::getCurrentTimeMicroSeconds();
										const CE::Global::Time targetTime = curTime + sleepTime;
										do {
											if (Engine::WindowHandler::UpdateOnce() == false && Engine::WindowHandler::WaitForMessage((targetTime - curTime) / 1000) == true) { break; }
											curTime = Impl::Global::Game::getCurrentTimeMicroSeconds();
										} while (curTime < targetTime);
									}
								}
								else
								{
									taskWorker->RunWorkOrWait();
								}
							}

						game_shutdown:
							Log::WriteToLog("STOP GAME", Log::Type::Info);
							Engine::Audio::Core::Shutdown();
							Engine::WindowHandler::onStop();
							//Stopping threads
							Impl::Global::Game::registerThreadStop();
							Engine::Game::SignalStop();
							const size_t tc = Impl::Global::Game::g_threadCount;
							for (size_t a = 0; a < tc; a++) { Impl::Global::Game::g_threads[a]->Awake(); }

							while (!Impl::Global::Game::canThreadStop())
							{
								const unsigned long long ctm = Impl::Global::Game::getCurrentTimeMicroSeconds();
								const unsigned long long ct = ctm / 1000;
								for (size_t a = 0; a < Impl::Global::Game::g_threadCount; a++)
								{
									Engine::Thread::Thread* p = Impl::Global::Game::g_threads[a];
									if (p != nullptr && p->IsRunning()) { p->Stop(); }
								}
								Sleep(static_cast<DWORD>(tsi.GetSleepTime(ctm, Impl::Global::Game::DEFAULT_SLEEP_TIME) / 1000));
							}
						}
						else { Stop(); }//Launcher abort
#ifdef _DEBUG
						delete Impl::Global::Game::g_threadRespond;
#endif
					}

					//Release stuff that may trigger some external tasks
					Engine::WorldStreamer::Release();

					//Release stuff that requires (and waits for) some tasks to finish:
					Impl::World::ComponentManager_v2::ComponentAllocator::Shutdown();
					Engine::Audio::Core::Release();
					Engine::FileLoader::waitOnLoading();

					//Stop all task execution:
					Impl::Global::Task::Release();

					//Delete threads
					for (size_t a = 0; a < Impl::Global::Game::g_threadCount; a++)
					{
						Engine::Thread::Thread* p = Impl::Global::Game::g_threads[a];
						delete p;
					}

					//Release other stuff:
					Impl::Files::Image::Shutdown();
					Impl::Global::Video::Release();
					Impl::Global::Mouse::Release();
					Impl::Global::Localization::Release();
					Impl::Global::Graphic::Release();
					Impl::Interface::Release();
					Impl::Global::Steam::Release();
					Impl::Global::Input::OnRelease();

					Impl::Global::Audio::Release();
					Engine::Audio::Core::FinalRelease();
					Engine::Graphic::Core::FinalRelease();
					Impl::World::ComponentManager::ReleaseSyncs();
					Impl::World::ComponentManager_v2::ComponentAllocator::ReleaseSyncs();
					Impl::World::Entity_v2::ReleaseSyncs();
					Impl::Files::FileIO_v1::CloseAllFiles();

					//Delete task-system threads
					Impl::Global::Task::Finish();

					//Release file system
					Engine::FileLoader::onStop();
					Impl::Files::FileIO_v1::Release();
					Impl::Files::Buffer_v1::Release();
					Engine::Compress::NetBuffer::SecureNetBuffer::Terminate();
					Impl::Files::Compressor_v2::Release();
					Impl::Files::TerminateFileHandler();

					SAVE_RELEASE(Impl::Global::Game::g_syncPtrs);
					SAVE_RELEASE(Impl::Global::Game::g_syncModulPath);
#ifdef _DEBUG
					SAVE_RELEASE(Impl::Global::Game::g_syncResponde);
#endif
					SAVE_RELEASE(dbgLayer);

					Impl::Helper::Lock::Release();
#ifdef _DEBUG
					//Delete un-released save-ptr:
					if (IsDebugMode() && Impl::Global::Game::g_ptrs->size() > 0)
					{
						Log::WriteToLog("Non-released ptrs:\tName:\t\t\t\tRefCounts:\tFile:", Log::Type::Warning);
						for (auto a = Impl::Global::Game::g_ptrs->begin(); a != Impl::Global::Game::g_ptrs->end(); a++)
						{
							Helper::ISavePtr* p = *a;
							std::stringstream ad;
							ad << "\t\t" << "0x" << std::setfill('0') << std::setw(sizeof(p) * 2) << std::hex << p << "\t\t  ";
							const std::string name = p->GetDebugName();
							ad << name;
							for (size_t l = 16; l > name.length() + 2 && l > 0; l -= 4) { ad << "\t"; }
							const std::string rc = std::to_string(p->GetRefCount());
							const size_t sl = name.length() > 14 ? name.length() - 14 : 0;
							const size_t ml = rc.length() > 13 ? 0 : (13 - rc.length());
							const size_t sc = ml > sl ? ml - sl : 0;
							for (size_t b = 0; b < sc; b++) { ad << " "; }
							ad << rc << "\t" << p->GetSourceFile();
							Log::WriteToLog(ad.str(), Log::Type::Warning);
						}
					}
#endif
					Impl::Global::Game::g_running = false;
					Debug::Error le = Impl::Global::Debug::getLastError();
					if (le != Debug::Error::OK && game != nullptr) { game->OnCrash(le); }
					Impl::Global::Log::onUpdate(0, 0);
#ifdef _DEBUG
					for (auto a = Impl::Global::Game::g_ptrs->begin(); a != Impl::Global::Game::g_ptrs->end(); a++) { (*a)->Delete(); }
					delete Impl::Global::Game::g_ptrs;
#endif
					Impl::OS::Terminate();
					//Release managed memory elements
					Impl::World::ComponentManager::Release();
					Impl::World::ComponentManager_v2::ComponentAllocator::Terminate();
					Impl::World::Entity_v2::Terminate();
					Impl::Global::Debug::Release();
					Impl::Global::Log::Release();
					if (Impl::Global::Game::g_hasModulPath) { delete Impl::Global::Game::g_modulPath; }

					if (lobby != nullptr) { lobby->Delete(); }
					if (launcher != nullptr) { launcher->Delete(); }
					if (input != nullptr) { input->Delete(); }
					if (game != nullptr) { game->Delete(); }
					if (dbg != nullptr) { dbg->Delete(); }

					//Release memory management
					Impl::Global::Memory::OnStop();

					SetEvent(Impl::Global::Game::g_onStopEvent);
				}
				CLOAKENGINE_API void CLOAK_CALL StartEngineAsync(In IGameEventFactory* factory, In const GameInfo& info)
				{
					Impl::Global::Game::AsyncEngineInfo* ai = new Impl::Global::Game::AsyncEngineInfo();
					ai->Factory = factory;
					ai->Info = info;
					CreateThread(nullptr, 0, Impl::Global::Game::runEngineAsync, ai, 0, nullptr);
				}
				CLOAKENGINE_API void CLOAK_CALL StartEngine(In IGameEventFactory* factory, In const GameInfo& info, In HWND window)
				{
					Engine::WindowHandler::setExternWindow(window);
					return StartEngine(factory, info);
				}
				CLOAKENGINE_API void CLOAK_CALL StartEngineAsync(In IGameEventFactory* factory, In const GameInfo& info, In HWND window)
				{
					Engine::WindowHandler::setExternWindow(window);
					return StartEngineAsync(factory, info);
				}
				CLOAKENGINE_API bool CLOAK_CALL IsDebugMode() 
				{
#ifdef _DEBUG
					return true;
#else
					return Impl::Global::Game::g_debugMode; 
#endif
				}
				CLOAKENGINE_API bool CLOAK_CALL HasWindow() { return Impl::Global::Game::g_hasWindow; }
				CLOAKENGINE_API void CLOAK_CALL Stop() { Impl::Global::Game::g_run = false; }
				CLOAKENGINE_API void CLOAK_CALL WaitForStop()
				{
					WaitForSingleObject(Impl::Global::Game::g_onStopEvent, INFINITE);
				}
				CLOAKENGINE_API void CLOAK_CALL SetFPS(In_opt float fps) 
				{
					if (CloakDebugCheckOK(fps >= 0.0f, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						Impl::Global::Game::g_sleepTime = fps <= 0 ? 0 : (1000000 / fps);
						Engine::Game::SetFPS(fps);
						for (size_t a = 0; a < Impl::Global::Game::g_threadCount; a++)
						{
							Engine::Thread::Thread* p = Impl::Global::Game::g_threads[a];
							p->SetFPS(fps);
						}
					}
				}
				CLOAKENGINE_API float CLOAK_CALL GetFPS(Out_opt FPSInfo* info)
				{
					float r = Engine::Game::GetFPS();
					if (info != nullptr)
					{
						ZeroMemory(info, sizeof(FPSInfo));
						info->Thread[0].FPS = r;
						info->Thread[1].FPS = Impl::Global::Game::g_fps;
						for (size_t a = 0; a < Impl::Global::Game::g_threadCount; a++)
						{
							CLOAK_ASSUME(a + 2 < ARRAYSIZE(info->Thread));
							info->Thread[a + 2].FPS = Impl::Global::Game::g_threads[a]->GetFPS();
						}
						info->Count = Impl::Global::Game::g_threadCount + 2;
					}
					return r;
				}
				CLOAKENGINE_API bool CLOAK_CALL IsRunning() { return Impl::Global::Game::g_running; }

#define CHECK_CLASS_BEGIN(cRiid,iRiid,ptr,svPtr,cErr) { REFIID _cRiid=cRiid, _iRiid=iRiid; void** _ptr=ptr; Impl::Helper::SavePtr** _svPtr=&svPtr; Debug::Error* _err=&cErr;
#define CHECK_CLASS_DETAILED(path, nameAPI, nameImpl, ...) if(_cRiid==__uuidof(API::path::nameAPI) || _cRiid==__uuidof(Impl::path::nameImpl)) \
{\
	*_svPtr=new Impl::path::nameImpl(__VA_ARGS__);\
	if(*_svPtr==nullptr){*_err=Debug::Error::GAME_MEMORY;}\
	else if(SUCCEEDED((*_svPtr)->QueryInterface(_iRiid,_ptr)))\
	{*_err=Debug::Error::OK;}\
	else {*_err=Debug::Error::NO_INTERFACE;}\
} else
#define CHECK_CLASS(path,name,...) CHECK_CLASS_DETAILED(path,I##name,name,__VA_ARGS__)
#define CHECK_CLASS_END() {*_err=Debug::Error::NO_CLASS;}}

				inline Debug::Error CLOAK_CALL CreateInterface(In REFIID classRiid, In REFIID interfRiid, Outptr Impl::Helper::SavePtr** res, Outptr void** ppvObject)
				{
					Debug::Error e = Debug::Error::GAME_MEMORY;
					if (res != nullptr) { *res = nullptr; }
					if (Impl::Global::Game::g_running == true && ppvObject != nullptr)
					{
						Impl::Helper::SavePtr* p = nullptr;
						*ppvObject = nullptr;

						CHECK_CLASS_BEGIN(classRiid, interfRiid, ppvObject, p, e)
							CHECK_CLASS(Files::Compressor_v2, Reader)
							CHECK_CLASS(Files::Compressor_v2, Writer)
							CHECK_CLASS(Files::Configuration_v1, Configuration)
							CHECK_CLASS(Files::Scene_v1, GeneratedScene)
							CHECK_CLASS_DETAILED(Files::Language_v2, IString, StringProxy)
							CHECK_CLASS(Helper::SavePtr_v1, SavePtr)
							CHECK_CLASS(Helper::SyncSection_v1, SyncSection)
							CHECK_CLASS(Helper::SyncSection_v1, ConditionSyncSection)
							CHECK_CLASS(Interface::BasicGUI_v1, BasicGUI)
							CHECK_CLASS(Interface::Button_v1, Button)
							CHECK_CLASS(Interface::CheckButton_v1, CheckButton)
							CHECK_CLASS(Interface::DropDownMenu_v1, DropDownMenu)
							CHECK_CLASS(Interface::EditBox_v1, EditBox)
//							CHECK_CLASS(Interface::EditBox_v1, MultiLineEditBox)
							CHECK_CLASS(Interface::Frame_v1, Frame)
							CHECK_CLASS(Interface::Label_v1, Label)
							CHECK_CLASS(Interface::Slider_v1, Slider)
							CHECK_CLASS(Interface::ProgressBar_v1, ProgressBar)
							CHECK_CLASS(Interface::LoadScreen_v1, LoadScreen)
							CHECK_CLASS(Interface::ListMenu_v1, ListMenu)
							CHECK_CLASS(Interface::MoveableFrame_v1, MoveableFrame)
							CHECK_CLASS(World::Entity_v1, Entity)
						CHECK_CLASS_END()
						if (p != nullptr)
						{
							p->Initialize();
							if (res != nullptr) { *res = p; }
							else { p->Release(); }
						}
						if (e == Debug::Error::GAME_MEMORY) { Log::WriteToLog("Memory allocation failed!", Log::Type::Warning); }
					}
					return e;
				}


				CLOAKENGINE_API Debug::Error CLOAK_CALL CreateInterface(In REFIID riid, Outptr void** ppvObject)
				{
					return CreateInterface(riid, riid, nullptr, ppvObject);
				}
				CLOAKENGINE_API Debug::Error CLOAK_CALL CreateInterface(In REFIID classRiid, In REFIID interfRiid, Outptr void** ppvObject)
				{
					return CreateInterface(classRiid, interfRiid, nullptr, ppvObject);
				}
				CLOAKENGINE_API Debug::Error CLOAK_CALL CreateInterface(In const std::string& file, In REFIID riid, Outptr void __RPC_FAR* __RPC_FAR* ppvObject)
				{
					return CreateInterface(file, riid, riid, ppvObject);
				}
				CLOAKENGINE_API Debug::Error CLOAK_CALL CreateInterface(In const std::string& file, In REFIID classRiid, In REFIID interfRiid, Outptr void** ppvObject)
				{
					Impl::Helper::SavePtr* r = nullptr;
					Debug::Error res = CreateInterface(classRiid, interfRiid, &r, ppvObject);
					if (r != nullptr)
					{
						r->SetSourceFile(file);
						r->Release();
					}
					return res;
				}
#undef CHECK_CLASS
#undef CHECK_CLASS_BEGIN
#undef CHECK_CLASS_END
				CLOAKENGINE_API Debug::Error CLOAK_CALL GetDebugLayer(In REFIID riid, Outptr void** ppvObject)
				{
					*ppvObject = nullptr;
					if (IsDebugMode())
					{
						Impl::Global::DebugLayer* l = Impl::Global::Game::g_dbgLayer;
						HRESULT hRet = l->QueryInterface(riid, ppvObject);
						if (FAILED(hRet)) { return Debug::Error::NO_INTERFACE; }
						return Debug::Error::OK;
					}
					return Debug::Error::ILLEGAL_DISABLED;
				}
				CLOAKENGINE_API void CLOAK_CALL Pause()
				{
					Impl::Global::Game::TimeInfo lI = Impl::Global::Game::g_pauseTime;
					if (lI.Paused == false)
					{
						Impl::Global::Game::TimeInfo nI;
						do {
							nI = lI;
							if (nI.Paused == false)
							{
								nI.Paused = true;
								nI.LastPaused = GetTimeStamp();
							}
						} while (!Impl::Global::Game::g_pauseTime.compare_exchange_strong(lI, nI));
					}
				}
				CLOAKENGINE_API bool CLOAK_CALL IsPaused()
				{
					return Impl::Global::Game::g_pauseTime.load().Paused;
				}
				CLOAKENGINE_API void CLOAK_CALL Resume()
				{
					Impl::Global::Game::TimeInfo lI = Impl::Global::Game::g_pauseTime;
					if (lI.Paused == true)
					{
						Impl::Global::Game::TimeInfo nI;
						do {
							nI = lI;
							if (nI.Paused == true)
							{
								nI.Paused = false;
								nI.Offset += GetTimeStamp() - nI.LastPaused;
							}
						} while (!Impl::Global::Game::g_pauseTime.compare_exchange_strong(lI, nI));
					}
				}
				CLOAKENGINE_API Time CLOAK_CALL GetGameTimeStamp()
				{
					return Impl::Global::Game::getCurrentTimeMilliSeconds() - (Impl::Global::Game::g_pauseTime.load().Offset + Impl::Global::Game::g_runTime.load().Offset);
				}
				CLOAKENGINE_API Time CLOAK_CALL GetTimeStamp()
				{
					return Impl::Global::Game::getCurrentTimeMilliSeconds() - Impl::Global::Game::g_runTime.load().Offset;
				}
				CLOAKENGINE_API Date CLOAK_CALL GetLocalDate()
				{
					SYSTEMTIME st;
					GetLocalTime(&st);
					return Date(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				}
				CLOAKENGINE_API Date CLOAK_CALL GetUTCDate()
				{
					SYSTEMTIME st;
					GetSystemTime(&st);
					return Date(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
				}
			}
		}
	}
}