#pragma once
#ifndef CE_TEST_MESHCOMPILER_H
#define CE_TEST_MESHCOMPILER_H

#include "CloakEngine/CloakEngine.h"
#include "CompilerBase.h"
#include "CloakCompiler/Image.h"

#include <string>

namespace MeshCompiler {
	bool CLOAK_CALL readMaterialFromConfig(CloakCompiler::Image::ImageInfo* imi, CloakEngine::Files::IValue* mat, unsigned int blockSize, std::u16string location = u"");
	void CLOAK_CALL compileIn(std::string inputPath, std::string outputPath, std::string tempPath, CompilerFlag flags = CompilerFlags::None, const CloakEngine::Files::CompressType* compress = nullptr);
}

#endif