#include "stdafx.h"
#include "MeshCompiler.h"

#include "ImageCompiler.h"

#include "CloakCompiler/Converter.h"
#include "CloakCompiler/Mesh.h"
#include "CloakCompiler/Image.h"

#define CLOAK_MESHFILE_LIST "CMFL"

#define COMPILE_MATERIAL
#define COMPILE_MESH

namespace MeshCompiler {
	inline void CLOAK_CALL addLocation(const std::u16string& location, CloakCompiler::Image::BuildInfo* i)
	{
		if (i->Meta.Type == CloakCompiler::Image::BuildType::NORMAL)
		{
			i->Normal.Path = location + i->Normal.Path;
		}
		else
		{
			i->Bordered.Center = location + i->Bordered.Center;
			i->Bordered.CornerBottomLeft = location + i->Bordered.CornerBottomLeft;
			i->Bordered.CornerBottomRight = location + i->Bordered.CornerBottomRight;
			i->Bordered.CornerTopLeft = location + i->Bordered.CornerTopLeft;
			i->Bordered.CornerTopRight = location + i->Bordered.CornerTopRight;
			i->Bordered.SideBottom = location + i->Bordered.SideBottom;
			i->Bordered.SideLeft = location + i->Bordered.SideLeft;
			i->Bordered.SideRight = location + i->Bordered.SideRight;
			i->Bordered.SideTop = location + i->Bordered.SideTop;
		}
	}
	inline void CLOAK_CALL addLocation(const std::u16string& location, CloakCompiler::Image::OptionalInfo* i)
	{
		if (i->Use) { addLocation(location, static_cast<CloakCompiler::Image::BuildInfo*>(i)); }
	}
	inline void CLOAK_CALL readNormalMapInfo(CloakCompiler::Image::ImageInfo* imi, CloakEngine::Files::IValue* constants)
	{
		CloakEngine::Files::IValue* normMapT = constants->get("NormalMap");

		const bool normMapX = normMapT->get("XPointsLeft")->toBool(false);
		const bool normMapY = normMapT->get("YPointsUp")->toBool(true);
		const bool normMapZ = normMapT->get("OnlyPositiveZ")->toBool(true);
		imi->Material.NormalMapFlags = CloakCompiler::Image::NORMAL_FLAG_NONE;
		imi->Material.NormalMapFlags |= normMapX ? CloakCompiler::Image::NORMAL_FLAG_X_LEFT : CloakCompiler::Image::NORMAL_FLAG_X_RIGHT;
		imi->Material.NormalMapFlags |= normMapY ? CloakCompiler::Image::NORMAL_FLAG_Y_TOP : CloakCompiler::Image::NORMAL_FLAG_Y_BOTTOM;
		imi->Material.NormalMapFlags |= normMapZ ? CloakCompiler::Image::NORMAL_FLAG_Z_FULL_RANGE : CloakCompiler::Image::NORMAL_FLAG_Z_HALF_RANGE;
	}
	inline float CLOAK_CALL readPorosity(In CloakEngine::Files::IValue* constants)
	{
		return constants->get("Porosity")->toFloat(0.35f);
	}
	void CLOAK_CALL compileMaterials(CloakEngine::Files::IValue* root, const CloakCompiler::EncodeDesc& encode, const std::vector<CloakCompiler::Image::ImageInfo>& mats, const CloakEngine::Files::FileInfo& fi, const std::u16string& odict, CloakCompiler::Image::SupportedQuality supQual, unsigned int blockSize, CloakEngine::Files::CompressType ct, std::u16string location)
	{
		bool readErr = false;

		CloakEngine::Files::IValue* config = root->get("Config");
		CloakEngine::Files::IValue* imgs = root->get("Materials");

		CloakCompiler::Image::Desc desc;

		CloakEngine::Files::IValue* ycca = config->get("YCCA");
		desc.File.YCCA.Y = static_cast<unsigned int>(ycca->get("Y")->toInt(8));
		desc.File.YCCA.C = static_cast<unsigned int>(ycca->get("C")->toInt(8));
		desc.File.YCCA.A = static_cast<unsigned int>(ycca->get("A")->toInt(8));
		CloakEngine::Files::IValue* rgba = config->get("RGBA");
		desc.File.RGBA.R = static_cast<unsigned int>(rgba->get("R")->toInt(8));
		desc.File.RGBA.G = static_cast<unsigned int>(rgba->get("G")->toInt(8));
		desc.File.RGBA.B = static_cast<unsigned int>(rgba->get("B")->toInt(8));
		desc.File.RGBA.A = static_cast<unsigned int>(rgba->get("A")->toInt(8));
		CloakEngine::Files::IValue* mat = config->get("Material");
		desc.File.Material.A = static_cast<unsigned int>(mat->get("A")->toInt(16));
		CloakEngine::Files::IValue* rgbe = config->get("RGBE");
		desc.File.RGBE.R = static_cast<unsigned int>(rgbe->get("R")->toInt(8));
		desc.File.RGBE.G = static_cast<unsigned int>(rgbe->get("G")->toInt(8));
		desc.File.RGBE.B = static_cast<unsigned int>(rgbe->get("B")->toInt(8));
		desc.File.RGBE.Exponent = static_cast<unsigned int>(rgbe->get("E")->toInt(8));

		const bool encM = config->get("EncodeMaterial")->toBool(true);

		const bool ovr = config->get("Overwrite")->toBool();
		if (encM == true)
		{
			desc.Imgs.reserve(mats.size());
			if (ovr)
			{
				const size_t tarSize = mats.size();
				if (imgs->size() >= tarSize)
				{
					for (size_t a = 0; a < imgs->size(); a++)
					{
						CloakCompiler::Image::ImageInfo imi;
						imi.Heap = CloakCompiler::Image::Heap::WORLD;
						imi.SupportedQuality = supQual;
						imi.Type = CloakCompiler::Image::ImageInfoType::MATERIAL;
						if (readMaterialFromConfig(&imi, imgs->get(a), blockSize, location) == false) { readErr = true; }
						desc.Imgs.push_back(imi);
					}
				}
				else
				{
					CloakEngine::Global::Log::WriteToLog("Error: Not enough materials were given (needed at least " + std::to_string(tarSize) + ")", CloakEngine::Global::Log::Type::Info);
					readErr = true;
				}
			}
			else
			{
				for (size_t a = 0; a < mats.size(); a++) 
				{
					CloakCompiler::Image::ImageInfo imi = mats[a];
					CloakEngine::Files::IValue* constants = imgs->get(a)->get("Constants");
					readNormalMapInfo(&imi, constants);
					switch (imi.Material.Type)
					{
						case CloakCompiler::Image::MaterialType::Opaque:
							addLocation(location, &imi.Material.Opaque.Textures.Albedo);
							addLocation(location, &imi.Material.Opaque.Textures.NormalMap);
							addLocation(location, &imi.Material.Opaque.Textures.Metallic);
							addLocation(location, &imi.Material.Opaque.Textures.AmbientOcclusion);
							addLocation(location, &imi.Material.Opaque.Textures.RoughnessMap);
							addLocation(location, &imi.Material.Opaque.Textures.SpecularMap);
							addLocation(location, &imi.Material.Opaque.Textures.IndexOfRefractionMap);
							addLocation(location, &imi.Material.Opaque.Textures.ExtinctionCoefficientMap);
							addLocation(location, &imi.Material.Opaque.Textures.DisplacementMap);
							addLocation(location, &imi.Material.Opaque.Textures.PorosityMap);

							imi.Material.Opaque.Constants.Porosity = readPorosity(constants);
							break;
						case CloakCompiler::Image::MaterialType::AlphaMapped:
							addLocation(location, &imi.Material.AlphaMapped.Textures.Albedo);
							addLocation(location, &imi.Material.AlphaMapped.Textures.NormalMap);
							addLocation(location, &imi.Material.AlphaMapped.Textures.Metallic);
							addLocation(location, &imi.Material.AlphaMapped.Textures.AmbientOcclusion);
							addLocation(location, &imi.Material.AlphaMapped.Textures.RoughnessMap);
							addLocation(location, &imi.Material.AlphaMapped.Textures.SpecularMap);
							addLocation(location, &imi.Material.AlphaMapped.Textures.IndexOfRefractionMap);
							addLocation(location, &imi.Material.AlphaMapped.Textures.ExtinctionCoefficientMap);
							addLocation(location, &imi.Material.AlphaMapped.Textures.DisplacementMap);
							addLocation(location, &imi.Material.AlphaMapped.Textures.AlphaMap);
							addLocation(location, &imi.Material.AlphaMapped.Textures.PorosityMap);

							imi.Material.AlphaMapped.Constants.Porosity = readPorosity(constants);
							break;
						case CloakCompiler::Image::MaterialType::Transparent:
							addLocation(location, &imi.Material.Transparent.Textures.Albedo);
							addLocation(location, &imi.Material.Transparent.Textures.NormalMap);
							addLocation(location, &imi.Material.Transparent.Textures.Metallic);
							addLocation(location, &imi.Material.Transparent.Textures.AmbientOcclusion);
							addLocation(location, &imi.Material.Transparent.Textures.RoughnessMap);
							addLocation(location, &imi.Material.Transparent.Textures.SpecularMap);
							addLocation(location, &imi.Material.Transparent.Textures.IndexOfRefractionMap);
							addLocation(location, &imi.Material.Transparent.Textures.ExtinctionCoefficientMap);
							addLocation(location, &imi.Material.Transparent.Textures.DisplacementMap);
							addLocation(location, &imi.Material.Transparent.Textures.PorosityMap);

							imi.Material.Transparent.Constants.Porosity = readPorosity(constants);
							break;
						default:
							break;
					}
					desc.Imgs.push_back(imi); 
				}
			}
			if (readErr == false)
			{
				if (desc.Imgs.size() > 0)
				{
					CloakEngine::Files::IWriter* write = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&write));
					write->SetTarget(odict + u"//" + fi.RelativePath, CloakCompiler::Image::FileType, ct, encode.targetGameID);
					ImageCompiler::compilePrepared(write, desc, encode, fi);
					write->Save();
					write->Release();
					CloakEngine::Global::Log::WriteToLog(std::to_string(desc.Imgs.size()) + " Materials encoded!", CloakEngine::Global::Log::Type::Info);
				}
				else
				{
					CloakEngine::Global::Log::WriteToLog("No Materials encoded!", CloakEngine::Global::Log::Type::Info);
				}
			}
		}
	}
	bool CLOAK_CALL InitMaterialInfo(Inout CloakCompiler::Image::BuildInfo* info, In CloakEngine::Files::IValue* img, In const std::string& name, unsigned int blockSize, std::u16string location)
	{
		info->Meta.Type = CloakCompiler::Image::BuildType::NORMAL;
		info->Normal.Path = location + CloakEngine::Helper::StringConvert::ConvertToU16(img->get(name)->get("FilePath")->toString(""));
		info->Meta.BlockSize = static_cast<unsigned int>(blockSize);

		const std::string space = img->get(name)->get("ColorSpace")->toString("Material");
		if (space.compare("RGBA") == 0) { info->Meta.Space = CloakCompiler::Image::ColorSpace::RGBA; }
		else if (space.compare("YCCA") == 0) { info->Meta.Space = CloakCompiler::Image::ColorSpace::YCCA; }
		else if (space.compare("Material") == 0) { info->Meta.Space = CloakCompiler::Image::ColorSpace::Material; }
		else
		{
			CloakEngine::Global::Log::WriteToLog("Error: Unknown color space '" + space + "'. Possible values are 'RGBA', 'YCCA' or 'Material'", CloakEngine::Global::Log::Type::Info);
			return true;
		}
		return false;
	}
	bool CLOAK_CALL InitMaterialInfo(Inout CloakCompiler::Image::OptionalInfo* info, In CloakEngine::Files::IValue* img, In const std::string& name, unsigned int blockSize, std::u16string location)
	{
		info->Use = img->has(name);
		if (info->Use)
		{
			return InitMaterialInfo(static_cast<CloakCompiler::Image::BuildInfo*>(info), img, name, blockSize, location);
		}
		else
		{
			info->Meta.Type = CloakCompiler::Image::BuildType::NORMAL;
			info->Normal.Path = u"";
			info->Meta.BlockSize = 0;
			info->Meta.Space = CloakCompiler::Image::ColorSpace::Material;
		}
		return false;
	}
	bool CLOAK_CALL checkValue(In float a, In_opt float minV = 0, In_opt float maxV = 1)
	{
		return a >= minV && a <= maxV;
	}
	bool CLOAK_CALL checkColor(In const CloakEngine::Helper::Color::RGBX& c)
	{
		return checkValue(c.R) && checkValue(c.G) && checkValue(c.B);
	}
	bool CLOAK_CALL checkColor(In const CloakEngine::Helper::Color::RGBA& c)
	{
		return checkColor(static_cast<CloakEngine::Helper::Color::RGBX>(c)) && checkValue(c.A);
	}

	bool CLOAK_CALL readMaterialFromConfig(CloakCompiler::Image::ImageInfo* imi, CloakEngine::Files::IValue* mat, unsigned int blockSize, std::u16string location)
	{
		imi->Material.MaxMipMapCount = mat->get("MipMapCount")->toInt(0);
		const std::string matT = mat->get("Type")->toString("Opaque");
		CloakEngine::Files::IValue* textures = mat->get("Textures");
		CloakEngine::Files::IValue* constant = mat->get("Constants");

		CloakEngine::Files::IValue* constAlb = constant->get("Albedo");
		CloakEngine::Files::IValue* constFrs = constant->get("Fresnel");
		CloakEngine::Files::IValue* constSpec = constant->get("Specular");
		readNormalMapInfo(imi, constant);

		if (matT.compare("Opaque") == 0)
		{
			imi->Material.Type = CloakCompiler::Image::MaterialType::Opaque;
			auto& mati = imi->Material.Opaque;

			InitMaterialInfo(&mati.Textures.Albedo, textures, "Albedo", blockSize, location);
			InitMaterialInfo(&mati.Textures.NormalMap, textures, "NormalMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.Metallic, textures, "Metallic", blockSize, location);
			InitMaterialInfo(&mati.Textures.AmbientOcclusion, textures, "AmbientOcclusion", blockSize, location);
			InitMaterialInfo(&mati.Textures.RoughnessMap, textures, "RoughnessMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.SpecularMap, textures, "SpecularMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.IndexOfRefractionMap, textures, "IndexOfRefractionMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.ExtinctionCoefficientMap, textures, "ExtinctionCoefficientMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.DisplacementMap, textures, "DisplacementMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.PorosityMap, textures, "PorosityMap", blockSize, location);

			mati.Constants.Albedo.R = constAlb->get("R")->toFloat(0);
			mati.Constants.Albedo.G = constAlb->get("G")->toFloat(0);
			mati.Constants.Albedo.B = constAlb->get("B")->toFloat(0);
			mati.Constants.Metallic = constant->get("Metallic")->toFloat(0);
			mati.Constants.Roughness = constant->get("Roughness")->toFloat(0);
			mati.Constants.Porosity = readPorosity(constant);
			if (constFrs->isArray())
			{
				for (size_t a = 0; a < 3; a++)
				{
					mati.Constants.Fresnel[a].RefractionIndex = constFrs->get(a)->get("RefractionIndex")->toFloat(0);
					mati.Constants.Fresnel[a].ExtinctionCoefficient = constFrs->get(a)->get("ExtinctionCoefficient")->toFloat(0);
				}
			}
			else
			{
				for (size_t a = 0; a < 3; a++)
				{
					mati.Constants.Fresnel[a].RefractionIndex = constFrs->get("RefractionIndex")->toFloat(0);
					mati.Constants.Fresnel[a].ExtinctionCoefficient = constFrs->get("ExtinctionCoefficient")->toFloat(0);
				}
			}

			if (checkColor(mati.Constants.Albedo) == false)
			{
				CloakEngine::Global::Log::WriteToLog("Error: Albedo values must be between zero and one", CloakEngine::Global::Log::Type::Info);
				return false;
			}
			if (checkValue(mati.Constants.Metallic) == false)
			{
				CloakEngine::Global::Log::WriteToLog("Error: Metallic value must be between zero and one", CloakEngine::Global::Log::Type::Info);
				return false;
			}
		}
		else if (matT.compare("AlphaMapped") == 0)
		{
			imi->Material.Type = CloakCompiler::Image::MaterialType::AlphaMapped;
			auto& mati = imi->Material.AlphaMapped;

			InitMaterialInfo(&mati.Textures.Albedo, textures, "Albedo", blockSize, location);
			InitMaterialInfo(&mati.Textures.NormalMap, textures, "NormalMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.Metallic, textures, "Metallic", blockSize, location);
			InitMaterialInfo(&mati.Textures.AmbientOcclusion, textures, "AmbientOcclusion", blockSize, location);
			InitMaterialInfo(&mati.Textures.RoughnessMap, textures, "RoughnessMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.SpecularMap, textures, "SpecularMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.IndexOfRefractionMap, textures, "IndexOfRefractionMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.ExtinctionCoefficientMap, textures, "ExtinctionCoefficientMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.DisplacementMap, textures, "DisplacementMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.AlphaMap, textures, "AlphaMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.PorosityMap, textures, "PorosityMap", blockSize, location);

			mati.Constants.Albedo.R = constAlb->get("R")->toFloat(0);
			mati.Constants.Albedo.G = constAlb->get("G")->toFloat(0);
			mati.Constants.Albedo.B = constAlb->get("B")->toFloat(0);
			mati.Constants.Metallic = constant->get("Metallic")->toFloat(0);
			mati.Constants.Roughness = constant->get("Roughness")->toFloat(0);
			mati.Constants.Porosity = readPorosity(constant);
			if (constFrs->isArray())
			{
				for (size_t a = 0; a < 3; a++)
				{
					mati.Constants.Fresnel[a].RefractionIndex = constFrs->get(a)->get("RefractionIndex")->toFloat(0);
					mati.Constants.Fresnel[a].ExtinctionCoefficient = constFrs->get(a)->get("ExtinctionCoefficient")->toFloat(0);
				}
			}
			else
			{
				for (size_t a = 0; a < 3; a++)
				{
					mati.Constants.Fresnel[a].RefractionIndex = constFrs->get("RefractionIndex")->toFloat(0);
					mati.Constants.Fresnel[a].ExtinctionCoefficient = constFrs->get("ExtinctionCoefficient")->toFloat(0);
				}
			}

			if (checkColor(mati.Constants.Albedo) == false)
			{
				CloakEngine::Global::Log::WriteToLog("Error: Albedo values must be between zero and one", CloakEngine::Global::Log::Type::Info);
				return false;
			}
			if (checkValue(mati.Constants.Metallic) == false)
			{
				CloakEngine::Global::Log::WriteToLog("Error: Metallic value must be between zero and one", CloakEngine::Global::Log::Type::Info);
				return false;
			}
		}
		else if (matT.compare("Transparent") == 0)
		{
			imi->Material.Type = CloakCompiler::Image::MaterialType::Transparent;
			auto& mati = imi->Material.Transparent;

			InitMaterialInfo(&mati.Textures.Albedo, textures, "Albedo", blockSize, location);
			InitMaterialInfo(&mati.Textures.NormalMap, textures, "NormalMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.Metallic, textures, "Metallic", blockSize, location);
			InitMaterialInfo(&mati.Textures.AmbientOcclusion, textures, "AmbientOcclusion", blockSize, location);
			InitMaterialInfo(&mati.Textures.RoughnessMap, textures, "RoughnessMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.SpecularMap, textures, "SpecularMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.IndexOfRefractionMap, textures, "IndexOfRefractionMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.ExtinctionCoefficientMap, textures, "ExtinctionCoefficientMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.DisplacementMap, textures, "DisplacementMap", blockSize, location);
			InitMaterialInfo(&mati.Textures.PorosityMap, textures, "PorosityMap", blockSize, location);

			mati.Constants.Albedo.R = constAlb->get("R")->toFloat(0);
			mati.Constants.Albedo.G = constAlb->get("G")->toFloat(0);
			mati.Constants.Albedo.B = constAlb->get("B")->toFloat(0);
			mati.Constants.Albedo.A = constAlb->get("A")->toFloat(0);
			mati.Constants.Metallic = constant->get("Metallic")->toFloat(0);
			mati.Constants.Roughness = constant->get("Roughness")->toFloat(0);
			mati.Constants.Porosity = readPorosity(constant);
			if (constFrs->isArray())
			{
				for (size_t a = 0; a < 3; a++)
				{
					mati.Constants.Fresnel[a].RefractionIndex = constFrs->get(a)->get("RefractionIndex")->toFloat(0);
					mati.Constants.Fresnel[a].ExtinctionCoefficient = constFrs->get(a)->get("ExtinctionCoefficient")->toFloat(0);
				}
			}
			else
			{
				for (size_t a = 0; a < 3; a++)
				{
					mati.Constants.Fresnel[a].RefractionIndex = constFrs->get("RefractionIndex")->toFloat(0);
					mati.Constants.Fresnel[a].ExtinctionCoefficient = constFrs->get("ExtinctionCoefficient")->toFloat(0);
				}
			}

			if (checkColor(mati.Constants.Albedo) == false)
			{
				CloakEngine::Global::Log::WriteToLog("Error: Albedo values must be between zero and one", CloakEngine::Global::Log::Type::Info);
				return false;
			}
			if (checkValue(mati.Constants.Metallic) == false)
			{
				CloakEngine::Global::Log::WriteToLog("Error: Metallic value must be between zero and one", CloakEngine::Global::Log::Type::Info);
				return false;
			}
		}
		else
		{
			CloakEngine::Global::Log::WriteToLog("Error: Unknown material type '" + matT + "'. Possible values are 'Opaque', 'AlphaMapped' or 'Transparent'", CloakEngine::Global::Log::Type::Info);
			return false;
		}
		return true;
	}
	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags, const CloakEngine::Files::CompressType* compress)
	{
		std::u16string idict = CloakEngine::Helper::StringConvert::ConvertToU16(inputPath);
		std::u16string odict = CloakEngine::Helper::StringConvert::ConvertToU16(outputPath);
		std::u16string tdict = CloakEngine::Helper::StringConvert::ConvertToU16(tempPath);
		CloakEngine::List<CloakEngine::Files::FileInfo> paths;
		CloakEngine::Files::listAllFilesInDictionary(idict, CloakEngine::Helper::StringConvert::ConvertToU16(CLOAK_MESHFILE_LIST), &paths);
		CloakEngine::Files::IConfiguration* list = nullptr;
		CREATE_INTERFACE(CE_QUERY_ARGS(&list));
		if (paths.size() == 0)
		{
			CloakEngine::Global::Log::WriteToLog((std::string)"No " + CLOAK_MESHFILE_LIST + "-Files found", CloakEngine::Global::Log::Type::Info);
		}
		else
		{
			for (size_t pathNum = 0; pathNum < paths.size(); pathNum++)
			{
				const CloakEngine::Files::FileInfo& fi = paths[pathNum];
				list->readFile(fi.FullPath, CloakEngine::Files::ConfigType::JSON);
				CloakEngine::Files::IValue* root = list->getRootValue();
				CloakEngine::Files::IValue* config = root->get("Config");
				CloakEngine::Files::IValue* mesh = root->get("Mesh");

				const std::u16string topDir = fi.RootPath;
				CloakEngine::Global::Log::WriteToLog(u"Encoding IImage '" + fi.RelativePath + u"'...", CloakEngine::Global::Log::Type::Info);
				CloakEngine::Files::CompressType ct = CloakEngine::Files::CompressType::LZC;
				bool readErr = false;

				const std::string compress = config->get("Compress")->toString("LZC");
				if (compress.compare("NONE") == 0) { ct = CloakEngine::Files::CompressType::NONE; }
				else if (compress.compare("LZC") == 0) { ct = CloakEngine::Files::CompressType::LZC; }
				else if (compress.compare("LZW") == 0) { ct = CloakEngine::Files::CompressType::LZW; }
				else if (compress.compare("HUFFMAN") == 0) { ct = CloakEngine::Files::CompressType::HUFFMAN; }
				else if (compress.compare("DEFLATE") == 0) { ct = CloakEngine::Files::CompressType::LZH; }
				else if (compress.compare("LZ4") == 0) { ct = CloakEngine::Files::CompressType::LZ4; }
				else if (compress.compare("ZLIB") == 0) { ct = CloakEngine::Files::CompressType::ZLIB; }
				else
				{
					CloakEngine::Global::Log::WriteToLog("Error: Unknown compress type '" + compress + "'. Possible values are 'NONE', 'LZC', 'LZW', 'HUFFMAN', 'DEFLATE', 'LZ4' and 'ZLIB'", CloakEngine::Global::Log::Type::Info);
					readErr = true;
				}
				CloakEngine::Files::UGID cgid;
				const std::string target = config->get("Target")->toString("Test");
				if (target.compare("Test") == 0) { cgid = GameID::Test; }
				else if (target.compare("Editor") == 0) { cgid = GameID::Editor; }
				else
				{
					CloakEngine::Global::Log::WriteToLog("Error: Unknown target '" + target + "'. Possible values are 'Test' or 'Editor'", CloakEngine::Global::Log::Type::Info);
					readErr = true;
				}

				std::u16string location = topDir;
				if (mesh->has("Location"))
				{
					location = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(mesh->get("Location")->toString());
					const char16_t llc = location[location.length() - 1];
					if (llc != u'/' && llc != u'\\') { location += u"\\"; }
				}

				const bool compileMesh = mesh->has("File");
				std::u16string meshFile = compileMesh ? location + CloakEngine::Helper::StringConvert::ConvertToU16(mesh->get("File")->toString()) : u"";
				meshFile = CloakEngine::Files::GetFullFilePath(meshFile);
				std::u16string matLoc = CloakEngine::Files::GetFullFilePath(location);
				const std::string boundType = mesh->get("BoundingType")->toString("OOB");
				CloakCompiler::Mesh::BoundingVolume bv;
				if (boundType.compare("NONE") == 0) { bv = CloakCompiler::Mesh::BoundingVolume::None; }
				else if (boundType.compare("OOB") == 0 || boundType.compare("OOBB") == 0) { bv = CloakCompiler::Mesh::BoundingVolume::OOBB; }
				else if (boundType.compare("Sphere") == 0) { bv = CloakCompiler::Mesh::BoundingVolume::Sphere; }
				else
				{
					CloakEngine::Global::Log::WriteToLog("Error: Unknown bounding volume setting '" + boundType + "'. Possible values are 'NONE', 'OOB', 'OOBB' (which is an alias for 'OOB') or 'Sphere'", CloakEngine::Global::Log::Type::Info);
					readErr = true;
				}
				bool uIB = mesh->get("IndexBuffer")->toBool(true);

				CloakCompiler::Image::SupportedQuality supQual;

				const std::string quality = mesh->get("Quality")->toString("LOW");
				if (quality.compare("LOW") == 0) { supQual = CloakCompiler::Image::SupportedQuality::LOW; }
				else if (quality.compare("MEDIUM") == 0) { supQual = CloakCompiler::Image::SupportedQuality::MEDIUM; }
				else if (quality.compare("HIGH") == 0) { supQual = CloakCompiler::Image::SupportedQuality::HIGH; }
				else if (quality.compare("ULTRA") == 0) { supQual = CloakCompiler::Image::SupportedQuality::ULTRA; }
				else
				{
					CloakEngine::Global::Log::WriteToLog("Error: Unknown quality '" + quality + "'. Possible values are 'LOW', 'MEDIUM', 'HIGH' or 'ULTRA'", CloakEngine::Global::Log::Type::Info);
					readErr = true;
				}
				unsigned int blockSize = static_cast<unsigned int>(mesh->get("BlockSize")->toInt());

				CloakCompiler::Mesh::Desc desc;
				std::vector<CloakCompiler::Image::ImageInfo> mats;
				std::string err;
				if (CloakCompiler::Converter::ConvertOBJFile(meshFile, supQual, blockSize, compileMesh ? &desc : nullptr, &mats, &err, matLoc) == false)
				{
					CloakEngine::Global::Log::WriteToLog("Error: " + err, CloakEngine::Global::Log::Type::Info);
					readErr = true;
				}
				else if (err.length() > 0)
				{
					CloakEngine::Global::Log::WriteToLog("Warning: " + err, CloakEngine::Global::Log::Type::Info);
				}
				CloakDebugLog("Read " + std::to_string(mats.size()) + " Materials");
				desc.Bounding = bv;
				desc.UseIndexBuffer = uIB;

				if (readErr == false)
				{
#if (defined(COMPILE_MATERIAL) || defined(COMPILE_MESH))
					CloakCompiler::EncodeDesc encode;
					encode.flags = CloakCompiler::EncodeFlags::NONE;
					if ((flags & CompilerFlags::ForceRecompile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_READ; }
					if ((flags & CompilerFlags::NoTempFile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_WRITE; }
					encode.targetGameID = cgid;
					encode.tempPath = tdict + u"\\" + fi.RelativePath;

#ifdef COMPILE_MATERIAL
					CloakEngine::Files::IValue* material = mesh->get("Material");
					compileMaterials(material, encode, mats, fi, odict, supQual, blockSize, ct, location);
#endif

#ifdef COMPILE_MESH
					if (compileMesh)
					{
						CloakEngine::Files::IWriter* write = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&write));
						write->SetTarget(odict + u"\\" + fi.RelativePath, CloakCompiler::Mesh::FileType, ct, encode.targetGameID);

						CloakCompiler::Mesh::EncodeToFile(write, encode, desc, [=](CloakCompiler::Mesh::RespondeInfo ri) {
							switch (ri.Code)
							{
								case CloakCompiler::Mesh::RespondeCode::CALC_MATERIALS:
									CloakEngine::Global::Log::WriteToLog("Reorder material IDs", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::CALC_BINORMALS:
									CloakEngine::Global::Log::WriteToLog("Calculating vertex data", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::CALC_BOUNDING:
									CloakEngine::Global::Log::WriteToLog("Calculating bounding volume", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::CALC_SORT:
									CloakEngine::Global::Log::WriteToLog("Sorting data by materials", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::CALC_BONES:
									CloakEngine::Global::Log::WriteToLog("Calculating bone count", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::CALC_INDICES:
									CloakEngine::Global::Log::WriteToLog("Calculating index buffer", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::WRITE_VERTICES:
									CloakEngine::Global::Log::WriteToLog("Writing vertices", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::WRITE_BOUNDING:
									CloakEngine::Global::Log::WriteToLog("Write bounding volume", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::WRITE_TMP:
									CloakEngine::Global::Log::WriteToLog("Writing temp file", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::CHECK_TMP:
									CloakEngine::Global::Log::WriteToLog("Reading temp file", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::FINISH_SUCCESS:
									CloakEngine::Global::Log::WriteToLog(u"Mesh '" + fi.RelativePath + u"' encoded!", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::FINISH_ERROR:
									CloakEngine::Global::Log::WriteToLog(u"Failed to encode mesh '" + fi.RelativePath + u"'!", CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::ERROR_BINORMALS:
									CloakEngine::Global::Log::WriteToLog("Failed calculating vertex data of vertex " + std::to_string(ri.Vertex) + " (polygon " + std::to_string(ri.Polygon) + "): " + ri.Msg, CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::ERROR_BOUNDING:
									CloakEngine::Global::Log::WriteToLog("Failed to calculate bounding volume: " + ri.Msg, CloakEngine::Global::Log::Type::Info);
									break;
								case CloakCompiler::Mesh::RespondeCode::ERROR_VERTEXREF:
									CloakEngine::Global::Log::WriteToLog("Vertex-ID out of bounds: " + ri.Msg, CloakEngine::Global::Log::Type::Info);
									break;
								default:
									break;
							}
						});
						write->Save();
						write->Release();
					}
					else
					{
						CloakEngine::Global::Log::WriteToLog("No Mesh was encoded!", CloakEngine::Global::Log::Type::Info);
					}
#endif
#else
					CloakEngine::Global::Log::WriteToLog("Read file: Vertices (" + std::to_string(desc.Vertices.size()) + "):");
					for (size_t a = 0; a < desc.Vertices.size(); a++)
					{
						const CloakCompiler::Mesh::Vertex& v = desc.Vertices[a];
						CloakEngine::Global::Math::Point p(v.Position);
						CloakEngine::Global::Math::Point n(v.Normal);
						std::string r = "\t"+std::to_string(a)+": Position = [" + std::to_string(p.X) + "|" + std::to_string(p.Y) + "|" + std::to_string(p.Z) + "] Normal = [" + std::to_string(n.X) + "|" + std::to_string(n.Y) + "|" + std::to_string(n.Z) + "] TexCoord = [" + std::to_string(v.TexCoord.U) + "|" + std::to_string(v.TexCoord.V) + "]";
						CloakEngine::Global::Log::WriteToLog(r);
					}
					CloakEngine::Global::Log::WriteToLog("Polygons (" + std::to_string(desc.Polygons.size()) + "):");
					for (size_t a = 0; a < desc.Polygons.size(); a++)
					{
						const CloakCompiler::Mesh::Polygon& p = desc.Polygons[a];
						std::string r = "\t" + std::to_string(a) + ": Points = [" + std::to_string(p.Point[0]) + "|" + std::to_string(p.Point[1]) + "|" + std::to_string(p.Point[2]) + "] Material = " + std::to_string(p.Material) + " AutoGenerate Normal: " + std::to_string(p.AutoGenerateNormal);
						CloakEngine::Global::Log::WriteToLog(r);
					}
#endif
				}

				list->removeUnused();
				list->saveFile(fi.FullPath, CloakEngine::Files::ConfigType::JSON);
			}
		}

		SAVE_RELEASE(list);
	}
}