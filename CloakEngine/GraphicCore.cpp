#include "stdafx.h"
#include "CloakEngine/Global/Memory.h"

#include "Engine/Graphic/Core.h"
#include "Engine/WindowHandler.h"
#include "Engine/FileLoader.h"
#include "Engine/WorldStreamer.h"
#include "Engine/Graphic/DefaultRenderPass.h"
#include "Engine/Physics/Core.h"

#include "Implementation/Rendering/Manager.h"
#include "Implementation/Rendering/ColorBuffer.h"
#include "Implementation/Rendering/StructuredBuffer.h"
#include "Implementation/Rendering/Context.h"
#include "Implementation/Rendering/VertexData.h"
#include "Implementation/Rendering/DepthBuffer.h"
#include "Implementation/Rendering/RenderWorker.h"
#include "Implementation/Global/Video.h"
#include "Implementation/Global/Graphic.h"
#include "Implementation/Interface/BasicGUI.h"
#include "Implementation/Global/Debug.h"
#include "Implementation/Interface/LoadScreen.h"
#include "Implementation/OS.h"
#include "Implementation/Files/Scene.h"
#include "Implementation/Components/Position.h"
#include "Implementation/Components/Camera.h"
#include "Implementation/Components/GraphicUpdate.h"

#include "CloakEngine/Global/Video.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/World/ComponentManager.h"

#include <atomic>

#define CHECK_UPDATE(o) (o::IsWaitingForUpdate()==false)

#ifdef _DEBUG
#define SET_DEBUG_RESOURCE_NAME(rsc, name) (rsc)->SetDebugResourceName(name)
#else
#define SET_DEBUG_RESOURCE_NAME(rsc, name)
#endif

#define RENDERING_POST_PROCESS_BUFFER_COUNT 2
#define RENDERING_DEFERRED_BUFFER_COUNT 4
#define RENDERING_OIB_BUFFER_COUNT 2

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			namespace Core {
				constexpr float LOD_RANGE_LOW = 1;
				constexpr float LOD_RANGE_MID = LOD_RANGE_LOW / 2;
				constexpr float LOD_RANGE_HIGH = LOD_RANGE_MID / 2;
				constexpr float LOD_RANGE_ULTRA = LOD_RANGE_HIGH / 2;
				constexpr float DEFAULT_GAMMA = 1 / 2.2f;

				Impl::Rendering::IManager* g_cmdListManager = nullptr;
				Impl::Rendering::ISwapChain* g_SwapChain = nullptr;
				API::Global::Memory::PageStackAllocator* g_frameAlloc = nullptr;

				struct UpdateScreenInfo {
					bool shouldUpdate = false;
					uint64_t flag = 0;
				};

				struct PassEffects {
					float Gamma = DEFAULT_GAMMA;
					API::Global::Graphic::BloomMode Bloom;
					API::Global::Graphic::MSAA MSAA;
				};

				std::atomic<bool> g_vSync = false;
				std::atomic<bool> g_updatedClip = true;
				std::atomic<UINT> g_curFrameCount = 0;
				std::atomic<UINT> g_usedFrameCount = 0;
				std::atomic<UINT> g_frameIndex = 0;
				std::atomic<UINT> g_lastShownFrame = 0;
				std::atomic<bool> g_init = false;
				std::atomic<bool> g_updateProjMatrix = false;
				std::atomic<bool> g_loadedShader = false;
				std::atomic<bool> g_hadLoadScreen = false;
				std::atomic<bool> g_checkDrawing = false;
				std::atomic<bool> g_waitForResize = false;
				std::atomic<uint32_t> g_screenWidth = 0;
				std::atomic<uint32_t> g_screenHeight = 0;
				std::atomic<float> g_fov = 0;
				std::atomic<UpdateScreenInfo> g_updScreen;
				bool g_lastLoadingScreen = false;
				bool g_lastVideo = false;
				PassEffects g_passEffects;

				uint64_t* g_frameFences = nullptr;
				API::Helper::ISyncSection* g_syncFrameFence = nullptr;

				API::Rendering::IRenderPass* g_renderPass = nullptr;

				struct CamData {
					Impl::Components::Camera* Camera;
					API::Rendering::CameraData API;
				};
				namespace DrawWorker {
					struct SetupCommand {
						API::Rendering::WorkerSetup setup;
						API::Rendering::IPipelineState* pso;
						API::Files::MaterialType matType;
						API::Rendering::DRAW_TYPE type;
						size_t passID;
						void* Data;
						API::Global::Math::Vector camPos;
						float camViewDist;
						size_t camID;
						API::Files::LOD maxLOD;
					};
#pragma warning(push)
#pragma warning(disable: 4250)
					class Worker : public virtual Impl::Rendering::IRenderWorker, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL Worker();
							CLOAK_CALL ~Worker();
							void CLOAK_CALL_THIS EnableMultithreading(In bool enable, In API::Rendering::IManager* manager);

							void* CLOAK_CALL_THIS StartDraw(In API::Rendering::IManager* manager, In API::Rendering::IContext* context, In size_t bufferSize) override;
							void* CLOAK_CALL_THIS StartDraw(In API::Rendering::IManager* manager, In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In size_t bufferSize) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In bool increasePassID) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In bool increasePassID) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In bool increasePassID) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In bool increasePassID) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD = API::Files::LOD::ULTRA, In_opt bool increasePassID = false) override;
							void CLOAK_CALL_THIS Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID) override;
							void CLOAK_CALL_THIS Draw(In API::Rendering::IDrawable3D* toDraw) override;
							void CLOAK_CALL_THIS FinishDraw(Inout API::Rendering::IContext** context, In_opt bool increasePassID = true) override;
							void CLOAK_CALL_THIS WaitForExecution() override;

							void CLOAK_CALL_THIS SetCamera(In size_t id) override;

							void CLOAK_CALL_THIS StartUpdate(In Impl::Rendering::IManager* manager);
							void CLOAK_CALL_THIS PushToUpdate(In API::Rendering::IDrawable2D* toUpdate);
							void CLOAK_CALL_THIS PushToUpdate(In Impl::Components::IGraphicUpdate* toUpdate);
							void CLOAK_CALL_THIS PushToUpdate(In Impl::Components::IGraphicUpdatePosition* toUpdate, In Impl::Components::Position* pos);
							void CLOAK_CALL_THIS PushToUpdate(In Impl::Components::Camera* toUpdate, In Impl::Components::Position* pos, In API::Rendering::CameraData* data);
							void CLOAK_CALL_THIS FinishUpdate();
							bool CLOAK_CALL_THIS Initialized() const;
							void CLOAK_CALL_THIS EndFrame();

							void CLOAK_CALL_THIS OnDrawEnd();
							void CLOAK_CALL_THIS OnUpdateEnd();
							void CLOAK_CALL_THIS OnDraw(In size_t id, In API::Rendering::IDrawable3D* toDraw, In size_t setupState);
							void CLOAK_CALL_THIS OnUpdate(In size_t id, In API::Rendering::IDrawable2D* toUpdate);
							void CLOAK_CALL_THIS OnUpdate(In size_t id, In Impl::Components::IGraphicUpdate* toUpdate);
							void CLOAK_CALL_THIS OnUpdate(In size_t id, Impl::Components::IGraphicUpdatePosition* toUpdate, In Impl::Components::Position* pos);
						protected:
							Success(return == true) bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

							API::Global::Time m_curTime;
							SRWLOCK m_srw;
							size_t m_passID;
							size_t m_size;
							size_t m_adaptCount;
							size_t m_bufferSize;
							size_t m_curSetupID;
							size_t m_camID;
							Impl::Rendering::IContext** m_contextPool;
							API::Rendering::IContext3D** m_context3D;
							size_t* m_setupStates;
							Impl::Rendering::ISingleQueue* m_queue;
							void* m_dataBuffer;
							API::Files::LOD m_maxLOD;
							API::Rendering::DRAW_TAG m_drawTag;
							API::Global::Math::Vector m_camPos;
							API::Rendering::IPipelineState* m_pso;
							API::Rendering::DRAW_TYPE m_type;
							float m_camViewDist;
							API::Global::Task m_endingTask;
							API::List<SetupCommand> m_setups;
							bool m_waitForExec;
#ifdef _DEBUG
							std::atomic<bool> m_finishedExec = false;
#endif
					};
#pragma warning(pop)
					CLOAK_CALL Worker::Worker() : m_srw(SRWLOCK_INIT)
					{
						m_passID = 0;
						m_size = 0;
						m_adaptCount = 0;
						m_bufferSize = 0;
						m_curSetupID = 0;
						m_camID = 0;
						m_contextPool = nullptr;
						m_context3D = nullptr;
						m_setupStates = nullptr;
						m_dataBuffer = nullptr;
						m_queue = nullptr;
						m_pso = nullptr;
						m_waitForExec = false;
					}
					CLOAK_CALL Worker::~Worker()
					{
						WaitForExecution();
						API::Global::Memory::MemoryHeap::Free(m_contextPool);
						API::Global::Memory::MemoryHeap::Free(m_context3D);
						API::Global::Memory::MemoryHeap::Free(m_dataBuffer);
						API::Global::Memory::MemoryHeap::Free(m_setupStates);
					}
					void CLOAK_CALL_THIS Worker::EnableMultithreading(In bool enable, In API::Rendering::IManager* manager)
					{
						const size_t s = enable == true ? API::Global::GetExecutionThreadCount() : 1;
						if (m_size != s)
						{
							for (size_t a = 0; a < m_size; a++) { SAVE_RELEASE(m_contextPool[a]); }
							API::Global::Memory::MemoryHeap::Free(m_contextPool);
							API::Global::Memory::MemoryHeap::Free(m_context3D);
							API::Global::Memory::MemoryHeap::Free(m_setupStates);
							m_adaptCount = manager->GetNodeCount();
							m_size = s;
							m_contextPool = reinterpret_cast<Impl::Rendering::IContext**>(API::Global::Memory::MemoryHeap::Allocate(max(sizeof(Impl::Rendering::IContext*), sizeof(Impl::Rendering::ICopyContext*)) * m_size));
							m_context3D = reinterpret_cast<API::Rendering::IContext3D**>(API::Global::Memory::MemoryHeap::Allocate(sizeof(API::Rendering::IContext3D*) * m_size));
							for (size_t a = 0; a < m_size; a++)
							{
								m_contextPool[a] = nullptr;
								m_context3D[a] = nullptr;
							}
							if (m_size > 1)
							{
								m_setupStates = reinterpret_cast<size_t*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(size_t) * m_size));
								for (size_t a = 0; a < m_size; a++)
								{
									m_setupStates[a] = 0;
								}
							}
							else { m_setupStates = nullptr; }
						}
					}

					void* CLOAK_CALL_THIS Worker::StartDraw(In API::Rendering::IManager* manager, In API::Rendering::IContext* context, In size_t bufferSize)
					{
						CLOAK_ASSUME(context != nullptr);
						CLOAK_ASSUME(manager != nullptr);
						WaitForExecution();
						CLOAK_ASSUME(m_waitForExec == false);
						if (m_bufferSize < bufferSize)
						{
							m_bufferSize = bufferSize;
							API::Global::Memory::MemoryHeap::Free(m_dataBuffer);
							m_dataBuffer = API::Global::Memory::MemoryHeap::Allocate(m_bufferSize);
						}
						Impl::Rendering::IContext* con = nullptr;
						HRESULT hRet = context->QueryInterface(CE_QUERY_ARGS(&con));
						CLOAK_ASSUME(SUCCEEDED(hRet));
						m_queue = con->GetQueue();
						if (m_size > 1)
						{
							context->CloseAndExecute();
							SAVE_RELEASE(con);
							m_endingTask = [this](In size_t id) { this->OnDrawEnd(); };
						}
						else { m_contextPool[0] = con; }
						SAVE_RELEASE(context);
						return m_dataBuffer;
					}
					void* CLOAK_CALL_THIS Worker::StartDraw(In API::Rendering::IManager* manager, In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In size_t bufferSize)
					{
						CLOAK_ASSUME(manager != nullptr);
						WaitForExecution();
						CLOAK_ASSUME(m_waitForExec == false);
						if (m_bufferSize < bufferSize)
						{
							m_bufferSize = bufferSize;
							API::Global::Memory::MemoryHeap::Free(m_dataBuffer);
							m_dataBuffer = API::Global::Memory::MemoryHeap::Allocate(m_bufferSize);
						}
						Impl::Rendering::IManager* man = nullptr;
						HRESULT hRet = manager->QueryInterface(CE_QUERY_ARGS(&man));
						CLOAK_ASSUME(SUCCEEDED(hRet));
						m_queue = man->GetQueue(type, nodeID);
						if (m_size > 1)
						{
							m_endingTask = [this](In size_t id) { this->OnDrawEnd(); };
						}
						else
						{
							m_contextPool[0] = nullptr;
							m_queue->CreateContext(CE_QUERY_ARGS(&m_contextPool[0]));
						}
						SAVE_RELEASE(man);
						return m_dataBuffer;
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In_opt API::Files::LOD maxLOD, In_opt bool increasePassID)
					{
						Setup(setup, bufferOffset, m_type, m_drawTag, m_pso, m_camPos, m_camViewDist, maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In bool increasePassID)
					{
						Setup(setup, bufferOffset, m_type, m_drawTag, m_pso, m_camPos, m_camViewDist, m_maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In_opt API::Files::LOD maxLOD, In_opt bool increasePassID)
					{
						Setup(setup, bufferOffset, type, tag, m_pso, m_camPos, m_camViewDist, maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In bool increasePassID)
					{
						Setup(setup, bufferOffset, type, tag, m_pso, m_camPos, m_camViewDist, m_maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In_opt API::Files::LOD maxLOD, In_opt bool increasePassID)
					{
						Setup(setup, bufferOffset, type, tag, pso, m_camPos, m_camViewDist, maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In bool increasePassID)
					{
						Setup(setup, bufferOffset, type, tag, pso, m_camPos, m_camViewDist, m_maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In_opt API::Files::LOD maxLOD, In_opt bool increasePassID)
					{
						Setup(setup, bufferOffset, m_type, m_drawTag, pso, m_camPos, m_camViewDist, maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In bool increasePassID)
					{
						Setup(setup, bufferOffset, m_type, m_drawTag, pso, m_camPos, m_camViewDist, m_maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD, In_opt bool increasePassID)
					{
						Setup(setup, bufferOffset, m_type, m_drawTag, pso, camPos, viewDistance, maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID)
					{
						Setup(setup, bufferOffset, m_type, m_drawTag, pso, camPos, viewDistance, m_maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD, In_opt bool increasePassID)
					{
						Setup(setup, bufferOffset, m_type, m_drawTag, m_pso, camPos, viewDistance, maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID)
					{
						Setup(setup, bufferOffset, m_type, m_drawTag, m_pso, camPos, viewDistance, m_maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In_opt API::Files::LOD maxLOD, In_opt bool increasePassID)
					{
						CLOAK_ASSUME(m_waitForExec == false);
						void* data = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(m_dataBuffer) + bufferOffset);
						const size_t s = m_queue->GetNodeID() - 1;
						m_drawTag = tag;
						m_camPos = camPos;
						m_camViewDist = viewDistance;
						API::Files::MaterialType matType = API::Files::MaterialType::Unknown;
						m_maxLOD = maxLOD;
						m_pso = pso;
						m_type = type;
						if ((tag & API::Rendering::DRAW_TAG::DRAW_TAG_OPAQUE) != static_cast<API::Rendering::DRAW_TAG>(0)) { matType |= API::Files::MaterialType::Opaque; }
						if ((tag & API::Rendering::DRAW_TAG::DRAW_TAG_ALPHA) != static_cast<API::Rendering::DRAW_TAG>(0)) { matType |= API::Files::MaterialType::AlphaMapped; }
						if ((tag & API::Rendering::DRAW_TAG::DRAW_TAG_TRANSPARENT) != static_cast<API::Rendering::DRAW_TAG>(0)) { matType |= API::Files::MaterialType::Transparent; }
						if (increasePassID) { m_passID++; }
						if (m_size > 1)
						{
							SetupCommand sc;
							sc.Data = data;
							sc.passID = m_passID;
							sc.setup = setup;
							sc.pso = pso;
							sc.type = type;
							sc.matType = matType;
							sc.camID = m_camID;
							sc.camPos = m_camPos;
							sc.camViewDist = m_camViewDist;
							sc.maxLOD = m_maxLOD;
							AcquireSRWLockExclusive(&m_srw);
							m_setups.push_back(sc);
							ReleaseSRWLockExclusive(&m_srw);
							m_curSetupID++;
						}
						else
						{
							m_contextPool[0]->SetWorker(this, m_passID, matType, type, m_camPos, m_camViewDist, m_camID, m_maxLOD);
							m_contextPool[0]->SetPipelineState(pso);
							m_context3D[0] = setup(m_contextPool[0], m_passID, data);
							CLOAK_ASSUME(m_context3D[0] != nullptr);
						}
					}
					void CLOAK_CALL_THIS Worker::Setup(In API::Rendering::WorkerSetup setup, In size_t bufferOffset, In API::Rendering::DRAW_TYPE type, In API::Rendering::DRAW_TAG tag, In API::Rendering::IPipelineState* pso, In const API::Global::Math::Vector& camPos, In float viewDistance, In bool increasePassID)
					{
						Setup(setup, bufferOffset, type, tag, pso, camPos, viewDistance, m_maxLOD, increasePassID);
					}
					void CLOAK_CALL_THIS Worker::Draw(In API::Rendering::IDrawable3D* toDraw)
					{
						CLOAK_ASSUME(m_waitForExec == false);
						if (toDraw != nullptr)
						{
							if (m_size > 1)
							{
								CLOAK_ASSUME(m_curSetupID > 0);
								toDraw->AddRef();
								size_t curSetupState = m_curSetupID;
								CLOAK_ASSUME(m_waitForExec == false);
								API::Global::Task t = [this, toDraw, curSetupState](In size_t id) { this->OnDraw(id, toDraw, curSetupState); };
								m_endingTask.AddDependency(t);
								t.Schedule(CE::Global::Threading::Priority::High);
							}
							else
							{
								toDraw->Draw(m_context3D[0]);
							}
						}
					}
					void CLOAK_CALL_THIS Worker::FinishDraw(Inout API::Rendering::IContext** context, In_opt bool increasePassID)
					{
						CLOAK_ASSUME(m_waitForExec == false);
						if (m_size > 1)
						{
							m_endingTask.Schedule(CE::Global::Threading::Priority::High);
							m_queue->CreateContext(CE_QUERY_ARGS(context));
							m_waitForExec = true;
						}
						else
						{
							m_context3D[0]->Execute();
							*context = m_contextPool[0];
							m_contextPool[0] = nullptr;
						}
						if (increasePassID) { m_passID++; }
					}
					void CLOAK_CALL_THIS Worker::WaitForExecution()
					{
						if (m_waitForExec == true)
						{
							m_endingTask.WaitForExecution();
#ifdef _DEBUG
							CLOAK_ASSUME(m_finishedExec.exchange(false) == true);
#endif
							m_queue->FlushExecutions(g_frameAlloc);
							m_waitForExec = false;
						}
					}

					void CLOAK_CALL_THIS Worker::SetCamera(In size_t id)
					{
						m_camID = id;
					}
					void CLOAK_CALL_THIS Worker::StartUpdate(In Impl::Rendering::IManager* manager)
					{
						WaitForExecution();
						CLOAK_ASSUME(m_waitForExec == false);
						m_curTime = API::Global::Game::GetGameTimeStamp();
						m_queue = manager->GetQueue(API::Rendering::CONTEXT_TYPE_GRAPHIC_COPY, 0);
						if (m_size > 1)
						{
							m_endingTask = [this](In size_t id) { this->OnUpdateEnd(); };
						}
						else
						{
							m_contextPool[0] = nullptr;
							m_queue->CreateContext(CE_QUERY_ARGS(&m_contextPool[0]));
						}
					}
					void CLOAK_CALL_THIS Worker::PushToUpdate(In API::Rendering::IDrawable2D* toUpdate)
					{
						CLOAK_ASSUME(m_waitForExec == false);
						if (toUpdate != nullptr)
						{
							if (m_size > 1)
							{
								toUpdate->AddRef();
								API::Global::Task t = [this, toUpdate](In size_t id) { this->OnUpdate(id, toUpdate); };
								m_endingTask.AddDependency(t);
								t.Schedule(CE::Global::Threading::Priority::High);
							}
							else
							{
								toUpdate->UpdateDrawInformations(m_curTime, m_contextPool[0], g_cmdListManager);
							}
						}
					}
					void CLOAK_CALL_THIS Worker::PushToUpdate(In Impl::Components::IGraphicUpdate* toUpdate)
					{
						CLOAK_ASSUME(m_waitForExec == false);
						if (toUpdate != nullptr)
						{
							if (m_size > 1)
							{
								toUpdate->AddRef();
								API::Global::Task t = [this, toUpdate](In size_t id) { this->OnUpdate(id, toUpdate); };
								m_endingTask.AddDependency(t);
								t.Schedule(CE::Global::Threading::Priority::High);
							}
							else
							{
								toUpdate->Update(m_curTime, m_contextPool[0], g_cmdListManager);
							}
						}
					}
					void CLOAK_CALL_THIS Worker::PushToUpdate(In Impl::Components::IGraphicUpdatePosition* toUpdate, In Impl::Components::Position* pos)
					{
						CLOAK_ASSUME(m_waitForExec == false);
						if (toUpdate != nullptr && pos != nullptr)
						{
							if (m_size > 1)
							{
								toUpdate->AddRef();
								pos->AddRef();
								API::Global::Task t = [this, toUpdate, pos](In size_t id) {this->OnUpdate(id, toUpdate, pos); };
								m_endingTask.AddDependency(t);
								t.Schedule(CE::Global::Threading::Priority::High);
							}
							else
							{
								toUpdate->Update(m_curTime, pos, m_contextPool[0], g_cmdListManager);
							}
						}
					}
					void CLOAK_CALL_THIS Worker::PushToUpdate(In Impl::Components::Camera* toUpdate, In Impl::Components::Position* pos, In API::Rendering::CameraData* data)
					{
						CLOAK_ASSUME(m_waitForExec == false);
						if (toUpdate != nullptr && pos != nullptr)
						{
							//Cameras must be updated in this thread, as their data is required before rendering other things!
							toUpdate->Update(m_curTime, pos, g_screenWidth, g_screenHeight, g_fov, g_updatedClip, data);
						}
					}
					void CLOAK_CALL_THIS Worker::FinishUpdate()
					{
						CLOAK_ASSUME(m_waitForExec == false);
						if (m_size > 1)
						{
							m_endingTask.Schedule(CE::Global::Threading::Priority::High);
							m_waitForExec = true;
						}
						else
						{
							m_contextPool[0]->CloseAndExecute();
							SAVE_RELEASE(m_contextPool[0]);
						}
					}
					bool CLOAK_CALL_THIS Worker::Initialized() const { return m_size > 0; }
					void CLOAK_CALL_THIS Worker::EndFrame()
					{
						WaitForExecution();
						m_passID = 0;
					}

					Success(return == true) bool CLOAK_CALL_THIS Worker::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Rendering::IRenderWorker)) { *ptr = (API::Rendering::IRenderWorker*)this; return true; }
						return SavePtr::iQueryInterface(riid, ptr);
					}
					void CLOAK_CALL_THIS Worker::OnDrawEnd()
					{
						CLOAK_ASSUME(m_size > 1);
						for (size_t a = 0; a < m_size; a++)
						{
							if (m_setupStates[a] != 0)
							{
								CLOAK_ASSUME(m_contextPool[a] != nullptr);
								CLOAK_ASSUME(m_context3D[a] != nullptr);
								m_context3D[a]->Execute();
								m_contextPool[a]->CloseAndExecute(false, false);
								SAVE_RELEASE(m_contextPool[a]);
								m_setupStates[a] = 0;
							}
						}
						m_setups.clear();
						m_curSetupID = 0;
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec.exchange(true) == false);
#endif
					}
					void CLOAK_CALL_THIS Worker::OnUpdateEnd()
					{
						CLOAK_ASSUME(m_size > 1);
						Impl::Rendering::ICopyContext** copyPool = reinterpret_cast<Impl::Rendering::ICopyContext**>(m_contextPool);
						for (size_t a = 0; a < m_size; a++)
						{
							if (m_setupStates[a] != 0)
							{
								CLOAK_ASSUME(copyPool[a] != nullptr);
								copyPool[a]->CloseAndExecute(false, false);
								SAVE_RELEASE(copyPool[a]);
								m_setupStates[a] = 0;
							}
						}
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec.exchange(true) == false);
#endif
					}
					void CLOAK_CALL_THIS Worker::OnDraw(In size_t id, In API::Rendering::IDrawable3D* toDraw, In size_t setupState)
					{	
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec == false);
#endif
						CLOAK_ASSUME(setupState > 0);
						CLOAK_ASSUME(id < m_size);
						CLOAK_ASSUME((m_setupStates[id] == 0) == (m_contextPool[id] == nullptr));
						if (m_setupStates[id] != setupState)
						{
							if (m_setupStates[id] == 0) 
							{
								m_queue->CreateContext(CE_QUERY_ARGS(&m_contextPool[id]));
								m_contextPool[id]->BlockTransitions(true);
							}
							m_setupStates[id] = setupState;
							AcquireSRWLockShared(&m_srw);
							CLOAK_ASSUME(m_setups.size() >= setupState);
							SetupCommand sc = m_setups[setupState - 1];
							ReleaseSRWLockShared(&m_srw);
							m_contextPool[id]->SetWorker(this, sc.passID, sc.matType, sc.type, sc.camPos, sc.camViewDist, sc.camID, sc.maxLOD);
							m_contextPool[id]->SetPipelineState(sc.pso);
							m_context3D[id] = sc.setup(m_contextPool[id], sc.passID, sc.Data);
							CLOAK_ASSUME(m_context3D[id] != nullptr);
						}
						toDraw->Draw(m_context3D[id]);
						toDraw->Release();
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec == false);
#endif
					}
					void CLOAK_CALL_THIS Worker::OnUpdate(In size_t id, In API::Rendering::IDrawable2D* toUpdate)
					{
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec == false);
#endif
						CLOAK_ASSUME(id < m_size);
						CLOAK_ASSUME((m_setupStates[id] == 0) == (m_contextPool[id] == nullptr));
						Impl::Rendering::ICopyContext** copyPool = reinterpret_cast<Impl::Rendering::ICopyContext**>(m_contextPool);
						if (m_setupStates[id] == 0) 
						{ 
							m_queue->CreateContext(CE_QUERY_ARGS(&copyPool[id]));
							m_setupStates[id] = 1;
						}
						CLOAK_ASSUME(copyPool[id] != nullptr);
						toUpdate->UpdateDrawInformations(m_curTime, copyPool[id], g_cmdListManager);
						toUpdate->Release();
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec == false);
#endif
					}
					void CLOAK_CALL_THIS Worker::OnUpdate(In size_t id, In Impl::Components::IGraphicUpdate* toUpdate)
					{
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec == false);
#endif
						CLOAK_ASSUME(id < m_size);
						CLOAK_ASSUME((m_setupStates[id] == 0) == (m_contextPool[id] == nullptr));
						Impl::Rendering::ICopyContext** copyPool = reinterpret_cast<Impl::Rendering::ICopyContext**>(m_contextPool);
						if (m_setupStates[id] == 0)
						{
							m_queue->CreateContext(CE_QUERY_ARGS(&copyPool[id]));
							m_setupStates[id] = 1;
						}
						CLOAK_ASSUME(copyPool[id] != nullptr);
						toUpdate->Update(m_curTime, copyPool[id], g_cmdListManager);
						toUpdate->Release();
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec == false);
#endif
					}
					void CLOAK_CALL_THIS Worker::OnUpdate(In size_t id, Impl::Components::IGraphicUpdatePosition* toUpdate, In Impl::Components::Position* pos)
					{
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec == false);
#endif
						CLOAK_ASSUME(id < m_size);
						CLOAK_ASSUME((m_setupStates[id] == 0) == (m_contextPool[id] == nullptr));
						Impl::Rendering::ICopyContext** copyPool = reinterpret_cast<Impl::Rendering::ICopyContext**>(m_contextPool);
						if (m_setupStates[id] == 0)
						{
							m_queue->CreateContext(CE_QUERY_ARGS(&copyPool[id]));
							m_setupStates[id] = 1;
						}
						CLOAK_ASSUME(copyPool[id] != nullptr);
						toUpdate->Update(m_curTime, pos, copyPool[id], g_cmdListManager);
						toUpdate->Release();
						pos->Release();
#ifdef _DEBUG
						CLOAK_ASSUME(m_finishedExec == false);
#endif
					}
				}
				DrawWorker::Worker* g_worker = nullptr;
				
				inline void CLOAK_CALL RenderFrame(In UINT frameIndex, In API::Global::Time etime)
				{
					bool loadScreen = FileLoader::isLoading(API::Files::Priority::NORMAL);
					if (loadScreen == false) { loadScreen = Impl::Global::Game::checkThreadWaiting(); }
					const bool video = API::Global::Video::IsVideoPlaying();

					Engine::WorldStreamer::NodeList nodeList(g_frameAlloc);
					Engine::WorldStreamer::UpdateNodeToWorld(g_frameAlloc, &nodeList);
					API::World::Group<Impl::Components::Camera, Impl::Components::Position> cameras;
					API::World::Group<Impl::Components::IGraphicUpdate> graphUpdate;
					API::World::Group<Impl::Components::IGraphicUpdatePosition, Impl::Components::Position> graphPosUpdate;

					size_t camCount = 0;
					CamData* camDatas = reinterpret_cast<CamData*>(g_frameAlloc->Allocate(sizeof(CamData)*cameras.size()));

					API::Helper::Lock physicLock = Physics::LockPositionUpdate();
					g_worker->StartUpdate(g_cmdListManager);

					if (loadScreen == false && video == false)
					{
						for (const auto& a : graphUpdate)
						{
							Impl::Components::IGraphicUpdate* upd = a.Get<Impl::Components::IGraphicUpdate>();
							g_worker->PushToUpdate(upd);
							upd->Release();
						}
						for (const auto& a : graphPosUpdate)
						{
							Impl::Components::IGraphicUpdatePosition* upd = a.Get<Impl::Components::IGraphicUpdatePosition>();
							Impl::Components::Position* pos = a.Get<Impl::Components::Position>();
							g_worker->PushToUpdate(upd, pos);
							upd->Release();
							pos->Release();
						}
					}

					const size_t updI = Impl::Interface::BeginToUpdate();
					for (size_t a = 0; a < updI; a++) { g_worker->PushToUpdate(Impl::Interface::GetUpdateGUI(a)); }
					Impl::Interface::EndToUpdate();

					//Cameras are updated after everything else, as they aren't updated with multithreading
					if (loadScreen == false && video == false)
					{
						for (const auto& a : cameras)
						{
							Impl::Components::Camera* cam = a.Get<Impl::Components::Camera>();
							Impl::Components::Position* pos = a.Get<Impl::Components::Position>();
							g_worker->PushToUpdate(cam, pos, &camDatas[camCount].API);
							camDatas[camCount].Camera = cam;
							camCount++;
							pos->Release();
						}
					}

					API::Rendering::PassData pass;
					pass.HDR = g_SwapChain->SupportHDR();
					pass.Gamma = g_passEffects.Gamma;
					pass.MSAA = g_passEffects.MSAA;
					pass.Bloom = g_passEffects.Bloom;
					pass.ResetFrame = (g_lastLoadingScreen == true || g_lastVideo == true) && loadScreen == false && video == false;

					g_worker->FinishUpdate();
					physicLock.unlock();
					g_updatedClip = false;

					API::Rendering::IColorBuffer* screen = g_SwapChain->GetCurrentBackBuffer();
					API::Rendering::IContext* context = nullptr;
					g_cmdListManager->BeginContext(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, CE_QUERY_ARGS(&context));

					if (loadScreen == false && video == false && camCount > 0)
					{
						for (size_t a = 0; a < camCount; a++)
						{
							API::Rendering::RenderTargetData target = camDatas[a].Camera->GetRenderTarget(screen);
							if (target.Target != nullptr)
							{
								g_worker->SetCamera(camDatas[a].API.WorldID);
								g_renderPass->OnRenderCamera(g_cmdListManager, &context, g_worker, camDatas[a].Camera, target, camDatas[a].API, pass, etime);
								CLOAK_ASSUME(context != nullptr);
								target.Target->Release();
							}
						}
						//Wait for execution to ensure the UI has been updated before rendering it
						g_worker->WaitForExecution();

						size_t guiSize = 0;
						API::Interface::IBasicGUI** guis = Impl::Interface::GetVisibleGUIs(g_frameAlloc, &guiSize);
						g_renderPass->OnRenderInterface(g_cmdListManager, &context, g_worker, screen, guiSize, guis, pass);
						CLOAK_ASSUME(context != nullptr);
						for (size_t a = 0; a < guiSize; a++) { guis[a]->Release(); }
						loadScreen = FileLoader::isLoading(API::Files::Priority::NORMAL);
					}
					for (size_t a = 0; a < camCount; a++)
					{
						camDatas[a].Camera->Release();
						if (camDatas[a].API.Skybox != nullptr) { camDatas[a].API.Skybox->Release(); }
					}
#ifdef _DEBUG
					if (g_lastLoadingScreen != loadScreen)
					{
						CloakDebugLog("Show LoadScreen: " + std::to_string(loadScreen));
					}
#endif
					if (video == true)
					{
						context->TransitionResource(screen, API::Rendering::STATE_RENDER_TARGET, true);
						context->ClearColor(screen);

						//TODO: Render video
					}
					else if (loadScreen == true)
					{
						context->TransitionResource(screen, API::Rendering::STATE_RENDER_TARGET, true);
						context->ClearColor(screen);

						//TODO: Get and render loading screen
					}

					g_worker->EndFrame();
					context->TransitionResource(screen, API::Rendering::STATE_PRESENT);
					context->CloseAndExecute();
					SAVE_RELEASE(context);

					g_lastLoadingScreen = loadScreen;
					g_lastVideo = video;
				}

				HRESULT CLOAK_CALL CreateSwapChain(In const API::Global::Graphic::Settings& gset)
				{
					WaitForGPU();

					HRESULT hRet;
					if (g_SwapChain == nullptr) { g_SwapChain = g_cmdListManager->CreateSwapChain(gset, &hRet); }
					else { hRet = g_SwapChain->Recreate(gset); }
					if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_NO_SWAPCHAIN, false))
					{
						Impl::Rendering::SWAP_CHAIN_RESULT r = g_SwapChain->Present(false, Impl::Rendering::PRESENT_TEST);
						if (CloakCheckOK(r != Impl::Rendering::SWAP_CHAIN_RESULT::FAILED, API::Global::Debug::Error::GRAPHIC_NO_SWAPCHAIN, false))
						{
							g_frameIndex = g_SwapChain->GetCurrentBackBufferIndex();
							WaitForGPU();
							g_curFrameCount = gset.BackBufferCount + 1;
						}
					}
					return hRet;
				}
				inline bool CLOAK_CALL SetFullscreenState(In bool fullscreen)
				{
					if (g_SwapChain != nullptr)
					{
						bool curState = g_SwapChain->GetFullscreenState();
						if (fullscreen != curState)
						{
							g_waitForResize = true;
							g_SwapChain->SetFullscreenState(fullscreen);
							return true;
						}
					}
					return false;
				}
				HRESULT CLOAK_CALL ResizeBuffers(In const API::Global::Graphic::Settings& gset, In const API::Global::Graphic::Settings& oldSet)
				{
					CloakDebugLog("Settings Resize to size: " + std::to_string(gset.Resolution.Width) + " | " + std::to_string(gset.Resolution.Height) + " (Previous: " + std::to_string(oldSet.Resolution.Width) + " | " + std::to_string(oldSet.Resolution.Height) + ")");

					if (gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN || oldSet.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN)
					{
						Impl::Rendering::SWAPCHAIN_MODE mode;
						mode.Width = gset.Resolution.Width;
						mode.Height = gset.Resolution.Height;
						mode.Scaling = Impl::Rendering::SCALING_UNSPECIFIED;
						mode.ScanlineOrdering = Impl::Rendering::ORDER_UNSPECIFIED;
						if (gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN) { g_SwapChain->ResizeTarget(mode); }
						const bool changed = SetFullscreenState(gset.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN);
						if (changed == false || (changed == true && gset.WindowMode != API::Global::Graphic::WindowMode::FULLSCREEN && gset.Resolution.Width == oldSet.Resolution.Width && gset.Resolution.Height == oldSet.Resolution.Height))
						{
							ResizeBuffers(gset.Resolution.Width, gset.Resolution.Height);
						}
					}

					g_frameIndex = g_SwapChain->GetCurrentBackBufferIndex();
					g_curFrameCount = gset.BackBufferCount + 1;
					return S_OK;
				}
				void CLOAK_CALL ResizeBuffers(In UINT width, In UINT height)
				{
					CloakDebugLog("Manual Resize to size: " + std::to_string(width) + " | " + std::to_string(height));
					if (g_SwapChain != nullptr)
					{
						HRESULT hRet = S_OK;
						hRet = g_SwapChain->ResizeBuffers(g_curFrameCount, width, height);
						CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_RESIZE, false);
						g_frameIndex = g_SwapChain->GetCurrentBackBufferIndex();

						RECT rc;
						rc.left = 0;
						rc.top = 0;
						rc.right = width;
						rc.bottom = height;
						Impl::Global::Video::OnSizeChange(&rc);
						g_waitForResize = false;
					}
				}
				void CLOAK_CALL InitSettings(In const API::Global::Graphic::Settings& gset)
				{
					g_vSync = gset.VSync;
					API::Helper::Lock lock(g_syncFrameFence);
					uint64_t* n = NewArray(uint64_t, gset.BackBufferCount + 1);
					const UINT mfc = min(gset.BackBufferCount + 1, g_usedFrameCount);
					for (UINT a = 0; a < mfc; a++) { n[a] = g_frameFences[a]; }
					for (UINT a = g_usedFrameCount; a < gset.BackBufferCount + 1; a++) { n[a] = 0; }
					DeleteArray(g_frameFences);
					g_frameFences = n;
					g_usedFrameCount = gset.BackBufferCount + 1;
				}

				void CLOAK_CALL InitPipeline()
				{
					HRESULT hRet = S_OK;
					g_frameAlloc = new API::Global::Memory::PageStackAllocator();
					g_worker = new DrawWorker::Worker();
					
					if (SUCCEEDED(hRet))//Error handling is done by command list manager
					{
						//TODO: Init sync sections
						CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncFrameFence));
					}

					Impl::Interface::LoadScreen::Initialize();
				}
				void CLOAK_CALL Load()
				{
					API::Global::Graphic::Settings set;
					API::Global::Graphic::GetSettings(&set);
					g_cmdListManager = Impl::Rendering::IManager::Create(set.RenderMode);
					if (CloakCheckOK(g_cmdListManager != nullptr, API::Global::Debug::Error::GRAPHIC_NO_MANAGER, true))
					{
						HRESULT hRet = g_cmdListManager->Create(static_cast<UINT>(set.AdapterID));
						if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_FEATURE_UNSUPPORETD, true))
						{

						}
					}
				}
				void CLOAK_CALL Start()
				{
					if (g_cmdListManager->IsReady())
					{
						API::Global::Graphic::Settings set;
						API::Global::Graphic::GetSettings(&set);
						InitSettings(set);
						HRESULT hRet = CreateSwapChain(set);
						if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_SWAPCHAIN, true)) { return; }
						g_screenWidth = static_cast<uint32_t>(set.Resolution.Width);
						g_screenHeight = static_cast<uint32_t>(set.Resolution.Height);
						g_updatedClip = true;

						if (g_renderPass == nullptr) { g_renderPass = new DefaultRenderPass(); }
						g_renderPass->OnInit(g_cmdListManager);

						//TODO: further initializing
						WaitForGPU();
						g_updateProjMatrix = true;
						g_init = true;
					}
				}
				void CLOAK_CALL UpdateSettings(In const API::Global::Graphic::Settings& newSet, In const API::Global::Graphic::Settings& oldSet, In Lock::SettingsFlag flag)
				{
					g_screenWidth = static_cast<uint32_t>(newSet.Resolution.Width);
					g_screenHeight = static_cast<uint32_t>(newSet.Resolution.Height);
					g_fov = newSet.FieldOfView;
					g_updatedClip = true;
					g_passEffects.MSAA = newSet.MSAA;
					g_passEffects.Bloom = newSet.Bloom;
					g_passEffects.Gamma = newSet.Gamma;
					bool bufUpdate = false, psoUpdate = false, matUpdate = false;
					if (newSet.Resolution.Height != oldSet.Resolution.Height || newSet.Resolution.Width != oldSet.Resolution.Width)
					{
						matUpdate = true;
						bufUpdate = true;
					}
					if (newSet.FieldOfView != oldSet.FieldOfView) { matUpdate = true; }
					if (newSet.WindowMode != oldSet.WindowMode || newSet.BackBufferCount + 1 != g_curFrameCount || newSet.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN) { bufUpdate = true; }
					if ((flag & Lock::SettingsFlag::Shader) != Lock::SettingsFlag::None || newSet.Tessellation != oldSet.Tessellation) { psoUpdate = true; }
					InitSettings(newSet);

					if (matUpdate) { g_updateProjMatrix = true; }
					g_renderPass->OnResize(g_cmdListManager, newSet, oldSet, psoUpdate);
					if (bufUpdate) 
					{ 
						HRESULT hRet = ResizeBuffers(newSet, oldSet);
						CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_RESIZE, true); 
					}

					if (newSet.MultiThreadedRendering != oldSet.MultiThreadedRendering || g_worker->Initialized() == false)
					{
						g_worker->EnableMultithreading(newSet.MultiThreadedRendering && g_cmdListManager->SupportMultiThreadedRendering(), g_cmdListManager);
					}
					g_checkDrawing = true;
				}
				void CLOAK_CALL Update(In unsigned long long etime)
				{
					g_cmdListManager->Update();
					if (g_init && WindowHandler::isFocused() && Lock::CheckSettingsUpdateMaster() && g_waitForResize == false)
					{
						if (g_checkDrawing == true)
						{
							Impl::Rendering::SWAP_CHAIN_RESULT hRet = g_SwapChain->Present(false, Impl::Rendering::PRESENT_TEST);
							if (hRet == Impl::Rendering::SWAP_CHAIN_RESULT::OK) { g_checkDrawing = false; }
							g_frameIndex = g_SwapChain->GetCurrentBackBufferIndex();
						}
						else
						{
							const unsigned int frameIndex = g_frameIndex;
							API::Global::Task task = [frameIndex, etime](In size_t id) { RenderFrame(frameIndex, etime); };
							task.Schedule(CE::Global::Threading::Priority::High);
							const uint64_t frameFence = g_frameFences[frameIndex];

							while (task.Finished() == false || g_cmdListManager->IsFenceComplete(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, frameFence) == false) 
							{
								if (WindowHandler::UpdateOnce() == false)
								{
									task.WaitForExecution(CE::Global::Threading::Flag::All);
								}
							}

							const bool vsync = g_vSync.load();
							Impl::Global::Task::IHelpWorker* worker = Impl::Global::Task::GetCurrentWorker();
							do {
								const Impl::Rendering::SWAP_CHAIN_RESULT pr = g_SwapChain->Present(vsync, Impl::Rendering::PRESENT_NO_WAIT);
								if (pr != Impl::Rendering::SWAP_CHAIN_RESULT::NEED_RETRY)
								{
									if (pr == Impl::Rendering::SWAP_CHAIN_RESULT::OCCLUDED) { g_checkDrawing = true; }
									break;
								}
								if (WindowHandler::UpdateOnce() == false)
								{
									worker->HelpWork(CE::Global::Threading::Flag::None);
								}
							} while (true);
							
							g_lastShownFrame = frameIndex + 1;
							const uint64_t ff = g_cmdListManager->IncrementFence(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0);
							API::Helper::Lock lock(g_syncFrameFence);
							g_frameFences[frameIndex] = ff;
							lock.unlock();
							g_frameIndex = g_SwapChain->GetCurrentBackBufferIndex();
							g_frameAlloc->Clear();
							return;
						}
					}
					while (WindowHandler::UpdateOnce() == true) {}
				}
				void CLOAK_CALL Submit()
				{
					//TODO
				}
				void CLOAK_CALL Present()
				{
					//TODO
				}
				void CLOAK_CALL FinalRelease()
				{
					delete g_worker;
					delete g_frameAlloc;

					Impl::Interface::LoadScreen::Terminate();
					if (g_cmdListManager != nullptr)
					{
						g_cmdListManager->WaitForGPU();
						SetFullscreenState(false);
						g_cmdListManager->WaitForGPU();
						g_cmdListManager->ShutdownAdapters();
					}

					DeleteArray(g_frameFences);

					SAVE_RELEASE(g_syncFrameFence);
					SAVE_RELEASE(g_renderPass);
					if (g_cmdListManager != nullptr) { g_cmdListManager->ShutdownMemory(); }
					SAVE_RELEASE(g_SwapChain);

					if (g_cmdListManager != nullptr)
					{
						g_cmdListManager->ShutdownDevice();
						g_cmdListManager->Release();
					}
				}
				void CLOAK_CALL FlagUpdateScreen() 
				{ 
					UpdateScreenInfo usi;
					usi.shouldUpdate = true;
					usi.flag = g_cmdListManager->IncrementFence(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0);
					g_updScreen.exchange(usi);
				}
				void CLOAK_CALL WaitForFrame(In UINT frame) 
				{
					if (frame < g_usedFrameCount)
					{
						API::Helper::Lock lock(g_syncFrameFence);
						uint64_t val = g_frameFences[frame];
						lock.unlock();
						g_cmdListManager->WaitForFence(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, val);
					}
				}
				void CLOAK_CALL WaitForGPU()
				{
					g_cmdListManager->WaitForGPU();
				}
				Success(return != nullptr)
				Ret_notnull Impl::Rendering::IManager* CLOAK_CALL GetCommandListManager() { return g_cmdListManager; }
				void CLOAK_CALL SetRenderingPass(In API::Rendering::IRenderPass* pass)
				{
					SWAP(g_renderPass, pass);
				}
				UINT CLOAK_CALL GetFrameCount() { return g_usedFrameCount; }
				UINT CLOAK_CALL GetLastShownFrame() { return g_lastShownFrame; }
			}
			namespace Core_v2 {
				enum class SettingsState {
					REQUIRE_INIT,
					REQUIRE_UPDATE,
					UP_TO_DATE,
				};
				struct ResizeMsg {
					uint32_t Width : 32;
					uint32_t Height : 31;
					uint32_t Update : 1;
					CLOAK_CALL ResizeMsg(In UINT w, In UINT h, In_opt bool upd = true) : Width(static_cast<UINT>(w)), Height(static_cast<UINT>(h)), Update(upd == true ? TRUE : FALSE) {}
				};

				CE::RefPointer<Impl::Rendering::IManager> g_manager = nullptr;
				CE::RefPointer<Impl::Rendering::ISwapChain> g_swapChain = nullptr;
				CE::RefPointer<API::Rendering::IRenderPass> g_renderPass = nullptr;
				CE::RefPointer<API::Helper::ISyncSection> g_syncSettings = nullptr;

				bool g_vSync = false;
				bool g_inResize = false;
				bool g_occluded = false;
				UINT g_frameIndex = 0;
				size_t g_frameCount = 0;
				uint64_t* g_fences = nullptr;

				std::atomic<SettingsState> g_updSettings = SettingsState::REQUIRE_INIT;
				std::atomic<ResizeMsg> g_resize = ResizeMsg(0, 0, false);
				API::Global::Graphic::Settings g_curSettings;
				API::Global::Graphic::Settings g_newSettings;

				inline void CLOAK_CALL RecreateFences(In const API::Global::Graphic::Settings& set)
				{
					const size_t nfc = set.BackBufferCount + 1;
					if (nfc != g_frameCount)
					{
						const size_t mfc = min(nfc, g_frameCount);
						uint64_t* n = reinterpret_cast<uint64_t*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(uint64_t) * nfc));
						for (size_t a = 0; a < mfc; a++) { n[a] = g_fences[a]; }
						for (size_t a = g_frameCount; a < nfc; a++) { n[a] = 0; }
						API::Global::Memory::MemoryHeap::Free(g_fences);
						g_fences = n;
						g_frameCount = nfc;
					}
				}

				void CLOAK_CALL InitializePipeline()
				{
					API::Global::Graphic::Settings set;
					API::Global::Graphic::GetSettings(&set);
					g_manager = Impl::Rendering::IManager::Create(set.RenderMode);
					if (CloakCheckOK(g_manager != nullptr, API::Global::Debug::Error::GRAPHIC_NO_MANAGER, true))
					{
						HRESULT hRet = g_manager->Create(static_cast<UINT>(set.AdapterID));
						if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_FEATURE_UNSUPPORETD, true)) {}
					}
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncSettings));
				}
				void CLOAK_CALL InitializeWindow()
				{
					Impl::Interface::LoadScreen::Initialize();
					CLOAK_ASSUME(g_manager != nullptr);
					if (g_manager->IsReady())
					{
						API::Global::Graphic::Settings set;
						API::Global::Graphic::GetSettings(&set);
						g_curSettings = set;
						g_curSettings.WindowMode = API::Global::Graphic::WindowMode::WINDOW;
						RecreateFences(set);

						HRESULT hRet = S_OK;
						g_swapChain = g_manager->CreateSwapChain(set, &hRet);
						if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_NO_SWAPCHAIN, true)) { return; }
						if (set.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN)
						{
							g_swapChain->ResizeTarget(set.Resolution.Width, set.Resolution.Height);
							g_swapChain->SetFullscreenState(true);
						}
						Impl::Rendering::SWAP_CHAIN_RESULT r = g_swapChain->Present(false, Impl::Rendering::PRESENT_TEST);
						if (CloakCheckError(r != Impl::Rendering::SWAP_CHAIN_RESULT::FAILED, API::Global::Debug::Error::GRAPHIC_NO_SWAPCHAIN, true)) { return; }
						g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();
						if (g_renderPass == nullptr) { g_renderPass = CE::RefPointer<API::Rendering::IRenderPass>::Construct<DefaultRenderPass>(); }
						g_renderPass->OnInit(g_manager);
					}
				}
				void CLOAK_CALL Shutdown()
				{
					Impl::Interface::LoadScreen::Terminate();
					if (g_manager != nullptr)
					{
						g_manager->WaitForGPU();
						if (g_swapChain != nullptr) { g_swapChain->SetFullscreenState(false); }
						g_manager->WaitForGPU();
						g_manager->ShutdownAdapters();
					}

					API::Global::Memory::MemoryHeap::Free(g_fences);

					g_renderPass.Free();
					if (g_manager != nullptr) { g_manager->ShutdownMemory(); }
					g_swapChain.Free();
					if (g_manager != nullptr) { g_manager->ShutdownDevice(); }
					g_manager.Free();
					g_syncSettings.Free();
				}
				void CLOAK_CALL UpdateSettings(In const API::Global::Graphic::Settings& nset)
				{
					API::Helper::WriteLock lock(g_syncSettings);
					g_newSettings = nset;
					lock.unlock();
					
					SettingsState e = g_updSettings.load();
					SettingsState n = SettingsState::REQUIRE_UPDATE;
					while (e == SettingsState::UP_TO_DATE && g_updSettings.compare_exchange_weak(e, n) == false) {}
				}
				void CLOAK_CALL OnResize(In UINT width, In UINT height)
				{
					g_resize.store(ResizeMsg(width, height), std::memory_order_release);
				}
				void CLOAK_CALL Submit(In size_t threadID, In API::Global::Time etime)
				{
					g_manager->Update();
					ResizeMsg msg = g_resize.load(std::memory_order_acquire);
					if (msg.Update == TRUE)
					{
						ResizeMsg n = msg;
						n.Update = FALSE;
						while (g_resize.compare_exchange_weak(msg, n, std::memory_order_acq_rel) == false) 
						{
							n = msg;
							n.Update = FALSE;
						}
						HRESULT hRet = g_swapChain->ResizeBuffers(g_frameCount, n.Width, n.Height);
						CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_RESIZE, false);
						g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

						RECT rc;
						rc.left = 0;
						rc.top = 0;
						rc.right = n.Width;
						rc.bottom = n.Height;
						Impl::Global::Video::OnSizeChange(&rc);

						//TODO: Call g_renderPass->OnResize to resize GBuffers
						g_inResize = false;
					}
					else if (g_inResize == true) { return; }

					const SettingsState curSS = g_updSettings.exchange(SettingsState::UP_TO_DATE);
					if (curSS == SettingsState::UP_TO_DATE)
					{
						//TODO: Render Frame
					}
					else
					{
						API::Helper::ReadLock lock(g_syncSettings);
						const API::Global::Graphic::Settings last = g_curSettings;
						const API::Global::Graphic::Settings next = g_curSettings = g_newSettings;
						lock.unlock();
						g_manager->WaitForGPU();
						//TODO: Push settings to window handler

						g_vSync = next.VSync;
						g_renderPass->OnResize(g_manager, next, last, curSS == SettingsState::REQUIRE_INIT || last.Tessellation != next.Tessellation);
						if (last.BackBufferCount != next.BackBufferCount) { RecreateFences(next); }
						if (last.Resolution.Width != next.Resolution.Width || last.Resolution.Height != next.Resolution.Height || last.WindowMode != next.WindowMode)
						{
							g_swapChain->ResizeTarget(next.Resolution.Width, next.Resolution.Height);
							if ((last.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN) != (next.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN))
							{
								g_swapChain->SetFullscreenState(next.WindowMode == API::Global::Graphic::WindowMode::FULLSCREEN);
							}
							g_inResize = true;
						}
						//TODO:
						/*
						g_screenWidth = static_cast<uint32_t>(newSet.Resolution.Width);
						g_screenHeight = static_cast<uint32_t>(newSet.Resolution.Height);
						g_fov = newSet.FieldOfView;
						g_updatedClip = true;
						g_passEffects.MSAA = newSet.MSAA;
						g_passEffects.Bloom = newSet.Bloom;
						g_passEffects.Gamma = newSet.Gamma;
						if (newSet.MultiThreadedRendering != oldSet.MultiThreadedRendering || g_worker->Initialized() == false)
						{
							g_worker->EnableMultithreading(newSet.MultiThreadedRendering && g_cmdListManager->SupportMultiThreadedRendering(), g_cmdListManager);
						}
						g_checkDrawing = true;
						*/
					}
				}
				void CLOAK_CALL Present(In size_t threadID)
				{
					if (g_occluded == true || g_inResize == true)
					{
						Impl::Rendering::SWAP_CHAIN_RESULT hRet = g_swapChain->Present(false, Impl::Rendering::PRESENT_TEST);
						if (hRet == Impl::Rendering::SWAP_CHAIN_RESULT::OK) { g_occluded = false; }
					}
					else
					{
						Impl::Global::Task::IHelpWorker* worker = Impl::Global::Task::GetCurrentWorker(threadID);
						worker->HelpWorkUntil([]() { return g_manager->IsFenceComplete(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, g_fences[g_frameIndex]); }, API::Global::Threading::Flag::None, true);
						worker->HelpWorkWhile([]() {
							const Impl::Rendering::SWAP_CHAIN_RESULT pr = g_swapChain->Present(g_vSync, Impl::Rendering::PRESENT_NO_WAIT);
							if (pr != Impl::Rendering::SWAP_CHAIN_RESULT::NEED_RETRY)
							{
								if (pr == Impl::Rendering::SWAP_CHAIN_RESULT::OCCLUDED) { g_occluded = true; }
								return false;
							}
							return true;
						}, API::Global::Threading::Flag::None, true);
						g_fences[g_frameIndex] = g_manager->IncrementFence(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0);
					}
					g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();
				}
			}
		}
	}
}