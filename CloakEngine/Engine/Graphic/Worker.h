#pragma once
#ifndef CE_E_GRAPHIC_WORKER_H
#define CE_E_GRAPHIC_WORKER_H

#include "CloakEngine/Defines.h"

#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Rendering/RenderWorker.h"

namespace CloakEngine {
	namespace Engine {
		namespace Graphic {
			Impl::Rendering::IRenderWorker* CLOAK_CALL GetWorker();
		}
	}
}

#endif