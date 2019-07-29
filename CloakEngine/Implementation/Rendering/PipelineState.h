#pragma once
#ifndef CE_IMPL_RENDEIRNG_PIPELINESTATE_H
#define CE_IMPL_RENDEIRNG_PIPELINESTATE_H

#include "CloakEngine/Rendering/PipelineState.h"
#include "CloakEngine/Files/Shader.h"
#include "CloakEngine/Global/Graphic.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/RootSignature.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace PipelineState_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{2D2E7011-F5FD-4D7C-8A38-0362BAF74211}") IPipelineState : public virtual API::Rendering::IPipelineState{
					public:
						virtual IRootSignature* CLOAK_CALL_THIS GetRoot() const = 0;
						virtual bool CLOAK_CALL_THIS UseTessellation() const = 0;
						virtual PIPELINE_STATE_TYPE CLOAK_CALL_THIS GetType() const = 0;
				};
			}
		}
	}
}

#endif