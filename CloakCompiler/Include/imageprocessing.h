///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  This file was automaticly created with CloakEditor©. Do not change this file!                            //
//                                                                                                           //
//  Copyright 2019 Thomas Degener                                                                            //
//                                                                                                           //
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,                      //
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR                 //
//  PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE                 //
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,          //
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.    //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef CE_SHADER_ImageProcessing_1394302934_H
#define CE_SHADER_ImageProcessing_1394302934_H

#ifdef CLOAKENGINE_VERSION
	#include "CloakEngine/Defines.h"
	#include "CloakEngine/Helper/Types.h"
	#include "CloakEngine/Rendering/Defines.h"
	#include "CloakEngine/Global/Graphic.h"
#endif

#ifdef _MSC_VER
	#pragma comment(lib, "ImageProcessing.lib")
#endif

#include <cstddef>
#include <cstdint>

namespace CloakEngine {
	namespace Shader {
		namespace ImageProcessing {
			constexpr std::uint32_t Version = 1003;
			enum ShaderModel {
				HLSL_5_0 = 0,
				HLSL_5_1 = 1,
				HLSL_6_0 = 2,
				HLSL_6_1 = 3,
				HLSL_6_2 = 4,
				HLSL_6_3 = 5,
				HLSL_6_4 = 6,
				HLSL_6_5 = 7,
			};
#ifdef CLOAKENGINE_VERSION
			inline ShaderModel CLOAK_CALL GetModel(In const CloakEngine::API::Rendering::SHADER_MODEL& model)
			{
				if(model == CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::SHADER_HLSL, 5, 0, 0)) { return ShaderModel::HLSL_5_0; }
				else if(model >= CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::SHADER_HLSL, 5, 1, 0)) { return ShaderModel::HLSL_5_1; }
				else if(model == CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::SHADER_DXIL, 6, 0, 0)) { return ShaderModel::HLSL_6_0; }
				else if(model == CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::SHADER_DXIL, 6, 1, 0)) { return ShaderModel::HLSL_6_1; }
				else if(model == CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::SHADER_DXIL, 6, 2, 0)) { return ShaderModel::HLSL_6_2; }
				else if(model == CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::SHADER_DXIL, 6, 3, 0)) { return ShaderModel::HLSL_6_3; }
				else if(model == CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::SHADER_DXIL, 6, 4, 0)) { return ShaderModel::HLSL_6_4; }
				else if(model >= CloakEngine::API::Rendering::SHADER_MODEL(CloakEngine::API::Rendering::SHADER_DXIL, 6, 5, 0)) { return ShaderModel::HLSL_6_5; }
				return static_cast<ShaderModel>(8);
			}
#endif
			inline namespace Shaders {
				namespace RadianceSecondSum {
					void __fastcall GetVS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetVS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetVS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetVS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetVS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetVS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetVS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetVS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetVS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetHS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetHS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetHS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetHS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetHS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetHS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetHS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetHS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetHS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetDS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetDS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetDS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetDS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetDS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetDS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetDS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetDS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetDS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetGS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetGS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetGS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetGS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetGS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetGS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetGS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetGS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetGS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetPS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetPS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetPS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetPS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetPS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetPS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetPS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetPS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetPS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetCS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetCS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetCS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetCS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetCS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetCS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetCS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetCS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetCS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
				}
				namespace RadianceFirstSum {
					void __fastcall GetVS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetVS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetVS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetVS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetVS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetVS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetVS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetVS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetVS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetHS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetHS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetHS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetHS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetHS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetHS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetHS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetHS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetHS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetDS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetDS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetDS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetDS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetDS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetDS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetDS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetDS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetDS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetGS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetGS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetGS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetGS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetGS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetGS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetGS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetGS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetGS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetPS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetPS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetPS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetPS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetPS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetPS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetPS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetPS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetPS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
					void __fastcall GetCS(ShaderModel model, std::uint32_t waveSize, bool tessellation, std::uint32_t msaaCount, bool complex, const void** result, std::size_t* resultLength);
#ifdef CLOAKENGINE_VERSION
					inline CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetCS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In_opt std::uint32_t waveSize = 4, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false)
					{
						CloakEngine::API::Rendering::SHADER_CODE res;
						GetCS(GetModel(model), waveSize, tessellation, msaaCount, complex, &res.Code, &res.Length);
						return res;
					}
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetCS(In const CloakEngine::API::Rendering::Hardware& hardware, In_opt bool tessellation = false, In_opt std::uint32_t msaaCount = 1, In_opt bool complex = false) { return GetCS(hardware.ShaderModel, hardware.LaneCount, tessellation, msaaCount, complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetCS(In const CloakEngine::API::Rendering::SHADER_MODEL& model, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt std::uint32_t waveSize = 4, In_opt bool complex = false) { return GetCS(model, waveSize, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
					CLOAK_FORCEINLINE CloakEngine::API::Rendering::SHADER_CODE CLOAK_CALL GetCS(In const CloakEngine::API::Rendering::Hardware& hardware, In const CloakEngine::API::Global::Graphic::Settings& settings, In_opt bool complex = false) { return GetCS(hardware.ShaderModel, hardware.LaneCount, settings.Tessellation, static_cast<uint32_t>(settings.MSAA), complex); }
#endif
				}
			}
		}
	}
}
//Namespace Alias:
namespace ImageProcessing = CloakEngine::Shader::ImageProcessing;

#endif
