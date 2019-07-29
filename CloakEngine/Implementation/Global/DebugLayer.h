#pragma once
#ifndef CE_IMPL_GLOBAL_DEBUGLAYER_H
#define CE_IMPL_GLOBAL_DEBUGLAYER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/DebugLayer.h"

#include "Implementation/Helper/SavePtr.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			inline namespace DebugLayer_v1 {
				class CLOAK_UUID("{77F1E0C7-6568-4DCD-8ACF-B2B270746778}") DebugLayer : public virtual API::Global::DebugLayer_v1::IDebugLayer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL DebugLayer();
						virtual CLOAK_CALL ~DebugLayer();
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif