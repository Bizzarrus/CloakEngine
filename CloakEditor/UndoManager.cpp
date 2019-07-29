#include "stdafx.h"
#include "UndoManager.h"

#include <vector>

namespace Editor {
	namespace UndoManager {
		std::vector<IUndoable*> g_stack;
		size_t g_pos;

		void CLOAK_CALL OnStart()
		{
			g_pos = 0;
		}
		void CLOAK_CALL OnStop()
		{
			for (size_t a = 0; a < g_stack.size(); a++) { delete g_stack[a]; }
			g_pos = 0;
		}
		void CLOAK_CALL AddAction(In IUndoable* a)
		{
			if (a != nullptr)
			{
				if (g_stack.size() > g_pos)
				{
					delete g_stack[g_pos];
					g_stack[g_pos] = a;
				}
				else { g_stack.push_back(a); }
				g_pos++;
				a->Do();
			}
		}
		void CLOAK_CALL Undo()
		{
			if (CanUndo())
			{
				g_stack[--g_pos]->Undo();
			}
		}
		void CLOAK_CALL Redo()
		{
			if (CanRedo())
			{
				g_stack[g_pos++]->Do();
			}
		}
		bool CLOAK_CALL CanUndo()
		{
			return g_pos > 0;
		}
		bool CLOAK_CALL CanRedo()
		{
			return g_pos < g_stack.size();
		}
	}
}