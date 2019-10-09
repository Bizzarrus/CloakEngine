#pragma once
#ifndef CE_API_FILES_FILEHANDLER_H
#define CE_API_FILES_FILEHANDLER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Helper/EventBase.h"
#include "CloakEngine/Files/Buffer.h"

#include <string>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Events {
				struct onLoad {

				};
				struct onUnload {

				};
			}
		}
		namespace Files {
			inline namespace FileHandler_v1 {
				struct FileInfo {
					std::u16string RootPath;
					std::u16string FullPath;
					std::u16string RelativePath;
					std::u16string Name;
					std::u16string Typ;
				};
				enum class Priority : uint8_t { LOW = 1, NORMAL = 2, HIGH = 3 };
				enum class LOD { ULTRA = 0, HIGH = 1, NORMAL = 2, LOW = 3 };
				CLOAKENGINE_API void CLOAK_CALL listAllFilesInDictionary(In const API::Files::Buffer_v1::UFI& ufi, In std::u16string fileTyp, Out API::List<FileInfo>* paths);
				CLOAKENGINE_API void CLOAK_CALL listAllFilesInDictionary(In const API::Files::Buffer_v1::UFI& ufi, Out API::List<FileInfo>* paths);
				CLOAKENGINE_API Global::Debug::Error CLOAK_CALL OpenFile(In const API::Files::Buffer_v1::UFI& ufi, In REFIID riid, Outptr void** ptr);
				CLOAKENGINE_API Global::Debug::Error CLOAK_CALL OpenFile(In const API::Files::Buffer_v1::UFI& ufi, In REFIID classRiid, In REFIID riid, Outptr void** ptr);
				CLOAK_INTERFACE_ID("{F1E296A3-9E8E-494B-874C-0F450E91D09A}") IFileHandler : public virtual Helper::SavePtr_v1::ISavePtr, public virtual Helper::IEventCollector<Global::Events::onLoad, Global::Events::onUnload> {
					public:
						virtual ULONG CLOAK_CALL_THIS load(In_opt API::Files::Priority prio = API::Files::Priority::LOW) = 0;
						virtual ULONG CLOAK_CALL_THIS unload() = 0;
						virtual bool CLOAK_CALL_THIS isLoaded(In API::Files::Priority prio) = 0;
						virtual bool CLOAK_CALL_THIS isLoaded() const = 0;
				};
				CLOAK_INTERFACE_ID("{9FA8CE85-10C3-4980-ABE4-6138B313BAD0}") ILODFile : public virtual Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS RequestLOD(In LOD lod) = 0;
				};
			}
		}
	}
}

#endif