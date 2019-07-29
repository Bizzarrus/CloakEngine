#pragma once
#ifndef CE_API_GLOBAL_STEAM_H
#define CE_API_GLOBAL_STEAM_H

#include "CloakEngine/Defines.h"

#include <string>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Steam {
				struct Achievement {
					std::string Name;
					std::string Description;
					bool Achieved;
				};
				bool CLOAKENGINE_API IsConnected();
				namespace Stat {
					enum class Type { INT, FLOAT, AVERANGE };
					void CLOAKENGINE_API Register(In unsigned int ID, In const char* strID, In Type type);
					void CLOAKENGINE_API SetValue(In unsigned int ID, In int value);
					void CLOAKENGINE_API SetValue(In unsigned int ID, In float value);
					void CLOAKENGINE_API SetValue(In unsigned int ID, In float value, In double time);
					void CLOAKENGINE_API GetValue(In unsigned int ID, Out int* value);
					void CLOAKENGINE_API GetValue(In unsigned int ID, Out float* value);
				}
				namespace Achievements {
					void CLOAKENGINE_API Register(In unsigned int ID, In const char* strID, In const Achievement& a);
					bool CLOAKENGINE_API Get(In unsigned int ID, Out Achievement* a);
				}
			}
		}
	}
}

#endif