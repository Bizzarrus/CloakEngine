#pragma once
#ifndef CE_IMPL_FILES_SCENE_H
#define CE_IMPL_FILES_SCENE_H

#include "CloakEngine/Files/Scene.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Rendering/Context.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Rendering/RenderPass.h"

#include "Implementation/Files/FileHandler.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Helper/SyncSection.h"
#include "Implementation/World/Entity.h"

#include "Engine/LinkedList.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			inline namespace Position_v1 {
				class Position;
			}
		}
		namespace Files {
			inline namespace Scene_v1 {
				constexpr float WORLDNODE_SIZE = 32.0f;
				constexpr float WORLDNODE_HALFSIZE = WORLDNODE_SIZE / 2;
				
				class WorldNode;
				class Scene;
				class GeneratedScene;

				typedef Engine::LinkedList<Impl::Components::Position_v1::Position*> ObjectList;
				typedef ObjectList::const_iterator ObjectRef;

				struct PortalInfo {
					WorldNode* Node;
					float Distance;
				};
				struct TerrainInfo {
					API::Rendering::IColorBuffer* HeightMap;
					API::Global::Math::IntVector Position;
					float CornerOffset[4];
				};

				class CLOAK_UUID("{90393F5C-9CA7-4E4E-960D-8A6583F838FA}") WorldNode : public virtual API::Files::Scene_v1::IWorldNode, public virtual API::Files::FileHandler_v1::ILODFile {
					public:
						CLOAK_CALL WorldNode(In Scene* par, In const API::Global::Math::IntVector& pos);
						virtual CLOAK_CALL ~WorldNode();

						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Remove(In API::World::IEntity* object) override;

						virtual IWorldNode* CLOAK_CALL_THIS GetAdjacentNode(In API::Files::Scene_v1::AdjacentPos pos) const override;
						virtual API::Files::Scene_v1::IScene* CLOAK_CALL_THIS GetParentScene() const override;
						virtual void CLOAK_CALL_THIS AddPortal(In IWorldNode* oth, In float dist) override;
						virtual const API::Global::Math::IntVector& CLOAK_CALL_THIS GetPosition() const override;

						virtual void CLOAK_CALL_THIS RequestLOD(In API::Files::LOD lod) override;

						virtual uint64_t CLOAK_CALL_THIS GetRefCount() override;
						virtual API::Global::Debug::Error CLOAK_CALL_THIS Delete() override;
						Success(return == S_OK) Post_satisfies(*ppvObject != nullptr) virtual HRESULT STDMETHODCALLTYPE QueryInterface(In REFIID riid, Outptr void** ppvObject) override;
						virtual uint64_t CLOAK_CALL_THIS AddRef() override;
						virtual uint64_t CLOAK_CALL_THIS Release() override;
						virtual void CLOAK_CALL_THIS SetDebugName(In const std::string& s) override;
						virtual void CLOAK_CALL_THIS SetSourceFile(In const std::string& s) override;
						virtual std::string CLOAK_CALL_THIS GetDebugName() const override;
						virtual std::string CLOAK_CALL_THIS GetSourceFile() const override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);

						virtual void CLOAK_CALL_THIS SetAdjacentNodes(In WorldNode* adj[6]);
						virtual WorldNode* CLOAK_CALL_THIS GetAdjacentWorldNode(In API::Files::Scene_v1::AdjacentPos pos) const;

						virtual bool CLOAK_CALL_THIS isLoaded() const;
						virtual void CLOAK_CALL_THIS load();
						virtual void CLOAK_CALL_THIS unload();
						
						virtual bool CLOAK_CALL_THIS UpdateFloodFillCounter(In uint32_t count);
						virtual uint32_t CLOAK_CALL_THIS GetFloodFillCounter();
						virtual void CLOAK_CALL_THIS GetPortals(Out API::List<PortalInfo>* ports);
						virtual bool CLOAK_CALL_THIS setStreamFlag(In bool f);

						virtual void CLOAK_CALL_THIS Add(In Impl::Components::Position_v1::Position* pc, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rot, In const API::Global::Math::Vector& scale);
						virtual void CLOAK_CALL_THIS Add(In Impl::Components::Position_v1::Position* pc, In const API::Global::Math::Vector& pos, In const API::Global::Math::IntVector& npos, In const API::Global::Math::Quaternion& rot, In const API::Global::Math::Vector& scale);
						virtual void CLOAK_CALL_THIS SnapToNode(Out WorldNode** node, Inout API::Global::Math::Vector* pos);
						virtual void CLOAK_CALL_THIS SnapToNode(Out WorldNode** node, Inout API::Global::Math::Vector* pos, In const API::Global::Math::IntVector& npos);
						virtual void CLOAK_CALL_THIS Remove(In const ObjectRef& objRef);
						virtual void CLOAK_CALL_THIS Add(In Impl::Components::Position_v1::Position* obj, Out ObjectRef* objRef);
						virtual float CLOAK_CALL_THIS GetRadius(In API::Global::Memory::PageStackAllocator* alloc);
						virtual const API::Global::Math::Matrix& CLOAK_CALL_THIS GetNodeToWorld(Out_opt API::Rendering::WorldID* worldID = nullptr);
						virtual void CLOAK_CALL_THIS SetNodeToWorld(In const API::Global::Math::Matrix& NTW, In API::Rendering::WorldID worldID);
					protected:
						std::atomic<bool> m_inStream;
						std::atomic<ULONG> m_loadCount;
						std::atomic<uint32_t> m_floodFill;
						std::atomic<API::Files::LOD> m_lod;
						const API::Global::Math::IntVector m_pos;
						Scene* const m_parent;
						ObjectList m_objects;
						API::Helper::ISyncSection* m_sync;
						API::Helper::ISyncSection* m_drawSync;
						WorldNode* m_adjNodes[6];
						API::List<PortalInfo> m_portals;
						API::Global::Math::Matrix m_NTW;
						API::Rendering::WorldID m_worldID;
				};

				class GeneratedWorldNode : public WorldNode {
					public:
						CLOAK_CALL GeneratedWorldNode(In GeneratedScene* par, In const API::Global::Math::IntVector& pos);
						virtual CLOAK_CALL ~GeneratedWorldNode();
						virtual void CLOAK_CALL_THIS SnapToNode(Out WorldNode** node, Inout API::Global::Math::Vector* pos) override;
						virtual void CLOAK_CALL_THIS SnapToNode(Out WorldNode** node, Inout API::Global::Math::Vector* pos, In const API::Global::Math::IntVector& npos) override;
						virtual void CLOAK_CALL_THIS SetAdjacentNode(In WorldNode* node, In API::Files::Scene_v1::AdjacentPos dir);
				};

				class CLOAK_UUID("{894A1A80-D19D-4386-94F8-706CC9B92556}") Scene : public virtual API::Files::Scene_v1::IScene, public FileHandler_v1::RessourceHandler{
					public:
						CLOAK_CALL Scene();
						virtual CLOAK_CALL ~Scene();
						virtual void CLOAK_CALL_THIS SetSkybox(In API::Files::ICubeMap* skybox, In float lightIntensity) override;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In const API::Helper::Color::RGBX& col) override;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In float R, In float G, In float B) override;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In float R, In float G, In float B, In float intensity) override;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In const API::Helper::Color::RGBX& col, In float intensity) override;
						virtual void CLOAK_CALL_THIS SetAmbientLight(In const API::Helper::Color::RGBA& col) override;
						virtual void CLOAK_CALL_THIS SetAmbientLightIntensity(In float intensity) override;
						virtual API::Files::Scene_v1::IWorldNode* CLOAK_CALL_THIS GetWorldNode(In int X, In int Y, In int Z) override;
						virtual void CLOAK_CALL_THIS SetGravity(In const API::Global::Math::Vector& dir) override;
						virtual void CLOAK_CALL_THIS EnableGravity() override;
						virtual void CLOAK_CALL_THIS DisableGravity() override;
						virtual const API::Helper::Color::RGBA& CLOAK_CALL_THIS GetAmbientLight() const override;
						virtual bool CLOAK_CALL_THIS GetSkybox(Out float* lightIntensity, In REFIID riid, Outptr void** ppvObject) const override;

						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::DualQuaternion& pr, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In const API::Global::Math::Vector& pos, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::IEntity* object, In float X, In float Y, In float Z, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In_opt float scale = 1) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In float scaleX, In float scaleY, In float scaleZ) override;
						virtual void CLOAK_CALL_THIS Add(In API::World::Entity_v1::IEntity* object, In const API::Global::Math::WorldPosition& pos, In const API::Global::Math::Quaternion& rotation, In const API::Global::Math::Vector& scale) override;
						virtual void CLOAK_CALL_THIS Remove(In API::World::IEntity* object) override;

						virtual void CLOAK_CALL_THIS AddToScene(In API::World::IEntity* object);
						virtual void CLOAK_CALL_THIS RemoveFromScene(In API::World::IEntity* object);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual bool CLOAK_CALL_THIS iCheckSetting(In const API::Global::Graphic::Settings& nset) const override;
						virtual LoadResult CLOAK_CALL_THIS iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) override;
						virtual void CLOAK_CALL_THIS iUnload() override;

						API::Helper::ISyncSection* m_syncDraw;
						API::Files::ICubeMap* m_skybox;
						float m_skyIntensity;
						API::Helper::Color::RGBA m_ambient;
						WorldNode* m_root;
						void* m_nodes;
						size_t m_nodeCount;
						API::FlatSet<Impl::World::Entity*> m_entities;
				};

				class CLOAK_UUID("{D5024EBC-1BD7-42FE-9DFB-F4EF31264861}") GeneratedScene : public virtual API::Files::Scene_v1::IGeneratedScene, public Scene {
					public:
						CLOAK_CALL GeneratedScene();
						virtual CLOAK_CALL ~GeneratedScene();
						void CLOAK_CALL Resize(In API::Files::Scene_v1::AdjacentPos dir);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual LoadResult CLOAK_CALL_THIS iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) override;
						virtual void CLOAK_CALL_THIS iUnload() override;
						virtual bool CLOAK_CALL_THIS iLoadFile(Out bool* keepLoadingBackground) override;
						virtual void CLOAK_CALL_THIS iUnloadFile() override;
						virtual void CLOAK_CALL_THIS iDelayedLoadFile(Out bool* keepLoadingBackground) override;

						GeneratedWorldNode* m_topRightFront;
						GeneratedWorldNode* m_bottomLeftBack;
				};

				DECLARE_FILE_HANDLER(Scene, "{8BC16CF4-07B8-4954-B7D4-C330E73890F8}");
			}
		}
	}
}

#pragma warning(pop)

#endif