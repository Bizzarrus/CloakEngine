#pragma once
#ifndef CC_API_MESH_H
#define CC_API_MESH_H

#include "CloakCompiler/Defines.h"
#include "CloakCompiler/EncoderBase.h"
#include "CloakEngine/CloakEngine.h"

#include <vector>
#include <string>

namespace CloakCompiler {
	CLOAKCOMPILER_API_NAMESPACE namespace API {
		namespace Mesh {
			constexpr CloakEngine::Files::FileType FileType{ "MeshFile","CEMF",1003 };

			enum class RespondeCode {
				CALC_MATERIALS,
				CALC_BINORMALS,
				CALC_BOUNDING,
				CALC_SORT,
				CALC_BONES,
				CALC_INDICES,
				WRITE_VERTICES,
				WRITE_BOUNDING,
				WRITE_TMP,
				CHECK_TMP,
				FINISH_SUCCESS,
				FINISH_ERROR,
				ERROR_BINORMALS,
				ERROR_BOUNDING,
				ERROR_VERTEXREF,
			};
			struct RespondeInfo {
				RespondeCode Code;
				std::string Msg;
				size_t Polygon;
				size_t Vertex;
			};

			enum class BoundingVolume {
				None,
				OOBB,
				Sphere,
			};
			struct TexCoord {
				float U;
				float V;
			};
			struct Bone {
				size_t BoneID;
				float Weight;
			};
			struct Vector {
				union {
					float Data[4];
					struct {
						float X;
						float Y;
						float Z;
						float W;
					};
				};
				CLOAKCOMPILER_API Vector();
				CLOAKCOMPILER_API Vector(In const CloakEngine::Global::Math::Vector& v);
				CLOAKCOMPILER_API Vector& operator=(In const Vector& v);
				CLOAKCOMPILER_API Vector& operator=(In const CloakEngine::Global::Math::Vector& v);
				CLOAKCOMPILER_API operator CloakEngine::Global::Math::Vector();
				CLOAKCOMPILER_API operator const CloakEngine::Global::Math::Vector() const;
			};
			struct Vertex {
				Vector Position;
				Vector Normal;
				TexCoord TexCoord;
				Bone Bones[4];
			};
			/*
			Binormal and Tangent are auto-generated
			Formula in shader to get bumped normal:
			rn = (2*normalMap.Sample(texCoord)) - float2(1,1);
			normal = normalize((rn.x * input.tangent) + (rn.y * input.binormal) + (rn.z * input.normal))
			*/
			struct Polygon {
				uint32_t Point[3];
				bool AutoGenerateNormal = false;
				uint32_t Material;
			};
			struct Desc {
				std::vector<Vertex> Vertices;
				std::vector<Polygon> Polygons;
				BoundingVolume Bounding;
				bool UseIndexBuffer = true;
			};

			CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In std::function<void(const RespondeInfo&)> response = false);
		}
	}
}

#endif