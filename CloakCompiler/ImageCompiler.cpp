#include "stdafx.h"
#include "CloakCompiler/Image.h"

#include "Engine/LibWriter.h"
#include "Engine/Image/TextureEncoding.h"
#include "Engine/Image/EntryList.h"
#include "Engine/Image/EntryArray.h"
#include "Engine/Image/EntryCube.h"
#include "Engine/Image/EntryAnimation.h"
#include "Engine/Image/EntryMaterial.h"
#include "Engine/Image/EntrySkybox.h"

#include <sstream>

namespace CloakCompiler {
	namespace API {
		namespace Image {
			namespace v1008 {
				constexpr size_t MAX_BIT_SIZE = 32;

				namespace Lib {
					constexpr const char* GET_START = "bool __fastcall GetImage";
					constexpr const char* GET_MAIN_ARGS = "std::uint32_t entryID, std::uint32_t imageID, Image* result";

					void CLOAK_CALL WriteDataTypes(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount)
					{
						Engine::Lib::WriteWithTabs(header, tabCount, "enum FORMAT_TYPE {");
						for (size_t a = 0; a < ARRAYSIZE(Engine::Image::Lib::FORMAT_TYPE_NAMES); a++) { Engine::Lib::WriteWithTabs(header, tabCount + 1, "FORMAT_TYPE_" + std::string(Engine::Image::Lib::FORMAT_TYPE_NAMES[a].Name) + " = " + std::to_string(a) + ","); }
						Engine::Lib::WriteWithTabs(header, tabCount, "};");

						Engine::Lib::WriteWithTabs(header, tabCount, "struct Texture {");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "const void* Data;");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "std::uint64_t RowPitch;");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "std::uint64_t SlicePitch;");
						Engine::Lib::WriteWithTabs(header, tabCount, "};");
						Engine::Lib::WriteWithTabs(header, tabCount, "struct Image {");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "struct {");
						Engine::Lib::WriteWithTabs(header, tabCount + 2, "std::uint8_t BitsPerChannel;");
						Engine::Lib::WriteWithTabs(header, tabCount + 2, "std::uint8_t ChannelCount;");
						Engine::Lib::WriteWithTabs(header, tabCount + 2, "FORMAT_TYPE Type;");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "} Format;");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "std::uint32_t Width;");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "std::uint32_t Height;");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "std::uint32_t MipMapCount;");
						Engine::Lib::WriteWithTabs(header, tabCount + 1, "const Texture* MipMaps;");
						Engine::Lib::WriteWithTabs(header, tabCount, "};");
					}
					void CLOAK_CALL WriteGetterDeclerations(In const CE::RefPointer<CloakEngine::Files::IWriter>& header, In size_t tabCount)
					{
						Engine::Lib::WriteWithTabs(header, tabCount, std::string(Lib::GET_START) + "(" + std::string(Lib::GET_MAIN_ARGS) + ");");
					}
				}

				CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In ResponseFunc responseFunc)
				{
					
					CE::RefPointer<CloakEngine::Files::IWriter> file = nullptr;
					if ((desc.WriteMode & WriteMode::File) != WriteMode::None) { file = output; }
					Engine::Image::v1008::ResponseCaller response(responseFunc);
					std::atomic<bool> err = false;
					if (file != nullptr)
					{
						for (size_t a = 0; a < ARRAYSIZE(desc.File.RGBA.Bits) && err == false; a++)
						{
							if (desc.File.RGBA.Bits[a] > MAX_BIT_SIZE || desc.File.RGBA.Bits[a] == 0) { err = true; }
							else { file->WriteBits(6, desc.File.RGBA.Bits[a] - 1); }
						}
						for (size_t a = 0; a < ARRAYSIZE(desc.File.RGBE.Bits) && err == false; a++)
						{
							if (desc.File.RGBE.Bits[a] > MAX_BIT_SIZE || desc.File.RGBA.Bits[a] == 0) { err = true; }
							else { file->WriteBits(6, desc.File.RGBE.Bits[a] - 1); }
						}
						for (size_t a = 0; a < ARRAYSIZE(desc.File.YCCA.Bits) && err == false; a++)
						{
							if (desc.File.YCCA.Bits[a] > MAX_BIT_SIZE || desc.File.YCCA.Bits[a] == 0) { err = true; }
							else { file->WriteBits(6, desc.File.YCCA.Bits[a] - 1); }
						}
						for (size_t a = 0; a < ARRAYSIZE(desc.File.Material.Bits) && err == false; a++)
						{
							if (desc.File.Material.Bits[a] > MAX_BIT_SIZE || desc.File.Material.Bits[a] == 0) { err = true; }
							else { file->WriteBits(6, desc.File.Material.Bits[a] - 1); }
						}
					}
					if (err) { Engine::Image::v1008::SendResponse(response, RespondeState::ERR_COLOR_BITS, "One or more color bits is 0 or greater than " + std::to_string(MAX_BIT_SIZE)); }
					else if (desc.Imgs.size() == 0) { Engine::Image::v1008::SendResponse(response, RespondeState::ERR_NO_IMGS, "Image count is 0"); }
					else
					{
						Engine::Image::v1008::BitInfoList BitInfos = Engine::Image::v1008::BitInfoList();
						for (size_t a = 0; a < 4; a++) { BitInfos.RGBA.Channel[a] = desc.File.RGBA.Bits[a]; }
						for (size_t a = 0; a < 4; a++) { BitInfos.RGBE.Channel[a] = desc.File.RGBE.Bits[a]; }
						for (size_t a = 0; a < 4; a++) { BitInfos.YCCA.Channel[a] = desc.File.YCCA.Bits[a < 2 ? a : (a - 1)]; }
						for (size_t a = 0; a < 4; a++) { BitInfos.Material.Channel[a] = desc.File.Material.Bits[0]; }

						if (file != nullptr)
						{
							file->WriteBits(1, desc.File.NoBackgroundLoading ? 1 : 0);
							file->WriteDynamic(desc.Imgs.size());
						}
						CE::List<std::pair<size_t, Engine::Image::v1008::IEntry*>> entries;
						entries.reserve(desc.Imgs.size());
						CE::RefPointer<CE::Helper::ISyncSection> sync = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&sync));

						CE::Global::Task finish = CE::Global::PushTask([&entries](In size_t threadID) {
							for (size_t a = 0; a < entries.size(); a++) { delete entries[a].second; }
						});
						CE::Global::Task checkTemp = nullptr;
						CE::Global::Task writeLib = nullptr;
						Engine::Lib::CMakeFileAsync cmake(desc.Lib.Name);
						//Set up file build path
						if (file != nullptr)
						{
							file->MoveToNextByte();
							CE::RefPointer<CE::Files::IMultithreadedWriteBuffer> fileBuffer = CE::Files::CreateMultithreadedWriteBuffer(file, CE::Global::Threading::Flag::IO);
							fileBuffer->SetFinishTask(finish);
							CE::Global::Task writeTemp = CE::Global::PushTask([&entries, finish, &err, &encode, &BitInfos, &response](In size_t threadID) {
								if (err == false)
								{
									if ((encode.flags & EncodeFlags::NO_TEMP_WRITE) == EncodeFlags::NONE)
									{
										for (size_t a = 0; a < entries.size(); a++)
										{
											CE::Global::Task t = CE::Global::PushTask([&entries, a, &encode, &BitInfos, &response](In size_t threadID) {
												CLOAK_ASSUME(a < entries.size());
												entries[a].second->WriteTemp(encode, BitInfos, response, a + 1);
											});
											finish.AddDependency(t);
											t.Schedule(CE::Global::Threading::Flag::IO, threadID);
										}
									}
								}
							});
							finish.AddDependency(writeTemp);
							CE::Global::Task writeLayers = CE::Global::PushTask([&entries, writeTemp, &err, fileBuffer](In size_t threadID) {
								if (err == false)
								{
									size_t maxLayCount = 0;
									for (size_t a = 0; a < entries.size(); a++)
									{
										const size_t mlc = entries[a].second->GetMaxLayerLength();
										maxLayCount = max(maxLayCount, mlc);
									}
									CE::Global::Task nextLayer = writeTemp;
									for (size_t a = 0; a < maxLayCount + 3; a++)
									{
										CE::Global::Task layer = CE::Global::PushTask([nextLayer, a, &entries, fileBuffer](In size_t threadID) {
											for (size_t b = 0; b < entries.size(); b++)
											{
												CE::Global::Task t = CE::Global::PushTask([&entries, a, b, fileBuffer](In size_t threadID) {
													CLOAK_ASSUME(b < entries.size());
													const auto& r = entries[b];
													CE::RefPointer<CE::Files::IWriter> output = fileBuffer->CreateLocalWriter();
													output->WriteDynamic(r.first);
													r.second->WriteLayer(output, a + 1);
													output->Save();
												});
												CLOAK_ASSUME(nextLayer.IsScheduled() == false);
												nextLayer.AddDependency(t);
												t.Schedule(threadID);
											}
										});
										CLOAK_ASSUME(nextLayer.IsScheduled() == false);
										nextLayer.AddDependency(layer);
										nextLayer.Schedule(threadID);
										nextLayer = layer;
										layer.Schedule(threadID);
									}
									nextLayer.Schedule(threadID);
								}
							});
							writeTemp.AddDependency(writeLayers);
							CE::Global::Task writeHeader = CE::Global::PushTask([&entries, &err, writeLayers, fileBuffer](In size_t threadID) {
								for (size_t a = 0; a < entries.size(); a++) { err = err || entries[a].second->GotError(); }
								if (err == false)
								{
									for (size_t a = 0; a < entries.size(); a++)
									{
										CE::Global::Task t = CE::Global::PushTask([&entries, a, fileBuffer](In size_t threadID) {
											CLOAK_ASSUME(a < entries.size());
											const auto& r = entries[a];
											CE::RefPointer<CE::Files::IWriter> output = fileBuffer->CreateLocalWriter();
											output->WriteDynamic(r.first);
											r.second->WriteHeader(output);
											output->Save();
										});
										writeLayers.AddDependency(t);
										t.Schedule(threadID);
									}
								}
							});
							writeLayers.AddDependency(writeHeader);
							checkTemp = CE::Global::PushTask([&entries, &err, &encode, writeHeader, &BitInfos, &response](In size_t threadID) {
								for (size_t a = 0; a < entries.size(); a++) { err = err || entries[a].second->GotError(); }
								if (err == false)
								{
									for (size_t a = 0; a < entries.size(); a++)
									{
										CE::Global::Task enc = CE::Global::PushTask([&entries, a, &BitInfos, &response](In size_t threadID) {
											CLOAK_ASSUME(a < entries.size());
											entries[a].second->Encode(BitInfos, response, a + 1);
										});
										if ((encode.flags & EncodeFlags::NO_TEMP_READ) == EncodeFlags::NONE)
										{
											CE::Global::Task t = CE::Global::PushTask([&entries, a, &BitInfos, &response, &encode](In size_t threadID) {
												CLOAK_ASSUME(a < entries.size());
												entries[a].second->CheckTemp(encode, BitInfos, response, a + 1);
											});
											enc.AddDependency(t);
											t.Schedule(CE::Global::Threading::Flag::IO, threadID);
										}
										writeHeader.AddDependency(enc);
										enc.Schedule(threadID);
									}
								}
							});
							writeHeader.AddDependency(checkTemp);
							writeHeader.Schedule();
							writeLayers.Schedule();
							writeTemp.Schedule();
						}
						//Set up lib build path
						if ((desc.WriteMode & WriteMode::Lib) != WriteMode::None)
						{
							CE::Global::Task writeMainHeader = CE::Global::PushTask([&desc](In size_t threadID) {
								CE::RefPointer<CloakEngine::Files::IWriter> header = nullptr;
								CREATE_INTERFACE(CE_QUERY_ARGS(&header));
								header->SetTarget(desc.Lib.Path.Append(desc.Lib.Name), "H", CloakEngine::Files::CompressType::NONE, true);
								Engine::Lib::WriteHeaderIntro(header, Engine::Lib::Type::Image, desc.Lib.Name, desc.Lib.Path.GetHash());

								Engine::Lib::WriteWithTabs(header, 0, "#ifdef CLOAKENGINE_VERSION");
								Engine::Lib::WriteWithTabs(header, 1, "#include \"CloakEngine/Defines.h\"");
								Engine::Lib::WriteWithTabs(header, 1, "#include \"CloakEngine/Helper/Types.h\"");
								Engine::Lib::WriteWithTabs(header, 1, "#include \"CloakEngine/Rendering/Defines.h\"");
								Engine::Lib::WriteWithTabs(header, 0, "#endif");
								Engine::Lib::WriteWithTabs(header, 0, "");
								Engine::Lib::WriteWithTabs(header, 0, "#ifdef _MSC_VER");
								Engine::Lib::WriteWithTabs(header, 1, "#pragma comment(lib, \"" + desc.Lib.Name + ".lib\")");
								Engine::Lib::WriteWithTabs(header, 0, "#endif");
								Engine::Lib::WriteWithTabs(header, 0, "");
								size_t tabCount = Engine::Lib::WriteCommonIntro(header, Engine::Lib::Type::Image, desc.Lib.Name);
								Engine::Lib::WriteWithTabs(header, tabCount, "constexpr std::uint32_t Version = " + std::to_string(FileType.Version) + ";");
								Engine::Lib::WriteWithTabs(header, tabCount, "constexpr std::uint32_t EntryCount = " + std::to_string(desc.Imgs.size()) + ";");
								Lib::WriteDataTypes(header, tabCount);
								Lib::WriteGetterDeclerations(header, tabCount);
								Engine::Lib::WriteWithTabs(header, 0, "#ifdef CLOAKENGINE_VERSION");
								Engine::Lib::WriteWithTabs(header, tabCount, "CE::Rendering::Format CLOAK_CALL GetFormat(In const Image& img)");
								Engine::Lib::WriteWithTabs(header, tabCount, "{");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "constexpr CE::Rendering::Format Table[] = {");
								for (size_t a = 0; a < ARRAYSIZE(Engine::Image::Lib::FORMAT_TYPE_NAMES); a++)
								{
									for (size_t b = 8; b <= 32; b <<= 1)
									{
										for (size_t c = 0; c < 4; c++)
										{
											if (b < Engine::Image::Lib::FORMAT_TYPE_NAMES[a].MinMemorySize || b > Engine::Image::Lib::FORMAT_TYPE_NAMES[a].MaxMemorySize)
											{
												Engine::Lib::WriteWithTabs(header, tabCount + 2, "CE::Rendering::Format::UNKNOWN,");
											}
											else
											{
												std::string name = "CE::Rendering::Format::";
												switch (c)
												{
													case 3: name = "A" + std::to_string(b) + name;
													case 2: name = "B" + std::to_string(b) + name;
													case 1: name = "G" + std::to_string(b) + name;
													case 0: name = "R" + std::to_string(b) + name;
													default: name = name + "_" + Engine::Image::Lib::FORMAT_TYPE_NAMES[a].Name; break;
												}
												Engine::Lib::WriteWithTabs(header, tabCount + 2, name + ",");
											}
										}
									}
								}
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "};");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "uint32_t v = img.Format.ChannelCount;");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "switch(img.Format.BitsPerChannel)");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "{");
								Engine::Lib::WriteWithTabs(header, tabCount + 2, "case 8: v += 0; break;");
								Engine::Lib::WriteWithTabs(header, tabCount + 2, "case 16: v += 4; break;");
								Engine::Lib::WriteWithTabs(header, tabCount + 2, "case 32: v += 8; break;");
								Engine::Lib::WriteWithTabs(header, tabCount + 2, "default: return CE::Rendering::Format::UNKNOWN;");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "}");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "switch(img.Format.Type)");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "{");
								for (size_t a = 0; a < ARRAYSIZE(Engine::Image::Lib::FORMAT_TYPE_NAMES); a++)
								{
									Engine::Lib::WriteWithTabs(header, tabCount + 2, "case FORMAT_TYPE_" + std::string(Engine::Image::Lib::FORMAT_TYPE_NAMES[a].Name) + ": v += " + std::to_string(12 * a) + "; break;");
								}
								Engine::Lib::WriteWithTabs(header, tabCount + 2, "default: return CE::Rendering::Format::UNKNOWN;");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "}");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "if(v > 0) { return Table[v - 1]; }");
								Engine::Lib::WriteWithTabs(header, tabCount + 1, "return CE::Rendering::Format::UNKNOWN;");
								Engine::Lib::WriteWithTabs(header, tabCount, "}");
								Engine::Lib::WriteWithTabs(header, 0, "#endif");
								tabCount = Engine::Lib::WriteCommonOutro(header, tabCount);
								Engine::Lib::WriteHeaderOutro(header, 0);
							});
							finish.AddDependency(writeMainHeader);
							writeMainHeader.Schedule(CE::Global::Threading::Flag::IO);
							CE::Global::Task compileLib = CE::Global::PushTask([&err, &desc](In size_t threadID) {
								if (err == false)
								{
									Engine::Lib::Compile(desc.Lib.Path, desc.Lib.Generator);
								}
							});
							finish.AddDependency(compileLib);
							CE::Global::Task writeCommonHeader = CE::Global::PushTask([&desc](In size_t threadID) {
								CE::RefPointer<CloakEngine::Files::IWriter> file = Engine::Lib::CreateSourceFile(desc.Lib.Path, "Common", "H");

								Engine::Lib::WriteHeaderIntro(file, Engine::Lib::Type::Image, desc.Lib.Name, desc.Lib.Path.GetHash());
								size_t tabCount = Engine::Lib::WriteCommonIntro(file, Engine::Lib::Type::Image, desc.Lib.Name);
								Lib::WriteDataTypes(file, tabCount);
								Lib::WriteGetterDeclerations(file, tabCount);
								tabCount = Engine::Lib::WriteCommonOutro(file, tabCount);
								Engine::Lib::WriteHeaderOutro(file, 0);
							});
							compileLib.AddDependency(writeCommonHeader);
							writeCommonHeader.Schedule(CE::Global::Threading::Flag::IO);

							writeLib = CE::Global::PushTask([&entries, &desc, &err, compileLib, &cmake, &BitInfos](In size_t threadID) {
								if (err == false)
								{
									CE::Global::Task writeMainFile = CE::Global::PushTask([&entries, &desc](In size_t threadID) {
										CE::RefPointer<CloakEngine::Files::IWriter> file = Engine::Lib::CreateSourceFile(desc.Lib.Path, "Main", "CPP");
										Engine::Lib::WriteCopyright(file);

										Engine::Lib::WriteWithTabs(file, 0, "#include \"Common.h\"");
										for (size_t a = 0; a < entries.size(); a++) { Engine::Lib::WriteWithTabs(file, 0, "#include \"Entry" + std::to_string(entries[a].first) + ".h\""); }
										Engine::Lib::WriteWithTabs(file, 0, "");
										size_t tabCount = Engine::Lib::WriteCommonIntro(file, Engine::Lib::Type::Image, desc.Lib.Name);
										Engine::Lib::WriteWithTabs(file, tabCount, std::string(Lib::GET_START) + "(" + std::string(Lib::GET_MAIN_ARGS) + ")");
										Engine::Lib::WriteWithTabs(file, tabCount, "{");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "if(result == nullptr) { return false; }");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->Format.R = 0;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->Format.G = 0;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->Format.B = 0;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->Format.A = 0;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->Format.Type = FORMAT_TYPE_UINT;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->Width = 0;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->Height = 0;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->MipMapCount = 0;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "result->MipMaps = nullptr;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "switch(entryID)");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "{");
										for (size_t a = 0; a < entries.size(); a++)
										{
											const auto& b = entries[a];
											Engine::Lib::WriteWithTabs(file, tabCount + 2, "case " + std::to_string(b.first) + ": return Get" + std::to_string(b.first) + "(imageID, result);");
										}
										Engine::Lib::WriteWithTabs(file, tabCount + 2, "default: return false;");
										Engine::Lib::WriteWithTabs(file, tabCount + 1, "}");
										Engine::Lib::WriteWithTabs(file, tabCount, "}");
										tabCount = Engine::Lib::WriteCommonOutro(file, tabCount);
									});
									compileLib.AddDependency(writeMainFile);
									writeMainFile.Schedule(CE::Global::Threading::Flag::IO);
									for (size_t a = 0; a < entries.size(); a++)
									{
										CE::Global::Task entryFiles = CE::Global::PushTask([&desc, &entries, a, compileLib, &cmake, &BitInfos](In size_t threadID) {
											const auto& e = entries[a];
											const std::string hName = "Entry" + std::to_string(e.first);
											cmake.AddFile(hName + ".cpp");

											CE::RefPointer<CloakEngine::Files::IWriter> file = Engine::Lib::CreateSourceFile(desc.Lib.Path, hName, "CPP");
											Engine::Lib::WriteCopyright(file);
											Engine::Lib::WriteWithTabs(file, 0, "#include \"" + hName + ".h\"");
											Engine::Lib::WriteWithTabs(file, 0, "");
											e.second->WriteLibFiles(file, desc.Lib, BitInfos.RGBA, e.first, cmake, compileLib);
											file->Save();
										});
										CE::Global::Task entryHeader = CE::Global::PushTask([&desc, &entries, a, &cmake](In size_t threadID) {
											const auto& e = entries[a];
											const std::string hName = "Entry" + std::to_string(e.first);
											cmake.AddFile(hName + ".h");

											CE::RefPointer<CloakEngine::Files::IWriter> file = Engine::Lib::CreateSourceFile(desc.Lib.Path, hName, "H");
											Engine::Lib::WriteHeaderIntro(file, Engine::Lib::Type::Image, desc.Lib.Name + "_" + hName, desc.Lib.Path.GetHash());
											Engine::Lib::WriteWithTabs(file, 0, "#include \"Common.h\"");
											Engine::Lib::WriteWithTabs(file, 0, "");
											size_t tabCount = Engine::Lib::WriteCommonIntro(file, Engine::Lib::Type::Image, desc.Lib.Name);
											Engine::Lib::WriteWithTabs(file, tabCount, "bool Get" + std::to_string(e.first) + "(std::uint32_t imageID, Image* result);");
											tabCount = Engine::Lib::WriteCommonOutro(file, tabCount);
											Engine::Lib::WriteHeaderOutro(file, 0);
											file->Save();
										});
										compileLib.AddDependency(entryFiles);
										compileLib.AddDependency(entryHeader);
										entryFiles.Schedule(CE::Global::Threading::Flag::IO);
										entryHeader.Schedule(CE::Global::Threading::Flag::IO);
									}
								}
							});
							compileLib.AddDependency(writeLib);
							CE::Global::Task writeCMakeFileStart = CE::Global::PushTask([&desc, &cmake, compileLib](In size_t threadID) {
								cmake.Create(Engine::Lib::Type::Image, desc.Lib.Path, desc.Lib.SharedLib, compileLib);
								cmake.AddFile("Common.h", true);
								cmake.AddFile("Main.cpp", false);
							});
							writeLib.AddDependency(writeCMakeFileStart);

							writeCMakeFileStart.Schedule();
							compileLib.Schedule();
						}

						
						for (size_t a = 0; a < desc.Imgs.size(); a++)
						{
							CE::Global::Task t = CE::Global::PushTask([&desc, a, &response, &err, sync, &entries](In size_t threadID) {
								const ImageInfo& i = desc.Imgs[a];
								uint8_t QualityOffset = 0;
								switch (i.SupportedQuality)
								{
									case SupportedQuality::LOW:
										QualityOffset = 0;
										break;
									case SupportedQuality::MEDIUM:
										QualityOffset = 1;
										break;
									case SupportedQuality::HIGH:
										QualityOffset = 2;
										break;
									case SupportedQuality::ULTRA:
										QualityOffset = 3;
										break;
									default:
										err = true;
										Engine::Image::v1008::SendResponse(response, RespondeState::ERR_ILLEGAL_PARAM, a + 1, "Unknown quality");
										return;
								}
								Engine::Image::v1008::IEntry* ne = nullptr;
								switch (i.Type)
								{
									case ImageInfoType::LIST:
										ne = new Engine::Image::v1008::EntryList(desc.Imgs[a].List, response, a + 1, QualityOffset, i.Heap);
										break;
									case ImageInfoType::ARRAY:
										ne = new Engine::Image::v1008::EntryArray(desc.Imgs[a].Array, response, a + 1, QualityOffset, i.Heap);
										break;
									case ImageInfoType::CUBE:
										ne = new Engine::Image::v1008::EntryCube(desc.Imgs[a].Cube, response, a + 1, QualityOffset, i.Heap);
										break;
									case ImageInfoType::ANIMATION:
										ne = new Engine::Image::v1008::EntryAnimation(desc.Imgs[a].Animation, response, a + 1, QualityOffset, i.Heap);
										break;
									case ImageInfoType::MATERIAL:
										ne = new Engine::Image::v1008::EntryMaterial(desc.Imgs[a].Material, response, a + 1, QualityOffset, i.Heap);
										break;
									case ImageInfoType::SKYBOX:
										ne = new Engine::Image::v1008::EntrySkybox(desc.Imgs[a].Skybox, response, a + 1, QualityOffset, i.Heap);
										break;
									default:
										err = true;
										Engine::Image::v1008::SendResponse(response, RespondeState::ERR_ILLEGAL_PARAM, a + 1, "Unknown type");
										return;
								}
								CLOAK_ASSUME(ne != nullptr);
								CE::Helper::Lock lock(sync);
								entries.push_back(std::make_pair(a, ne));
							});
							checkTemp.AddDependency(t);
							writeLib.AddDependency(t);
							finish.AddDependency(t);
							t.Schedule();
						}
						checkTemp.Schedule();
						writeLib.Schedule();
						finish.Schedule();
						finish.WaitForExecution();
					}
					Engine::Image::v1008::SendResponse(response, RespondeState::DONE_FILE);
				}
			}
		}
	}
}

//TODO: Delete test formats
#define TEST_FORM
#ifdef TEST_FORM
namespace ImageTestForm {
	enum FORMAT_TYPE {FLOAT, UNORM, UINT, SNORM, SINT};
	struct Texture {
		const void* Data;
		std::uint64_t RowPitch;
		std::uint64_t SlicePitch;
	};
	struct Image {
		struct {
			std::uint8_t BitsPerChannel;
			std::uint8_t ChannelCount;
			FORMAT_TYPE Type;
		} Format;
		std::uint32_t Width;
		std::uint32_t Height;
		std::uint32_t MipMapCount;
		const Texture* MipMaps;
	};

	const uint8_t TD1[] = { 1, 2, 3, 4, 5, 6, 7, 5, 4 };
	const uint8_t TD2[] = { 2, 4, 8, 7, 1, 3, 4, 8, 9 };
	constexpr Texture Data[] = {
		{TD1, 3, 9},
		{TD2, 3, 9}
	};
	constexpr Image Info{ {8, 3, UINT}, 16, 32, 2, Data };

	CE::Rendering::Format CLOAK_CALL Get(In const Image& img)
	{
		constexpr CE::Rendering::Format ft[] = {
			CE::Rendering::Format::R8G8B8A8_SINT,
		};
		return ft[0];
	}
}
#endif