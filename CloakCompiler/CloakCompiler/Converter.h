#pragma once
#ifndef CC_API_CONVERTER_H
#define CC_API_CONVERTER_H

#include "CloakCompiler/Defines.h"
#include "CloakCompiler/Mesh.h"
#include "CloakCompiler/Image.h"

#include <vector>

namespace CloakCompiler {
	CLOAKCOMPILER_API_NAMESPACE namespace API {
		namespace Converter {
			CLOAKCOMPILER_API bool CLOAK_CALL ConvertOBJFile(In const std::u16string& filePath, In Image::SupportedQuality quality, In unsigned int blockSize, Out Mesh::Desc* mesh, Out std::vector<Image::ImageInfo>* materials, Out std::string* error, In_opt std::u16string matPath = u"");
		}
	}
}

#endif