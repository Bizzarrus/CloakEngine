#pragma once
#ifndef CE_E_GRAPHIC_CORE_H
#define CE_E_GRAPHIC_CORE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Files/Shader.h"
#include "CloakEngine/Files/Image.h"
#include "CloakEngine/Helper/Logic.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Rendering/RenderPass.h"

#include "Implementation/Rendering/Manager.h"
#include "Implementation/Rendering/Context.h"
#include "Implementation/Rendering/PipelineState.h"
#include "Implementation/Files/Shader.h"
#include "Implementation/Global/Task.h"

#include "Engine/Graphic/GraphicLock.h"

#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			namespace Core {
				void CLOAK_CALL InitPipeline();
				void CLOAK_CALL Load();
				void CLOAK_CALL Start();
				void CLOAK_CALL UpdateSettings(In const API::Global::Graphic::Settings& newSet, In const API::Global::Graphic::Settings& oldSet, In Lock::SettingsFlag flag);
				void CLOAK_CALL Update(In unsigned long long etime);
				void CLOAK_CALL Submit();
				void CLOAK_CALL Present();
				void CLOAK_CALL FinalRelease();
				void CLOAK_CALL WaitForFrame(In UINT frame);
				void CLOAK_CALL WaitForGPU();
				void CLOAK_CALL ResizeBuffers(In UINT width, In UINT height);
				Ret_notnull Impl::Rendering::IManager* CLOAK_CALL GetCommandListManager();
				void CLOAK_CALL SetRenderingPass(In API::Rendering::IRenderPass* pass);
				UINT CLOAK_CALL GetFrameCount();
				UINT CLOAK_CALL GetLastShownFrame();
			}
			namespace Core_v2 {
				void CLOAK_CALL InitializePipeline();
				void CLOAK_CALL InitializeWindow();
				void CLOAK_CALL Shutdown();
				void CLOAK_CALL UpdateSettings(In const API::Global::Graphic::Settings& nset);
				void CLOAK_CALL OnResize(In UINT width, In UINT height);
				void CLOAK_CALL Submit(In size_t threadID, In API::Global::Time etime);
				void CLOAK_CALL Present(In size_t threadID);
			}
		}
	}
}

#endif