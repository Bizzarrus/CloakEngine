#pragma once
#ifndef CC_E_TEXTURELOADER_H
#define CC_E_TEXTURELOADER_H

#include "CloakCompiler/Defines.h"

#include "CloakEngine/Helper/Color.h"
#include "CloakEngine/Files/Buffer.h"

#include <string>

namespace CloakCompiler {
	namespace Engine {
		namespace TextureLoader {
			struct Image {
				CloakEngine::Helper::Color::RGBA* data = nullptr;
				size_t dataSize = 0;
				UINT width = 0;
				UINT height = 0;
				CLOAK_CALL ~Image();
			};
			CLOAK_INTERFACE_BASIC ICodec {
				public:
					virtual HRESULT CLOAK_CALL_THIS Decode(In const CloakEngine::Files::UFI& path, Out TextureLoader::Image* img) = 0;
			};
			HRESULT CLOAK_CALL LoadTextureFromFile(std::u16string filePath, Image* img);
		}
	}
}

#endif
