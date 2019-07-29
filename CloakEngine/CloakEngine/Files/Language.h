#pragma once
#ifndef CE_API_FILES_LANGUAGE_H
#define CE_API_FILES_LANGUAGE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/FileHandler.h"
#include "CloakEngine/Helper/Color.h"
#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Files/Font.h"

#include <string>

#define INVALID_STRING_ENTRY static_cast<size_t>(-1)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Language_v2 {
				typedef size_t TextID;
				typedef size_t LanguageID;
				typedef float FontSize;

				enum class Justify { LEFT, CENTER, RIGHT };
				enum class AnchorPoint { TOP, CENTER, BOTTOM };
				struct TextDesc {
					Justify Justify;
					AnchorPoint Anchor;
					FontSize FontSize;
					API::Global::Math::Space2D::Point TopLeft;
					API::Global::Math::Space2D::Point BottomRight;
					float LetterSpace = 0;
					float LineSpace = 0;
					API::Helper::Color::RGBA Color[3];
					bool Grayscale = false;
				};

				CLOAK_INTERFACE_ID("{5F54FC24-83CC-4621-9E7C-D90B968CF60A}") IString : public virtual FileHandler_v1::IFileHandler{
					public:
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const std::u16string& text) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const std::string& text) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In float data) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In unsigned int data) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In int data) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In IString* link) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In const API::Helper::Color::RGBA& color1) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t default1) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t channel1, In_range(0, 2) uint8_t channel2, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2, In const API::Helper::Color::RGBA& color3) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In const API::Helper::Color::RGBA& color2, In_range(0, 2) uint8_t default3) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2, In const API::Helper::Color::RGBA& color3) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In const API::Helper::Color::RGBA& color1, In_range(0, 2) uint8_t default2, In_range(0, 2) uint8_t default3) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2, In const API::Helper::Color::RGBA& color3) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t default1, In const API::Helper::Color::RGBA& color2, In_range(0, 2) uint8_t default3) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2, In const API::Helper::Color::RGBA& color3) = 0;
						virtual void CLOAK_CALL_THIS Set(In size_t entry, In_range(0, 2) uint8_t default1, In_range(0, 2) uint8_t default2, In_range(0, 2) uint8_t default3) = 0;
						virtual void CLOAK_CALL_THIS SetDefaultColor(In size_t entry) = 0;
						virtual void CLOAK_CALL_THIS SetLineBreak(In size_t entry) = 0;
						virtual void CLOAK_CALL_THIS Draw(In API::Rendering::IContext2D* context, In API::Rendering::IDrawable2D* drawing, In API::Files::Font_v2::IFont* font, In const TextDesc& desc, In_opt UINT Z = 0, In_opt bool supressLineBreaks = false) = 0;
						virtual std::u16string CLOAK_CALL_THIS GetAsString() const = 0;
				};
				CLOAK_INTERFACE_ID("{4FD5B56C-8446-4A8D-8CBA-D190CB09459D}") ILanguage : public virtual FileHandler_v1::IFileHandler{
					public:
						virtual void CLOAK_CALL_THIS SetLanguage(In LanguageID language) = 0;
						virtual IString* CLOAK_CALL_THIS GetString(In size_t pos) = 0;
				};
			}
		}
	}
}

#endif