#pragma once
#ifndef CE_IMPL_FILES_SHADER_H
#define CE_IMPL_FILES_SHADER_H

#include "CloakEngine/Files/Shader.h"
#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Rendering/Manager.h"
#include "CloakEngine/Rendering/RootDescription.h"
#include "Implementation/Files/FileHandler.h"
#include "Implementation/Rendering/PipelineState.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace Shader_v1 {
				CLOAK_INTERFACE_BASIC CLOAK_UUID("{6042BA77-65CA-45A8-8667-C5BE1A2D4800}") IShader : public API::Files::Shader_v1::IShader {
					public:
						virtual bool CLOAK_CALL_THIS GetShaderData(Out API::Rendering::PIPELINE_STATE_DESC* desc, Out API::Rendering::RootDescription* root, In const API::Global::Graphic::Settings& gset, In API::Rendering::SHADER_MODEL version, In const API::Rendering::Hardware& hardware) const = 0;
				};

				class CLOAK_UUID("{88FD6C08-F220-489B-9CF9-7C4ED5DB4311}") Shader : public IShader, public virtual FileHandler_v1::RessourceHandler {
					public:
						CLOAK_CALL_THIS Shader();
						virtual CLOAK_CALL_THIS ~Shader();

						virtual bool CLOAK_CALL_THIS GetShaderData(Out API::Rendering::PIPELINE_STATE_DESC* desc, Out API::Rendering::RootDescription* root, In const API::Global::Graphic::Settings& gset, In API::Rendering::SHADER_MODEL version, In const API::Rendering::Hardware& hardware) const override;
					protected:
						virtual LoadResult CLOAK_CALL_THIS iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) override;
						virtual void CLOAK_CALL_THIS iUnload() override;
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
				};
				DECLARE_FILE_HANDLER(Shader, "{836942EE-B517-4EDD-B5FC-B412249F003E}");
			}
		}
	}
}

#pragma warning(pop)

#endif