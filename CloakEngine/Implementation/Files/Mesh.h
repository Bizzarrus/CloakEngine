#pragma once
#ifndef CE_IMPL_FILES_MESH_H
#define CE_IMPL_FILES_MESH_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Mesh.h"
#include "CloakEngine/Global/Math.h"

#include "Implementation/Files/FileHandler.h"
#include "Implementation/Rendering/Manager.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace Mesh_v1 {
				class CLOAK_UUID("{9151C99A-17A5-453D-A669-5C7B084AF11C}") Mesh : public virtual API::Files::Mesh_v1::IMesh, public virtual FileHandler_v1::RessourceHandler {
					public:
						CLOAK_CALL Mesh();
						virtual CLOAK_CALL ~Mesh();
						virtual API::Files::IBoundingVolume* CLOAK_CALL_THIS CreateBoundingVolume() const override;
						virtual API::Rendering::IVertexBuffer* CLOAK_CALL_THIS GetVertexBuffer(In size_t slot, In API::Files::LOD lod) const override;
						Ret_maybenull virtual API::Rendering::IIndexBuffer* CLOAK_CALL_THIS GetIndexBuffer(In API::Files::LOD lod) const override;
						virtual size_t CLOAK_CALL_THIS GetMaterialCount() const override;
						virtual size_t CLOAK_CALL_THIS GetMaterialInfoCount() const override;
						virtual API::Files::MeshMaterialInfo CLOAK_CALL_THIS GetMaterialInfo(In size_t mat, In API::Files::LOD lod) const override;

						virtual void CLOAK_CALL_THIS RequestLOD(In API::Files::LOD lod) override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual bool CLOAK_CALL_THIS iCheckSetting(In const API::Global::Graphic::Settings& nset) const override;
						virtual LoadResult CLOAK_CALL_THIS iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) override;
						virtual void CLOAK_CALL_THIS iUnload() override;

						size_t m_matCount;
						size_t m_matIDCount;
						API::Files::MeshMaterialInfo* m_mats;
						API::Rendering::IVertexBuffer* m_vertex[2];
						API::Rendering::IIndexBuffer* m_index;
						API::Files::BoundingDesc m_boundDesc;
						uint32_t m_boneCount;
						bool m_includeStrips;
						bool m_tessellation;
				};

				DECLARE_FILE_HANDLER(Mesh, "{3ED988F1-2EDE-4423-840C-1CA21C2D9DC6}");
			}
		}
	}
}

#pragma warning(pop)

#endif