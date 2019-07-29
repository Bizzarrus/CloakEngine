#include "stdafx.h"
#include "Implementation/Files/FileHandler.h"

#include "Implementation/Files/Font.h"
#include "Implementation/Files/Image.h"
#include "Implementation/Files/Language.h"
#include "Implementation/Files/Mesh.h"
#include "Implementation/Files/Shader.h"

#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/StringConvert.h"

#include <strsafe.h>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <unordered_map>

#include <sstream>

namespace std {
	template<> struct hash<CloakEngine::API::Files::Buffer_v1::UFI>
	{
		size_t CLOAK_CALL operator()(In const CloakEngine::API::Files::Buffer_v1::UFI& u) const { return u.GetHash(); }
	};
}

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace FileHandler_v1 {
				API::Helper::ISyncSection* g_sync = nullptr;
				API::HashMap<API::Files::Buffer_v1::UFI, FileHandler*>* g_loadedFiles;

				void CLOAK_CALL listDictionary(In const std::u16string& dict, In const std::u16string& extDict, In const std::u16string& typ, Inout API::List<API::Files::FileHandler_v1::FileInfo>* paths)
				{
					WIN32_FIND_DATA wfd;
					TCHAR rdir[MAX_PATH];
					StringCchCopy(rdir, MAX_PATH, API::Helper::StringConvert::ConvertToW(dict + u"*").c_str());
					HANDLE hFind = FindFirstFile(rdir, &wfd);
					if (hFind != INVALID_HANDLE_VALUE)
					{
						do {
							std::u16string name = API::Helper::StringConvert::ConvertToU16(wfd.cFileName);
							if (name.length() > 0 && (name.length() != 2 || name.compare(u"..") != 0) && (name.length() != 1 || name.compare(u".") != 0))
							{
								if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { listDictionary(dict + name + u"\\", name + u"\\", typ, paths); }
								else
								{
									size_t f = name.find_last_of(u".");
									std::u16string ext = u"";
									if (f != name.npos) 
									{ 
										ext = name.substr(f + 1); 
										name = name.substr(0, f);
									}
									std::transform(ext.begin(), ext.end(), ext.begin(), tolower);

									API::Files::FileHandler_v1::FileInfo fi;
									fi.RootPath = dict;
									fi.FullPath = dict + name + u"." + ext;
									fi.RelativePath = extDict + name;
									fi.Name = name;
									fi.Typ = ext;
									if (typ.length() == 0 || ext.compare(typ) == 0) { paths->push_back(fi); }
								}
							}
						} while (FindNextFile(hFind, &wfd) != 0);
						FindClose(hFind);
					}
				}
				void CLOAK_CALL InitializeFileHandler()
				{
					g_loadedFiles = new API::HashMap<API::Files::Buffer_v1::UFI, FileHandler*>();
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync));
				}
				void CLOAK_CALL TerminateFileHandler()
				{
					SAVE_RELEASE(g_sync);
					delete g_loadedFiles;
				}

				CLOAK_CALL_THIS RessourceHandler::RessourceHandler()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					INIT_EVENT(onUnload, m_sync);
					INIT_EVENT(onLoad, m_sync);
					m_load = 0;
					DEBUG_NAME(RessourceHandler);
				}
				CLOAK_CALL_THIS RessourceHandler::~RessourceHandler()
				{
					SAVE_RELEASE(m_sync);
				}
				ULONG CLOAK_CALL_THIS RessourceHandler::load(In_opt API::Files::Priority prio)
				{
					const ULONG r = m_load.fetch_add(1) + 1;
					if (r == 1) { iRequestLoad(prio); }
					else { iSetPriority(prio); }
					return r;
				}
				ULONG CLOAK_CALL_THIS RessourceHandler::unload()
				{
					const ULONG r = m_load.fetch_sub(1);
					if (r == 1) { iRequestUnload(); }
					else if (r == 0) { m_load = 0; }
					return r;
				}
				bool CLOAK_CALL_THIS RessourceHandler::isLoaded(In API::Files::Priority prio)
				{
					if (m_load == 0) { return false; }
					if (!m_isLoaded)
					{
						iSetPriority(prio);
						return false;
					}
					return true;
				}
				bool CLOAK_CALL_THIS RessourceHandler::isLoaded() const { return m_isLoaded; }
				API::Global::Debug::Error CLOAK_CALL_THIS RessourceHandler::Delete()
				{
					if (isLoaded()) { onUnload(); }
					return SavePtr::Delete();
				}

				void CLOAK_CALL_THIS RessourceHandler::loadDelayed()
				{
					if (m_load > 0) { iRequestDelayedLoad(); }
				}

				LoadResult CLOAK_CALL_THIS RessourceHandler::iLoadDelayed(In API::Files::IReader* read, In API::Files::FileVersion version) { return LoadResult::Finished; }
				Success(return == true) bool CLOAK_CALL_THIS RessourceHandler::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::FileHandler_v1::IFileHandler)) { *ptr = (API::Files::FileHandler_v1::IFileHandler*)this; return true; }
					else if (riid == __uuidof(Impl::Files::FileHandler_v1::RessourceHandler)) { *ptr = (Impl::Files::FileHandler_v1::RessourceHandler*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL_THIS FileHandler::FileHandler(In const API::Files::Buffer_v1::UFI& path) : m_filePath(path)
				{
					m_read = nullptr;
					m_version = 0;
				}
				CLOAK_CALL_THIS FileHandler::~FileHandler()
				{
					SAVE_RELEASE(m_read);
					API::Helper::Lock lock(g_sync);
					for (auto it = g_loadedFiles->begin(); it != g_loadedFiles->end(); it++)
					{
						if (it->second == this) 
						{ 
							g_loadedFiles->erase(it);
							break;
						}
					}
				}

				bool CLOAK_CALL_THIS FileHandler::iLoadFile(Out bool* keepLoadingBackground)
				{
					bool res = false;
					*keepLoadingBackground = false;
					API::Helper::WriteLock lock(m_sync);
					API::Files::IReader* read = nullptr;
					if (CREATE_INTERFACE(CE_QUERY_ARGS(&read)) == API::Global::Debug::Error::OK)
					{
						m_version = read->SetTarget(m_filePath, getFileType());
						if (m_version > 0)
						{
							res = true;
							LoadResult klb = iLoad(read, m_version);
							*keepLoadingBackground = (klb == LoadResult::KeepLoading);
							if (klb != LoadResult::Finished && m_version > 0) 
							{ 
								CLOAK_ASSUME(m_read == nullptr);
								m_read = read; 
								m_read->AddRef(); 
							}
							API::Global::Events::onLoad ol;
							triggerEvent(ol);
						}
					}
					SAVE_RELEASE(read);
					return res;
				}
				void CLOAK_CALL_THIS FileHandler::iUnloadFile()
				{
					API::Helper::WriteLock lock(m_sync);
					API::Global::Events::onUnload ol;
					triggerEvent(ol);
					iUnload();
					SAVE_RELEASE(m_read);
					m_version = 0;
				}
				void CLOAK_CALL_THIS FileHandler::iDelayedLoadFile(Out bool* keepLoadingBackground)
				{
					API::Helper::WriteLock lock(m_sync);
					if (m_version > 0)
					{
						CLOAK_ASSUME(m_read != nullptr);
						LoadResult klb = iLoadDelayed(m_read, m_version);
						*keepLoadingBackground = (klb == LoadResult::KeepLoading);
						if (klb == LoadResult::Finished) { SAVE_RELEASE(m_read); m_version = 0; }
					}
				}
				Success(return == true) bool CLOAK_CALL_THIS FileHandler::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(Impl::Files::FileHandler_v1::FileHandler)) { *ptr = (Impl::Files::FileHandler_v1::FileHandler*)this; return true; }
					return RessourceHandler::iQueryInterface(riid, ptr);
				}
			}
		}
	}
	namespace API {
		namespace Files {
			namespace FileHandler_v1 {
				CLOAKENGINE_API void CLOAK_CALL listAllFilesInDictionary(In const API::Files::Buffer_v1::UFI& ufi, In std::u16string fileTyp, Out API::List<FileInfo>* paths)
				{
					if (CloakCheckError(paths != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false)) { return; }
					paths->clear();
					std::u16string dict = API::Helper::StringConvert::ConvertToU16(ufi.GetPath());
					if (dict.length() > 0)
					{
						if (dict[dict.length() - 1] != u'/' && dict[dict.length() - 1] != u'\\') { dict += u'/'; }
						std::u16string t = fileTyp;
						std::transform(t.begin(), t.end(), t.begin(), tolower);
						Impl::Files::FileHandler_v1::listDictionary(dict, u"", t, paths);
					}
				}
				CLOAKENGINE_API void CLOAK_CALL listAllFilesInDictionary(In const API::Files::Buffer_v1::UFI& ufi, Out API::List<FileInfo>* paths) { listAllFilesInDictionary(ufi, u"", paths); }

#define CHECK_FILE(cl, ri,h,path, vers, ft) if(cl==__uuidof(Impl::Files::vers::ft##File) || ri==__uuidof(API::Files::vers::I##ft)){h=new Impl::Files::vers::ft##File(path);} else

				CLOAKENGINE_API Global::Debug::Error CLOAK_CALL OpenFile(In const API::Files::Buffer_v1::UFI& ufi, In REFIID riid, Outptr void** ptr) { return OpenFile(ufi, riid, riid, ptr); }
				CLOAKENGINE_API Global::Debug::Error CLOAK_CALL OpenFile(In const API::Files::Buffer_v1::UFI& ufi, In REFIID classRiid, In REFIID riid, Outptr void** ptr)
				{
					API::Helper::Lock lock(Impl::Files::g_sync);
					auto it = Impl::Files::g_loadedFiles->find(ufi);
					if (it != Impl::Files::g_loadedFiles->end())
					{
						Impl::Files::FileHandler* h = it->second;
						HRESULT hRet = h->QueryInterface(riid, ptr);
						return SUCCEEDED(hRet) ? Global::Debug::Error::OK : Global::Debug::Error::NO_INTERFACE;
					}
					else
					{
						Impl::Files::FileHandler* h = nullptr;
						CHECK_FILE(classRiid, riid, h, ufi, Font_v2, Font)
						CHECK_FILE(classRiid, riid, h, ufi, Image_v2, Image)
						CHECK_FILE(classRiid, riid, h, ufi, Language_v2, Language)
						CHECK_FILE(classRiid, riid, h, ufi, Mesh_v1, Mesh)
						CHECK_FILE(classRiid, riid, h, ufi, Shader_v1, Shader)
						{ return Global::Debug::Error::NO_CLASS; }
						(*Impl::Files::g_loadedFiles)[ufi] = h;
						HRESULT hRet = h->QueryInterface(riid, ptr);
						h->Release();
						return SUCCEEDED(hRet) ? Global::Debug::Error::OK : Global::Debug::Error::NO_INTERFACE;
					}
				}
			}
		}
	}
}