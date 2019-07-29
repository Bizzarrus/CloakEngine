#pragma once
#ifndef CE_API_RENDERING_DATATYPES_H
#define CE_API_RENDERING_DATATYPES_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Helper/Color.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			typedef float FLOAT1;
			struct CLOAKENGINE_API FLOAT2 {
				FLOAT1 X;
				FLOAT1 Y;
				CLOAK_CALL FLOAT2();
				CLOAK_CALL FLOAT2(In FLOAT1 x, In_opt FLOAT1 y = 0);
				CLOAK_CALL FLOAT2(In const API::Global::Math::Space2D::Vector& v);
				CLOAK_CALL FLOAT2(In const API::Global::Math::Space2D::Point& v);
				CLOAK_CALL FLOAT2(In const API::Global::Math::Vector& v);
				CLOAK_CALL FLOAT2(In const API::Global::Math::Point& v);
				FLOAT2& CLOAK_CALL operator=(In const API::Global::Math::Space2D::Vector& v);
				FLOAT2& CLOAK_CALL operator=(In const API::Global::Math::Space2D::Point& v);
				FLOAT2& CLOAK_CALL operator=(In const API::Global::Math::Vector& v);
				FLOAT2& CLOAK_CALL operator=(In const API::Global::Math::Point& v);
				FLOAT1& CLOAK_CALL operator[](In size_t a);
			};
			struct CLOAKENGINE_API FLOAT3 {
				FLOAT1 X;
				FLOAT1 Y;
				FLOAT1 Z;
				CLOAK_CALL FLOAT3();
				CLOAK_CALL FLOAT3(In FLOAT1 x, In_opt FLOAT1 y = 0, In_opt FLOAT1 z = 0);
				CLOAK_CALL FLOAT3(In const API::Global::Math::Space2D::Vector& v);
				CLOAK_CALL FLOAT3(In const API::Global::Math::Vector& v);
				CLOAK_CALL FLOAT3(In const API::Global::Math::Point& v);
				CLOAK_CALL FLOAT3(In const API::Helper::Color::RGBX& c);
				FLOAT3& CLOAK_CALL operator=(In const API::Global::Math::Space2D::Vector& v);
				FLOAT3& CLOAK_CALL operator=(In const API::Global::Math::Vector& v);
				FLOAT3& CLOAK_CALL operator=(In const API::Global::Math::Point& v);
				FLOAT3& CLOAK_CALL operator=(In const API::Helper::Color::RGBX& c);
				FLOAT1& CLOAK_CALL operator[](In size_t a);
			};
			struct CLOAKENGINE_API FLOAT4 {
				FLOAT1 X;
				FLOAT1 Y;
				FLOAT1 Z;
				FLOAT1 W;
				CLOAK_CALL FLOAT4();
				CLOAK_CALL FLOAT4(In FLOAT1 x, In_opt FLOAT1 y = 0, In_opt FLOAT1 z = 0, In_opt FLOAT1 w = 0);
				CLOAK_CALL FLOAT4(In const API::Global::Math::Vector& v);
				CLOAK_CALL FLOAT4(In const API::Global::Math::Point& v);
				CLOAK_CALL FLOAT4(In const API::Helper::Color::RGBX& c);
				CLOAK_CALL FLOAT4(In const API::Helper::Color::RGBA& c);
				FLOAT4& CLOAK_CALL operator=(In const API::Global::Math::Vector& v);
				FLOAT4& CLOAK_CALL operator=(In const API::Global::Math::Point& v);
				FLOAT4& CLOAK_CALL operator=(In const API::Helper::Color::RGBX& c);
				FLOAT4& CLOAK_CALL operator=(In const API::Helper::Color::RGBA& c);
				FLOAT1& CLOAK_CALL operator[](In size_t a);
			};
			typedef UINT UINT1;
			struct CLOAKENGINE_API UINT2 {
				UINT1 X;
				UINT1 Y;
				CLOAK_CALL UINT2();
				CLOAK_CALL UINT2(In UINT1 x, In_opt UINT1 y = 0);
			};
			struct CLOAKENGINE_API UINT3 {
				UINT1 X;
				UINT1 Y;
				UINT1 Z;
				CLOAK_CALL UINT3();
				CLOAK_CALL UINT3(In UINT1 x, In_opt UINT1 y = 0, In_opt UINT1 z = 0);
			};
			struct CLOAKENGINE_API UINT4 {
				UINT1 X;
				UINT1 Y;
				UINT1 Z;
				UINT1 W;
				CLOAK_CALL UINT4();
				CLOAK_CALL UINT4(In UINT1 x, In_opt UINT1 y = 0, In_opt UINT1 z = 0, In_opt UINT1 w = 0);
			};
			typedef INT INT1;
			struct CLOAKENGINE_API INT2 {
				INT1 X;
				INT1 Y;
				CLOAK_CALL INT2();
				CLOAK_CALL INT2(In INT1 x, In_opt INT1 y = 0);
			};
			struct CLOAKENGINE_API INT3 {
				INT1 X;
				INT1 Y;
				INT1 Z;
				CLOAK_CALL INT3();
				CLOAK_CALL INT3(In INT1 x, In_opt INT1 y = 0, In_opt INT1 z = 0);
			};
			struct CLOAKENGINE_API INT4 {
				INT1 X;
				INT1 Y;
				INT1 Z;
				INT1 W;
				CLOAK_CALL INT4();
				CLOAK_CALL INT4(In INT1 x, In_opt INT1 y = 0, In_opt INT1 z = 0, In_opt INT1 w = 0);
			};
			struct CLOAK_ALIGN MATRIX {
				FLOAT1 Data[16];
				CLOAK_CALL MATRIX();
				CLOAK_CALL MATRIX(In FLOAT1(&data)[16]);
				CLOAK_CALL MATRIX(In FLOAT4 r0, In FLOAT4 r1, In FLOAT4 r2, In FLOAT4 r3);
				CLOAK_CALL MATRIX(In const API::Global::Math::Matrix& m);
				MATRIX& CLOAK_CALL_THIS operator=(In const MATRIX& o);
				MATRIX& CLOAK_CALL_THIS operator=(In const API::Global::Math::Matrix& m);
			};

			//Size-Checks:
			static_assert(sizeof(FLOAT1) == 4,	"FLOAT1 must be exactly 32 bit wide");
			static_assert(sizeof(FLOAT2) == 8,	"FLOAT2 must be exactly 64 bit wide");
			static_assert(sizeof(FLOAT3) == 12,	"FLOAT3 must be exactly 96 bit wide");
			static_assert(sizeof(FLOAT4) == 16,	"FLOAT4 must be exactly 128 bit wide");

			static_assert(sizeof(UINT1) == 4,	"UINT1 must be exactly 32 bit wide");
			static_assert(sizeof(UINT2) == 8,	"UINT2 must be exactly 64 bit wide");
			static_assert(sizeof(UINT3) == 12,	"UINT3 must be exactly 96 bit wide");
			static_assert(sizeof(UINT4) == 16,	"UINT4 must be exactly 128 bit wide");

			static_assert(sizeof(MATRIX) == 64,	"MATRIX must be exactly 512 bit wide");
		}
	}
}

#endif