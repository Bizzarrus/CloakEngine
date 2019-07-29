#pragma once
#ifndef CC_E_IMAGE_MIPMAPGENERATION_H
#define CC_E_IMAGE_MIPMAPGENERATION_H

#include "Engine/Image/Core.h"

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			inline namespace v1008 {
				void CLOAK_CALL CreateMipMapLevels(In uint32_t mipMapCount, In uint8_t qualityOffset, Inout MipTexture* mips, In UINT W, In UINT H, In const API::Image::v1008::BuildMeta& meta);
				void CLOAK_CALL CalculateMipMaps(In uint32_t mipMapCount, In uint8_t qualityOffset, In const Engine::TextureLoader::Image& img, In const API::Image::v1008::BuildMeta& meta, Inout MipTexture* mips);
			}
		}
	}
}

#endif