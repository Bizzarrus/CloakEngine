#pragma once
#ifndef CC_API_SHADER_H
#define CC_API_SHADER_H

//#define NEW_PER_SHADER

#include "CloakCompiler/Defines.h"
#include "CloakCompiler/EncoderBase.h"
#include "CloakEngine/CloakEngine.h"

#include <string>
#include <functional>

namespace CloakCompiler {
	CLOAKCOMPILER_API_NAMESPACE namespace API {
		namespace Shader {
#ifdef DISABLED
			namespace Shader_v1002 {
				constexpr CloakEngine::Files::FileType FileType{ "MultiShader","CESF",1002 };
				enum class Completed { YES, WARNING, NO, NOT_USED, NOT_CHANGED, COMPILING, TEMP_CHECK, TEMP_WRITE };
				enum class ShaderType { VS, HS, DS, GS, PS, CS, PSO };
				enum class Configuration {
					None				= 0x00,
					MSAA				= 0x01,
					Complex				= 0x02,
					Tessellation		= 0x04,
					Compute				= 0x08,
					FXAA				= 0x10,

					MSAA_NoAA			= MSAA | 0x0100,
					MSAA_x2				= MSAA | 0x0200,
					MSAA_x4				= MSAA | 0x0300,
					MSAA_x8				= MSAA | 0x0400,

					Complex_S			= Complex | 0x1000,
					Complex_C			= Complex | 0x2000,
					Complex_S_NoAA		= Complex_S | 0x0100,
					Complex_S_x2		= Complex_S | 0x0200,
					Complex_S_x4		= Complex_S | 0x0300,
					Complex_S_x8		= Complex_S | 0x0400,
					Complex_C_x2		= Complex_C | 0x0200,
					Complex_C_x4		= Complex_C | 0x0300,
					Complex_C_x8		= Complex_C | 0x0400,
					
					Tessellation_On		= Tessellation | 0x04000,
					Tessellation_Off	= Tessellation | 0x08000,
					
					Compute_W4			= Compute | 0x10000,
					Compute_W32			= Compute | 0x20000,
					Compute_W64			= Compute | 0x40000,
					Compute_W128		= Compute | 0x80000,
					
				};
				DEFINE_ENUM_FLAG_OPERATORS(Configuration);
				struct SINGLE_SHADER_INFO {
					bool Use = false;
					std::u16string FilePath;
					std::string Main;
				};
				struct SHADER_INFO_BASIC {
					bool Use = false;
					SINGLE_SHADER_INFO VS;
					SINGLE_SHADER_INFO PS;
					SINGLE_SHADER_INFO GS;
					SINGLE_SHADER_INFO HS;
					SINGLE_SHADER_INFO DS;
					SINGLE_SHADER_INFO CS;
				};
				struct SHADER_INFO_MSAA {
					bool Use = false;
					SHADER_INFO_BASIC NoAA;
					SHADER_INFO_BASIC MSAAx2;
					SHADER_INFO_BASIC MSAAx4;
					SHADER_INFO_BASIC MSAAx8;
				};
				struct SHADER_INFO_COMPLEX_SIMPLE {
					bool Use = false;
					SHADER_INFO_BASIC NoAA;
					SHADER_INFO_BASIC MSAAx2;
					SHADER_INFO_BASIC MSAAx4;
					SHADER_INFO_BASIC MSAAx8;
				};
				struct SHADER_INFO_COMPLEX_COMPLEX {
					bool Use = false;
					SHADER_INFO_BASIC MSAAx2;
					SHADER_INFO_BASIC MSAAx4;
					SHADER_INFO_BASIC MSAAx8;
				};
				struct SHADER_INFO_COMPLEX {
					bool Use = false;
					SHADER_INFO_COMPLEX_SIMPLE Simple;
					SHADER_INFO_COMPLEX_COMPLEX Complex;
				};
				struct SHADER_INFO_TESSELLATION {
					bool Use = false;
					SHADER_INFO_BASIC NoTessellation;
					SHADER_INFO_BASIC Tessellation;
				};
				struct SHADER_INFO_COMPUTE {
					bool Use = false;
					SHADER_INFO_BASIC Wave4;
					SHADER_INFO_BASIC Wave32;
					SHADER_INFO_BASIC Wave64;
					SHADER_INFO_BASIC Wave128;
				};
				struct SHADER_INFO_FXAA {
					bool Use = false;
					SHADER_INFO_BASIC FXAA;
				};

				enum class ShaderInfoType { None, MSAA, Complex, Tessellation, Compute, FXAA };
				enum WRITE_MODE {
					WRITE_MODE_NONE = 0x0,
					WRITE_MODE_FILE = 0x1,
					WRITE_MODE_HEADER = 0x2,
					WRITE_MODE_PSO = 0x4,
				};
				DEFINE_ENUM_FLAG_OPERATORS(WRITE_MODE);
				struct SHADER_INFO_VERSION {
					bool Use = false;
					SHADER_INFO_BASIC None;
					SHADER_INFO_MSAA MSAA;
					SHADER_INFO_COMPLEX Complex;
					SHADER_INFO_TESSELLATION Tessellation;
					SHADER_INFO_COMPUTE Compute;
					SHADER_INFO_FXAA FXAA;
				};
#define CC_CALL_INFO(Name,...) SHADER_INFO_VERSION Name
				struct SHADER_INFO {
					ShaderInfoType Type;
					std::string Name;
					CE_PER_SHADER_VERSION(CC_CALL_INFO,CE_SEP_SIMECOLON)
				};
#undef CC_CALL_INFO
				struct SHADER_DESC {
					bool Debug = false;
					CloakEngine::Rendering::PIPELINE_STATE_DESC PSO;
					CloakEngine::Rendering::RootDescription Root;
				};
				struct SHADER_ENCODE {
					WRITE_MODE Mode;
					CloakEngine::Files::UFI Header;
					std::string ShaderName;
				};
				struct SHADER {
					SHADER_DESC Desc;
					SHADER_INFO Info;
				};
				struct ResponseInfo {
					Completed Status;
					size_t ShaderIndex;
					std::string ShaderName;
					Configuration Configuration;
					ShaderType ShaderType;
					CloakEngine::Rendering::SHADER_MODEL Version;
					std::string Message;
				};
				CLOAKCOMPILER_API void CLOAK_CALL Encode(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const SHADER_ENCODE& desc, In const CloakEngine::List<SHADER>& shader, In_opt std::function<void(const ResponseInfo&)> response = false);
			}
#endif
			namespace Shader_v1003 {
				constexpr CloakEngine::Files::FileType FileType{ "MultiShader","CESF",1003 };

				enum class Status 
				{ 
					FINISHED,
					FAILED,
					WARNING,
					INFO,
					SKIPPED,
				};
				enum class ShaderPass { 
					VS = 0, 
					HS = 1, 
					DS = 2, 
					GS = 3, 
					PS = 4, 
					CS = 5, 
				};
				constexpr ShaderPass ALL_PASSES[] = {
					ShaderPass::VS,
					ShaderPass::HS,
					ShaderPass::DS,
					ShaderPass::GS,
					ShaderPass::PS,
					ShaderPass::CS
				};
				
				class Configuration {
					private:
						enum class Mask {
							Complex			= 0x00000001,
							Tessellation	= 0x00000002,
							Debug			= 0x00000004,
							MSAA			= 0x000000F0,
							WaveSize		= 0x0000FF00,
						};
						static constexpr uint32_t CLOAK_CALL GetMaskShift(In Mask m)
						{
							uint32_t s = 0;
							while (((static_cast<uint32_t>(m) >> s) & 0x1) == 0) { s++; }
							return s;
						}

						uint32_t m_value;

						inline void CLOAK_CALL_THIS Set(In Mask m, In uint32_t v)
						{
							const uint32_t xm = static_cast<uint32_t>(m);
							m_value = (m_value & ~xm) | ((v << GetMaskShift(m)) & xm);
						}
						inline uint32_t CLOAK_CALL_THIS Get(In Mask m) const
						{
							const uint32_t xm = static_cast<uint32_t>(m);
							return (m_value & xm) >> GetMaskShift(m);
						}
					public:
						CLOAK_CALL Configuration() : m_value(0) {}

						void CLOAK_CALL_THIS SetComplex(In bool v) { Set(Mask::Complex, v ? 1 : 0); }
						void CLOAK_CALL_THIS SetTessellation(In bool v) { Set(Mask::Tessellation, v ? 1 : 0); }
						void CLOAK_CALL_THIS SetDebug(In bool v) { Set(Mask::Debug, v ? 1 : 0); }
						void CLOAK_CALL_THIS SetMSAA(In uint8_t v) { Set(Mask::MSAA, v); }
						void CLOAK_CALL_THIS SetWaveSize(In uint8_t v) { Set(Mask::WaveSize, v); }

						bool CLOAK_CALL_THIS GetComplex() const { return Get(Mask::Complex) == 1; }
						bool CLOAK_CALL_THIS GetTessellation() const { return Get(Mask::Tessellation) == 1; }
						bool CLOAK_CALL_THIS GetDebug() const { return Get(Mask::Debug) == 1; }
						uint8_t CLOAK_CALL_THIS GetMSAA() const { return Get(Mask::MSAA); }
						uint8_t CLOAK_CALL_THIS GetWaveSize() const { return Get(Mask::WaveSize); }
				};
				enum WRITE_MODE {
					WRITE_MODE_NONE = 0x0,
					WRITE_MODE_FILE = 0x1,
					WRITE_MODE_LIB = 0x2,
					WRITE_MODE_PSO = 0x4,
				};
				DEFINE_ENUM_FLAG_OPERATORS(WRITE_MODE);
				enum SHADER_TYPE {
					SHADER_TYPE_NONE = 0,
					SHADER_TYPE_MSAA = (1 << 0),
					SHADER_TYPE_COMPLEX = (1 << 1) | SHADER_TYPE_MSAA,
					SHADER_TYPE_TESSELLATION = (1 << 2),
					SHADER_TYPE_WAVESIZE = (1 << 3),
				};
				DEFINE_ENUM_FLAG_OPERATORS(SHADER_TYPE);
				enum OPTION {
					OPTION_NONE = 0,
					OPTION_DEBUG = (1 << 0),
					OPTION_16_BIT_TYPES = (1 << 1),
					OPTION_OPTIMIZATION_LEVEL_0 = (1 << 2), //Lowest optimization level
					OPTION_OPTIMIZATION_LEVEL_1 = (1 << 3),
					OPTION_OPTIMIZATION_LEVEL_2 = (1 << 4),
					OPTION_OPTIMIZATION_LEVEL_3 = (1 << 5), //Highest optimization level
					OPTION_OPTIMIZATION_LEVEL_HIGHEST = OPTION_OPTIMIZATION_LEVEL_3,
					OPTION_OPTIMIZATION_LEVEL_LOWEST = OPTION_OPTIMIZATION_LEVEL_0,

					OPTION_DEBUG_SKIP_OPTIMIZATION = (1 << 31), // Compiler skips optimization completly. If OPTION_DEBUG is set, this option will not have an effect
				};
				DEFINE_ENUM_FLAG_OPERATORS(OPTION);
				enum USAGE_MASK : uint32_t {
					USAGE_MASK_NEVER = 0,
					USAGE_MASK_ALLWAYS = ~0Ui32,

					USAGE_MASK_COMPLEX_ON = 1 << 0, //Use only during complex compilation
					USAGE_MASK_COMPLEX_OFF = 1 << 1, //Use only during simple compilation
					USAGE_MASK_COMPLEX_ALLWAYS = USAGE_MASK_COMPLEX_OFF | USAGE_MASK_COMPLEX_ON, //Use independend of complex/simple setting

					USAGE_MASK_TESSELLATION_ON = 1 << 2, //Use only if tessellation is on
					USAGE_MASK_TESSELLATION_OFF = 1 << 3, //Use only if tessellation is off
					USAGE_MASK_TESSELLATION_ALLWAYS = USAGE_MASK_TESSELLATION_ON | USAGE_MASK_TESSELLATION_OFF, //Use independend of tessellation setting

					USAGE_MASK_WAVE_SIZE_4 = 1 << 4, //Use only if wave size is set to 4
					USAGE_MASK_WAVE_SIZE_32 = 1 << 5, //Use only if wave size is set to 32
					USAGE_MASK_WAVE_SIZE_64 = 1 << 6, //Use only if wave size is set to 64
					USAGE_MASK_WAVE_SIZE_128 = 1 << 7, //Use only if wave size is set to 128
					USAGE_MASK_WAVE_SIZE_ALLWAYS = USAGE_MASK_WAVE_SIZE_4 | USAGE_MASK_WAVE_SIZE_32 | USAGE_MASK_WAVE_SIZE_64 | USAGE_MASK_WAVE_SIZE_128, //Use for all wave sizes

					USAGE_MASK_MSAA_1 = 1 << 8, //Use only if MSAA is set to 1
					USAGE_MASK_MSAA_2 = 1 << 9, //Use only if MSAA is set to 2
					USAGE_MASK_MSAA_4 = 1 << 10, //Use only if MSAA is set to 4
					USAGE_MASK_MSAA_8 = 1 << 11, //Use only if MSAA is set to 8
					USAGE_MASK_MSAA_ALLWAYS = USAGE_MASK_MSAA_1 | USAGE_MASK_MSAA_2 | USAGE_MASK_MSAA_4 | USAGE_MASK_MSAA_8, //Use for all MSAA counts
				};
				DEFINE_ENUM_FLAG_OPERATORS(USAGE_MASK);

				struct Response {
					Status Status;
					std::string Message;
					struct {
						size_t ID;
						ShaderPass Pass;
						CloakEngine::Rendering::SHADER_MODEL Model;
						Configuration Configuration;
					} Shader;
				};
				struct SHADER_DEFINE {
					std::string Name;
					std::string Value;
				};
				struct SHADER_NAME {
					USAGE_MASK Use = USAGE_MASK_ALLWAYS;
					ShaderPass Pass;
					CE::Rendering::SHADER_MODEL Model;
					std::string MainFuncName;
					CloakEngine::Files::UFI File;
					const SHADER_DEFINE* Defines;
					size_t DefineCount;
				};
				struct SHADER_DESC {
					SHADER_TYPE Type;
					OPTION Options;
					const SHADER_NAME* Shaders;
					size_t ShaderCount;
				};
				struct SHADER {
					std::string Name;
					SHADER_DESC Desc;
				};
				struct SHADER_LIB_COMPILE {
					CloakEngine::Files::UFI Path;
					bool WriteNamespaceAlias;
					LibGenerator Generator;
					bool SharedLib;
				};
				struct SHADER_ENCODE {
					WRITE_MODE Mode;
					std::string ShaderName;
					SHADER_LIB_COMPILE Lib;
				};

				CLOAKCOMPILER_API void CLOAK_CALL Encode(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const SHADER_ENCODE& desc, In const CE::List<SHADER>& shaders, In std::function<void(const Response&)> response);
			}
		}
	}
}

#endif