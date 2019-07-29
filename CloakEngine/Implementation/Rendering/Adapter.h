#pragma once
#ifndef CE_IMPL_RENDERING_ADAPTER_H
#define CE_IMPL_RENDERING_ADAPTER_H

#include "Implementation/Rendering/Defines.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Queue_v1 {
				RENDERING_INTERFACE IAdapter {
					public:
						virtual QUEUE_TYPE CLOAK_CALL_THIS GetType() const = 0;
						virtual size_t CLOAK_CALL_THIS GetNodeID() const = 0;
						virtual UINT CLOAK_CALL_THIS GetNodeMask() const = 0;
						virtual void CLOAK_CALL_THIS CreateContext(In REFIID riid, Outptr void** ptr) = 0;
						virtual bool CLOAK_CALL_THIS IsFenceComplete(In uint64_t val) = 0;
						virtual bool CLOAK_CALL_THIS WaitForFence(In uint64_t val) = 0;
						virtual void CLOAK_CALL_THIS WaitForAdapter(In IAdapter* q, In uint64_t fence) = 0;
						virtual uint64_t CLOAK_CALL_THIS FlushExecutions(In API::Global::Memory::PageStackAllocator* alloc, In_opt bool wait = false) = 0;
						virtual void CLOAK_CALL_THIS ReleaseOnFence(In API::Helper::ISavePtr* ptr, In uint64_t fence) = 0;
				};
			}
		}
	}
}

#endif