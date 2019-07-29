#pragma once
#ifndef CE_IMPL_WORLD_COMPONENTMANAGER_H
#define CE_IMPL_WORLD_COMPONENTMANAGER_H

#include "CloakEngine/World/ComponentManager.h"
#include "Implementation/World/Entity.h"

namespace CloakEngine {
	namespace Impl {
		namespace World {
			namespace ComponentManager {
				uint32_t CLOAK_CALL GetID(In const type_id& guid);
				void* CLOAK_CALL Allocate(In const type_id& id, In size_t size, In size_t alignment);
				void CLOAK_CALL Free(In const type_id& id, In void* ptr);
				void CLOAK_CALL Initialize();
				void CLOAK_CALL ReleaseSyncs();
				void CLOAK_CALL Release();
				void* CLOAK_CALL AllocateEntity(In size_t size);
				void CLOAK_CALL FreeEntity(In void* ptr);
				void CLOAK_CALL UpdateEntityCache(Entity* e, In bool remove);
			}
		}
	}
}

#endif