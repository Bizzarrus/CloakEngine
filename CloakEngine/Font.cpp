#include "stdafx.h"

#include "Implementation/Files/Font.h"
#include "Implementation/Rendering/Manager.h"
#include "Implementation/Global/Localization.h"

#include "CloakEngine/Global/Localization.h"
#include "CloakEngine/Helper/StringConvert.h"

#include "Engine/Graphic/Core.h"

#include <sstream>
#include <queue>
#include <stack>

#define PIX_DISTANT 3
#define MAX_IMG_SIZE 999936
#define EQUALS_FLOAT(a,b) (abs(a-b)<0.000001f)

#define IMG_SPACE_BEGIN 1
#define IMG_SPACE_END 2

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Font_v2 {
				constexpr API::Files::FileType g_fileType{ "BitFont","CEBF",1003 };

				namespace v1003 {
					inline uint8_t CLOAK_CALL RoundFloatU8(float f)
					{
						f = f * 0xff;
						if (f - floorf(f) < 0.5f) { return static_cast<uint8_t>(floorf(f)); }
						return static_cast<uint8_t>(ceilf(f));
					}
					inline uint32_t CLOAK_CALL alignUp(In uint32_t v, In uint32_t align)
					{
						return (v + align - 1) & ~(align - 1);
					}
				}

				CLOAK_CALL_THIS Font::Font()
				{
					m_texture = nullptr;
					DEBUG_NAME(Font);
				}
				CLOAK_CALL_THIS Font::~Font()
				{
					SAVE_RELEASE(m_texture);
				}
				API::Files::Font_v2::LetterInfo CLOAK_CALL_THIS Font::GetLetterInfo(In char32_t c) const
				{
					API::Helper::ReadLock lock(m_sync);
					API::Files::Font_v2::LetterInfo info;
					if (c == U' ')
					{
						info.TopLeft.X = 0;
						info.TopLeft.Y = 0;
						info.BottomRight.X = 0;
						info.BottomRight.Y = 0;
						info.Width = m_spaceWidth;
						info.Type = API::Files::Font_v2::LetterType::SPACE;
					}
					else
					{
						const auto& f = m_fontInfo.find(c);
						if (f == m_fontInfo.end())
						{
							info.TopLeft.X = 0;
							info.TopLeft.Y = 0;
							info.BottomRight.X = 0;
							info.BottomRight.Y = 0;
							info.Width = 0;
							info.Type = API::Files::Font_v2::LetterType::SINGLECOLOR;
						}
						else { return f->second; }
					}
					return info;
				}
				float CLOAK_CALL_THIS Font::getLineHeight(In float lineSpace) const
				{
					return 1 + lineSpace;
				}
				API::Rendering::IColorBuffer* CLOAK_CALL_THIS Font::GetTexture() const
				{
					API::Helper::ReadLock lock(m_sync);
					return m_texture;
				}
			
				Success(return == true) bool CLOAK_CALL_THIS Font::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, FileHandler_v1::RessourceHandler, Files::Font_v2, Font);
				}
				LoadResult CLOAK_CALL_THIS Font::iLoad(In API::Files::IReader* read, In API::Files::FileVersion version)
				{
					API::Helper::WriteLock lock(m_sync);
					CloakDebugLog("Read font version " + std::to_string(version));
					if (version >= 1003)
					{
						const unsigned int bitLen = static_cast<unsigned int>(read->ReadBits(8) + 1);
						const uint32_t spaceWidth = static_cast<uint32_t>(read->ReadBits(bitLen));
						const uint32_t mipMaps = static_cast<uint32_t>(read->ReadBits(4));
						const uint32_t rW = static_cast<uint32_t>(read->ReadBits(20));
						const unsigned int bitW = static_cast<unsigned int>(read->ReadBits(4));
						const unsigned int bitH = static_cast<unsigned int>(read->ReadBits(4));
						const uint32_t lineHeight = static_cast<uint32_t>(read->ReadBits(bitH)) + IMG_SPACE_BEGIN + IMG_SPACE_END;
						m_spaceWidth = static_cast<float>(static_cast<long double>(spaceWidth) / lineHeight);
						size_t lineC = 0;
						while (read->ReadBits(1) == 1) { lineC += 16; }
						lineC += static_cast<size_t>(read->ReadBits(4));
						const uint32_t rH = static_cast<uint32_t>(lineHeight*lineC);
						const size_t bitL = static_cast<size_t>(ceil(log2(lineC)));
						const uint32_t maxVal = (1 << bitLen) - 1;

						API::Rendering::TEXTURE_DESC desc;
						desc.AccessType = API::Rendering::VIEW_TYPE::SRV;
						desc.clearValue = API::Helper::Color::Black;
						desc.Format = API::Rendering::Format::R8G8B8A8_UNORM;
						desc.Height = v1003::alignUp(rH, 1U << mipMaps);
						desc.Width = v1003::alignUp(rW, 1U << mipMaps);
						desc.Texture.MipMaps = mipMaps;
						desc.Texture.SampleCount = 1;
						desc.Type = API::Rendering::TEXTURE_TYPE::TEXTURE;
						Impl::Rendering::IManager* man = Engine::Graphic::Core::GetCommandListManager();
						API::Rendering::IColorBuffer* cb = man->CreateColorBuffer(API::Rendering::CONTEXT_TYPE_GRAPHIC, 0, desc);
						m_texture = nullptr;
						HRESULT hRet = cb->QueryInterface(CE_QUERY_ARGS(&m_texture));
						CLOAK_ASSUME(SUCCEEDED(hRet));
						SAVE_RELEASE(cb);
						m_texture->SetDetailLevel(0);
						API::Rendering::SUBRESOURCE_DATA* srd = NewArray(API::Rendering::SUBRESOURCE_DATA, mipMaps);
						uint8_t** data = NewArray(uint8_t*, mipMaps);

						//Init first mip map by reading data:
						srd[0].RowPitch = desc.Width * 4;
						srd[0].SlicePitch = desc.Height*srd[0].RowPitch;
						data[0] = NewArray(uint8_t, srd[0].SlicePitch);
						memset(data[0], 0, srd[0].SlicePitch * sizeof(uint8_t));
						srd[0].pData = data[0];

						size_t lLine = 0;
						uint32_t curW = IMG_SPACE_BEGIN;
						uint32_t curH = IMG_SPACE_BEGIN;

						while (read->ReadBits(1) == 1)
						{
							//Decode letter:
							const size_t line = static_cast<size_t>(read->ReadBits(bitL));
							API::Files::Font_v2::LetterInfo info;
							const char32_t letter = static_cast<char32_t>(read->ReadBits(32));
							const uint32_t W = static_cast<uint32_t>(read->ReadBits(bitW));
							const unsigned int type = static_cast<unsigned int>(read->ReadBits(2));
							size_t maxC = 0;
							if (type == 0) { info.Type = API::Files::Font_v2::LetterType::SINGLECOLOR; maxC = 1; }
							else if (type == 1) { info.Type = API::Files::Font_v2::LetterType::MULTICOLOR; maxC = 4; }
							else if (type == 2) { info.Type = API::Files::Font_v2::LetterType::TRUECOLOR; maxC = 4; }
							info.TopLeft.X = static_cast<float>(static_cast<long double>(curW) / desc.Width);
							info.TopLeft.Y = static_cast<float>(static_cast<long double>(line*lineHeight) / desc.Height);
							info.BottomRight.X = static_cast<float>(static_cast<long double>(curW + W) / desc.Width);
							info.BottomRight.Y = static_cast<float>(static_cast<long double>(((line + 1)*lineHeight) - (IMG_SPACE_BEGIN + IMG_SPACE_END)) / desc.Height);
							info.Width = static_cast<float>(static_cast<long double>(W) / lineHeight);
							m_fontInfo[letter] = info;
							if (line != lLine)
							{
								curH += lineHeight;
								curW = IMG_SPACE_BEGIN;
								CLOAK_ASSUME(curH + lineHeight - (IMG_SPACE_END + IMG_SPACE_BEGIN) <= desc.Height);
							}
							CLOAK_ASSUME(curW + W + IMG_SPACE_END <= desc.Width);

							const size_t bitLL = static_cast<size_t>(ceil(log2(bitLen + 1)));
							for (size_t c = 0; c < maxC; c++)
							{
								bool decode = true;
								if (info.Type == API::Files::Font_v2::LetterType::MULTICOLOR && c < 3) { decode = read->ReadBits(1) == 1; }
								if (decode)
								{
									for (uint32_t y = 0; y < lineHeight - (IMG_SPACE_END + IMG_SPACE_BEGIN); y++)
									{
										uint32_t x = 0;
										uint32_t tr = 0;
										size_t bl = 0;
										while (x < W)
										{
											if (tr == 0)
											{
												tr = static_cast<uint32_t>(read->ReadBits(bitW));
												bl = static_cast<size_t>(read->ReadBits(bitLL));
											}
											const size_t p = (x + curW + ((curH + y)*desc.Width)) * 4;
											const uint32_t dat = static_cast<uint8_t>(read->ReadBits(bl));
											const uint8_t rd = v1003::RoundFloatU8(static_cast<float>(static_cast<long double>(dat) / maxVal));
											if (info.Type == API::Files::Font_v2::LetterType::SINGLECOLOR)
											{
												data[0][p + 0] = 255;
												data[0][p + 1] = 255;
												data[0][p + 2] = 255;
												data[0][p + 3] = rd;
											}
											else if (info.Type == API::Files::Font_v2::LetterType::MULTICOLOR)
											{
												data[0][p + c] = rd;
											}
											else if (info.Type == API::Files::Font_v2::LetterType::TRUECOLOR)
											{
												data[0][p + c] = rd;
											}
											x++;
											tr--;
										}
									}
								}
							}


							curW += W + IMG_SPACE_BEGIN + IMG_SPACE_END;
						}

						//Init all other mip maps:
						//This is the cheapest way to calculate the mip maps (we grant width and high of previous mip level to be even)
						for (size_t a = 1; a < mipMaps; a++)
						{
							const uint32_t cW = max(1, desc.Width >> a);
							const uint32_t cH = max(1, desc.Height >> a);
							srd[a].RowPitch = cW * 4;
							srd[a].SlicePitch = cH*srd[a].RowPitch;
							data[a] = NewArray(uint8_t, srd[a].SlicePitch);
							memset(data[a], 0, srd[a].SlicePitch * sizeof(uint8_t));
							srd[a].pData = data[a];

							const LONG_PTR lW = srd[a - 1].RowPitch;

							for (uint32_t y = 0; y < cH; y++)
							{
								for (uint32_t x = 0; x < cW; x++)
								{
									size_t p[4];
									p[0] = static_cast<size_t>((8 * x) + (2 * lW*y));
									p[1] = static_cast<size_t>(p[0] + 4);
									p[2] = static_cast<size_t>(p[0] + lW);
									p[3] = static_cast<size_t>(p[1] + lW);

									__m128 c[4];
									__m128 norm = _mm_set1_ps(4);
									for (size_t d = 0; d < 4; d++)
									{
										c[d] = _mm_div_ps(_mm_set_ps(data[a - 1][p[d] + 0], data[a - 1][p[d] + 1], data[a - 1][p[d] + 2], data[a - 1][p[d] + 3]), norm);
									}
									__m128 res = _mm_add_ps(_mm_add_ps(c[0], c[1]), _mm_add_ps(c[2], c[3]));
									const size_t cp = static_cast<size_t>(4 * (x + (y*cW)));
									CLOAK_ALIGN float rf[4];
									_mm_store_ps(rf, res);
									for (size_t d = 0; d < 4; d++)
									{
										const uint16_t r = static_cast<uint16_t>(rf[d] + 0.5f);
										data[a][cp + d] = r > 255 ? 255 : static_cast<uint8_t>(r);
									}
								}
							}
						}

						man->InitializeTexture(m_texture, mipMaps, srd, true);
						man->GenerateViews(&m_texture, 1, API::Rendering::VIEW_TYPE::ALL, API::Rendering::DescriptorPageType::GUI);

						for (size_t a = 0; a < mipMaps; a++) { DeleteArray(data[a]); }
						DeleteArray(data);
						DeleteArray(srd);
						CloakDebugLog("Font loaded! Letter Hight = " + std::to_string(lineHeight) + " letters: " + std::to_string(m_fontInfo.size()) + " mipMaps = " + std::to_string(mipMaps));
#ifdef _DEBUG
						m_texture->SetDebugName("Font texture");
#endif
					}
					return LoadResult::Finished;
				}
				void CLOAK_CALL_THIS Font::iUnload()
				{
					API::Helper::WriteLock lock(m_sync);
					m_fontInfo.clear();
					SAVE_RELEASE(m_texture);
				}

				IMPLEMENT_FILE_HANDLER(Font);
			}
		}
	}
}