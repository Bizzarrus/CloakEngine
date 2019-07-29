#pragma once
#ifndef CE_E_GRAPHIC_DEFAULTRENDERPASS_H
#define CE_E_GRAPHIC_DEFAULTRENDERPASS_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/RenderPass.h"

#include "Implementation/Helper/SavePtr.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			class DefaultRenderPass : public virtual API::Rendering::IRenderPass, public Impl::Helper::SavePtr_v1::SavePtr {
				public:
					CLOAK_CALL DefaultRenderPass();
					virtual CLOAK_CALL ~DefaultRenderPass();
					virtual void CLOAK_CALL_THIS OnInit(In API::Rendering::IManager* manager) override;
					virtual void CLOAK_CALL_THIS OnResize(In API::Rendering::IManager* manager, In const API::Global::Graphic::Settings& newSet, In const API::Global::Graphic::Settings& oldSet, In bool updateShaders) override;
					virtual void CLOAK_CALL_THIS OnRenderCamera(In API::Rendering::IManager* manager, Inout API::Rendering::IContext** context, In API::Rendering::IRenderWorker* worker, In API::Components::ICamera* camera, In const API::Rendering::RenderTargetData& target, In const API::Rendering::CameraData& camData, In const API::Rendering::PassData& pass, In API::Global::Time etime) override;
					virtual void CLOAK_CALL_THIS OnRenderInterface(In API::Rendering::IManager* manager, Inout API::Rendering::IContext** context, In API::Rendering::IRenderWorker* worker, In API::Rendering::IColorBuffer* screen, In size_t numGuis, In API::Interface::IBasicGUI** guis, In const API::Rendering::PassData& pass, In API::Global::Time etime) override;
				protected:
					Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
			};
		}
	}
}

#pragma warning(pop)

#endif