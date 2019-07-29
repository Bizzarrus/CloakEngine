#include "stdafx.h"
#include "Implementation/Global/DebugLayer.h"

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace DebugLayer_v1 {
				CLOAK_CALL DebugLayer::DebugLayer()
				{

				}
				CLOAK_CALL DebugLayer::~DebugLayer()
				{

				}

				Success(return == true) bool CLOAK_CALL_THIS DebugLayer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SavePtr, Global::DebugLayer_v1, DebugLayer);
				}
			}
		}
	}
}