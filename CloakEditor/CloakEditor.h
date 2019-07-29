#pragma once
#ifndef CE_EDIT_H
#define CE_EDIT_H

#include "CloakEngine/CloakEngine.h"
#include "AFXCore.h"
#include "Project/Project.h"

#define C_EDIT_VERSION "1000"

namespace Editor {
	void CLOAK_CALL Stop();
	CWinAppEx* CLOAK_CALL GetApp();
	void CLOAK_CALL SetProject(In Project::Project* p);
	void CLOAK_CALL SetStatus(In const std::string& str, In int pos);
	namespace Console {
		void CLOAK_CALL WriteToLog(In const std::u16string& str);
		void CLOAK_CALL WriteToLog(In const std::string& str);
	}
	namespace Document {
		void CLOAK_CALL OpenMesh(In_opt std::string file = "");
		void CLOAK_CALL OpenImage(In_opt std::string file = "");
	}
}

#endif