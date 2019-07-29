#pragma once
#ifndef CC_E_IMAGE_ENTRY_H
#define CC_E_IMAGE_ENTRY_H

#include "CloakCompiler/Image.h"
#include "Engine/Image/Core.h"
#include "Engine/LibWriter.h"

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			inline namespace v1008 {
				CLOAK_INTERFACE_BASIC IEntry{
					public:
						virtual CLOAK_CALL ~IEntry() {}
						virtual bool CLOAK_CALL_THIS GotError() const = 0;
						virtual void CLOAK_CALL_THIS CheckTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum) = 0;
						virtual void CLOAK_CALL_THIS WriteTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum) = 0;
						virtual void CLOAK_CALL_THIS Encode(In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum) = 0;
						virtual void CLOAK_CALL_THIS WriteHeader(In const CE::RefPointer<CloakEngine::Files::IWriter>& write) = 0;
						virtual size_t CLOAK_CALL_THIS GetMaxLayerLength() = 0;
						virtual void CLOAK_CALL_THIS WriteLayer(In const CE::RefPointer<CloakEngine::Files::IWriter>& write, In size_t layer) = 0;
						virtual void CLOAK_CALL_THIS WriteLibFiles(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const API::Image::LibInfo& lib, In const BitInfo& bits, In size_t entryID, In const Engine::Lib::CMakeFile& cmake, In const CE::Global::Task& finishTask) = 0;
				};
			}
		}
	}
}

#endif