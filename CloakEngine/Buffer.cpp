#include "stdafx.h"
#include "Implementation/Files/Buffer.h"
#include "Implementation/Files/FileIOBuffer.h"
#include "Implementation/Global/Game.h"
#include "Implementation/Global/Memory.h"
#include "Implementation/Global/Task.h"
#include "Implementation/Helper/SyncSection.h"

#include "CloakEngine/Helper/StringConvert.h"
#include "CloakEngine/Helper/Types.h"
#include "CloakEngine/Files/Hash/SHA256.h"
#include "CloakEngine/Files/Hash/Blake2.h"
#include "CloakEngine/Files/Hash/FNV.h"
#include "CloakEngine/Helper/SyncSection.h"

#include "Engine/Compress/Huffman.h"
#include "Engine/Compress/LZC.h"
#include "Engine/Compress/LZW.h"
#include "Engine/Compress/UTF.h"
#include "Engine/Compress/AAC.h"
#include "Engine/Compress/LZ4.h"
#include "Engine/Compress/ZLIB.h"
#include "Engine/Compress/NetBuffer/HTTPBuffer.h"

#include <algorithm>
#include <sstream>
#include <stdlib.h>
#include <locale>
#include <ws2tcpip.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <filesystem>
#include <random>
#include <iomanip>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

//#define USE_THREADPOOLIO
//#define PRINT_DEBUG_INFO

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace Buffer_v1 {
#ifdef _DEBUG
				constexpr size_t ROOT_PATH_OFFSET = 4;
#else
				constexpr size_t ROOT_PATH_OFFSET = 3;
#endif
				std::string* g_rootPath = nullptr;
				API::Helper::ISyncSection* g_syncPath = nullptr;

				inline bool CLOAK_CALL CanWait() { return Impl::Global::Game::canThreadRun(); }
				inline std::string CLOAK_CALL GetEnvironmentPath(In const std::string& name)
				{
					size_t reqSize;
					getenv_s(&reqSize, nullptr, 0, name.c_str());
					if (reqSize == 0) { return "$" + name + "$"; }
					else
					{
						char* t = reinterpret_cast<char*>(API::Global::Memory::MemoryPool::Allocate(reqSize * sizeof(char)));
						getenv_s(&reqSize, t, reqSize, name.c_str());
						std::string res(t);
						API::Global::Memory::MemoryPool::Free(t);
						return res;
					}
				}
				inline std::string CLOAK_CALL GetRootPath()
				{
					API::Helper::Lock lock(g_syncPath);
					if (g_rootPath == nullptr)
					{
						//Release Modul path is [ROOT]/Bin/Windows 10/Win32
						//Debug Modul path is [ROOT]/Bin/Windows 10/Win32/W10 Debug
						std::string modulPath = Impl::Global::Game::getModulePath();
						size_t a = 0;
						size_t f;
						do {
							const size_t f1 = modulPath.find_last_of('/');
							const size_t f2 = modulPath.find_last_of('\\');
							f = f1 == modulPath.npos ? f2 : (f2 == modulPath.npos ? f1 : max(f1, f2));
							if (f != modulPath.npos) { modulPath = modulPath.substr(0, f); }
							a++;
						} while (f != modulPath.npos && a < ROOT_PATH_OFFSET);
						g_rootPath = new std::string(modulPath);
					}
					return *g_rootPath;
				}

				void CLOAK_CALL Initialize()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncPath));
				}
				void CLOAK_CALL Release()
				{
					SAVE_RELEASE(g_syncPath);
					delete g_rootPath;
				}

				CLOAK_CALL VirtualWriterBuffer::VirtualWriterBuffer()
				{
					DEBUG_NAME(VirtualWriterBuffer);
					m_rbCID = 0;
					m_start = nullptr;
					m_cur = nullptr;
					m_pos = 0;
				}
				CLOAK_CALL VirtualWriterBuffer::~VirtualWriterBuffer()
				{
					PAGE* p = m_start;
					while (p != nullptr)
					{
						PAGE* n = p->Next;
						API::Global::Memory::MemoryHeap::Free(p);
						p = n;
					}
				}
				unsigned long long CLOAK_CALL_THIS VirtualWriterBuffer::GetPosition() const
				{
					return m_pos;
				}
				void CLOAK_CALL_THIS VirtualWriterBuffer::WriteByte(In uint8_t b)
				{
					const uint64_t p = (m_pos++) % PAGE_SIZE;
					if (p == 0)
					{
						if (m_cur == nullptr || m_cur->Next == nullptr)
						{
							PAGE* np = reinterpret_cast<PAGE*>(API::Global::Memory::MemoryHeap::Allocate(FULL_PAGE_SIZE));
							np->Next = nullptr;
							if (m_cur == nullptr)
							{
								m_start = np;
							}
							else
							{
								m_cur->Next = np;
							}
							m_cur = np;
						}
						else
						{
							m_cur = m_cur->Next;
						}
					}
					reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(m_cur) + sizeof(PAGE))[p] = b;
				}
				void CLOAK_CALL_THIS VirtualWriterBuffer::Save()
				{
					
				}
				CE::RefPointer<API::Files::Buffer_v1::IReadBuffer> CLOAK_CALL_THIS VirtualWriterBuffer::GetReadBuffer()
				{
					return CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>::Construct<VirtualReaderBuffer>(this);
				}
				void CLOAK_CALL_THIS VirtualWriterBuffer::Clear()
				{
					m_pos = 0;
					m_cur = m_start;
					m_rbCID = (m_rbCID + 1) % 255;
				}

				Success(return == true) bool CLOAK_CALL_THIS VirtualWriterBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::IVirtualWriteBuffer)) { *ptr = (API::Files::IVirtualWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::IWriteBuffer)) { *ptr = (API::Files::IWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::IBuffer)) { *ptr = (API::Files::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL VirtualReaderBuffer::VirtualReaderBuffer(In VirtualWriterBuffer* src)
				{
					DEBUG_NAME(VirtualReaderBuffer);
					m_src = src;
					m_page = nullptr;
					if (m_src != nullptr) 
					{ 
						m_src->AddRef(); 
						m_page = m_src->m_start;
					}
					m_pos = 0;
					m_lrbCID = 0;
				}
				CLOAK_CALL VirtualReaderBuffer::~VirtualReaderBuffer()
				{
					SAVE_RELEASE(m_src);
				}
				unsigned long long CLOAK_CALL_THIS VirtualReaderBuffer::GetPosition() const
				{
					return m_pos;
				}
				uint8_t CLOAK_CALL_THIS VirtualReaderBuffer::ReadByte()
				{
					if (m_src != nullptr)
					{
						if (m_lrbCID != m_src->m_rbCID)
						{
							m_lrbCID = m_src->m_rbCID;
							m_pos = 0;
							m_page = m_src->m_start;
						}
						const uint64_t p = m_pos % VirtualWriterBuffer::PAGE_SIZE;
						if (m_pos > 0 && p == 0 && m_page != nullptr)
						{
							m_page = m_page->Next;
						}
						m_pos++;
						if (p < m_src->m_pos)
						{
							if (m_page == nullptr)
							{
								uint64_t i = m_pos - 1;
								m_page = m_src->m_start;
								while (i > VirtualWriterBuffer::PAGE_SIZE)
								{
									CLOAK_ASSUME(m_page != nullptr);
									m_page = m_page->Next;
									i -= VirtualWriterBuffer::PAGE_SIZE;
								}
								CLOAK_ASSUME(i == p);
							}
							if (m_page != nullptr)
							{
								return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(m_page) + sizeof(VirtualWriterBuffer::PAGE))[p];
							}
						}
					}
					return 0;
				}
				bool CLOAK_CALL_THIS VirtualReaderBuffer::HasNextByte()
				{
					if (m_src != nullptr)
					{
						if (m_lrbCID != m_src->m_rbCID)
						{
							m_lrbCID = m_src->m_rbCID;
							m_pos = 0;
							m_page = m_src->m_start;
						}
						return m_pos < m_src->m_pos;
					}
					return false;
				}

				Success(return == true) bool CLOAK_CALL_THIS VirtualReaderBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL MultithreadedWriteBuffer::MultithreadedWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& dst, In_opt std::function<void(In void* userData, In uint64_t size)> onWrite, In_opt API::Global::Threading::ScheduleHint writingHint) : m_onWrite(onWrite), m_writeTaskHint(writingHint), m_target(dst)
				{
					DEBUG_NAME(MultithreadedWriteBuffer);
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					m_readyToWriteBegin = nullptr;
					m_readyToWriteEnd = nullptr;
					m_hasTask = false;
					m_task = nullptr;
					m_finish = nullptr;
				}
				CLOAK_CALL MultithreadedWriteBuffer::~MultithreadedWriteBuffer()
				{
					CLOAK_ASSUME(m_task.Finished());
				}
				CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer> CLOAK_CALL_THIS MultithreadedWriteBuffer::CreateLocalWriteBuffer(In_opt void* userData)
				{
					return CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>::Construct<InternWriteBuffer>(this, userData);
				}
				CE::RefPointer<API::Files::Compressor_v2::IWriter> CLOAK_CALL_THIS MultithreadedWriteBuffer::CreateLocalWriter(In_opt void* userData)
				{
					CE::RefPointer<API::Files::Compressor_v2::IWriter> f = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&f));
					f->SetTarget(CreateLocalWriteBuffer(userData));
					return f;
				}
				void CLOAK_CALL_THIS MultithreadedWriteBuffer::SetFinishTask(In const API::Global::Task& task)
				{
					m_finish = task;
					m_finish.AddDependency(m_task);
					m_finish.Schedule();
				}
				void CLOAK_CALL_THIS MultithreadedWriteBuffer::OnWrite()
				{
					if (m_hasTask.exchange(false) == true)
					{
						API::Helper::Lock lock(m_sync);
						PageList* pl = m_readyToWriteBegin;
						m_readyToWriteBegin = nullptr;
						m_readyToWriteEnd = nullptr;
						lock.unlock();

						while (pl != nullptr)
						{
							PageList* cpl = pl;
							pl = pl->Next;

#ifdef _DEBUG
							uint64_t dbg_maxSize = PAGE_LIST_SIZE;
							Page* dbg_p = cpl->Page;
							while (dbg_p != nullptr)
							{
								CLOAK_ASSUME(cpl->Size >= dbg_maxSize);
								dbg_maxSize += PAGE_SIZE;
								dbg_p = dbg_p->Next;
							}
							CLOAK_ASSUME(cpl->Size <= dbg_maxSize);
#endif

							if (m_onWrite) { m_onWrite(cpl->UserData, cpl->Size); }

							uint8_t* data = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(cpl) + sizeof(PageList));
							uint64_t dataSize = min(cpl->Size, PAGE_LIST_SIZE);
							for (uint64_t a = 0; a < dataSize; a++) { m_target->WriteByte(data[a]); }
							cpl->Size -= dataSize;
							Page* p = cpl->Page;
							while (p != nullptr && cpl->Size > 0)
							{
								data = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(p) + sizeof(Page));
								dataSize = min(cpl->Size, PAGE_SIZE);
								for (uint64_t a = 0; a < dataSize; a++) { m_target->WriteByte(data[a]); }
								cpl->Size -= dataSize;
								Page* o = p;
								p = p->Next;
								API::Global::Memory::MemoryHeap::Free(o);
							}
							API::Global::Memory::MemoryHeap::Free(cpl);
						}
					}
				}

				Success(return == true) bool CLOAK_CALL_THIS MultithreadedWriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IMultithreadedWriteBuffer)) { *ptr = (API::Files::Buffer_v1::IMultithreadedWriteBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL MultithreadedWriteBuffer::InternWriteBuffer::InternWriteBuffer(In MultithreadedWriteBuffer* parent, In void* userData) : m_parent(parent), m_userData(userData)
				{
					m_curPage = nullptr;
					m_resList = nullptr;
					m_writePos = 0;
					m_writeSize = 0;
					m_bytePos = 0;
					m_dst = nullptr;
					m_refCount = 1;
					m_parent->AddRef();
				}
				CLOAK_CALL MultithreadedWriteBuffer::InternWriteBuffer::~InternWriteBuffer()
				{
					Save();
					m_parent->Release();
				}

				uint64_t CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::AddRef()
				{
					m_refCount++;
					return m_refCount;
				}
				uint64_t CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::Release()
				{
					CLOAK_ASSUME(m_refCount > 0);
					const uint64_t r = --m_refCount;
					if (r == 0) { delete this; }
					return r;
				}

				uint64_t CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::GetRefCount()
				{
					return m_refCount;
				}
				API::Global::Debug::Error CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::Delete()
				{
					return API::Global::Debug::Error::ILLEGAL_FUNC_CALL;
				}
				Success(return == S_OK) Post_satisfies(*ppvObject != nullptr) HRESULT STDMETHODCALLTYPE MultithreadedWriteBuffer::InternWriteBuffer::QueryInterface(In REFIID riid, Outptr void** ppvObject)
				{
					*ppvObject = nullptr;
					if (riid == __uuidof(API::Files::Buffer_v1::IWriteBuffer)) { *ppvObject = (API::Files::Buffer_v1::IWriteBuffer*)this; return S_OK; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ppvObject = (API::Files::Buffer_v1::IBuffer*)this; return S_OK; }
					else if (riid == __uuidof(API::Helper::SavePtr_v1::ISavePtr)) { *ppvObject = (API::Helper::SavePtr_v1::ISavePtr*)this; return S_OK; }
					return E_NOINTERFACE;
				}
				void CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::SetDebugName(In const std::string& s) {}
				void CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::SetSourceFile(In const std::string& s){}
				std::string CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::GetDebugName() const { return m_parent->GetDebugName(); }
				std::string CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::GetSourceFile() const { return m_parent->GetSourceFile(); }

				unsigned long long CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::GetPosition() const { return m_bytePos; }

				void CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::WriteByte(In uint8_t b)
				{
					if (m_writePos == m_writeSize)
					{
#ifdef _DEBUG
						uint64_t dbg_maxSize = PAGE_LIST_SIZE;
						if (m_resList != nullptr)
						{
							Page* dbg_p = m_resList->Page;
							while (dbg_p != nullptr)
							{
								CLOAK_ASSUME(m_bytePos >= dbg_maxSize);
								dbg_maxSize += PAGE_SIZE;
								dbg_p = dbg_p->Next;
							}
						}
						CLOAK_ASSUME(m_bytePos <= dbg_maxSize);
#endif

						void* ptr = API::Global::Memory::MemoryHeap::Allocate(FULL_PAGE_SIZE);
						if (m_resList == nullptr)
						{
							m_resList = reinterpret_cast<PageList*>(ptr);
							m_resList->Next = nullptr;
							m_resList->Page = nullptr;
							m_resList->Size = 0;
							m_resList->UserData = m_userData;
							m_writeSize = PAGE_LIST_SIZE;
							m_dst = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(m_resList) + sizeof(PageList));
						}
						else
						{
							Page* p = reinterpret_cast<Page*>(ptr);
							p->Next = nullptr;
							if (m_curPage != nullptr) { m_curPage->Next = p; }
							else { m_resList->Page = p; }
							m_curPage = p;
							m_writeSize = PAGE_SIZE;
							m_dst = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(p) + sizeof(Page));
						}
#ifdef _DEBUG
						dbg_maxSize = PAGE_LIST_SIZE;
						Page* dbg_p = m_resList->Page;
						while (dbg_p != nullptr)
						{
							CLOAK_ASSUME(m_bytePos + m_writePos >= dbg_maxSize);
							dbg_maxSize += PAGE_SIZE;
							dbg_p = dbg_p->Next;
						}
						CLOAK_ASSUME(m_bytePos + m_writePos <= dbg_maxSize);
#endif
						m_bytePos += m_writePos;
						m_writePos = 0;
					}
					m_dst[m_writePos++] = b;
				}
				void CLOAK_CALL_THIS MultithreadedWriteBuffer::InternWriteBuffer::Save()
				{
					m_bytePos += m_writePos;
					if (m_bytePos > 0)
					{
						CLOAK_ASSUME(m_resList != nullptr);
#ifdef _DEBUG
						uint64_t dbg_maxSize = PAGE_LIST_SIZE;
						Page* dbg_p = m_resList->Page;
						while (dbg_p != nullptr)
						{
							CLOAK_ASSUME(m_bytePos >= dbg_maxSize);
							dbg_maxSize += PAGE_SIZE;
							dbg_p = dbg_p->Next;
						}
						CLOAK_ASSUME(m_bytePos <= dbg_maxSize);
#endif
						m_resList->Next = nullptr;
						m_resList->Size = m_bytePos;
						API::Helper::Lock lock(m_parent->m_sync);
						//The tasks writing into this MTWB might be specificly ordered. To preserve this order, we need to append allways at the END of the ready-to-write list:
						if (m_parent->m_readyToWriteEnd == nullptr)
						{
							m_parent->m_readyToWriteBegin = m_parent->m_readyToWriteEnd = m_resList;
						}
						else
						{
							m_parent->m_readyToWriteEnd->Next = m_resList;
							m_parent->m_readyToWriteEnd = m_resList;
						}
						if (m_parent->m_hasTask.exchange(true) == false)
						{
							CE::RefPointer<MultithreadedWriteBuffer> parent = m_parent;
							API::Global::Task t = API::Global::PushTask([parent](In size_t threadID) { parent->OnWrite(); });
							t.AddDependency(m_parent->m_task);
							m_parent->m_finish.AddDependency(t);
							t.Schedule(m_parent->m_writeTaskHint);
							m_parent->m_task = t;
						}
						lock.unlock();

						m_bytePos = 0;
						m_writePos = 0;
						m_writeSize = 0;
						m_dst = nullptr;
						m_curPage = nullptr;
						m_resList = nullptr;
					}
				}

				void* CLOAK_CALL MultithreadedWriteBuffer::InternWriteBuffer::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL MultithreadedWriteBuffer::InternWriteBuffer::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
			}
		}
	}
	namespace API {
		namespace Files {
			namespace Buffer_v1 {
				namespace UniqueFileIdentifier {
					constexpr uint32_t TYPE_NONE = 0;
					constexpr uint32_t TYPE_PATH = 1;
					constexpr size_t HASH_SIZE = API::Files::Hash::Blake2::DIGEST_SIZE + 1;

					enum PATH_INPUT {
						DOT,
						DDOT,
						BACKSLASH,
						SLASH,
						OTHER,
						QUESTION,

						PATH_INPUT_NUM,
					};
					enum PATH_STATES {
						ROOT,
						ROOT_ESCAPE,
						ROOT_ENV,
						ROOT_ENV_ESCAPE,
						PATH_START,
						PATH,
						PATH_ESCAPE,
						PATH_ENV,
						PATH_ENV_ESCAPE,
						PATH_BACK_1,
						PATH_BACK_2,

						PATH_STATES_NUM,
					};
					constexpr PATH_STATES PATH_MACHINE[PATH_STATES_NUM][PATH_INPUT_NUM] = {
						/*						DOT				DDOT		BACKSLAH		SLASH		OTHER		QUESTION	*/
						/*ROOT				*/ {PATH,			PATH_START,	ROOT_ESCAPE,	PATH_START,	ROOT,		ROOT_ENV,	},
						/*ROOT_ESCAPE		*/ {ROOT,			ROOT,		ROOT,			ROOT,		ROOT,		ROOT,		},
						/*ROOT_ENV			*/ {ROOT_ENV,		ROOT_ENV,	ROOT_ENV_ESCAPE,ROOT_ENV,	ROOT_ENV,	ROOT,		},
						/*ROOT_ENV_ESCAPE	*/ {ROOT_ENV,		ROOT_ENV,	ROOT_ENV,		ROOT_ENV,	ROOT_ENV,	ROOT_ENV,	},
						/*PATH_START		*/ {PATH_BACK_1,	PATH,		PATH_ESCAPE,	PATH_START,	PATH,		PATH_ENV,	},
						/*PATH				*/ {PATH,			PATH,		PATH_ESCAPE,	PATH_START,	PATH,		PATH_ENV,	},
						/*PATH_ESCAPE		*/ {PATH,			PATH,		PATH_START,		PATH,		PATH,		PATH,		},
						/*PATH_ENV			*/ {PATH_ENV,		PATH_ENV,	PATH_ENV_ESCAPE,PATH_ENV,	PATH_ENV,	PATH,		},
						/*PATH_ENV_ESCAPE	*/ {PATH_ENV,		PATH_ENV,	PATH_ENV,		PATH_ENV,	PATH_ENV,	PATH_ENV,	},
						/*PATH_BACK_1		*/ {PATH_BACK_2,	PATH,		PATH_ESCAPE,	PATH_START,	PATH,		PATH_ENV,	},
						/*PATH_BACK_2		*/ {PATH_START,		PATH_START,	PATH_START,		PATH_START,	PATH_START,	PATH_START,	},
					};

					inline void CLOAK_CALL AddPath(Inout API::List<std::string>* stack, Inout std::stringstream& text, In const std::string& path)
					{
						text << path;
						std::string p = text.str();
						size_t lp = 0;
						for (size_t a = 0; a < p.length(); a++)
						{
							if (p[a] == '/' || p[a] == '\\') 
							{ 
								std::string n = p.substr(lp, a - lp);
								stack->push_back(n); 
								lp = a + 1; 
							}
						}
						if (lp != 0) 
						{
							text.str(std::string());
							text << p.substr(lp);
						}
					}
					//TODO: Someday I need to revisit this to support all unicode characters
					inline char CLOAK_CALL toLower(In char c)
					{
						unsigned char uc = static_cast<unsigned char>(c);
						if (uc >= 'A' && uc <= 'Z') { uc = uc + 'a' - 'A'; }
						return static_cast<char>(uc);
					}
					inline void CLOAK_CALL GetFullPath(In std::string filePath, Out CE::List<std::string>* stack)
					{
						CLOAK_ASSUME(stack != nullptr);
						std::stringstream t;
						std::stringstream evn;
						const size_t pfl = filePath.length();
						PATH_STATES state = stack->empty() ? PATH_STATES::ROOT : PATH_STATES::PATH_START;
						for (size_t a = 0; a < pfl; a++)
						{
							PATH_INPUT input = PATH_INPUT::OTHER;
							const char c = filePath[a];
							if (c == '.') { input = PATH_INPUT::DOT; }
							else if (c == ':') { input = PATH_INPUT::DDOT; }
							else if (c == '\\') { input = PATH_INPUT::BACKSLASH; }
							else if (c == '/') { input = PATH_INPUT::SLASH; }
							else if (c == '?') { input = PATH_INPUT::QUESTION; }
							PATH_STATES lstate = state;
							state = PATH_MACHINE[state][input];
							switch (state)
							{
								case ROOT:
								{
									if (lstate == ROOT_ENV)
									{
										AddPath(stack, t, Impl::Files::Buffer_v1::GetEnvironmentPath(evn.str()));
										evn.str(std::string());
										if (stack->size() > 0)
										{
											if (t.tellp() > 0) { state = PATH; }
											else { state = PATH_START; }
										}
									}
									else { t << c; }
									break;
								}
								case ROOT_ESCAPE: {break; }
								case ROOT_ENV:
								{
									if (lstate == ROOT_ENV || lstate == ROOT_ENV_ESCAPE) { evn << c; }
									break;
								}
								case ROOT_ENV_ESCAPE: {break; }
								case PATH_START:
								{
									if (lstate == ROOT && input == DDOT)
									{
										std::string r = t.str();
										t.str(std::string());
										if (r.compare("root") == 0)
										{
											AddPath(stack, t, Impl::Files::Buffer_v1::GetRootPath());
											if (t.tellp() > 0)
											{
												stack->push_back(t.str());
												t.str(std::string());
											}
										}
										else if (r.compare("module") == 0)
										{
											AddPath(stack, t, Impl::Global::Game::getModulePath());
											if (t.tellp() > 0)
											{
												stack->push_back(t.str());
												t.str(std::string());
											}
										}
										else { t << r << c; }
									}
									else if (lstate == PATH_BACK_2)
									{
										if (stack->empty() == false) { stack->pop_back(); }
									}
									else if(t.tellp() > 0)
									{
										stack->push_back(t.str());
										t.str(std::string());
									}
									break;
								}
								case PATH:
								{
									if (lstate == PATH_ENV)
									{
										AddPath(stack, t, Impl::Files::Buffer_v1::GetEnvironmentPath(evn.str()));
										evn.str(std::string());
									}
									else if (lstate == PATH_ESCAPE)
									{
										stack->push_back(t.str());
										t.str(std::string());
										t << c;
									}
									else { t << c; }
									break;
								}
								case PATH_ESCAPE: {break; }
								case PATH_ENV:
								{
									if (lstate == PATH_ENV || lstate == PATH_ENV_ESCAPE) { evn << c; }
									break;
								}
								case PATH_ENV_ESCAPE: {break; }
								case PATH_BACK_1: {break; }
								case PATH_BACK_2: {break; }
								default:
									break;
							}
						}
						if (state == PATH || state == PATH_ESCAPE || state == PATH_START)
						{
							const std::string s = t.str();
							if (s.length() > 0) { stack->push_back(s); }
						}
					}
					inline std::string CLOAK_CALL GetAsFilePath(In const CE::List<std::string>& stack)
					{
						std::stringstream r;
						for (size_t a = 0; a < stack.size(); a++)
						{
							if (a > 0) { r << '/'; }
							r << stack[a];
						}
						return r.str();
					}
					inline std::string CLOAK_CALL GetFullPathAsString(In const std::string& filePath)
					{
						CE::List<std::string> stack;
						GetFullPath(filePath, &stack);
						return GetAsFilePath(stack);
					}
					inline void* CLOAK_CALL Allocate(In CE::List<std::string>* stack, In const std::string& filePath)
					{
						if (filePath.length() > 0) { GetFullPath(filePath, stack); }
						size_t rs = sizeof(uint32_t);
						for (size_t a = 0; a < stack->size(); a++) { rs += (stack->at(a).length() + 1) * sizeof(char); }
						uintptr_t res = reinterpret_cast<uintptr_t>(API::Global::Memory::MemoryPool::Allocate(rs));
						*reinterpret_cast<uint32_t*>(res) = static_cast<uint32_t>(stack->size());
						char* r = reinterpret_cast<char*>(res + sizeof(uint32_t));
						for (size_t a = 0, b=0; a < stack->size(); a++)
						{
							for (size_t c = 0; c < stack->at(a).length(); b++, c++) { r[b] = stack->at(a)[c]; }
							r[b++] = '\0';
						}
						return reinterpret_cast<void*>(res);
					}
					inline void* CLOAK_CALL Copy(In uint32_t type, In const void* src)
					{
						if (type == TYPE_PATH)
						{
							uintptr_t res = reinterpret_cast<uintptr_t>(src);
							const uint32_t s = *reinterpret_cast<uint32_t*>(res);
							const char* or = reinterpret_cast<const char*>(res + sizeof(uint32_t));
							size_t rs = sizeof(uint32_t);
							for (size_t a = 0, b = 0; a < s; b++)
							{
								if (or[b] == '\0') { a++; }
								rs += sizeof(char);
							}
							void* dst = API::Global::Memory::MemoryPool::Allocate(rs);
							memcpy(dst, src, rs);
							return dst;
						}
						return nullptr;
					}
					inline bool CLOAK_CALL Compare(In uint32_t type, In const void* a, In const void* b)
					{
						if (a == nullptr || b == nullptr) { return a == b; }
						if (type == TYPE_PATH)
						{
							uintptr_t ra = reinterpret_cast<uintptr_t>(a);
							uintptr_t rb = reinterpret_cast<uintptr_t>(b);
							const uint32_t as = *reinterpret_cast<uint32_t*>(ra);
							const uint32_t bs = *reinterpret_cast<uint32_t*>(rb);
							if (as != bs) { return false; }
							const char* oa = reinterpret_cast<const char*>(ra + sizeof(uint32_t));
							const char* ob = reinterpret_cast<const char*>(rb + sizeof(uint32_t));
							for (size_t c = 0, d = 0; c < as; d++)
							{
								if (toLower(oa[d]) != toLower(ob[d])) { return false; }
								if (oa[d] == '\0') { c++; }
							}
							return true;
						}
						return false;
					}
					inline void CLOAK_CALL GetFilePath(In const void* ptr, Out CE::List<std::string>* stack)
					{
						CLOAK_ASSUME(stack != nullptr);
						CLOAK_ASSUME(ptr != nullptr);
						uintptr_t res = reinterpret_cast<uintptr_t>(ptr);
						const uint32_t s = *reinterpret_cast<uint32_t*>(res);
						const char* r = reinterpret_cast<const char*>(res + sizeof(uint32_t));
						stack->clear();
						stack->reserve(s);
						std::stringstream ss;
						for (size_t a = 0, b = 0; a < s; b++)
						{
							if (r[b] == '\0') 
							{ 
								a++; 
								stack->push_back(ss.str());
								ss.str(std::string());
							}
							else { ss << r[b]; }
						}
					}
					inline std::string CLOAK_CALL GetFilePath(In const void* ptr, In_opt std::string type = "")
					{
						uintptr_t res = reinterpret_cast<uintptr_t>(ptr);
						const uint32_t s = *reinterpret_cast<uint32_t*>(res);
						const char* r = reinterpret_cast<const char*>(res + sizeof(uint32_t));
						std::stringstream ss;
						std::stringstream lt;
						for (size_t a = 0, b = 0; true; b++)
						{
							if (r[b] == '\0')
							{
								a++;
								if (a < s) { ss << '/'; lt.str(std::string()); }
								else { break; }
							}
							else 
							{ 
								ss << r[b]; 
								if (r[b] == '.') { lt.str(""); }
								else { lt << toLower(r[b]); }
							}
						}
						if (type.length() > 0)
						{
							std::transform(type.begin(), type.end(), type.begin(), toLower);
							if (type.compare(lt.str()) != 0) { ss << '.' << type; }
						}
						return ss.str();
					}

					inline size_t CLOAK_CALL GenerateHash()
					{
						uint8_t res[HASH_SIZE];
						memset(res, 0, sizeof(uint8_t)*HASH_SIZE);
						res[0] = 'N';
						return API::Files::Hash::FNV::Calculate<size_t>(HASH_SIZE, res);
					}
					inline size_t CLOAK_CALL GenerateHash(In CE::List<std::string>* stack, In const std::string& filePath)
					{
						if (filePath.length() > 0) { GetFullPath(filePath, stack); }
						std::string path = GetAsFilePath(*stack);
						std::transform(path.begin(), path.end(), path.begin(), toLower);
						uint8_t res[HASH_SIZE];
						API::Files::Hash::Blake2 hash;
						hash.Update(reinterpret_cast<const uint8_t*>(&path[0]), path.length());
						hash.Finish(&res[1]);
						res[0] = 'F';
						return API::Files::Hash::FNV::Calculate<size_t>(HASH_SIZE, res);
					}
					inline void* CLOAK_CALL Allocate(In const std::string& filePath)
					{
						CE::List<std::string> stack;
						return Allocate(&stack, filePath);
					}
					inline void* CLOAK_CALL Allocate(In uint32_t type, In const void* ptr, In const std::string& filePath)
					{
						CE::List<std::string> stack;
						if (type == TYPE_PATH)
						{
							GetFilePath(ptr, &stack);
							return Allocate(&stack, filePath);
						}
						return nullptr;
					}
					inline void* CLOAK_CALL Allocate(In uint32_t type, In const void* ptr, In uint32_t offset)
					{
						CE::List<std::string> stack;
						if (type == TYPE_PATH)
						{
							GetFilePath(ptr, &stack);
							for (uint32_t a = 0; a < offset && stack.size() > 0; a++) { stack.pop_back(); }
							return Allocate(&stack, "");
						}
						return nullptr;
					}
					inline size_t CLOAK_CALL GenerateHash(In const std::string& filePath)
					{
						CE::List<std::string> stack;
						return GenerateHash(&stack, filePath);
					}
					inline size_t CLOAK_CALL GenerateHash(In uint32_t type, In const void* ptr, In const std::string& filePath)
					{
						CE::List<std::string> stack;
						if (type == TYPE_PATH)
						{
							GetFilePath(ptr, &stack);
							return GenerateHash(&stack, filePath);
						}
						return GenerateHash();
					}
					inline size_t CLOAK_CALL GenerateHash(In uint32_t type, In const void* ptr, In uint32_t offset)
					{
						CE::List<std::string> stack;
						if (type == TYPE_PATH)
						{
							GetFilePath(ptr, &stack);
							for (uint32_t a = 0; a < offset && stack.size() > 0; a++) { stack.pop_back(); }
							return GenerateHash(&stack, "");
						}
						return GenerateHash();
					}
				}
				namespace UniqueGameIdentifier {
					inline void CLOAK_CALL GuidToID(In const GUID& x, Out uint8_t ID[UGID::ID_SIZE])
					{
						uint8_t fd[16];
						fd[0] = static_cast<uint8_t>((x.Data1 >> 0x00) & 0xFF);
						fd[1] = static_cast<uint8_t>((x.Data1 >> 0x08) & 0xFF);
						fd[2] = static_cast<uint8_t>((x.Data1 >> 0x10) & 0xFF);
						fd[3] = static_cast<uint8_t>((x.Data1 >> 0x18) & 0xFF);
						fd[4] = static_cast<uint8_t>((x.Data2 >> 0x00) & 0xFF);
						fd[5] = static_cast<uint8_t>((x.Data2 >> 0x08) & 0xFF);
						fd[6] = static_cast<uint8_t>((x.Data3 >> 0x00) & 0xFF);
						fd[7] = static_cast<uint8_t>((x.Data3 >> 0x08) & 0xFF);
						for (size_t a = 0; a < ARRAYSIZE(x.Data4); a++) { fd[a + 8] = x.Data4[a]; }

						uint8_t rr[API::Files::Hash::SHA256::DIGEST_SIZE];
						API::Files::Hash::SHA256 rh;
						rh.Update(fd, ARRAYSIZE(fd));
						rh.Finish(rr);

						for (size_t a = 0; a < UGID::ID_SIZE; a++) { ID[a] = 0; }
						for (size_t a = 0; a < max(UGID::ID_SIZE, ARRAYSIZE(rr)); a++) { ID[a % UGID::ID_SIZE] ^= rr[a % ARRAYSIZE(rr)]; }
					}
				}

				CLOAK_CALL UGID::UGID()
				{
					for (size_t a = 0; a < ID_SIZE; a++) { ID[a] = 0; }
				}
				CLOAK_CALL UGID::UGID(In const UGID& x)
				{
					for (size_t a = 0; a < ID_SIZE; a++) { ID[a] = x.ID[a]; }
				}
				CLOAK_CALL UGID::UGID(In const GUID& x)
				{
					UniqueGameIdentifier::GuidToID(x, ID);
				}
				UGID& CLOAK_CALL_THIS UGID::operator=(In const UGID& x)
				{
					for (size_t a = 0; a < ID_SIZE; a++) { ID[a] = x.ID[a]; }
					return *this;
				}
				UGID& CLOAK_CALL_THIS UGID::operator=(In const GUID& x)
				{
					UniqueGameIdentifier::GuidToID(x, ID);
					return *this;
				}


				CLOAKENGINE_API CE::RefPointer<IReadBuffer> CLOAK_CALL CreateUTFRead(In const CE::RefPointer<IReadBuffer>& input)
				{
					return CE::RefPointer<IReadBuffer>::Construct<Engine::Compress::UTF::UTFReadBuffer>(input);
				}
				CLOAKENGINE_API CE::RefPointer<IReadBuffer> CLOAK_CALL CreateCompressRead(In const CE::RefPointer<IReadBuffer>& input, In CompressType type)
				{
					switch (type)
					{
						case CloakEngine::API::Files::Buffer_v1::CompressType::NONE: { return input; }
						case CloakEngine::API::Files::Buffer_v1::CompressType::LZC:
						{
							return CE::RefPointer<IReadBuffer>::Construct<Engine::Compress::LZC::LZCReadBuffer>(input, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::HUFFMAN:
						{
							return CE::RefPointer<IReadBuffer>::Construct<Engine::Compress::Huffman::HuffmanReadBuffer>(input, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::LZW:
						{
							return CE::RefPointer<IReadBuffer>::Construct<Engine::Compress::LZW::LZWReadBuffer>(input, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::LZH:
						{
							return CE::RefPointer<IReadBuffer>::Construct<Engine::Compress::LZC::LZCReadBuffer>(input, API::Files::Buffer_v1::CompressType::HUFFMAN);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::ARITHMETIC:
						{
							return CE::RefPointer<IReadBuffer>::Construct<Engine::Compress::AAC::AACReadBuffer>(input, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::LZ4:
						{
							return CE::RefPointer<IReadBuffer>::Construct<Engine::Compress::LZ4::LZ4ReadBuffer>(input, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::ZLIB:
						{
							return CE::RefPointer<IReadBuffer>::Construct<Engine::Compress::ZLIB::ZLIBReadBuffer>(input, API::Files::Buffer_v1::CompressType::NONE);
						}
						default:
							break;
					}
					return nullptr;
				}
				CLOAKENGINE_API CE::RefPointer<IWriteBuffer> CLOAK_CALL CreateCompressWrite(In const CE::RefPointer<IWriteBuffer>& output, In CompressType type)
				{
					switch (type)
					{
						case CloakEngine::API::Files::Buffer_v1::CompressType::NONE:
						{
							return output;
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::LZC:
						{
							return CE::RefPointer<IWriteBuffer>::Construct<Engine::Compress::LZC::LZCWriteBuffer>(output, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::HUFFMAN:
						{
							return CE::RefPointer<IWriteBuffer>::Construct<Engine::Compress::Huffman::HuffmanWriteBuffer>(output, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::LZW:
						{
							return CE::RefPointer<IWriteBuffer>::Construct<Engine::Compress::LZW::LZWWriteBuffer>(output, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::LZH:
						{
							return CE::RefPointer<IWriteBuffer>::Construct<Engine::Compress::LZC::LZCWriteBuffer>(output, API::Files::Buffer_v1::CompressType::HUFFMAN);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::ARITHMETIC:
						{
							return CE::RefPointer<IWriteBuffer>::Construct<Engine::Compress::AAC::AACWriteBuffer>(output, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::LZ4:
						{
							return CE::RefPointer<IWriteBuffer>::Construct<Engine::Compress::LZ4::LZ4WriteBuffer>(output, API::Files::Buffer_v1::CompressType::NONE);
						}
						case CloakEngine::API::Files::Buffer_v1::CompressType::ZLIB:
						{
							return CE::RefPointer<IWriteBuffer>::Construct<Engine::Compress::ZLIB::ZLIBWriteBuffer>(output, API::Files::Buffer_v1::CompressType::NONE);
						}
						default:
							break;
					}
					return nullptr;
				}
				CLOAKENGINE_API CE::RefPointer<IVirtualWriteBuffer> CLOAK_CALL CreateVirtualWriteBuffer()
				{
					return CE::RefPointer<IVirtualWriteBuffer>::Construct<Impl::Files::Buffer_v1::VirtualWriterBuffer>();
				}
				CLOAKENGINE_API CE::RefPointer<IVirtualWriteBuffer> CLOAK_CALL CreateTempWriteBuffer()
				{
					return CE::RefPointer<IVirtualWriteBuffer>::Construct<Impl::Files::FileIO_v1::TempFileWriteBuffer>();
				}
				CLOAKENGINE_API CE::RefPointer<IHTTPReadBuffer> CLOAK_CALL CreateHTTPRead(In const std::string& host, In_opt HTTPMethod method, In_opt uint16_t port)
				{
					return CE::RefPointer<IHTTPReadBuffer>::Construct<Engine::Compress::NetBuffer::HTTPBuffer>(host, port, method);
				}
				CLOAKENGINE_API CE::RefPointer<IHTTPReadBuffer> CLOAK_CALL CreateHTTPRead(In const std::string& host, In uint16_t port, In_opt HTTPMethod method)
				{
					return CE::RefPointer<IHTTPReadBuffer>::Construct<Engine::Compress::NetBuffer::HTTPBuffer>(host, port, method);
				}
				CLOAKENGINE_API CE::RefPointer<IHTTPReadBuffer> CLOAK_CALL CreateHTTPRead()
				{
					return CE::RefPointer<IHTTPReadBuffer>::Construct<Engine::Compress::NetBuffer::HTTPBuffer>();
				}
				CLOAKENGINE_API CE::RefPointer<IMultithreadedWriteBuffer> CLOAK_CALL CreateMultithreadedWriteBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IWriteBuffer>& dst, In_opt std::function<void(In void* userData, In uint64_t size)> onWrite, In_opt API::Global::Threading::ScheduleHint writingHint)
				{
					return CE::RefPointer<IMultithreadedWriteBuffer>::Construct<Impl::Files::Buffer_v1::MultithreadedWriteBuffer>(dst, onWrite, writingHint);
				}
				CLOAKENGINE_API CE::RefPointer<IMultithreadedWriteBuffer> CLOAK_CALL CreateMultithreadedWriteBuffer(In const CE::RefPointer<IWriteBuffer>& dst, In API::Global::Threading::ScheduleHint writingHint)
				{
					return CreateMultithreadedWriteBuffer(dst, nullptr, writingHint);
				}
				CLOAKENGINE_API std::string CLOAK_CALL GetFullFilePath(In const std::string& path)
				{
					return UniqueFileIdentifier::GetFullPathAsString(path);
				}
				CLOAKENGINE_API std::u16string CLOAK_CALL GetFullFilePath(In const std::u16string& path)
				{
					return API::Helper::StringConvert::ConvertToU16(UniqueFileIdentifier::GetFullPathAsString(API::Helper::StringConvert::ConvertToU8(path)));
				}

				CLOAK_CALL UFI::UFI() : m_type(UniqueFileIdentifier::TYPE_NONE), m_data(nullptr), m_hash(UniqueFileIdentifier::GenerateHash()) {}

				CLOAK_CALL UFI::UFI(In const std::string& filePath) : m_type(UniqueFileIdentifier::TYPE_PATH), m_data(UniqueFileIdentifier::Allocate(filePath)), m_hash(UniqueFileIdentifier::GenerateHash(filePath)) {}
				CLOAK_CALL UFI::UFI(In const std::u16string& filePath) : UFI(API::Helper::StringConvert::ConvertToU8(filePath)) {}
				CLOAK_CALL UFI::UFI(In const std::wstring& filePath) : UFI(API::Helper::StringConvert::ConvertToU8(filePath)) {}
				CLOAK_CALL UFI::UFI(In const std::string_view& filePath) : UFI(std::string(filePath)) {}
				CLOAK_CALL UFI::UFI(In const std::u16string_view& filePath) : UFI(std::u16string(filePath)) {}
				CLOAK_CALL UFI::UFI(In const std::wstring_view& filePath) : UFI(std::wstring(filePath)) {}
				CLOAK_CALL UFI::UFI(In const char* filePath) : UFI(std::string(filePath)) {}
				CLOAK_CALL UFI::UFI(In const char16_t* filePath) : UFI(std::u16string(filePath)) {}
				CLOAK_CALL UFI::UFI(In const wchar_t* filePath) : UFI(std::wstring(filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& o) : m_type(o.m_type), m_data(UniqueFileIdentifier::Copy(o.m_type,o.m_data)), m_hash(o.m_hash) {}
				CLOAK_CALL UFI::UFI(In UFI&& o) : m_type(o.m_type), m_data(o.m_data), m_hash(o.m_hash) {}

				CLOAK_CALL UFI::UFI(In const UFI& u, In uint32_t offset) : m_type(UniqueFileIdentifier::TYPE_PATH), m_data(UniqueFileIdentifier::Allocate(u.m_type, u.m_data, offset)), m_hash(UniqueFileIdentifier::GenerateHash(u.m_type, u.m_data, offset)) {}

				CLOAK_CALL UFI::UFI(In const UFI& u, In const std::string& filePath) : m_type(UniqueFileIdentifier::TYPE_PATH), m_data(UniqueFileIdentifier::Allocate(u.m_type, u.m_data, filePath)), m_hash(UniqueFileIdentifier::GenerateHash(u.m_type, u.m_data, filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& u, In const std::u16string& filePath) : UFI(u, API::Helper::StringConvert::ConvertToU8(filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& u, In const std::wstring& filePath) : UFI(u, API::Helper::StringConvert::ConvertToU8(filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& u, In const std::string_view& filePath) : UFI(u, std::string(filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& u, In const std::u16string_view& filePath) : UFI(u, std::u16string(filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& u, In const std::wstring_view& filePath) : UFI(u, std::wstring(filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& u, In const char* filePath) : UFI(u, std::string(filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& u, In const char16_t* filePath) : UFI(u, std::u16string(filePath)) {}
				CLOAK_CALL UFI::UFI(In const UFI& u, In const wchar_t* filePath) : UFI(u, std::wstring(filePath)) {}

				CLOAK_CALL UFI::~UFI() 
				{ 
					API::Global::Memory::MemoryPool::Free(m_data);
				}
				UFI& CLOAK_CALL_THIS UFI::operator=(In const UFI& o)
				{
					API::Global::Memory::MemoryPool::Free(m_data);
					m_type = o.m_type;
					m_data = UniqueFileIdentifier::Copy(o.m_type, o.m_data);
					m_hash = o.m_hash;
					return *this;
				}
				bool CLOAK_CALL_THIS UFI::operator==(In const UFI& o) const 
				{
					if (m_hash != o.m_hash || m_type != o.m_type) { return false; }
					return UniqueFileIdentifier::Compare(m_type, m_data, o.m_data);
				}
				bool CLOAK_CALL_THIS UFI::operator!=(In const UFI& o) const { return !operator==(o); }
				CE::RefPointer<IReadBuffer> CLOAK_CALL_THIS UFI::OpenReader(In_opt bool silent, In_opt std::string fileType) const
				{
					if (m_type == UniqueFileIdentifier::TYPE_PATH) { return CE::RefPointer<IReadBuffer>::Construct<Impl::Files::FileIO_v1::FileReadBuffer>(UniqueFileIdentifier::GetFilePath(m_data, fileType), silent); }
					return nullptr;
				}
				CE::RefPointer<IWriteBuffer> CLOAK_CALL_THIS UFI::OpenWriter(In_opt bool silent, In_opt std::string fileType) const
				{
					if (m_type == UniqueFileIdentifier::TYPE_PATH) { return CE::RefPointer<IWriteBuffer>::Construct<Impl::Files::FileIO_v1::FileWriteBuffer>(UniqueFileIdentifier::GetFilePath(m_data, fileType), silent); }
					return nullptr;
				}
				size_t CLOAK_CALL_THIS UFI::GetHash() const { return m_hash; }
				std::string CLOAK_CALL_THIS UFI::GetRoot() const
				{
					if (m_type == UniqueFileIdentifier::TYPE_PATH)
					{
						CE::List<std::string> stack;
						UniqueFileIdentifier::GetFilePath(m_data, &stack);
						return stack.front();
					}
					return "";
				}
				std::string CLOAK_CALL_THIS UFI::GetPath() const
				{
					if (m_type == UniqueFileIdentifier::TYPE_PATH)
					{
						CE::List<std::string> stack;
						UniqueFileIdentifier::GetFilePath(m_data, &stack);
						if (stack.size() > 0) { stack.pop_back(); }
						return UniqueFileIdentifier::GetAsFilePath(stack);
					}
					return "";
				}
				std::string CLOAK_CALL_THIS UFI::GetName() const
				{
					if (m_type == UniqueFileIdentifier::TYPE_PATH)
					{
						CE::List<std::string> stack;
						UniqueFileIdentifier::GetFilePath(m_data, &stack);
						if (stack.size() > 0) { return stack.back(); }
					}
					return "";
				}
				std::string CLOAK_CALL_THIS UFI::ToString() const
				{
					if (m_type == UniqueFileIdentifier::TYPE_PATH)
					{
						CE::List<std::string> stack;
						UniqueFileIdentifier::GetFilePath(m_data, &stack);
						return UniqueFileIdentifier::GetAsFilePath(stack);
					}
					return "";
				}

				UFI CLOAK_CALL_THIS UFI::Append(In const std::string& filePath) const { return UFI(*this, filePath); }
				UFI CLOAK_CALL_THIS UFI::Append(In const std::u16string& filePath) const { return UFI(*this, filePath); }
				UFI CLOAK_CALL_THIS UFI::Append(In const std::wstring& filePath) const { return UFI(*this, filePath); }
				UFI CLOAK_CALL_THIS UFI::Append(In const std::string_view& filePath) const { return UFI(*this, filePath); }
				UFI CLOAK_CALL_THIS UFI::Append(In const std::u16string_view& filePath) const { return UFI(*this, filePath); }
				UFI CLOAK_CALL_THIS UFI::Append(In const std::wstring_view& filePath) const { return UFI(*this, filePath); }
				UFI CLOAK_CALL_THIS UFI::Append(In const char* filePath) const { return UFI(*this, filePath); }
				UFI CLOAK_CALL_THIS UFI::Append(In const char16_t* filePath) const { return UFI(*this, filePath); }
				UFI CLOAK_CALL_THIS UFI::Append(In const wchar_t* filePath) const { return UFI(*this, filePath); }
			}
		}
	}
}