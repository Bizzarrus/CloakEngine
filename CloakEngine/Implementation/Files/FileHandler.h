#pragma once
#ifndef CE_IMPL_FILES_FILEHANDLER_H
#define CE_IMPL_FILES_FILEHANDLER_H

#include "CloakEngine/Files/FileHandler.h"
#include "Implementation/Helper/SavePtr.h"
#include "Engine/FileLoader.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Files/Compressor.h"

#include "Implementation/Helper/EventBase.h"

#include <string>
#include <atomic>

#define DECLARE_FILE_HANDLER(Name,uuid) class CLOAK_UUID(uuid) Name##File : public FileHandler_v1::FileHandler, public Name {public: CLOAK_CALL Name##File(In const API::Files::Buffer_v1::UFI& ufi);virtual CLOAK_CALL ~Name##File();protected: Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;virtual inline const API::Files::FileType& CLOAK_CALL_THIS getFileType() const override;}
#define IMPLEMENT_FILE_HANDLER(Name) CLOAK_CALL Name##File::Name##File(In const API::Files::Buffer_v1::UFI& ufi) : FileHandler(ufi) {}\
CLOAK_CALL Name##File::~Name##File(){}\
Success(return == true) bool CLOAK_CALL_THIS Name##File::iQueryInterface(In REFIID riid, Outptr void** ptr){\
if (riid == __uuidof(Name##File)) { *ptr = (Name##File*)this; return true; }\
else if (riid == __uuidof(Impl::Files::FileHandler_v1::FileHandler)) { *ptr = (Impl::Files::FileHandler_v1::FileHandler*)this; return true; }\
return Name::iQueryInterface(riid, ptr);}\
inline const API::Files::FileType& CLOAK_CALL_THIS Name##File::getFileType() const { return g_fileType; }

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace FileHandler_v1 {
				void CLOAK_CALL InitializeFileHandler();
				void CLOAK_CALL TerminateFileHandler();
				enum class LoadResult { Finished, KeepLoading, Waiting };
				class CLOAK_UUID("{EA0A6E4D-C225-4FE5-A93D-B6944BE2AC78}") RessourceHandler : public virtual API::Files::FileHandler_v1::IFileHandler, public Helper::SavePtr_v1::SavePtr, public virtual Engine::FileLoader::ILoad, IMPL_EVENT(onLoad), IMPL_EVENT(onUnload) {
					public:
						CLOAK_CALL_THIS RessourceHandler();
						virtual CLOAK_CALL_THIS ~RessourceHandler();
						virtual ULONG CLOAK_CALL_THIS load(In_opt API::Files::Priority prio = API::Files::Priority::LOW) override;
						virtual ULONG CLOAK_CALL_THIS unload() override;
						virtual bool CLOAK_CALL_THIS isLoaded(In API::Files::Priority prio) override;
						virtual bool CLOAK_CALL_THIS isLoaded() const override;
						virtual API::Global::Debug::Error CLOAK_CALL_THIS Delete() override;

						virtual void CLOAK_CALL_THIS loadDelayed();
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual LoadResult CLOAK_CALL_THIS iLoad(In API::Files::IReader* read, In API::Files::FileVersion version) = 0;
						virtual LoadResult CLOAK_CALL_THIS iLoadDelayed(In API::Files::IReader* read, In API::Files::FileVersion version);
						virtual void CLOAK_CALL_THIS iUnload() = 0;

						std::atomic<ULONG> m_load;
						API::Helper::ISyncSection* m_sync;
				};
				class CLOAK_UUID("{3F6B9C0A-A4FD-491F-BDE8-257DFB6BD1E9}") FileHandler : public virtual RessourceHandler {
					public:
						CLOAK_CALL_THIS FileHandler(In const API::Files::Buffer_v1::UFI& ufi);
						virtual CLOAK_CALL_THIS ~FileHandler();
					protected:
						virtual bool CLOAK_CALL_THIS iLoadFile(Out bool* keepLoadingBackground) override;
						virtual void CLOAK_CALL_THIS iUnloadFile() override;
						virtual void CLOAK_CALL_THIS iDelayedLoadFile(Out bool* keepLoadingBackground) override;
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						virtual inline const API::Files::FileType& CLOAK_CALL_THIS getFileType() const = 0;

						const API::Files::Buffer_v1::UFI m_filePath;
						API::Files::IReader* m_read;
						API::Files::FileVersion m_version;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif