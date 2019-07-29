// stdafx.h: Includedatei f�r Standardsystem-Includedateien
// oder h�ufig verwendete projektspezifische Includedateien,
// die nur in unregelm��igen Abst�nden ge�ndert werden.
//

#pragma once

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#define _ENABLE_ATOMIC_ALIGNMENT_FIX
//Memory leak detection:
#ifdef _DEBUG
#include <stdlib.h>
#endif

#define WIN32_LEAN_AND_MEAN             // Selten verwendete Komponenten aus Windows-Headern ausschlie�en
// Windows-Headerdateien:
#include <windows.h>
#include <math.h>
#include <string.h>
#include <wchar.h>
#include <stdlib.h>


#include "CloakEngine/Defines.h"
#ifndef NO_INTRINSIC
#pragma intrinsic(fabs,strcmp,labs,strcpy,_rotl,memcmp,strlen,_rotr,memcpy,_lrotl,_strset,memset,_lrotr,abs,strcat)
#pragma intrinsic(acos,cosh,pow,tanh,asin,fmod,sinh)
#pragma intrinsic(atan,exp,log10,sqrt,atan2,log,sin,tan,cos)
#ifdef X64
#pragma intrinsic(sinf,tanf,cosf)
#endif
#endif

#include "CloakEngine/Global/Memory.h"
// TODO: Hier auf zus�tzliche Header, die das Programm erfordert, verweisen.
namespace std {
	CLOAK_FORCEINLINE std::string to_string(In bool r) { return r ? "true" : "false"; }
}