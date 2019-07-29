#pragma once
#ifndef CE_API_RENDERING_PIPELINESTATE_H
#define CE_API_RENDERING_PIPELINESTATE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Files/Shader.h"
#include "CloakEngine/Rendering/RootDescription.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace PipelineState_v1 {
				CLOAK_INTERFACE_ID("{731FB2B1-FC92-435E-A5FD-D6ECA1980378}") IPipelineState : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual bool CLOAK_CALL_THIS Reset(In const API::Rendering::PIPELINE_STATE_DESC& desc, In const API::Rendering::RootDescription& root) = 0;
						virtual bool CLOAK_CALL_THIS Reset(In const SHADER_SET& shader, In size_t dataSize, In_reads_bytes(dataSize) const void* data) = 0;
						virtual bool CLOAK_CALL_THIS Reset(In const API::Global::Graphic::Settings& set, In API::Files::IShader* shader) = 0;
				};
			}
		}
	}
}

#endif