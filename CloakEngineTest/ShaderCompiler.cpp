#include "stdafx.h"
#include "ShaderCompiler.h"

#include "CloakCompiler/Shader.h"

#include <vector>
#include <string>
#include <codecvt>
#include <sstream>

#define CLOAK_SHADERFILE_LIST "CSFL"

namespace ShaderCompiler {
	constexpr size_t MAX_TAB_COUNT = 9;
	constexpr size_t TAB_SIZE = 8;

	struct ResultCount {
		size_t Succeeded = 0;
		size_t Errors = 0;
		size_t Skipped = 0;
	};
	struct MessageHint {
		std::string Name;
		std::string Message;
	};
	struct MessageResult {
		CE::List<MessageHint> Errors;
		CE::List<MessageHint> Warnings;
	};

	template<typename T> constexpr inline bool IsFlagSet(In T value, In T flag) { return (value & flag) == flag; }
	CLOAK_FORCEINLINE bool CLOAK_CALL StartsWith(In const std::string& a, In const std::string& b) { return a.size() >= b.size() && a.compare(b) == 0; }
	CLOAK_FORCEINLINE bool CLOAK_CALL GetShaderType(In const std::string& name, Out CloakCompiler::Shader::Shader_v1003::SHADER_TYPE* res)
	{
		if (name.compare("None") == 0 || name.compare("Default") == 0) { *res = CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_NONE; return true; }
		else if (name.compare("MSAA") == 0 || name.compare("MultiSampled") == 0) { *res = CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_MSAA; return true; }
		else if (name.compare("Complex") == 0) { *res = CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_COMPLEX; return true; }
		else if (name.compare("Tessellation") == 0) { *res = CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_TESSELLATION; return true; }
		else if (name.compare("WaveSize") == 0 || name.compare("Compute") == 0) { *res = CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_WAVESIZE; return true; }
		return false;
	}
	CLOAK_FORCEINLINE bool CLOAK_CALL GetShaderModel(In const std::string& name, Out CE::Rendering::SHADER_MODEL* res)
	{
		if (StartsWith(name, "Vulkan")) 
		{
			if (name.compare("Vulkan 1.0") == 0 || name.compare("Vulkan") == 0) { *res = CE::Rendering::MODEL_VULKAN_1_0; return true; }
			else if (name.compare("Vulkan 1.1") == 0) { *res = CE::Rendering::MODEL_VULKAN_1_1; return true; }
		}
		else if (StartsWith(name, "SPIR-V"))
		{
			if (name.compare("SPIR-V 1.0") == 0 || name.compare("SPIR-V 1") == 0 || name.compare("SPIR-V") == 0) { *res = CE::Rendering::SHADER_MODEL(CE::Rendering::SHADER_SPIRV, 1, 0); return true; }
			else if (name.compare("SPIR-V 1.1") == 0) { *res = CE::Rendering::SHADER_MODEL(CE::Rendering::SHADER_SPIRV, 1, 1); return true; }
			else if (name.compare("SPIR-V 1.2") == 0) { *res = CE::Rendering::SHADER_MODEL(CE::Rendering::SHADER_SPIRV, 1, 2); return true; }
			else if (name.compare("SPIR-V 1.3") == 0) { *res = CE::Rendering::SHADER_MODEL(CE::Rendering::SHADER_SPIRV, 1, 3); return true; }
		}
		else if (StartsWith(name, "HLSL"))
		{
			if (name.compare("HLSL 5.0") == 0 || name.compare("HLSL 5") == 0) { *res = CE::Rendering::MODEL_HLSL_5_0; return true; }
			else if (name.compare("HLSL 5.1") == 0) { *res = CE::Rendering::MODEL_HLSL_5_1; return true; }
			else if (name.compare("HLSL 6.0") == 0 || name.compare("HLSL 6") == 0) { *res = CE::Rendering::MODEL_HLSL_6_0; return true; }
			else if (name.compare("HLSL 6.1") == 0) { *res = CE::Rendering::MODEL_HLSL_6_1; return true; }
			else if (name.compare("HLSL 6.2") == 0) { *res = CE::Rendering::MODEL_HLSL_6_2; return true; }
			else if (name.compare("HLSL 6.3") == 0) { *res = CE::Rendering::MODEL_HLSL_6_3; return true; }
			else if (name.compare("HLSL 6.4") == 0) { *res = CE::Rendering::MODEL_HLSL_6_4; return true; }
		}
		else if (StartsWith(name, "DirectX"))
		{
			if (name.compare("DirectX 11.0") == 0 || name.compare("DirectX 11") == 0) { *res = CE::Rendering::MODEL_DIRECTX_11_0; return true; }
			else if (name.compare("DirectX 11.1") == 0) { *res = CE::Rendering::MODEL_DIRECTX_11_1; return true; }
			else if (name.compare("DirectX 11.2") == 0) { *res = CE::Rendering::MODEL_DIRECTX_11_2; return true; }
			else if (name.compare("DirectX 11.3") == 0) { *res = CE::Rendering::MODEL_DIRECTX_11_3; return true; }
			else if (name.compare("DirectX 12.0") == 0 || name.compare("DirectX 12") == 0) { *res = CE::Rendering::MODEL_DIRECTX_12_0; return true; }
		}
		//TODO: Add support to additional shading languages
		return false;
	}
	CLOAK_FORCEINLINE bool CLOAK_CALL GetShaderPass(In const std::string& name, Out CloakCompiler::Shader::Shader_v1003::ShaderPass* res)
	{
		if (name.compare("VS") == 0) { *res = CloakCompiler::Shader::Shader_v1003::ShaderPass::VS; return true; }
		else if (name.compare("HS") == 0) { *res = CloakCompiler::Shader::Shader_v1003::ShaderPass::HS; return true; }
		else if (name.compare("DS") == 0) { *res = CloakCompiler::Shader::Shader_v1003::ShaderPass::DS; return true; }
		else if (name.compare("GS") == 0) { *res = CloakCompiler::Shader::Shader_v1003::ShaderPass::GS; return true; }
		else if (name.compare("PS") == 0) { *res = CloakCompiler::Shader::Shader_v1003::ShaderPass::PS; return true; }
		else if (name.compare("CS") == 0) { *res = CloakCompiler::Shader::Shader_v1003::ShaderPass::CS; return true; }
		return false;
	}
	CLOAK_FORCEINLINE bool CLOAK_CALL GetShaderDefine(In CE::Files::IValue* v, Out CloakCompiler::Shader::Shader_v1003::SHADER_DEFINE* cdef)
	{
		cdef->Name = v->getName();
		if (v->isBool()) { cdef->Value = "(" + std::string(v->toBool() == true ? "true" : "false") + ")"; }
		else if (v->isString()) { cdef->Value = v->toString(); }
		else if (v->isInt()) { cdef->Value = "(" + std::to_string(v->toInt()) + ")"; }
		else if (v->isFloat()) { cdef->Value = "((float)" + std::to_string(v->toFloat()) + ")"; }
		else { return false; }
		return true;
	}
	CLOAK_FORCEINLINE CloakCompiler::Shader::Shader_v1003::USAGE_MASK CLOAK_CALL GetUsageMask(In CE::Files::IValue* v, In CloakCompiler::Shader::Shader_v1003::USAGE_MASK defaultValue)
	{
		CloakCompiler::Shader::Shader_v1003::USAGE_MASK res = defaultValue;
		if (v->isString())
		{
			const std::string s = v->toString();
			if (s.compare("Allways") == 0) { res = CloakCompiler::Shader::Shader_v1003::USAGE_MASK_ALLWAYS; }
		}
		else if (v->isObjekt())
		{
			CE::Files::IValue* r = nullptr;
			for (size_t a = 0; v->enumerateChildren(a, &r); a++)
			{
				const std::string rn = r->getName();
				if (rn.compare("Complex") == 0)
				{
					res &= ~CloakCompiler::Shader::Shader_v1003::USAGE_MASK_COMPLEX_ALLWAYS;
					if (r->isBool()) { res |= r->toBool() == true ? CloakCompiler::Shader::Shader_v1003::USAGE_MASK_COMPLEX_ON : CloakCompiler::Shader::Shader_v1003::USAGE_MASK_COMPLEX_OFF; }
					else if (r->isString() && r->toString().compare("Allways") == 0) { res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_COMPLEX_ALLWAYS; }
					else if (r->isArray())
					{
						for (size_t b = 0; b < r->size(); b++)
						{
							res |= r->get(b)->toBool() == true ? CloakCompiler::Shader::Shader_v1003::USAGE_MASK_COMPLEX_ON : CloakCompiler::Shader::Shader_v1003::USAGE_MASK_COMPLEX_OFF;
						}
					}
				}
				else if (rn.compare("Tessellation") == 0)
				{
					res &= ~CloakCompiler::Shader::Shader_v1003::USAGE_MASK_TESSELLATION_ALLWAYS;
					if (r->isBool()) { res |= r->toBool() == true ? CloakCompiler::Shader::Shader_v1003::USAGE_MASK_TESSELLATION_ON : CloakCompiler::Shader::Shader_v1003::USAGE_MASK_TESSELLATION_OFF; }
					else if (r->isString() && r->toString().compare("Allways") == 0) { res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_TESSELLATION_ALLWAYS; }
					else if (r->isArray())
					{
						for (size_t b = 0; b < r->size(); b++)
						{
							res |= r->get(b)->toBool() == true ? CloakCompiler::Shader::Shader_v1003::USAGE_MASK_TESSELLATION_ON : CloakCompiler::Shader::Shader_v1003::USAGE_MASK_TESSELLATION_OFF;
						}
					}
				}
				else if (rn.compare("WaveSize") == 0)
				{
					res &= ~CloakCompiler::Shader::Shader_v1003::USAGE_MASK_WAVE_SIZE_ALLWAYS;
					if (r->isString() && r->toString().compare("Allways") == 0) { res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_WAVE_SIZE_ALLWAYS; }
					else
					{
						const size_t s = r->isArray() ? r->size() : 1;
						for (size_t b = 0; b < s; b++)
						{
							CE::Files::IValue* c = r->isArray() ? r->get(b) : r;
							int cv = 0;
							if (c->isString())
							{
								try {
									cv = std::stoi(c->toString());
								}
								catch (...) { continue; }
							}
							else if(c->isInt()) { cv = c->toInt(); }
							else { continue; }
							switch (cv)
							{
								case 4: res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_WAVE_SIZE_4; break;
								case 32: res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_WAVE_SIZE_32; break;
								case 64: res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_WAVE_SIZE_64; break;
								case 128: res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_WAVE_SIZE_128; break;
								default:
									break;
							}
						}
					}
				}
				else if (rn.compare("MSAA") == 0)
				{
					res &= ~CloakCompiler::Shader::Shader_v1003::USAGE_MASK_MSAA_ALLWAYS;
					if (r->isString() && r->toString().compare("Allways") == 0) { res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_MSAA_ALLWAYS; }
					else
					{
						const size_t s = r->isArray() ? r->size() : 1;
						for (size_t b = 0; b < s; b++)
						{
							CE::Files::IValue* c = r->isArray() ? r->get(b) : r;
							int cv = 0;
							if (c->isString())
							{
								try {
									cv = std::stoi(c->toString());
								}
								catch (...) { continue; }
							}
							else if (c->isInt()) { cv = c->toInt(); }
							else { continue; }
							switch (cv)
							{
								case 1: res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_MSAA_1; break;
								case 2: res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_MSAA_2; break;
								case 4: res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_MSAA_4; break;
								case 8: res |= CloakCompiler::Shader::Shader_v1003::USAGE_MASK_MSAA_8; break;
								default:
									break;
							}
						}
					}
				}
			}
		}
		return res;
	}

	CLOAK_FORCEINLINE void CLOAK_CALL PrintResponde(In const CloakCompiler::Shader::Shader_v1003::Response& r, In const CE::List<CloakCompiler::Shader::Shader_v1003::SHADER>& shaders, Inout MessageResult* messageResults)
	{
		std::stringstream res;
		res << "[" << shaders[r.Shader.ID].Name << "][";
		switch (r.Shader.Model.Language)
		{
			case CE::Rendering::SHADER_DXIL:
			case CE::Rendering::SHADER_HLSL:
				res << "HLSL";
				break;
			case CE::Rendering::SHADER_ESSL:
				res << "ESSL";
				break;
			case CE::Rendering::SHADER_GLSL:
				res << "GLSL";
				break;
			case CE::Rendering::SHADER_MSL_iOS:
				res << "MSL (iOS)";
				break;
			case CE::Rendering::SHADER_MSL_MacOS:
				res << "MSL";
				break;
			default: CLOAK_ASSUME(false); break;
		}
		res << " " << r.Shader.Model.Major << "." << r.Shader.Model.Minor;
		if (r.Shader.Model.Patch != 0) { res << "." << r.Shader.Model.Patch; }
		res << " ";
		switch (r.Shader.Pass)
		{
			case CloakCompiler::Shader::Shader_v1003::ShaderPass::VS: res << "VS"; break;
			case CloakCompiler::Shader::Shader_v1003::ShaderPass::PS: res << "PS"; break;
			case CloakCompiler::Shader::Shader_v1003::ShaderPass::GS: res << "GS"; break;
			case CloakCompiler::Shader::Shader_v1003::ShaderPass::DS: res << "DS"; break;
			case CloakCompiler::Shader::Shader_v1003::ShaderPass::HS: res << "HS"; break;
			case CloakCompiler::Shader::Shader_v1003::ShaderPass::CS: res << "CS"; break;
			default: CLOAK_ASSUME(false); break;
		}
		res << "]";

		CloakCompiler::Shader::Shader_v1003::SHADER_TYPE type = shaders[r.Shader.ID].Desc.Type;
		if (IsFlagSet(type, CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_COMPLEX))
		{
			res << "[";
			if (r.Shader.Configuration.GetComplex() == true) { res << "Complex"; }
			else { res << "Simple"; }
			res << "]";
		}
		if (IsFlagSet(type, CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_MSAA))
		{
			const uint32_t msaaC = r.Shader.Configuration.GetMSAA();
			if (msaaC > 1) { res << "[MSAA x" << msaaC << "]";}
		}
		if (IsFlagSet(type, CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_TESSELLATION))
		{
			if (r.Shader.Configuration.GetTessellation() == true) { res << "[Tessellation]"; }
		}
		if (IsFlagSet(type, CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_WAVESIZE))
		{
			res << "[WaveSize " << static_cast<uint32_t>(r.Shader.Configuration.GetWaveSize()) << "]";
		}
		res << ": ";
		std::stringstream msg;
		for (size_t a = res.str().length(); a < MAX_TAB_COUNT * TAB_SIZE; a++) { msg << " "; }

		switch (r.Status)
		{
			case CloakCompiler::Shader::Shader_v1003::Status::FINISHED: msg << "YES"; break;
			case CloakCompiler::Shader::Shader_v1003::Status::FAILED: 
				if (messageResults != nullptr) { messageResults->Errors.push_back({ res.str(), r.Message }); }
				msg << "ERROR\n\n" << r.Message;
				break;
			case CloakCompiler::Shader::Shader_v1003::Status::WARNING: 
				if (messageResults != nullptr) { messageResults->Warnings.push_back({ res.str(), r.Message }); }
				msg << "WARNING\n\n" << r.Message;
				break;
			case CloakCompiler::Shader::Shader_v1003::Status::INFO: return;
			case CloakCompiler::Shader::Shader_v1003::Status::SKIPPED: msg << "SKIPPED\n\n" << r.Message; break;
			default: CLOAK_ASSUME(false); break;
		}
		CE::Global::Log::WriteToLog("\t" + res.str() + msg.str());
	}

	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags, const CloakEngine::Files::CompressType* compress)
	{
		CloakEngine::List<CloakEngine::Files::FileInfo> paths;
		CloakEngine::Files::listAllFilesInDictionary(inputPath, CloakEngine::Helper::StringConvert::ConvertToU16(CLOAK_SHADERFILE_LIST), &paths);
		CE::RefPointer<CloakEngine::Files::IConfiguration> list = nullptr;
		CREATE_INTERFACE(CE_QUERY_ARGS(&list));
		if (paths.size() == 0) { CloakEngine::Global::Log::WriteToLog("No shader files were found!", CloakEngine::Global::Log::Type::Info); }
		else
		{
			for (const auto& fi : paths)
			{
				bool err = false;
				list->readFile(fi.FullPath, CE::Files::ConfigType::JSON);
				CE::Files::IValue* root = list->getRootValue();

				//Parse config:
				CE::Files::IValue* config = root->get("Config");
				CloakCompiler::Shader::Shader_v1003::SHADER_ENCODE shEncode;
				shEncode.Lib.Path = CE::Files::UFI(outputPath + "/" + CloakEngine::Helper::StringConvert::ConvertToU8(fi.RelativePath), 1);
				shEncode.ShaderName = CloakEngine::Helper::StringConvert::ConvertToU8(fi.Name);
				const size_t snf1 = shEncode.ShaderName.find_last_of('/');
				const size_t snf2 = shEncode.ShaderName.find_last_of('\\');
				if (snf1 == shEncode.ShaderName.npos && snf2 != shEncode.ShaderName.npos) { shEncode.ShaderName = shEncode.ShaderName.substr(snf2 + 1); }
				else if (snf1 != shEncode.ShaderName.npos && snf2 == shEncode.ShaderName.npos) { shEncode.ShaderName = shEncode.ShaderName.substr(snf1 + 1); }
				else if (snf1 != shEncode.ShaderName.npos && snf2 != shEncode.ShaderName.npos) { shEncode.ShaderName = shEncode.ShaderName.substr(max(snf1, snf2) + 1); }
				shEncode.ShaderName = shEncode.ShaderName.substr(0, shEncode.ShaderName.find_last_of('.'));
				std::replace(shEncode.ShaderName.begin(), shEncode.ShaderName.end(), '.', '_');
				shEncode.Mode = CloakCompiler::Shader::Shader_v1003::WRITE_MODE_NONE;
				if (config->get("CreateHeader")->toBool(true) == true) 
				{ 
					shEncode.Mode |= CloakCompiler::Shader::Shader_v1003::WRITE_MODE_LIB; 
					shEncode.Lib.WriteNamespaceAlias = config->get("NamespaceAlias")->toBool(true);
					shEncode.Lib.SharedLib = config->get("SharedLib")->toBool(false);
					std::string genName = config->get("LibGenerator")->toString("Ninja");
					if (genName.compare("Ninja") == 0) { shEncode.Lib.Generator = CloakCompiler::LibGenerator::Ninja; }
					else if (genName.compare("VS2015") == 0) { shEncode.Lib.Generator = CloakCompiler::LibGenerator::VisualStudio_2015; }
					else if (genName.compare("VS2017") == 0) { shEncode.Lib.Generator = CloakCompiler::LibGenerator::VisualStudio_2017; }
					else if (genName.compare("VS2019") == 0) { shEncode.Lib.Generator = CloakCompiler::LibGenerator::VisualStudio_2019; }
					else 
					{
						CloakEngine::Global::Log::WriteToLog("Error: Unknown lib-generator type '" + genName + "'. Possible values are 'Ninja', 'VS2015', 'VS2017' and 'VS2019'", CloakEngine::Global::Log::Type::Info);
						continue;
					}
				}
				if (config->get("CreateFile")->toBool(true) == true) { shEncode.Mode |= CloakCompiler::Shader::Shader_v1003::WRITE_MODE_FILE; }

				CloakEngine::Files::CompressType ct = CloakEngine::Files::CompressType::NONE;
				std::string ict = config->get("CompressType")->toString("LZC");
				if (ict.compare("NONE") == 0) { ct = CloakEngine::Files::CompressType::NONE; }
				else if (ict.compare("LZC") == 0) { ct = CloakEngine::Files::CompressType::LZC; }
				else if (ict.compare("LZW") == 0) { ct = CloakEngine::Files::CompressType::LZW; }
				else if (ict.compare("HUFFMAN") == 0) { ct = CloakEngine::Files::CompressType::HUFFMAN; }
				else if (ict.compare("DEFLATE") == 0) { ct = CloakEngine::Files::CompressType::LZH; }
				else if (ict.compare("LZ4") == 0) { ct = CloakEngine::Files::CompressType::LZ4; }
				else if (ict.compare("ZLIB") == 0) { ct = CloakEngine::Files::CompressType::ZLIB; }
				else
				{
					CloakEngine::Global::Log::WriteToLog("Error: Unknown compress type '" + ict + "'. Possible values are 'NONE', 'LZC', 'LZW', 'HUFFMAN', 'DEFLATE', 'LZ4' and 'ZLIB'", CloakEngine::Global::Log::Type::Info);
					continue;
				}
				CloakEngine::Files::UGID cgid;
				int gid = config->get("TargetCGID")->toInt(0);
				if (gid == 0) { cgid = GameID::Test; }
				else if (gid == 1) { cgid = GameID::Editor; }
				if (compress != nullptr) { ct = *compress; }

				bool glbDebug = false;
				if (config->has("Debug") && config->get("Debug")->toBool(false) == true) { glbDebug = true; }

				CloakCompiler::EncodeDesc encode;
				encode.flags = CloakCompiler::EncodeFlags::NONE;
				if ((flags & CompilerFlags::ForceRecompile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_READ; }
				if ((flags & CompilerFlags::NoTempFile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_WRITE; }
				encode.targetGameID = cgid;
				encode.tempPath = CloakEngine::Helper::StringConvert::ConvertToU16(tempPath) + u"/" + fi.RelativePath;

				//Parse shader list:
				CE::List<CloakCompiler::Shader::Shader_v1003::SHADER> shaders;
				CE::List<CloakCompiler::Shader::Shader_v1003::SHADER_NAME> names;
				CE::List<CloakCompiler::Shader::Shader_v1003::SHADER_DEFINE> defines;
				CE::List<std::pair<size_t, size_t>> shaderNames;
				CE::List<std::pair<size_t, size_t>> shaderDefines;
				CE::Files::IValue* shD = root->get("Shader");
				CE::Files::IValue* shader = nullptr;
				for (size_t a = 0; shD->enumerateChildren(a, &shader); a++)
				{
					CloakCompiler::Shader::Shader_v1003::SHADER cur;
					cur.Name = shader->getName();
					cur.Desc.Type = CloakCompiler::Shader::Shader_v1003::SHADER_TYPE_NONE;
					if (shader->has("Type"))
					{
						CE::Files::IValue* type = shader->get("Type");
						if (type->isString()) 
						{ 
							const std::string v = type->toString();
							CloakCompiler::Shader::Shader_v1003::SHADER_TYPE rt;
							if (GetShaderType(v, &rt)) { cur.Desc.Type = rt; }
							else
							{
								CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "': Type '" + v + "' is unknown");
								goto shader_got_error;
							}
						}
						else if (type->isArray())
						{
							for (size_t b = 0; b < type->size(); b++) 
							{ 
								const std::string v = type->get(b)->toString();
								CloakCompiler::Shader::Shader_v1003::SHADER_TYPE rt;
								if (GetShaderType(v, &rt)) { cur.Desc.Type |= rt; }
								else
								{
									CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "': Type '" + v + "' is unknown");
									goto shader_got_error;
								}
							}
						}
						else
						{
							CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "': Type definition is unclear (must be string or array of strings)");
							goto shader_got_error;
						}
					}

					cur.Desc.Options = CloakCompiler::Shader::Shader_v1003::OPTION_NONE;
					bool debug = glbDebug;
					if (shader->has("Debug")) { debug = shader->get("Debug")->toBool(false); }
					if (debug == true) { cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_DEBUG; }
					if (shader->has("SkipOptimization") && shader->get("SkipOptimization")->toBool(false) == true) { cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_DEBUG_SKIP_OPTIMIZATION; }
					if (shader->has("16BitTypes") && shader->get("16BitTypes")->toBool(false) == true) { cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_16_BIT_TYPES; }
					if (shader->has("OptimizationLevel"))
					{
						CE::Files::IValue* oL = shader->get("OptimizationLevel");
						if (oL->isInt())
						{
							const auto ov = oL->toInt();
							switch (ov)
							{
								case 0: cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_OPTIMIZATION_LEVEL_0; break;
								case 1: cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_OPTIMIZATION_LEVEL_1; break;
								case 2: cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_OPTIMIZATION_LEVEL_2; break;
								case 3: cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_OPTIMIZATION_LEVEL_3; break;
								default:
									if (ov >= 0) { cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_OPTIMIZATION_LEVEL_HIGHEST; }
									else { cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_OPTIMIZATION_LEVEL_LOWEST; }
									break;
							}
						}
						else
						{
							CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "': Optimization Level definition is unclear (must be integer)");
							goto shader_got_error;
						}
					}
					else { cur.Desc.Options |= CloakCompiler::Shader::Shader_v1003::OPTION_OPTIMIZATION_LEVEL_HIGHEST; }

					const size_t firstName = names.size();
					if (shader->has("Files") == true)
					{
						CE::Files::IValue* sList = shader->get("Files");
						const size_t sLs = sList->isArray() ? sList->size() : 1;
						for (size_t b = 0; b < sLs; b++)
						{
							const size_t firstDefine = defines.size();

							CE::Files::IValue* named = sList->isArray() ? sList->get(b) : sList;
							CloakCompiler::Shader::Shader_v1003::SHADER_NAME ns;
							ns.DefineCount = 0;
							ns.Defines = nullptr;

							CE::HashList<CE::Rendering::SHADER_MODEL> models;
							CE::Files::IValue* mods = named->get("Version");
							if (mods->isString())
							{
								const std::string v = mods->toString();
								if (v.compare("All") == 0)
								{
									for (size_t c = 0; c < ARRAYSIZE(CE::Rendering::SHADER_VERSIONS); c++) { models.insert(CE::Rendering::SHADER_VERSIONS[c]); }
								}
								else
								{
									CE::Rendering::SHADER_MODEL rm;
									if (GetShaderModel(v, &rm)) { models.insert(rm); }
									else
									{
										CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "': Version '" + v + "' is unknown");
										goto shader_got_error;
									}
								}
							}
							else if (mods->isArray())
							{
								for (size_t c = 0; c < mods->size(); c++)
								{
									const std::string v = mods->get(c)->toString();
									CE::Rendering::SHADER_MODEL rm;
									if (GetShaderModel(v, &rm)) { models.insert(rm); }
									else
									{
										CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "': Version '" + v + "' is unknown");
										goto shader_got_error;
									}
								}
							}

							CE::List<CloakCompiler::Shader::Shader_v1003::ShaderPass> passes;
							CE::Files::IValue* pass = named->get("Pass");
							if (pass->isString())
							{
								const std::string v = pass->toString();
								if (v.compare("All") == 0)
								{
									passes.push_back(CloakCompiler::Shader::Shader_v1003::ShaderPass::CS);
									passes.push_back(CloakCompiler::Shader::Shader_v1003::ShaderPass::VS);
									passes.push_back(CloakCompiler::Shader::Shader_v1003::ShaderPass::PS);
									passes.push_back(CloakCompiler::Shader::Shader_v1003::ShaderPass::GS);
									passes.push_back(CloakCompiler::Shader::Shader_v1003::ShaderPass::HS);
									passes.push_back(CloakCompiler::Shader::Shader_v1003::ShaderPass::DS);
								}
								else
								{
									CloakCompiler::Shader::Shader_v1003::ShaderPass rp;
									if (GetShaderPass(v, &rp)) { passes.push_back(rp); }
									else
									{
										CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "': Pass '" + v + "' is unknown");
										goto shader_got_error;
									}
								}
							}
							else if (pass->isArray())
							{
								for (size_t c = 0; c < pass->size(); c++)
								{
									const std::string v = pass->get(c)->toString();
									CloakCompiler::Shader::Shader_v1003::ShaderPass rp;
									if (GetShaderPass(v, &rp)) { passes.push_back(rp); }
									else
									{
										CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "': Pass '" + v + "' is unknown");
										goto shader_got_error;
									}
								}
							}
							else
							{
								CE::Global::Log::WriteToLog("Error in Shader '" + shader->getName() + "' file " + std::to_string(b) + ": Pass definition is unclear (must be string or array of strings)");
								goto shader_got_error;
							}

							if (named->has("Usage")) { ns.Use = GetUsageMask(named->get("Usage"), CloakCompiler::Shader::Shader_v1003::USAGE_MASK_ALLWAYS); }
							else { ns.Use = CloakCompiler::Shader::Shader_v1003::USAGE_MASK_ALLWAYS; }
							if (named->has("UseNot")) { ns.Use &= ~GetUsageMask(named->get("UseNot"), CloakCompiler::Shader::Shader_v1003::USAGE_MASK_NEVER); }
							if (ns.Use == CloakCompiler::Shader::Shader_v1003::USAGE_MASK_NEVER)
							{
								CE::Global::Log::WriteToLog("Warning in Shader '" + shader->getName() + "' file " + std::to_string(b) + ": File is not used");
								continue;
							}

							bool hmf = false;
							std::string mfn = "";
							if (named->has("Function"))
							{
								mfn = named->get("Function")->toString();
								hmf = mfn.length() > 0;
							}

							ns.File = fi.RootPath + CE::Helper::StringConvert::ConvertToU16(named->get("File")->toString());

							if (named->has("Defines"))
							{
								CloakCompiler::Shader::Shader_v1003::SHADER_DEFINE cdef;
								CE::Files::IValue* defL = named->get("Defines");
								if (defL->isObjekt())
								{
									CE::Files::IValue* def = nullptr;
									for (size_t c = 0; defL->enumerateChildren(c, &def); c++)
									{
										if (GetShaderDefine(def, &cdef) == true) { defines.push_back(cdef); }
										else
										{
											CE::Global::Log::WriteToLog("Warning in Shader '" + shader->getName() + "' file " + std::to_string(b) + ": Define '" + def->getName() + "' has no valid type");
										}
									}
								}
								else if (defL->isArray())
								{
									for (size_t c = 0; c < defL->size(); c++)
									{
										CE::Files::IValue* def = defL->get(c);
										if (def->isString()) { cdef.Name = def->toString(); }
										else
										{
											CE::Global::Log::WriteToLog("Warning in Shader '" + shader->getName() + "' file " + std::to_string(b) + ": Define name must be a string");
										}
									}
								}
								else if (defL->isString())
								{
									cdef.Name = defL->toString();
									cdef.Value = "";
									defines.push_back(cdef);
								}
								else
								{
									CE::Global::Log::WriteToLog("Warning in Shader '" + shader->getName() + "' file " + std::to_string(b) + ": Defines must be an array, object or string!");
									continue;
								}
							}

							for (size_t c = 0; c < passes.size(); c++)
							{
								if (hmf == false)
								{
									mfn = "main";
									switch (passes[c])
									{
										case CloakCompiler::Shader::Shader_v1003::ShaderPass::CS: mfn += "CS"; break;
										case CloakCompiler::Shader::Shader_v1003::ShaderPass::VS: mfn += "VS"; break;
										case CloakCompiler::Shader::Shader_v1003::ShaderPass::PS: mfn += "PS"; break;
										case CloakCompiler::Shader::Shader_v1003::ShaderPass::GS: mfn += "GS"; break;
										case CloakCompiler::Shader::Shader_v1003::ShaderPass::DS: mfn += "DS"; break;
										case CloakCompiler::Shader::Shader_v1003::ShaderPass::HS: mfn += "HS"; break;
										default: CLOAK_ASSUME(false); break;
									}
								}
								ns.Pass = passes[c];
								ns.MainFuncName = mfn;
								for (const auto& c : models)
								{
									ns.Model = c;
									names.push_back(ns);
									shaderDefines.push_back(std::make_pair(firstDefine, defines.size()));
								}
							}
						}
					}
					//We delay putting in shader count and pointer, since the resizing of the names-list might invalidate the pointer
					cur.Desc.ShaderCount = 0;
					cur.Desc.Shaders = nullptr;

					shaderNames.push_back(std::make_pair(firstName, names.size()));
					shaders.push_back(cur);
					continue;
				shader_got_error:
					err = true;
				}
				CLOAK_ASSUME(shaders.size() == shaderNames.size());
				for (size_t a = 0; a < shaders.size(); a++)
				{
					const auto p = shaderNames[a];
					CLOAK_ASSUME(p.first <= p.second);
					CLOAK_ASSUME(p.second <= names.size());
					for (size_t b = p.first; b < p.second; b++)
					{
						const auto d = shaderDefines[b];
						names[b].DefineCount = d.second - d.first;
						if (d.second > d.first) { names[b].Defines = &defines[d.first]; }
						else { names[b].Defines = nullptr; }
					}
					shaders[a].Desc.ShaderCount = p.second - p.first;
					if (p.second > p.first) { shaders[a].Desc.Shaders = &names[p.first]; }
					else { shaders[a].Desc.Shaders = nullptr; }
				}
				
				if (err == false)
				{
					//Compiling:
					CloakEngine::Global::Log::WriteToLog(u"Compiling shader '" + fi.RelativePath + u"':", CloakEngine::Global::Log::Type::Info);

					CloakEngine::Files::IWriter* write = nullptr;
					if ((shEncode.Mode & CloakCompiler::Shader::Shader_v1003::WRITE_MODE_FILE) != CloakCompiler::Shader::Shader_v1003::WRITE_MODE_NONE)
					{
						CREATE_INTERFACE(CE_QUERY_ARGS(&write));
						write->SetTarget(CloakEngine::Helper::StringConvert::ConvertToU16(outputPath) + u"/" + fi.RelativePath, CloakCompiler::Shader::Shader_v1003::FileType, ct, encode.targetGameID);
					}
					MessageResult msgRes;
					ResultCount result;
					result.Errors = 0;
					result.Skipped = 0;
					result.Succeeded = 0;
					CloakCompiler::Shader::Shader_v1003::Encode(write, encode, shEncode, shaders, [&](In const CloakCompiler::Shader::Shader_v1003::Response& r) {
						switch (r.Status)
						{
							case CloakCompiler::Shader::Shader_v1003::Status::FAILED: result.Errors++; break;
							case CloakCompiler::Shader::Shader_v1003::Status::FINISHED: result.Succeeded++; break;
							case CloakCompiler::Shader::Shader_v1003::Status::SKIPPED: result.Skipped++; break;
							default: break;
						}
						PrintResponde(r, shaders, &msgRes);
					});
					if (write != nullptr)
					{
						write->Save();
						write->Release();
					}
					CloakEngine::Global::Log::WriteToLog("Shader compiled! Succeeded: " + std::to_string(result.Succeeded) + " | Failed: " + std::to_string(result.Errors) + " | Skipped: " + std::to_string(result.Skipped), CloakEngine::Global::Log::Type::Info);
					if (msgRes.Warnings.size() > 0)
					{
						std::stringstream rs;
						rs << "Warnings:";
						for (size_t a = 0; a < msgRes.Warnings.size(); a++)
						{
							rs << "\n" << msgRes.Warnings[a].Name << msgRes.Warnings[a].Message;
						}
						CloakEngine::Global::Log::WriteToLog(rs.str(), CloakEngine::Global::Log::Type::Info);
					}
					if (msgRes.Errors.size() > 0)
					{
						std::stringstream rs;
						rs << "Errors:";
						for (size_t a = 0; a < msgRes.Errors.size(); a++)
						{
							rs << "\n" << msgRes.Errors[a].Name << msgRes.Errors[a].Message;
						}
						CloakEngine::Global::Log::WriteToLog(rs.str(), CloakEngine::Global::Log::Type::Info);
					}
					list->removeUnused();
					list->saveFile(fi.FullPath, CloakEngine::Files::ConfigType::JSON);
				}
			}
		}
	}
}
