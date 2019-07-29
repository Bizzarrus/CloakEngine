#pragma once
#ifndef CC_E_TEMPHANDLER_H
#define CC_E_TEMPHANDLER_H

#include "CloakCompiler/Defines.h"
#include "CloakEngine/CloakEngine.h"

#include <string>
#include <vector>

namespace CloakCompiler {
	namespace Engine {
		namespace TempHandler {
			struct FileDateInfo {
				ULARGE_INTEGER CreateDate;
				ULARGE_INTEGER LastChange;
				bool HasDate;
				CLOAK_CALL FileDateInfo();
			};
			void CLOAK_CALL ReadDates(const std::vector<std::u16string>& files, std::vector<FileDateInfo>* dates);
			bool CLOAK_CALL CheckDateChanges(const std::vector<FileDateInfo>& dates, CloakEngine::Files::IReader* read);
			bool CLOAK_CALL CheckDateChanges(const std::vector<FileDateInfo>& dates, CloakEngine::Files::IReader* read, const CloakEngine::Files::UGID& targetGameID);
			void CLOAK_CALL WriteDates(const std::vector<FileDateInfo>& dates, CloakEngine::Files::IWriter* write);
			void CLOAK_CALL WriteDates(const std::vector<FileDateInfo>& dates, CloakEngine::Files::IWriter* write, const CloakEngine::Files::UGID& targetGameID);
			void CLOAK_CALL WriteGameID(CloakEngine::Files::IWriter* write, const CloakEngine::Files::UGID& targetGameID);
			bool CLOAK_CALL CheckGameID(CloakEngine::Files::IReader* read, const CloakEngine::Files::UGID& targetGameID);
		}
	}
}

#endif