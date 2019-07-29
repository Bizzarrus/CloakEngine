#pragma once
#ifndef CE_TEST_IMAGECOMPILER_H
#define CE_TEST_IMAGECOMPILER_H

#include "CloakEngine/CloakEngine.h"
#include "CompilerBase.h"
#include "CloakCompiler/Image.h"

#include <string>

namespace ImageCompiler {
	void CLOAK_CALL compilePrepared(CloakEngine::Files::IWriter* write, CloakCompiler::Image::Desc desc, const CloakCompiler::EncodeDesc& encode, const CloakEngine::Files::FileInfo& fi);
	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags = CompilerFlags::None, const CloakEngine::Files::CompressType* compress = nullptr);
}

#endif