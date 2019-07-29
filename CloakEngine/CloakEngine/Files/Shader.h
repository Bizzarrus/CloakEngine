#pragma once
#ifndef CE_API_FILES_SHADER_H
#define CE_API_FILES_SHADER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/FileHandler.h"

//TODO: Real-Time Deep Shadow Maps ( https://volumetricshadows.files.wordpress.com/2011\09\report.pdf ) for shadows of transparent
//objects

#define CE_SEP_SIMECOLON ;
#define CE_SEP_COMMA ,
#define CE_SEP_EMPTY 

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Shader_v1 {
				CLOAK_INTERFACE_ID("{F222F0A5-CB16-4C05-9D94-DE031496AD14}") IShader : public virtual Files::FileHandler_v1::IFileHandler {
					public:
						
				};
			}
		}
	}
}

#endif