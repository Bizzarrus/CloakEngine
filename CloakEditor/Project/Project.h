#pragma once
#ifndef CE_EDIT_PROJECT_PROJECT_H
#define CE_EDIT_PROJECT_PROJECT_H

#include "CloakEngine/CloakEngine.h"
#include "FileTree.h"

namespace Editor {
	namespace Project {
		class Project {
			public:
				CLOAK_CALL Project(In const std::u16string& name);
				FileTree* CLOAK_CALL_THIS GetFileTree();
			protected:
				FileTree m_tree;
		};
	}
}

#endif