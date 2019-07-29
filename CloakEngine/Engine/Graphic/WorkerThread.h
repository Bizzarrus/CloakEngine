#pragma once
#ifndef CE_E_GRAPHIC_WORKERTHREAD_H
#define CE_E_GRAPHIC_WORKERTHREAD_H

#include "CloakEngine\Defines.h"
#include "CloakEngine\Global\Graphic.h"
#include "CloakEngine\Rendering\ColorBuffer.h"
#include "CloakEngine\Rendering\DepthBuffer.h"

#include "Implementation\Rendering\PipelineState.h"
#include "Implementation\Rendering\Context.h"
#include "Implementation\Rendering\CommandListManager.h"

#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			enum class WorkerThreadFormat { Screen, Shadow };
			struct WorkerThreadSetupDesc {
				Impl::Rendering::PipelineState* pso;
				WorkerThreadFormat format;
				API::Rendering::IColorBuffer** rtv;
				size_t rtvCount;
				API::Rendering::IDepthBuffer* dsv = nullptr;
				bool readOnlyDepth = false;
			};
			class WorkerThread {
				public:	
					CLOAK_CALL WorkerThread();
					CLOAK_CALL ~WorkerThread();
					void CLOAK_CALL_THIS onRun();

					void CLOAK_CALL_THIS StartDraw(_In_ const WorkerThreadSetupDesc& desc);
					void CLOAK_CALL_THIS Draw(_In_ API::Rendering::IDrawable* toDraw);
					void CLOAK_CALL_THIS EndDraw();
					void CLOAK_CALL_THIS Shutdown();

					static void CLOAK_CALL WaitForWorker(_In_reads_(count) WorkerThread* threads, _In_ size_t count);
					//Notice: PassToThreads will execute and release the given context and begin a new one. If threadCount is 0, the given context is used to draw all objects on this thread before the context is executed. PassToThreads may block until the worker threads executed all draw commands.
					static void CLOAK_CALL PassToThreads(_In_reads_(threadCount) WorkerThread* threads, _In_ size_t threadCount, _In_reads_(drawCount) API::Rendering::IDrawable** toDraw, _In_ size_t drawCount, _Inout_ Impl::Rendering::GraphicContext** context, _In_ Impl::Rendering::CommandListManager* cmdListMan, _In_ const WorkerThreadSetupDesc& initDesc);
				private:
					enum class State { Start, Render, End };
					struct BasicCommand {
						State state;
					};
					struct StartCommand : public BasicCommand {
						API::Rendering::IColorBuffer** RTVs;
						API::Rendering::IDepthBuffer* DSV;
						Impl::Rendering::PipelineState* PSO;
						size_t numRTV;
						bool readOnlyDepth;
						WorkerThreadFormat format;
					};
					struct RenderCommand : public BasicCommand {
						API::Rendering::IDrawable* toDraw;
					};
					struct EndCommand : public BasicCommand {

					};

					static constexpr size_t MaxSemaCount = 511;
					static constexpr size_t SlotSize = max(sizeof(StartCommand), max(sizeof(RenderCommand), sizeof(EndCommand)));
					static constexpr size_t BufferByteSize = (MaxSemaCount + 1)*SlotSize;

					BYTE m_buffer[BufferByteSize];
					HANDLE m_sema;
					HANDLE m_thread;
					HANDLE m_event;
					HANDLE m_stop;
					DWORD m_threadID;
					size_t m_bufferPos;
					bool m_didSetup;
					std::atomic<bool> m_run;
			};
		}
	}
}

#endif