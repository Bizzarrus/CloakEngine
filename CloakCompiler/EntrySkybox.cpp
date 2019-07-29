#include "stdafx.h"
#include "Engine/Image/EntrySkybox.h"
#include "Engine/Image/MipMapGeneration.h"
#include "Engine/Image/TextureEncoding.h"
#include "Engine/TempHandler.h"

#include "imageprocessing.h"

#ifdef _DEBUG
#include <sstream>
#endif

//See https://dirkiek.wordpress.com/2015\05/31/physically-based-rendering-and-image-based-lighting/ for IBL
//	-> See https://graphics.stanford.edu/papers/envmap/envmap.pdf for irradiance
//	-> See https://learnopengl.com/PBR/IBL/Specular-IBL for radiance map

#define V1008_FUNC_IR(name) float name(In const CloakEngine::Global::Math::Point& dir)

//#define PRINT_IMAGES

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			namespace v1008 {
				namespace EntrySkybox_v1 {
					constexpr size_t RADIANCE_LUT_IMG_ID = 6;
					constexpr size_t RADIANCE_COL_IMG_ID = 7;
					constexpr CloakEngine::Files::FileType g_TempFileType{ "ImageTemp","CEIT",1008 };

					inline CloakEngine::Global::Math::Vector CLOAK_CALL CubePixelToDirection(In size_t side, In float u, In float v)
					{
						switch (side)
						{
							case Engine::Image::Cube::SIDE_RIGHT:
								return CloakEngine::Global::Math::Vector(1, -v, -u).Normalize();
							case Engine::Image::Cube::SIDE_LEFT:
								return CloakEngine::Global::Math::Vector(-1, -v, u).Normalize();
							case Engine::Image::Cube::SIDE_TOP:
								return CloakEngine::Global::Math::Vector(u, 1, v).Normalize();
							case Engine::Image::Cube::SIDE_BOTTOM:
								return CloakEngine::Global::Math::Vector(u, -1, -v).Normalize();
							case Engine::Image::Cube::SIDE_FRONT:
								return CloakEngine::Global::Math::Vector(u, -v, 1).Normalize();
							case Engine::Image::Cube::SIDE_BACK:
								return CloakEngine::Global::Math::Vector(-u, -v, -1).Normalize();
							default:
								return CloakEngine::Global::Math::Vector(0, 0, 0);
						}
					}
					inline CloakEngine::Global::Math::Vector CLOAK_CALL CubePixelToDirection(In size_t side, In UINT x, In UINT y, In UINT cubeWidth, In UINT cubeHeight)
					{
						const float u = ((static_cast<float>(x << 1) + 1) / cubeWidth) - 1;
						const float v = ((static_cast<float>(y << 1) + 1) / cubeHeight) - 1;
						return CubePixelToDirection(side, u, v);
					}
					inline CloakEngine::Global::Math::Vector CLOAK_CALL CubePixelToDirection(In size_t side, In UINT x, In UINT y, In UINT cubeSize) { return CubePixelToDirection(side, x, y, cubeSize, cubeSize); }
					inline CloakEngine::Global::Math::Vector CLOAK_CALL CubePixelToDirection(In size_t side, In UINT x, In UINT y, In const Engine::TextureLoader::Image sides[6]) { return CubePixelToDirection(side, x, y, sides[side].width, sides[side].height); }
					inline CloakEngine::Global::Math::Vector CLOAK_CALL CubePixelToDirection(In size_t side, In UINT x, In UINT y, In const Dimensions& size) { return CubePixelToDirection(side, x, y, size.W, size.H); }
					
					namespace Irradiance {
						constexpr float MC[] = { 0.429043f, 0.511664f, 0.743125f, 0.886227f, 0.247708f };

						typedef V1008_FUNC_IR((*WeightFunction));
						V1008_FUNC_IR(Y00) { return 0.282095f; }
						V1008_FUNC_IR(Y12) { return 0.488603f*dir.X; }
						V1008_FUNC_IR(Y11) { return 0.488603f*dir.Z; }
						V1008_FUNC_IR(Y10) { return 0.488603f*dir.Y; }
						V1008_FUNC_IR(Y24) { return 0.546274f*((dir.X*dir.X) - dir.Y); }
						V1008_FUNC_IR(Y23) { return 1.092548f*dir.X*dir.Z; }
						V1008_FUNC_IR(Y22) { return 0.315392f*((3 * dir.Z*dir.Z) - 1); }
						V1008_FUNC_IR(Y21) { return 1.092548f*dir.Y*dir.Z; }
						V1008_FUNC_IR(Y20) { return 1.092548f*dir.X*dir.Y; }

						constexpr WeightFunction Y[] = { Y00, Y10, Y11, Y12, Y20, Y21, Y22, Y23, Y24 };

						//Irradiance is calculated on CPU, while the GPU calculates the radiance, so both have something to do
						inline void CLOAK_CALL Encode(In CloakEngine::Files::IWriter* write, In const MipTexture sides[6], In API::Image::v1008::ColorChannel channels)
						{
							//Multiple instances of L (namely ltx and lty) are used to reduce rounding errors during calculation

							//Calculated irradiance coefficients (9 per color channel)
							long double L[3][ARRAYSIZE(Y)];
							for (size_t a = 0; a < ARRAYSIZE(L); a++)
							{
								for (size_t b = 0; b < ARRAYSIZE(Y); b++) { L[a][b] = 0; }
							}
							for (size_t side = 0; side < 6; side++)
							{
								long double lty[ARRAYSIZE(L)][ARRAYSIZE(Y)];
								for (size_t a = 0; a < ARRAYSIZE(lty); a++)
								{
									for (size_t b = 0; b < ARRAYSIZE(Y); b++) { lty[a][b] = 0; }
								}
								for (UINT y = 0; y < sides[side].levels[0].Size.H; y++)
								{
									long double ltx[ARRAYSIZE(L)][ARRAYSIZE(Y)];
									for (size_t a = 0; a < ARRAYSIZE(ltx); a++)
									{
										for (size_t b = 0; b < ARRAYSIZE(Y); b++) { ltx[a][b] = 0; }
									}
									for (UINT x = 0; x < sides[side].levels[0].Size.W; x++)
									{
										CloakEngine::Global::Math::Point dir = static_cast<CloakEngine::Global::Math::Point>(CubePixelToDirection(side, x, y, sides[side].levels[0].Size).Normalize());
										float c[ARRAYSIZE(Y)];
										for (size_t a = 0; a < ARRAYSIZE(Y); a++) { c[a] = Y[a](dir); }
										CloakEngine::Helper::Color::RGBA e = sides[side].levels[0].Get(x, y);
										for (size_t a = 0; a < 3; a++) { e[a] *= e.A; }//Alpha normalization
										for (size_t a = 0; a < 3; a++) { e[a] = std::pow(max(0, e[a]), 2.2f); }//To Linear color
										const long double sf = std::sin(std::acos(static_cast<long double>(dir.Z)));
										for (size_t a = 0; a < ARRAYSIZE(ltx); a++)
										{
											for (size_t b = 0; b < ARRAYSIZE(Y); b++)
											{
												ltx[a][b] += sf * c[b] * e[a];
												CLOAK_ASSUME(ltx[a][b] == ltx[a][b]); //check for NaN
												CLOAK_ASSUME(std::abs(ltx[a][b]) != std::numeric_limits<float>::infinity());
											}
										}
									}
									for (size_t a = 0; a < ARRAYSIZE(lty); a++)
									{
										for (size_t b = 0; b < ARRAYSIZE(Y); b++) { lty[a][b] += ltx[a][b] / sides[side].levels[0].Size.W; }
									}
								}
								for (size_t a = 0; a < ARRAYSIZE(L); a++)
								{
									for (size_t b = 0; b < ARRAYSIZE(Y); b++) { L[a][b] += lty[a][b] / sides[side].levels[0].Size.H; }
								}
							}
							for (size_t a = 0; a < ARRAYSIZE(L); a++)
							{
								if ((channels & static_cast<API::Image::v1008::ColorChannel>(1 << a)) != API::Image::v1008::ColorChannel::Channel_None)
								{
									write->WriteDouble(32, MC[0] * L[a][8]); //a0
									write->WriteDouble(32, MC[0] * L[a][4]); //a1
									write->WriteDouble(32, MC[0] * L[a][7]); //a2
									write->WriteDouble(32, MC[1] * L[a][3]); //a3
									write->WriteDouble(32, MC[0] * L[a][5]); //a4
									write->WriteDouble(32, MC[1] * L[a][1]); //a5
									write->WriteDouble(32, MC[2] * L[a][6]); //a6
									write->WriteDouble(32, MC[1] * L[a][2]); //a7
									write->WriteDouble(32, (MC[3] * L[a][0]) - (MC[4] * L[a][6])); //a8

									/* Shader:
									Calculate Diffuse Color with C = c(p) * E(n)
										with	c(p) = Albedo color
												E(n) = dot(mul(n, M), n)
													with	n = normal vector
															M = Matrix:
																a0	|	a1	|	a2	|	a3
																a1	|	-a0	|	a4	|	a5
																a2	|	a4	|	a6	|	a7
																a3	|	a5	|	a7	|	a8
									
									*/
								}
							}
						}
					}

					CLOAK_CALL EntrySkybox::EntrySkybox(In const API::Image::v1008::IMAGE_INFO_SKYBOX& img, In const ResponseCaller& response, In size_t imgNum, In uint8_t qualityOffset, In API::Image::v1008::Heap heap)
					{
						m_channel = img.EncodingChannels;
						m_heap = heap;
						m_qualityOffset = qualityOffset;
						m_err = false;
						m_useTemp = false;
						m_radianceMapSize = img.IBL.Radiance.Color.CubeSize;
						m_cSamples = img.IBL.Radiance.LUT.CosNVSamples;
						m_rSamples = img.IBL.Radiance.LUT.RoughnessSamples;
						m_radianceSamples = img.IBL.Radiance.Color.NumSamples;
						m_LUTSamples = img.IBL.Radiance.LUT.NumSamples;
						m_LUTBlockSize = img.IBL.Radiance.LUT.BlockSize;
						m_radianceBlockSize = img.IBL.Radiance.Color.BlockSize;
						m_radianceMipCount = img.IBL.Radiance.Color.MaxMipMapCount;

						if (m_cSamples == 0) { SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "CosNVSamples can't be zero!"); m_err = true; }
						if (m_rSamples == 0) { SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "RoughnessSamples can't be zero!"); m_err = true; }
						//Load basic Cube Map:
						switch (img.Type)
						{
							case API::Image::v1008::CubeType::CUBE:
							{
								const API::Image::v1008::BuildInfo* Images[6];
								Images[Engine::Image::Cube::CUBE_ORDER[0]] = &img.Cube.Front;
								Images[Engine::Image::Cube::CUBE_ORDER[1]] = &img.Cube.Back;
								Images[Engine::Image::Cube::CUBE_ORDER[2]] = &img.Cube.Left;
								Images[Engine::Image::Cube::CUBE_ORDER[3]] = &img.Cube.Right;
								Images[Engine::Image::Cube::CUBE_ORDER[4]] = &img.Cube.Top;
								Images[Engine::Image::Cube::CUBE_ORDER[5]] = &img.Cube.Bottom;

								Engine::TextureLoader::Image i;
								for (size_t a = 0; a < 6 && m_err == false; a++)
								{
									HRESULT hRet = Engine::ImageBuilder::BuildImage(*Images[a], &i, &m_metas[a].Builder);
									m_metas[a].Encoding = Images[a]->Meta;
									if (FAILED(hRet)) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_BUILDING, imgNum, "Couldn't build image", a + 1); m_err = true; }
									else
									{
										CalculateMipMaps(img.MaxMipMapCount, qualityOffset, i, Images[a]->Meta, &m_textures[a]);
										if (m_textures[a].levelCount <= qualityOffset) { SendResponse(response, API::Image::v1008::RespondeState::ERR_QUALITY, imgNum, "Image is not large enought to support this quality setting", a + 1); m_err = true; }
										m_layerCount = max(m_layerCount, m_textures[a].levelCount);
									}
								}
								break;
							}
							case API::Image::v1008::CubeType::HDRI:
							{
								Engine::TextureLoader::Image i;
								HRESULT hRet = Engine::ImageBuilder::BuildImage(img.HDRI.Image, &i, &m_metas[0].Builder);
								m_metas[0].Encoding = img.HDRI.Image.Meta;
								for (size_t a = 1; a < 6; a++) { m_metas[a] = m_metas[0]; }
								if (FAILED(hRet)) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_BUILDING, imgNum, "Couldn't build image", 1); m_err = true; }
								else 
								{
									Engine::TextureLoader::Image sides[6];
									SendResponse(response, API::Image::v1008::RespondeState::INFO, imgNum, "Start converting HDRI File to texture cube");
									m_err = Engine::Image::Cube::HDRIToCube(i, img.HDRI.CubeSize, sides);
									SendResponse(response, API::Image::v1008::RespondeState::INFO, imgNum, "Finished converting HDRI File to texture cube");
									for (size_t a = 0; a < 6 && m_err == false; a++)
									{
										CalculateMipMaps(img.MaxMipMapCount, qualityOffset, sides[a], img.HDRI.Image.Meta, &m_textures[a]);
										if (m_textures[a].levelCount <= qualityOffset) { SendResponse(response, API::Image::v1008::RespondeState::ERR_QUALITY, imgNum, "Image is not large enought to support this quality setting", a + 1); m_err = true; }
										m_layerCount = max(m_layerCount, m_textures[a].levelCount);
									}
								}
								break;
							}
							default:
								SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "Unknown type");
								m_err = true;
								break;
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
					}
					CLOAK_CALL EntrySkybox::~EntrySkybox()
					{
						DeleteArray(m_layers);
					}
					bool CLOAK_CALL_THIS EntrySkybox::GotError() const { return m_err; }
					void CLOAK_CALL_THIS EntrySkybox::CheckTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
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
							if (suc) { suc = (static_cast<API::Image::ImageInfoType>(read->ReadBits(3)) == API::Image::ImageInfoType::SKYBOX); }
							if (suc) { suc = (static_cast<API::Image::v1008::Heap>(read->ReadBits(2)) == m_heap); }
							if (suc) { suc = (static_cast<uint8_t>(read->ReadBits(2)) == m_qualityOffset); }
							for (size_t a = 0; a < 6 && suc; a++)
							{
								std::vector<Engine::TempHandler::FileDateInfo> dates;
								Engine::TempHandler::ReadDates(m_metas[a].Builder.CheckPaths, &dates);
								suc = !Engine::TempHandler::CheckDateChanges(dates, read);
								if (suc) { suc = (read->ReadBits(8) == m_metas[a].Builder.BasicFlags); }
								if (suc) { suc = (read->ReadDynamic() == m_metas[a].Encoding.BlockSize); }
								if (suc) { suc = (static_cast<API::Image::v1008::ColorSpace>(read->ReadBits(3)) == m_metas[a].Encoding.Space); }
								if (suc) { suc = (static_cast<uint32_t>(read->ReadBits(10)) == m_textures[a].levelCount); }
							}
							if (suc) { suc = (static_cast<API::Image::v1008::ColorChannel>(read->ReadBits(4)) == m_channel); }
							if (suc) { suc = (m_cSamples == read->ReadBits(32)); }
							if (suc) { suc = (m_rSamples == read->ReadBits(32)); }
							if (suc) { suc = (m_LUTSamples == read->ReadBits(32)); }
							if (suc) { suc = (m_radianceSamples == read->ReadBits(32)); }
							if (suc) { suc = (m_radianceMapSize == read->ReadBits(32)); }
							if (suc) { suc = (m_radianceBlockSize == read->ReadBits(16)); }
							if (suc) { suc = (m_LUTBlockSize == read->ReadBits(16)); }
							if (suc) { suc = (m_radianceMipCount == read->ReadBits(32)); }

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
					void CLOAK_CALL_THIS EntrySkybox::WriteTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
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
							write->WriteBits(3, static_cast<uint8_t>(API::Image::ImageInfoType::CUBE));
							write->WriteBits(2, static_cast<uint8_t>(m_heap));
							write->WriteBits(2, m_qualityOffset);
							for (size_t a = 0; a < 6; a++)
							{
								std::vector<Engine::TempHandler::FileDateInfo> dates;
								Engine::TempHandler::ReadDates(m_metas[a].Builder.CheckPaths, &dates);
								Engine::TempHandler::WriteDates(dates, write);
								write->WriteBits(8, m_metas[a].Builder.BasicFlags);
								write->WriteDynamic(m_metas[a].Encoding.BlockSize);
								write->WriteBits(3, static_cast<uint8_t>(m_metas[a].Encoding.Space));
								write->WriteBits(10, m_textures[a].levelCount);
							}
							write->WriteBits(4, static_cast<uint8_t>(m_channel));
							write->WriteBits(32, m_cSamples);
							write->WriteBits(32, m_rSamples);
							write->WriteBits(32, m_LUTSamples);
							write->WriteBits(32, m_radianceSamples);
							write->WriteBits(32, m_radianceMapSize);
							write->WriteBits(16, m_radianceBlockSize);
							write->WriteBits(16, m_LUTBlockSize);
							write->WriteBits(32, m_radianceMipCount);

							for (size_t a = 0; a < m_layerCount; a++)
							{
								write->WriteDynamic(m_layers[a].ByteLength);
								write->WriteDynamic(m_layers[a].BitLength);
								write->WriteBuffer(m_layers[a].Buffer, m_layers[a].ByteLength, m_layers[a].BitLength);
							}

							SAVE_RELEASE(write);
						}
					}
					void CLOAK_CALL_THIS EntrySkybox::Encode(In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
					{
						if (m_useTemp == false && m_err == false)
						{
							SendResponse(response, API::Image::v1008::RespondeState::START_IMG_HEADER, imgNum);
							//Encode Header:
							CloakEngine::Files::IWriter* write = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&write));
							write->SetTarget(m_layers[0].Buffer);

							write->WriteBits(3, 5); //Skybox type
							write->WriteBits(2, m_qualityOffset);
							write->WriteBits(4, static_cast<uint8_t>(m_channel));
							if (EncodeHeap(write, m_heap) == false) { SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "Unknown heap type"); m_err = true; }
							const UINT lW = m_textures[0].levels[0].Size.W;
							const UINT lH = m_textures[0].levels[0].Size.H;
							const size_t levelCount = m_textures[0].levelCount;
							write->WriteBits(10, levelCount);
							write->WriteDynamic(lW);
							if (lW != lH) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Cube images need to be quadratic"); m_err = true; }
							bool hr = false;
							for (size_t a = 0; a < 6 && hr == false; a++) { hr = CheckHR(m_textures[a], m_channel); }

							for (size_t a = 0; a < 6; a++)
							{
								if (EncodeMeta(write, m_metas[a], hr) == false) { SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "Unknown color space"); m_err = true; }
								if (m_textures[a].levels[0].Size.W != lW || m_textures[a].levels[0].Size.H != lH || m_textures[a].levelCount != levelCount)
								{
									SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Textures need to be the same size", a + 1);
									m_err = true;
								}
							}


							CloakEngine::Rendering::IManager* manager = nullptr;
							CloakEngine::Global::Graphic::GetManager(CE_QUERY_ARGS(&manager));
							CE::Rendering::Hardware hardware = manager->GetHardwareSetting();

							CloakEngine::Rendering::STATIC_SAMPLER_DESC sdr;
							sdr.AddressU = CloakEngine::Rendering::MODE_CLAMP;
							sdr.AddressV = CloakEngine::Rendering::MODE_CLAMP;
							sdr.AddressW = CloakEngine::Rendering::MODE_CLAMP;
							sdr.BorderColor = CloakEngine::Rendering::STATIC_BORDER_COLOR_OPAQUE_BLACK;
							sdr.CompareFunction = CloakEngine::Rendering::CMP_FUNC_ALWAYS;
							sdr.Filter = CloakEngine::Rendering::FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
							sdr.MaxAnisotropy = 0;
							sdr.MaxLOD = CloakEngine::Rendering::MAX_LOD;
							sdr.MinLOD = 0;
							sdr.MipLODBias = 0;
							sdr.RegisterID = 0;
							sdr.Space = 0;
							sdr.Visible = CloakEngine::Rendering::VISIBILITY_PIXEL;

							CloakEngine::Rendering::IPipelineState* psoRadiance1 = manager->CreatePipelineState();
							CloakEngine::Rendering::IPipelineState* psoRadiance2 = manager->CreatePipelineState();

							CloakEngine::Rendering::RootDescription rootRadiance1(2, 1);
							rootRadiance1.At(0).SetAsSRV(0, 1, CloakEngine::Rendering::VISIBILITY_PIXEL);
							rootRadiance1.At(1).SetAsConstants(0, 3, CloakEngine::Rendering::VISIBILITY_PIXEL);
							rootRadiance1.SetStaticSampler(0, sdr);

							CloakEngine::Rendering::RootDescription rootRadiance2(2, 1);
							rootRadiance2.At(0).SetAsSRV(0, 1, CloakEngine::Rendering::VISIBILITY_PIXEL);
							rootRadiance2.At(1).SetAsConstants(0, 1, CloakEngine::Rendering::VISIBILITY_PIXEL);
							rootRadiance2.SetStaticSampler(0, sdr);
							
							CloakEngine::Rendering::PIPELINE_STATE_DESC pdr;
							{
								auto& blend = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_BLEND>();
								blend.AlphaToCoverage = false;
								blend.IndependentBlend = false;
								for (size_t a = 0; a < CloakEngine::Rendering::MAX_RENDER_TARGET; a++)
								{
									blend.RenderTarget[a].BlendEnable = false;
									blend.RenderTarget[a].BlendOperation = CloakEngine::Rendering::BLEND_OP_ADD;
									blend.RenderTarget[a].BlendOperationAlpha = CloakEngine::Rendering::BLEND_OP_ADD;
									blend.RenderTarget[a].DstBlend = CloakEngine::Rendering::BLEND_ZERO;
									blend.RenderTarget[a].DstBlendAlpha = CloakEngine::Rendering::BLEND_ZERO;
									blend.RenderTarget[a].SrcBlend = CloakEngine::Rendering::BLEND_ONE;
									blend.RenderTarget[a].SrcBlendAlpha = CloakEngine::Rendering::BLEND_ONE;
									blend.RenderTarget[a].LogicOperation = CloakEngine::Rendering::LOGIC_OP_AND;
									blend.RenderTarget[a].LogicOperationEnable = false;
									blend.RenderTarget[a].WriteMask = CloakEngine::Rendering::COLOR_ALL;
								}

								auto& depthStencil = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_DEPTH_STENCIL>();
								depthStencil.BackFace.DepthFailOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.BackFace.PassOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.BackFace.StencilFailOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.BackFace.StencilFunction = CloakEngine::Rendering::CMP_FUNC_ALWAYS;
								depthStencil.FrontFace.DepthFailOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.FrontFace.PassOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.FrontFace.StencilFailOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.FrontFace.StencilFunction = CloakEngine::Rendering::CMP_FUNC_ALWAYS;
								depthStencil.DepthEnable = false;
								depthStencil.DepthFunction = CloakEngine::Rendering::CMP_FUNC_ALWAYS;
								depthStencil.DepthWriteMask = CloakEngine::Rendering::WRITE_MASK_ALL;
								depthStencil.StencilEnable = false;
								depthStencil.StencilReadMask = 0;
								depthStencil.StencilWriteMask = 0;
								depthStencil.DepthBoundsTests = false;
								depthStencil.Format = CloakEngine::Rendering::Format::D24_UNORM_S8_UINT;

								auto& renderTargets = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_RENDER_TARGETS>();
								renderTargets.NumRenderTargets = 6;
								for (size_t a = 0; a < CloakEngine::Rendering::MAX_RENDER_TARGET; a++) { renderTargets.RTVFormat[a] = CloakEngine::Rendering::Format::R16G16B16A16_FLOAT; }

								auto& rasterizer = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_RASTERIZER>();
								rasterizer.AntialiasedLine = false;
								rasterizer.ConservativeRasterization = CloakEngine::Rendering::CONSERVATIVE_RASTERIZATION_OFF;
								rasterizer.CullMode = CloakEngine::Rendering::CULL_BACK;
								rasterizer.DepthBias = 0;
								rasterizer.DepthBiasClamp = 0;
								rasterizer.DepthClip = true;
								rasterizer.FillMode = CloakEngine::Rendering::FILL_SOLID;
								rasterizer.ForcedSampleCount = 0;
								rasterizer.FrontCounterClockwise = false;
								rasterizer.Multisample = false;
								rasterizer.SlopeScaledDepthBias = 0;

								auto& sample = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_SAMPLE_DESC>();
								sample.Count = 1;
								sample.Quality = 0;

								auto& stream = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_STREAM_OUTPUT>();
								stream.BufferStrides = nullptr;
								stream.Entries = nullptr;
								stream.NumEntries = 0;
								stream.NumStrides = 0;
								stream.RasterizedStreamIndex = 0;

								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_SAMPLE_MASK>() = CloakEngine::Rendering::DEFAULT_SAMPLE_MASK;
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT>() = CloakEngine::Rendering::INDEX_BUFFER_STRIP_CUT_DISABLED;
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_INPUT_LAYOUT>().Type = CloakEngine::Rendering::INPUT_LAYOUT_TYPE_NONE;
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>() = CloakEngine::Rendering::TOPOLOGY_TYPE_TRIANGLE;

								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_VS>() = ImageProcessing::RadianceFirstSum::GetVS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_PS>() = ImageProcessing::RadianceFirstSum::GetPS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_HS>() = ImageProcessing::RadianceFirstSum::GetHS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_DS>() = ImageProcessing::RadianceFirstSum::GetDS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_GS>() = ImageProcessing::RadianceFirstSum::GetGS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_CS>() = ImageProcessing::RadianceFirstSum::GetCS(hardware);
							}

							if (m_err == false && psoRadiance1->Reset(pdr, rootRadiance1) == false)
							{
								SendResponse(response, API::Image::RespondeState::ERR_GPU_INIT, imgNum, "PSO 'Skybox first sum' failed to initialize");
								m_err = true;
							}

							pdr.Reset();
							{
								auto& blend = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_BLEND>();
								blend.AlphaToCoverage = false;
								blend.IndependentBlend = false;
								for (size_t a = 0; a < CloakEngine::Rendering::MAX_RENDER_TARGET; a++)
								{
									blend.RenderTarget[a].BlendEnable = false;
									blend.RenderTarget[a].BlendOperation = CloakEngine::Rendering::BLEND_OP_ADD;
									blend.RenderTarget[a].BlendOperationAlpha = CloakEngine::Rendering::BLEND_OP_ADD;
									blend.RenderTarget[a].DstBlend = CloakEngine::Rendering::BLEND_ZERO;
									blend.RenderTarget[a].DstBlendAlpha = CloakEngine::Rendering::BLEND_ZERO;
									blend.RenderTarget[a].SrcBlend = CloakEngine::Rendering::BLEND_ONE;
									blend.RenderTarget[a].SrcBlendAlpha = CloakEngine::Rendering::BLEND_ONE;
									blend.RenderTarget[a].LogicOperation = CloakEngine::Rendering::LOGIC_OP_AND;
									blend.RenderTarget[a].LogicOperationEnable = false;
									blend.RenderTarget[a].WriteMask = CloakEngine::Rendering::COLOR_ALL;
								}

								auto& depthStencil = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_DEPTH_STENCIL>();
								depthStencil.BackFace.DepthFailOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.BackFace.PassOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.BackFace.StencilFailOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.BackFace.StencilFunction = CloakEngine::Rendering::CMP_FUNC_ALWAYS;
								depthStencil.FrontFace.DepthFailOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.FrontFace.PassOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.FrontFace.StencilFailOperation = CloakEngine::Rendering::STENCIL_OP_KEEP;
								depthStencil.FrontFace.StencilFunction = CloakEngine::Rendering::CMP_FUNC_ALWAYS;
								depthStencil.DepthEnable = false;
								depthStencil.DepthFunction = CloakEngine::Rendering::CMP_FUNC_ALWAYS;
								depthStencil.DepthWriteMask = CloakEngine::Rendering::WRITE_MASK_ALL;
								depthStencil.StencilEnable = false;
								depthStencil.StencilReadMask = 0;
								depthStencil.StencilWriteMask = 0;
								depthStencil.DepthBoundsTests = false;
								depthStencil.Format = CloakEngine::Rendering::Format::D24_UNORM_S8_UINT;

								auto& renderTargets = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_RENDER_TARGETS>();
								renderTargets.NumRenderTargets = 1;
								for (size_t a = 0; a < CloakEngine::Rendering::MAX_RENDER_TARGET; a++) { renderTargets.RTVFormat[a] = CloakEngine::Rendering::Format::R16G16_FLOAT; }

								auto& rasterizer = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_RASTERIZER>();
								rasterizer.AntialiasedLine = false;
								rasterizer.ConservativeRasterization = CloakEngine::Rendering::CONSERVATIVE_RASTERIZATION_OFF;
								rasterizer.CullMode = CloakEngine::Rendering::CULL_BACK;
								rasterizer.DepthBias = 0;
								rasterizer.DepthBiasClamp = 0;
								rasterizer.DepthClip = true;
								rasterizer.FillMode = CloakEngine::Rendering::FILL_SOLID;
								rasterizer.ForcedSampleCount = 0;
								rasterizer.FrontCounterClockwise = false;
								rasterizer.Multisample = false;
								rasterizer.SlopeScaledDepthBias = 0;

								auto& sample = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_SAMPLE_DESC>();
								sample.Count = 1;
								sample.Quality = 0;

								auto& stream = pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_STREAM_OUTPUT>();
								stream.BufferStrides = nullptr;
								stream.Entries = nullptr;
								stream.NumEntries = 0;
								stream.NumStrides = 0;
								stream.RasterizedStreamIndex = 0;

								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT>() = CloakEngine::Rendering::INDEX_BUFFER_STRIP_CUT_DISABLED;
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_INPUT_LAYOUT>().Type = CloakEngine::Rendering::INPUT_LAYOUT_TYPE_NONE;
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>() = CloakEngine::Rendering::TOPOLOGY_TYPE_TRIANGLE;
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_SAMPLE_MASK>() = CloakEngine::Rendering::DEFAULT_SAMPLE_MASK;
							
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_VS>() = ImageProcessing::RadianceSecondSum::GetVS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_PS>() = ImageProcessing::RadianceSecondSum::GetPS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_HS>() = ImageProcessing::RadianceSecondSum::GetHS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_DS>() = ImageProcessing::RadianceSecondSum::GetDS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_GS>() = ImageProcessing::RadianceSecondSum::GetGS(hardware);
								pdr.Get<CloakEngine::Rendering::PSO_SUBTYPE_CS>() = ImageProcessing::RadianceSecondSum::GetCS(hardware);
							}

							if (m_err == false && psoRadiance2->Reset(pdr, rootRadiance2) == false)
							{
								SendResponse(response, API::Image::RespondeState::ERR_GPU_INIT, imgNum, "PSO 'Skybox second sum' failed to initialize");
								m_err = true;
							}

							CloakEngine::Rendering::TEXTURE_DESC td;
							td.AccessType = CloakEngine::Rendering::VIEW_TYPE::CUBE | CloakEngine::Rendering::VIEW_TYPE::SRV;
							td.clearValue = CloakEngine::Helper::Color::Black;
							td.Format = CloakEngine::Rendering::Format::R16G16B16A16_FLOAT;
							td.Height = m_textures[0].levels[0].Size.H;
							td.Width = m_textures[0].levels[0].Size.W;
							td.Type = CloakEngine::Rendering::TEXTURE_TYPE::TEXTURE_CUBE;
							td.TextureCube.MipMaps = static_cast<uint32_t>(m_textures[0].levelCount);

							CloakEngine::Helper::Color::RGBA** texData = NewArray(CloakEngine::Helper::Color::RGBA*, 6 * td.TextureCube.MipMaps);
							for (size_t a = 0; a < 6; a++) 
							{
								for (size_t b = 0; b < td.TextureCube.MipMaps; b++)
								{
									const size_t p = (a*td.TextureCube.MipMaps) + b;
									texData[p] = m_textures[a].levels[b].Color;
								}
							}

							CloakEngine::Rendering::IColorBuffer* tex = manager->CreateColorBuffer(CloakEngine::Rendering::CONTEXT_TYPE_GRAPHIC, 1, td, texData);
							manager->GenerateViews(&tex);
							DeleteArray(texData);

							MipTexture col[6];
							API::Image::BuildMeta meta;
							meta.BlockSize = m_radianceBlockSize;
							meta.Space = API::Image::ColorSpace::RGBA;
							meta.Type = API::Image::BuildType::NORMAL;
							size_t mipCount = m_radianceMipCount;
							for (size_t a = 0; a < 6 && m_err == false; a++)
							{
								CreateMipMapLevels(m_radianceMipCount, 0, &col[a], m_radianceMapSize, m_radianceMapSize, meta);
								mipCount = min(mipCount, col[a].levelCount);
							}
							const size_t nrT = 1 + (6 * mipCount);
							CloakEngine::Rendering::IColorBuffer** res = NewArray(CloakEngine::Rendering::IColorBuffer*, nrT);
							if (m_err == false)
							{
								for (size_t a = 0; a < nrT; a++)
								{
									td.AccessType = CloakEngine::Rendering::VIEW_TYPE::RTV;
									td.clearValue = CloakEngine::Helper::Color::RGBA(0, 0, 0, 0);
									td.Format = a == 0 ? CloakEngine::Rendering::Format::R16G16_FLOAT : CloakEngine::Rendering::Format::R16G16B16A16_FLOAT;
									if (a == 0)
									{
										td.Width = m_cSamples;
										td.Height = m_rSamples;
									}
									else
									{
										const size_t mip = (a - 1) / 6;
										const size_t tid = (a - 1) % 6;
										td.Width = col[tid].levels[mip].Size.W;
										td.Height = col[tid].levels[mip].Size.H;
									}
									td.Type = CloakEngine::Rendering::TEXTURE_TYPE::TEXTURE;
									td.Texture.MipMaps = 1;
									td.Texture.SampleCount = 1;
									res[a] = manager->CreateColorBuffer(CloakEngine::Rendering::CONTEXT_TYPE_GRAPHIC, 1, td);
								}
								manager->GenerateViews(&res[0], 1);
								for (size_t a = 1; a < nrT; a += 6) { manager->GenerateViews(&res[a], 6); }

							
								CloakEngine::Rendering::IContext* con = nullptr;
								manager->BeginContext(CloakEngine::Rendering::CONTEXT_TYPE_GRAPHIC, 1, CE_QUERY_ARGS(&con));

								for (size_t a = 0; a < nrT; a++) { con->ClearColor(res[a]); }
								con->SetPipelineState(psoRadiance1);
								con->SetSRV(0, tex->GetSRV());
								con->SetPrimitiveTopology(CloakEngine::Rendering::PRIMITIVE_TOPOLOGY::TOPOLOGY_TRIANGLELIST);
								for (size_t a = 0; a < mipCount; a++)
								{
									con->SetConstants(1, mipCount == 1 ? 0.5f : (static_cast<float>(static_cast<long double>(a) / (mipCount - 1))), static_cast<UINT>(m_radianceSamples), static_cast<float>(4 * CloakEngine::Global::Math::PI / (6 * m_textures[0].levels[0].Size.W * m_textures[0].levels[0].Size.W)));
									con->SetRenderTargets(6, &res[(a * 6) + 1]);
									con->SetViewportAndScissor(0, 0, static_cast<float>(col[0].levels[a].Size.W), static_cast<float>(col[0].levels[a].Size.W));
									con->Draw(3);
								}
								con->SetPipelineState(psoRadiance2);
								con->SetSRV(0, tex->GetSRV());
								con->SetConstants(1, static_cast<UINT>(m_LUTSamples));
								con->SetRenderTargets(1, &res[0]);
								con->SetViewportAndScissor(0, 0, static_cast<float>(m_cSamples), static_cast<float>(m_rSamples));
								con->Draw(3);

								con->CloseAndExecute();
								SAVE_RELEASE(con);

								SendResponse(response, API::Image::RespondeState::WRITE_UPDATE_INFO, imgNum, "Start irradiance encoding...");
								Irradiance::Encode(write, m_textures, m_channel);
								SendResponse(response, API::Image::RespondeState::WRITE_UPDATE_INFO, imgNum, "Load radiance from GPU...");

								MipMapLevel LUT;
								LUT.Allocate(m_cSamples, m_rSamples);
								for (size_t a = 0; a < nrT; a++)
								{
									if (a == 0)
									{
										manager->ReadBuffer(LUT.Size.GetSize(), LUT.Color, res[a]);
									}
									else
									{
										const size_t mip = (a - 1) / 6;
										const size_t tid = (a - 1) % 6;
										manager->ReadBuffer(col[tid].levels[mip].Size.GetSize(), col[tid].levels[mip].Color, res[a]);
									}
								}
								SendResponse(response, API::Image::RespondeState::WRITE_UPDATE_INFO, imgNum, "Start radiance encoding...");
								
#if defined(_DEBUG) && defined(PRINT_IMAGES)
								std::function<void(const MipMapLevel&, const std::string&)> printTex = [](const MipMapLevel& tex, const std::string& prev) {
									std::stringstream s;
									s << prev << std::endl;
									s << "\tW: " << tex.Size.W << " H: " << tex.Size.H;
									for (UINT y = 0; y < tex.Size.H; y++)
									{
										s << std::endl << "\t\t";
										for (UINT x = 0; x < tex.Size.W; x++)
										{
											s << "[ ";
											for (size_t c = 0; c < 4; c++)
											{
												if (c != 0) { s << " | "; }
												float cv = tex.Get(x, y)[c];
												s << cv;
											}
											s << " ]";
										}
									}
									CloakDebugLog(s.str());
								};
								for (size_t a = 0; a < 6; a++)
								{
									for (size_t b = m_textures[a].levelCount; b > 0; b--)
									{
										MipMapLevel texLevel;
										texLevel.Allocate(m_textures[a].levels[b - 1].Size.W, m_textures[a].levels[b - 1].Size.H);
										manager->ReadBuffer(texLevel.Size.GetSize(), texLevel.Color, tex, tex->GetSubresourceIndex(b - 1, a));
										printTex(texLevel, "Source Image Side [" + std::to_string(a) + "][" + std::to_string(b - 1) + "]");
									}
								}
								//printTex(LUT, "LUT Texture:");
								for (size_t a = 0; a < ARRAYSIZE(col); a++)
								{
									for (size_t b = col[a].levelCount; b > 0; b--)
									{
										printTex(col[a].levels[b - 1], "Reflection Cube Side [" + std::to_string(a) + "][" + std::to_string(b - 1) + "]:");
									}
								}
#endif
								TextureMeta tm;
								tm.Builder.BasicFlags = 0;
								tm.Encoding.BlockSize = m_LUTBlockSize;
								tm.Encoding.Space = API::Image::ColorSpace::RGBA;
								tm.Encoding.Type = API::Image::BuildType::NORMAL;

								EncodeTexture(write, bitInfos, LUT, tm, response, RADIANCE_LUT_IMG_ID, 0, API::Image::ColorChannel::Channel_1 | API::Image::ColorChannel::Channel_2, false);

								write->WriteBits(10, mipCount);
								write->WriteDynamic(m_radianceMapSize);
								tm.Encoding.BlockSize = m_radianceBlockSize;
								for (size_t a = 0; a < ARRAYSIZE(col); a++)
								{
									for (size_t b = col[a].levelCount; b > 0; b--)
									{
										EncodeTexture(write, bitInfos, col[a].levels[b - 1], tm, response, RADIANCE_COL_IMG_ID + a, b, API::Image::ColorChannel::Channel_Color, hr, a == 0 ? nullptr : &col[a - 1].levels[b - 1], b == col[a].levelCount ? nullptr : &col[a].levels[b]);
									}
								}
							}
							
							for (size_t a = 0; a < nrT; a++) { SAVE_RELEASE(res[a]); }
							DeleteArray(res);
							SAVE_RELEASE(tex);
							SAVE_RELEASE(psoRadiance1);
							SAVE_RELEASE(psoRadiance2);
							SAVE_RELEASE(manager);

							m_layers[0].BitLength = write->GetBitPosition();
							m_layers[0].ByteLength = write->GetPosition();
							write->Save();
							SendResponse(response, API::Image::v1008::RespondeState::DONE_IMG_HEADER, imgNum);

							SendResponse(response, API::Image::v1008::RespondeState::START_IMG_SUB, imgNum);
							//Encode layers:
							for (size_t a = 1; a < m_layerCount && m_err == false; a++)
							{
								write->SetTarget(m_layers[a].Buffer);

								size_t prev = 6;
								for (size_t b = 0; b < 6 && m_err == false; b++)
								{
									MipMapLevel* prevTex = nullptr;
									MipMapLevel* prevMip = nullptr;
									if (prev < 6) { prevTex = &m_textures[prev].levels[a - 1]; }
									if (m_textures[b].levelCount > a) { prevMip = &m_textures[b].levels[a]; }
									m_err = m_err || EncodeTexture(write, bitInfos, m_textures[b].levels[a - 1], m_metas[b], response, imgNum, b, m_channel, hr, prevTex, prevMip);
									prev = b;
								}

								m_layers[a].BitLength = write->GetBitPosition();
								m_layers[a].ByteLength = write->GetPosition();
								write->Save();
							}
							SendResponse(response, API::Image::v1008::RespondeState::DONE_IMG_SUB, imgNum);
							SAVE_RELEASE(write);
						}
					}
					void CLOAK_CALL_THIS EntrySkybox::WriteHeader(In const CE::RefPointer<CloakEngine::Files::IWriter>& write)
					{
						if (m_err == false)
						{
							write->WriteBuffer(m_layers[0].Buffer, m_layers[0].ByteLength, m_layers[0].BitLength);
						}
					}
					size_t CLOAK_CALL_THIS EntrySkybox::GetMaxLayerLength()
					{
						if (m_err == false) { return m_layerCount - (1 + m_qualityOffset); }
						return 0;
					}
					void CLOAK_CALL_THIS EntrySkybox::WriteLayer(In const CE::RefPointer<CloakEngine::Files::IWriter>& write, In size_t layer)
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
					void CLOAK_CALL_THIS EntrySkybox::WriteLibFiles(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const API::Image::LibInfo& lib, In size_t entryID, In const Lib::CMakeFile& cmake, In const CE::Global::Task& finishTask)
					{

					}
				}
			}
		}
	}
}