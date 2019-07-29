#pragma once
#ifndef CE_IMPL_COMPONENTS_RENDER_H
#define CE_IMPL_COMPONENTS_RENDER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Components/Render.h"

#include "Implementation/Components/GraphicUpdate.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			inline namespace Render_v1 {
				class CLOAK_UUID("{4144FE9F-D486-4B78-8A5B-7D1FD1E79F46}") Render : public API::Components::Render_v1::IRender, public virtual Impl::Components::IGraphicUpdatePosition {
					public:
						typedef type_tuple<API::Components::Render_v1::IRender, Impl::Components::IGraphicUpdatePosition> Super;

						CLOAK_CALL Render(In API::World::IEntity* parent, In type_id typeID);
						CLOAK_CALL Render(In API::World::IEntity* parent, In API::World::Component* src);
						virtual CLOAK_CALL ~Render();

						virtual void CLOAK_CALL_THIS SetMesh(In API::Files::IMesh* mesh) override;
						virtual void CLOAK_CALL_THIS SetMaterials(In_reads(size) API::Files::IMaterial** mat, In size_t size) override;
						virtual void CLOAK_CALL_THIS SetMaterials(In const API::List<API::Files::IMaterial*>& mats, In_opt size_t size = static_cast<size_t>(-1)) override;
						virtual void CLOAK_CALL_THIS SetMaterialCount(In size_t size) override;
						virtual void CLOAK_CALL_THIS SetMaterial(In size_t id, In API::Files::IMaterial* mat) override;
						virtual void CLOAK_CALL_THIS SetMaterial(In size_t id, In const API::RefPointer<API::Files::IMaterial>& mat) override;

						virtual void CLOAK_CALL_THIS OnLoad(In API::Files::Priority prio, In API::Files::LOD lod) override;
						virtual void CLOAK_CALL_THIS OnUnload() override;

						virtual float CLOAK_CALL_THIS GetBoundingRadius() const override;
						virtual void CLOAK_CALL_THIS Update(In API::Global::Time time, In Impl::Components::Position* position, In API::Rendering::ICopyContext* context, In API::Rendering::IManager* manager) override;

						virtual void CLOAK_CALL_THIS Draw(In API::Rendering::IContext3D* context, In_opt API::Files::LOD lod = API::Files::LOD::ULTRA) override;
					protected:
						bool m_loaded;
						API::Files::LOD m_lod;
						API::Files::IMaterial** m_mats;
						size_t m_matCount;
						size_t m_meshMatCount;
						API::Files::IMesh* m_mesh;
						API::Files::IBoundingVolume* m_bounding;
						API::Rendering::IConstantBuffer* m_localBuffer;
						API::Global::Math::Vector m_pos;
						API::Rendering::WorldID m_worldID;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif