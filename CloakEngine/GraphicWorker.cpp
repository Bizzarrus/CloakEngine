#include "stdafx.h"
#include "Engine/Graphic/Worker.h"

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {

#pragma warning(push)
#pragma warning(disable: 4250)
			class Worker : public virtual Impl::Rendering::IRenderWorker {
				public:
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
			};
#pragma warning(pop)


			Impl::Rendering::IRenderWorker* CLOAK_CALL GetWorker()
			{

			}
		}
	}
}
