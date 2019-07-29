#pragma once
#ifndef CE_IMPL_RENDEIRNG_QUERY_H
#define CE_IMPL_RENDEIRNG_QUERY_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/Query.h"
#include "Implementation/Rendering/Defines.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Query_v1 {
				RENDERING_INTERFACE CLOAK_UUID("{D7D9EBC6-F7B1-4479-8A15-4E40FD23E656}") IQuery : public virtual API::Rendering::Query_v1::IQuery {
					public:

				};
			}
		}
	}
}

#pragma warning(pop)

#endif