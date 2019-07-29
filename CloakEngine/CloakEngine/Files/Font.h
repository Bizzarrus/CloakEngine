#pragma once
#ifndef CE_API_FILES_FONT_H
#define CE_API_FILES_FONT_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/FileHandler.h"
#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Global/Math.h"

#include <string>
#include <functional>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Font_v2 {
				enum class LetterType { SINGLECOLOR, MULTICOLOR, TRUECOLOR, SPACE };
				struct LetterInfo {
					float Width;
					LetterType Type;
					API::Global::Math::Space2D::Point TopLeft;
					API::Global::Math::Space2D::Point BottomRight;
				};
				CLOAK_INTERFACE_ID("{E72E8675-85C6-4F97-AF4A-105B08EE6733}") IFont : public virtual Files::FileHandler_v1::IFileHandler{
					public:
						virtual LetterInfo CLOAK_CALL_THIS GetLetterInfo(In char32_t c) const = 0;
						virtual float CLOAK_CALL_THIS getLineHeight(In float lineSpace) const = 0;
						virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS GetTexture() const = 0;
				};
			}
		}
	}
}

#endif