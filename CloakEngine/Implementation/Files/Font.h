#pragma once
#ifndef CE_IMPL_FILES_FONT_H
#define CE_IMPL_FILES_FONT_H

#include "CloakEngine/Files/Font.h"
#include "CloakEngine/Global/Graphic.h"

#include "Implementation/Rendering/ColorBuffer.h"
#include "Implementation/Files/FileHandler.h"

#include "Engine/LinkedList.h"

#include <unordered_map>

#define GET_FONT_SIZE(size) (size/1000.0f)

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Font_v2 {
				class CLOAK_UUID("{CB3F7D4A-6FD9-4D91-8667-C1ED87F9483C}") Font : public virtual API::Files::Font_v2::IFont, public virtual FileHandler_v1::RessourceHandler {
					public:
						CLOAK_CALL_THIS Font();
						virtual CLOAK_CALL_THIS ~Font();
						virtual API::Files::Font_v2::LetterInfo CLOAK_CALL_THIS GetLetterInfo(In char32_t c) const override;
						virtual float CLOAK_CALL_THIS getLineHeight(In float lineSpace) const override;
						virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetTexture() const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual LoadResult CLOAK_CALL_THIS iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) override;
						virtual void CLOAK_CALL_THIS iUnload() override;

						API::FlatMap<char32_t, API::Files::Font_v2::LetterInfo> m_fontInfo;
						Impl::Rendering::IColorBuffer* m_texture;
						float m_spaceWidth;
				};
				DECLARE_FILE_HANDLER(Font, "{DDC10208-34B6-45D6-AD72-10F46EADC6DF}");
			}
		}
	}
}

#pragma warning(pop)

#endif