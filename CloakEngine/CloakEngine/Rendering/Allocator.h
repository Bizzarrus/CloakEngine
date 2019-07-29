#pragma once
#ifndef CE_API_RENDERING_ALLOCATOR_H
#define CE_API_RENDERING_ALLOCATOR_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Rendering/Defines.h"
#include "CloakEngine/Rendering/Resource.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace Allocator_v1 {
				enum class DescriptorPageType { UTILITY, WORLD, GUI };
				CLOAK_INTERFACE_ID("{FFD15C30-D253-4060-BC79-A9CAAA348F63}") IAllocator : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS PushStack() = 0;
						virtual void CLOAK_CALL_THIS PopStack() = 0;
						virtual GPU_ADDRESS CLOAK_CALL_THIS Alloc(In size_t size, In size_t allign) = 0;
						virtual intptr_t CLOAK_CALL_THIS GetSizeOfFreeSpace() const = 0;
				};
				CLOAK_INTERFACE_ID("{80D08D5E-DA81-4662-8F07-BCEC5663EF4E}") IReadBackPage : public virtual API::Rendering::Resource_v1::IResource{
					public:
						virtual void CLOAK_CALL_THIS ReadData(In size_t offset, In size_t size, Out_writes_bytes(size) void* res) = 0;
						virtual void CLOAK_CALL_THIS AddReadCount(In size_t offset) = 0;
						virtual void CLOAK_CALL_THIS RemoveReadCount(In size_t offset) = 0;
				};
				class CLOAKENGINE_API ReadBack {
					public:
						CLOAK_CALL ReadBack(IReadBackPage* page, In size_t offset, In size_t size);
						CLOAK_CALL ReadBack(In const ReadBack& o);
						CLOAK_CALL ReadBack(In ReadBack&& o);
						CLOAK_CALL ~ReadBack();
						ReadBack& CLOAK_CALL_THIS operator=(In const ReadBack& o);
						ReadBack& CLOAK_CALL_THIS operator=(In ReadBack&& o);

						void* CLOAK_CALL_THIS GetData();
						size_t CLOAK_CALL_THIS GetSize() const;
						IReadBackPage* CLOAK_CALL_THIS GetResource() const;
						size_t CLOAK_CALL_THIS GetOffset() const;
					private:
						bool m_state;
						IReadBackPage* m_page;
						size_t m_offset;
						size_t m_size;
						void* m_data;
				};
			}
		}
	}
}

#endif