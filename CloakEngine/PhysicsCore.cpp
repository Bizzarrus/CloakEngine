#include "stdafx.h"
#include "Engine/Physics/Core.h"

#include "CloakEngine/World/ComponentManager.h"
#include "Implementation/Components/Moveable.h"

namespace CloakEngine {
	namespace Engine {
		namespace Physics {
			API::Helper::ISyncSection* g_syncPosUpdate = nullptr;

			void CLOAK_CALL Start(In size_t threadID)
			{
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncPosUpdate));
			}
			void CLOAK_CALL Update(In size_t threadID, In API::Global::Time etime)
			{
				//TODO: Do physics simulation


				//Update object positions:
				API::Helper::Lock lock(g_syncPosUpdate);
				const API::Global::Time time = API::Global::Game::GetGameTimeStamp();
				API::World::Group<Impl::Components::Moveable> moveables;
				for (const auto& a : moveables) 
				{ 
					a.Get<Impl::Components::Moveable>()->Update(time); 
				}
			}
			void CLOAK_CALL Release(In size_t threadID)
			{
				SAVE_RELEASE(g_syncPosUpdate);
			}

			API::Helper::Lock CLOAK_CALL LockPositionUpdate()
			{
				return API::Helper::Lock(g_syncPosUpdate);
			}
		}
	}
}