#include "stdafx.h"
#include "CloakEngine/Helper/Color.h"

namespace CloakEngine {
	namespace API {
		namespace Helper {
			namespace Color {
				CLOAKENGINE_API CLOAK_CALL_THIS RGBX::RGBX() : RGBX(0, 0, 0) {}
				CLOAKENGINE_API CLOAK_CALL_THIS RGBX::RGBX(In float r, In float g, In float b)
				{
					R = r;
					G = g;
					B = b;
				}
				CLOAKENGINE_API CLOAK_CALL_THIS RGBX::RGBX(In const RGBA& rgba) : RGBX(rgba.R, rgba.G, rgba.B) {}
				CLOAKENGINE_API CLOAK_CALL_THIS RGBX::operator RGBA() { return RGBA(R, G, B, 1.0f); }
				CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS RGBX::operator+(In const RGBX& b) const { return RGBX(R + b.R, G + b.G, B + b.B); }
				CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS RGBX::operator-(In const RGBX& b) const { return RGBX(R - b.R, G - b.G, B - b.B); }
				CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS RGBX::operator*(In const RGBX& b) const { return RGBX(R * b.R, G * b.G, B * b.B); }
				CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS RGBX::operator/(In const RGBX& b) const { return RGBX(R / b.R, G / b.G, B / b.B); }
				CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS RGBX::operator*(In float x) const { return RGBX(R*x, G*x, B*x); }
				CLOAKENGINE_API inline RGBX CLOAK_CALL_THIS RGBX::operator/(In float x) const { return RGBX(R / x, G / x, B / x); }
				CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS RGBX::operator+=(In const RGBX& b)
				{
					R += b.R;
					G += b.G;
					B += b.B;
					return *this;
				}
				CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS RGBX::operator-=(In const RGBX& b)
				{
					R -= b.R;
					G -= b.G;
					B -= b.B;
					return *this;
				}
				CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS RGBX::operator*=(In const RGBX& b)
				{
					R *= b.R;
					G *= b.G;
					B *= b.B;
					return *this;
				}
				CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS RGBX::operator/=(In const RGBX& b)
				{
					R /= b.R;
					G /= b.G;
					B /= b.B;
					return *this;
				}
				CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS RGBX::operator=(In const RGBX& b)
				{
					R = b.R;
					G = b.G;
					B = b.B;
					return *this;
				}
				CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS RGBX::operator*=(In float x)
				{
					R *= x;
					G *= x;
					B *= x;
					return *this;
				}
				CLOAKENGINE_API inline RGBX& CLOAK_CALL_THIS RGBX::operator/=(In float x)
				{
					R /= x;
					G /= x;
					B /= x;
					return *this;
				}
				CLOAKENGINE_API inline float CLOAK_CALL_THIS RGBX::operator[](In size_t p) const
				{
					if (p == 0) { return R; }
					if (p == 1) { return G; }
					if (p == 2) { return B; }
					return 0;
				}
				CLOAKENGINE_API inline float& CLOAK_CALL_THIS RGBX::operator[](In size_t p)
				{
					if (p == 0) { return R; }
					if (p == 1) { return G; }
					return B;
				}


				CLOAKENGINE_API CLOAK_CALL_THIS RGBA::RGBA() : RGBA(0, 0, 0, 0) {}
				CLOAKENGINE_API CLOAK_CALL_THIS RGBA::RGBA(In float r, In float g, In float b, In float a)
				{
					R = r;
					G = g;
					B = b;
					A = a;
				}
				CLOAKENGINE_API CLOAK_CALL_THIS RGBA::RGBA(In const RGBX& rgbx) : RGBA(rgbx.R, rgbx.G, rgbx.B, 1) {}
				CLOAKENGINE_API CLOAK_CALL_THIS RGBA::operator RGBX() { return RGBX(R, G, B); }
				CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS RGBA::operator+(In const RGBA& b) const { return RGBA(R + b.R, G + b.G, B + b.B, A + b.A); }
				CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS RGBA::operator-(In const RGBA& b) const { return RGBA(R - b.R, G - b.G, B - b.B, A - b.A); }
				CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS RGBA::operator*(In const RGBA& b) const { return RGBA(R * b.R, G * b.G, B * b.B, A * b.A); }
				CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS RGBA::operator/(In const RGBA& b) const { return RGBA(R / b.R, G / b.G, B / b.B, A / b.A); }
				CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS RGBA::operator*(In float x) const { return RGBA(R*x, G*x, B*x, A*x); }
				CLOAKENGINE_API inline RGBA CLOAK_CALL_THIS RGBA::operator/(In float x) const { return RGBA(R / x, G / x, B / x, A / x); }
				CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS RGBA::operator+=(In const RGBA& b)
				{
					R += b.R;
					G += b.G;
					B += b.B;
					A += b.A;
					return *this;
				}
				CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS RGBA::operator-=(In const RGBA& b)
				{
					R -= b.R;
					G -= b.G;
					B -= b.B;
					A -= b.A;
					return *this;
				}
				CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS RGBA::operator*=(In const RGBA& b)
				{
					R *= b.R;
					G *= b.G;
					B *= b.B;
					A *= b.A;
					return *this;
				}
				CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS RGBA::operator/=(In const RGBA& b)
				{
					R /= b.R;
					G /= b.G;
					B /= b.B;
					A /= b.A;
					return *this;
				}
				CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS RGBA::operator=(In const RGBX& b)
				{
					R = b.R;
					G = b.G;
					B = b.B;
					A = 1.0f;
					return *this;
				}
				CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS RGBA::operator=(In const RGBA& b)
				{
					R = b.R;
					G = b.G;
					B = b.B;
					A = b.A;
					return *this;
				}
				CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS RGBA::operator*=(In float x)
				{
					R *= x;
					G *= x;
					B *= x;
					A *= x;
					return *this;
				}
				CLOAKENGINE_API inline RGBA& CLOAK_CALL_THIS RGBA::operator/=(In float x)
				{
					R /= x;
					G /= x;
					B /= x;
					A /= x;
					return *this;
				}
				CLOAKENGINE_API inline float CLOAK_CALL_THIS RGBA::operator[](In size_t p) const
				{
					if (p == 0) { return R; }
					if (p == 1) { return G; }
					if (p == 2) { return B; }
					if (p == 3) { return A; }
					return 0;
				}
				CLOAKENGINE_API inline float& CLOAK_CALL_THIS RGBA::operator[](In size_t p)
				{
					if (p == 0) { return R; }
					if (p == 1) { return G; }
					if (p == 2) { return B; }
					return A;
				}
			}
		}
	}
}