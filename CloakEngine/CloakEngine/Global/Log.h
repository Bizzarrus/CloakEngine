#pragma once
#ifndef CE_API_GLOBAL_LOG_H
#define CE_API_GLOBAL_LOG_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Memory.h"

#include <string>

#ifdef CLOAKENGINE_EXPORTS
#define CloakDefaultLogSource CloakEngine::API::Global::Log::Source::Engine
#else
#define CloakDefaultLogSource CloakEngine::API::Global::Log::Source::Game
#endif

#define CloakDefaultLogType CloakEngine::API::Global::Log::Type::Info

#ifdef _DEBUG
#define CloakDebugLog(text) CloakEngine::API::Global::Log::WriteToLog(static_cast<std::string>("[")+__FILE__+"]["+std::to_string(__LINE__)+"]: "+(text),CloakEngine::API::Global::Log::Type::Debug)
#else
#define CloakDebugLog(text) 
#endif

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Log {
				enum class Type {
					Info,
					Warning,
					Error,
					Debug,
					Console,
					File,
				};
				enum class Source {
					Engine,
					Game,
					Modification,
				};
				CLOAKENGINE_API void CLOAK_CALL SetLogFilePath(In const std::u16string& path);
				CLOAKENGINE_API void CLOAK_CALL WriteToLog(In const std::string& str, In Source source, In_opt Type typ = CloakDefaultLogType);
				CLOAKENGINE_API void CLOAK_CALL WriteToLog(In const std::u16string& str, In Source source, In_opt Type typ = CloakDefaultLogType);
				CLOAKENGINE_API void CLOAK_CALL WriteToLog(In const std::string& str, In_opt Type typ = CloakDefaultLogType, In_opt Source source = CloakDefaultLogSource);
				CLOAKENGINE_API void CLOAK_CALL WriteToLog(In const std::u16string& str, In_opt Type typ = CloakDefaultLogType, In_opt Source source = CloakDefaultLogSource);
			}
		}
	}
}

#endif