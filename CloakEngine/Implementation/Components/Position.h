#pragma once
#ifndef CE_IMPL_COMPONENTS_POSITION_H
#define CE_IMPL_COMPONENTS_POSITION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Components/Position.h"
#include "Implementation/Files/Scene.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			inline namespace Position_v1 {
				class CLOAK_UUID("{A88C4A3F-0088-4EB6-AE74-7DEE213AFF71}") Position : public virtual API::Components::Position_v1::IPosition {
					public:
						typedef API::Components::Position_v1::IPosition Super;

						CLOAK_CALL Position(In API::World::IEntity* parent, In type_id typeID);
						CLOAK_CALL Position(In API::World::IEntity* parent, In API::World::Component* src);
						virtual CLOAK_CALL ~Position();

						virtual void CLOAK_CALL_THIS OnEnable() override;
						virtual void CLOAK_CALL_THIS OnDisable() override;

						virtual CLOAKENGINE_API API::Global::Math::WorldPosition CLOAK_CALL_THIS GetPosition() const override;
						virtual CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS GetLocalPosition() const override;
						virtual CLOAKENGINE_API API::Global::Math::Quaternion CLOAK_CALL_THIS GetRotation() const override;
						virtual CLOAKENGINE_API API::Global::Math::Quaternion CLOAK_CALL_THIS GetLocalRotation() const override;
						virtual CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS GetScale() const override;
						virtual CLOAKENGINE_API API::Global::Math::Vector CLOAK_CALL_THIS GetLocalScale() const override;
						virtual CLOAKENGINE_API API::Files::IWorldNode* CLOAK_CALL_THIS GetNode() const override;
						virtual CLOAKENGINE_API API::RefPointer<API::Components::IPosition> CLOAK_CALL_THIS GetParent() const override;

						virtual CLOAKENGINE_API void CLOAK_CALL_THIS SetParent(In IPosition* par) override;

						virtual const API::Global::Math::Matrix& CLOAK_CALL_THIS GetLocalToNode(In API::Global::Time time, Out_opt Impl::Files::WorldNode** node = nullptr);
						virtual void CLOAK_CALL_THIS SetNode(In API::Files::IWorldNode* node, In const API::Global::Math::Vector& offset, In const API::Global::Math::Quaternion& rot, In const API::Global::Math::Vector& scale);
						virtual void CLOAK_CALL_THIS RemoveFromNode(In_opt API::Files::IWorldNode* callNode = nullptr);
					protected:
						virtual void CLOAK_CALL_THIS iUpdateNode(In Impl::Files::WorldNode* last, In Impl::Files::WorldNode* next);
						virtual void CLOAK_CALL_THIS iRemoveChild(In Position* pos);
						virtual void CLOAK_CALL_THIS iAddChild(In Position* pos);
						virtual void CLOAK_CALL_THIS iRemoveParent();
						virtual void CLOAK_CALL_THIS iSetNode(In Impl::Files::WorldNode* node);

						API::Global::Math::Vector m_offset;
						Impl::Files::WorldNode* m_node;
						Impl::Files::ObjectRef m_ref;
						API::Global::Math::Quaternion m_rot;
						API::Global::Math::Vector m_scale;
						API::Global::Math::Matrix m_LTN;
						API::Global::Time m_LTNTime;
						API::FlatSet<Position*> m_childs;
						Position* m_anchor;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif