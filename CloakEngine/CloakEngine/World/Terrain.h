#pragma once
#ifndef CE_API_WORLD_TERRAIN_H
#define CE_API_WORLD_TERRAIN_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Rendering/Context.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace World {
			inline namespace Terrain_v1 {
				enum class TerrainType {
					Heightfield,
					Vectorfield,
				};
				CLOAK_INTERFACE_ID("{4D73B890-3D0C-4352-A3DF-60D9695268C8}") ITerrain : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual TerrainType CLOAK_CALL_THIS GetType() const = 0;
						virtual void CLOAK_CALL_THIS Draw(In API::Rendering::IContext* context, In UINT rootIndexField, In_opt UINT rootIndexLookUp = INVALID_ROOT_INDEX, In_opt UINT rootIndexAtlas = INVALID_ROOT_INDEX) = 0;
				};
			}
		}
	}
}

#endif