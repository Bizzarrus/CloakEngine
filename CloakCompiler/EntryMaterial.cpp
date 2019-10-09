#include "stdafx.h"
#include "Engine/Image/EntryMaterial.h"
#include "Engine/Image/MipMapGeneration.h"
#include "Engine/Image/TextureEncoding.h"
#include "Engine/TempHandler.h"

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			namespace v1008 {
				namespace EntryMaterial_v1 {
					constexpr CloakEngine::Files::FileType g_TempFileType{ "ImageTemp","CEIT",1008 };

					struct CombineHint {
						const Engine::TextureLoader::Image* img = nullptr;
						uint8_t channel = 0;
					};
					inline size_t CLOAK_CALL GetChannelCount(In API::Image::v1008::ColorChannel c)
					{
						size_t r = 0;
						if ((c & API::Image::v1008::ColorChannel::Channel_1) != API::Image::v1008::ColorChannel::Channel_None) { r++; }
						if ((c & API::Image::v1008::ColorChannel::Channel_2) != API::Image::v1008::ColorChannel::Channel_None) { r++; }
						if ((c & API::Image::v1008::ColorChannel::Channel_3) != API::Image::v1008::ColorChannel::Channel_None) { r++; }
						if ((c & API::Image::v1008::ColorChannel::Channel_4) != API::Image::v1008::ColorChannel::Channel_None) { r++; }
						return r;
					}
					inline bool CLOAK_CALL CombineImages(In const Engine::TextureLoader::Image& img1, In const Engine::TextureLoader::Image& img2, In API::Image::v1008::ColorChannel src1, In API::Image::v1008::ColorChannel src2, In API::Image::v1008::ColorChannel dst1, In API::Image::v1008::ColorChannel dst2, Out Engine::TextureLoader::Image* res, In const ResponseCaller& response, In size_t imgNum, In size_t texNum)
					{
						if (img1.width != img2.width || img1.height != img2.height) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Cannot combine images: Need to be the same size!", texNum); return true; }
						if ((dst1 & dst2) != API::Image::v1008::ColorChannel::Channel_None) { SendResponse(response, API::Image::v1008::RespondeState::ERR_CHANNEL_MISMATCH, imgNum, "Try to combine at same destination channel", texNum); return true; }
						if (GetChannelCount(src1) != GetChannelCount(dst1) || GetChannelCount(src2) != GetChannelCount(dst2)) { SendResponse(response, API::Image::v1008::RespondeState::ERR_CHANNEL_MISMATCH, imgNum, "Destination channel count does not match source channel count", texNum); return true; }
						if (res->data != nullptr) { delete[] res->data; }
						res->width = img1.width;
						res->height = img1.height;
						res->dataSize = img1.dataSize;
						res->data = new CloakEngine::Helper::Color::RGBA[res->dataSize];
						CombineHint hints[4];
						for (uint8_t a = 0, b = 0; a < 4; a++)
						{
							const API::Image::v1008::ColorChannel d = static_cast<API::Image::v1008::ColorChannel>(1 << a);
							if ((src1 & d) != API::Image::v1008::ColorChannel::Channel_None)
							{
								while ((dst1 & static_cast<API::Image::v1008::ColorChannel>(1 << b)) == API::Image::v1008::ColorChannel::Channel_None) { b++; }
								hints[b].img = &img1;
								hints[b].channel = a;
								b++;
							}
						}
						for (uint8_t a = 0, b = 0; a < 4; a++)
						{
							const API::Image::v1008::ColorChannel d = static_cast<API::Image::v1008::ColorChannel>(1 << a);
							if ((src2 & d) != API::Image::v1008::ColorChannel::Channel_None)
							{
								while ((dst2 & static_cast<API::Image::v1008::ColorChannel>(1 << b)) == API::Image::v1008::ColorChannel::Channel_None) { b++; }
								hints[b].img = &img2;
								hints[b].channel = a;
								b++;
							}
						}
						for (UINT y = 0; y < img1.height; y++)
						{
							for (UINT x = 0; x < img1.width; x++)
							{
								const size_t p = (static_cast<size_t>(y)*img1.width) + x;
								for (uint8_t c = 0; c < 4; c++)
								{
									if (hints[c].img != nullptr)
									{
										res->data[p][c] = hints[c].img->data[p][hints[c].channel];
									}
									else
									{
										res->data[p][c] = 0;
									}
								}
							}
						}
						return false;
					}
					inline void CLOAK_CALL FillImage(Inout Engine::TextureLoader::Image* img, In float value, In API::Image::v1008::ColorChannel dst)
					{
						for (UINT y = 0; y < img->height; y++)
						{
							for (UINT x = 0; x < img->width; x++)
							{
								const size_t p = (static_cast<size_t>(y)*img->width) + x;
								for (uint8_t c = 0; c < 4; c++)
								{
									if ((dst & static_cast<API::Image::v1008::ColorChannel>(1 << c)) != API::Image::v1008::ColorChannel::Channel_None) { img->data[p][c] = value; }
								}
							}
						}
					}
					//Computes pre-multiplied albedo texture
					inline bool CLOAK_CALL ComputeAlbedo(In const Engine::TextureLoader::Image& albedo, In const Engine::TextureLoader::Image& metallic, Out Engine::TextureLoader::Image* res, In const ResponseCaller& response, In size_t imgNum, In size_t texNum)
					{
						if (albedo.width != metallic.width || albedo.height != metallic.height) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Cannot combine images: Need to be the same size!", texNum); return true; }
						if (res->data != nullptr) { delete[] res->data; }
						res->width = albedo.width;
						res->height = albedo.height;
						res->dataSize = albedo.dataSize;
						res->data = new CloakEngine::Helper::Color::RGBA[res->dataSize];
						for (UINT y = 0; y < res->height; y++)
						{
							for (UINT x = 0; x < res->width; x++)
							{
								const size_t p = (static_cast<size_t>(y)*res->width) + x;
								for (size_t c = 0; c < 3; c++) { res->data[p][c] = albedo.data[p][c] * (1 - metallic.data[p][0]); }
								res->data[p][3] = albedo.data[p][3];
							}
						}
						return false;
					}
					inline bool CLOAK_CALL ComputeNormalMap(In const Engine::TextureLoader::Image& normal, In const Engine::TextureLoader::Image& porosity, In const Engine::TextureLoader::Image& ao, Out Engine::TextureLoader::Image* res, In API::Image::v1008::NORMAL_FLAGS flags, In const ResponseCaller& response, In size_t imgNum, In size_t texNum)
					{
						if (normal.width != ao.width || normal.height != ao.height) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Cannot combine images: Need to be the same size!", texNum); return true; }
						if (res->data != nullptr) { delete[] res->data; }
						res->width = normal.width;
						res->height = normal.height;
						res->dataSize = normal.dataSize;
						res->data = new CloakEngine::Helper::Color::RGBA[res->dataSize];
						CloakEngine::Global::Math::Point norm;
						for (UINT y = 0; y < res->height; y++)
						{
							for (UINT x = 0; x < res->width; x++)
							{
								const size_t p = (static_cast<size_t>(y)*res->width) + x;
								if (static_cast<uint32_t>(flags & API::Image::v1008::NORMAL_FLAG_X_LEFT) != 0) { norm.X = 1 - normal.data[p][0]; }
								else { norm.X = normal.data[p][0]; }
								if (static_cast<uint32_t>(flags & API::Image::v1008::NORMAL_FLAG_Y_BOTTOM) != 0) { norm.Y = 1 - normal.data[p][1]; }
								else { norm.Y = normal.data[p][1]; }
								if (static_cast<uint32_t>(flags & API::Image::v1008::NORMAL_FLAG_Z_HALF_RANGE) != 0) { norm.Z = (normal.data[p][2] * 2) - 1; }
								else { norm.Z = normal.data[p][2]; }
								norm.X = (norm.X * 2) - 1;
								norm.Y = (norm.Y * 2) - 1;
								norm = static_cast<CloakEngine::Global::Math::Point>(static_cast<CloakEngine::Global::Math::Vector>(norm).Normalize());

								/*
								Stereoskopic projection would be:
								float dnorm = 1 / (1 + norm.Z);
								norm.X = norm.X * dnorm;
								norm.Y = norm.Y * dnorm;

								Use in shader with:
								dnorm = 2 / (1 + (nX * nX) + (nY * nY));
								X = norm.X * dnorm;
								Y = norm.Y * dnorm;
								Z = dnorm - 1;

								Ortogrpahic projection would be:
								norm.X = norm.X;
								norm.Y = norm.Y;

								Use in shader with:
								X = norm.X;
								Y = norm.Y;
								Z = sqrt(1 - ((nX * nX) + (nY * nY)));


								Stereoskopic results in better textur interpolation quality, but worse representation of normals with Z near 1
								*/
								res->data[p][0] = (norm.X + 1)*0.5f;
								res->data[p][1] = (norm.Y + 1)*0.5f;
								res->data[p][2] = porosity.data[p][0];
								res->data[p][3] = ao.data[p][0];
							}
						}
						return false;
					}
					//Computes specular map by IndexOfRefraction and Extinction Coefficient (Expecting sorrounding material is air)
					//TODO: Write IOR and EC values in structured buffer, write indices pointing into that buffer to texture
					//	-> Allows to use actual IOR and EC values in shader (improved fresnel term, etc)
					//	-> requires manuel mip map generation, as indices are not lerp-able
					inline bool CLOAK_CALL ComputeSpecular(In const Engine::TextureLoader::Image& ior, In const Engine::TextureLoader::Image& ecf, Out Engine::TextureLoader::Image* res, In const ResponseCaller& response, In size_t imgNum, In size_t texNum, In float maxIOR, In float maxEC)
					{
						if (ior.width != ecf.width || ior.height != ecf.height) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Cannot combine images: Need to be the same size!", texNum); return true; }
						if (res->data != nullptr) { delete[] res->data; }
						res->width = ior.width;
						res->height = ior.height;
						res->dataSize = ior.dataSize;
						res->data = new CloakEngine::Helper::Color::RGBA[res->dataSize];
						for (UINT y = 0; y < res->height; y++)
						{
							for (UINT x = 0; x < res->width; x++)
							{
								const size_t p = (static_cast<size_t>(y)*res->width) + x;
								for (size_t c = 0; c < 3; c++)
								{
									const float n = ior.data[p][c] * maxIOR;
									const float k = ecf.data[p][c] * maxEC;
									const float ns = n - 1;
									const float ng = n + 1;
									const float ks = k * k;
									res->data[p][c] = ((ns*ns) + ks) / ((ng*ng) + ks);
								}
							}
						}
						return false;
					}
					//Computes specular map by albedo, metallic and an default value
					inline bool CLOAK_CALL ComputeSpecular(In const Engine::TextureLoader::Image& albedo, In const Engine::TextureLoader::Image& metallic, Out Engine::TextureLoader::Image* res, In const ResponseCaller& response, In size_t imgNum, In size_t texNum)
					{
						if (albedo.width != metallic.width || albedo.height != metallic.height) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Cannot combine images: Need to be the same size!", texNum); return true; }
						if (res->data != nullptr) { delete[] res->data; }
						res->width = albedo.width;
						res->height = albedo.height;
						res->dataSize = albedo.dataSize;
						res->data = new CloakEngine::Helper::Color::RGBA[res->dataSize];
						for (UINT y = 0; y < res->height; y++)
						{
							for (UINT x = 0; x < res->width; x++)
							{
								const size_t p = (static_cast<size_t>(y)*res->width) + x;
								float m = metallic.data[p][0];
								for (size_t c = 0; c < 3; c++) { res->data[p][c] = (albedo.data[p][c] * m) + ((1 - m)*0.04f); }
							}
						}
						return false;
					}
					inline bool CLOAK_CALL TryLoadTexture(In const API::Image::v1008::OptionalInfo& info, In const ResponseCaller& response, In size_t imgNum, In size_t texNum, Out Engine::TextureLoader::Image* res, Out TextureMeta* meta)
					{
						if (info.Use)
						{
							HRESULT hRet = Engine::ImageBuilder::BuildImage(info, res, &meta->Builder);
							meta->Encoding = info.Meta;
							if (FAILED(hRet)) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_BUILDING, imgNum, "Couldn't build image", texNum); return true; }
						}
						else
						{
							if (res->data != nullptr) { delete[] res->data; }
							res->width = 0;
							res->height = 0;
							res->dataSize = 0;
							res->data = nullptr;
						}
						return false;
					}
					inline bool CLOAK_CALL TryFillConstants(In FillPlan** plans, In size_t planSize, Inout Engine::TextureLoader::Image* imgs, In TextureMeta* meta, In const ResponseCaller& response, In size_t imgNum, Out TextureMeta* combMeta, In uint8_t qualityOffset)
					{
						UINT W = 1;
						UINT H = 1;
						for (size_t a = 0; a < planSize; a++)
						{
							Engine::TextureLoader::Image* img = &imgs[plans[a]->img];
							W = max(W, img->width);
							H = max(H, img->height);
						}
						bool foundMeta = false;
						size_t metaPos = 0;
						for (size_t a = 0; a < planSize; a++)
						{
							Engine::TextureLoader::Image* img = &imgs[plans[a]->img];
							if (img->width != 0 && img->height != 0)
							{
								if (foundMeta == false)
								{
									foundMeta = true;
									metaPos = plans[a]->img;
								}
								else
								{
									const TextureMeta& m = meta[metaPos];
									const TextureMeta& n = meta[plans[a]->img];
									if (m.Encoding.BlockSize != n.Encoding.BlockSize || m.Encoding.Space != n.Encoding.Space) { SendResponse(response, API::Image::v1008::RespondeState::ERR_BLOCKMETA_MISMATCH, imgNum, "Meta of combining textures does not match!", plans[a]->img + 1); return true; }
								}
							}
						}
						if (foundMeta)
						{
							*combMeta = meta[metaPos];
							for (size_t a = 0; a < planSize; a++)
							{
								Engine::TextureLoader::Image* img = &imgs[plans[a]->img];
								if (metaPos != plans[a]->img)
								{
									const TextureMeta& m = meta[metaPos];
									TextureMeta* n = &meta[plans[a]->img];
									n->Encoding.BlockSize = m.Encoding.BlockSize;
									n->Encoding.Space = m.Encoding.Space;
								}
								if (img->width == 0 || img->height == 0)
								{
									if (img->data != nullptr) { delete[] img->data; }
									img->width = W;
									img->height = H;
									img->dataSize = static_cast<size_t>(W)*H;
									img->data = new CloakEngine::Helper::Color::RGBA[img->dataSize];
									for (size_t c = 0; c < 4; c++) { FillImage(img, plans[a]->Constant[c], static_cast<API::Image::v1008::ColorChannel>(1 << c)); }
								}
								else if (img->width != W || img->height != H) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_SIZE, imgNum, "Textures need to be the same size!", plans[a]->img + 1); return true; }
							}
						}
						else
						{
							/*
							const UINT s = 1 << (qualityOffset + 4);
							for (size_t a = 0; a < planSize; a++)
							{
								Engine::TextureLoader::Image* img = &imgs[plans[a]->img];
								if (img->data != nullptr) { delete[] img->data; }
								img->width = s;
								img->height = s;
								img->dataSize = static_cast<size_t>(img->width)*img->height;
								img->data = new CloakEngine::Helper::Color::RGBA[img->dataSize];
								for (size_t c = 0; c < 4; c++) { FillImage(img, plans[a]->Constant[c], static_cast<API::Image::v1008::ColorChannel>(1 << c)); }
							}
							*/
							for (size_t a = 0; a < planSize; a++)
							{
								Engine::TextureLoader::Image* img = &imgs[plans[a]->img];
								if (img->data != nullptr) { delete[] img->data; }
								img->width = 1;
								img->height = 1;
								img->dataSize = static_cast<size_t>(img->width)*img->height;
								img->data = new CloakEngine::Helper::Color::RGBA[img->dataSize];
								for (size_t c = 0; c < 4; c++) { FillImage(img, plans[a]->Constant[c], static_cast<API::Image::v1008::ColorChannel>(1 << c)); }
							}
						}
						return false;
					}

					CLOAK_CALL EntryMaterial::EntryMaterial(In const API::Image::v1008::IMAGE_INFO_MATERIAL& img, In const ResponseCaller& response, In size_t imgNum, In uint8_t qualityOffset, In API::Image::v1008::Heap heap)
					{
						m_heap = heap;
						m_qualityOffset = qualityOffset;
						m_type = img.Type;
						m_err = false;
						m_normFlags = img.NormalMapFlags;

						switch (m_type)
						{
							case CloakCompiler::API::Image::v1008::MaterialType::Opaque:
							{
								Engine::TextureLoader::Image tmp[ARRAYSIZE(m_metas)];
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.Albedo, response, imgNum, 1, &tmp[0], &m_metas[0]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.NormalMap, response, imgNum, 2, &tmp[1], &m_metas[1]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.Metallic, response, imgNum, 3, &tmp[2], &m_metas[2]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.AmbientOcclusion, response, imgNum, 4, &tmp[3], &m_metas[3]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.RoughnessMap, response, imgNum, 5, &tmp[4], &m_metas[4]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.SpecularMap, response, imgNum, 6, &tmp[5], &m_metas[5]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.IndexOfRefractionMap, response, imgNum, 7, &tmp[6], &m_metas[6]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.ExtinctionCoefficientMap, response, imgNum, 8, &tmp[7], &m_metas[7]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.DisplacementMap, response, imgNum, 9, &tmp[8], &m_metas[8]);
								m_err = m_err || TryLoadTexture(img.Opaque.Textures.PorosityMap, response, imgNum, 10, &tmp[9], &m_metas[9]);

								m_useDisplacement = img.Opaque.Textures.DisplacementMap.Use;

								float maxIOR = img.Opaque.Textures.HighestIndexOfRefraction;
								float maxEC = img.Opaque.Textures.HighestExtinctionCoefficient;
								if (tmp[6].dataSize == 0)
								{
									maxIOR = 0;
									for (size_t a = 0; a < 3; a++) { maxIOR = max(maxIOR, img.Opaque.Constants.Fresnel[a].RefractionIndex); }
								}
								if (tmp[7].dataSize == 0)
								{
									maxEC = 0;
									for (size_t a = 0; a < 3; a++) { maxEC = max(maxEC, img.Opaque.Constants.Fresnel[a].ExtinctionCoefficient); }
								}
								if (maxIOR == 0) { maxIOR = 1; }
								if (maxEC == 0) { maxEC = 1; }

								m_fillPlan[0].Constant = img.Opaque.Constants.Albedo;
								m_fillPlan[1].Constant = CloakEngine::Helper::Color::RGBA(0.5f, 0.5f, 1.0f, 0);
								m_fillPlan[2].Constant = CloakEngine::Helper::Color::RGBA(img.Opaque.Constants.Metallic, 0, 0, 0);
								m_fillPlan[3].Constant = CloakEngine::Helper::Color::RGBA(1, 0, 0, 0);
								m_fillPlan[4].Constant = CloakEngine::Helper::Color::RGBA(img.Opaque.Constants.Roughness, 0, 0, 0);
								m_fillPlan[5].Constant = CloakEngine::Helper::Color::RGBA(0, 0, 0, 0);
								m_fillPlan[6].Constant = CloakEngine::Helper::Color::RGBA(img.Opaque.Constants.Fresnel[0].RefractionIndex / maxIOR, img.Opaque.Constants.Fresnel[1].RefractionIndex / maxIOR, img.Opaque.Constants.Fresnel[2].RefractionIndex / maxIOR, 0);
								m_fillPlan[7].Constant = CloakEngine::Helper::Color::RGBA(img.Opaque.Constants.Fresnel[0].ExtinctionCoefficient / maxEC, img.Opaque.Constants.Fresnel[1].ExtinctionCoefficient / maxEC, img.Opaque.Constants.Fresnel[2].ExtinctionCoefficient / maxEC, 0);
								m_fillPlan[8].Constant = CloakEngine::Helper::Color::RGBA(0.5f, 0, 0, 0);
								m_fillPlan[9].Constant = CloakEngine::Helper::Color::RGBA(img.Opaque.Constants.Porosity, 0, 0, 0);
								for (size_t a = 0; a < ARRAYSIZE(m_metas); a++) { m_fillPlan[a].img = a; }
								FillPlan* plan0[] = { &m_fillPlan[0],&m_fillPlan[2] };
								FillPlan* plan1[] = { &m_fillPlan[1],&m_fillPlan[9],&m_fillPlan[3] };
								FillPlan* plan2[] = { &m_fillPlan[4],&m_fillPlan[5],&m_fillPlan[6],&m_fillPlan[7] };
								FillPlan* plan3[] = { &m_fillPlan[0],&m_fillPlan[2],&m_fillPlan[4],&m_fillPlan[5] };

								if (img.Opaque.Textures.SpecularMap.Use == false)
								{
									float minIOR = 0;
									for (size_t c = 0; c < 3; c++) { if (c == 0 || m_fillPlan[6].Constant[c] < minIOR) { minIOR = m_fillPlan[6].Constant[c]; } }
									if (img.Opaque.Textures.IndexOfRefractionMap.Use || img.Opaque.Textures.ExtinctionCoefficientMap.Use || minIOR > 0)
									{
										m_err = m_err || TryFillConstants(plan0, ARRAYSIZE(plan0), tmp, m_metas, response, imgNum, &m_comMetas[0], qualityOffset);
										m_err = m_err || TryFillConstants(plan1, ARRAYSIZE(plan1), tmp, m_metas, response, imgNum, &m_comMetas[1], qualityOffset);
										m_err = m_err || TryFillConstants(plan2, ARRAYSIZE(plan2), tmp, m_metas, response, imgNum, &m_comMetas[2], qualityOffset);
										m_err = m_err || ComputeSpecular(tmp[6], tmp[7], &tmp[5], response, imgNum, 7, maxIOR, maxEC);
									}
									else
									{
										m_err = m_err || TryFillConstants(plan1, ARRAYSIZE(plan1), tmp, m_metas, response, imgNum, &m_comMetas[1], qualityOffset);
										m_err = m_err || TryFillConstants(plan3, ARRAYSIZE(plan3), tmp, m_metas, response, imgNum, &m_comMetas[0], qualityOffset);
										m_comMetas[2] = m_comMetas[0];
										m_err = m_err || ComputeSpecular(tmp[0], tmp[2], &tmp[5], response, imgNum, 1);
									}
								}
								m_comMetas[3] = m_metas[8];

								Engine::TextureLoader::Image textures[3];
								m_err = m_err || ComputeAlbedo(tmp[0], tmp[2], &textures[0], response, imgNum, 1);
								m_err = m_err || ComputeNormalMap(tmp[1], tmp[9], tmp[3], &textures[1], img.NormalMapFlags, response, imgNum, 2);
								m_err = m_err || CombineImages(tmp[5], tmp[4], API::Image::v1008::ColorChannel::Channel_Color, API::Image::v1008::ColorChannel::Channel_1, API::Image::v1008::ColorChannel::Channel_Color, API::Image::v1008::ColorChannel::Channel_Alpha, &textures[2], response, imgNum, 6);

								if (m_err == false)
								{
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[0], m_comMetas[0].Encoding, &m_textures[0]);
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[1], m_comMetas[1].Encoding, &m_textures[1]);
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[2], m_comMetas[2].Encoding, &m_textures[2]);
									if (m_useDisplacement) { CalculateMipMaps(img.MaxMipMapCount, qualityOffset, tmp[8], m_comMetas[3].Encoding, &m_textures[3]); }
									for (size_t a = 0; a < ARRAYSIZE(m_textures); a++) { m_layerCount = max(m_layerCount, m_textures[a].levelCount); }

									for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
									{
										if (m_textures[a].levelCount <= qualityOffset + 3u && (m_textures[a].levelCount != 1 || m_textures[a].levels[0].Size.W != 1 || m_textures[a].levels[0].Size.H != 1))
										{
											size_t texNum = 0;
											if (a == 1) { texNum = 1; }
											else if (a == 2) { texNum = 5; }
											else if (a == 3) { texNum = 8; }
											SendResponse(response, API::Image::v1008::RespondeState::ERR_QUALITY, imgNum, "Image is not large enought to support this quality setting", texNum + 1);
											m_err = true;
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
								}
								break;
							}
							case CloakCompiler::API::Image::v1008::MaterialType::AlphaMapped:
							{
								Engine::TextureLoader::Image tmp[ARRAYSIZE(m_metas)];
								Engine::TextureLoader::Image textures[3];
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.Albedo, response, imgNum, 1, &tmp[0], &m_metas[0]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.NormalMap, response, imgNum, 2, &tmp[1], &m_metas[1]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.Metallic, response, imgNum, 3, &tmp[2], &m_metas[2]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.AmbientOcclusion, response, imgNum, 4, &tmp[3], &m_metas[3]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.RoughnessMap, response, imgNum, 5, &tmp[4], &m_metas[4]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.SpecularMap, response, imgNum, 6, &tmp[5], &m_metas[5]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.IndexOfRefractionMap, response, imgNum, 7, &tmp[6], &m_metas[6]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.ExtinctionCoefficientMap, response, imgNum, 8, &tmp[7], &m_metas[7]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.DisplacementMap, response, imgNum, 9, &tmp[8], &m_metas[8]);
								m_err = m_err || TryLoadTexture(img.AlphaMapped.Textures.PorosityMap, response, imgNum, 10, &tmp[9], &m_metas[9]);

								m_useDisplacement = img.AlphaMapped.Textures.DisplacementMap.Use;
								if (m_err == false)
								{
									float maxIOR = img.AlphaMapped.Textures.HighestIndexOfRefraction;
									float maxEC = img.AlphaMapped.Textures.HighestExtinctionCoefficient;
									if (tmp[6].dataSize == 0)
									{
										maxIOR = 0;
										for (size_t a = 0; a < 3; a++) { maxIOR = max(maxIOR, img.AlphaMapped.Constants.Fresnel[a].RefractionIndex); }
									}
									if (tmp[7].dataSize == 0)
									{
										maxEC = 0;
										for (size_t a = 0; a < 3; a++) { maxEC = max(maxEC, img.AlphaMapped.Constants.Fresnel[a].ExtinctionCoefficient); }
									}

									HRESULT hRet = Engine::ImageBuilder::BuildImage(img.AlphaMapped.Textures.AlphaMap, &tmp[10], &m_metas[10].Builder);
									if (FAILED(hRet)) { SendResponse(response, API::Image::v1008::RespondeState::ERR_IMG_BUILDING, imgNum, "Couldn't build image", 10); m_err = true; }

									m_fillPlan[0].Constant = img.AlphaMapped.Constants.Albedo;
									m_fillPlan[1].Constant = CloakEngine::Helper::Color::RGBA(0.5f, 0.5f, 1.0f, 0);
									m_fillPlan[2].Constant = CloakEngine::Helper::Color::RGBA(img.AlphaMapped.Constants.Metallic, 0, 0, 0);
									m_fillPlan[3].Constant = CloakEngine::Helper::Color::RGBA(1, 0, 0, 0);
									m_fillPlan[4].Constant = CloakEngine::Helper::Color::RGBA(img.AlphaMapped.Constants.Roughness, 0, 0, 0);
									m_fillPlan[5].Constant = CloakEngine::Helper::Color::RGBA(0, 0, 0, 0);
									m_fillPlan[6].Constant = CloakEngine::Helper::Color::RGBA(img.AlphaMapped.Constants.Fresnel[0].RefractionIndex / maxIOR, img.AlphaMapped.Constants.Fresnel[1].RefractionIndex / maxIOR, img.AlphaMapped.Constants.Fresnel[2].RefractionIndex / maxIOR, 0);
									m_fillPlan[7].Constant = CloakEngine::Helper::Color::RGBA(img.AlphaMapped.Constants.Fresnel[0].ExtinctionCoefficient / maxEC, img.AlphaMapped.Constants.Fresnel[1].ExtinctionCoefficient / maxEC, img.AlphaMapped.Constants.Fresnel[2].ExtinctionCoefficient / maxEC, 0);
									m_fillPlan[8].Constant = CloakEngine::Helper::Color::RGBA(0.5f, 0, 0, 0);
									m_fillPlan[9].Constant = CloakEngine::Helper::Color::RGBA(img.AlphaMapped.Constants.Porosity, 0, 0, 0);
									m_fillPlan[10].Constant = CloakEngine::Helper::Color::RGBA(1, 0, 0, 0);
									for (size_t a = 0; a < ARRAYSIZE(m_metas); a++) { m_fillPlan[a].img = a; }
									FillPlan* plan0[] = { &m_fillPlan[0],&m_fillPlan[2],&m_fillPlan[10] };
									FillPlan* plan1[] = { &m_fillPlan[1],&m_fillPlan[9],&m_fillPlan[3] };
									FillPlan* plan2[] = { &m_fillPlan[4],&m_fillPlan[5],&m_fillPlan[6],&m_fillPlan[7] };
									FillPlan* plan3[] = { &m_fillPlan[0],&m_fillPlan[2],&m_fillPlan[4],&m_fillPlan[5],&m_fillPlan[10] };

									if (img.AlphaMapped.Textures.SpecularMap.Use == false)
									{
										if (img.AlphaMapped.Textures.IndexOfRefractionMap.Use || img.AlphaMapped.Textures.ExtinctionCoefficientMap.Use)
										{
											m_err = m_err || TryFillConstants(plan0, ARRAYSIZE(plan0), tmp, m_metas, response, imgNum, &m_comMetas[0], qualityOffset);
											m_err = m_err || TryFillConstants(plan1, ARRAYSIZE(plan1), tmp, m_metas, response, imgNum, &m_comMetas[1], qualityOffset);
											m_err = m_err || TryFillConstants(plan2, ARRAYSIZE(plan2), tmp, m_metas, response, imgNum, &m_comMetas[2], qualityOffset);
											m_err = m_err || ComputeSpecular(tmp[6], tmp[7], &tmp[5], response, imgNum, 7, maxIOR, maxEC);
										}
										else
										{
											m_err = m_err || TryFillConstants(plan1, ARRAYSIZE(plan1), tmp, m_metas, response, imgNum, &m_comMetas[1], qualityOffset);
											m_err = m_err || TryFillConstants(plan3, ARRAYSIZE(plan3), tmp, m_metas, response, imgNum, &m_comMetas[0], qualityOffset);
											m_comMetas[2] = m_comMetas[0];
											m_err = m_err || ComputeSpecular(tmp[0], tmp[2], &tmp[5], response, imgNum, 1);
										}
									}
									m_comMetas[3] = m_metas[8];

									Engine::TextureLoader::Image tmpAlbedo;
									m_err = m_err || CombineImages(tmp[0], tmp[10], API::Image::v1008::ColorChannel::Channel_Color, API::Image::v1008::ColorChannel::Channel_1, API::Image::v1008::ColorChannel::Channel_Color, API::Image::v1008::ColorChannel::Channel_Alpha, &tmpAlbedo, response, imgNum, 1);
									m_err = m_err || ComputeAlbedo(tmpAlbedo, tmp[2], &textures[0], response, imgNum, 1);
									m_err = m_err || ComputeNormalMap(tmp[1], tmp[9], tmp[3], &textures[1], img.NormalMapFlags, response, imgNum, 2);
									m_err = m_err || CombineImages(tmp[5], tmp[4], API::Image::v1008::ColorChannel::Channel_Color, API::Image::v1008::ColorChannel::Channel_1, API::Image::v1008::ColorChannel::Channel_Color, API::Image::v1008::ColorChannel::Channel_Alpha, &textures[2], response, imgNum, 6);
								}
								if (m_err == false)
								{
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[0], m_comMetas[0].Encoding, &m_textures[0]);
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[1], m_comMetas[1].Encoding, &m_textures[1]);
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[2], m_comMetas[2].Encoding, &m_textures[2]);
									if (m_useDisplacement) { CalculateMipMaps(img.MaxMipMapCount, qualityOffset, tmp[8], m_comMetas[3].Encoding, &m_textures[3]); }
									for (size_t a = 0; a < ARRAYSIZE(m_textures); a++) { m_layerCount = max(m_layerCount, m_textures[a].levelCount); }

									for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
									{
										if (m_textures[a].levelCount <= qualityOffset + 3u && (m_textures[a].levelCount != 1 || m_textures[a].levels[0].Size.W != 1 || m_textures[a].levels[0].Size.H != 1))
										{
											size_t texNum = 0;
											if (a == 1) { texNum = 1; }
											else if (a == 2) { texNum = 5; }
											else if (a == 3) { texNum = 8; }
											SendResponse(response, API::Image::v1008::RespondeState::ERR_QUALITY, imgNum, "Image is not large enought to support this quality setting", texNum + 1);
											m_err = true;
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
								}
								break;
							}
							case CloakCompiler::API::Image::v1008::MaterialType::Transparent:
							{
								Engine::TextureLoader::Image tmp[ARRAYSIZE(m_metas)];
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.Albedo, response, imgNum, 1, &tmp[0], &m_metas[0]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.NormalMap, response, imgNum, 2, &tmp[1], &m_metas[1]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.Metallic, response, imgNum, 3, &tmp[2], &m_metas[2]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.AmbientOcclusion, response, imgNum, 4, &tmp[3], &m_metas[3]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.RoughnessMap, response, imgNum, 5, &tmp[4], &m_metas[4]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.SpecularMap, response, imgNum, 6, &tmp[5], &m_metas[5]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.IndexOfRefractionMap, response, imgNum, 7, &tmp[6], &m_metas[6]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.ExtinctionCoefficientMap, response, imgNum, 8, &tmp[7], &m_metas[7]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.DisplacementMap, response, imgNum, 9, &tmp[8], &m_metas[8]);
								m_err = m_err || TryLoadTexture(img.Transparent.Textures.PorosityMap, response, imgNum, 10, &tmp[9], &m_metas[9]);

								m_useDisplacement = img.Transparent.Textures.DisplacementMap.Use;
								float maxIOR = img.Transparent.Textures.HighestIndexOfRefraction;
								float maxEC = img.Transparent.Textures.HighestExtinctionCoefficient;
								if (tmp[6].dataSize == 0)
								{
									maxIOR = 0;
									for (size_t a = 0; a < 3; a++) { maxIOR = max(maxIOR, img.Transparent.Constants.Fresnel[a].RefractionIndex); }
								}
								if (tmp[7].dataSize == 0)
								{
									maxEC = 0;
									for (size_t a = 0; a < 3; a++) { maxEC = max(maxEC, img.Transparent.Constants.Fresnel[a].ExtinctionCoefficient); }
								}

								m_fillPlan[0].Constant = img.Transparent.Constants.Albedo;
								m_fillPlan[1].Constant = CloakEngine::Helper::Color::RGBA(0.5f, 0.5f, 1.0f, 0);
								m_fillPlan[2].Constant = CloakEngine::Helper::Color::RGBA(img.Transparent.Constants.Metallic, 0, 0, 0);
								m_fillPlan[3].Constant = CloakEngine::Helper::Color::RGBA(1, 0, 0, 0);
								m_fillPlan[4].Constant = CloakEngine::Helper::Color::RGBA(img.Transparent.Constants.Roughness, 0, 0, 0);
								m_fillPlan[5].Constant = CloakEngine::Helper::Color::RGBA(0, 0, 0, 0);
								m_fillPlan[6].Constant = CloakEngine::Helper::Color::RGBA(img.Transparent.Constants.Fresnel[0].RefractionIndex / maxIOR, img.Transparent.Constants.Fresnel[1].RefractionIndex / maxIOR, img.Transparent.Constants.Fresnel[2].RefractionIndex / maxIOR, 0);
								m_fillPlan[7].Constant = CloakEngine::Helper::Color::RGBA(img.Transparent.Constants.Fresnel[0].ExtinctionCoefficient / maxEC, img.Transparent.Constants.Fresnel[1].ExtinctionCoefficient / maxEC, img.Transparent.Constants.Fresnel[2].ExtinctionCoefficient / maxEC, 0);
								m_fillPlan[8].Constant = CloakEngine::Helper::Color::RGBA(0.5f, 0, 0, 0);
								m_fillPlan[9].Constant = CloakEngine::Helper::Color::RGBA(img.Transparent.Constants.Porosity, 0, 0, 0);
								for (size_t a = 0; a < ARRAYSIZE(m_metas); a++) { m_fillPlan[a].img = a; }
								FillPlan* plan0[] = { &m_fillPlan[0],&m_fillPlan[2] };
								FillPlan* plan1[] = { &m_fillPlan[1],&m_fillPlan[9],&m_fillPlan[3] };
								FillPlan* plan2[] = { &m_fillPlan[4],&m_fillPlan[5],&m_fillPlan[6],&m_fillPlan[7] };
								FillPlan* plan3[] = { &m_fillPlan[0],&m_fillPlan[2],&m_fillPlan[4],&m_fillPlan[5] };

								if (img.Transparent.Textures.SpecularMap.Use == false)
								{
									if (img.Transparent.Textures.IndexOfRefractionMap.Use || img.Transparent.Textures.ExtinctionCoefficientMap.Use)
									{
										m_err = m_err || TryFillConstants(plan0, ARRAYSIZE(plan0), tmp, m_metas, response, imgNum, &m_comMetas[0], qualityOffset);
										m_err = m_err || TryFillConstants(plan1, ARRAYSIZE(plan1), tmp, m_metas, response, imgNum, &m_comMetas[1], qualityOffset);
										m_err = m_err || TryFillConstants(plan2, ARRAYSIZE(plan2), tmp, m_metas, response, imgNum, &m_comMetas[2], qualityOffset);
										m_err = m_err || ComputeSpecular(tmp[6], tmp[7], &tmp[5], response, imgNum, 7, maxIOR, maxEC);
									}
									else
									{
										m_err = m_err || TryFillConstants(plan1, ARRAYSIZE(plan1), tmp, m_metas, response, imgNum, &m_comMetas[1], qualityOffset);
										m_err = m_err || TryFillConstants(plan3, ARRAYSIZE(plan3), tmp, m_metas, response, imgNum, &m_comMetas[0], qualityOffset);
										m_comMetas[2] = m_comMetas[0];
										m_err = m_err || ComputeSpecular(tmp[0], tmp[2], &tmp[5], response, imgNum, 1);
									}
								}
								m_comMetas[3] = m_metas[8];

								Engine::TextureLoader::Image textures[3];
								m_err = m_err || ComputeAlbedo(tmp[0], tmp[2], &textures[0], response, imgNum, 1);
								m_err = m_err || ComputeNormalMap(tmp[1], tmp[9], tmp[3], &textures[1], img.NormalMapFlags, response, imgNum, 2);
								m_err = m_err || CombineImages(tmp[5], tmp[4], API::Image::v1008::ColorChannel::Channel_Color, API::Image::v1008::ColorChannel::Channel_1, API::Image::v1008::ColorChannel::Channel_Color, API::Image::v1008::ColorChannel::Channel_Alpha, &textures[2], response, imgNum, 6);

								if (m_err == false)
								{
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[0], m_comMetas[0].Encoding, &m_textures[0]);
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[1], m_comMetas[1].Encoding, &m_textures[1]);
									CalculateMipMaps(img.MaxMipMapCount, qualityOffset, textures[2], m_comMetas[2].Encoding, &m_textures[2]);
									if (m_useDisplacement) { CalculateMipMaps(img.MaxMipMapCount, qualityOffset, tmp[8], m_comMetas[3].Encoding, &m_textures[3]); }
									for (size_t a = 0; a < ARRAYSIZE(m_textures); a++) { m_layerCount = max(m_layerCount, m_textures[a].levelCount); }

									for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
									{
										if (m_textures[a].levelCount <= qualityOffset + 3u && (m_textures[a].levelCount != 1 || m_textures[a].levels[0].Size.W != 1 || m_textures[a].levels[0].Size.H != 1))
										{
											size_t texNum = 0;
											if (a == 1) { texNum = 1; }
											else if (a == 2) { texNum = 5; }
											else if (a == 3) { texNum = 8; }
											SendResponse(response, API::Image::v1008::RespondeState::ERR_QUALITY, imgNum, "Image is not large enought to support this quality setting", texNum + 1);
											m_err = true;
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
								}
								break;
							}
							default:
								break;
						}
					}
					CLOAK_CALL EntryMaterial::~EntryMaterial()
					{
						DeleteArray(m_layers);
					}
					bool CLOAK_CALL_THIS EntryMaterial::GotError() const { return m_err; }
					void CLOAK_CALL_THIS EntryMaterial::CheckTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
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
							if (suc) { suc = (static_cast<API::Image::ImageInfoType>(read->ReadBits(3)) == API::Image::ImageInfoType::MATERIAL); }
							if (suc) { suc = (static_cast<API::Image::v1008::Heap>(read->ReadBits(2)) == m_heap); }
							if (suc) { suc = (static_cast<uint8_t>(read->ReadBits(2)) == m_qualityOffset); }
							if (suc) { suc = (static_cast<API::Image::v1008::MaterialType>(read->ReadBits(2)) == m_type); }
							if (suc) { suc = ((read->ReadBits(1) == 1) == m_useDisplacement); }
							if (suc) { suc = (static_cast<API::Image::v1008::NORMAL_FLAGS>(read->ReadBits(8)) == m_normFlags); }
							for (size_t a = 0; a < ARRAYSIZE(m_metas) && suc; a++)
							{
								std::vector<Engine::TempHandler::FileDateInfo> dates;
								Engine::TempHandler::ReadDates(m_metas[a].Builder.CheckPaths, &dates);
								suc = !Engine::TempHandler::CheckDateChanges(dates, read);
								if (suc) { suc = (read->ReadBits(8) == m_metas[a].Builder.BasicFlags); }
								if (suc) { suc = (read->ReadDynamic() == m_metas[a].Encoding.BlockSize); }
								if (suc) { suc = (static_cast<API::Image::v1008::ColorSpace>(read->ReadBits(3)) == m_metas[a].Encoding.Space); }
								for (size_t b = 0; b < 4 && suc; b++) { suc = (read->ReadDouble(32) == m_fillPlan[a].Constant[b]); }
							}
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement) && suc; a++)
							{
								suc = (static_cast<size_t>(read->ReadBits(10)) == m_textures[a].levelCount);
							}

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
					void CLOAK_CALL_THIS EntryMaterial::WriteTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
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
							write->WriteBits(3, static_cast<uint8_t>(API::Image::ImageInfoType::MATERIAL));
							write->WriteBits(2, static_cast<uint8_t>(m_heap));
							write->WriteBits(2, m_qualityOffset);
							write->WriteBits(2, static_cast<uint8_t>(m_type));
							write->WriteBits(1, m_useDisplacement ? 1 : 0);
							write->WriteBits(8, static_cast<uint8_t>(m_normFlags));
							for (size_t a = 0; a < ARRAYSIZE(m_metas); a++)
							{
								std::vector<Engine::TempHandler::FileDateInfo> dates;
								Engine::TempHandler::ReadDates(m_metas[a].Builder.CheckPaths, &dates);
								Engine::TempHandler::WriteDates(dates, write);
								write->WriteBits(8, m_metas[a].Builder.BasicFlags);
								write->WriteDynamic(m_metas[a].Encoding.BlockSize);
								write->WriteBits(3, static_cast<uint8_t>(m_metas[a].Encoding.Space));
								for (size_t b = 0; b < 4; b++) { write->WriteDouble(32, m_fillPlan[a].Constant[b]); }
							}
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
							{
								write->WriteBits(10, m_textures[a].levelCount);
							}

							for (size_t a = 0; a < m_layerCount; a++)
							{
								write->WriteDynamic(m_layers[a].ByteLength);
								write->WriteDynamic(m_layers[a].BitLength);
								write->WriteBuffer(m_layers[a].Buffer, m_layers[a].ByteLength, m_layers[a].BitLength);
							}

							SAVE_RELEASE(write);
						}
					}
					void CLOAK_CALL_THIS EntryMaterial::Encode(In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum)
					{
						if (m_useTemp == false && m_err == false)
						{
							//Encode Header:
							SendResponse(response, API::Image::v1008::RespondeState::START_IMG_HEADER, imgNum);
							CloakEngine::Files::IWriter* write = nullptr;
							CREATE_INTERFACE(CE_QUERY_ARGS(&write));
							write->SetTarget(m_layers[0].Buffer);

							write->WriteBits(3, 4); //Material type
							if (m_type == API::Image::v1008::MaterialType::Opaque) { write->WriteBits(2, 0); }
							else if (m_type == API::Image::v1008::MaterialType::AlphaMapped) { write->WriteBits(2, 1); }
							else if (m_type == API::Image::v1008::MaterialType::Transparent) { write->WriteBits(2, 2); }
							write->WriteBits(2, m_qualityOffset);
							write->WriteBits(1, m_useDisplacement ? 1 : 0);
							if (EncodeHeap(write, m_heap) == false) { SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "Unknown heap type"); m_err = true; }

							bool constants[4];
							bool hr[4];
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
							{
								API::Image::v1008::ColorChannel channel = API::Image::v1008::ColorChannel::Channel_All;
								if (m_type == API::Image::v1008::MaterialType::Opaque && a == 0) { channel = API::Image::v1008::ColorChannel::Channel_Color; }
								else if (a == 3) { channel = API::Image::v1008::ColorChannel::Channel_1; }
								hr[a] = CheckHR(m_textures[a], channel);
							}
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
							{
								if (EncodeMeta(write, m_comMetas[a], hr[a]) == false) { SendResponse(response, API::Image::v1008::RespondeState::ERR_ILLEGAL_PARAM, imgNum, "Unknown color space"); m_err = true; }
								if (m_textures[a].levelCount == 1 && m_textures[a].levels[0].Size.W == 1 && m_textures[a].levels[0].Size.H == 1)
								{
									constants[a] = true;
									write->WriteBits(1, 1);
								}
								else
								{
									constants[a] = false;
									write->WriteBits(1, 0);
									write->WriteBits(10, m_textures[a].levelCount);
									write->WriteDynamic(m_textures[a].levels[0].Size.W);
									write->WriteDynamic(m_textures[a].levels[0].Size.H);
								}
							}
							for (size_t a = 0; a < 4 && (a < 3 || m_useDisplacement); a++)
							{
								if (constants[a] == true)
								{
									size_t texNum = 0;
									if (a == 1) { texNum = 1; }
									else if (a == 2) { texNum = 5; }
									else if (a == 3) { texNum = 8; }
									SendResponse(response, API::Image::v1008::RespondeState::WRITE_CONSTANT_HEADER, imgNum, texNum + 1);
									API::Image::v1008::ColorChannel channel = API::Image::v1008::ColorChannel::Channel_All;
									if (m_type == API::Image::v1008::MaterialType::Opaque && a == 0) { channel = API::Image::v1008::ColorChannel::Channel_Color; }
									else if (a == 3) { channel = API::Image::v1008::ColorChannel::Channel_1; }
									m_err = m_err || EncodeTexture(write, bitInfos, m_textures[a].levels[0], m_comMetas[a], response, imgNum, a, channel, hr[a], nullptr, nullptr);
								}
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
								for (size_t b = 0; b < 4 && (b < 3 || m_useDisplacement); b++)
								{
									if (m_textures[b].levelCount >= a && constants[b] == false)
									{
										MipMapLevel* prevMip = nullptr;
										if (m_textures[b].levelCount > a) { prevMip = &m_textures[b].levels[a]; }
										API::Image::v1008::ColorChannel channel = API::Image::v1008::ColorChannel::Channel_All;
										if (m_type == API::Image::v1008::MaterialType::Opaque && b == 0) { channel = API::Image::v1008::ColorChannel::Channel_Color; }
										else if (b == 3) { channel = API::Image::v1008::ColorChannel::Channel_1; }
										m_err = m_err || EncodeTexture(write, bitInfos, m_textures[b].levels[a - 1], m_comMetas[b], response, imgNum, b, channel, hr[a], nullptr, prevMip);
									}
								}
								m_layers[a].BitLength = write->GetBitPosition();
								m_layers[a].ByteLength = write->GetPosition();
								write->Save();
							}
							SendResponse(response, API::Image::v1008::RespondeState::DONE_IMG_SUB, imgNum);
							SAVE_RELEASE(write);
						}
					}
					void CLOAK_CALL_THIS EntryMaterial::WriteHeader(In const CE::RefPointer<CloakEngine::Files::IWriter>& write)
					{
						if (m_err == false)
						{
							write->WriteBuffer(m_layers[0].Buffer, m_layers[0].ByteLength, m_layers[0].BitLength);
						}
					}
					size_t CLOAK_CALL_THIS EntryMaterial::GetMaxLayerLength()
					{
						if (m_err == false) { return m_layerCount - (1 + m_qualityOffset); }
						return 0;
					}
					void CLOAK_CALL_THIS EntryMaterial::WriteLayer(In const CE::RefPointer<CloakEngine::Files::IWriter>& write, In size_t layer)
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
					void CLOAK_CALL_THIS EntryMaterial::WriteLibFiles(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const API::Image::LibInfo& lib, In const BitInfo& bits, In size_t entryID, In const Engine::Lib::CMakeFile& cmake, In const CE::Global::Task& finishTask)
					{

					}
				}
			}
		}
	}
}