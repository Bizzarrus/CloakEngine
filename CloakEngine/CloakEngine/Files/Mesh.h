#pragma once
#ifndef CE_API_FILES_MESH_H
#define CE_API_FILES_MESH_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/FileHandler.h"
#include "CloakEngine/Files/BoundingVolume.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Rendering/StructuredBuffer.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Mesh_v1 {
				struct MeshMaterialInfo {
					size_t Offset;
					size_t SizeList;
					size_t SizeStrip;
					size_t MaterialID;
				};
				CLOAK_INTERFACE_ID("{91A1A4AD-7695-4623-82DC-239FEE2CC216}") IMesh : public virtual Files::FileHandler_v1::IFileHandler, public virtual Files::FileHandler_v1::ILODFile{
					public:
						virtual IBoundingVolume* CLOAK_CALL_THIS CreateBoundingVolume() const = 0;
						virtual Rendering::IVertexBuffer* CLOAK_CALL_THIS GetVertexBuffer(In size_t slot, In API::Files::LOD lod) const = 0;
						Ret_maybenull virtual Rendering::IIndexBuffer* CLOAK_CALL_THIS GetIndexBuffer(In API::Files::LOD lod) const = 0;
						virtual size_t CLOAK_CALL_THIS GetMaterialCount() const = 0;
						virtual size_t CLOAK_CALL_THIS GetMaterialInfoCount() const = 0;
						virtual MeshMaterialInfo CLOAK_CALL_THIS GetMaterialInfo(In size_t mat, In API::Files::LOD lod) const = 0;
				};
			}
		}
	}
}

#endif
