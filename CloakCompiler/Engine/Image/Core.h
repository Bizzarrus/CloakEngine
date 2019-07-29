#pragma once
#ifndef CC_E_IMAGE_CORE_H
#define CC_E_IMAGE_CORE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Compressor.h"
#include "CloakEngine/Files/ExtendedBuffers.h"

#include "CloakCompiler/EncoderBase.h"
#include "CloakCompiler/Image.h"

#include "Engine/ImageBuilder.h"

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			inline namespace v1008 {
				uint8_t CLOAK_CALL BitLen(In uint64_t val);

				struct Dimensions {
					UINT W;
					UINT H;
					CLOAK_CALL Dimensions() { W = 0; H = 0; }
					CLOAK_CALL Dimensions(In UINT w, In UINT h) { W = w; H = h; }
					CLOAK_CALL Dimensions(In const Dimensions& o) { W = o.W; H = o.H; }
					size_t CLOAK_CALL GetSize() const { return static_cast<size_t>(W)*H; }
				};
				struct MipMapLevel {
					CloakEngine::Helper::Color::RGBA* Color = nullptr;
					Dimensions Size;
					API::Image::v1008::BuildMeta Meta;
					CLOAK_CALL MipMapLevel();
					CLOAK_CALL MipMapLevel(In const MipMapLevel& m);
					CLOAK_CALL ~MipMapLevel();

					void CLOAK_CALL_THIS Allocate(In UINT w, In UINT h);
					void CLOAK_CALL_THIS Set(In UINT x, In UINT y, In const CloakEngine::Helper::Color::RGBA& c);
					const CloakEngine::Helper::Color::RGBA& CLOAK_CALL_THIS Get(In UINT x, In UINT y) const;
					CloakEngine::Helper::Color::RGBA& CLOAK_CALL_THIS Get(In UINT x, In UINT y);
					const CloakEngine::Helper::Color::RGBA& CLOAK_CALL_THIS operator[](In UINT p) const;
					CloakEngine::Helper::Color::RGBA& CLOAK_CALL_THIS operator[](In UINT p);
					MipMapLevel& CLOAK_CALL_THIS operator=(In const MipMapLevel& m);
				};
				struct MipTexture {
					//level[0] is the biggest mip map, level[levelCount-1] the smallest
					MipMapLevel* levels = nullptr;
					size_t levelCount = 0;
					CLOAK_CALL ~MipTexture();
				};

				typedef int64_t BitColorData;
				struct BitInfo {
					struct SingleInfo {
						uint8_t Bits = 0;
						uint8_t Len = 0;
						BitColorData MaxVal = 0;
						void CLOAK_CALL_THIS operator=(In uint8_t bits);
					} Channel[4];
					BitInfo& CLOAK_CALL_THIS operator=(In const BitInfo& bi);
				};
				struct BitInfoList {
					BitInfo RGBA;
					BitInfo RGBE;
					BitInfo YCCA;
					BitInfo Material;
				};
				struct BitColor {
					BitColorData Data[4];
					BitColorData& CLOAK_CALL_THIS operator[](In size_t s);
					const BitColorData& CLOAK_CALL_THIS operator[](In size_t s) const;
				};
				struct BitTexture {
					Dimensions Size;
					BitColor* Data = nullptr;

					CLOAK_CALL BitTexture(In const MipMapLevel& texture, In const BitInfo& bitInfo);
					CLOAK_CALL BitTexture(In const BitTexture& t);
					CLOAK_CALL ~BitTexture();
					BitColor* CLOAK_CALL_THIS operator[](In UINT y);
					BitColor* CLOAK_CALL_THIS at(In UINT y);
					const BitColor* CLOAK_CALL_THIS operator[](In UINT y) const;
					const BitColor* CLOAK_CALL_THIS at(In UINT y) const;
					BitTexture& CLOAK_CALL_THIS operator=(In const BitTexture& t);
				};
				struct TextureMeta {
					Engine::ImageBuilder::BuildMeta Builder;
					API::Image::v1008::BuildMeta Encoding;
				};

				struct LayerBuffer {
					CloakEngine::Files::IVirtualWriteBuffer* Buffer;
					unsigned long long ByteLength;
					unsigned int BitLength;
					virtual CLOAK_CALL ~LayerBuffer();
				};

				class ResponseCaller {
					private:
						const API::Image::v1008::ResponseFunc m_func;
						CE::RefPointer<CE::Helper::ISyncSection> m_sync = nullptr;
					public:
						explicit CLOAK_CALL ResponseCaller(In const API::Image::v1008::ResponseFunc& func);
						void CLOAK_CALL_THIS Call(const API::Image::v1008::RespondeInfo& ri) const;
				};

				namespace Cube {
					constexpr size_t SIDE_FRONT = 4;
					constexpr size_t SIDE_BACK = 5;
					constexpr size_t SIDE_LEFT = 1;
					constexpr size_t SIDE_RIGHT = 0;
					constexpr size_t SIDE_TOP = 2;
					constexpr size_t SIDE_BOTTOM = 3;
					constexpr size_t CUBE_ORDER[] = { SIDE_FRONT,SIDE_BACK,SIDE_LEFT,SIDE_RIGHT,SIDE_TOP,SIDE_BOTTOM };

					bool CLOAK_CALL HDRIToCube(In const Engine::TextureLoader::Image& hdri, In uint32_t cubeSize, Out Engine::TextureLoader::Image sides[6]);
				}

				void CLOAK_CALL SendResponse(In const ResponseCaller& response, In API::Image::v1008::RespondeState state, In_opt size_t imgNum = 0, In_opt std::string msg = "", In_opt size_t imgSubNum = 0);
				void CLOAK_CALL SendResponse(In const ResponseCaller& response, In API::Image::v1008::RespondeState state, In size_t imgNum, In size_t imgSubNum);
				void CLOAK_CALL SendResponse(In const ResponseCaller& response, In API::Image::v1008::RespondeState state, In const std::string& msg);
				bool CLOAK_CALL EncodeColorSpace(In CloakEngine::Files::IWriter* write, In const API::Image::v1008::BuildMeta& meta);
				bool CLOAK_CALL EncodeMeta(In CloakEngine::Files::IWriter* write, In const TextureMeta& meta, In bool hr);
				bool CLOAK_CALL EncodeHeap(In CloakEngine::Files::IWriter* write, In API::Image::v1008::Heap heap);
				void CLOAK_CALL ConvertToYCCA(Inout MipMapLevel* tex);
				bool CLOAK_CALL CanColorConvert(In API::Image::v1008::ColorSpace space, In API::Image::v1008::ColorChannel channel);
			}
		}
	}
}

#endif