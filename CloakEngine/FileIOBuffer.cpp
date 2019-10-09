#include "stdafx.h"
#include "Implementation/Files/FileIOBuffer.h"
#include "Implementation/Global/Memory.h"
#include "Implementation/Global/Task.h"
#include "Implementation/Helper/SyncSection.h"

#include <sstream>
#include <filesystem>

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace FileIO_v1 {
				enum class FollowUpType { None, Read, Write, Finish };
				struct Overlapped {
					OVERLAPPED Data;
					Overlapped* Previous;
					uint64_t FenceDelta; // Only valid if IsFollowUp == false
					std::atomic<uint64_t> ProcessedBytes; // Only valid if IsFollowUp == false
					uint64_t* ResultProcessedBytes; // Only valid if IsFollowUp == false
					bool IsFollowUp;
					FollowUpType FollowType;
					struct {
						union {
							struct {
								Overlapped* Next;
								void* Buffer;
								uint32_t Size;
							} Read;
							struct {
								Overlapped* Next;
								const void* Buffer;
								uint32_t Size;
							} Write;
							struct {
								uint32_t Size;
							} Finish;
						};
					} FollowUp;
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
					uint64_t CommandID;
#endif
					CLOAK_CALL ~Overlapped() { if (Data.hEvent != nullptr) { CloseHandle(Data.hEvent); } }
					void* CLOAK_CALL operator new(In size_t s);
					void CLOAK_CALL operator delete(In void* ptr);
				};
				struct File {
					std::atomic<FILE_FENCE> LastFence;
					std::atomic<uint64_t> PendingOperations;
					uint64_t CursorPos;
					uint64_t Size;
					FILE_FENCE NextFence;
					bool IsCached;
					bool CanBeCached;
					std::atomic<bool> IsInLRU;
					Overlapped* LastOverlapped;
					API::Helper::ISyncSection* SyncFence;
					struct {
						DWORD Access;
						DWORD Share;
						DWORD Creation;
						DWORD Flags;
					} Flags;
					union {
						struct {
							HANDLE FileHandle;
#ifdef USE_THREADPOOLIO
							PTP_IO IOThread;
#endif
							struct {
								File* Next;
								File* Last;
							} LRU;
						} Loaded;
						struct {
							TCHAR* FilePath;
						} Cached;
					};

#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
					std::string FileName;
#endif

					void* CLOAK_CALL operator new(In size_t s);
					void CLOAK_CALL operator delete(In void* ptr);
				};
				typedef Impl::Global::Memory::TypedPool<File> FilePool;
				typedef Impl::Global::Memory::TypedPool<Overlapped> OverlappedPool;

				FilePool* g_filePool = nullptr;
				OverlappedPool* g_overlappedPool = nullptr;
				File* g_lruStart = nullptr;
				File* g_lruEnd = nullptr;
				std::string* g_tempPath = nullptr;
				std::default_random_engine g_randomEngine;
				API::Helper::ISyncSection* g_syncPath = nullptr;
				API::Helper::ISyncSection* g_syncFile = nullptr;
				API::Helper::ISyncSection* g_syncOverlapped = nullptr;
				API::Helper::ISyncSection* g_syncLRU = nullptr;
#ifndef USE_THREADPOOLIO
				HANDLE g_portHandle = INVALID_HANDLE_VALUE;
#endif
				namespace AccessCheckMachine {
					enum class InputType {
						DPOINT,
						SLASH,
						OTHER,
					};
					enum State {
						START,
						ROOT,
						PATH_START,
						PATH,
						FAILED,
					};
					constexpr State MACHINE[][3] = {
						/*					DPOINT		SLASH		OTHER	*/
						/*START*/		{	ROOT,		PATH_START,	START,	},
						/*ROOT*/		{	FAILED,		PATH,		ROOT,	},
						/*PATH_START*/	{	FAILED,		PATH,		PATH,	},
						/*PATH*/		{	FAILED,		PATH_START,	PATH,	},
						/*FAILED*/		{	FAILED,		FAILED,		FAILED	},
					};
				}


				void* CLOAK_CALL File::operator new(In size_t s)
				{
					API::Helper::Lock lock(g_syncFile);
					return g_filePool->Allocate();
				}
				void CLOAK_CALL File::operator delete(In void* ptr)
				{
					API::Helper::Lock lock(g_syncFile);
					g_filePool->Free(ptr);
				}
				void* CLOAK_CALL Overlapped::operator new(In size_t s)
				{
					API::Helper::Lock lock(g_syncOverlapped);
					return g_overlappedPool->Allocate();
				}
				void CLOAK_CALL Overlapped::operator delete(In void* ptr)
				{
					API::Helper::Lock lock(g_syncOverlapped);
					g_overlappedPool->Free(ptr);
				}

#if CHECK_OS(WINDOWS,0)
				HANDLE g_security = INVALID_HANDLE_VALUE;
				enum class IOOperation { READ, WRITE, LOCK, LOCK_READ, LOCK_WRITE };
				struct IOParam {
					IOOperation Type;
					File* File;
					uint64_t CursorPosition;
					uint64_t* ResultProcessedBytes;
					union {
						struct {
							void* Buffer;
							uint32_t BufferSize;
						} Read;
						struct {
							const void* Buffer;
							uint32_t BufferSize;
						} Write;
						struct {
							uint64_t Size;
							bool Exclusive;
						} Lock;
						struct {
							void* Buffer;
							uint32_t BufferSize;
							bool Exclusive;
						} LockRead;
						struct {
							const void* Buffer;
							uint32_t BufferSize;
							bool Exclusive;
						} LockWrite;
					};
				};

#ifdef USE_THREADPOOLIO
#define DECLARE_IO_CALLBACK(Name) void WINAPI Name(Inout PTP_CALLBACK_INSTANCE instance, Inout_opt void* context, Inout void* ovlLpv, In ULONG succUL, In ULONG_PTR numberOfBytes, Inout PTP_IO io)
#else
#define DECLARE_IO_CALLBACK(Name) void CLOAK_CALL Name(Inout_opt File* file, Inout OVERLAPPED* overlapped, In BOOL success, In DWORD numberOfBytes)
#endif
				DECLARE_IO_CALLBACK(FileIOCallback);

				bool CLOAK_CALL GetFilePath(In HANDLE fileHandle, Out std::tstring* path)
				{
					*path = TEXT("");
					uint8_t infoBuffer[sizeof(FILE_NAME_INFO) + (sizeof(TCHAR) * _MAX_PATH)];
					ZeroMemory(&infoBuffer[0], ARRAYSIZE(infoBuffer));
					FILE_NAME_INFO* info = reinterpret_cast<FILE_NAME_INFO*>(&infoBuffer[0]);
					BOOL res = GetFileInformationByHandleEx(fileHandle, FileNameInfo, info, ARRAYSIZE(infoBuffer));
					if (res == FALSE) { return false; }
					*path = std::tstring(info->FileName, info->FileNameLength);
					return true;
				}
				void CLOAK_CALL WaitForFile(In FILE_ID file, In FILE_FENCE fence)
				{
					if (file == nullptr) { return; }
					File* f = reinterpret_cast<File*>(file);
					if (f->LastFence.load() < fence)
					{
						Impl::Global::Task::IHelpWorker* worker = Impl::Global::Task::GetCurrentWorker();
#ifdef USE_THREADPOOLIO
						worker->HelpWorkWhile([f, fence]() { return f->LastFence.load() < fence; }, ~API::Global::Threading::Flag::IO);
#else
						worker->HelpWorkWhile([f, fence]() {
							DWORD byteCount = 0;
							ULONG_PTR context = 0;
							OVERLAPPED* ovl = nullptr;
							while (true)
							{
								if (GetQueuedCompletionStatus(g_portHandle, &byteCount, &context, &ovl, 0) == TRUE)
								{
									//IO Operation finished successfuly
									FileIOCallback(reinterpret_cast<File*>(context), ovl, TRUE, byteCount);
								}
								else if (ovl != nullptr)
								{
									//IO Operation finished but failed
									FileIOCallback(reinterpret_cast<File*>(context), ovl, FALSE, byteCount);
								}
								else 
								{
									//No IO Operation has finished yet
									break; 
								}
							}
							return f->LastFence.load() < fence;
						}, ~API::Global::Threading::Flag::IO);
#endif
					}
				}
				void CLOAK_CALL WaitForFile(In FILE_ID file)
				{
					if (file == nullptr) { return; }
					File* f = reinterpret_cast<File*>(file);
					WaitForFile(file, f->NextFence);
				}
				void CLOAK_CALL RequestCache()
				{
					API::Helper::Lock lock(g_syncLRU);
					while (g_lruStart == nullptr)
					{
						Impl::Global::Task::IHelpWorker* worker = Impl::Global::Task::GetCurrentWorker();
						Impl::Helper::Lock::LOCK_DATA lockData;
						Impl::Helper::Lock::UnlockAll(&lockData);
						do {
							worker->HelpWork();
							API::Helper::Lock ilock(g_syncLRU);
							if (g_lruStart != nullptr) { break; }
						} while (true);
						Impl::Helper::Lock::RecoverAll(lockData);
					}
					File* t = g_lruStart;
					g_lruStart->Loaded.LRU.Last = nullptr;
					g_lruStart = g_lruStart->Loaded.LRU.Next;

					std::tstring path;
					if (GetFilePath(t->Loaded.FileHandle, &path) == false) { return; }
					WaitForFile(t, t->NextFence);
#ifdef USE_THREADPOOLIO
					CloseThreadpoolIo(t->Loaded.IOThread);
#endif
					CloseHandle(t->Loaded.FileHandle);

					t->IsCached = true;
					TCHAR* p = reinterpret_cast<TCHAR*>(API::Global::Memory::MemoryPool::Allocate(sizeof(TCHAR) * (path.length() + 1)));
					t->Cached.FilePath = p;
					for (size_t a = 0; a < path.length(); a++) { p[a] = path[a]; }
					p[path.length()] = '\0';
				}
				void CLOAK_CALL PushIntoLRU(In File* file)
				{
					API::Helper::Lock lock(g_syncLRU);
					API::Helper::ReadLock flock(file->SyncFence);
					if (file->PendingOperations == 0)
					{
						CLOAK_ASSUME(file->IsCached == false);
						if (file->CanBeCached == false) { return; }
						if (file->IsInLRU == true) { return; }
						file->IsInLRU = true;
						if (g_lruEnd == nullptr)
						{
							CLOAK_ASSUME(g_lruStart == nullptr);
							g_lruStart = g_lruEnd = file;
						}
						else
						{
							g_lruEnd->Loaded.LRU.Next = file;
							file->Loaded.LRU.Last = g_lruEnd;
							file->Loaded.LRU.Next = nullptr;
							g_lruEnd = file;
						}
					}
				}
				void CLOAK_CALL RemoveFromLRU(In File* file)
				{
					API::Helper::Lock lock(g_syncLRU);
					if (file->IsInLRU == false) { return; }
					file->IsInLRU = false;
					if (file->IsCached == true)
					{
						std::tstring path(file->Cached.FilePath);
						API::Global::Memory::MemoryPool::Free(file->Cached.FilePath);
						file->IsCached = false;
						file->Loaded.FileHandle = CreateFile(path.c_str(), file->Flags.Access, file->Flags.Share, nullptr, file->Flags.Creation, file->Flags.Flags, nullptr);
						if (file->Loaded.FileHandle == INVALID_HANDLE_VALUE)
						{
							if (GetLastError() == ERROR_TOO_MANY_OPEN_FILES)
							{
								do {
									RequestCache();
									file->Loaded.FileHandle = CreateFile(path.c_str(), file->Flags.Access, file->Flags.Share, nullptr, file->Flags.Creation, file->Flags.Flags, nullptr);
								} while (file->Loaded.FileHandle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_TOO_MANY_OPEN_FILES);
							}
							if (file->Loaded.FileHandle == INVALID_HANDLE_VALUE) { CloakError(API::Global::Debug::Error::FILE_IO_ERROR, true, "Failed to re-open cached file: " + std::to_string(GetLastError())); return; }
						}
#ifdef USE_THREADPOOLIO
						file->Loaded.IOThread = CreateThreadpoolIo(file->Loaded.FileHandle, FileIOCallback, file, nullptr);
#else
						CreateIoCompletionPort(file->Loaded.FileHandle, g_portHandle, reinterpret_cast<ULONG_PTR>(file), 0);
#endif
					}
					else
					{
						if (file->Loaded.LRU.Last != nullptr)
						{
							file->Loaded.LRU.Last->Loaded.LRU.Next = file->Loaded.LRU.Next;
							if (file->Loaded.LRU.Next != nullptr) { file->Loaded.LRU.Next->Loaded.LRU.Last = file->Loaded.LRU.Last; }
							else
							{
								CLOAK_ASSUME(file == g_lruEnd);
								g_lruEnd = file->Loaded.LRU.Last;
							}
						}
						else if (g_lruStart == file)
						{
							g_lruStart = file->Loaded.LRU.Next;
							if (g_lruEnd == file) { g_lruEnd = g_lruStart; }
						}
					}
				}

				DECLARE_IO_CALLBACK(FileIOCallback)
				{
#ifdef USE_THREADPOOLIO
					File* file = reinterpret_cast<File*>(context);
					OVERLAPPED* overlapped = reinterpret_cast<OVERLAPPED*>(ovlLpv);
					BOOL success = succUL == NO_ERROR ? TRUE : FALSE;
#endif
					CLOAK_ASSUME(file != nullptr);
					CLOAK_ASSUME(overlapped != nullptr);
					Overlapped* o = get_from_member(&Overlapped::Data, overlapped);
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
					std::string dgt = "IO Callback (" + f->FileName + ") Command #" + std::to_string(o->CommandID) + " Processed Bytes = " + std::to_string(numberOfBytes) + "\n";
					OutputDebugStringA(dgt.c_str());
#endif
					CLOAK_ASSUME((o->IsFollowUp == true) == (o->FenceDelta == 0));
					CLOAK_ASSUME(o->IsFollowUp == false || o->Previous != nullptr);
					Overlapped* fo = o;
					while (fo->IsFollowUp == true) { fo = fo->Previous; }
					fo->ProcessedBytes += numberOfBytes;

					if(o->FollowType != FollowUpType::None)
					{
						if (success == TRUE && o->FollowType != FollowUpType::Finish)
						{
#ifdef USE_THREADPOOLIO
							StartThreadpoolIo(f->Loaded.IOThread);
#endif
							switch (o->FollowType)
							{
								case FollowUpType::Read:
									CLOAK_ASSUME(o->FollowUp.Read.Next != nullptr);
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
									dgt = "IO Callback (" + f->FileName + ") Queue Command #" + std::to_string(o->CommandID) + ": READ at " + std::to_string((static_cast<uint64_t>(o->FollowUp.Read.Next->Data.OffsetHigh) << 32) | o->FollowUp.Read.Next->Data.Offset) + "\n";
									OutputDebugStringA(dgt.c_str());
#endif
									success = ::ReadFile(file->Loaded.FileHandle, o->FollowUp.Read.Buffer, o->FollowUp.Read.Size, nullptr, &o->FollowUp.Read.Next->Data);
									break;
								case FollowUpType::Write:
									CLOAK_ASSUME(o->FollowUp.Write.Next != nullptr);
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
									dgt = "IO Callback (" + f->FileName + ") Queue Command #" + std::to_string(o->CommandID) + ": WRITE at " + std::to_string((static_cast<uint64_t>(o->FollowUp.Write.Next->Data.OffsetHigh) << 32) | o->FollowUp.Write.Next->Data.Offset) + "\n";
									OutputDebugStringA(dgt.c_str());
#endif
									success = ::WriteFile(file->Loaded.FileHandle, o->FollowUp.Write.Buffer, o->FollowUp.Write.Size, nullptr, &o->FollowUp.Write.Next->Data);
									break;
								default: CLOAK_ASSUME(false); break;
							}
							const DWORD err = success == TRUE ? ERROR_SUCCESS : GetLastError();
							if (err == ERROR_SUCCESS || err == ERROR_IO_PENDING) { return; }
#ifdef USE_THREADPOOLIO
							CancelThreadpoolIo(f->Loaded.IOThread);
#endif
						}
						//Move to last Overlapped in the order (if we aren't there already)
						while (o->FollowType != FollowUpType::Finish)
						{
							switch (o->FollowType)
							{
								case FollowUpType::Read:
									o = o->FollowUp.Read.Next;
									break;
								case FollowUpType::Write:
									o = o->FollowUp.Write.Next;
									break;
								default: CLOAK_ASSUME(false); break;
							}
						}
						//Unlock file (this must be done wether or not every operation in the order actually succeeded)
						success = ::UnlockFileEx(file->Loaded.FileHandle, 0, static_cast<DWORD>(o->FollowUp.Finish.Size & 0xFFFFFFFF), 0, &o->Data) == TRUE ? success : FALSE;
						if (success == FALSE) 
						{
							const DWORD err = GetLastError();
							CloakError(API::Global::Debug::Error::FILE_IO_ERROR, false, "IO Operation failed with error code " + std::to_string(err));
						}
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
						dgt = "IO Callback (" + f->FileName + ") Command #" + std::to_string(o->CommandID) + " finished (UNLOCK)\n";
						OutputDebugStringA(dgt.c_str());
#endif
					}
					CLOAK_ASSUME(o->FollowType == FollowUpType::None || o->FollowType == FollowUpType::Finish);
					//Move to first Overlapped in the order, deleting all follow-ups on the way
					while (o->IsFollowUp == true)
					{
						fo = o;
						o = fo->Previous;
						delete fo;
					}

					CLOAK_ASSUME(o->FenceDelta != 0 && o->IsFollowUp == false);
					API::Stack<Overlapped*> stack;
					API::Helper::Lock lock(file->SyncFence);
					//Push this order and all previously sequenced orders, that still exist, in the stack:
					fo = o;
					do {
						stack.push(fo);
						fo = fo->Previous;
					} while (fo != nullptr);
					while (true)
					{
						Overlapped* no = stack.top();
						if (no->FenceDelta > 0) //This will be true for at least o itself
						{
							//Order 'no' is not yet finished
							if (no != o) { no->FenceDelta += o->FenceDelta; }
							else { file->LastFence += o->FenceDelta; }
							no->Previous = nullptr;
							break;
						}
						else
						{
							delete no;
							stack.pop();
						}
					}
					o->FenceDelta = 0; //Fence Delta was added to either the LastFence value, or to another FenceDelta of a previously sequenced order
					if (o->ResultProcessedBytes != nullptr) { *o->ResultProcessedBytes = o->ProcessedBytes; }
					lock.unlock();
					const uint64_t p = file->PendingOperations.fetch_sub(1);
					CLOAK_ASSUME(p > 0);
					if (p == 1) { PushIntoLRU(file); }
				}
				//Returns wether the operation was successfully queued
				inline bool CLOAK_CALL ExecuteIOOperation(In const IOParam& param)
				{
					Overlapped* no = new Overlapped();
					no->Data.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
					no->Data.Internal = 0;
					no->Data.InternalHigh = 0;
					no->Data.Offset = static_cast<DWORD>(param.CursorPosition & 0xFFFFFFFF);
					no->Data.OffsetHigh = static_cast<DWORD>(param.CursorPosition >> 32);
					no->Previous = param.File->LastOverlapped;
					no->FenceDelta = 1;
					no->ProcessedBytes = 0;
					no->ResultProcessedBytes = param.ResultProcessedBytes;
					no->FollowType = FollowUpType::None;
					no->IsFollowUp = false;
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
					no->CommandID = param.File->NextFence;
#endif
					param.File->LastOverlapped = no;

#ifdef USE_THREADPOOLIO
					StartThreadpoolIo(param.File->Loaded.IOThread);
#endif
					BOOL result = FALSE;
					switch (param.Type)
					{
						case IOOperation::READ:
						{
							result = ::ReadFile(param.File->Loaded.FileHandle, param.Read.Buffer, static_cast<DWORD>(param.Read.BufferSize), nullptr, &no->Data);
							break;
						}
						case IOOperation::WRITE:
						{
							result = ::WriteFile(param.File->Loaded.FileHandle, param.Write.Buffer, static_cast<DWORD>(param.Write.BufferSize), nullptr, &no->Data);
							break;
						}
						case IOOperation::LOCK:
						{
							result = ::LockFileEx(param.File->Loaded.FileHandle, param.Lock.Exclusive == true ? LOCKFILE_EXCLUSIVE_LOCK : 0, 0, static_cast<DWORD>(param.Lock.Size & 0xFFFFFFFF), static_cast<DWORD>(param.Lock.Size >> 32), &no->Data);
							break;
						}
						case IOOperation::LOCK_READ:
						{
							Overlapped* fo = new Overlapped();
							fo->Data.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
							fo->Data.Internal = 0;
							fo->Data.InternalHigh = 0;
							fo->Data.Offset = static_cast<DWORD>(param.CursorPosition & 0xFFFFFFFF);
							fo->Data.OffsetHigh = static_cast<DWORD>(param.CursorPosition >> 32);
							fo->Previous = no;
							fo->FenceDelta = 0;
							fo->ProcessedBytes = 0;
							fo->ResultProcessedBytes = nullptr;
							fo->IsFollowUp = true;
							fo->FollowType = FollowUpType::Finish;
							fo->FollowUp.Finish.Size = param.LockRead.BufferSize;
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
							fo->CommandID = param.File->NextFence;
#endif

							no->FollowType = FollowUpType::Read;
							no->FollowUp.Read.Next = fo;
							no->FollowUp.Read.Buffer = param.LockRead.Buffer;
							no->FollowUp.Read.Size = param.LockRead.BufferSize;

							result = ::LockFileEx(param.File->Loaded.FileHandle, param.LockRead.Exclusive == true ? LOCKFILE_EXCLUSIVE_LOCK : 0, 0, static_cast<DWORD>(param.LockRead.BufferSize & 0xFFFFFFFF), 0, &no->Data);
							break;
						}
						case IOOperation::LOCK_WRITE:
						{
							Overlapped* fo = new Overlapped();
							fo->Data.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
							fo->Data.Internal = 0;
							fo->Data.InternalHigh = 0;
							fo->Data.Offset = static_cast<DWORD>(param.CursorPosition & 0xFFFFFFFF);
							fo->Data.OffsetHigh = static_cast<DWORD>(param.CursorPosition >> 32);
							fo->Previous = no;
							fo->FenceDelta = 0;
							fo->ProcessedBytes = 0;
							fo->ResultProcessedBytes = nullptr;
							fo->IsFollowUp = true;
							fo->FollowType = FollowUpType::Finish;
							fo->FollowUp.Finish.Size = param.LockWrite.BufferSize;
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
							fo->CommandID = param.File->NextFence;
#endif

							no->FollowType = FollowUpType::Write;
							no->FollowUp.Write.Next = fo;
							no->FollowUp.Write.Buffer = param.LockWrite.Buffer;
							no->FollowUp.Write.Size = param.LockWrite.BufferSize;

							result = ::LockFileEx(param.File->Loaded.FileHandle, param.LockWrite.Exclusive == true ? LOCKFILE_EXCLUSIVE_LOCK : 0, 0, static_cast<DWORD>(param.LockWrite.BufferSize & 0xFFFFFFFF), 0, &no->Data);
							break;
						}
						default: CLOAK_ASSUME(false); break;
					}
					if (result == FALSE && GetLastError() != ERROR_IO_PENDING)
					{
#ifdef USE_THREADPOOLIO
						CancelThreadpoolIo(param.File->Loaded.IOThread);
#endif
						param.File->LastOverlapped = no->Previous;

						Overlapped* follow = nullptr;
						switch (no->FollowType)
						{
							case FollowUpType::Read:
								follow = no->FollowUp.Read.Next;
								break;
							case FollowUpType::Write:
								follow = no->FollowUp.Write.Next;
								break;
							default: CLOAK_ASSUME(false); break;
						}
						do {
							Overlapped* n = follow;
							switch (n->FollowType)
							{
								case FollowUpType::Read:follow = n->FollowUp.Read.Next; break;
								case FollowUpType::Write: follow = n->FollowUp.Write.Next; break;
								default: follow = nullptr; break;
							}
							delete n;
						} while (follow != nullptr);

						delete no;

#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
						std::string dgt = "IO Queue Command #" + std::to_string(param.File->NextFence) + " FAILED\n";
						OutputDebugStringA(dgt.c_str());
#endif

						return false;
					}
#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
					else
					{
						std::string dgt = "IO Queue Command #" + std::to_string(param.File->NextFence) + " SUCCEEDED (Immediate = " + (result == TRUE ? "TRUE" : "FALSE") + ")\n";
						OutputDebugStringA(dgt.c_str());
					}
#endif
					param.File->NextFence++;
					return true;
				}

				void CLOAK_CALL InitSecurity()
				{
					HANDLE ptk = INVALID_HANDLE_VALUE;
					if (OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ, &ptk) == TRUE)
					{
						HANDLE stk = INVALID_HANDLE_VALUE;
						if (DuplicateToken(ptk, SecurityImpersonation, &stk) == TRUE)
						{
							g_security = stk;
						}
						CloseHandle(ptk);
					}
				}
				void CLOAK_CALL ReleaseSecurity()
				{
					CloseHandle(g_security);
				}
				bool CLOAK_CALL FileExists(In LPCTSTR name)
				{
					struct _stat64 buffer;
#ifdef UNICODE
					return _wstat64(name, &buffer) == 0;
#else
					return _stat64(name, &buffer) == 0;
#endif
				}
				bool CLOAK_CALL CheckAccess(In const std::tstring& name, In DWORD rights)
				{
					if (name.length() == 0) { return true; }
					bool res = false;
					DWORD length = 0;
					if (GetFileSecurity(name.c_str(), OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, nullptr, 0, &length) == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					{
						SECURITY_DESCRIPTOR* security = reinterpret_cast<SECURITY_DESCRIPTOR*>(API::Global::Memory::MemoryLocal::Allocate(length));
						if (GetFileSecurity(name.c_str(), OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, security, length, &length) == TRUE)
						{
							GENERIC_MAPPING map;
							map.GenericRead = FILE_GENERIC_READ;
							map.GenericWrite = FILE_GENERIC_WRITE;
							map.GenericExecute = FILE_GENERIC_EXECUTE;
							map.GenericAll = FILE_ALL_ACCESS;

							MapGenericMask(&rights, &map);

							PRIVILEGE_SET ps;
							DWORD pl = sizeof(ps);
							DWORD access = 0;
							BOOL result = FALSE;
							if (AccessCheck(security, g_security, rights, &map, &ps, &pl, &access, &result) == TRUE)
							{
								res = result == TRUE;
							}
						}
						API::Global::Memory::MemoryLocal::Free(security);
					}
#ifdef _DEBUG
					if (res == false) { CloakDebugLog("Access Check failed at path '" + CE::Helper::StringConvert::ConvertToU8(std::tstring(name)) + "' (Requested Rights: " + std::to_string(rights) + ")"); }
#endif
					return res;
				}
				bool CLOAK_CALL CanAccess(In const std::tstring& path, In DWORD rights)
				{
#ifdef UNICODE
					std::wstringstream n;
#else
					std::stringstream n;
#endif
					std::tstring lpath = TEXT("");
					AccessCheckMachine::State state = AccessCheckMachine::START;
					for (size_t a = 0; a <= path.length(); a++)
					{
						bool fin = (a == path.length());
						bool check = fin;
						if (fin == false)
						{
							const TCHAR c = path[a];
							AccessCheckMachine::InputType it = AccessCheckMachine::InputType::OTHER;
							if (c == TEXT('/') || c == TEXT('\\')) { it = AccessCheckMachine::InputType::SLASH; }
							else if (c == TEXT(':')) { it = AccessCheckMachine::InputType::DPOINT; }

							state = AccessCheckMachine::MACHINE[state][static_cast<uint32_t>(it)];
							check = (state == AccessCheckMachine::PATH_START);
							if (check == true && a + 1 == path.length()) { fin = true; }
						}
						if (check == true)
						{
							const std::tstring p = path.substr(0, a);
							if (FileExists(p.c_str()) == false)
							{
								if (CheckAccess(lpath, FILE_ADD_SUBDIRECTORY | FILE_TRAVERSE) == false) { return false; }
								else { CreateDirectory(p.c_str(), nullptr); }
							}
							else
							{
								if (CheckAccess(lpath, FILE_TRAVERSE) == false) { return false; }
							}
							if (fin == true) { return CheckAccess(p.c_str(), rights); }
							lpath = p;
						}
					}
					return false;
				}
				FILE_ID CLOAK_CALL OpenFile(In const std::tstring& name, In bool read, In bool silent, In bool temp, In_opt bool create = true)
				{
					if (read == false)
					{
						const size_t f1 = name.find_last_of('/');
						const size_t f2 = name.find_last_of('\\');
						const size_t f = f1 == name.npos ? f2 : (f2 == name.npos ? f1 : (f1 < f2 ? f2 : f1));
						if (f != name.npos)
						{
							const std::tstring path = name.substr(0, f + 1);
							const bool hac = CanAccess(path, FILE_ADD_FILE | FILE_TRAVERSE);
							if (hac == false)
							{
								API::Global::Log::WriteToLog("Can't access file '" + API::Helper::StringConvert::ConvertToU8(name), API::Global::Log::Type::Warning);
								return nullptr;
							}
						}
					}
					File* f = new File();
					const DWORD access = (read == true ? (GENERIC_READ | FILE_READ_ATTRIBUTES) : (GENERIC_WRITE | (create == true ? 0 : FILE_READ_ATTRIBUTES)));
					const DWORD share = (read == true ? FILE_SHARE_READ : 0) | (temp == true ? (FILE_SHARE_READ | FILE_SHARE_DELETE) : 0);
					const DWORD creation = (read == true || create == false) ? OPEN_EXISTING : CREATE_ALWAYS;
					const DWORD flags = (temp == true ? (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE) : (FILE_ATTRIBUTE_NORMAL | (read == true ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_FLAG_WRITE_THROUGH))) | FILE_FLAG_OVERLAPPED;

					f->Loaded.FileHandle = CreateFile(name.c_str(), access, share, nullptr, creation, flags, nullptr);
					if (f->Loaded.FileHandle == INVALID_HANDLE_VALUE)
					{
						if (GetLastError() == ERROR_TOO_MANY_OPEN_FILES)
						{
							do {
								RequestCache();
								f->Loaded.FileHandle = CreateFile(name.c_str(), access, share, nullptr, creation, flags, nullptr);
							} while (f->Loaded.FileHandle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_TOO_MANY_OPEN_FILES);
						}
						if (f->Loaded.FileHandle == INVALID_HANDLE_VALUE)
						{
							if (silent == false)
							{
								API::Global::Log::WriteToLog("Can't open file '" + API::Helper::StringConvert::ConvertToU8(name) + "' for " + (read ? "reading" : "writing"), API::Global::Log::Type::Warning);
							}
							delete f;
							return nullptr;
						}
					}
#ifdef _DEBUG
#ifdef PRINT_DEBUG_INFO
					f->FileName = API::Files::StringConvert::ConvertToU8(name);
#endif
					f->IsInLRU = false;
#endif
#ifdef USE_THREADPOOLIO
					f->Loaded.IOThread = CreateThreadpoolIo(f->Loaded.FileHandle, FileIOCallback, f, nullptr);
#else
					CreateIoCompletionPort(f->Loaded.FileHandle, g_portHandle, reinterpret_cast<ULONG_PTR>(f), 0);
#endif
					f->CursorPos = 0;
					f->Size = 0;
					f->LastFence = 0;
					f->NextFence = 0;
					f->IsCached = false;
					f->CanBeCached = (temp == false || read == true);
					f->PendingOperations = 0;
					f->LastOverlapped = nullptr;
					f->Loaded.LRU.Next = nullptr;
					f->Loaded.LRU.Last = nullptr;
					f->Flags.Access = access;
					f->Flags.Creation = creation;
					f->Flags.Flags = flags;
					f->Flags.Share = share;
					f->SyncFence = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&f->SyncFence));
					if (read == true || create == false)
					{
						LARGE_INTEGER size;
						if (GetFileSizeEx(f->Loaded.FileHandle, &size) == TRUE)
						{
							f->Size = static_cast<uint64_t>(size.QuadPart > 0 ? size.QuadPart : 0);
							if (create == false) { f->CursorPos = f->Size; }
						}
					}
					PushIntoLRU(f);
					return reinterpret_cast<FILE_ID>(f);
				}
				FILE_ID CLOAK_CALL OpenFile(In FILE_ID fileID, In bool read, In bool silent, In bool temp)
				{
					if (fileID == nullptr) { return nullptr; }
					File* of = reinterpret_cast<File*>(fileID);
					API::Helper::Lock lock(g_syncLRU);
					if (of->IsCached == true) { return OpenFile(of->Cached.FilePath, read, silent, temp, false); }

					std::tstring path;
					if (GetFilePath(of->Loaded.FileHandle, &path) == false)
					{
						if (silent == false)
						{
							API::Global::Log::WriteToLog("Can't query file path", API::Global::Log::Type::Warning);
						}
						return nullptr;
					}
					return OpenFile(path, read, silent, temp, false);
				}
				void CLOAK_CALL CloseFile(In FILE_ID file)
				{
					if (file == nullptr) { return; }
					File* f = reinterpret_cast<File*>(file);
					WaitForFile(file, f->NextFence);
					if (f->LastOverlapped != nullptr) { delete f->LastOverlapped; }
					SAVE_RELEASE(f->SyncFence);
					API::Helper::Lock lock(g_syncLRU);
					if (f->IsCached == false)
					{
						RemoveFromLRU(f);
#ifdef USE_THREADPOOLIO
						CloseThreadpoolIo(f->Loaded.IOThread);
#endif
						CloseHandle(f->Loaded.FileHandle);
					}
					else
					{
						API::Global::Memory::MemoryPool::Free(f->Cached.FilePath);
					}
					delete f;
				}
				unsigned long long CLOAK_CALL SeekFile(In FILE_ID file, In API::Files::Buffer_v1::SeekOrigin origin, In long long offset)
				{
					if (file == nullptr) { return 0; }
					File* f = reinterpret_cast<File*>(file);
					switch (origin)
					{
						case CloakEngine::API::Files::Buffer_v1::SeekOrigin::START:
						{
							f->CursorPos = offset;
							return offset;
						}
						case CloakEngine::API::Files::Buffer_v1::SeekOrigin::CURRENT_POS:
						{
							if (offset < 0 && static_cast<uint64_t>(-offset)>f->CursorPos) { f->CursorPos = 0; }
							else { f->CursorPos += offset; }
							return f->CursorPos;
						}
						case CloakEngine::API::Files::Buffer_v1::SeekOrigin::END:
						{
							if (offset < 0 && static_cast<uint64_t>(-offset)>f->Size) { f->CursorPos = 0; }
							else { f->CursorPos = f->Size + offset; }
							return f->CursorPos;
						}
						default:
							CLOAK_ASSUME(false);
							return 0;
					}
				}
				FILE_FENCE CLOAK_CALL WriteFile(In FILE_ID file, In unsigned long long bufferSize, In_reads(bufferSize) uint8_t* buffer)
				{
					if (file != nullptr && buffer != nullptr && bufferSize > 0)
					{
						const uint32_t bfs = static_cast<uint32_t>(bufferSize * sizeof(uint8_t));
						File* f = reinterpret_cast<File*>(file);
						const uint64_t pos = f->CursorPos;
						f->CursorPos += bfs;
						f->Size = max(f->Size, pos + bfs);
						if (f->PendingOperations.fetch_add(1) == 0) { RemoveFromLRU(f); }

						IOParam iop;
						iop.CursorPosition = pos;
						iop.File = f;
						iop.ResultProcessedBytes = nullptr;
						iop.Type = IOOperation::LOCK_WRITE;
						iop.LockWrite.Buffer = buffer;
						iop.LockWrite.BufferSize = bfs;
						iop.LockWrite.Exclusive = true;

#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
						std::string dgt = "Queue IO Operation: LOCK_WRITE\n";
						OutputDebugStringA(dgt.c_str());
#endif

						if (ExecuteIOOperation(iop) == false)
						{
							if (f->PendingOperations.fetch_sub(1) == 1) { PushIntoLRU(f); }
							return 0;
						}

						return f->NextFence;
					}
					return 0;
				}
				FILE_FENCE CLOAK_CALL ReadFile(In FILE_ID file, In unsigned long long count, Out_writes_to(count, return) uint8_t* buffer, Out uint64_t* resultCount)
				{
					if (file != nullptr && buffer != nullptr && count > 0)
					{
						uint64_t bfs = static_cast<uint64_t>(count * sizeof(uint8_t));
						File* f = reinterpret_cast<File*>(file);
						if (f->PendingOperations.fetch_add(1) == 0) { RemoveFromLRU(f); }
						const uint64_t pos = f->CursorPos;
						f->CursorPos += bfs;
						if (f->CursorPos >= f->Size) 
						{ 
							f->CursorPos = f->Size; 
							bfs = f->Size - pos;
							if (bfs == 0) 
							{
								if (resultCount != nullptr) { *resultCount = 0; }
								if (f->PendingOperations.fetch_sub(1) == 1) { PushIntoLRU(f); }
								return 0;
							}
						}
						CLOAK_ASSUME(bfs > 0 && pos + bfs <= f->Size);

						IOParam iop;
						iop.CursorPosition = pos;
						iop.File = f;
						iop.ResultProcessedBytes = resultCount;
						iop.Type = IOOperation::LOCK_READ;
						iop.LockRead.Buffer = buffer;
						iop.LockRead.BufferSize = static_cast<uint32_t>(bfs);
						iop.LockRead.Exclusive = false;

#if (defined(_DEBUG) && defined(PRINT_DEBUG_INFO))
						std::string dgt = "Queue IO Operation (" + f->FileName + "): LOCK_READ\n";
						OutputDebugStringA(dgt.c_str());
#endif
						if (ExecuteIOOperation(iop) == false)
						{
							if (f->PendingOperations.fetch_sub(1) == 1) { PushIntoLRU(f); }
							return 0;
						}

						return f->NextFence;
					}
					return 0;
				}
#endif //CHECK_OS(WINDOWS,0)

				inline bool CLOAK_CALL FileExists(In const std::string& s)
				{
					std::tstring t = API::Helper::StringConvert::ConvertToT(s);
					return FileExists(t.c_str());
				}
				inline std::string CLOAK_CALL GetTempFilePath()
				{
					API::Helper::Lock lock(g_syncPath);
					if (g_tempPath == nullptr)
					{
						std::wstring path = std::filesystem::temp_directory_path().wstring();
						if (path[path.length() - 1] != L'/' && path[path.length() - 1] != L'\\') { path.append(L"/"); }
						path.append(L"CloakEngineTemp/");
						if (CanAccess(path, FILE_ADD_FILE | FILE_LIST_DIRECTORY) == true) { goto found_path; }
						path = API::Helper::StringConvert::ConvertToW(API::Files::GetFullFilePath("?APPDATA?/CloakEngineTemp/"));
						if (CanAccess(path, FILE_ADD_FILE | FILE_LIST_DIRECTORY) == true) { goto found_path; }
						path = API::Helper::StringConvert::ConvertToW(API::Files::GetFullFilePath("?HOMEDRIVE?/?HOMEPATH?/Documents/My Games/CloakEngineTemp/"));
						if (CanAccess(path, FILE_ADD_FILE | FILE_LIST_DIRECTORY) == true) { goto found_path; }
						path = API::Helper::StringConvert::ConvertToW(API::Files::GetFullFilePath("root:CloakEngineTemp/"));

					found_path:
						g_tempPath = new std::string(API::Helper::StringConvert::ConvertToU8(path));
					}
					return *g_tempPath;
				}
				inline std::tstring CLOAK_CALL GetUniqueTempFileName()
				{
					const std::string path = GetTempFilePath();
					std::string file;
					std::uniform_int_distribution<uint32_t> d(0, 0xFFFF);
					std::stringstream s;
					do {
						s.str("");
						s << std::hex << std::setfill('0') << std::uppercase;
						s << std::setw(4) << d(g_randomEngine) << "-";
						s << std::setw(4) << d(g_randomEngine) << "-";
						s << std::setw(4) << d(g_randomEngine) << "-";
						s << std::setw(4) << d(g_randomEngine) << ".tmp";
						file = path + s.str();
					} while (FileExists(file) == true);
					return API::Helper::StringConvert::ConvertToT(file);
				}



				void CLOAK_CALL Initialize()
				{
					g_randomEngine = std::default_random_engine(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
					InitSecurity();
					g_filePool = new FilePool();
					g_overlappedPool = new OverlappedPool();
					g_portHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, static_cast<DWORD>(API::Global::GetExecutionThreadCount()));
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncLRU));
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncFile));
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncOverlapped));
				}
				void CLOAK_CALL CloseAllFiles()
				{
					for (auto a = g_filePool->begin(); a != g_filePool->end();)
					{
						auto& b = *(a++);
						CloseFile(&b);
					}
				}
				void CLOAK_CALL Release()
				{
					CloseHandle(g_portHandle);
					ReleaseSecurity();
					SAVE_RELEASE(g_syncOverlapped);
					SAVE_RELEASE(g_syncFile);
					SAVE_RELEASE(g_syncLRU);
					delete g_tempPath;
					delete g_filePool;
					delete g_overlappedPool;
				}

				CLOAK_CALL FileWriteBuffer::FileWriteBuffer(In const std::string& filePath, In bool silent) : FileWriteBuffer(OpenFile(API::Helper::StringConvert::ConvertToT(filePath), false, silent, false)) {}
				CLOAK_CALL FileWriteBuffer::FileWriteBuffer(In FILE_ID fileID)
				{
					DEBUG_NAME(FileWriteBuffer);
					m_file = fileID;
					m_bytePos = 0;
					m_byteOffset = 0;
					m_pagePos = 0;
					for (size_t a = 0; a < PAGE_SIZE; a++) { m_pageFence[a] = 0; }
				}
				CLOAK_CALL FileWriteBuffer::~FileWriteBuffer()
				{
					CloseFile(m_file);
				}
				unsigned long long CLOAK_CALL_THIS FileWriteBuffer::GetPosition() const
				{
					return m_byteOffset + m_bytePos;
				}
				void CLOAK_CALL_THIS FileWriteBuffer::WriteByte(In uint8_t b)
				{
					m_page[m_pagePos][m_bytePos++] = b;
					if (m_bytePos >= DATA_SIZE)
					{
						WriteData(DATA_SIZE);
						m_byteOffset += DATA_SIZE;
						m_bytePos = 0;
					}
				}
				void CLOAK_CALL_THIS FileWriteBuffer::Save()
				{
					WriteData(m_bytePos);
					m_byteOffset += m_bytePos;
					m_bytePos = 0;
				}
				void CLOAK_CALL_THIS FileWriteBuffer::Seek(In API::Files::Buffer_v1::SeekOrigin origin, In long long byteOffset)
				{
					if (origin == API::Files::Buffer_v1::SeekOrigin::CURRENT_POS && byteOffset <= 0 && static_cast<uint64_t>(m_bytePos) >= static_cast<uint64_t>(-byteOffset))
					{
						m_bytePos += static_cast<int>(byteOffset);
						m_byteOffset += byteOffset;
					}
					else
					{
						WriteData(m_bytePos);
						m_bytePos = 0;
						m_byteOffset = SeekFile(m_file, origin, byteOffset);
					}
				}

				Success(return == true) bool CLOAK_CALL_THIS FileWriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::IWriteBuffer)) { *ptr = (API::Files::IWriteBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::IBuffer)) { *ptr = (API::Files::IBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IFileBuffer)) { *ptr = (API::Files::Buffer_v1::IFileBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS FileWriteBuffer::WriteData(In uint64_t bytes)
				{
					CLOAK_ASSUME(bytes <= DATA_SIZE);
					const uint64_t p = m_pagePos;
					m_pagePos = (m_pagePos + 1) % PAGE_SIZE;
					m_pageFence[p] = WriteFile(m_file, bytes, m_page[p]);
					WaitForFile(m_file, m_pageFence[m_pagePos]);
				}

				CLOAK_CALL FileReadBuffer::FileReadBuffer(In const std::string& filePath, In bool silent) : FileReadBuffer(OpenFile(API::Helper::StringConvert::ConvertToT(filePath), true, silent, false)) {}
				CLOAK_CALL FileReadBuffer::FileReadBuffer(In FILE_ID fileID)
				{
					DEBUG_NAME(FileReadBuffer);
					m_file = fileID;
					m_byteOffset = 0;
					m_bytePos = 0;
					m_pagePos = 0;
					for (size_t a = 0; a < PAGE_SIZE; a++) 
					{ 
						m_pageLength[a] = 0;
						m_pageFence[a] = ReadFile(m_file, DATA_SIZE, m_page[a], &m_pageLength[a]); 
					}
					WaitForFile(m_file, m_pageFence[m_pagePos]);
				}
				CLOAK_CALL FileReadBuffer::~FileReadBuffer()
				{
					CloseFile(m_file);
				}
				unsigned long long CLOAK_CALL_THIS FileReadBuffer::GetPosition() const
				{
					return m_byteOffset + m_bytePos;
				}
				uint8_t CLOAK_CALL_THIS FileReadBuffer::ReadByte()
				{
					if (m_bytePos < m_pageLength[m_pagePos])
					{
						uint8_t r = m_page[m_pagePos][m_bytePos++];
						if (m_bytePos >= DATA_SIZE)
						{
							ReadData();
							m_byteOffset += m_bytePos;
							m_bytePos = 0;
						}
						return r;
					}
					return 0;
				}
				bool CLOAK_CALL_THIS FileReadBuffer::HasNextByte()
				{
					return m_bytePos < m_pageLength[m_pagePos];
				}
				void CLOAK_CALL_THIS FileReadBuffer::Seek(In API::Files::Buffer_v1::SeekOrigin origin, In long long byteOffset)
				{
					switch (origin)
					{
						case CloakEngine::API::Files::Buffer_v1::SeekOrigin::START:
							byteOffset -= m_byteOffset + m_bytePos;
							//Fall through
						case CloakEngine::API::Files::Buffer_v1::SeekOrigin::CURRENT_POS:
							if (byteOffset >= 0 || static_cast<uint64_t>(-byteOffset) < m_bytePos)
							{
								m_bytePos += byteOffset;
								uint64_t lp = m_pagePos;
								if (m_bytePos >= DATA_SIZE)
								{
									do {
										m_byteOffset += DATA_SIZE;
										m_bytePos -= DATA_SIZE;
										m_pagePos = (m_pagePos + 1) % PAGE_SIZE;
										WaitForFile(m_file, m_pageFence[m_pagePos]);
									} while (m_bytePos >= DATA_SIZE && lp != m_pagePos);
								}
								if (m_bytePos < DATA_SIZE) 
								{
									//We managed to seek without flushing previously reads. However, if we seeked beyond a page, we will need to sequence new reads
									while (lp != m_pagePos)
									{
										m_pageFence[lp] = ReadFile(m_file, DATA_SIZE, m_page[lp], &m_pageLength[lp]);
										lp = (lp + 1) % PAGE_SIZE;
									}
									return; 
								}
							}
							origin = CloakEngine::API::Files::Buffer_v1::SeekOrigin::START;
							byteOffset += m_byteOffset + m_bytePos;
							break;
						case CloakEngine::API::Files::Buffer_v1::SeekOrigin::END:
							break;
						default:
							break;
					}
					//Seek, flush all previously sequenced reads, and sequence new reading calls
					m_byteOffset = SeekFile(m_file, origin, byteOffset);
					m_bytePos = 0;
					m_pagePos = 0;
					for (size_t a = 0; a < PAGE_SIZE; a++)
					{
						m_pageLength[a] = 0;
						m_pageFence[a] = ReadFile(m_file, DATA_SIZE, m_page[a], &m_pageLength[a]);
					}
					WaitForFile(m_file, m_pageFence[m_pagePos]);
				}

				Success(return == true) bool CLOAK_CALL_THIS FileReadBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IReadBuffer)) { *ptr = (API::Files::Buffer_v1::IReadBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IBuffer)) { *ptr = (API::Files::Buffer_v1::IBuffer*)this; return true; }
					else if (riid == __uuidof(API::Files::Buffer_v1::IFileBuffer)) { *ptr = (API::Files::Buffer_v1::IFileBuffer*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
				void CLOAK_CALL_THIS FileReadBuffer::ReadData()
				{
					if constexpr (PAGE_SIZE > 1)
					{
						const uint64_t op = m_pagePos;
						m_pagePos = (m_pagePos + 1) % PAGE_SIZE;
						WaitForFile(m_file, m_pageFence[m_pagePos]);
						m_pageFence[op] = ReadFile(m_file, DATA_SIZE, m_page[op], &m_pageLength[op]);
					}
					else
					{
						m_pageFence[0] = ReadFile(m_file, DATA_SIZE, m_page[0], &m_pageLength[0]);
						WaitForFile(m_file, m_pageFence[0]);
					}
					CLOAK_ASSUME(m_pageLength[m_pagePos] <= DATA_SIZE);
				}

				CLOAK_CALL TempFileWriteBuffer::TempFileWriteBuffer() : FileWriteBuffer(OpenFile(GetUniqueTempFileName(), false, false, true)), m_state(0)
				{
					DEBUG_NAME(TempFileWriteBuffer);
					m_syncFile = m_file;
				}
				CE::RefPointer<API::Files::Buffer_v1::IReadBuffer> CLOAK_CALL_THIS TempFileWriteBuffer::GetReadBuffer()
				{
					return CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>::Construct<TempFileReadBuffer>(OpenFile(m_file, true, false, true), this);
				}
				void CLOAK_CALL_THIS TempFileWriteBuffer::Clear()
				{
					Save();
					CloseFile(m_file);
					m_file = OpenFile(GetUniqueTempFileName(), false, false, true);
					m_bytePos = 0;
					m_byteOffset = 0;
					m_syncFile = m_file;
					m_state++;
				}

				Success(return == true) bool CLOAK_CALL_THIS TempFileWriteBuffer::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::Buffer_v1::IVirtualWriteBuffer)) { *ptr = (API::Files::Buffer_v1::IVirtualWriteBuffer*)this; return true; }
					return FileWriteBuffer::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL TempFileReadBuffer::TempFileReadBuffer(In FILE_ID fileID, In TempFileWriteBuffer* parent) : m_parent(parent), FileReadBuffer(fileID)
				{
					CLOAK_ASSUME(parent != nullptr);
					parent->AddRef();
					m_state = parent->m_state;
				}
				CLOAK_CALL TempFileReadBuffer::~TempFileReadBuffer()
				{
					m_parent->Release();
				}

				uint8_t CLOAK_CALL_THIS TempFileReadBuffer::ReadByte()
				{
					checkReload();
					return FileReadBuffer::ReadByte();
				}
				bool CLOAK_CALL_THIS TempFileReadBuffer::HasNextByte()
				{
					checkReload();
					return FileReadBuffer::HasNextByte();
				}
				void CLOAK_CALL_THIS TempFileReadBuffer::Seek(In API::Files::Buffer_v1::SeekOrigin origin, In long long byteOffset)
				{
					checkReload();
					return FileReadBuffer::Seek(origin, byteOffset);
				}

				void CLOAK_CALL_THIS TempFileReadBuffer::checkReload()
				{
					uint64_t state = m_parent->m_state.load();
					if (m_state != state)
					{
						do {
							CloseFile(m_file);
							m_state = state;
							m_file = OpenFile(m_parent->m_syncFile.load(), true, false, true);
						} while (m_state != (state = m_parent->m_state.load()));
						m_byteOffset = 0;
						m_bytePos = 0;
						ReadData();
					}
				}
			}
		}
	}
}