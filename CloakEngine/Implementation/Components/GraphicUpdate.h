#pragma once
#ifndef CE_IMPL_COMPONENTS_GRAPHICUPDATE_H
#define CE_IMPL_COMPONENTS_GRAPHICUPDATE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/World/Entity.h"
#include "CloakEngine/Rendering/Manager.h"
#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Global/Game.h"

#include "Implementation/Components/Position.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			inline namespace GraphicUpdate_v1 {
				CLOAK_COMPONENT("{71DAFE48-1F07-4E22-BE2D-66E1BDDD2B37}", IGraphicUpdate) {
					public:
						CLOAKENGINE_API CLOAK_CALL IGraphicUpdate(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID) {}

						virtual void CLOAK_CALL_THIS Update(In API::Global::Time time, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager) = 0;
				};
				CLOAK_COMPONENT("{5ADBC30B-A197-40E7-AEF7-5F3DABC6DD23}", IGraphicUpdatePosition) {
					public:
						CLOAKENGINE_API CLOAK_CALL IGraphicUpdatePosition(In API::World::IEntity* e, In type_id typeID) : Component(e, typeID) {}

						virtual void CLOAK_CALL_THIS Update(In API::Global::Time time, In Impl::Components::Position* position, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager) = 0;
						virtual float CLOAK_CALL_THIS GetBoundingRadius() const = 0;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif