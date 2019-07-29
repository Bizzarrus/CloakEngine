#pragma once
#ifndef CE_TEST_SHADERCOMPILER_H
#define CE_TEST_SHADERCOMPILER_H

#include "CloakEngine/CloakEngine.h"
#include "CompilerBase.h"

#include <string>

namespace ShaderCompiler {
	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags = CompilerFlags::None, const CloakEngine::Files::CompressType* compress = nullptr);
}

#endif