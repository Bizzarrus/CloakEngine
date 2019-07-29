#pragma once
#ifndef CE_API_COMPONENTS_RENDER_H
#define CE_API_COMPONENTS_RENDER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/World/Entity.h"
#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Files/Mesh.h"
#include "CloakEngine/Files/Image.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Components {
			inline namespace Render_v1 {
				CLOAK_COMPONENT("{49AC7561-E326-4586-9C0D-743118468D90}", IRender), public virtual Rendering::IDrawable3D {
					public:
						CLOAKENGINE_API CLOAK_CALL IRender(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID) {}
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetMesh(In API::Files::IMesh* mesh) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetMaterials(In_reads(size) Files::IMaterial** mat, In size_t size) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetMaterials(In const API::List<Files::IMaterial*>& mats, In_opt size_t size = static_cast<size_t>(-1)) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetMaterialCount(In size_t size) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetMaterial(In size_t id, In Files::IMaterial* mat) = 0;
						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetMaterial(In size_t id, In const API::RefPointer<Files::IMaterial>& mat) = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif