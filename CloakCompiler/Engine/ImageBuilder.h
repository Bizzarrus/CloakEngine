#pragma once
#ifndef CC_E_IMAGEBUILDER_H
#define CC_E_IMAGEBUILDER_H

#include "CloakCompiler/Defines.h"
#include "CloakEngine/CloakEngine.h"
#include "CloakCompiler/Image.h"
#include "Engine/TextureLoader.h"

#include <vector>

namespace CloakCompiler {
	namespace Engine {
		namespace ImageBuilder {
			struct BuildMeta {
				std::vector<std::u16string> CheckPaths;
				uint8_t BasicFlags;
			};
			HRESULT CLOAK_CALL BuildImage(In const API::Image::v1008::BuildInfo& info, Out Engine::TextureLoader::Image* img, Out BuildMeta* meta);
			HRESULT CLOAK_CALL MetaFromInfo(In const API::Image::v1008::BuildInfo& info, Out BuildMeta* meta);
		}
	}
}

#endif