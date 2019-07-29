#pragma once
#ifndef CC_E_IMAGE_TEXTUREENCODING_H
#define CC_E_IMAGE_TEXTUREENCODING_H

#include "Engine/Image/Core.h"

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			inline namespace v1008 {
				namespace Lib {
					struct FormatTypeName {
						const char* Name;
						size_t MinMemorySize;
						size_t MaxMemorySize;
						bool HDR;
					};
					constexpr FormatTypeName FORMAT_TYPE_NAMES[] = { {"FLOAT", 16, ~0, true}, {"UNORM", 0, 16, false} };

					bool CLOAK_CALL EncodeTexture(In const CE::RefPointer<CE::Files::IWriter> file, In const MipTexture& texture, In API::Image::v1008::ColorChannel channel, In const BitInfo& bits, In bool writeMipMaps, In size_t entryID, In size_t textureID, In size_t tabCount);
					bool CLOAK_CALL FitsOneFile(In size_t textureCount, In_reads(textureCount) const MipTexture* textures, In API::Image::v1008::ColorChannel channel, In const BitInfo& bits, In bool writeMipMaps);
				}

				bool CLOAK_CALL EncodeTexture(In CloakEngine::Files::IWriter* write, In const BitInfo& bits, In const MipMapLevel& texture, In const ResponseCaller& response, In size_t imgNum, In size_t texNum, In API::Image::v1008::ColorChannel channels, In uint16_t blockSize, In bool hr, In_opt const MipMapLevel* prevTex = nullptr, In_opt const MipMapLevel* prevMip = nullptr);
				bool CLOAK_CALL EncodeTexture(In CloakEngine::Files::IWriter* write, In const BitInfoList& bitInfo, In const MipMapLevel& texture, In const TextureMeta& meta, In const ResponseCaller& response, In size_t imgNum, In size_t texNum, In API::Image::v1008::ColorChannel channels, In bool hr, In_opt const MipMapLevel* prevTex = nullptr, In_opt const MipMapLevel* prevMip = nullptr);
				void CLOAK_CALL EncodeAlphaMap(In CloakEngine::Files::IWriter* write, In const MipMapLevel& texture);
				bool CLOAK_CALL CheckHR(In const MipTexture& tex, In API::Image::ColorChannel channels);
			}
		}
	}
}

#endif