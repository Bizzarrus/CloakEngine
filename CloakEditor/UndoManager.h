#pragma once
#ifndef CE_EDIT_UNDOMANAGER_H
#define CE_EDIT_UNDOMANAGER_H

#include "CloakEngine/CloakEngine.h"

namespace Editor {
	class IUndoable {
		public:
			virtual void CLOAK_CALL_THIS Do() = 0;
			virtual void CLOAK_CALL_THIS Undo() = 0;
	};
	namespace UndoManager {
		void CLOAK_CALL OnStart();
		void CLOAK_CALL OnStop();
		void CLOAK_CALL AddAction(In IUndoable* a);
		void CLOAK_CALL Undo();
		void CLOAK_CALL Redo();
		bool CLOAK_CALL CanUndo();
		bool CLOAK_CALL CanRedo();
	}
}

#endif