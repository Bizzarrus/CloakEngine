// stdafx.h: Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Komponenten aus Windows-Headern ausschließen
// Windows-Headerdateien:
#include <windows.h>


// TODO: Hier auf zusätzliche Header, die das Programm erfordert, verweisen.
#define DATA_PATH(path) (static_cast<std::u16string>(u"root:Data/CloakEngineTest/")+u##path)
#define COMP_PATH(path) (static_cast<std::string>("root:Compiler/")+##path)
