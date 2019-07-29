#include "stdafx.h"
#include "Implementation/Files/Shader.h"

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Files/Compressor.h"

#include "Engine/FileLoader.h"
#include "Engine/Graphic/Core.h"

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Shader_v1 {
				constexpr API::Files::FileType g_fileType = { "MultiShader","CESF",1001 };
#ifdef DISABLED
				CLOAK_CALL_THIS ShaderData::~ShaderData()
				{
					if (data != nullptr) { DeleteArray(data); data = nullptr; }
				}

				namespace ReadShaderBasic {
					inline void CLOAK_CALL reset(Out_opt ShaderData* data)
					{
						if (data != nullptr)
						{
							if (data->data != nullptr) { DeleteArray(data->data); }
							data->data = nullptr;
							data->size = 0;
						}
					}
					inline void CLOAK_CALL reset(Inout SingleShader* data)
					{
						reset(&data->VS);
						reset(&data->HS);
						reset(&data->DS);
						reset(&data->GS);
						reset(&data->PS);
						reset(&data->CS);
					}
					inline void CLOAK_CALL reset(Inout AntiAliasedShader* data)
					{
						reset(&data->NoAA);
						reset(&data->FXAA);
						reset(&data->MSAAx2);
						reset(&data->MSAAx4);
						reset(&data->MSAAx8);
					}
					inline void CLOAK_CALL reset(Inout ComplexShader* data)
					{
						reset(&data->NoAA);
						reset(&data->MSAAx2);
						reset(&data->MSAAx4);
						reset(&data->MSAAx8);
					}
					inline void CLOAK_CALL reset(Inout TessellationShader* data)
					{
						reset(&data->NoTessellation);
						reset(&data->Tessellation);
					}
					inline void CLOAK_CALL readShader(In API::Files::IReader* read, Outref ShaderData& data, In API::Files::FileVersion version, In bool use)
					{
						if (use) { use = read->ReadBits(1) == 1; }
						if (use)
						{
							size_t sbl = 0;
							while (read->ReadBits(1) == 1) { sbl += 1 << 10; }
							sbl += static_cast<size_t>(read->ReadBits(10));
							if (sbl > 0)
							{
								BYTE* c = NewArray(BYTE, sbl);
								read->ReadBytes(sbl, c, sbl);
								data.data = c;
								data.size = sbl;
							}
							else
							{
								if (data.data != nullptr) { DeleteArray(data.data); }
								data.data = nullptr;
								data.size = 0;
							}
						}
						else
						{
							if (data.data != nullptr) { DeleteArray(data.data); }
							data.data = nullptr;
							data.size = 0;
						}
					}
					inline void CLOAK_CALL readShader(In API::Files::IReader* read, Outref SingleShader& data, In API::Files::FileVersion version, In bool use)
					{
						if (use) { use = read->ReadBits(1) == 1; }
						readShader(read, data.CS, version, use);
						readShader(read, data.DS, version, use);
						readShader(read, data.GS, version, use);
						readShader(read, data.HS, version, use);
						readShader(read, data.PS, version, use);
						readShader(read, data.VS, version, use);
					}
					inline void CLOAK_CALL readShader(In API::Files::IReader* read, Outref AntiAliasedShader& data, In API::Files::FileVersion version)
					{
						const bool use = read->ReadBits(1) == 1;
						readShader(read, data.NoAA, version, use);
						readShader(read, data.FXAA, version, use);
						readShader(read, data.MSAAx2, version, use);
						readShader(read, data.MSAAx4, version, use);
						readShader(read, data.MSAAx8, version, use);
					}
					inline void CLOAK_CALL readShader(In API::Files::IReader* read, Outref ComplexShader& data, In API::Files::FileVersion version)
					{
						const bool use = read->ReadBits(1) == 1;
						readShader(read, data.NoAA, version, use);
						readShader(read, data.MSAAx2, version, use);
						readShader(read, data.MSAAx4, version, use);
						readShader(read, data.MSAAx8, version, use);
					}
					inline void CLOAK_CALL readShader(In API::Files::IReader* read, Outref TessellationShader& data, In API::Files::FileVersion version)
					{
						const bool use = read->ReadBits(1) == 1;
						readShader(read, data.NoTessellation, version, use);
						readShader(read, data.Tessellation, version, use);
					}
					inline void CLOAK_CALL copyShader(In const ShaderData& data, Inout ShaderData* res)
					{
						if (data.size > 0)
						{
							if (res->data != nullptr) { DeleteArray(res->data); }
							res->size = data.size;
							if (data.size > 0)
							{
								BYTE* c = NewArray(BYTE, data.size);
								for (size_t a = 0; a < data.size; a++) { c[a] = data.data[a]; }
								res->data = c;
							}
							else { res->data = nullptr; }
						}
						else
						{
							if (res->data != nullptr) { DeleteArray(res->data); res->data = nullptr; }
							res->size = 0;
						}
					}
					inline void CLOAK_CALL copyShader(In const SingleShader& data, Inout SingleShader* res)
					{
						copyShader(data.VS, &res->VS);
						copyShader(data.PS, &res->PS);
						copyShader(data.HS, &res->HS);
						copyShader(data.GS, &res->GS);
						copyShader(data.DS, &res->DS);
						copyShader(data.CS, &res->CS);
					}
					inline void CLOAK_CALL copyClear(Inout ShaderData* res)
					{
						if (res->data != nullptr) { DeleteArray(res->data); res->data = nullptr; }
						res->size = 0;
					}
					inline void CLOAK_CALL copyClear(Inout SingleShader* res)
					{
						copyClear(&res->VS);
						copyClear(&res->PS);
						copyClear(&res->HS);
						copyClear(&res->GS);
						copyClear(&res->DS);
						copyClear(&res->CS);
					}
					inline void CLOAK_CALL copyShader(In API::Global::Graphic::AntiAliasing aaSet, In const AntiAliasedShader& data, Inout SingleShader* res)
					{
						switch (aaSet)
						{
							case CloakEngine::API::Global::Graphic::AntiAliasing::NONE:
								copyShader(data.NoAA, res);
								break;
							case CloakEngine::API::Global::Graphic::AntiAliasing::FXAA:
								copyShader(data.FXAA, res);
								break;
							case CloakEngine::API::Global::Graphic::AntiAliasing::MSAAx2:
								copyShader(data.MSAAx2, res);
								break;
							case CloakEngine::API::Global::Graphic::AntiAliasing::MSAAx4:
								copyShader(data.MSAAx4, res);
								break;
							case CloakEngine::API::Global::Graphic::AntiAliasing::MSAAx8:
								copyShader(data.MSAAx8, res);
								break;
							default:
								break;
						}
					}
					inline void CLOAK_CALL copyShaderComplex(In API::Global::Graphic::AntiAliasing aaSet, In const ComplexShader& data, Inout SingleShader* res, In bool complex)
					{
						switch (aaSet)
						{
							case CloakEngine::API::Global::Graphic::AntiAliasing::NONE:
								if (complex == false) { copyShader(data.NoAA, res); return; }
								break;
							case CloakEngine::API::Global::Graphic::AntiAliasing::MSAAx2:
								if (complex == true) { copyShader(data.MSAAx2, res); return; }
								break;
							case CloakEngine::API::Global::Graphic::AntiAliasing::MSAAx4:
								if (complex == true) { copyShader(data.MSAAx4, res); return; }
								break;
							case CloakEngine::API::Global::Graphic::AntiAliasing::MSAAx8:
								if (complex == true) { copyShader(data.MSAAx8, res); return; }
								break;
							default:
								break;
						}
						copyClear(res);
					}
					inline void CLOAK_CALL copyShader(In bool useTess, In const TessellationShader& data, Inout SingleShader* res)
					{
						if (useTess) { copyShader(data.Tessellation, res); }
						else { copyShader(data.NoTessellation, res); }
					}
				}

#define CE_SHADER_RESET(Name) ReadShaderBasic::reset(&data->Name)
#define CE_SHADER_READ_SING(Name) ReadShaderBasic::readShader(read,data.Name,version,true)
#define CE_SHADER_READ_ALIS(Name) ReadShaderBasic::readShader(read,data.Name,version)
#define CE_SHADER_READ_COMP(Name) ReadShaderBasic::readShader(read,data.Name,version)
#define CE_SHADER_READ_TESS(Name) ReadShaderBasic::readShader(read,data.Name,version)

				inline void CLOAK_CALL reset(Inout VersionData* data)
				{
					//CE_PER_SHADER(CE_SHADER_RESET, CE_SHADER_RESET, CE_SHADER_RESET, CE_SHADER_RESET, CE_SEP_SIMECOLON)
				}
				inline void CLOAK_CALL reset(Inout Data* data)
				{
					if (data != nullptr)
					{
						reset(&data->DX_5_1);
					}
				}
				inline void CLOAK_CALL readShader(In API::Files::IReader* read, Outref VersionData& data, In API::Files::FileVersion version)
				{
					//CE_PER_SHADER(CE_SHADER_READ_SING, CE_SHADER_READ_ALIS, CE_SHADER_READ_COMP, CE_SHADER_READ_TESS, CE_SEP_SIMECOLON)
				}
				inline void CLOAK_CALL readShader(In API::Files::IReader* read, Inout_opt Data* data, In API::Files::FileVersion version)
				{
					if (data != nullptr)
					{
						readShader(read, data->DX_5_1, version);
					}
				}

#define CE_COPY_SHADER_SING(Name) case Rendering::ShaderTyp::Name: ReadShaderBasic::copyShader(data.Name,res); break
#define CE_COPY_SHADER_ALIS(Name) case Rendering::ShaderTyp::Name: ReadShaderBasic::copyShader(aaSet,data.Name,res); break
#define CE_COPY_SHADER_COMP(Name) case Rendering::ShaderTyp::Name##Simple: ReadShaderBasic::copyShaderComplex(API::Global::Graphic::AntiAliasing::NONE,data.Name,res,false); break; case Rendering::ShaderTyp::Name##Complex: ReadShaderBasic::copyShaderComplex(aaSet,data.Name,res,true); break
#define CE_COPY_SHADER_TESS(Name) case Rendering::ShaderTyp::Name: ReadShaderBasic::copyShader(useTess,data.Name,res); break

				inline void CLOAK_CALL copyShader(In Rendering::ShaderTyp pass, In API::Global::Graphic::AntiAliasing aaSet, In bool useTess, In const VersionData& data, Inout SingleShader* res)
				{
					switch (pass)
					{
						CE_PER_SHADER(CE_COPY_SHADER_SING, CE_COPY_SHADER_ALIS, CE_COPY_SHADER_COMP, CE_COPY_SHADER_TESS, CE_SEP_SIMECOLON)
						default:
							break;
					}
				}
				inline void CLOAK_CALL copyShader(In Rendering::ShaderTyp pass, In Rendering::ShaderVersion version, In API::Global::Graphic::AntiAliasing aaSet, In bool useTess, In const Data& data, Inout SingleShader* res)
				{
					if (res != nullptr)
					{
						ReadShaderBasic::reset(res);
						switch (version)
						{
							case CloakEngine::Impl::Rendering::ShaderVersion::DX_5_1:
								copyShader(pass, aaSet, useTess, data.DX_5_1, res);
								break;
							default:
								break;
						}
					}
				}
#endif

				CLOAK_CALL_THIS Shader::Shader()
				{
					DEBUG_NAME(Shader);
				}
				CLOAK_CALL_THIS Shader::~Shader()
				{

				}
				bool CLOAK_CALL_THIS Shader::GetShaderData(Out API::Rendering::PIPELINE_STATE_DESC* desc, Out API::Rendering::RootDescription* root, In const API::Global::Graphic::Settings& gset, In API::Rendering::SHADER_MODEL version, In const API::Rendering::Hardware& hardware) const
				{
					//TODO
					return false;
				}

				LoadResult CLOAK_CALL_THIS Shader::iLoad(In API::Files::IReader* read, In API::Files::FileVersion version)
				{
					if (version > 0) 
					{ 
						//readShader(read, &m_data, version); 
					}
					return LoadResult::Finished;
				}
				void CLOAK_CALL_THIS Shader::iUnload()
				{
					//reset(&m_data);
				}
				Success(return == true) bool CLOAK_CALL_THIS Shader::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(Impl::Files::IShader)) { *ptr = (Impl::Files::IShader*)this; return true; }
					else CLOAK_QUERY(riid, ptr, RessourceHandler, Files::Shader_v1, Shader);
				}

				IMPLEMENT_FILE_HANDLER(Shader);
			}
		}
	}
}