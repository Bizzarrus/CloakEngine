#pragma once
#ifndef CE_API_FILES_SCENE_H
#define CE_API_FILES_SCENE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/FileHandler.h"
#include "CloakEngine/Files/Image.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Rendering/Context.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace World {
			inline namespace Entity_v1 {
				class IEntity;
			}
		}
		namespace Files {
			inline namespace Scene_v1 {
				class IScene;

				enum class AdjacentPos {
					Right = 0x0, //X+
					Left = 0x1, //X-
					Top = 0x2, //Y+
					Bottom = 0x3, //Y-
					Front = 0x4, //Z+
					Back = 0x5, //Z-
				};
				CLOAK_INTERFACE_ID("{517FDBE9-DC61-47CA-AD9F-755A6D21A62C}") IWorldContainer : public virtual Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::Vector& pos, In const Global::Math::Quaternion& rotation, In_opt float scale = 1) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::Vector& pos, In const Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::Vector& pos, In const Global::Math::Quaternion& rotation, In const Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::DualQuaternion& pr, In_opt float scale = 1) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::DualQuaternion& pr, In float scaleX, In float scaleY, In float scaleZ) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::DualQuaternion& pr, In const Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::Vector& pos, In_opt float scale = 1) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::Vector& pos, In float scaleX, In float scaleY, In float scaleZ) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::Vector& pos, In const Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In float X, In float Y, In float Z, In_opt float scale = 1) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In float X, In float Y, In float Z, In float scaleX, In float scaleY, In float scaleZ) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In float X, In float Y, In float Z, In const Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In float X, In float Y, In float Z, In const Global::Math::Quaternion& rotation, In_opt float scale = 1) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In float X, In float Y, In float Z, In const Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In float X, In float Y, In float Z, In const Global::Math::Quaternion& rotation, In const Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::WorldPosition& pos, In_opt float scale = 1) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::WorldPosition& pos, In float scaleX, In float scaleY, In float scaleZ) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::WorldPosition& pos, In const Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::WorldPosition& pos, In const Global::Math::Quaternion& rotation, In_opt float scale = 1) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::WorldPosition& pos, In const Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) = 0;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const Global::Math::WorldPosition& pos, In const Global::Math::Quaternion& rotation, In const Global::Math::Vector& scale) = 0;
						virtual void CLOAK_CALL_THIS Remove(In API::World::Entity_v1::IEntity* object) = 0;
				};
				CLOAK_INTERFACE_ID("{5265D16C-F370-43ED-8B77-84378FF8C7CF}") IWorldNode : public virtual IWorldContainer {
					public:
						virtual IWorldNode* CLOAK_CALL_THIS GetAdjacentNode(In AdjacentPos pos) const = 0;
						virtual IScene* CLOAK_CALL_THIS GetParentScene() const = 0;
						virtual void CLOAK_CALL_THIS AddPortal(In IWorldNode* oth, In float dist) = 0;
						virtual const API::Global::Math::IntVector& CLOAK_CALL_THIS GetPosition() const = 0;
				};
				CLOAK_INTERFACE_ID("{DDDCC280-6A40-4A51-BBD2-56B3E66F6AF5}") IScene : public virtual Files::FileHandler_v1::IFileHandler, public virtual IWorldContainer{
					public:
						virtual void CLOAK_CALL_THIS SetSkybox(In Files::ICubeMap* skybox, In float lightIntensity) = 0;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In const API::Helper::Color::RGBX& col) = 0;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In float R, In float G, In float B) = 0;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In float R, In float G, In float B, In float intensity) = 0;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In const API::Helper::Color::RGBX& col, In float intensity) = 0;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In const API::Helper::Color::RGBA& col) = 0;
						virtual void CLOAK_CALL_THIS SetAmbientLightIntensity(In float intensity) = 0;
						virtual void CLOAK_CALL_THIS SetGravity(In const API::Global::Math::Vector& dir) = 0;
						virtual void CLOAK_CALL_THIS EnableGravity() = 0;
						virtual void CLOAK_CALL_THIS DisableGravity() = 0;

						virtual IWorldNode* CLOAK_CALL_THIS GetWorldNode(In int X, In int Y, In int Z) = 0;
						virtual const API::Helper::Color::RGBA& CLOAK_CALL_THIS GetAmbientLight() const = 0;
						virtual bool CLOAK_CALL_THIS GetSkybox(Out float* lightIntensity, In REFIID riid, Outptr void** ppvObject) const = 0;
				};
				CLOAK_INTERFACE_ID("{2B1C6D8F-4ADA-4A7B-B0F6-DE6D5D099C67}") IGeneratedScene : public virtual IScene{

				};
			}
		}
	}
}

#pragma warning(pop)

#endif