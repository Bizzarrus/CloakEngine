#pragma once
#ifndef CE_IMPL_INTERFACE_RAYTESTCONTEXT_H
#define CE_IMPL_INTERFACE_RAYTESTCONTEXT_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Interface/BasicGUI.h"

namespace CloakEngine {
	namespace Impl {
		namespace Interface {
			namespace RayTestContext {
				void CLOAK_CALL Initialize();
				void CLOAK_CALL Release();
				API::Interface::IBasicGUI* CLOAK_CALL FindGUIAt(In const API::Global::Math::Space2D::Point& pos);
			}
		}
	}
}

#endif