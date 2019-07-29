#include "stdafx.h"
#include "Engine\Graphic\WorkerThread.h"
#include "Engine\Graphic\Core.h"

#include "Implementation\Rendering\Context.h"
#include "Implementation\Global\Game.h"

#include "CloakEngine\Global\Debug.h"

#define MAX_WAIT 500

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			DWORD WINAPI runThreadFunc(_In_ LPVOID p)
			{
				WorkerThread* t = (WorkerThread*)p;
				if (t != nullptr) 
				{ 
					t->onRun(); 
					return 0;
				}
				return (DWORD)API::Global::Debug::Error::THREAD_VOID_ERROR;
			}
			void CLOAK_CALL InitContext(_In_ Impl::Rendering::GraphicContext* context, _In_ WorkerThreadFormat format)
			{
				switch (format)
				{
					case WorkerThreadFormat::Screen:
						Core::InitContext::SetScreenRect(context);
						Core::InitContext::SetFrameBuffer(context);
						break;
					case WorkerThreadFormat::Shadow:
						//TODO
						break;
					default:
						break;
				}
			}

			CLOAK_CALL WorkerThread::WorkerThread()
			{
				m_didSetup = false;
				m_run = true;
				m_bufferPos = 0;
				m_sema = CreateSemaphore(nullptr, 0, MaxSemaCount, nullptr);
				m_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
				m_stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
				m_thread = CreateThread(nullptr, 0, runThreadFunc, this, 0, &m_threadID);
			}
			CLOAK_CALL WorkerThread::~WorkerThread()
			{
				Shutdown();
				DWORD hR = WaitForSingleObject(m_stop, 2 * MAX_WAIT);
				if (hR == WAIT_TIMEOUT) { CloakError(API::Global::Debug::Error::THREAD_NO_RETURN, false); }
				CloseHandle(m_stop);
				CloseHandle(m_thread);
				CloseHandle(m_event);
				CloseHandle(m_sema);
			}
			void CLOAK_CALL_THIS WorkerThread::onRun()
			{
				Impl::Rendering::GraphicContext* context = nullptr;
				size_t iPos = 0;
				while (m_run && Impl::Global::Game::canThreadRun())
				{
					DWORD hR = 0;
					do {
						hR = WaitForSingleObject(m_sema, MAX_WAIT);
					} while (hR == WAIT_TIMEOUT && Impl::Global::Game::canThreadRun() && m_run);
					if (hR == WAIT_OBJECT_0)
					{
						BasicCommand* bcmd = reinterpret_cast<BasicCommand*>(&m_buffer[iPos]);
						switch (bcmd->state)
						{
							case State::Start:
							{
								StartCommand* cmd = reinterpret_cast<StartCommand*>(bcmd);
								Impl::Rendering::Context::Begin(IID_PPV_ARGS(&context));
								context->SetPSO(cmd->PSO);
								context->SetRenderTargets(cmd->numRTV, cmd->RTVs, cmd->DSV, cmd->readOnlyDepth);
								context->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
								InitContext(context, cmd->format);
								delete[] cmd->RTVs;
								break;
							}
							case State::Render:
							{
								RenderCommand* cmd = reinterpret_cast<RenderCommand*>(bcmd);
								cmd->toDraw->Draw(context);
								cmd->toDraw->Release();
								break;
							}
							case State::End:
							{
								EndCommand* cmd = reinterpret_cast<EndCommand*>(bcmd);
								context->CloseAndExecute(false, false);
								SAVE_RELEASE(context);
								SetEvent(m_event);
								break;
							}
							default:
								break;
						}
						iPos = (iPos + SlotSize) % BufferByteSize;
					}
				}
				SetEvent(m_stop);
			}

			void CLOAK_CALL_THIS WorkerThread::StartDraw(_In_ const WorkerThreadSetupDesc& desc)
			{
				if (m_didSetup == false)
				{
					m_didSetup = true;
					const size_t iPos = m_bufferPos;
					m_bufferPos = (m_bufferPos + SlotSize) % BufferByteSize;
					StartCommand* cmd = reinterpret_cast<StartCommand*>(&m_buffer[iPos]);
					cmd->state = State::Start;
					cmd->DSV = desc.dsv;
					cmd->format = desc.format;
					cmd->numRTV = desc.rtvCount;
					cmd->PSO = desc.pso;
					cmd->readOnlyDepth = desc.readOnlyDepth;
					cmd->RTVs = new API::Rendering::IColorBuffer*[cmd->numRTV];
					for (size_t a = 0; a < cmd->numRTV; a++) { cmd->RTVs[a] = desc.rtv[a]; }
					ReleaseSemaphore(m_sema, 1, nullptr);
				}
				else { CloakError(API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false); }
			}
			void CLOAK_CALL_THIS WorkerThread::Draw(_In_ API::Rendering::IDrawable* toDraw)
			{
				if (m_didSetup == true)
				{
					if (toDraw != nullptr)
					{
						const size_t iPos = m_bufferPos;
						m_bufferPos = (m_bufferPos + SlotSize) % BufferByteSize;
						RenderCommand* cmd = reinterpret_cast<RenderCommand*>(&m_buffer[iPos]);
						cmd->state = State::Render;
						cmd->toDraw = toDraw;
						toDraw->AddRef();
						ReleaseSemaphore(m_sema, 1, nullptr);
					}
				}
				else { CloakError(API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false); }
			}
			void CLOAK_CALL_THIS WorkerThread::EndDraw()
			{
				if (m_didSetup == true)
				{
					m_didSetup = false;
					const size_t iPos = m_bufferPos;
					m_bufferPos = (m_bufferPos + SlotSize) % BufferByteSize;
					EndCommand* cmd = reinterpret_cast<EndCommand*>(&m_buffer[iPos]);
					cmd->state = State::End;
					ReleaseSemaphore(m_sema, 1, nullptr);
				}
				else { CloakError(API::Global::Debug::Error::ILLEGAL_FUNC_CALL, false); }
			}
			void CLOAK_CALL_THIS WorkerThread::Shutdown() { m_run = false; }

			void CLOAK_CALL WorkerThread::WaitForWorker(_In_reads_(count) WorkerThread* threads, _In_ size_t count)
			{
				if (Impl::Global::Game::canThreadRun())
				{
					HANDLE* evs = new HANDLE[count];
					for (size_t a = 0; a < count; a++) { evs[a] = threads[a].m_event; }
					DWORD hR = 0;
					do {
						hR = WaitForMultipleObjects(static_cast<DWORD>(count), evs, TRUE, MAX_WAIT);
					} while (hR == WAIT_TIMEOUT && Impl::Global::Game::canThreadRun());
					if (hR >= WAIT_OBJECT_0 && hR < WAIT_OBJECT_0 + count)
					{
						for (size_t a = 0; a < count; a++) { ResetEvent(evs[a]); }
					}
					delete[] evs;
				}
			}
			void CLOAK_CALL WorkerThread::PassToThreads(_In_reads_(threadCount) WorkerThread* threads, _In_ size_t threadCount, _In_reads_(drawCount) API::Rendering::IDrawable** toDraw, _In_ size_t drawCount, _Inout_ Impl::Rendering::GraphicContext** context, _In_ Impl::Rendering::CommandListManager* cmdListMan, _In_ const WorkerThreadSetupDesc& setup)
			{
				if (context != nullptr && toDraw != nullptr && drawCount > 0)
				{
					Impl::Rendering::GraphicContext* con = *context;
					if (con != nullptr)
					{
						if (threadCount > 0 && threads != nullptr && cmdListMan != nullptr)
						{
							for (size_t a = 0; a < setup.rtvCount; a++) { con->TransitionResource(setup.rtv[a], D3D12_RESOURCE_STATE_RENDER_TARGET); }
							if (setup.dsv != nullptr) { con->TransitionResource(setup.dsv, setup.readOnlyDepth ? D3D12_RESOURCE_STATE_DEPTH_READ : D3D12_RESOURCE_STATE_DEPTH_WRITE); }
							con->CloseAndExecute();
							for (size_t a = 0; a < threadCount; a++) { threads[a].StartDraw(setup); }
							for (size_t a = 0; a < drawCount; a++) { threads[a].Draw(toDraw[a]); }
							for (size_t a = 0; a < threadCount; a++) { threads[a].EndDraw(); }
							WaitForWorker(threads, threadCount);
							cmdListMan->FlushExecutions();
						}
						else
						{
							con->SetPSO(setup.pso);
							con->SetRenderTargets(setup.rtvCount, setup.rtv, setup.dsv, setup.readOnlyDepth);
							con->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
							InitContext(con, setup.format);

							for (size_t a = 0; a < drawCount; a++) { toDraw[a]->Draw(con); }
							con->CloseAndExecute();
						}
						SAVE_RELEASE(con);
						Impl::Rendering::Context::Begin(IID_PPV_ARGS(context));
					}
				}
			}
		}
	}
}