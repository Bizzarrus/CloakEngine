#pragma once
#ifndef CE_IMPL_RENDERING_VERTEXDATA_H
#define CE_IMPL_RENDERING_VERTEXDATA_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Rendering/DataTypes.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			struct CLOAK_ALIGN VERTEX_2D {
				API::Rendering::FLOAT2 Position;
				API::Rendering::FLOAT2 Size;
				API::Rendering::FLOAT2 TexTopLeft;
				API::Rendering::FLOAT2 TexBottomRight;
				API::Rendering::FLOAT4 Color[3];
				API::Rendering::FLOAT4 Scissor;
				API::Rendering::FLOAT2 Rotation;
				API::Rendering::UINT1 Flags;

				void* CLOAK_CALL operator new(In size_t s);
				void* CLOAK_CALL operator new[](In size_t s);
				void CLOAK_CALL operator delete(In void* ptr);
				void CLOAK_CALL operator delete[](In void* ptr);
			};
			struct CLOAK_ALIGN VERTEX_3D_POSITION {
				API::Rendering::FLOAT4 Position;

				void* CLOAK_CALL operator new(In size_t s);
				void* CLOAK_CALL operator new[](In size_t s);
				void CLOAK_CALL operator delete(In void* ptr);
				void CLOAK_CALL operator delete[](In void* ptr);
			};
			struct CLOAK_ALIGN VERTEX_3D {
				API::Rendering::FLOAT4 Normal;
				API::Rendering::FLOAT4 Binormal;
				API::Rendering::FLOAT4 Tangent;
				API::Rendering::FLOAT4 BoneWeight;
				API::Rendering::UINT4 BoneID;
				API::Rendering::FLOAT2 TexCoord;

				void* CLOAK_CALL operator new(In size_t s);
				void* CLOAK_CALL operator new[](In size_t s);
				void CLOAK_CALL operator delete(In void* ptr);
				void CLOAK_CALL operator delete[](In void* ptr);
			};
			struct CLOAK_ALIGN VERTEX_TERRAIN {
				API::Rendering::FLOAT4 Position;
				API::Rendering::FLOAT4 CornerOffset;
				API::Rendering::UINT2 Adjacency;
				API::Rendering::UINT1 PatchCountPerEdge;

				void* CLOAK_CALL operator new(In size_t s);
				void* CLOAK_CALL operator new[](In size_t s);
				void CLOAK_CALL operator delete(In void* ptr);
				void CLOAK_CALL operator delete[](In void* ptr);
			};
		}
	}
}

#endif