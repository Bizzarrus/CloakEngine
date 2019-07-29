#pragma once
#ifndef CC_API_IMAGE_H
#define CC_API_IMAGE_H

#include "CloakCompiler/Defines.h"
#include "CloakCompiler/EncoderBase.h"
#include "CloakEngine/CloakEngine.h"

#include <string>
#include <functional>

namespace CloakCompiler {
	CLOAKCOMPILER_API_NAMESPACE namespace API {
		namespace Image {
			enum class SizeFlag { DYNAMIC = 0, FIXED = 1 };
			enum class ImageInfoType { LIST = 0x0, ARRAY = 0x1, CUBE = 0x2, ANIMATION = 0x3, MATERIAL = 0x4, SKYBOX = 0x5 };
			enum class BuildType {NORMAL,BORDERED};
			enum class WriteMode {
				None = 0,
				File = 1 << 0,
				Lib = 1 << 1,
			};
			DEFINE_ENUM_FLAG_OPERATORS(WriteMode);
			struct IMAGE_BUILD_NORMAL {
				std::u16string Path = u"";
			};
			struct IMAGE_BUILD_BORDERED {
				std::u16string CornerTopLeft = u"";
				std::u16string CornerTopRight = u"";
				std::u16string CornerBottomLeft = u"";
				std::u16string CornerBottomRight = u"";
				std::u16string SideLeft = u"";
				std::u16string SideRight = u"";
				std::u16string SideTop = u"";
				std::u16string SideBottom = u"";
				std::u16string Center = u"";
			};
			inline namespace v1008 {
				constexpr CloakEngine::Files::FileType FileType{ "InteliImage","CEII",1014 };
				constexpr uint32_t GENERATE_ALL_MIP_MAPS = 1023;
				constexpr uint32_t GENERATE_NO_MIP_MAPS = 0;

				enum class ColorChannel {
					Channel_1 = 0x1,
					Channel_2 = 0x2,
					Channel_3 = 0x4,
					Channel_4 = 0x8,
					Channel_None = 0x0,
					Channel_All = 0xF,
					Channel_Color = 0x7,
					Channel_Alpha = Channel_4,
				};
				DEFINE_ENUM_FLAG_OPERATORS(ColorChannel);
				enum class ColorSpace { RGBA, YCCA, Material };
				enum class SupportedQuality { LOW, MEDIUM, HIGH, ULTRA };
				enum class Heap { GUI, WORLD, UTILITY };
				enum class CubeType { HDRI, CUBE };
				enum class RespondeState
				{
					ERR_COLOR_BITS,
					ERR_NO_IMGS,
					ERR_ILLEGAL_PARAM,
					ERR_IMG_BUILDING,
					ERR_IMG_SIZE,
					ERR_QUALITY,
					ERR_BLOCKSIZE,
					ERR_COLOR_CHANNELS,
					ERR_CHANNEL_MISMATCH,
					ERR_BLOCKMETA_MISMATCH,
					ERR_GPU_INIT,

					WARN_SHOWTIME,

					CHECK_TEMP,
					WRITE_TEMP,

					START_IMG_HEADER,
					START_IMG_SUB,

					WRITE_CONSTANT_HEADER,
					WRITE_UPDATE_INFO,

					INFO,

					DONE_IMG_HEADER,
					DONE_IMG_SUB,
					DONE_FILE,
				};
				struct RespondeInfo {
					RespondeState State;
					size_t ImgNum;
					size_t ImgSubNum;
					std::string Msg = "";
				};
				enum class MaterialType {
					Opaque,
					AlphaMapped,
					Transparent,
				};
				enum NORMAL_FLAGS {
					NORMAL_FLAG_NONE = 0x00,
					NORMAL_FLAG_X_LEFT = 0x01,
					NORMAL_FLAG_X_RIGHT = 0x00,
					NORMAL_FLAG_Y_TOP = 0x00,
					NORMAL_FLAG_Y_BOTTOM = 0x02,
					NORMAL_FLAG_Z_FULL_RANGE = 0x00,
					NORMAL_FLAG_Z_HALF_RANGE = 0x04,
					NORMAL_FLAG_DEFAULT = NORMAL_FLAG_X_RIGHT | NORMAL_FLAG_Y_TOP | NORMAL_FLAG_Z_FULL_RANGE,
				};
				DEFINE_ENUM_FLAG_OPERATORS(NORMAL_FLAGS);
				struct FresnelInfo {
					float RefractionIndex;
					float ExtinctionCoefficient;
				};
				struct BuildMeta {
					BuildType Type = BuildType::NORMAL;
					uint16_t BlockSize = 0;
					ColorSpace Space = ColorSpace::RGBA;
				};
				struct BuildInfo {
					BuildMeta Meta;
					IMAGE_BUILD_NORMAL Normal;
					IMAGE_BUILD_BORDERED Bordered;
				};
				struct AnimationInfo : public BuildInfo {
					CloakEngine::Global::Time ShowTime = 0;
				};
				struct OptionalInfo : public BuildInfo {
					bool Use = true;
				};
				/*
				PBR Formulars:

				Fresnel Schlick approximation:
					F_Schlick(e, h) = c_spec + (1 - c_spec) * ((1 - dot(e, h)) ^ 5)
					with	h = halfway vector (v + l) / 2
							v = View direction
							l = Light direction
							e = either v or l

				Specular Color (offline computation):
					c_spec = ((n - 1) ^ 2) / (((n + 1) ^ 2) + (k ^ 2))
					with	n = Index of Refraction
							k = Extinction coefficient

				Microfacet BRDF:
					f(l, v) = (F(l,h) * G(l, v, h) * D(h)) / (4 * dot(l, n) * dot(n, v))
					with	l = Light Direction
							h = Half way vector
							n = Surface normal
							v = View direction
							F = Fresnell approximation (per color value)
							G = Geometry Factor (scalar value)
							D = Normal Distribution (scalar value)
							See http://blog.selfshadow.com/publications/s2015-shading-course/hoffman/s2015_pbs_physics_math_slides.pdf for G and D functions
							See also https://mediatech.aalto.fi/~jaakko/T111-5310/K2013\03_RenderingEquation.pdf

				Lambert diffuse:
					f = (1 - m) * c_albedo / pi
					with	m = Metallic factor [0 - 1]

				Result = ((k_s * f_BRDF) + (k_d * f_Diffuse)) * c_light * dot(l, n)
					with	k_s = F (see Microfacet BRDF)
							k_d = (1 - k_s)

					See https://learnopengl.com/#!PBR/Theory


				*/
				/*
				Texture Layout:
				[	R	G	B	A	]	Albedo (pre-multiplied with metallic factor)		=> f_Diffuse
				[	Nx	Ny	P	AO	]	Normal Map X and Y + Porosity + Ambient Occlusion shadowing factor
				[	Sr	Sg	Sb	Rg	]	Specular Color + Roughness							=> c_spec | Rg
				[	D				]	Displacement
				*/
				struct MATERIAL_OPAQUE {
					struct {
						OptionalInfo Albedo;
						OptionalInfo NormalMap;
						OptionalInfo Metallic;
						OptionalInfo AmbientOcclusion;
						OptionalInfo RoughnessMap;
						OptionalInfo SpecularMap;
						OptionalInfo IndexOfRefractionMap;
						OptionalInfo ExtinctionCoefficientMap;
						OptionalInfo DisplacementMap;
						OptionalInfo PorosityMap;
						float HighestIndexOfRefraction = 0;
						float HighestExtinctionCoefficient = 0;
					} Textures;
					struct {
						CloakEngine::Helper::Color::RGBX Albedo;
						float Roughness;
						float Metallic;
						FresnelInfo Fresnel[3];
						float Porosity;
					} Constants;
				};
				struct MATERIAL_ALPHAMAP {
					struct {
						OptionalInfo Albedo;
						OptionalInfo NormalMap;
						OptionalInfo Metallic;
						OptionalInfo AmbientOcclusion;
						OptionalInfo RoughnessMap;
						OptionalInfo SpecularMap;
						OptionalInfo IndexOfRefractionMap;
						OptionalInfo ExtinctionCoefficientMap;
						OptionalInfo DisplacementMap;
						OptionalInfo PorosityMap;
						float HighestIndexOfRefraction = 0;
						float HighestExtinctionCoefficient = 0;

						BuildInfo AlphaMap;
					} Textures;
					struct {
						CloakEngine::Helper::Color::RGBX Albedo;
						float Roughness;
						float Metallic;
						FresnelInfo Fresnel[3];
						float Porosity;
					} Constants;
				};
				struct MATERIAL_TRANSPARENT {
					struct {
						OptionalInfo Albedo;
						OptionalInfo NormalMap;
						OptionalInfo Metallic;
						OptionalInfo AmbientOcclusion;
						OptionalInfo RoughnessMap;
						OptionalInfo SpecularMap;
						OptionalInfo IndexOfRefractionMap;
						OptionalInfo ExtinctionCoefficientMap;
						OptionalInfo DisplacementMap;
						OptionalInfo PorosityMap;
						float HighestIndexOfRefraction = 0;
						float HighestExtinctionCoefficient = 0;
					} Textures;
					struct {
						CloakEngine::Helper::Color::RGBA Albedo;
						float Roughness;
						float Metallic;
						FresnelInfo Fresnel[3];
						float Porosity;
					} Constants;
				};
				struct IMAGE_INFO_LIST {
					CloakEngine::List<BuildInfo> Images;
					bool AlphaMap = false;
					uint32_t MaxMipMapCount = GENERATE_ALL_MIP_MAPS;
					SizeFlag Flags = SizeFlag::FIXED;
					ColorChannel EncodingChannels = ColorChannel::Channel_All;
				};
				struct IMAGE_INFO_ARRAY {
					CloakEngine::List<BuildInfo> Images;
					uint32_t MaxMipMapCount = GENERATE_ALL_MIP_MAPS;
					ColorChannel EncodingChannels = ColorChannel::Channel_All;
				};
				struct IMAGE_INFO_CUBE {
					CubeType Type;
					struct {
						BuildInfo Image; // HDR longtitude - latitude image
						uint32_t CubeSize;
					} HDRI;
					struct {
						BuildInfo Right; // Face 4
						BuildInfo Left; // Face 2 
						BuildInfo Top; // Face 1 
						BuildInfo Bottom; // Face 3
						BuildInfo Front; // Face 0
						BuildInfo Back; // Face 5
					} Cube;
					uint32_t MaxMipMapCount = GENERATE_ALL_MIP_MAPS;
					ColorChannel EncodingChannels = ColorChannel::Channel_Color;
				};
				struct IMAGE_INFO_SKYBOX : public IMAGE_INFO_CUBE {
					struct {
						struct {
							struct {
								uint32_t CubeSize;
								uint32_t NumSamples;
								uint16_t BlockSize;
								uint32_t MaxMipMapCount = GENERATE_ALL_MIP_MAPS;
							} Color;
							struct {
								uint32_t NumSamples;
								uint32_t RoughnessSamples;
								uint32_t CosNVSamples;
								uint16_t BlockSize;
							} LUT;
						} Radiance;
						struct {
							
						} Irradiance;
					} IBL;
				};
				struct IMAGE_INFO_ANIMATION {
					CloakEngine::List<AnimationInfo> Images;
					bool AlphaMap = false;
					uint32_t MaxMipMapCount = GENERATE_ALL_MIP_MAPS;
					ColorChannel EncodingChannels = ColorChannel::Channel_All;
				};
				struct IMAGE_INFO_MATERIAL {
					MaterialType Type;
					uint32_t MaxMipMapCount = GENERATE_ALL_MIP_MAPS;
					NORMAL_FLAGS NormalMapFlags = NORMAL_FLAG_DEFAULT;
					MATERIAL_OPAQUE Opaque;
					MATERIAL_ALPHAMAP AlphaMapped;
					MATERIAL_TRANSPARENT Transparent;
					//See https://www.marmoset.co/posts/basic-theory-of-physically-based-rendering/ for PBR hints
				};
				struct ImageInfo {
					std::string Name;
					ImageInfoType Type = ImageInfoType::LIST;
					SupportedQuality SupportedQuality = SupportedQuality::LOW;
					Heap Heap = Heap::GUI;
					IMAGE_INFO_LIST List;
					IMAGE_INFO_ARRAY Array;
					IMAGE_INFO_CUBE Cube;
					IMAGE_INFO_ANIMATION Animation;
					IMAGE_INFO_MATERIAL Material;
					IMAGE_INFO_SKYBOX Skybox;
				};

				struct FileInfo {
					union {
						struct {
							uint8_t R;
							uint8_t G;
							uint8_t B;
							uint8_t A;
						};
						uint8_t Bits[4];
					} RGBA;
					union {
						struct {
							uint8_t R;
							uint8_t G;
							uint8_t B;
							uint8_t Exponent;
						};
						uint8_t Bits[4];
					} RGBE;
					union {
						struct {
							uint8_t Y;
							uint8_t C;
							uint8_t A;
						};
						uint8_t Bits[3];
					} YCCA;
					union {
						struct {
							uint8_t A;
						};
						uint8_t Bits[1];
					} Material;
					bool NoBackgroundLoading = false;
				};
				struct LibInfo {
					std::string Name;
					CE::Files::UFI Path;
					LibGenerator Generator;
					bool WriteNamespaceAlias;
					bool SharedLib;
					bool WriteMipMaps;
				};
				struct Desc {
					WriteMode WriteMode;
					CloakEngine::List<ImageInfo> Imgs;
					FileInfo File;
					LibInfo Lib;
				};
				typedef std::function<void(const RespondeInfo&)> ResponseFunc;
				CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In ResponseFunc response = false);
			}
		}
	}
}

#endif