#include "stdafx.h"
#include "LanguageCompiler.h"

#include "CloakCompiler/Language.h"

#define CLOAK_LANGFILE_LIST "CLTL"

namespace LanguageCompiler {
	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags, const CloakEngine::Files::CompressType* compress)
	{
		std::u16string idict = CloakEngine::Helper::StringConvert::ConvertToU16(inputPath);
		std::u16string odict = CloakEngine::Helper::StringConvert::ConvertToU16(outputPath);
		std::u16string tdict = CloakEngine::Helper::StringConvert::ConvertToU16(tempPath);
		CloakEngine::List<CloakEngine::Files::FileInfo> paths;
		CloakEngine::Files::listAllFilesInDictionary(idict, CloakEngine::Helper::StringConvert::ConvertToU16(CLOAK_LANGFILE_LIST), &paths);
		CloakEngine::Files::IConfiguration* list = nullptr;
		CREATE_INTERFACE(CE_QUERY_ARGS(&list));
		if (paths.size() == 0)
		{
			CloakEngine::Global::Log::WriteToLog((std::string)"No " + CLOAK_LANGFILE_LIST + "-Files found", CloakEngine::Global::Log::Type::Info);
		}
		else
		{
			for (size_t a = 0; a < paths.size(); a++)
			{
				const CloakEngine::Files::FileInfo& fi = paths[a];
				list->readFile(fi.FullPath, CloakEngine::Files::ConfigType::INI);
				CloakEngine::Files::IValue* root = list->getRootValue();
				CloakEngine::Global::Log::WriteToLog(u"Encoding Localisation '" + fi.RelativePath + u"'...", CloakEngine::Global::Log::Type::Info);
				CloakCompiler::Language::v1002::Desc desc;
				CloakCompiler::Language::v1002::HeaderDesc hdesc;
				hdesc.CreateHeader = root->get("Config")->get("bHeader")->toBool(false);
				hdesc.HeaderName = CloakEngine::Helper::StringConvert::ConvertToU8(fi.RelativePath);
				hdesc.Path = odict + u"//" + fi.RelativePath;
				CloakEngine::Files::CompressType ct = CloakEngine::Files::CompressType::LZC;
				std::string ict = root->get("Config")->get("sCompressType")->toString("LZC");
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
				if (compress != nullptr) { ct = *compress; }

				CloakEngine::Files::UGID cgid;
				int gid = root->get("Config")->get("iTargetCGID")->toInt(0);
				if (gid == 0) { cgid = GameID::Test; }
				else if (gid == 1) { cgid = GameID::Editor; }

				CloakEngine::Files::IValue* langNames = root->get("Languages");
				std::vector<std::pair<size_t, std::string>> Langs;
				CloakEngine::Files::IValue* child = nullptr;
				for (size_t a = 0; langNames->enumerateChildren(a, &child); a++)
				{
					Langs.push_back(std::make_pair(static_cast<size_t>(child->toInt(0)), child->getName()));
				}

				std::vector<std::string> ids;
				for (size_t a = 0; a < Langs.size(); a++)
				{
					CloakCompiler::Language::v1002::LanguageDesc langDesc;
					CloakCompiler::Language::v1002::LanguageDesc* ld = &(desc.Translation[Langs[a].first] = langDesc);

					CloakEngine::Files::IValue* lang = root->get(Langs[a].second);
					CloakEngine::Files::IValue* key = nullptr;
					for (size_t b = 0; lang->enumerateChildren(b, &key); b++)
					{
						CloakCompiler::Language::v1002::TextID fid = 0;
						for (CloakCompiler::Language::v1002::TextID c = 0; c < ids.size(); c++)
						{
							if (ids[c].compare(key->getName()) == 0) { fid = c; goto fid_found; }
						}
						ids.push_back(key->getName());
						fid = ids.size() - 1;

						fid_found:
						ld->Text[fid] = CloakEngine::Helper::StringConvert::ConvertToU16(key->toString());
					}
				}
				for (size_t a = 0; a < ids.size(); a++) { desc.TextNames[a] = ids[a]; }

				list->removeUnused();
				list->saveFile(fi.FullPath, CloakEngine::Files::ConfigType::INI);

				CloakCompiler::EncodeDesc encode;
				encode.flags = CloakCompiler::EncodeFlags::NONE;
				if ((flags & CompilerFlags::ForceRecompile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_READ; }
				if ((flags & CompilerFlags::NoTempFile) != 0) { encode.flags |= CloakCompiler::EncodeFlags::NO_TEMP_WRITE; }
				encode.targetGameID = cgid;
				encode.tempPath = tdict + u"//" + fi.RelativePath;

				CloakEngine::Files::IWriter* write = nullptr;
				CREATE_INTERFACE(CE_QUERY_ARGS(&write));
				write->SetTarget(odict + u"//" + fi.RelativePath, CloakCompiler::Language::v1002::FileType, ct, encode.targetGameID);
				CloakCompiler::Language::v1002::EncodeToFile(write, encode, desc, hdesc, [=](const CloakCompiler::Language::v1002::RespondeInfo& ri) {
					switch (ri.Code)
					{
						case CloakCompiler::Language::v1002::ResponseCode::FINISH_LANGUAGE:
						{
							for (size_t a = 0; a < Langs.size(); a++)
							{
								if (Langs[a].first == ri.Language)
								{
									CloakEngine::Global::Log::WriteToLog("Encode Language '" + Langs[a].second + "'", CloakEngine::Global::Log::Type::Info);
									break;
								}
							}
							break;
						}
						case CloakCompiler::Language::v1002::ResponseCode::FINISH_TEXT:
						{
							//Notice: May disable this output with large files
							for (size_t a = 0; a < Langs.size(); a++)
							{
								if (Langs[a].first == ri.Language)
								{
									CloakEngine::Global::Log::WriteToLog("Encode Text '" + ids[ri.Text] + "' of language '" + Langs[a].second + "'", CloakEngine::Global::Log::Type::Info);
									break;
								}
							}
							break;
						}
						case CloakCompiler::Language::v1002::ResponseCode::READ_TEMP:
						{
							break;
						}
						case CloakCompiler::Language::v1002::ResponseCode::WRITE_TEMP:
						{
							CloakEngine::Global::Log::WriteToLog("Write temp file...", CloakEngine::Global::Log::Type::Info);
							break;
						}
						case CloakCompiler::Language::v1002::ResponseCode::WRITE_HEADER:
						{
							CloakEngine::Global::Log::WriteToLog("Write header...", CloakEngine::Global::Log::Type::Info);
							break;
						}
					}
				});
				write->Save();
				write->Release();
				CloakEngine::Global::Log::WriteToLog(u"Localisation '" + fi.RelativePath + u"' encoded!", CloakEngine::Global::Log::Type::Info);
			}
		}
		list->Release();
	}
}