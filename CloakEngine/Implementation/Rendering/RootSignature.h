#pragma once
#ifndef CE_IMPL_RENDEIRNG_ROOTSIGNATURE_H
#define CE_IMPL_RENDEIRNG_ROOTSIGNATURE_H

#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Rendering/RootDescription.h"
#include "Implementation/Rendering/Defines.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace RootSignature_v1 {
				constexpr size_t MAX_ROOT_PARAMETERS = API::Rendering::RootDescription::MAX_ROOT_PARAMETER_COUNT;
				RENDERING_INTERFACE CLOAK_UUID("{22FBCDED-B0CB-42B7-A0A3-F7847D50985D}") IRootSignature : public virtual API::Helper::ISavePtr {
					public:
						virtual void CLOAK_CALL_THIS Finalize(In const API::Rendering::RootDescription& desc) = 0;
						virtual UINT CLOAK_CALL_THIS GetTableSize(In DWORD index) const = 0;
						virtual uint32_t CLOAK_CALL_THIS GetDescriptorBitMap() const = 0;
						virtual API::Rendering::ROOT_PARAMETER_TYPE CLOAK_CALL_THIS GetParameterType(In size_t id) const = 0;
						virtual UINT CLOAK_CALL_THIS RequestTextureAtlasSize(In DWORD rootIndex, In UINT size) = 0;
				};
			}
		}
	}
}

#endif