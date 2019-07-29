#include "stdafx.h"
#include "Engine/Image/MipMapGeneration.h"

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			namespace v1008 {
				inline void CLOAK_CALL ResizeImage(In const MipMapLevel* prev, Out MipMapLevel* next)
				{
					const UINT iW = prev->Size.W;
					const UINT iH = prev->Size.H;
					const UINT oH = next->Size.H;
					const UINT oW = next->Size.W;
					const bool oddX = (iW & 0x1) != 0;
					const bool oddY = (iH & 0x1) != 0;
					CLOAK_ASSUME(iW >= oW && iH >= oH);
					if (oW == iW && oH == iH)
					{
						for (UINT y = 0; y < iH; y++)
						{
							for (UINT x = 0; x < iW; x++) { next->Set(x, y, prev->Get(x, y)); }
						}
					}
					else if (oddY)
					{
						for (UINT py = 0, ny = 0, my = 0; ny < oH; py += 2, ny++, my++)
						{
							if (oddX)
							{
								for (UINT px = 0, nx = 0, mx = 0; nx < oW; px += 2, nx++, mx++)
								{
									//Weights
									const long double xA = static_cast<long double>(oW - mx) / iW;
									const long double xB = static_cast<long double>(oW) / iW;
									const long double xC = static_cast<long double>(mx + 1) / iW;
									const long double yA = static_cast<long double>(oH - my) / iH;
									const long double yB = static_cast<long double>(oH) / iH;
									const long double yC = static_cast<long double>(my + 1) / iH;
									//Tables
									const CloakEngine::Helper::Color::RGBA pd[] = {
										prev->Get(px,py),		prev->Get(px + 1,py),		prev->Get(px + 2,py),
										prev->Get(px,py + 1),	prev->Get(px + 1,py + 1),	prev->Get(px + 2,py + 1),
										prev->Get(px,py + 2),	prev->Get(px + 1,py + 2),	prev->Get(px + 2,py + 2),
									};
									const long double wt[] = {
										xA*yA, xB*yA, xC*yA,
										xA*yB, xB*yB, xC*yB,
										xA*yC, xB*yC, xC*yC,
									};
									//Calculation
									CloakEngine::Helper::Color::RGBA col;
									for (size_t c = 0; c < 4; c++)
									{
										long double rc = 0;
										for (size_t t = 0; t < ARRAYSIZE(pd); t++) { rc += pd[t][c] * wt[t]; }
										col[c] = static_cast<float>(rc);
									}
									next->Set(nx, ny, col);
								}
							}
							else
							{
								for (UINT px = 0, nx = 0; nx < oW; px += 2, nx++)
								{
									//Weights
									const long double xA = 0.5;
									const long double xB = 0.5;
									const long double yA = static_cast<long double>(oH - my) / iH;
									const long double yB = oH == iH ? 0 : static_cast<long double>(oH) / iH;
									const long double yC = oH == iH ? 0 : static_cast<long double>(my + 1) / iH;
									//Tables
									const CloakEngine::Helper::Color::RGBA pd[] = {
										prev->Get(px,py),		prev->Get(px + 1,py),
										prev->Get(px,py + 1),	prev->Get(px + 1,py + 1),
										prev->Get(px,py + 2),	prev->Get(px + 1,py + 2),
									};
									const long double wt[] = {
										xA*yA, xB*yA,
										xA*yB, xB*yB,
										xA*yC, xB*yC,
									};
									//Calculation
									CloakEngine::Helper::Color::RGBA col;
									for (size_t c = 0; c < 4; c++)
									{
										long double rc = 0;
										for (size_t t = 0; t < ARRAYSIZE(pd); t++) { rc += pd[t][c] * wt[t]; }
										col[c] = static_cast<float>(rc);
									}
									next->Set(nx, ny, col);
								}
							}
						}
					}
					else
					{
						for (UINT py = 0, ny = 0; ny < oH; py += 2, ny++)
						{
							if (oddX)
							{
								for (UINT px = 0, nx = 0, mx = 0; nx < oW; px += 2, nx++, mx++)
								{
									//Weights
									const long double xA = static_cast<long double>(oW - mx) / iW;
									const long double xB = oW == iW ? 0 : static_cast<long double>(oW) / iW;
									const long double xC = oW == iW ? 0 : static_cast<long double>(mx + 1) / iW;
									const long double yA = 0.5;
									const long double yB = 0.5;
									//Tables
									const CloakEngine::Helper::Color::RGBA pd[] = {
										prev->Get(px,py),		prev->Get(px + 1,py),		prev->Get(px + 2,py),
										prev->Get(px,py + 1),	prev->Get(px + 1,py + 1),	prev->Get(px + 2,py + 1),
									};
									const long double wt[] = {
										xA*yA, xB*yA, xC*yA,
										xA*yB, xB*yB, xC*yB,
									};
									//Calculation
									CloakEngine::Helper::Color::RGBA col;
									for (size_t c = 0; c < 4; c++)
									{
										long double rc = 0;
										for (size_t t = 0; t < ARRAYSIZE(pd); t++) { rc += pd[t][c] * wt[t]; }
										col[c] = static_cast<float>(rc);
									}
									next->Set(nx, ny, col);
								}
							}
							else
							{
								for (UINT px = 0, nx = 0; nx < oW; px += 2, nx++)
								{
									//Weights
									const long double xA = 0.5;
									const long double xB = 0.5;
									const long double yA = 0.5;
									const long double yB = 0.5;
									//Tables
									const CloakEngine::Helper::Color::RGBA pd[] = {
										prev->Get(px,py),		prev->Get(px + 1,py),
										prev->Get(px,py + 1),	prev->Get(px + 1,py + 1),
									};
									const long double wt[] = {
										xA*yA, xB*yA,
										xA*yB, xB*yB,
									};
									//Calculation
									CloakEngine::Helper::Color::RGBA col;
									for (size_t c = 0; c < 4; c++)
									{
										long double rc = 0;
										for (size_t t = 0; t < ARRAYSIZE(pd); t++) { rc += pd[t][c] * wt[t]; }
										col[c] = static_cast<float>(rc);
									}
									next->Set(nx, ny, col);
								}
							}
						}
					}
				}

				bool CLOAK_CALL NextMipMap(Inout UINT* W, Inout UINT* H)
				{
					if (*W == 1 && *H == 1) { return false; }
					if (*W > 1) { *W >>= 1; }
					if (*H > 1) { *H >>= 1; }
					return true;
				}
				void CLOAK_CALL CreateMipMapLevels(In uint32_t mipMapCount, In uint8_t qualityOffset, Inout MipTexture* mips, In UINT W, In UINT H, In const API::Image::v1008::BuildMeta& meta)
				{
					if (W == 0 || H == 0)
					{
						mips->levelCount = 0;
						mips->levels = nullptr;
					}
					else
					{
						CloakEngine::List<Dimensions> mipSize;
						do {
							mipSize.emplace_back(W, H);
						} while ((mipSize.size() <= mipMapCount || mipSize.size() <= qualityOffset) && NextMipMap(&W, &H));
						mips->levelCount = mipSize.size();
						mips->levels = NewArray(MipMapLevel, mips->levelCount);
						for (size_t a = 0; a < mips->levelCount; a++)
						{
							const Dimensions& d = mipSize[a];
							mips->levels[a].Allocate(d.W, d.H);
							mips->levels[a].Meta = meta;
						}
					}
				}
				void CLOAK_CALL CalculateMipMaps(In uint32_t mipMapCount, In uint8_t qualityOffset, In const Engine::TextureLoader::Image& img, In const API::Image::v1008::BuildMeta& meta, Inout MipTexture* mips)
				{
					CreateMipMapLevels(mipMapCount, qualityOffset, mips, img.width, img.height, meta);
					if (mips->levelCount > 0)
					{
						for (size_t a = 0; a < img.dataSize; a++) { mips->levels[0].Color[a] = img.data[a]; }
						for (size_t a = 1; a < mips->levelCount; a++) { ResizeImage(&mips->levels[a - 1], &mips->levels[a]); }
					}
				}
			}
		}
	}
}