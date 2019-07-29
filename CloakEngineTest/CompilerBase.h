#pragma once
#ifndef CE_TEST_COMPILERBASE_H
#define CE_TEST_COMPILERBASE_H

#include "CloakEngine/CloakEngine.h"

typedef unsigned int CompilerFlag;
namespace CompilerFlags {
	const CompilerFlag None = 0x00;
	const CompilerFlag ForceRecompile = 0x01;
	const CompilerFlag NoTempFile = 0x02;
}
namespace GameID {
	// {3D2BC168-3E85-4C3B-87EE-E475FE0B9573}
	static const GUID Test = { 0x3d2bc168, 0x3e85, 0x4c3b, { 0x87, 0xee, 0xe4, 0x75, 0xfe, 0xb, 0x95, 0x73 } };
	// {6EFD9477-F866-40A7-8280-11FE5AD5287D}
	static const GUID Editor = { 0x6efd9477, 0xf866, 0x40a7, { 0x82, 0x80, 0x11, 0xfe, 0x5a, 0xd5, 0x28, 0x7d } };
}
#endif
