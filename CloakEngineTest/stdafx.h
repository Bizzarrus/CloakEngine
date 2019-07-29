// stdafx.h: Includedatei f�r Standardsystem-Includedateien
// oder h�ufig verwendete projektspezifische Includedateien,
// die nur in unregelm��igen Abst�nden ge�ndert werden.
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Komponenten aus Windows-Headern ausschlie�en
// Windows-Headerdateien:
#include <windows.h>


// TODO: Hier auf zus�tzliche Header, die das Programm erfordert, verweisen.
#define DATA_PATH(path) (static_cast<std::u16string>(u"root:Data/CloakEngineTest/")+u##path)
#define COMP_PATH(path) (static_cast<std::string>("root:Compiler/")+##path)
