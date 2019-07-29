#pragma once
#ifndef CE_API_GLOBAL_DRAW_H
#define CE_API_GLOBAL_DRAW_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Helper/Color.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Draw {
				struct GUIData {
					Math::Point Position;
					Math::Point Size;
					Helper::Color::RGBA ColorFilter;
					struct {
						float Left;
						float Right;
						float Top;
						float Bottom;
					} BorderWidth;
					UINT32 UseGraysave;
					UINT32 UseTexture;
					UINT32 TextureID;
				};
				struct WorldData {
					Math::Vector Normal;
					Math::Vector Binormal;
					Math::Vector Tangent;
					Math::Vector Position;
					Math::Point TextureCoord;
				};

			}
		}
	}
}

#endif