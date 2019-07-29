#pragma once
#ifndef CE_IMPL_RENDERING_RENDERWORKER_H
#define CE_IMPL_RENDERING_RENDERWORKER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/RenderPass.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace RenderWorker_v1 {
				CLOAK_INTERFACE_ID("{88A4C5D8-0730-4B70-AFD2-2F313E52E4D3}") IRenderWorker : public virtual API::Rendering::RenderPass_v1::IRenderWorker {

				};
			}
		}
	}
}

#endif