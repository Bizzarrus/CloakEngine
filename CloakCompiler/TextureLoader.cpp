#include "stdafx.h"
#include "Engine/TextureLoader.h"
#include "Engine/Codecs/HDRI.h"

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/StringConvert.h"

#include <wincodec.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#define MAXSIZE (16384)

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) && !defined(DXGI_1_2_FORMATS)
#define DXGI_1_2_FORMATS
#endif

namespace CloakCompiler {
	namespace Engine {
		namespace TextureLoader {
			IWICImagingFactory* g_imgFactory = nullptr;

			struct WICTranslate
			{
				GUID wic;
				DXGI_FORMAT format;
			};

			struct WICConvert
			{
				GUID src;
				GUID target;
			};

			const WICTranslate g_WICFormats[] =
			{
				{ GUID_WICPixelFormat128bppRGBAFloat, DXGI_FORMAT_R32G32B32A32_FLOAT },

				{ GUID_WICPixelFormat64bppRGBAHalf, DXGI_FORMAT_R16G16B16A16_FLOAT },
				{ GUID_WICPixelFormat64bppRGBA, DXGI_FORMAT_R16G16B16A16_UNORM },

				{ GUID_WICPixelFormat32bppRGBA, DXGI_FORMAT_R8G8B8A8_UNORM },
				{ GUID_WICPixelFormat32bppBGRA, DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
				{ GUID_WICPixelFormat32bppBGR, DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1  || Notice: X8 means 8 unused(!) bits

				{ GUID_WICPixelFormat32bppRGBA1010102XR, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
				{ GUID_WICPixelFormat32bppRGBA1010102, DXGI_FORMAT_R10G10B10A2_UNORM },
				{ GUID_WICPixelFormat32bppRGBE, DXGI_FORMAT_R9G9B9E5_SHAREDEXP },

#ifdef DXGI_1_2_FORMATS

				{ GUID_WICPixelFormat16bppBGRA5551, DXGI_FORMAT_B5G5R5A1_UNORM },
				{ GUID_WICPixelFormat16bppBGR565, DXGI_FORMAT_B5G6R5_UNORM },
				{ GUID_WICPixelFormat96bppRGBFloat, DXGI_FORMAT_R32G32B32_FLOAT },
#endif
				{ GUID_WICPixelFormat32bppGrayFloat, DXGI_FORMAT_R32_FLOAT },
				{ GUID_WICPixelFormat16bppGrayHalf, DXGI_FORMAT_R16_FLOAT },
				{ GUID_WICPixelFormat16bppGray, DXGI_FORMAT_R16_UNORM },
				{ GUID_WICPixelFormat8bppGray, DXGI_FORMAT_R8_UNORM },

				{ GUID_WICPixelFormat8bppAlpha, DXGI_FORMAT_A8_UNORM },
			};

			const WICConvert g_WICConvertes[] =
			{
				{ GUID_WICPixelFormatBlackWhite, GUID_WICPixelFormat8bppGray },

				{ GUID_WICPixelFormat1bppIndexed, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat2bppIndexed, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat4bppIndexed, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat8bppIndexed, GUID_WICPixelFormat32bppRGBA },

				{ GUID_WICPixelFormat2bppGray, GUID_WICPixelFormat8bppGray },
				{ GUID_WICPixelFormat4bppGray, GUID_WICPixelFormat8bppGray },

				{ GUID_WICPixelFormat16bppGrayFixedPoint, GUID_WICPixelFormat16bppGrayHalf },
				{ GUID_WICPixelFormat32bppGrayFixedPoint, GUID_WICPixelFormat32bppGrayFloat },

#ifdef DXGI_1_2_FORMATS

				{ GUID_WICPixelFormat16bppBGR555, GUID_WICPixelFormat16bppBGRA5551 },
				{ GUID_WICPixelFormat32bppRGB, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat64bppRGB, GUID_WICPixelFormat64bppRGBA },
				{ GUID_WICPixelFormat64bppPRGBAHalf, GUID_WICPixelFormat64bppRGBAHalf },
#else

				{ GUID_WICPixelFormat16bppBGR555, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat16bppBGRA5551, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat16bppBGR565, GUID_WICPixelFormat32bppRGBA },

#endif // DXGI_1_2_FORMATS

				{ GUID_WICPixelFormat32bppBGR101010, GUID_WICPixelFormat32bppRGBA1010102 },

				{ GUID_WICPixelFormat24bppBGR, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat24bppRGB, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat32bppPBGRA, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat32bppPRGBA, GUID_WICPixelFormat32bppRGBA },

				{ GUID_WICPixelFormat48bppRGB, GUID_WICPixelFormat64bppRGBA },
				{ GUID_WICPixelFormat48bppBGR, GUID_WICPixelFormat64bppRGBA },
				{ GUID_WICPixelFormat64bppBGRA, GUID_WICPixelFormat64bppRGBA },
				{ GUID_WICPixelFormat64bppPRGBA, GUID_WICPixelFormat64bppRGBA },
				{ GUID_WICPixelFormat64bppPBGRA, GUID_WICPixelFormat64bppRGBA },

				{ GUID_WICPixelFormat48bppRGBFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
				{ GUID_WICPixelFormat48bppBGRFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
				{ GUID_WICPixelFormat64bppRGBAFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
				{ GUID_WICPixelFormat64bppBGRAFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
				{ GUID_WICPixelFormat64bppRGBFixedPoint, GUID_WICPixelFormat64bppRGBAHalf },
				{ GUID_WICPixelFormat64bppRGBHalf, GUID_WICPixelFormat64bppRGBAHalf },
				{ GUID_WICPixelFormat48bppRGBHalf, GUID_WICPixelFormat64bppRGBAHalf },

				{ GUID_WICPixelFormat96bppRGBFixedPoint, GUID_WICPixelFormat128bppRGBAFloat },
				{ GUID_WICPixelFormat128bppPRGBAFloat, GUID_WICPixelFormat128bppRGBAFloat },
				{ GUID_WICPixelFormat128bppRGBFloat, GUID_WICPixelFormat128bppRGBAFloat },
				{ GUID_WICPixelFormat128bppRGBAFixedPoint, GUID_WICPixelFormat128bppRGBAFloat },
				{ GUID_WICPixelFormat128bppRGBFixedPoint, GUID_WICPixelFormat128bppRGBAFloat },

				{ GUID_WICPixelFormat32bppCMYK, GUID_WICPixelFormat32bppRGBA },
				{ GUID_WICPixelFormat64bppCMYK, GUID_WICPixelFormat64bppRGBA },
				{ GUID_WICPixelFormat40bppCMYKAlpha, GUID_WICPixelFormat64bppRGBA },
				{ GUID_WICPixelFormat80bppCMYKAlpha, GUID_WICPixelFormat64bppRGBA },

				// n-channel images are not supported
			};

			IWICImagingFactory* CLOAK_CALL getImageFactory()
			{
				if (g_imgFactory == nullptr)
				{
					CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, CE_QUERY_ARGS(&g_imgFactory));
				}
				return g_imgFactory;
			}
			DXGI_FORMAT CLOAK_CALL getFormat(const GUID& guid)
			{
				for (size_t a = 0; a < _countof(g_WICFormats); a++)
				{
					if (memcmp(&g_WICFormats[a].wic, &guid, sizeof(GUID)) == 0) { return g_WICFormats[a].format; }
				}
				return DXGI_FORMAT_UNKNOWN;
			}
			UINT CLOAK_CALL getBitsPerPixel(const GUID& guid)
			{
				UINT bpp = 0;
				IWICImagingFactory* wic = getImageFactory();
				if (wic != nullptr)
				{
					IWICComponentInfo* cInfo = nullptr;
					if (SUCCEEDED(wic->CreateComponentInfo(guid, &cInfo)))
					{
						WICComponentType type;
						if (SUCCEEDED(cInfo->GetComponentType(&type)) && type == WICComponentType::WICPixelFormat)
						{
							IWICPixelFormatInfo* fInfo = nullptr;
							if (SUCCEEDED(cInfo->QueryInterface(CE_QUERY_ARGS(&fInfo))))
							{
								if (FAILED(fInfo->GetBitsPerPixel(&bpp))) { bpp = 0; }
							}
							RELEASE(fInfo);
						}
					}
					RELEASE(cInfo);
				}
				return bpp;
			}
			void CLOAK_CALL makeRGBA(CloakEngine::Helper::Color::RGBA* rgba, BYTE* data, UINT offset, DXGI_FORMAT format)
			{
				if (rgba != nullptr && data != nullptr)
				{
					switch (format)
					{
						case DXGI_FORMAT_R32G32B32A32_FLOAT:
						{
							DirectX::XMFLOAT4 c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.x;
							rgba->G = c.y;
							rgba->B = c.z;
							rgba->A = c.w;
							break;
						}
						case DXGI_FORMAT_R16G16B16A16_FLOAT:
						{
							DirectX::PackedVector::XMHALF4 c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = DirectX::PackedVector::XMConvertHalfToFloat(c.x);
							rgba->G = DirectX::PackedVector::XMConvertHalfToFloat(c.y);
							rgba->B = DirectX::PackedVector::XMConvertHalfToFloat(c.z);
							rgba->A = DirectX::PackedVector::XMConvertHalfToFloat(c.w);
							break;
						}
						case DXGI_FORMAT_R16G16B16A16_UNORM:
						{
							DirectX::PackedVector::XMUSHORTN4 c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.x / 65535.0f;
							rgba->G = c.y / 65535.0f;
							rgba->B = c.z / 65535.0f;
							rgba->A = c.w / 65535.0f;
							break;
						}
						case DXGI_FORMAT_R8G8B8A8_UNORM:
						{
							DirectX::PackedVector::XMCOLOR c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.b / 255.0f;
							rgba->G = c.g / 255.0f;
							rgba->B = c.r / 255.0f;
							rgba->A = c.a / 255.0f;
							break;
						}
						case DXGI_FORMAT_B8G8R8A8_UNORM:
						{
							DirectX::PackedVector::XMCOLOR c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.r / 255.0f;
							rgba->G = c.g / 255.0f;
							rgba->B = c.b / 255.0f;
							rgba->A = c.a / 255.0f;
							break;
						}
						case DXGI_FORMAT_B8G8R8X8_UNORM:
						{
							DirectX::PackedVector::XMCOLOR c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.r / 255.0f;
							rgba->G = c.g / 255.0f;
							rgba->B = c.b / 255.0f;
							rgba->A = 1;
							break;
						}
						case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
						{
							DirectX::PackedVector::XMUDECN4 c;
							memcpy(&c, data + offset, sizeof(c));
							DirectX::XMVECTOR v = DirectX::PackedVector::XMLoadUDecN4_XR(&c);
							DirectX::XMFLOAT4 d;
							DirectX::XMStoreFloat4(&d, v);
							rgba->R = d.x;
							rgba->G = d.y;
							rgba->B = d.z;
							rgba->A = d.w;
							break;
						}
						case DXGI_FORMAT_R10G10B10A2_UNORM:
						{
							DirectX::PackedVector::XMUDECN4 c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.x / 1023.0f;
							rgba->G = c.y / 1023.0f;
							rgba->B = c.z / 1023.0f;
							rgba->A = c.w / 1023.0f;
							break;
						}
						case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
						{
							DirectX::PackedVector::XMFLOAT3SE c;
							memcpy(&c, data + offset, sizeof(c));
							DirectX::XMVECTOR v = DirectX::PackedVector::XMLoadFloat3SE(&c);
							DirectX::XMFLOAT3 d;
							DirectX::XMStoreFloat3(&d, v);
							rgba->R = d.x;
							rgba->G = d.y;
							rgba->B = d.z;
							rgba->A = 1;
							break;
						}
						case DXGI_FORMAT_B5G5R5A1_UNORM:
						{
							DirectX::PackedVector::XMU555 c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.x / 031.0f;
							rgba->G = c.y / 031.0f;
							rgba->B = c.z / 031.0f;
							rgba->A = c.w / 031.0f;
							break;
						}
						case DXGI_FORMAT_B5G6R5_UNORM:
						{
							DirectX::PackedVector::XMU565 c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.x / 031.0f;
							rgba->G = c.y / 63.0f;
							rgba->B = c.z / 031.0f;
							rgba->A = 1;
							break;
						}
						case DXGI_FORMAT_R32_FLOAT:
						{
							float c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c;
							rgba->G = 0;
							rgba->B = 0;
							rgba->A = 0;
							break;
						}
						case DXGI_FORMAT_R16_FLOAT:
						{
							uint16_t c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = DirectX::PackedVector::XMConvertHalfToFloat(c);
							rgba->G = 0;
							rgba->B = 0;
							rgba->A = 0;
							break;
						}
						case DXGI_FORMAT_R16_UNORM:
						{
							uint16_t c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c/ 65535.0f;
							rgba->G = 0;
							rgba->B = 0;
							rgba->A = 0;
							break;
						}
						case DXGI_FORMAT_R8_UNORM:
						{
							BYTE c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c / 255.0f;
							rgba->G = 0;
							rgba->B = 0;
							rgba->A = 0;
							break;
						}
						case DXGI_FORMAT_A8_UNORM:
						{
							BYTE c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = 0;
							rgba->G = 0;
							rgba->B = 0;
							rgba->A = c / 255.0f;
							break;
						}
						case DXGI_FORMAT_R32G32B32_FLOAT:
						{
							DirectX::XMFLOAT3 c;
							memcpy(&c, data + offset, sizeof(c));
							rgba->R = c.x;
							rgba->G = c.y;
							rgba->B = c.z;
							rgba->A = 1;
							break;
						}
						default:
							rgba->R = -1.0f;
							rgba->B = -1.0f;
							rgba->G = -1.0f;
							rgba->A = -1.0f;
							break;
					}
				}
			}
			HRESULT CLOAK_CALL loadFromWIC(IWICBitmapFrameDecode* frame, Image* img)
			{
				HRESULT hRet = E_FAIL;
				IWICImagingFactory* wic = getImageFactory();
				if (wic != nullptr && frame != nullptr && img != nullptr)
				{
					if (img->dataSize > 0) 
					{ 
						delete[] img->data; 
						img->data = nullptr; 
						img->dataSize = 0; 
					}
					UINT tW, tH;
					hRet = frame->GetSize(&tW, &tH);
					if (SUCCEEDED(hRet) && tW > 0 && tH > 0)
					{
						if (tW > MAXSIZE || tH > MAXSIZE)
						{
							float r = static_cast<float>(tH) / static_cast<float>(tW);
							if (tW > tH)
							{
								img->width = MAXSIZE;
								img->height = static_cast<UINT>(MAXSIZE*r);
							}
							else
							{
								img->height = MAXSIZE;
								img->width = static_cast<UINT>(MAXSIZE / r);
							}
						}
						else
						{
							img->width = tW;
							img->height = tH;
						}
						WICPixelFormatGUID WICFormat;
						hRet = frame->GetPixelFormat(&WICFormat);
						if (SUCCEEDED(hRet))
						{
							WICPixelFormatGUID WICConv;
							memcpy(&WICConv, &WICFormat, sizeof(WICPixelFormatGUID));
							UINT bpp = 0;
							DXGI_FORMAT dxFormat = getFormat(WICFormat);
							if (dxFormat == DXGI_FORMAT_UNKNOWN)
							{
								for (size_t a = 0; a < _countof(g_WICConvertes) && bpp == 0; a++)
								{
									if (memcmp(&g_WICConvertes[a].src, &WICFormat, sizeof(WICPixelFormatGUID)) == 0)
									{
										memcpy(&WICConv, &g_WICConvertes[a].target, sizeof(WICPixelFormatGUID));
										dxFormat = getFormat(WICConv);
										if (dxFormat != DXGI_FORMAT_UNKNOWN) { bpp = getBitsPerPixel(WICConv); }
									}
								}
							}
							else
							{
								bpp = getBitsPerPixel(WICFormat);
							}
							if (bpp > 0)
							{
								UINT row = ((img->width*bpp) + 7) / 8;
								UINT size = row*img->height;
								BYTE* data = new BYTE[size];
								if (memcmp(&WICConv, &WICFormat, sizeof(WICPixelFormatGUID)) == 0 && tW == img->width && tH == img->height)
								{
									hRet = frame->CopyPixels(nullptr, row, size, data);
								}
								else if (tW != img->width || tH != img->height)
								{
									IWICBitmapScaler* scale = nullptr;
									hRet = wic->CreateBitmapScaler(&scale);
									if (SUCCEEDED(hRet))
									{
										hRet = scale->Initialize(frame, img->width, img->height, WICBitmapInterpolationModeFant);
										if (SUCCEEDED(hRet))
										{
											WICPixelFormatGUID WICScale;
											hRet = scale->GetPixelFormat(&WICScale);
											if (SUCCEEDED(hRet))
											{
												if (memcmp(&WICScale, &WICFormat, sizeof(WICPixelFormatGUID)) == 0)
												{
													hRet = scale->CopyPixels(0, row, size, data);
												}
												else
												{
													IWICFormatConverter* conv;
													hRet = wic->CreateFormatConverter(&conv);
													if (SUCCEEDED(hRet))
													{
														hRet = conv->Initialize(scale, WICConv, WICBitmapDitherTypeErrorDiffusion, nullptr, 0, WICBitmapPaletteTypeCustom);
														if (SUCCEEDED(hRet))
														{
															hRet = conv->CopyPixels(0, row, size, data);
														}
													}
													RELEASE(conv);
												}
											}
										}
									}
									RELEASE(scale);
								}
								else
								{
									IWICFormatConverter* conv;
									hRet = wic->CreateFormatConverter(&conv);
									if (SUCCEEDED(hRet))
									{
										hRet = conv->Initialize(frame, WICConv, WICBitmapDitherTypeErrorDiffusion, nullptr, 0, WICBitmapPaletteTypeCustom);
										if (SUCCEEDED(hRet))
										{
											hRet = conv->CopyPixels(0, row, size, data);
										}
									}
									RELEASE(conv);
								}
								if (SUCCEEDED(hRet))
								{
									img->dataSize = img->width*img->height;
									img->data = new CloakEngine::Helper::Color::RGBA[img->dataSize];
									UINT byPP = bpp / 8;
									for (UINT a = 0, offset = 0; a < img->dataSize && SUCCEEDED(hRet); a++, offset += byPP)
									{ 
										CloakEngine::Helper::Color::RGBA* rgba = &img->data[a];
										makeRGBA(rgba, data, offset, dxFormat);
										if (rgba->R < 0 || rgba->G < 0 || rgba->B < 0 || rgba->A < 0) { hRet = E_FAIL; }
									}
									if (FAILED(hRet))
									{
										delete[] img->data;
										img->data = nullptr;
										img->dataSize = 0;
									}
								}
								delete[] data;
							}
							else { hRet = E_FAIL; }
						}
					}
				}
				return hRet;
			}

			CLOAK_CALL Image::~Image()
			{
				if (dataSize > 0) { delete[] data; }
			}
			HRESULT CLOAK_CALL LoadTextureFromFile(std::u16string filePath, Image* img)
			{
				HRESULT hRet = E_FAIL;
				//Try to load image using WIC (Windows Imaging Component):
				if (FAILED(hRet))
				{
					IWICImagingFactory* wic = getImageFactory();
					if (wic != nullptr && img != nullptr)
					{
						IWICBitmapDecoder* decode = nullptr;
						hRet = wic->CreateDecoderFromFilename(CloakEngine::Helper::StringConvert::ConvertToW(CloakEngine::Files::GetFullFilePath(filePath)).c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decode);
						if (SUCCEEDED(hRet))
						{
							IWICBitmapFrameDecode* frame = nullptr;
							hRet = decode->GetFrame(0, &frame);
							if (SUCCEEDED(hRet)) { hRet = loadFromWIC(frame, img); }
							RELEASE(frame);
						}
						RELEASE(decode);
					}
				}
				//Try to load image using own codecs:
				if (FAILED(hRet))
				{
					ICodec* const codecs[] = { 
						new Codecs::HDRI(), 
					};
					for (size_t a = 0; a < ARRAYSIZE(codecs) && FAILED(hRet); a++)
					{
						hRet = codecs[a]->Decode(filePath, img);
					}
					for (size_t a = 0; a < ARRAYSIZE(codecs); a++) { delete codecs[a]; }
				}
				return hRet;
			}
		}
	}
}