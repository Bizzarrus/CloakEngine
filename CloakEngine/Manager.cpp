#include "stdafx.h"
#define NO_LIB_LOAD
#include "Implementation/Rendering/Manager.h"
#include "Implementation/Rendering/DX12/Lib.h"

#include <atomic>

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Manager_v1 {
				IManager* CLOAK_CALL IManager::Create(In API::Global::Graphic::RenderMode Type)
				{
					IManager* r = nullptr;
					switch (Type)
					{
						case CloakEngine::API::Global::Graphic::RenderMode::DX12:
							r = DX12::Lib::CreateManager();
							if (r != nullptr) { break; }
						default:
							API::Global::Log::WriteToLog("No render type supported!", API::Global::Log::Type::Error);
							break;
					}
					return r;
				}
				void CLOAK_CALL IManager::GetSupportedModes(Out API::List<API::Global::Graphic::RenderMode>* modes)
				{

				}
			}
		}
	}
}