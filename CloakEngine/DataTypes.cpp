#include "stdafx.h"
#include "CloakEngine/Rendering/DataTypes.h"

namespace CloakEngine {
	namespace API {
		namespace Rendering {
			CLOAK_CALL FLOAT2::FLOAT2() { X = 0; Y = 0; }
			CLOAK_CALL FLOAT2::FLOAT2(In FLOAT1 x, In_opt FLOAT1 y) { X = x; Y = y; }
			CLOAK_CALL FLOAT2::FLOAT2(In const API::Global::Math::Space2D::Vector& v) { X = v.X; Y = v.Y; }
			CLOAK_CALL FLOAT2::FLOAT2(In const API::Global::Math::Space2D::Point& v) { X = v.X; Y = v.Y; }
			CLOAK_CALL FLOAT2::FLOAT2(In const API::Global::Math::Vector& v) : FLOAT2((API::Global::Math::Point)v) {}
			CLOAK_CALL FLOAT2::FLOAT2(In const API::Global::Math::Point& v) { X = v.X; Y = v.Y; }
			FLOAT2& CLOAK_CALL FLOAT2::operator=(In const API::Global::Math::Space2D::Vector& v) { X = v.X; Y = v.Y; return *this; }
			FLOAT2& CLOAK_CALL FLOAT2::operator=(In const API::Global::Math::Space2D::Point& v) { X = v.X; Y = v.Y; return *this; }
			FLOAT2& CLOAK_CALL FLOAT2::operator=(In const API::Global::Math::Vector& v) { return *this = (API::Global::Math::Point)v; }
			FLOAT2& CLOAK_CALL FLOAT2::operator=(In const API::Global::Math::Point& v) { X = v.X; Y = v.Y; return *this; }
			FLOAT1& CLOAK_CALL FLOAT2::operator[](In size_t a) { assert(a < 2); switch (a) { case 0: return X; default: return Y; } }

			CLOAK_CALL FLOAT3::FLOAT3() { X = 0; Y = 0; Z = 0; }
			CLOAK_CALL FLOAT3::FLOAT3(In FLOAT1 x, In_opt FLOAT1 y, In_opt FLOAT1 z) { X = x; Y = y; Z = z; }
			CLOAK_CALL FLOAT3::FLOAT3(In const API::Global::Math::Space2D::Vector& v) { X = v.X; Y = v.Y; Z = v.A; }
			CLOAK_CALL FLOAT3::FLOAT3(In const API::Global::Math::Vector& v) : FLOAT3((API::Global::Math::Point)v) {}
			CLOAK_CALL FLOAT3::FLOAT3(In const API::Global::Math::Point& v) { X = v.X; Y = v.Y; Z = v.Z; }
			CLOAK_CALL FLOAT3::FLOAT3(In const API::Helper::Color::RGBX& c) { X = c.R; Y = c.G; Z = c.B; }
			FLOAT3& CLOAK_CALL FLOAT3::operator=(In const API::Global::Math::Space2D::Vector& v) { X = v.X; Y = v.Y; Z = v.A; return *this; }
			FLOAT3& CLOAK_CALL FLOAT3::operator=(In const API::Global::Math::Vector& v) { return *this = (API::Global::Math::Point)v; }
			FLOAT3& CLOAK_CALL FLOAT3::operator=(In const API::Global::Math::Point& v) { X = v.X; Y = v.Y; Z = v.Z; return *this; }
			FLOAT3& CLOAK_CALL FLOAT3::operator=(In const API::Helper::Color::RGBX& c) { X = c.R; Y = c.G; Z = c.B; return *this; }
			FLOAT1& CLOAK_CALL FLOAT3::operator[](In size_t a) { assert(a < 3); switch (a) { case 0: return X; case 1: return Y; default: return Z; } }

			CLOAK_CALL FLOAT4::FLOAT4() { X = 0; Y = 0; Z = 0; W = 0; }
			CLOAK_CALL FLOAT4::FLOAT4(In FLOAT1 x, In_opt FLOAT1 y, In_opt FLOAT1 z, In_opt FLOAT1 w) { X = x; Y = y; Z = z; W = w; }
			CLOAK_CALL FLOAT4::FLOAT4(In const API::Global::Math::Vector& v) : FLOAT4((API::Global::Math::Point)v) {}
			CLOAK_CALL FLOAT4::FLOAT4(In const API::Global::Math::Point& v) { X = v.X; Y = v.Y; Z = v.Z; W = v.W; }
			CLOAK_CALL FLOAT4::FLOAT4(In const API::Helper::Color::RGBX& c) { X = c.R; Y = c.G; Z = c.B; W = 1; }
			CLOAK_CALL FLOAT4::FLOAT4(In const API::Helper::Color::RGBA& c) { X = c.R; Y = c.G; Z = c.B; W = c.A; }
			FLOAT4& CLOAK_CALL FLOAT4::operator=(In const API::Global::Math::Vector& v) { return *this = (API::Global::Math::Point)v; }
			FLOAT4& CLOAK_CALL FLOAT4::operator=(In const API::Global::Math::Point& v) { X = v.X; Y = v.Y; Z = v.Z; W = v.W; return *this; }
			FLOAT4& CLOAK_CALL FLOAT4::operator=(In const API::Helper::Color::RGBX& c) { X = c.R; Y = c.G; Z = c.B; W = 1; return *this; }
			FLOAT4& CLOAK_CALL FLOAT4::operator=(In const API::Helper::Color::RGBA& c) { X = c.R; Y = c.G; Z = c.B; W = c.A; return *this; }
			FLOAT1& CLOAK_CALL FLOAT4::operator[](In size_t a) { assert(a < 4); switch (a) { case 0: return X; case 1: return Y; case 2: return Z; default: return W; } }

			CLOAK_CALL UINT2::UINT2() { X = 0; Y = 0; }
			CLOAK_CALL UINT2::UINT2(In UINT1 x, In_opt UINT1 y) { X = x; Y = y; }

			CLOAK_CALL UINT3::UINT3() { X = 0; Y = 0; Z = 0; }
			CLOAK_CALL UINT3::UINT3(In UINT1 x, In_opt UINT1 y, In_opt UINT1 z) { X = x; Y = y; Z = z; }

			CLOAK_CALL UINT4::UINT4() { X = 0; Y = 0; Z = 0; W = 0; }
			CLOAK_CALL UINT4::UINT4(In UINT1 x, In_opt UINT1 y, In_opt UINT1 z, In_opt UINT1 w) { X = x; Y = y; Z = z; W = w; }

			CLOAK_CALL INT2::INT2() { X = 0; Y = 0; }
			CLOAK_CALL INT2::INT2(In INT1 x, In_opt INT1 y) { X = x; Y = y; }

			CLOAK_CALL INT3::INT3() { X = 0; Y = 0; Z = 0; }
			CLOAK_CALL INT3::INT3(In INT1 x, In_opt INT1 y, In_opt INT1 z) { X = x; Y = y; Z = z; }

			CLOAK_CALL INT4::INT4() { X = 0; Y = 0; Z = 0; W = 0; }
			CLOAK_CALL INT4::INT4(In INT1 x, In_opt INT1 y, In_opt INT1 z, In_opt INT1 w) { X = x; Y = y; Z = z; W = w; }

			CLOAK_CALL MATRIX::MATRIX() {}
			CLOAK_CALL MATRIX::MATRIX(In FLOAT1(&data)[16])
			{
				for (size_t a = 0; a < 16; a++) { Data[a] = data[a]; }
			}
			CLOAK_CALL MATRIX::MATRIX(In FLOAT4 r0, In FLOAT4 r1, In FLOAT4 r2, In FLOAT4 r3)
			{
				for (size_t a = 0; a < 4; a++) { Data[ 0 + a] = r0[a]; }
				for (size_t a = 0; a < 4; a++) { Data[ 4 + a] = r1[a]; }
				for (size_t a = 0; a < 4; a++) { Data[ 8 + a] = r2[a]; }
				for (size_t a = 0; a < 4; a++) { Data[12 + a] = r3[a]; }
			}
			CLOAK_CALL MATRIX::MATRIX(In const API::Global::Math::Matrix& m)
			{
				for (size_t a = 0; a < 4; a++) { _mm_store_ps(&Data[a << 2], m.Row[a]); }
			}
			MATRIX& CLOAK_CALL_THIS MATRIX::operator=(In const MATRIX& o)
			{
				for (size_t a = 0; a < ARRAYSIZE(Data); a++) { Data[a] = o.Data[a]; }
				return *this;
			}
			MATRIX& CLOAK_CALL_THIS MATRIX::operator=(In const API::Global::Math::Matrix& m)
			{
				for (size_t a = 0; a < 4; a++) { _mm_store_ps(&Data[a << 2], m.Row[a]); }
				return *this;
			}
		}
	}
}