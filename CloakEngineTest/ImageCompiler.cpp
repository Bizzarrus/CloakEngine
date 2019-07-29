#include "stdafx.h"
#include "ImageCompiler.h"

#include "CloakCompiler/Image.h"
#include "MeshCompiler.h"

#include <codecvt>
#include <vector>

#define CLOAK_IMAGEFILE_LIST "CIFL"

namespace ImageCompiler {
	inline bool CLOAK_CALL readIBI(In CloakEngine::Files::IValue* i, In CloakEngine::Files::IValue* path, In const std::u16string& topDir, Out CloakCompiler::Image::BuildInfo* ibi)
	{
		ibi->Meta.BlockSize = static_cast<unsigned int>(i->get("BlockSize")->toInt());

		const std::string space = i->get("ColorSpace")->toString("RGBA");
		if (space.compare("RGBA") == 0) { ibi->Meta.Space = CloakCompiler::Image::ColorSpace::RGBA; }
		else if (space.compare("YCCA") == 0) { ibi->Meta.Space = CloakCompiler::Image::ColorSpace::YCCA; }
		else if (space.compare("Material") == 0) { ibi->Meta.Space = CloakCompiler::Image::ColorSpace::Material; }
		else
		{
			CloakEngine::Global::Log::WriteToLog("Error: Unknown color space '" + space + "'. Possible values are 'RGBA' or 'YCCA'", CloakEngine::Global::Log::Type::Info);
			return true;
		}

		const std::string type = i->get("Type")->toString("Normal");
		if (type.compare("Bordered") == 0)
		{
			ibi->Meta.Type = CloakCompiler::Image::BuildType::BORDERED;
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("TopLeft")->toString());
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("Top")->toString());
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("TopRight")->toString());
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("Left")->toString());
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("Center")->toString());
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("Right")->toString());
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("BottomLeft")->toString());
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("Bottom")->toString());
			ibi->Bordered.Center = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->get("BottomRight")->toString());
		}
		else if (type.compare("Normal") == 0)
		{
			ibi->Meta.Type = CloakCompiler::Image::BuildType::NORMAL;
			ibi->Normal.Path = topDir + CloakEngine::Helper::StringConvert::ConvertToU16(path->toString());
		}
		else
		{
			CloakEngine::Global::Log::WriteToLog("Error: Unknown image type '" + type + "'. Possible values are 'Bordered' or 'Normal'", CloakEngine::Global::Log::Type::Info);
			return true;
		}
		return false;
	}

	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags, const CloakEngine::Files::CompressType* compress)
	{
		std::u16string idict = CloakEngine::Helper::StringConvert::ConvertToU16(inputPath);
		std::u16string odict = CloakEngine::Helper::StringConvert::ConvertToU16(outputPath);
		std::u16string tdict = CloakEngine::Helper::StringConvert::ConvertToU16(tempPath);
		CloakEngine::List<CloakEngine::Files::FileInfo> paths;
		CloakEngine::Files::listAllFilesInDictionary(idict, CloakEngine::Helper::StringConvert::ConvertToU16(CLOAK_IMAGEFILE_LIST), &paths);
		CloakEngine::Files::IConfiguration* list = nullptr;
		CREATE_INTERFACE(CE_QUERY_ARGS(&list));
		if (paths.size() == 0)
		{
			CloakEngine::Global::Log::WriteToLog((std::string)"No " + CLOAK_IMAGEFILE_LIST + "-Files found", CloakEngine::Global::Log::Type::Info);
		}
		else
		{
			for (size_t pathNum = 0; pathNum < paths.size(); pathNum++)
			{
				const CloakEngine::Files::FileInfo& fi = paths[pathNum];
				list->readFile(fi.FullPath, CloakEngine::Files::ConfigType::JSON);
				CloakEngine::Files::IValue* root = list->getRootValue();
				CloakEngine::Files::IValue* config = root->get("Config");
				CloakEngine::Files::IValue* imgs = root->get("Images");
				
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
					CloakEngine::Global::Log::WriteToLog("Error: Unknown compress type '"+compress+"'. Possible values are 'NONE', 'LZC', 'LZW', 'HUFFMAN', 'DEFLATE', 'LZ4' and 'ZLIB'", CloakEngine::Global::Log::Type::Info); 
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

				for (size_t a = 0; a < imgs->size(); a++)
				{
					CloakCompiler::Image::ImageInfo imi;
					CloakEngine::Files::IValue* img = imgs->get(a);
					CloakEngine::Files::IValue* textures = img->get("Textures");

					const std::string heap = img->get("Heap")->toString("GUI");
					if (heap.compare("GUI") == 0) { imi.Heap = CloakCompiler::Image::Heap::GUI; }
					else if (heap.compare("World") == 0) { imi.Heap = CloakCompiler::Image::Heap::WORLD; }
					else if (heap.compare("Utility") == 0) { imi.Heap = CloakCompiler::Image::Heap::UTILITY; }
					else
					{
						CloakEngine::Global::Log::WriteToLog("Error: Unknown image heap '" + heap + "'. Possible values are 'GUI', 'World' or 'Utility'", CloakEngine::Global::Log::Type::Info);
						readErr = true;
					}

					const std::string quality = img->get("Quality")->toString("LOW");
					if (quality.compare("LOW") == 0) { imi.SupportedQuality = CloakCompiler::Image::SupportedQuality::LOW; }
					else if (quality.compare("MEDIUM") == 0) { imi.SupportedQuality = CloakCompiler::Image::SupportedQuality::MEDIUM; }
					else if (quality.compare("HIGH") == 0) { imi.SupportedQuality = CloakCompiler::Image::SupportedQuality::HIGH; }
					else if (quality.compare("ULTRA") == 0) { imi.SupportedQuality = CloakCompiler::Image::SupportedQuality::ULTRA; }
					else
					{
						CloakEngine::Global::Log::WriteToLog("Error: Unknown quality '" + quality + "'. Possible values are 'LOW', 'MEDIUM', 'HIGH' or 'ULTRA'", CloakEngine::Global::Log::Type::Info);
						readErr = true;
					}

					const std::string imgType = img->get("Type")->toString("List");
					if (imgType.compare("List") == 0)
					{
						imi.Type = CloakCompiler::Image::ImageInfoType::LIST;
						imi.List.AlphaMap = img->get("CreateAlphaMap")->toBool(false);
						imi.List.Flags = img->get("DynamicSize")->toBool(false) ? CloakCompiler::Image::SizeFlag::DYNAMIC : CloakCompiler::Image::SizeFlag::FIXED;
						imi.List.MaxMipMapCount = img->get("MipMapCount")->toInt(0);
						CloakCompiler::Image::ColorChannel toEncode = CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeR")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_1 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeG")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_2 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeB")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_3 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeA")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_4 : CloakCompiler::Image::ColorChannel::Channel_None;
						imi.List.EncodingChannels = toEncode;
						for (size_t b = 0; b < textures->size(); b++)
						{
							CloakCompiler::Image::BuildInfo ibi;
							CloakEngine::Files::IValue* i = textures->get(b);
							CloakEngine::Files::IValue* path = i->get("Path");
							readErr = readErr || readIBI(i, path, topDir, &ibi);
							imi.List.Images.push_back(ibi);
						}
					}
					else if (imgType.compare("TextureArray") == 0)
					{
						imi.Type = CloakCompiler::Image::ImageInfoType::ARRAY;
						imi.Array.MaxMipMapCount = img->get("MipMapCount")->toInt(0);
						CloakCompiler::Image::ColorChannel toEncode = CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeR")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_1 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeG")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_2 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeB")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_3 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeA")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_4 : CloakCompiler::Image::ColorChannel::Channel_None;
						imi.Array.EncodingChannels = toEncode;

						for (size_t b = 0; b < textures->size(); b++)
						{
							CloakCompiler::Image::BuildInfo ibi;
							CloakEngine::Files::IValue* i = textures->get(b);
							CloakEngine::Files::IValue* path = i->get("Path");

							readErr = readErr || readIBI(i, path, topDir, &ibi);
							imi.Array.Images.push_back(ibi);
						}
					}
					else if (imgType.compare("TextureCube") == 0)
					{
						imi.Type = CloakCompiler::Image::ImageInfoType::CUBE;
						CloakCompiler::Image::ColorChannel toEncode = CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeR")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_1 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeG")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_2 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeB")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_3 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeA")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_4 : CloakCompiler::Image::ColorChannel::Channel_None;
						imi.Cube.EncodingChannels = toEncode;
						imi.Skybox.MaxMipMapCount = img->get("MipMapCount")->toInt(0);

						if (textures->has("HDRI"))
						{
							imi.Cube.Type = CloakCompiler::Image::CubeType::HDRI;
							CloakEngine::Files::IValue* i = textures->get("HDRI");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Cube.HDRI.Image);
							imi.Cube.HDRI.CubeSize = static_cast<uint32_t>(i->get("CubeSize")->toInt(512));
						}
						else
						{
							imi.Cube.Type = CloakCompiler::Image::CubeType::CUBE;
							CloakEngine::Files::IValue* i = textures->get("Top");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Cube.Cube.Top);
							i = textures->get("Bottom");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Cube.Cube.Bottom);
							i = textures->get("Left");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Cube.Cube.Left);
							i = textures->get("Right");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Cube.Cube.Right);
							i = textures->get("Back");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Cube.Cube.Back);
							i = textures->get("Front");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Cube.Cube.Front);
						}
					}
					else if (imgType.compare("Skybox") == 0)
					{
						imi.Type = CloakCompiler::Image::ImageInfoType::SKYBOX;
						
						CloakCompiler::Image::ColorChannel toEncode = CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeR")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_1 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeG")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_2 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeB")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_3 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeA")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_4 : CloakCompiler::Image::ColorChannel::Channel_None;
						imi.Skybox.EncodingChannels = toEncode;
						imi.Skybox.MaxMipMapCount = img->get("MipMapCount")->toInt(0);

						CloakEngine::Files::IValue* rad = img->get("Radiance");
						imi.Skybox.IBL.Radiance.Color.CubeSize = static_cast<uint32_t>(rad->get("CubeSize")->toInt(512));
						imi.Skybox.IBL.Radiance.Color.NumSamples = static_cast<uint32_t>(rad->get("NumSamples")->toInt(1024));
						imi.Skybox.IBL.Radiance.Color.BlockSize = static_cast<uint16_t>(rad->get("BlockSize")->toInt(0));
						imi.Skybox.IBL.Radiance.Color.MaxMipMapCount = static_cast<uint32_t>(rad->get("MipMapCount")->toInt(1024));

						CloakEngine::Files::IValue* lut = img->get("LUT");
						imi.Skybox.IBL.Radiance.LUT.NumSamples = static_cast<uint32_t>(lut->get("NumSamples")->toInt(1024));
						imi.Skybox.IBL.Radiance.LUT.RoughnessSamples = static_cast<uint32_t>(lut->get("RoughnessSamples")->toInt(512));
						imi.Skybox.IBL.Radiance.LUT.CosNVSamples = static_cast<uint32_t>(lut->get("DirectionSamples")->toInt(512));
						imi.Skybox.IBL.Radiance.LUT.BlockSize = static_cast<uint16_t>(lut->get("BlockSize")->toInt(0));

						if (textures->has("HDRI"))
						{
							imi.Skybox.Type = CloakCompiler::Image::CubeType::HDRI;
							CloakEngine::Files::IValue* i = textures->get("HDRI");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Skybox.HDRI.Image);
							imi.Skybox.HDRI.CubeSize = static_cast<uint32_t>(i->get("CubeSize")->toInt(512));
						}
						else
						{
							imi.Skybox.Type = CloakCompiler::Image::CubeType::CUBE;
							CloakEngine::Files::IValue* i = textures->get("Top");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Skybox.Cube.Top);
							i = textures->get("Bottom");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Skybox.Cube.Bottom);
							i = textures->get("Left");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Skybox.Cube.Left);
							i = textures->get("Right");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Skybox.Cube.Right);
							i = textures->get("Back");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Skybox.Cube.Back);
							i = textures->get("Front");
							readErr = readErr || readIBI(i, i->get("Path"), topDir, &imi.Skybox.Cube.Front);
						}
					}
					else if (imgType.compare("Animation") == 0)
					{
						imi.Type = CloakCompiler::Image::ImageInfoType::ANIMATION;
						imi.Animation.AlphaMap = img->get("CreateAlphaMap")->toBool(false);
						imi.Animation.MaxMipMapCount = img->get("MipMapCount")->toInt(0);
						CloakCompiler::Image::ColorChannel toEncode = CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeR")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_1 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeG")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_2 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeB")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_3 : CloakCompiler::Image::ColorChannel::Channel_None;
						toEncode |= img->get("EncodeA")->toBool(true) ? CloakCompiler::Image::ColorChannel::Channel_4 : CloakCompiler::Image::ColorChannel::Channel_None;
						imi.Animation.EncodingChannels = toEncode;

						for (size_t b = 0; b < textures->size(); b++)
						{
							CloakCompiler::Image::AnimationInfo ibi;
							CloakEngine::Files::IValue* i = textures->get(b);
							CloakEngine::Files::IValue* path = i->get("Path");

							ibi.ShowTime = static_cast<CloakEngine::Global::Time>(i->get("ShowTime")->toInt());
							readErr = readErr || readIBI(i, path, topDir, &ibi);

							imi.Animation.Images.push_back(ibi);
						}
					}
					else if (imgType.compare("Material") == 0)
					{
						imi.Type = CloakCompiler::Image::ImageInfoType::MATERIAL;

						if (MeshCompiler::readMaterialFromConfig(&imi, img, static_cast<unsigned int>(img->get("BlockSize")->toInt())) == false) { readErr = true; }
					}
					else
					{
						CloakEngine::Global::Log::WriteToLog("Error: Unknown image type '" + imgType + "'. Possible values are 'List', 'TextureArray', 'TextureCube', 'Skybox', 'Material' or 'Animation'", CloakEngine::Global::Log::Type::Info);
						readErr = true;
					}
					desc.Imgs.push_back(imi);
				}

				list->removeUnused();
				list->saveFile(fi.FullPath, CloakEngine::Files::ConfigType::JSON);

				if (readErr == false)
				{
					CloakCompiler::EncodeDesc encode;
					encode.flags = CloakCompiler::EncodeFlags::NONE;
					if ((flags & CompilerFlags::ForceRecompile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_READ; }
					if ((flags & CompilerFlags::NoTempFile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_WRITE; }
					encode.targetGameID = cgid;
					encode.tempPath = tdict + u"/" + fi.RelativePath;

					CloakEngine::Files::IWriter* write = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&write));
					write->SetTarget(odict + u"/" + fi.RelativePath, CloakCompiler::Image::FileType, ct, encode.targetGameID);
					compilePrepared(write, desc, encode, fi);
					write->Save();
					write->Release();
				}
				else
				{
					CloakEngine::Global::Log::WriteToLog("An Error occured!", CloakEngine::Global::Log::Type::Info);
				}
			}
		}
		SAVE_RELEASE(list);
	}
	void CLOAK_CALL compilePrepared(CloakEngine::Files::IWriter* write, CloakCompiler::Image::Desc desc, const CloakCompiler::EncodeDesc& encode, const CloakEngine::Files::FileInfo& fi)
	{
		CloakCompiler::Image::EncodeToFile(write, encode, desc, [=](const CloakCompiler::Image::RespondeInfo& ri) {
			std::u16string imgPath = u"";
			std::u16string imgNum = u"";
			if (ri.ImgNum > 0)
			{
				imgNum = u"Image " + CloakEngine::Helper::StringConvert::ConvertToU16(std::to_string(ri.ImgNum - 1));
				if (ri.ImgSubNum > 0)
				{
					imgPath = imgNum + u": Texture " + CloakEngine::Helper::StringConvert::ConvertToU16(std::to_string(ri.ImgSubNum - 1));
					const CloakCompiler::Image::ImageInfo& imi = desc.Imgs[ri.ImgNum - 1];
					switch (imi.Type)
					{
						case CloakCompiler::Image::ImageInfoType::LIST:
						{
							const CloakCompiler::Image::BuildInfo& ibi = imi.List.Images[ri.ImgSubNum - 1];
							if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
							else { imgPath += u"<Bordered Image>"; }
							break;
						}
						case CloakCompiler::Image::ImageInfoType::ARRAY:
						{
							const CloakCompiler::Image::BuildInfo& ibi = imi.Array.Images[ri.ImgSubNum - 1];
							if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
							else { imgPath += u"<Bordered Image>"; }
							break;
						}
						case CloakCompiler::Image::ImageInfoType::CUBE:
						{
							if (ri.ImgSubNum - 1 == 0)
							{
								const CloakCompiler::Image::BuildInfo& ibi = imi.Cube.Cube.Front;
								if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
								else { imgPath += u"<Bordered Image>"; }
							}
							else if (ri.ImgSubNum - 1 == 1)
							{
								const CloakCompiler::Image::BuildInfo& ibi = imi.Cube.Cube.Back;
								if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
								else { imgPath += u"<Bordered Image>"; }
							}
							else if (ri.ImgSubNum - 1 == 2)
							{
								const CloakCompiler::Image::BuildInfo& ibi = imi.Cube.Cube.Left;
								if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
								else { imgPath += u"<Bordered Image>"; }
							}
							else if (ri.ImgSubNum - 1 == 3)
							{
								const CloakCompiler::Image::BuildInfo& ibi = imi.Cube.Cube.Right;
								if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
								else { imgPath += u"<Bordered Image>"; }
							}
							else if (ri.ImgSubNum - 1 == 4)
							{
								const CloakCompiler::Image::BuildInfo& ibi = imi.Cube.Cube.Top;
								if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
								else { imgPath += u"<Bordered Image>"; }
							}
							else if (ri.ImgSubNum - 1 == 5)
							{
								const CloakCompiler::Image::BuildInfo& ibi = imi.Cube.Cube.Bottom;
								if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
								else { imgPath += u"<Bordered Image>"; }
							}
							break;
						}
						case CloakCompiler::Image::ImageInfoType::ANIMATION:
						{
							const CloakCompiler::Image::BuildInfo& ibi = imi.Animation.Images[ri.ImgSubNum - 1];
							if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" ('" + ibi.Normal.Path + u"')"; }
							else { imgPath += u"<Bordered Image>"; }
							break;
						}
						case CloakCompiler::Image::ImageInfoType::MATERIAL:
						{
							const CloakCompiler::Image::BuildInfo* ibip = nullptr;
							switch (imi.Material.Type)
							{
								case CloakCompiler::Image::MaterialType::Opaque:
									switch (ri.ImgSubNum)
									{
										case 1: ibip = &imi.Material.Opaque.Textures.Albedo; imgPath += u" (Albedo"; break;
										case 2: ibip = &imi.Material.Opaque.Textures.NormalMap; imgPath += u" (NormalMap"; break;
										case 3: ibip = &imi.Material.Opaque.Textures.Metallic; imgPath += u" (Metallic"; break;
										case 4: ibip = &imi.Material.Opaque.Textures.AmbientOcclusion; imgPath += u" (AmbientOcclusion"; break;
										case 5: ibip = &imi.Material.Opaque.Textures.RoughnessMap; imgPath += u" (RoughnessMap"; break;
										case 6: ibip = &imi.Material.Opaque.Textures.SpecularMap; imgPath += u" (SpecularMap"; break;
										case 7: ibip = &imi.Material.Opaque.Textures.IndexOfRefractionMap; imgPath += u" (IndexOfRefractionMap"; break;
										case 8: ibip = &imi.Material.Opaque.Textures.ExtinctionCoefficientMap; imgPath += u" (ExtinctionCoefficientMap"; break;
										case 9: ibip = &imi.Material.Opaque.Textures.DisplacementMap; imgPath += u" (DisplacementMap"; break;
										default: break;
									}
									break;
								case CloakCompiler::Image::MaterialType::AlphaMapped:
									switch (ri.ImgSubNum)
									{
										case 1: ibip = &imi.Material.AlphaMapped.Textures.Albedo; imgPath += u" (Albedo"; break;
										case 2: ibip = &imi.Material.AlphaMapped.Textures.NormalMap; imgPath += u" (NormalMap"; break;
										case 3: ibip = &imi.Material.AlphaMapped.Textures.Metallic; imgPath += u" (Metallic"; break;
										case 4: ibip = &imi.Material.AlphaMapped.Textures.AmbientOcclusion; imgPath += u" (AmbientOcclusion"; break;
										case 5: ibip = &imi.Material.AlphaMapped.Textures.RoughnessMap; imgPath += u" (RoughnessMap"; break;
										case 6: ibip = &imi.Material.AlphaMapped.Textures.SpecularMap; imgPath += u" (SpecularMap"; break;
										case 7: ibip = &imi.Material.AlphaMapped.Textures.IndexOfRefractionMap; imgPath += u" (IndexOfRefractionMap"; break;
										case 8: ibip = &imi.Material.AlphaMapped.Textures.ExtinctionCoefficientMap; imgPath += u" (ExtinctionCoefficientMap"; break;
										case 9: ibip = &imi.Material.AlphaMapped.Textures.DisplacementMap; imgPath += u" (DisplacementMap"; break;
										case 10:ibip = &imi.Material.AlphaMapped.Textures.AlphaMap; imgPath += u" (AlphaMap"; break;
										default: break;
									}
									break;
								case CloakCompiler::Image::MaterialType::Transparent:
									switch (ri.ImgSubNum)
									{
										case 1: ibip = &imi.Material.Transparent.Textures.Albedo; imgPath += u" (Albedo"; break;
										case 2: ibip = &imi.Material.Transparent.Textures.NormalMap; imgPath += u" (NormalMap"; break;
										case 3: ibip = &imi.Material.Transparent.Textures.Metallic; imgPath += u" (Metallic"; break;
										case 4: ibip = &imi.Material.Transparent.Textures.AmbientOcclusion; imgPath += u" (AmbientOcclusion"; break;
										case 5: ibip = &imi.Material.Transparent.Textures.RoughnessMap; imgPath += u" (RoughnessMap"; break;
										case 6: ibip = &imi.Material.Transparent.Textures.SpecularMap; imgPath += u" (SpecularMap"; break;
										case 7: ibip = &imi.Material.Transparent.Textures.IndexOfRefractionMap; imgPath += u" (IndexOfRefractionMap"; break;
										case 8: ibip = &imi.Material.Transparent.Textures.ExtinctionCoefficientMap; imgPath += u" (ExtinctionCoefficientMap"; break;
										case 9: ibip = &imi.Material.Transparent.Textures.DisplacementMap; imgPath += u" (DisplacementMap"; break;
										default: break;
									}
									break;
								default:
									break;
							}
							if (ibip != nullptr)
							{
								const CloakCompiler::Image::BuildInfo& ibi = *ibip;
								if (ibi.Meta.Type == CloakCompiler::Image::BuildType::NORMAL) { imgPath += u" '" + ibi.Normal.Path + u"')"; }
								else { imgPath += u" <Bordered Image>)"; }
							}
							else { imgPath += u" <Unknown Material Image>"; }
							break;
						}
						default:
							break;
					}
				}
			}
			std::string texName = CloakEngine::Helper::StringConvert::ConvertToU8(imgPath);
			std::string imgName = CloakEngine::Helper::StringConvert::ConvertToU8(imgNum);
			switch (ri.State)
			{
				case CloakCompiler::Image::RespondeState::ERR_COLOR_BITS:
				case CloakCompiler::Image::RespondeState::ERR_NO_IMGS:
				case CloakCompiler::Image::RespondeState::ERR_QUALITY:
				case CloakCompiler::Image::RespondeState::ERR_ILLEGAL_PARAM:
				case CloakCompiler::Image::RespondeState::ERR_IMG_BUILDING:
				case CloakCompiler::Image::RespondeState::ERR_IMG_SIZE:
				case CloakCompiler::Image::RespondeState::ERR_BLOCKSIZE:
				case CloakCompiler::Image::RespondeState::ERR_COLOR_CHANNELS:
				case CloakCompiler::Image::RespondeState::ERR_CHANNEL_MISMATCH:
				case CloakCompiler::Image::RespondeState::ERR_BLOCKMETA_MISMATCH:
					if (ri.ImgNum == 0) { CloakEngine::Global::Log::WriteToLog("Error "+std::to_string(static_cast<size_t>(ri.State))+": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					else if (ri.ImgSubNum == 0) { CloakEngine::Global::Log::WriteToLog("Error " + std::to_string(static_cast<size_t>(ri.State)) + ": " + imgName + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					else { CloakEngine::Global::Log::WriteToLog("Error " + std::to_string(static_cast<size_t>(ri.State)) + ": " + texName + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					break;
				case CloakCompiler::Image::RespondeState::WARN_SHOWTIME:
					if (ri.ImgNum == 0) { CloakEngine::Global::Log::WriteToLog("Warning " + std::to_string(static_cast<size_t>(ri.State)) + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					else if (ri.ImgSubNum == 0) { CloakEngine::Global::Log::WriteToLog("Warning " + std::to_string(static_cast<size_t>(ri.State)) + ": " + imgName + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					else { CloakEngine::Global::Log::WriteToLog("Warning " + std::to_string(static_cast<size_t>(ri.State)) + ": " + texName + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					break;
				case CloakCompiler::Image::RespondeState::INFO:
					if (ri.ImgNum == 0) { CloakEngine::Global::Log::WriteToLog("Info " + std::to_string(static_cast<size_t>(ri.State)) + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					else if (ri.ImgSubNum == 0) { CloakEngine::Global::Log::WriteToLog("Info " + std::to_string(static_cast<size_t>(ri.State)) + ": " + imgName + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					else { CloakEngine::Global::Log::WriteToLog("Info " + std::to_string(static_cast<size_t>(ri.State)) + ": " + texName + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info); }
					break;
				case CloakCompiler::Image::RespondeState::WRITE_CONSTANT_HEADER:
					CloakEngine::Global::Log::WriteToLog("Write constants: " + texName, CloakEngine::Global::Log::Type::Info);
					break;
				case CloakCompiler::Image::RespondeState::CHECK_TEMP:
					CloakEngine::Global::Log::WriteToLog("Checking Temp: " + imgName, CloakEngine::Global::Log::Type::Info);
					break;
				case CloakCompiler::Image::RespondeState::WRITE_TEMP:
					CloakEngine::Global::Log::WriteToLog("Write Temp: " + imgName, CloakEngine::Global::Log::Type::Info);
					break;
				case CloakCompiler::Image::RespondeState::WRITE_UPDATE_INFO:
					CloakEngine::Global::Log::WriteToLog("\t" + imgName + ": " + ri.Msg, CloakEngine::Global::Log::Type::Info);
					break;
				case CloakCompiler::Image::RespondeState::START_IMG_HEADER:
					CloakEngine::Global::Log::WriteToLog("Writing header: " + imgName, CloakEngine::Global::Log::Type::Info);
					break;
				case CloakCompiler::Image::RespondeState::START_IMG_SUB:
					CloakEngine::Global::Log::WriteToLog("Writing layers: " + imgName, CloakEngine::Global::Log::Type::Info);
					break;
				case CloakCompiler::Image::RespondeState::DONE_IMG_HEADER:
					break;
				case CloakCompiler::Image::RespondeState::DONE_IMG_SUB:
					break;
				case CloakCompiler::Image::RespondeState::DONE_FILE:
					CloakEngine::Global::Log::WriteToLog(u"IImage '" + fi.RelativePath + u"' encoded!", CloakEngine::Global::Log::Type::Info);
					break;
				default:
					break;
			}
		});
	}
}