#include "stdafx.h"
#include "Engine/Image/Core.h"

#define PROJECT_HDRI_CUBE
#define PROJECT_CUBE_HDRI

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			namespace v1008 {
				const CloakEngine::Global::Math::Matrix g_ToYCbCr(0.299f, 0.587f, 0.114f, 0.0f, -0.168736f, -0.331264f, 0.5f, 0.5f, 0.5f, -0.418688f, -0.081312f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);

				uint8_t CLOAK_CALL BitLen(In uint64_t val)
				{
					return val == 0 ? 0 : (CloakEngine::Global::Math::bitScanReverse(val) + 1);
				}

				CLOAK_CALL MipMapLevel::MipMapLevel()
				{
					Size.W = 0;
					Size.H = 0;
					Color = nullptr;
				}
				CLOAK_CALL MipMapLevel::MipMapLevel(In const MipMapLevel& m)
				{
					operator=(m);
				}
				CLOAK_CALL MipMapLevel::~MipMapLevel() { DeleteArray(Color); }

				void CLOAK_CALL_THIS MipMapLevel::Allocate(In UINT w, In UINT h)
				{
					if (Size.W != w || Size.H != h)
					{
						DeleteArray(Color);
						Size.W = w;
						Size.H = h;
						Color = NewArray(CloakEngine::Helper::Color::RGBA, Size.GetSize());
					}
				}
				void CLOAK_CALL_THIS MipMapLevel::Set(In UINT x, In UINT y, In const CloakEngine::Helper::Color::RGBA& c)
				{
					const size_t p = (y*Size.W) + x;
					Color[p] = c;
				}
				const CloakEngine::Helper::Color::RGBA& CLOAK_CALL_THIS MipMapLevel::Get(In UINT x, In UINT y) const
				{
					if (x >= Size.W) { x = Size.W - 1; }
					if (y >= Size.H) { y = Size.H - 1; }
					return operator[]((y*Size.W) + x);
				}
				CloakEngine::Helper::Color::RGBA& CLOAK_CALL_THIS MipMapLevel::Get(In UINT x, In UINT y)
				{
					if (x >= Size.W) { x = Size.W - 1; }
					if (y >= Size.H) { y = Size.H - 1; }
					return operator[]((y*Size.W) + x);
				}
				const CloakEngine::Helper::Color::RGBA& CLOAK_CALL_THIS MipMapLevel::operator[](In UINT p) const 
				{
					CLOAK_ASSUME(p < Size.GetSize());
					return Color[p]; 
				}
				CloakEngine::Helper::Color::RGBA& CLOAK_CALL_THIS MipMapLevel::operator[](In UINT p) 
				{ 
					CLOAK_ASSUME(p < Size.GetSize());
					return Color[p]; 
				}
				MipMapLevel& CLOAK_CALL_THIS MipMapLevel::operator=(In const MipMapLevel& m)
				{
					Meta = m.Meta;
					Allocate(m.Size.W, m.Size.H);
					for (UINT y = 0; y < m.Size.H; y++)
					{
						for (UINT x = 0; x < m.Size.W; x++)
						{
							Set(x, y, m.Get(x, y));
						}
					}
					return *this;
				}

				CLOAK_CALL MipTexture::~MipTexture() { DeleteArray(levels); }

				void CLOAK_CALL_THIS BitInfo::SingleInfo::operator=(In uint8_t bits)
				{
					Bits = bits;
					MaxVal = (static_cast<BitColorData>(1) << bits) - 1;
					Len = BitLen(Bits);
				}

				BitInfo& CLOAK_CALL_THIS BitInfo::operator=(In const BitInfo& bi)
				{
					for (size_t a = 0; a < 4; a++)
					{
						Channel[a].Bits = bi.Channel[a].Bits;
						Channel[a].Len = bi.Channel[a].Len;
						Channel[a].MaxVal = bi.Channel[a].MaxVal;
					}
					return *this;
				}

				BitColorData& CLOAK_CALL_THIS BitColor::operator[](In size_t s) { return Data[s]; }
				const BitColorData& CLOAK_CALL_THIS BitColor::operator[](In size_t s) const { return Data[s]; }

				CLOAK_CALL BitTexture::BitTexture(In const MipMapLevel& texture, In const BitInfo& bitInfo) : Size(texture.Size)
				{
					const size_t s = static_cast<size_t>(Size.W)*Size.H;
					Data = NewArray(BitColor, s);

					for (UINT y = 0; y < Size.H; y++)
					{
						for (UINT x = 0; x < Size.W; x++)
						{
							const UINT p = (y*Size.W) + x;
							for (size_t c = 0; c < 4; c++)
							{
								const long double oc = static_cast<long double>(texture[p][c]);
								Data[p][c] = static_cast<BitColorData>(min(1, max(0, oc))*bitInfo.Channel[c].MaxVal);
							}
						}
					}
				}
				CLOAK_CALL BitTexture::BitTexture(In const BitTexture& t) : Size(t.Size)
				{
					const size_t s = static_cast<size_t>(Size.W)*Size.H;
					Data = NewArray(BitColor, s);
					for (size_t a = 0; a < s; a++) { Data[a] = t.Data[a]; }
				}
				CLOAK_CALL BitTexture::~BitTexture() { DeleteArray(Data); }
				BitColor* CLOAK_CALL_THIS BitTexture::operator[](In UINT y)
				{
					const size_t p = static_cast<size_t>(y)*Size.W;
					return &Data[p];
				}
				BitColor* CLOAK_CALL_THIS BitTexture::at(In UINT y)
				{
					const size_t p = static_cast<size_t>(y)*Size.W;
					return &Data[p];
				}
				const BitColor* CLOAK_CALL_THIS BitTexture::operator[](In UINT y) const
				{
					const size_t p = static_cast<size_t>(y)*Size.W;
					return &Data[p];
				}
				const BitColor* CLOAK_CALL_THIS BitTexture::at(In UINT y) const
				{
					const size_t p = static_cast<size_t>(y)*Size.W;
					return &Data[p];
				}
				BitTexture& CLOAK_CALL_THIS BitTexture::operator=(In const BitTexture& t)
				{
					const size_t s = static_cast<size_t>(t.Size.W)*t.Size.H;
					if (Size.W != t.Size.W || Size.H != t.Size.H)
					{
						DeleteArray(Data);
						Data = NewArray(BitColor, s);
					}
					for (size_t a = 0; a < s; a++) { Data[a] = t.Data[a]; }
					return *this;
				}

				CLOAK_CALL LayerBuffer::~LayerBuffer()
				{
					SAVE_RELEASE(Buffer);
				}
				
				CLOAK_CALL ResponseCaller::ResponseCaller(In const API::Image::v1008::ResponseFunc& func) : m_func(func)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
				}
				void CLOAK_CALL_THIS ResponseCaller::Call(const API::Image::v1008::RespondeInfo& ri) const
				{
					if (m_func)
					{
						CE::Helper::Lock lock(m_sync);
						m_func(ri);
					}
				}

				void CLOAK_CALL SendResponse(In const ResponseCaller& response, In API::Image::v1008::RespondeState state, In_opt size_t imgNum, In_opt std::string msg, In_opt size_t imgSubNum)
				{
					API::Image::v1008::RespondeInfo r;
					r.ImgNum = imgNum;
					r.ImgSubNum = imgSubNum;
					r.State = state;
					r.Msg = msg;
					response.Call(r);
				}
				void CLOAK_CALL SendResponse(In const ResponseCaller& response, In API::Image::v1008::RespondeState state, In size_t imgNum, In size_t imgSubNum) { SendResponse(response, state, imgNum, "", imgSubNum); }
				void CLOAK_CALL SendResponse(In const ResponseCaller& response, In API::Image::v1008::RespondeState state, In const std::string& msg) { SendResponse(response, state, 0, msg, 0); }
				bool CLOAK_CALL EncodeColorSpace(In CloakEngine::Files::IWriter* write, In const API::Image::v1008::BuildMeta& meta)
				{
					if (meta.Space == API::Image::v1008::ColorSpace::RGBA) { write->WriteBits(3, 0); }
					else if (meta.Space == API::Image::v1008::ColorSpace::YCCA) { write->WriteBits(3, 1); }
					else if (meta.Space == API::Image::v1008::ColorSpace::Material) { write->WriteBits(3, 2); }
					else { return false; }
					return true;
				}
				bool CLOAK_CALL EncodeMeta(In CloakEngine::Files::IWriter* write, In const TextureMeta& meta, In bool hr)
				{
					bool r = EncodeColorSpace(write, meta.Encoding);
					write->WriteBits(8, meta.Builder.BasicFlags);
					write->WriteBits(1, hr ? 1 : 0);
					return r;
				}
				bool CLOAK_CALL EncodeHeap(In CloakEngine::Files::IWriter* write, In API::Image::v1008::Heap heap)
				{
					if (heap == API::Image::v1008::Heap::GUI) { write->WriteBits(2, 0); }
					else if (heap == API::Image::v1008::Heap::WORLD) { write->WriteBits(2, 1); }
					else if (heap == API::Image::v1008::Heap::UTILITY) { write->WriteBits(2, 2); }
					else { return false; }
					return true;
				}
				void CLOAK_CALL ConvertToYCCA(Inout MipMapLevel* tex)
				{
					CloakEngine::Global::Math::Vector ccv;
					CloakEngine::Global::Math::Point ccp;
					CloakEngine::Helper::Color::RGBA ccc;
					for (UINT y = 0; y < tex->Size.H; y++)
					{
						for (UINT x = 0; x < tex->Size.W; x++)
						{
							const CloakEngine::Helper::Color::RGBA& r = tex->Get(x, y);
							ccv.Set(r[0], r[1], r[2]);
							ccv *= g_ToYCbCr;
							ccp = static_cast<CloakEngine::Global::Math::Point>(ccv);
							ccc[0] = ccp.X;
							ccc[1] = ccp.Y;
							ccc[2] = ccp.Z;
							ccc[3] = r[3];
							tex->Set(x, y, ccc);
						}
					}
				}
				bool CLOAK_CALL CanColorConvert(In API::Image::v1008::ColorSpace space, In API::Image::v1008::ColorChannel channel)
				{
					if (space == API::Image::v1008::ColorSpace::YCCA) { return (channel & API::Image::v1008::ColorChannel::Channel_Color) == API::Image::v1008::ColorChannel::Channel_Color; }
					return true;
				}

				namespace Cube {
					constexpr float CUBE_TOP_ANGLE = CloakEngine::Global::Math::CompileTime::atan(1.0f / CloakEngine::Global::Math::CompileTime::sqrt(2.0f));
					constexpr float CUBE_BOTTOM_ANGLE = static_cast<float>(CloakEngine::Global::Math::PI) - CUBE_TOP_ANGLE;

					template<typename A> inline A CLOAK_CALL frac(In A x)
					{
						A ip;
						return std::modf(x, &ip);
					}

					//Idea from: https://stackoverflow.com/questions/29678510/convert-21-equirectangular-panorama-to-cube-map
					bool CLOAK_CALL HDRIToCube(In const Engine::TextureLoader::Image& hdri, In uint32_t cubeSize, Out Engine::TextureLoader::Image sides[6])
					{
						if (hdri.width == hdri.height << 1)
						{
							const size_t sideStride = static_cast<size_t>(cubeSize)*cubeSize;
							//Create empty cube:
							for (size_t a = 0; a < 6; a++)
							{
								sides[a].width = static_cast<UINT>(cubeSize);
								sides[a].height = static_cast<UINT>(cubeSize);
								sides[a].dataSize = sideStride;
								sides[a].data = new CloakEngine::Helper::Color::RGBA[sideStride];
								for (size_t b = 0; b < sideStride; b++) { sides[a].data[b] = CloakEngine::Helper::Color::RGBA(0, 0, 0, 0); }
							}
							
							float* wightSum = NewArray(float, 6 * sideStride);
							memset(wightSum, 0, 6 * sideStride * sizeof(float));
#ifdef PROJECT_HDRI_CUBE
							//Project HDRI on Cube:
							for (UINT y = 0; y < hdri.height; y++)
							{
								const float angleY = static_cast<float>(y*CloakEngine::Global::Math::PI / hdri.height);
								for (UINT x = 0; x < hdri.width; x++)
								{
									const size_t srcPos = (y*hdri.width) + x;
									const float angleX = static_cast<float>(x * 2 * CloakEngine::Global::Math::PI / hdri.width);
									size_t side = 6;
									CloakEngine::Global::Math::Point dir;
									//Calculate Cube side/pos:
									if (angleY < CUBE_TOP_ANGLE) //Top
									{
										side = SIDE_TOP;
										dir.X = std::tan(angleY)*std::cos(angleX);
										dir.Y = std::tan(angleY)*std::sin(angleX);
										dir.Z = 1;
									}
									else if (angleY > CUBE_BOTTOM_ANGLE) //Bottom
									{
										side = SIDE_BOTTOM;
										dir.X = -std::tan(angleY)*std::cos(angleX);
										dir.Y = -std::tan(angleY)*std::sin(angleX);
										dir.Z = -1;
									}
									else if (angleX <= CloakEngine::Global::Math::PI / 4 || angleX > CloakEngine::Global::Math::PI*7.0f / 4.0f) //Left
									{
										side = SIDE_LEFT;
										dir.X = 1;
										dir.Y = std::tan(angleX);
										dir.Z = 1 / (std::tan(angleY)*std::cos(angleX));
										if (dir.Z < -1)
										{
											side = SIDE_BOTTOM;
											dir.X = -std::tan(angleY)*std::cos(angleX);
											dir.Y = -std::tan(angleY)*std::sin(angleX);
											dir.Z = -1;
										}
										else if (dir.Z > 1)
										{
											side = SIDE_TOP;
											dir.X = std::tan(angleY)*std::cos(angleX);
											dir.Y = std::tan(angleY)*std::sin(angleX);
											dir.Z = 1;
										}
									}
									else if (angleX <= CloakEngine::Global::Math::PI*3.0f / 4.0f) //Front
									{
										side = SIDE_FRONT;
										dir.X = std::tan(angleX - static_cast<float>(CloakEngine::Global::Math::PI / 2));
										dir.Y = 1;
										dir.Z = 1 / (std::tan(angleY)*std::cos(angleX - static_cast<float>(CloakEngine::Global::Math::PI / 2)));
										if (dir.Z < -1)
										{
											side = SIDE_BOTTOM;
											dir.X = -std::tan(angleY)*std::cos(angleX);
											dir.Y = -std::tan(angleY)*std::sin(angleX);
											dir.Z = -1;
										}
										else if (dir.Z > 1)
										{
											side = SIDE_TOP;
											dir.X = std::tan(angleY)*std::cos(angleX);
											dir.Y = std::tan(angleY)*std::sin(angleX);
											dir.Z = 1;
										}
									}
									else if (angleX <= CloakEngine::Global::Math::PI*5.0f / 4.0f) //Right
									{
										side = SIDE_RIGHT;
										dir.X = -1;
										dir.Y = std::tan(angleX);
										dir.Z = -1 / (std::tan(angleY)*std::cos(angleX));
										if (dir.Z < -1)
										{
											side = SIDE_BOTTOM;
											dir.X = -std::tan(angleY)*std::cos(angleX);
											dir.Y = -std::tan(angleY)*std::sin(angleX);
											dir.Z = -1;
										}
										else if (dir.Z > 1)
										{
											side = SIDE_TOP;
											dir.X = std::tan(angleY)*std::cos(angleX);
											dir.Y = std::tan(angleY)*std::sin(angleX);
											dir.Z = 1;
										}
									}
									else //Back
									{
										side = SIDE_BACK;
										dir.X = std::tan(angleX - static_cast<float>(CloakEngine::Global::Math::PI*3.0f / 2.0f));
										dir.Y = -1;
										dir.Z = 1 / (std::tan(angleY)*std::cos(angleX - static_cast<float>(CloakEngine::Global::Math::PI*3.0f / 2.0f)));
										if (dir.Z < -1)
										{
											side = SIDE_BOTTOM;
											dir.X = -std::tan(angleY)*std::cos(angleX);
											dir.Y = -std::tan(angleY)*std::sin(angleX);
											dir.Z = -1;
										}
										else if (dir.Z > 1)
										{
											side = SIDE_TOP;
											dir.X = std::tan(angleY)*std::cos(angleX);
											dir.Y = std::tan(angleY)*std::sin(angleX);
											dir.Z = 1;
										}
									}
									CLOAK_ASSUME(side < 6);
									//Convert direction/side to pixel coordinates:
									float fX = 0;
									float fY = 0;
									switch (side)
									{
										case SIDE_RIGHT: //Right
											fX = cubeSize * (1 + dir.Y) / 2.0f;
											fY = cubeSize * (1 - dir.Z) / 2.0f;
											break;
										case SIDE_LEFT: //Left
											fX = cubeSize * (1 + dir.Y) / 2.0f;
											fY = cubeSize * (1 - dir.Z) / 2.0f;
											break;
										case SIDE_TOP: //Top
											fX = cubeSize * (1 - dir.X) / 2.0f;
											fY = cubeSize * (1 + dir.Y) / 2.0f;
											break;
										case SIDE_BOTTOM: //Bottom
											fX = cubeSize * (1 - dir.X) / 2.0f;
											fY = cubeSize * (1 - dir.Y) / 2.0f;
											break;
										case SIDE_FRONT: //Front
											fX = cubeSize * (1 + dir.X) / 2.0f;
											fY = cubeSize * (1 - dir.Z) / 2.0f;
											break;
										case SIDE_BACK: //Back
											fX = cubeSize * (1 + dir.X) / 2.0f;
											fY = cubeSize * (1 - dir.Z) / 2.0f;
											break;
										default:
											break;
									}
									CLOAK_ASSUME(fX >= 0 && fX <= cubeSize);
									CLOAK_ASSUME(fY >= 0 && fY <= cubeSize);

									//Interpolate and fill:
									const uint32_t pos[] = { static_cast<uint32_t>(fX), static_cast<uint32_t>(fX) + 1, static_cast<uint32_t>(fY), static_cast<uint32_t>(fY) + 1 };
									const float weight[] = { 1 - frac(fX),frac(fX),1 - frac(fY),frac(fY) };
									for (size_t a = 0; a < 4; a++)
									{
										const size_t px = a & 0x1;
										const size_t py = (a >> 1) + 2;
										//We don't interpolate over edges (this would be very complex considering the rotations of the sides relative to each other)
										if (pos[px] >= 0 && pos[px] < sides[side].width && pos[py] >= 0 && pos[py] < sides[side].height)
										{
											const size_t dstPos = (pos[py] * sides[side].width) + pos[px];
											float w = weight[px] * weight[py];
											sides[side].data[dstPos] += hdri.data[srcPos] * w;
											wightSum[(side*sideStride) + dstPos] += w;
										}
									}
								}
							}
#endif
							//Project Cube on HDRI:
							for (size_t side = 0; side < 6; side++)
							{
								for (UINT y = 0; y < sides[side].height; y++)
								{
									for (UINT x = 0; x < sides[side].width; x++)
									{
										const size_t dstPos = (y*sides[side].width) + x;
										if (wightSum[(side*sideStride) + dstPos] >= 1.0f)
										{
											//Normalize, if already filled:
											sides[side].data[dstPos] /= wightSum[(side*sideStride) + dstPos];
										}
										else
										{
											sides[side].data[dstPos] = CloakEngine::Helper::Color::RGBA(0, 0, 0, 0);
#ifdef PROJECT_CUBE_HDRI
											CloakEngine::Global::Math::Point dir;
											//Convert pixel coordinates/side in direction:
											const float fX = (2.0f*x) / cubeSize;
											const float fY = (2.0f*y) / cubeSize;
											switch (side)
											{
												case SIDE_RIGHT: //Right
													dir.X = 1 - fX;
													dir.Y = 1;
													dir.Z = 1 - fY;
													break;
												case SIDE_LEFT: //Left
													dir.X = fX - 1;
													dir.Y = -1;
													dir.Z = 1 - fY;
													break;
												case SIDE_TOP: //Top
													dir.X = fY - 1;
													dir.Y = fX - 1;
													dir.Z = 1;
													break;
												case SIDE_BOTTOM: //Bottom
													dir.X = 1 - fY;
													dir.Y = fX - 1;
													dir.Z = -1;
													break;
												case SIDE_FRONT: //Front
													dir.X = 1;
													dir.Y = fX - 1;
													dir.Z = 1 - fY;
													break;
												case SIDE_BACK: //Back
													dir.X = -1;
													dir.Y = 1 - fX;
													dir.Z = 1 - fY;
													break;
												default:
													break;
											}
											//Calculate HDRI coordinates:
											float angleX = std::atan2(dir.Y, dir.X);
											float angleY = std::atan2(dir.Z, std::hypot(dir.X, dir.Y));
											const float U = static_cast<float>((2 * cubeSize*(angleX + (CloakEngine::Global::Math::PI / 2))) / CloakEngine::Global::Math::PI);
											const float V = static_cast<float>((2 * cubeSize*((CloakEngine::Global::Math::PI / 2) - angleY)) / CloakEngine::Global::Math::PI);

											//Interpolate and fill:
											const int32_t pos[] = { static_cast<int32_t>(U), static_cast<int32_t>(U) + 1, static_cast<int32_t>(V), static_cast<int32_t>(V) + 1 };
											const float weight[] = { 1 - frac(U),frac(U),1 - frac(V),frac(V) };
											for (size_t a = 0; a < 4; a++)
											{
												const size_t px = a & 0x1;
												const size_t py = (a >> 1) + 2;
												//Wrap ix
												int32_t ix = pos[px] % hdri.width;
												while (ix < 0) { ix += hdri.width; }
												//Clamp iy
												int32_t iy = pos[py];
												if (iy < 0) { iy = 0; }
												else if (static_cast<uint32_t>(iy) >= hdri.height) { iy = hdri.height - 1; }
												//Fill
												const size_t srcPos = (iy*hdri.width) + ix;
												sides[side].data[dstPos] += hdri.data[srcPos] * weight[px] * weight[py];
											}
#endif
										}
									}
								}
							}
							DeleteArray(wightSum);
						}
						return false;
					}
				}
			}
		}
	}
}
