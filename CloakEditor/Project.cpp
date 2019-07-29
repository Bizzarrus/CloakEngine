#include "stdafx.h"
#include "Project/Project.h"

namespace Editor {
	namespace Project {
		CLOAK_CALL Project::Project(In const std::u16string& name) : m_tree(name)
		{

		}
		FileTree* CLOAK_CALL_THIS Project::GetFileTree()
		{
			return &m_tree;
		}
	}
}