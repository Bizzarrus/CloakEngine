#include "stdafx.h"
#include "CloakCallback/CEDebug.h"
#include "CloakEditor.h"

namespace Editor {
	void CLOAK_CALL_THIS CEDebug::Delete() { delete this; }
	void CLOAK_CALL_THIS CEDebug::OnLogWrite(In const std::string& msg)
	{
		Console::WriteToLog(msg);
	}
}
