#pragma once
#ifndef CE_E_THREAD_H
#define CE_E_THREAD_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Task.h"
#include "Implementation/Helper/SavePtr.h"

#include <functional>
#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace Thread {
			constexpr size_t START_FUNC_COUNT = Impl::Global::Game::ThreadComState::MAX_LEVEL - 1;
			constexpr size_t STOP_FUNC_COUNT = 2;
			enum FLAGS {
				FLAG_NONE = 0,
				FLAG_LOCK_FPS = 1 << 0,
				FLAG_IGNORE_PAUSE = 1 << 1,
				FLAG_NO_RESPONSE_CHECK = 1 << 2,
				FLAG_PAUSE_ON_LOADING = 1 << 3,
				FLAG_IMPORTANT = 1 << 4,
			};
			DEFINE_ENUM_FLAG_OPERATORS(FLAGS);
			struct ThreadDesc {
				struct {
					std::function<void(In size_t threadID)> Start[START_FUNC_COUNT];
					std::function<void(In size_t threadID, API::Global::Time etime)> Update;
					std::function<void(In size_t threadID)> Stop[STOP_FUNC_COUNT];
				} Functions;
				float TargetFPS = 0;
				float FPSFactor = 1;
				FLAGS Flags = FLAG_NONE;
			};
			class Thread {
				public:
#ifdef _DEBUG
					CLOAK_CALL Thread(In float targetFPS, In uint32_t id, In_opt bool lockFPS = false, In_opt std::string label = "");
#else
					CLOAK_CALL Thread(In float targetFPS, In uint32_t id, In_opt bool lockFPS = false);
#endif
					virtual CLOAK_CALL ~Thread();

					virtual void CLOAK_CALL_THIS Awake() = 0;
					virtual void CLOAK_CALL_THIS Start() = 0;
					void CLOAK_CALL_THIS Stop();
					void CLOAK_CALL_THIS SetFPS(In float fps);

					bool CLOAK_CALL_THIS IsPaused() const;
					bool CLOAK_CALL_THIS IsRunning() const;
					uint32_t CLOAK_CALL_THIS GetID() const;
					float CLOAK_CALL_THIS GetFPS() const;
				protected:
					const bool m_lockFPS;
					const uint32_t m_id;
#ifdef _DEBUG
					const std::string m_label;
#endif
					Impl::Global::Game::ThreadSleepState m_tsi;
					Impl::Global::Game::ThreadFPSState m_tfi;
					Impl::Global::Game::ThreadComState m_canCom;
					std::atomic<bool> m_run;
					std::atomic<bool> m_running;
					std::atomic<bool> m_paused;
					std::atomic<bool> m_finished;
					std::atomic<bool> m_inactive;
					std::atomic<float> m_sleepTime;
					std::atomic<float> m_fps;
					API::Global::Time m_lastUpdate;
			};
			class CustomThread : public Thread {
				private:
					std::function<void(In size_t threadID)> m_start[START_FUNC_COUNT];
					std::function<void(In size_t threadID, API::Global::Time etime)> m_update;
					std::function<void(In size_t threadID)> m_stop[STOP_FUNC_COUNT];
					const FLAGS m_flags;
					const float m_fpsFactor;
					const bool m_important;
					API::Global::Task m_curTask;
				public:
#ifdef _DEBUG
					CLOAK_CALL CustomThread(In const ThreadDesc& desc, In uint32_t id, In_opt std::string label = "");
#else
					CLOAK_CALL CustomThread(In const ThreadDesc& desc, In uint32_t id);
#endif
					CLOAK_CALL ~CustomThread();
					void CLOAK_CALL_THIS Awake() override;
					void CLOAK_CALL_THIS Start() override;
					
					void CLOAK_CALL_THIS runInitialize(In size_t threadID);
					void CLOAK_CALL_THIS runUpdate(In size_t threadID);
					void CLOAK_CALL_THIS runStop(In size_t threadID);
			};
			namespace ThreadLoops {
				//Handles Threads:
				//	- Input
				//	- AI
				//	- Game
				//	- Animation
				//	- Physics
				//	- Rendering
				class RenderLoopThread : public Thread {
					private:
						static constexpr API::Global::Time PHYSICS_UPDATE_DURATION = 10000;
						static constexpr API::Global::Time MAX_PHYSICS_DELAY = PHYSICS_UPDATE_DURATION << 4;
						static constexpr CE::Global::Threading::Priority SIMULATION_IMPORTANT = CE::Global::Threading::Priority::High;
						static constexpr CE::Global::Threading::Priority RENDERING_IMPORTANT = CE::Global::Threading::Priority::High;

						API::Global::Task m_readInput;
						API::Global::Task m_updateAI;
						API::Global::Task m_updateGame;
						API::Global::Task m_readAnimation;
						API::Global::Task m_updateRootMotion;
						API::Global::Task m_updateAnimation;
						API::Global::Task m_updatePosition;
						API::Global::Task m_processAI;
						API::Global::Task m_updateVisibility;
						API::Global::Task m_mirrorGameState;
						API::Global::Task m_renderSubmit;
						API::Global::Task m_renderPresent;

						API::Global::Time m_curTime;
						API::Global::Time m_lastTime;
					public:
						CLOAK_CALL RenderLoopThread(In uint32_t id, In float targetFPS);
						CLOAK_CALL ~RenderLoopThread();

						void CLOAK_CALL_THIS Awake() override;
						void CLOAK_CALL_THIS Start() override;

						void CLOAK_CALL_THIS runInitialize(In size_t threadID);
						void CLOAK_CALL_THIS runStop(In size_t threadID);

						void CLOAK_CALL_THIS runReadInput(In size_t threadID);
						void CLOAK_CALL_THIS runUpdateAI(In size_t threadID);
						void CLOAK_CALL_THIS runUpdateGame(In size_t threadID);
						void CLOAK_CALL_THIS runProcessAI(In size_t threadID);
						void CLOAK_CALL_THIS runUpdatePosition(In size_t threadID);
						void CLOAK_CALL_THIS runReadAnimation(In size_t threadID);
						void CLOAK_CALL_THIS runUpdateRootMotion(In size_t threadID);
						void CLOAK_CALL_THIS runUpdateAnimation(In size_t threadID);
						void CLOAK_CALL_THIS runUpdatePhysics(In size_t threadID, In API::Global::Time startTime);
						void CLOAK_CALL_THIS runUpdateVisibility(In size_t threadID);
						void CLOAK_CALL_THIS runMirrorGameState(In size_t threadID);
						void CLOAK_CALL_THIS runRenderSubmit(In size_t threadID);
						void CLOAK_CALL_THIS runRenderPresent(In size_t threadID);
				};
			}
		}
	}
}

#endif