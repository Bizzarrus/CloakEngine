#include "stdafx.h"
#include "FontCompiler.h"

#include "CloakCompiler/Font.h"

#define CLOAK_FONTFILE_LIST "CFFL"

namespace FontCompiler {
	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags, const CloakEngine::Files::CompressType* compress)
	{
		std::u16string idict = CloakEngine::Helper::StringConvert::ConvertToU16(inputPath);
		std::u16string odict = CloakEngine::Helper::StringConvert::ConvertToU16(outputPath);
		std::u16string tdict = CloakEngine::Helper::StringConvert::ConvertToU16(tempPath);
		CloakEngine::List<CloakEngine::Files::FileInfo> paths;
		CloakEngine::Files::listAllFilesInDictionary(idict, CloakEngine::Helper::StringConvert::ConvertToU16(CLOAK_FONTFILE_LIST), &paths);
		CloakEngine::Files::IConfiguration* list = nullptr;
		CREATE_INTERFACE(CE_QUERY_ARGS(&list));
		CloakEngine::Files::CompressType ct = CloakEngine::Files::CompressType::LZC;
		if (paths.size() == 0)
		{
			CloakEngine::Global::Log::WriteToLog((std::string)"No " + CLOAK_FONTFILE_LIST + "-Files found", CloakEngine::Global::Log::Type::Info);
		}
		else
		{
			for (size_t a = 0; a < paths.size(); a++)
			{
				CloakEngine::Files::UGID cgid;

				const CloakEngine::Files::FileInfo& fi = paths[a];
				list->readFile(fi.FullPath, CloakEngine::Files::ConfigType::INI);
				CloakEngine::Files::IValue* root = list->getRootValue();
				const std::u16string topDir = fi.RootPath;
				CloakEngine::Global::Log::WriteToLog(u"Encoding Font '" + fi.RelativePath + u"'...", CloakEngine::Global::Log::Type::Info);
				CloakCompiler::Font::Desc desc;

				desc.BitLen = static_cast<unsigned int>(root->get("Config")->get("iBitLen")->toInt(8));
				desc.SpaceWidth = static_cast<unsigned int>(root->get("Config")->get("iSpaceWidth")->toInt(8));
				std::string comp = root->get("Config")->get("sCompressType")->toString("LZC");
				if (comp.compare("NONE") == 0) { ct = CloakEngine::Files::CompressType::NONE; }
				else if (comp.compare("LZC") == 0) { ct = CloakEngine::Files::CompressType::LZC; }
				else if (comp.compare("LZW") == 0) { ct = CloakEngine::Files::CompressType::LZW; }
				else if (comp.compare("HUFFMAN") == 0) { ct = CloakEngine::Files::CompressType::HUFFMAN; }
				else if (comp.compare("DEFLATE") == 0) { ct = CloakEngine::Files::CompressType::LZH; }
				else if (comp.compare("LZ4") == 0) { ct = CloakEngine::Files::CompressType::LZ4; }
				else if (comp.compare("ZLIB") == 0) { ct = CloakEngine::Files::CompressType::ZLIB; }
				else
				{
					CloakEngine::Global::Log::WriteToLog("Error: Unknown compress type '" + comp + "'. Possible values are 'NONE', 'LZC', 'LZW', 'HUFFMAN', 'DEFLATE', 'LZ4' and 'ZLIB'", CloakEngine::Global::Log::Type::Info);
					continue;
				}
				if (compress != nullptr) { ct = *compress; }
				int gid = root->get("Config")->get("iTargetCGID")->toInt(0);
				if (gid == 0) { cgid = GameID::Test; }
				else if (gid == 1) { cgid = GameID::Editor; }

				CloakEngine::Files::IValue* section = nullptr;
				CloakCompiler::Font::Letter letter;
				for (size_t a = 0; root->enumerateChildren(a, &section); a++)
				{
					if (section->isObjekt())
					{
						CloakEngine::Files::IValue* color = section->get("iColorType");
						CloakEngine::Files::IValue* file = section->get("sFile");
						if (color->isInt() && file->isString())
						{
							const unsigned int colTyp = static_cast<unsigned int>(color->toInt(0));
							switch (colTyp)
							{
								case 0:
									letter.Colored = CloakCompiler::Font::LetterColorType::SINGLECOLOR;
									break;
								case 1:
									letter.Colored = CloakCompiler::Font::LetterColorType::MULTICOLOR;
									break;
								case 2:
									letter.Colored = CloakCompiler::Font::LetterColorType::TRUECOLOR;
									break;
								default:
									letter.Colored = CloakCompiler::Font::LetterColorType::SINGLECOLOR;
									break;
							}
							std::u16string path = CloakEngine::Helper::StringConvert::ConvertToU16(file->toString(""));
							std::string s = section->getName();
							if (path.length() > 0)
							{
								letter.FilePath = topDir + path;
								if (section->getName().length() == 1)
								{
									desc.Alphabet[CloakEngine::Helper::StringConvert::ConvertToU32(s)[0]] = letter;
								}
								else if (s.substr(0, 1).compare("u") == 0 && s.length() == 5)
								{
									try {
										char32_t l = CloakEngine::Helper::StringConvert::ConvertToU32(static_cast<char16_t>(std::stoi(s.substr(1, 4), nullptr, 16)));
										desc.Alphabet[l] = letter;
									}
									catch (...) {}
								}
								else if (s.substr(0, 1).compare("U") == 0 && s.length() == 9)
								{
									try {
										char32_t l = static_cast<char32_t>(std::stoul(s.substr(1, 8), nullptr, 16));
										desc.Alphabet[l] = letter;
									}
									catch (...) {}
								}
							}
						}
					}
				}
				list->removeUnused();
				list->saveFile(fi.FullPath, CloakEngine::Files::ConfigType::INI);
				if (desc.Alphabet.size() > 0)
				{
					CloakCompiler::EncodeDesc encode;
					encode.flags = CloakCompiler::EncodeFlags::NONE;
					if ((flags & CompilerFlags::ForceRecompile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_READ; }
					if ((flags & CompilerFlags::NoTempFile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_WRITE; }
					encode.targetGameID = cgid;
					encode.tempPath = tdict + u"//" + fi.RelativePath;

					CloakEngine::Files::IWriter* write = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&write));
					write->SetTarget(odict + u"\\" + fi.RelativePath, CloakCompiler::Font::FileType, ct, encode.targetGameID);
					CloakCompiler::Font::EncodeToFile(write, encode, desc, [=](const CloakCompiler::Font::RespondeInfo& ri) {
						if (ri.Type == CloakCompiler::Font::RespondeType::COMPLETED)
						{
							CloakEngine::Global::Log::WriteToLog(u"Compiled character '" + CloakEngine::Helper::StringConvert::ConvertToU16(static_cast<std::u32string>(U"") + ri.Letter) + u"'", CloakEngine::Global::Log::Type::Info);
						}
					});
					write->Save();
					write->Release();
				}
				CloakEngine::Global::Log::WriteToLog(u"Font '" + fi.RelativePath + u"' encoded!", CloakEngine::Global::Log::Type::Info);
			}
		}
		SAVE_RELEASE(list);
	}
}