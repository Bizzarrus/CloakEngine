#pragma once
#ifndef CE_E_GRAPHIC_DEFAULTSHADERLIB_H
#define CE_E_GRPAHIC_DEFAULTSHADERLIB_H

#include "CloakEngine/Rendering/PipelineState.h"
#include "CloakEngine/Rendering/Manager.h"
#include "CloakEngine/Global/Graphic.h"

//Calls the makro 'Call' for each shader. The first parameter given to 'Call' is the name of the shader, the second is a bool telling wether or not it's a complex shader. All parameters passed in
//after 'Call' and 'Sep' will then be passed down to every 'Call' as remaining parameters. 'Sep' is appended after each line.
#define CE_PER_SHADER(Call, Sep, ...) \
_EXPAND( Call ( Skybox , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Interface , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( FadeOut , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( ToneMapping , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Phase1Opaque , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Phase1Alpha , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Phase1Transparent , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Phase2ClusterAssignment , true , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Phase2ClusterCompression , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Phase3Opaque , true , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Phase3Alpha , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( Phase3Transparent , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( PreMSAA , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( AmbientOpaque , true , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( AmbientAlpha , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( AmbientTransparent , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( DirectionalOpaque , true , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( DirectionalAlpha , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( DirectionalTransparent , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( TransparentResolve , true , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( LuminancePass1 , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( LuminancePass2 , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( DirectionalShadowOpaque , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( DirectionalShadowAlpha , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( BlurX , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( BlurY , false , __VA_ARGS__ ) ) Sep  \
_EXPAND( Call ( Resize , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( BloomFinish , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( BloomBright , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( OpaqueResolve , false , __VA_ARGS__ ) ) Sep \
_EXPAND( Call ( IBL , true , __VA_ARGS__ ) ) Sep

#define SHADER_TYP_false(Name) Name
#define SHADER_TYP_true(Name) Name##Simple, Name##Complex
#define SHADER_TYP(Name, Complex,...) SHADER_TYP_##Complex(Name)

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			namespace DefaultShaderLib {
				enum class ShaderType {
					CE_PER_SHADER(SHADER_TYP,CE_SEP_COMMA)
				};
				bool CLOAK_CALL ResetPSO(In API::Rendering::IPipelineState* pso, In ShaderType type, In const API::Global::Graphic::Settings& gset, In const API::Rendering::Hardware& hardware);
			}
		}
	}
}

#endif