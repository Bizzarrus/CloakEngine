#pragma once
#ifndef CC_E_CODECS_HDRI_H
#define CC_E_CODECS_HDRI_H

#include "CloakCompiler/Defines.h"
#include "CloakEngine/CloakEngine.h"
#include "Engine/TextureLoader.h"

namespace CloakCompiler {
	namespace Engine {
		namespace Codecs {
			class HDRI : public TextureLoader::ICodec {
				public:
					CLOAK_CALL HDRI();
					CLOAK_CALL ~HDRI();
					HRESULT CLOAK_CALL_THIS Decode(In const CloakEngine::Files::UFI& path, Out TextureLoader::Image* img) override;
				private:
					HRESULT CLOAK_CALL_THIS ReadHeader(In CloakEngine::Files::IReader* read);
					HRESULT CLOAK_CALL_THIS ReadData(In CloakEngine::Files::IReader* read, Out TextureLoader::Image* img);

					size_t m_format;
					float m_exposure;
					float m_gamma;
					UINT m_H;
					UINT m_W;
					bool m_nd[3];
			};
		}
	}
}

#endif