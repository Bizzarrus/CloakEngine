#include "stdafx.h"
#include "Engine/ImageBuilder.h"

#define PRINT_LOAD_ERR(path) CloakEngine::Global::Log::WriteToLog(u"Failed to read image '" + path + u"'", CloakEngine::Global::Log::Type::Warning)

#define LBI_FUNC_LOAD(path, res, num) hRet=TextureLoader::LoadTextureFromFile(path,&res[num]); if(FAILED(hRet)){PRINT_LOAD_ERR(path); return hRet;}
#define LBI_FUNC_META(path, res, num) res->CheckPaths.push_back(path)
#define LOAD_BORDERED_IMG(info, res, func)\
func(info.CornerTopLeft, res, 0);\
func(info.SideTop, res, 1);\
func(info.CornerTopRight, res, 2);\
func(info.SideLeft, res, 3);\
func(info.Center, res, 4);\
func(info.SideRight, res, 5);\
func(info.CornerBottomLeft, res, 6);\
func(info.SideBottom, res, 7);\
func(info.CornerBottomRight, res, 8)

namespace CloakCompiler {
	namespace Engine {
		namespace ImageBuilder {

			const uint8_t Flag_AdditionalInfo = 0x01;
			const uint8_t Flag_BorderedImg = 0x02;
			/*
			Flags 0x04 - 0x80 are reservered for later use
			*/

			HRESULT CLOAK_CALL BuildBorderedImage(In const API::Image::IMAGE_BUILD_BORDERED& info, Inout Engine::TextureLoader::Image* img)
			{
				if (img->dataSize > 0)
				{
					delete[] img->data;
					img->data = nullptr;
					img->dataSize = 0;
				}
				TextureLoader::Image parts[9];
				HRESULT hRet = S_OK;
				LOAD_BORDERED_IMG(info, parts, LBI_FUNC_LOAD);

				UINT maxW = 0;
				UINT maxH = 0;
				for (size_t a = 0; a < 9; a++)
				{
					maxW = max(maxW, parts[a].width);
					maxH = max(maxH, parts[a].height);
				}

				if (maxW > 0 && maxH > 0)
				{
					img->height = 3 * maxH;
					img->width = 3 * maxW;
					img->dataSize = img->width*img->height;
					img->data = new CloakEngine::Helper::Color::RGBA[img->dataSize];
					for (UINT a = 0; a < 9; a++)
					{
						const UINT xOff = (a % 3)*maxW;
						const UINT yOff = (a / 3)*maxH;
						if (parts[a].width == maxW && parts[a].height == maxH)
						{
							for (size_t y = 0; y < maxH; y++)
							{
								for (size_t x = 0; x < maxW; x++)
								{
									img->data[xOff + x + ((yOff + y)*maxW * 3)] = parts[a].data[x + (y*maxW)];
								}
							}
						}
						else
						{
							UINT xc = 0;
							UINT yc = 0;
							UINT px = 0;
							UINT py = 0;
							for (size_t y = 0; y < maxH; y++, yc += parts[a].height)
							{
								if (yc >= maxH) { yc -= maxH; py++; }
								const UINT yw[] = { maxH - yc, yc + parts[a].height > maxH ? (yc + parts[a].height) - maxH : 0 };
								const UINT yws = yw[0] + yw[1];
								const UINT yp[] = { py,py + 1 };
								for (size_t x = 0; x < maxW; x++, xc += parts[a].width)
								{
									if (xc >= maxW) { xc -= maxW; px++; }
									const UINT xw[] = { maxW - xc,xc + parts[a].width > maxW ? (xc + parts[a].width) - maxW : 0 };
									const UINT xws = xw[0] + xw[1];
									const UINT xp[] = { px,px + 1 };
									CloakEngine::Helper::Color::RGBA rc;
									for (size_t yi = 0; yi < 2 && yw[yi] > 0; yi++)
									{
										for (size_t xi = 0; xi < 2 && xw[xi] > 0; xi++)
										{
											const CloakEngine::Helper::Color::RGBA& pc = parts[a].data[xp[xi] + (parts[a].width*yp[yi])];
											for (size_t c = 0; c < 4; c++)
											{
												rc[c] = pc[c] * static_cast<float>((static_cast<long double>(yw[yi]) / yws)*(static_cast<long double>(xw[xi]) / xws));
											}
										}
									}
									img->data[xOff + x + ((yOff + y)*maxW * 3)] = rc;
								}
							}
						}
					}
					return S_OK;
				}
				return E_FAIL;
			}

			HRESULT CLOAK_CALL BuildImage(In const API::Image::v1008::BuildInfo& info, Out Engine::TextureLoader::Image* img, Out BuildMeta* meta)
			{
				HRESULT hRet = MetaFromInfo(info, meta);
				if (SUCCEEDED(hRet))
				{
					if (img == nullptr) { return E_FAIL; }
					if ((meta->BasicFlags & Flag_BorderedImg) != 0) { hRet = BuildBorderedImage(info.Bordered, img); }
					else
					{
						hRet = TextureLoader::LoadTextureFromFile(info.Normal.Path, img);
						if (FAILED(hRet)) { PRINT_LOAD_ERR(info.Normal.Path); return hRet; }
					}
				}
				return hRet;
			}
			HRESULT CLOAK_CALL MetaFromInfo(In const API::Image::v1008::BuildInfo& info, Out BuildMeta* meta)
			{
				if (meta == nullptr) { return E_FAIL; }
				meta->BasicFlags = 0;
				meta->CheckPaths.clear();
				switch (info.Meta.Type)
				{
					case API::Image::BuildType::NORMAL:
						meta->CheckPaths.push_back(info.Normal.Path);
						return S_OK;
					case API::Image::BuildType::BORDERED:
						meta->BasicFlags |= Flag_BorderedImg;
						LOAD_BORDERED_IMG(info.Bordered, meta, LBI_FUNC_META);
						return S_OK;
					default:
						break;
				}
				return E_FAIL;
			}
		}
	}
}