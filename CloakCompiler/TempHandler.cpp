#include "stdafx.h"
#include "Engine/TempHandler.h"

#include "CloakEngine/Global/Debug.h"

#include <codecvt>

namespace CloakCompiler {
	namespace Engine {
		namespace TempHandler {
			CLOAK_CALL FileDateInfo::FileDateInfo()
			{
				CreateDate.QuadPart = 0;
				LastChange.QuadPart = 0;
				HasDate = false;
			}
			void CLOAK_CALL ReadDates(const std::vector<std::u16string>& files, std::vector<FileDateInfo>* dates)
			{
				if (CloakCheckOK(dates != nullptr, CloakEngine::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
				{
					dates->resize(files.size());
					for (size_t a = 0; a < files.size(); a++)
					{
						dates->at(a) = FileDateInfo();
						HANDLE hFile = CreateFile(CloakEngine::Helper::StringConvert::ConvertToW(files[a]).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
						if (hFile != INVALID_HANDLE_VALUE)
						{
							FILETIME createTime;
							FILETIME lastChangeTime;
							if (GetFileTime(hFile, &createTime, nullptr, &lastChangeTime) != 0)
							{
								dates->at(a).CreateDate.HighPart = createTime.dwHighDateTime;
								dates->at(a).CreateDate.LowPart = createTime.dwLowDateTime;
								dates->at(a).LastChange.HighPart = lastChangeTime.dwHighDateTime;
								dates->at(a).LastChange.LowPart = lastChangeTime.dwLowDateTime;
								dates->at(a).HasDate = true;
							}
							CloseHandle(hFile);
						}
					}
				}
			}
			bool CLOAK_CALL CheckDateChanges(const std::vector<FileDateInfo>& dates, CloakEngine::Files::IReader* read)
			{
				if (read != nullptr)
				{
					size_t rdn = static_cast<size_t>(read->ReadBits(32));
					bool res = rdn != dates.size();
					for (size_t a = 0; res == false && a < dates.size(); a++)
					{
						DWORD hwCreate = static_cast<DWORD>(read->ReadBits(32));
						DWORD lwCreate = static_cast<DWORD>(read->ReadBits(32));
						DWORD hwChange = static_cast<DWORD>(read->ReadBits(32));
						DWORD lwChange = static_cast<DWORD>(read->ReadBits(32));
						const FileDateInfo& fdi = dates[a];
						if (fdi.LastChange.HighPart != hwChange || fdi.LastChange.LowPart != lwChange || fdi.CreateDate.HighPart != hwCreate || fdi.CreateDate.LowPart != lwCreate) { res = true; }
					}
					return res;
				}
				return true;
			}
			bool CLOAK_CALL CheckDateChanges(const std::vector<FileDateInfo>& dates, CloakEngine::Files::IReader* read, const CloakEngine::Files::UGID& targetGameID)
			{
				return CheckDateChanges(dates, read) || CheckGameID(read, targetGameID);
			}
			void CLOAK_CALL WriteDates(const std::vector<FileDateInfo>& dates, CloakEngine::Files::IWriter* write)
			{
				if (write != nullptr)
				{
					write->WriteBits(32, dates.size());
					for (size_t a = 0; a < dates.size(); a++)
					{
						const Engine::TempHandler::FileDateInfo& fcdi = dates[a];
						write->WriteBits(32, fcdi.CreateDate.HighPart);
						write->WriteBits(32, fcdi.CreateDate.LowPart);
						write->WriteBits(32, fcdi.LastChange.HighPart);
						write->WriteBits(32, fcdi.LastChange.LowPart);
					}
				}
			}
			void CLOAK_CALL WriteDates(const std::vector<FileDateInfo>& dates, CloakEngine::Files::IWriter* write, const CloakEngine::Files::UGID& targetGameID)
			{
				WriteDates(dates, write);
				WriteGameID(write, targetGameID);
			}
			void CLOAK_CALL WriteGameID(CloakEngine::Files::IWriter* write, const CloakEngine::Files::UGID& targetGameID)
			{
				if (write != nullptr)
				{
					write->WriteGameID(targetGameID);
				}
			}
			bool CLOAK_CALL CheckGameID(CloakEngine::Files::IReader* read, const CloakEngine::Files::UGID& targetGameID)
			{
				if (read != nullptr)
				{
					return !read->ReadGameID(targetGameID);
				}
				return true;
			}
		}
	}
}