#pragma once
#ifndef CE_TEST_FONTCOMPILER_H
#define CE_TEST_FONTCOMPILER_H

#include "CloakEngine/CloakEngine.h"
#include "CompilerBase.h"

namespace FontCompiler {
	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags = CompilerFlags::None, const CloakEngine::Files::CompressType* compress = nullptr);
}

#endif
