#pragma once
#ifndef CE_IMPL_RENDERING_RESOURCE_H
#define CE_IMPL_RENDERING_RESOURCE_H

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/Adapter.h"
#include "CloakEngine/Rendering/Resource.h"

#define RESOURCE_STATE_INVALID static_cast<API::Rendering::RESOURCE_STATE>(-1)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Resource_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{55F044AA-29DD-44B3-B156-EADED2C3978D}") IResource : public virtual API::Rendering::IResource {
					public:
						virtual const API::Rendering::GPU_DESCRIPTOR_HANDLE& CLOAK_CALL_THIS GetGPUAddress() const = 0;
						virtual void CLOAK_CALL_THIS SetResourceState(In API::Rendering::RESOURCE_STATE state, In_opt bool autoPromote = false) = 0;
						virtual void CLOAK_CALL_THIS SetTransitionResourceState(In API::Rendering::RESOURCE_STATE state) = 0;
						virtual API::Rendering::RESOURCE_STATE CLOAK_CALL_THIS GetResourceState() const = 0;
						virtual API::Rendering::RESOURCE_STATE CLOAK_CALL_THIS GetTransitionResourceState() const = 0;
						virtual bool CLOAK_CALL_THIS CanTransition() const = 0;
						virtual bool CLOAK_CALL_THIS SupportStatePromotion() const = 0;
						virtual uint8_t CLOAK_CALL_THIS GetDetailLevel() const = 0;
						virtual size_t CLOAK_CALL_THIS GetNodeID() const = 0;
						virtual void CLOAK_CALL_THIS RegisterUsage(In Impl::Rendering::IAdapter* queue) = 0;
						virtual void CLOAK_CALL_THIS UnregisterUsage(In uint64_t fence) = 0;
						Ret_notnull virtual API::Helper::ISyncSection* CLOAK_CALL_THIS GetSyncSection() = 0;
				};
			}
		}
	}
}


#endif