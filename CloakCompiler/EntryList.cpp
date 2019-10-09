#include "stdafx.h"
#include "Engine/Image/EntryList.h"
#include "Engine/Image/MipMapGeneration.h"
#include "Engine/Image/TextureEncoding.h"
#include "Engine/TempHandler.h"

#include <sstream>

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			namespace v1008 {
				namespace EntryList_v1 {
					constexpr CloakEngine::Files::FileType g_TempFileType{ "ImageTemp","CEIT",1008 };

					CLOAK_CALL EntryList::EntryList(In const API::Image::v1008::IMAGE_INFO_LIST& img, In const ResponseCaller& response, In size_t imgNum, In uint8_t qualityOffset, In API::Image::v1008::Heap heap)
					{
						if (img.Images.size() == 0) { SendResponse(response, API::Image::v1008::RespondeState::ERR_NO_IMGS, imgNum, "No textures were given!"); m_err = true; }
						m_channel = img.EncodingChannels;
						m_alphaMap = img.AlphaMap;
						m_sizeFlag = img.Flags;
						m_heap = heap;
						m_qualityOffset = qualityOffset;
						m_texCount = 0;
						m_layerCount = 0;
						m_useTemp = false;
						m_err = false;
						size_t imgCount = img.Images.size();
						m_textures = NewArray(MipTexture, imgCount);
						m_metas = NewArray(TextureMeta, imgCount);
						Engine::TextureLoader::Image i;
						for (size_t a = 0; a < imgCount && m_err == false; a++)
						{
							HRESULT hRet = Engine::ImageBuilder::BuildImage(img.Images[a], &i, &m_metas[m_texCount].Builder);
							m_metas[m_texCount].Encoding = img.Images[a].Meta;
							if (FAILED(hRet)) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_BUILDING, imgNum, "Couldn't build image", a + 1); m_err = true; }
							else
							{
								CalculateMipMaps(img.MaxMipMapCount, qualityOffset, i, img.Images[a].Meta, &m_textures[m_texCount]);
								if (m_textures[m_texCount].levelCount <= qualityOffset) { SendResponse(response, API::Image::v1008::RespondeState::ERR_QUALITY, imgNum, "Image is not large enought to support this quality setting", a + 1); m_err = true; }
								m_layerCount = max(m_layerCount, m_textures[m_texCount].levelCount);
								m_texCount++;
							}
						}
						if (m_err == false)
						{
							m_layerCount++;
							m_layers = NewArray(LayerBuffer, m_layerCount);
							for (size_t a = 0; a < m_layerCount; a++)
							{
								m_layers[a].Buffer = CloakEngine::Files::CreateVirtualWriteBuffer();
								m_layers[a].BitLength = 0;
								m_layers[a].ByteLength = 0;
							}
						}
						if (m_err == true)
						{
							DeleteArray(m_textures);
							DeleteArray(m_metas);
							m_texCount = 0;
							m_textures = nullptr;
							m_metas = nullptr;
						}
					}
					CLOAK_CALL EntryList::~EntryList()
					{
						DeleteArray(m_textures);
						DeleteArray(m_metas);
						DeleteArray(m_layers);
					}
					bool CLOAK_CALL_THIS EntryList::GotError() const { return m_err; }
					void CLOAK_CALL_THIS EntryList::CheckTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
					{
						if (m_err) { return; }
						CloakEngine::Files::IReader* read = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&read));
						if (read != nullptr)
						{
							const std::u16string tmpPath = encode.tempPath + u"_" + CloakEngine::Helper::StringConvert::ConvertToU16(std::to_string(imgNum));
							bool suc = (read->SetTarget(tmpPath, g_TempFileType, false, true) == g_TempFileType.Version);
							if (suc) { SendResponse(response, API::Image::v1008::RespondeState::CHECK_TEMP, imgNum); }
							if (suc) { suc = !Engine::TempHandler::CheckGameID(read, encode.targetGameID); }
							for (size_t a = 0; a < 4 && suc; a++) { suc = (read->ReadBits(6) + 1 == bitInfos.RGBA.Channel[a].Bits); }
							for (size_t a = 0; a < 4 && suc; a++) { suc = (read->ReadBits(6) + 1 == bitInfos.YCCA.Channel[a].Bits); }
							for (size_t a = 0; a < 4 && suc; a++) { suc = (read->ReadBits(6) + 1 == bitInfos.Material.Channel[a].Bits); }
							if (suc) { suc = (static_cast<API::Image::ImageInfoType>(read->ReadBits(3)) == API::Image::ImageInfoType::LIST); }
							if (suc) { suc = (static_cast<API::Image::v1008::Heap>(read->ReadBits(2)) == m_heap); }
							if (suc) { suc = (static_cast<uint8_t>(read->ReadBits(2)) == m_qualityOffset); }
							if (suc) { suc = (read->ReadDynamic() == m_texCount); }
							for (size_t a = 0; a < m_texCount && suc; a++)
							{
								std::vector<Engine::TempHandler::FileDateInfo> dates;
								Engine::TempHandler::ReadDates(m_metas[a].Builder.CheckPaths, &dates);
								suc = !Engine::TempHandler::CheckDateChanges(dates, read);
								if (suc) { suc = (read->ReadBits(8) == m_metas[a].Builder.BasicFlags); }
								if (suc) { suc = (read->ReadDynamic() == m_metas[a].Encoding.BlockSize); }
								if (suc) { suc = (static_cast<API::Image::v1008::ColorSpace>(read->ReadBits(3)) == m_metas[a].Encoding.Space); }
								if (suc) { suc = (static_cast<uint32_t>(read->ReadBits(10)) == m_textures[a].levelCount); }
							}
							if (suc) { suc = ((read->ReadBits(1) == 1) == m_alphaMap); }
							if (suc) { suc = (static_cast<API::Image::SizeFlag>(read->ReadBits(2)) == m_sizeFlag); }
							if (suc) { suc = (static_cast<API::Image::v1008::ColorChannel>(read->ReadBits(4)) == m_channel); }

							if (suc)
							{
								m_useTemp = true;
								for (size_t a = 0; a < m_layerCount; a++)
								{
									m_layers[a].ByteLength = static_cast<unsigned long long>(read->ReadDynamic());
									m_layers[a].BitLength = static_cast<unsigned int>(read->ReadDynamic());

									CloakEngine::Files::IWriter* tmpWrite = nullptr;
									CREATE_INTERFACE(CE_QUERY_ARGS(&tmpWrite));
									tmpWrite->SetTarget(m_layers[a].Buffer);
									for (unsigned long long b = 0; b < m_layers[a].ByteLength; b++) { tmpWrite->WriteBits(8, read->ReadBits(8)); }
									if (m_layers[a].BitLength > 0) { tmpWrite->WriteBits(m_layers[a].BitLength, read->ReadBits(m_layers[a].BitLength)); }
									tmpWrite->Save();
									SAVE_RELEASE(tmpWrite);
								}
							}

							SAVE_RELEASE(read);
						}
					}
					void CLOAK_CALL_THIS EntryList::WriteTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
					{
						if (m_err || m_useTemp) { return; }
						CloakEngine::Files::IWriter* write = nullptr;
						CREATE_INTERFACE(CE_QUERY_ARGS(&write));
						if (write != nullptr)
						{
							SendResponse(response, API::Image::v1008::RespondeState::WRITE_TEMP, imgNum);
							const std::u16string tmpPath = encode.tempPath + u"_" + CloakEngine::Helper::StringConvert::ConvertToU16(std::to_string(imgNum));
							write->SetTarget(tmpPath, g_TempFileType, CloakEngine::Files::CompressType::NONE);
							Engine::TempHandler::WriteGameID(write, encode.targetGameID);
							for (size_t a = 0; a < 4; a++) { write->WriteBits(6, bitInfos.RGBA.Channel[a].Bits - 1); }
							for (size_t a = 0; a < 4; a++) { write->WriteBits(6, bitInfos.YCCA.Channel[a].Bits - 1); }
							for (size_t a = 0; a < 4; a++) { write->WriteBits(6, bitInfos.Material.Channel[a].Bits - 1); }
							write->WriteBits(3, static_cast<uint8_t>(API::Image::ImageInfoType::LIST));
							write->WriteBits(2, static_cast<uint8_t>(m_heap));
							write->WriteBits(2, m_qualityOffset);
							write->WriteDynamic(m_texCount);
							for (size_t a = 0; a < m_texCount; a++)
							{
								std::vector<Engine::TempHandler::FileDateInfo> dates;
								Engine::TempHandler::ReadDates(m_metas[a].Builder.CheckPaths, &dates);
								Engine::TempHandler::WriteDates(dates, write);
								write->WriteBits(8, m_metas[a].Builder.BasicFlags);
								write->WriteDynamic(m_metas[a].Encoding.BlockSize);
								write->WriteBits(3, static_cast<uint8_t>(m_metas[a].Encoding.Space));
								write->WriteBits(10, m_textures[a].levelCount);
							}
							write->WriteBits(1, m_alphaMap ? 1 : 0);
							write->WriteBits(2, static_cast<uint8_t>(m_sizeFlag));
							write->WriteBits(4, static_cast<uint8_t>(m_channel));

							for (size_t a = 0; a < m_layerCount; a++)
							{
								write->WriteDynamic(m_layers[a].ByteLength);
								write->WriteDynamic(m_layers[a].BitLength);
								write->WriteBuffer(m_layers[a].Buffer, m_layers[a].ByteLength, m_layers[a].BitLength);
							}

							SAVE_RELEASE(write);
						}
					}
					void CLOAK_CALL_THIS EntryList::Encode(In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
					{
						if (m_useTemp == false && m_err == false)
						{
							SendResponse(response, API::Image::v1008::RespondeState::START_IMG_HEADER, imgNum);
							//Encode Header:
							CloakEngine::Files::IWriter* write = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&write));
							write->SetTarget(m_layers[0].Buffer);

							write->WriteBits(3, 0); //List type
							write->WriteDynamic(m_texCount);
							write->WriteBits(2, m_qualityOffset);
							write->WriteBits(1, m_alphaMap ? 1 : 0);
							write->WriteBits(4, static_cast<uint8_t>(m_channel));
							if (EncodeHeap(write, m_heap) == false) { SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "Unknown heap type"); m_err = true; }
							write->WriteBits(1, m_sizeFlag == API::Image::SizeFlag::DYNAMIC ? 1 : 0);
							UINT lW = 0;
							UINT lH = 0;
							bool* hr = NewArray(bool, m_texCount);
							for (size_t a = 0; a < m_texCount; a++)
							{
								hr[a] = CheckHR(m_textures[a], m_channel);
								if (EncodeMeta(write, m_metas[a], hr[a]) == false) { SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "Unknown color space"); m_err = true; }
								if (a == 0 || m_sizeFlag == API::Image::SizeFlag::DYNAMIC)
								{
									lW = m_textures[a].levels[0].Size.W;
									lH = m_textures[a].levels[0].Size.H;
									write->WriteBits(10, m_textures[a].levelCount);
									write->WriteDynamic(lW);
									write->WriteDynamic(lH);
								}
								else if (m_textures[a].levels[0].Size.W != lW || m_textures[a].levels[0].Size.H != lH)
								{
									SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Textures need to be the same size if SizeFlag is FIXED", a + 1);
									m_err = true;
								}
								if (m_alphaMap == true) { EncodeAlphaMap(write, m_textures[a].levels[0]); }
							}

							m_layers[0].BitLength = write->GetBitPosition();
							m_layers[0].ByteLength = write->GetPosition();
							write->Save();
							SendResponse(response, API::Image::v1008::RespondeState::DONE_IMG_HEADER, imgNum);

							SendResponse(response, API::Image::v1008::RespondeState::START_IMG_SUB, imgNum);
							//Encode layers:
							for (size_t a = 1; a < m_layerCount && m_err == false; a++)
							{
								write->SetTarget(m_layers[a].Buffer);

								size_t prev = m_texCount;
								for (size_t b = 0; b < m_texCount; b++)
								{
									if (m_textures[b].levelCount >= a)
									{
										MipMapLevel* prevTex = nullptr;
										MipMapLevel* prevMip = nullptr;
										if (prev < m_texCount) { prevTex = &m_textures[prev].levels[a - 1]; }
										if (m_textures[b].levelCount > a) { prevMip = &m_textures[b].levels[a]; }
										m_err = m_err || EncodeTexture(write, bitInfos, m_textures[b].levels[a - 1], m_metas[b], response, imgNum, b, m_channel, hr[b], prevTex, prevMip);
										prev = b;
									}
								}

								m_layers[a].BitLength = write->GetBitPosition();
								m_layers[a].ByteLength = write->GetPosition();
								write->Save();
							}
							SendResponse(response, API::Image::v1008::RespondeState::DONE_IMG_SUB, imgNum);
							SAVE_RELEASE(write);
							DeleteArray(hr);
						}
					}
					void CLOAK_CALL_THIS EntryList::WriteHeader(In const CE::RefPointer<CloakEngine::Files::IWriter>& write)
					{
						if (m_err == false)
						{
							write->WriteBuffer(m_layers[0].Buffer, m_layers[0].ByteLength, m_layers[0].BitLength);
						}
					}
					size_t CLOAK_CALL_THIS EntryList::GetMaxLayerLength()
					{
						if (m_err == false) { return m_layerCount - (1 + m_qualityOffset); }
						return 0;
					}
					void CLOAK_CALL_THIS EntryList::WriteLayer(In const CE::RefPointer<CloakEngine::Files::IWriter>& write, In size_t layer)
					{
						if (m_err == false)
						{
							if (layer + m_qualityOffset < 3 || layer + m_qualityOffset - 3 >= m_layerCount) { write->WriteBits(1, 0); }
							else
							{
								const size_t l = layer + m_qualityOffset - 3;
								write->WriteBits(1, 1);
								write->WriteBuffer(m_layers[l].Buffer, m_layers[l].ByteLength, m_layers[l].BitLength);
							}
						}
					}
					void CLOAK_CALL_THIS EntryList::WriteLibFiles(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const API::Image::LibInfo& lib, In const BitInfo& bits, In size_t entryID, In const Engine::Lib::CMakeFile& cmake, In const CE::Global::Task& finishTask)
					{
						const bool fitOneFile = Lib::FitsOneFile(m_texCount, m_textures, m_channel, bits, lib.WriteMipMaps);
						if (fitOneFile == false)
						{
							for (size_t a = 0; a < m_texCount; a++)
							{
								const std::string hName = "E" + std::to_string(entryID) + "T" + std::to_string(a);
								Engine::Lib::WriteWithTabs(file, 0, "#include \"" + hName + ".h\"");
								CE::Global::Task writeHeader = CE::Global::PushTask([hName, &cmake, &lib, entryID, a](In size_t threadID) {
									cmake.AddFile(hName + ".h");

									CE::RefPointer<CloakEngine::Files::IWriter> file = Engine::Lib::CreateSourceFile(lib.Path, hName, "H");
									Engine::Lib::WriteHeaderIntro(file, Engine::Lib::Type::Image, lib.Name + "_" + hName, lib.Path.GetHash());
									Engine::Lib::WriteWithTabs(file, 0, "#include \"Common.h\"");
									Engine::Lib::WriteWithTabs(file, 0, "");
									size_t tabCount = Engine::Lib::WriteCommonIntro(file, Engine::Lib::Type::Image, lib.Name);
									Engine::Lib::WriteWithTabs(file, tabCount, "bool Get" + std::to_string(entryID) + "T" + std::to_string(a) + "(Image* result);");
									tabCount = Engine::Lib::WriteCommonOutro(file, tabCount);
									Engine::Lib::WriteHeaderOutro(file, 0);
									file->Save();
								});
								CE::Global::Task writeFile = CE::Global::PushTask([hName, &cmake, &lib, entryID, a, this, &bits](In size_t threadID) {
									cmake.AddFile(hName + ".cpp");

									CE::RefPointer<CloakEngine::Files::IWriter> file = Engine::Lib::CreateSourceFile(lib.Path, hName, "CPP");
									Engine::Lib::WriteCopyright(file);
									Engine::Lib::WriteWithTabs(file, 0, "#include \"" + hName + ".h\"");
									Engine::Lib::WriteWithTabs(file, 0, "");
									size_t tabCount = Engine::Lib::WriteCommonIntro(file, Engine::Lib::Type::Image, lib.Name);
									Lib::EncodeTexture(file, m_textures[a], m_channel, bits, lib.WriteMipMaps, entryID, a, tabCount);
									Engine::Lib::WriteWithTabs(file, tabCount, "");
									Engine::Lib::WriteWithTabs(file, tabCount, "bool Get" + std::to_string(entryID) + "T" + std::to_string(a) + "(Image* result)");
									Engine::Lib::WriteWithTabs(file, tabCount, "{");
									Engine::Lib::WriteWithTabs(file, tabCount + 1, "*result = E" + std::to_string(entryID) + "T" + std::to_string(a) + ";");
									Engine::Lib::WriteWithTabs(file, tabCount + 1, "return true;");
									Engine::Lib::WriteWithTabs(file, tabCount, "}");
									tabCount = Engine::Lib::WriteCommonOutro(file, tabCount);
									file->Save();
								});
								finishTask.AddDependency(writeHeader);
								finishTask.AddDependency(writeFile);
								writeHeader.Schedule(CE::Global::Threading::Flag::IO);
								writeFile.Schedule(CE::Global::Threading::Flag::IO);
							}
							Engine::Lib::WriteWithTabs(file, 0, "");
						}
						size_t tabCount = Engine::Lib::WriteCommonIntro(file, Engine::Lib::Type::Image, lib.Name);
						if (fitOneFile == true)
						{
							for (size_t a = 0; a < m_texCount; a++)
							{
								Lib::EncodeTexture(file, m_textures[a], m_channel, bits, lib.WriteMipMaps, entryID, a, tabCount);
							}
							Engine::Lib::WriteWithTabs(file, tabCount, "");
						}
						Engine::Lib::WriteWithTabs(file, tabCount, "bool Get" + std::to_string(entryID) + "(std::uint32_t imageID, Image* result)");
						Engine::Lib::WriteWithTabs(file, tabCount, "{");
						Engine::Lib::WriteWithTabs(file, tabCount + 1, "switch(imageID)");
						Engine::Lib::WriteWithTabs(file, tabCount + 1, "{");
						for (size_t a = 0; a < m_texCount; a++)
						{
							if (m_textures[a].levelCount > 0)
							{
								if (fitOneFile == false)
								{
									Engine::Lib::WriteWithTabs(file, tabCount + 2, "case " + std::to_string(a) + ": return Get" + std::to_string(entryID) + "T" + std::to_string(a) + "(result);");
								}
								else
								{
									Engine::Lib::WriteWithTabs(file, tabCount + 2, "case " + std::to_string(a) + ": *result = E" + std::to_string(entryID) + "T" + std::to_string(a) + "; return true;");
								}
							}
						}
						Engine::Lib::WriteWithTabs(file, tabCount + 2, "default: break;");
						Engine::Lib::WriteWithTabs(file, tabCount + 1, "}");
						Engine::Lib::WriteWithTabs(file, tabCount + 1, "return false;");
						Engine::Lib::WriteWithTabs(file, tabCount, "}");
						Engine::Lib::WriteCommonOutro(file, tabCount);
					}
				}
			}
		}
	}
}