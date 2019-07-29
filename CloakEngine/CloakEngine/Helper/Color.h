#pragma once
#ifndef CE_API_HELPER_COLOR_H
#define CE_API_HELPER_COLOR_H

#include "CloakEngine/Defines.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Helper {
			namespace Color {
				struct RGBA;
				struct RGBX {
					float R = 0;
					float G = 0;
					float B = 0;
					CLOAKENGINE_API CLOAK_CALL_THIS RGBX();
					CLOAKENGINE_API CLOAK_CALL_THIS RGBX(In float r, In float g, In float b);
					CLOAKENGINE_API CLOAK_CALL_THIS RGBX(In const RGBA& rgba);
					CLOAKENGINE_API CLOAK_CALL_THIS operator RGBA();
					CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS operator+(In const RGBX& b) const;
					CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS operator-(In const RGBX& b) const;
					CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS operator*(In const RGBX& b) const;
					CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS operator/(In const RGBX& b) const;
					CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS operator*(In float x) const;
					CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS operator/(In float x) const;
					CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS operator+=(In const RGBX& b);
					CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS operator-=(In const RGBX& b);
					CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS operator*=(In const RGBX& b);
					CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS operator/=(In const RGBX& b);
					CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS operator=(In const RGBX& b);
					CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS operator*=(In float x);
					CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS operator/=(In float x);
					CLOAKENGINE_API inline float CLOAK_CALL_THIS operator[](In size_t p) const;
					CLOAKENGINE_API inline float& CLOAK_CALL_THIS operator[](In size_t p);
				};
				struct RGBA {
					float R = 0;
					float G = 0;
					float B = 0;
					float A = 0;
					CLOAKENGINE_API CLOAK_CALL_THIS RGBA();
					CLOAKENGINE_API CLOAK_CALL_THIS RGBA(In float r, In float g, In float b, In float a);
					CLOAKENGINE_API CLOAK_CALL_THIS RGBA(In const RGBX& rgbx);
					CLOAKENGINE_API explicit CLOAK_CALL_THIS operator RGBX();
					CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS operator+(In const RGBA& b) const;
					CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS operator-(In const RGBA& b) const;
					CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS operator*(In const RGBA& b) const;
					CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS operator/(In const RGBA& b) const;
					CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS operator*(In float x) const;
					CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS operator/(In float x) const;
					CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS operator+=(In const RGBA& b);
					CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS operator-=(In const RGBA& b);
					CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS operator*=(In const RGBA& b);
					CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS operator/=(In const RGBA& b);
					CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS operator=(In const RGBX& b);
					CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS operator=(In const RGBA& b);
					CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS operator*=(In float x);
					CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS operator/=(In float x);
					CLOAKENGINE_API inline float CLOAK_CALL_THIS operator[](In size_t p) const;
					CLOAKENGINE_API inline float& CLOAK_CALL_THIS operator[](In size_t p);
				};

				CLOAKENGINE_API inline RGBX CLOAK_CALL operator*(In const float a, In const RGBX& b) { return b * a; }
				CLOAKENGINE_API inline RGBA CLOAK_CALL operator*(In const float a, In const RGBA& b) { return b * a; }

				const RGBX Red(1, 0, 0);
				const RGBX Blue(0, 0, 1);
				const RGBX Green(0, 1, 0);
				const RGBX Black(0, 0, 0);
				const RGBX White(1, 1, 1);
				const RGBX Yellow(1, 1, 0);
				const RGBX Turquoise(0, 1, 1);
				const RGBX Violett(1, 0, 1);
				const RGBX Gray(0.7f, 0.7f, 0.7f);
				const RGBX DarkGray(0.3f, 0.3f, 0.3f);
			}
		}
	}
}

#endif