#include "stdafx.h"
#include "Implementation/Helper/SyncSection.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Global/Memory.h"

//If defined, currently locked objects are stored in a thread local hash list. If not, locked objects are stored in an global hash list
//#define REGISTER_THREAD_LOCAL

namespace CloakEngine {
	namespace Impl {
		namespace Helper {
			namespace SyncSection_v1 {
				typedef typename int_by_size<max(sizeof(void*) / 2, 2)>::unsigned_t SyncCount_t;
				struct SyncCount {
					SyncCount_t ReadCount;
					SyncCount_t WriteCount;
				};
#ifdef REGISTER_THREAD_LOCAL
				Impl::Global::Memory::LocalStorage::StorageID g_registerID;
				typedef API::HashList<ILockObject*> RegisterList;
#else
				API::HashList<ILockObject*>* g_lockObjects;
				SRWLOCK g_srwObjects = SRWLOCK_INIT;

				CLOAK_FORCEINLINE void CLOAK_CALL RegisterObject(In ILockObject* o)
				{
					AcquireSRWLockExclusive(&g_srwObjects);
					g_lockObjects->insert(o);
					ReleaseSRWLockExclusive(&g_srwObjects);
				}
#endif

				CLOAK_CALL WAIT_EVENT::WAIT_EVENT(In bool initState) : m_state(initState), m_srw(SRWLOCK_INIT)
				{
					InitializeConditionVariable(&m_cv);
				}
				CLOAK_CALL WAIT_EVENT::~WAIT_EVENT()
				{

				}
				void CLOAK_CALL_THIS WAIT_EVENT::Set()
				{
					AcquireSRWLockExclusive(&m_srw);
					m_state = true;
					WakeAllConditionVariable(&m_cv);
					ReleaseSRWLockExclusive(&m_srw);
				}
				void CLOAK_CALL_THIS WAIT_EVENT::Reset()
				{
					AcquireSRWLockExclusive(&m_srw);
					m_state = false;
					ReleaseSRWLockExclusive(&m_srw);
				}
				void CLOAK_CALL_THIS WAIT_EVENT::Wait()
				{
					AcquireSRWLockExclusive(&m_srw);
					while (m_state == false) { SleepConditionVariableSRW(&m_cv, &m_srw, INFINITE, 0); }
					ReleaseSRWLockExclusive(&m_srw);
				}

				CLOAK_CALL_THIS SyncSection::SyncSection() : m_srw(SRWLOCK_INIT)
				{
					DEBUG_NAME(SyncSection);
					m_id = API::Global::Memory::ThreadLocal::Allocate();
#ifndef REGISTER_THREAD_LOCAL
					RegisterObject(this);
#endif
				}
				CLOAK_CALL_THIS SyncSection::~SyncSection()
				{
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { API::Global::Memory::ThreadLocal::Free(m_id); }
					else
					{
						API::Global::Memory::ThreadLocal::Free(m_id, [](In void* ptr) {API::Global::Memory::MemoryPool::Free(ptr); });
					}
				}

				API::Global::Debug::Error CLOAK_CALL_THIS SyncSection::Delete()
				{
#ifndef REGISTER_THREAD_LOCAL
					AcquireSRWLockExclusive(&g_srwObjects);
					if (GetRefCount() == 0) { g_lockObjects->erase(this); }
					else { ReleaseSRWLockExclusive(&g_srwObjects); return API::Global::Debug::Error::ILLEGAL_FUNC_CALL; }
					ReleaseSRWLockExclusive(&g_srwObjects);
#endif
					return SavePtr::Delete();
				}

				bool CLOAK_CALL_THIS SyncSection::isLockedExclusive()
				{
					AddRef();
					bool res = true;
					SyncCount* i;
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { i = reinterpret_cast<SyncCount*>(API::Global::Memory::ThreadLocal::Get(m_id)); }
					else 
					{
						SyncCount** ri = reinterpret_cast<SyncCount**>(API::Global::Memory::ThreadLocal::Get(m_id));
						if (*ri == nullptr) { *ri = ::new(API::Global::Memory::MemoryPool::Allocate(sizeof(SyncCount)))SyncCount(); }
						i = *ri;
					}
					if (i->WriteCount > 0) { res = true; }
					else if (i->ReadCount > 0) { res = false; }
					else if (TryAcquireSRWLockExclusive(&m_srw))
					{
						ReleaseSRWLockExclusive(&m_srw);
						res = false;
					}
					Release();
					return res;
				}
				bool CLOAK_CALL_THIS SyncSection::isLockedShared()
				{
					AddRef();
					bool res = true;
					SyncCount* i;
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { i = reinterpret_cast<SyncCount*>(API::Global::Memory::ThreadLocal::Get(m_id)); }
					else
					{
						SyncCount** ri = reinterpret_cast<SyncCount * *>(API::Global::Memory::ThreadLocal::Get(m_id));
						if (*ri == nullptr) { *ri = ::new(API::Global::Memory::MemoryPool::Allocate(sizeof(SyncCount)))SyncCount(); }
						i = *ri;
					}
					if (i->ReadCount > 0 || i->WriteCount > 0) { res = true; }
					else if(TryAcquireSRWLockShared(&m_srw))
					{
						ReleaseSRWLockShared(&m_srw);
						res = false;
					}
					Release();
					return res;
				}
				void CLOAK_CALL_THIS SyncSection::lockExclusive()
				{
					AddRef();
					SyncCount* i;
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { i = reinterpret_cast<SyncCount*>(API::Global::Memory::ThreadLocal::Get(m_id)); }
					else
					{
						SyncCount** ri = reinterpret_cast<SyncCount * *>(API::Global::Memory::ThreadLocal::Get(m_id));
						if (*ri == nullptr) { *ri = ::new(API::Global::Memory::MemoryPool::Allocate(sizeof(SyncCount)))SyncCount(); }
						i = *ri;
					}
					if (i->WriteCount == 0)
					{
						if (i->ReadCount > 0) { ReleaseSRWLockShared(&m_srw); }
						else 
						{ 
							AddRef(); 
#ifdef REGISTER_THREAD_LOCAL
							RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
							if (*rl == nullptr) { *rl = new RegisterList(); }
							(*rl)->insert(this);
#endif
						}
						AcquireSRWLockExclusive(&m_srw);
					}
					i->WriteCount++;
					Release();
				}
				void CLOAK_CALL_THIS SyncSection::unlockExclusive()
				{
					AddRef();
					SyncCount* i;
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { i = reinterpret_cast<SyncCount*>(API::Global::Memory::ThreadLocal::Get(m_id)); }
					else
					{
						SyncCount** ri = reinterpret_cast<SyncCount * *>(API::Global::Memory::ThreadLocal::Get(m_id));
						if (*ri == nullptr) { *ri = ::new(API::Global::Memory::MemoryPool::Allocate(sizeof(SyncCount)))SyncCount(); }
						i = *ri;
					}
					CLOAK_ASSUME(i->WriteCount > 0);
					i->WriteCount--;
					if (i->WriteCount == 0)
					{
						ReleaseSRWLockExclusive(&m_srw);
						if (i->ReadCount > 0) { AcquireSRWLockShared(&m_srw); }
						else 
						{
#ifdef REGISTER_THREAD_LOCAL
							RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
							CLOAK_ASSUME(*rl != nullptr);
							(*rl)->erase(this);
#endif
							Release(); 
						}
					}
					Release();
				}
				void CLOAK_CALL_THIS SyncSection::lockShared()
				{
					AddRef();
					SyncCount* i;
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { i = reinterpret_cast<SyncCount*>(API::Global::Memory::ThreadLocal::Get(m_id)); }
					else
					{
						SyncCount** ri = reinterpret_cast<SyncCount * *>(API::Global::Memory::ThreadLocal::Get(m_id));
						if (*ri == nullptr) { *ri = ::new(API::Global::Memory::MemoryPool::Allocate(sizeof(SyncCount)))SyncCount(); }
						i = *ri;
					}
					if (i->ReadCount == 0 && i->WriteCount == 0)
					{
						AddRef();
#ifdef REGISTER_THREAD_LOCAL
						RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
						if (*rl == nullptr) { *rl = new RegisterList(); }
						(*rl)->insert(this);
#endif
						AcquireSRWLockShared(&m_srw);
					}
					i->ReadCount++;
					Release();
				}
				void CLOAK_CALL_THIS SyncSection::unlockShared()
				{
					AddRef();
					SyncCount* i;
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { i = reinterpret_cast<SyncCount*>(API::Global::Memory::ThreadLocal::Get(m_id)); }
					else
					{
						SyncCount** ri = reinterpret_cast<SyncCount * *>(API::Global::Memory::ThreadLocal::Get(m_id));
						if (*ri == nullptr) { *ri = ::new(API::Global::Memory::MemoryPool::Allocate(sizeof(SyncCount)))SyncCount(); }
						i = *ri;
					}
					CLOAK_ASSUME(i->ReadCount > 0);
					i->ReadCount--;
					if (i->ReadCount == 0 && i->WriteCount == 0)
					{
						ReleaseSRWLockShared(&m_srw);
#ifdef REGISTER_THREAD_LOCAL
						RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
						CLOAK_ASSUME(*rl != nullptr);
						(*rl)->erase(this);
#endif
						Release();
					}
					Release();
				}

				bool CLOAK_CALL_THIS SyncSection::Unlock(Out LOCK_STATE* state)
				{
					CLOAK_ASSUME(state != nullptr);
					SyncCount* i;
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { i = reinterpret_cast<SyncCount*>(API::Global::Memory::ThreadLocal::Get(m_id)); }
					else
					{
						SyncCount** ri = reinterpret_cast<SyncCount * *>(API::Global::Memory::ThreadLocal::Get(m_id));
						if (*ri == nullptr) { *ri = ::new(API::Global::Memory::MemoryPool::Allocate(sizeof(SyncCount)))SyncCount(); }
						i = *ri;
					}
					if (i->ReadCount > 0 || i->WriteCount > 0)
					{
						state->State16[0] = i->ReadCount;
						state->State16[1] = i->WriteCount;
						if (i->WriteCount > 0) { ReleaseSRWLockExclusive(&m_srw); }
						else { ReleaseSRWLockShared(&m_srw); }
						i->ReadCount = 0;
						i->WriteCount = 0;
						return true;
					}
					return false;
				}
				void CLOAK_CALL_THIS SyncSection::Recover(In const LOCK_STATE& state)
				{
					SyncCount* i;
					if constexpr (sizeof(SyncCount) <= sizeof(void*)) { i = reinterpret_cast<SyncCount*>(API::Global::Memory::ThreadLocal::Get(m_id)); }
					else
					{
						SyncCount** ri = reinterpret_cast<SyncCount * *>(API::Global::Memory::ThreadLocal::Get(m_id));
						if (*ri == nullptr) { *ri = ::new(API::Global::Memory::MemoryPool::Allocate(sizeof(SyncCount)))SyncCount(); }
						i = *ri;
					}
					CLOAK_ASSUME(i->ReadCount == 0 && i->WriteCount == 0);
					CLOAK_ASSUME(state.State16[0] > 0 || state.State16[1] > 0);
					i->ReadCount = state.State16[0];
					i->WriteCount = state.State16[1];
					if (i->WriteCount > 0) { AcquireSRWLockExclusive(&m_srw); }
					else { AcquireSRWLockShared(&m_srw); }
				}

				void* CLOAK_CALL SyncSection::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL SyncSection::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS SyncSection::iQueryInterface(In REFIID id, Outptr void** ptr)
				{
					CLOAK_QUERY(id, ptr, SavePtr, Helper::SyncSection_v1, SyncSection);
				}

				CLOAK_CALL ConditionSyncSection::ConditionSyncSection() : m_cv(CONDITION_VARIABLE_INIT)
				{
					DEBUG_NAME(ConditionSyncSection);
				}
				CLOAK_CALL ConditionSyncSection::~ConditionSyncSection()
				{

				}
				void CLOAK_CALL_THIS ConditionSyncSection::lockExclusive(In const std::function<bool()>& condition)
				{
					AddRef();
					SyncSection::lockExclusive();
					if (condition)
					{
						while (condition() == false) { SleepConditionVariableSRW(&m_cv, &m_srw, 500, 0); }
					}
					else
					{
						SleepConditionVariableSRW(&m_cv, &m_srw, INFINITE, 0);
					}
					Release();
				}
				void CLOAK_CALL_THIS ConditionSyncSection::lockShared(In const std::function<bool()>& condition)
				{
					AddRef();
					SyncSection::lockShared();
					if (condition)
					{
						while (condition() == false) { SleepConditionVariableSRW(&m_cv, &m_srw, 500, CONDITION_VARIABLE_LOCKMODE_SHARED); }
					}
					else
					{
						SleepConditionVariableSRW(&m_cv, &m_srw, INFINITE, CONDITION_VARIABLE_LOCKMODE_SHARED);
					}
					Release();
				}
				void CLOAK_CALL_THIS ConditionSyncSection::signalOne()
				{
					AddRef();
					WakeConditionVariable(&m_cv);
					Release();
				}
				void CLOAK_CALL_THIS ConditionSyncSection::signalAll()
				{
					AddRef();
					WakeAllConditionVariable(&m_cv);
					Release();
				}

				Success(return == true) bool CLOAK_CALL_THIS ConditionSyncSection::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SyncSection, Helper::SyncSection_v1, ConditionSyncSection);
				}

				CLOAK_CALL Semaphore::Semaphore() : m_srw(SRWLOCK_INIT), m_resize(true), m_emptySlots(true), m_fullSlots(false)
				{
					DEBUG_NAME(Semaphore);
					m_slots = 1;
					m_usedSlots = 0;
					m_regSlots = 0;
					m_readSlots = 0;
					m_tryResize = false;
					m_id = API::Global::Memory::ThreadLocal::Allocate();
#ifndef REGISTER_THREAD_LOCAL
					RegisterObject(this);
#endif
				}
				CLOAK_CALL Semaphore::~Semaphore()
				{
					API::Global::Memory::ThreadLocal::Free(m_id);
				}

				API::Global::Debug::Error CLOAK_CALL_THIS Semaphore::Delete()
				{
#ifndef REGISTER_THREAD_LOCAL
					AcquireSRWLockExclusive(&g_srwObjects);
					if (GetRefCount() == 0) { g_lockObjects->erase(this); }
					else { ReleaseSRWLockExclusive(&g_srwObjects); return API::Global::Debug::Error::ILLEGAL_FUNC_CALL; }
					ReleaseSRWLockExclusive(&g_srwObjects);
#endif
					return SavePtr::Delete();
				}

				bool CLOAK_CALL_THIS Semaphore::isLockedExclusive() 
				{ 
					AddRef();
					AcquireSRWLockExclusive(&m_srw);
					bool r = m_regSlots > 0;
					ReleaseSRWLockExclusive(&m_srw);
					Release();
					return r;
				}
				bool CLOAK_CALL_THIS Semaphore::isLockedShared()
				{
					AddRef();
					AcquireSRWLockExclusive(&m_srw);
					bool r = m_usedSlots > m_readSlots;
					ReleaseSRWLockExclusive(&m_srw);
					Release();
					return r;
				}
				void CLOAK_CALL_THIS Semaphore::lockExclusive()
				{
					AddRef();
					intptr_t* i = reinterpret_cast<intptr_t*>(API::Global::Memory::ThreadLocal::Get(m_id));
					CLOAK_ASSUME((*i) <= 0);
					if ((*i) == 0)
					{
						AcquireSRWLockExclusive(&m_srw);
						if (m_tryResize) 
						{ 
							ReleaseSRWLockExclusive(&m_srw);
							m_resize.Wait();
							AcquireSRWLockExclusive(&m_srw);
						}
						while (m_regSlots == m_slots)
						{
							ReleaseSRWLockExclusive(&m_srw);
							m_emptySlots.Wait();
							AcquireSRWLockExclusive(&m_srw);
						}
						m_regSlots++;
						if (m_regSlots == m_slots) { m_emptySlots.Reset(); }
						ReleaseSRWLockExclusive(&m_srw);
					}
					(*i) = (*i) - 1;
#ifdef REGISTER_THREAD_LOCAL
					RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
					if (*rl == nullptr) { *rl = new RegisterList(); }
					(*rl)->insert(this);
#endif
				}
				void CLOAK_CALL_THIS Semaphore::unlockExclusive()
				{
					intptr_t* i = reinterpret_cast<intptr_t*>(API::Global::Memory::ThreadLocal::Get(m_id));
					CLOAK_ASSUME((*i) <= 0);
					(*i) = (*i) + 1;
					if ((*i) == 0)
					{
						AcquireSRWLockExclusive(&m_srw);
						m_usedSlots++;
						m_fullSlots.Set();
						ReleaseSRWLockExclusive(&m_srw);
					}
#ifdef REGISTER_THREAD_LOCAL
					RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
					CLOAK_ASSUME(*rl != nullptr);
					(*rl)->erase(this);
#endif
					Release();
				}
				void CLOAK_CALL_THIS Semaphore::lockShared()
				{
					AddRef();
					intptr_t* i = reinterpret_cast<intptr_t*>(API::Global::Memory::ThreadLocal::Get(m_id));
					CLOAK_ASSUME((*i) >= 0);
					if ((*i) == 0)
					{
						AcquireSRWLockExclusive(&m_srw);
						while (m_usedSlots == m_readSlots)
						{ 
							ReleaseSRWLockExclusive(&m_srw);
							m_fullSlots.Wait();
							AcquireSRWLockExclusive(&m_srw);
						}
						m_readSlots++;
						if (m_readSlots == m_usedSlots) { m_fullSlots.Reset(); }
						ReleaseSRWLockExclusive(&m_srw);
					}
					(*i) = (*i) + 1;
#ifdef REGISTER_THREAD_LOCAL
					RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
					if (*rl == nullptr) { *rl = new RegisterList(); }
					(*rl)->insert(this);
#endif
				}
				void CLOAK_CALL_THIS Semaphore::unlockShared()
				{
					intptr_t* i = reinterpret_cast<intptr_t*>(API::Global::Memory::ThreadLocal::Get(m_id));
					CLOAK_ASSUME((*i) >= 0);
					(*i) = (*i) - 1;
					if ((*i) == 0)
					{
						AcquireSRWLockExclusive(&m_srw);
						m_usedSlots--;
						m_readSlots--;
						m_regSlots--;
						m_emptySlots.Set();
						ReleaseSRWLockExclusive(&m_srw);
					}
#ifdef REGISTER_THREAD_LOCAL
					RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
					CLOAK_ASSUME(*rl != nullptr);
					(*rl)->erase(this);
#endif
					Release();
				}
				void CLOAK_CALL_THIS Semaphore::setSlotCount(In size_t count)
				{
					AddRef();
					AcquireSRWLockExclusive(&m_srw);
					m_tryResize = true;
					m_resize.Reset();
					while (m_regSlots > count)
					{
						ReleaseSRWLockExclusive(&m_srw);
						m_emptySlots.Wait();
						AcquireSRWLockExclusive(&m_srw);
					}
					m_slots = static_cast<uint32_t>(count);
					m_tryResize = false;
					m_resize.Set();
					ReleaseSRWLockExclusive(&m_srw);
					Release();
				}

				bool CLOAK_CALL_THIS Semaphore::Unlock(Out LOCK_STATE* state)
				{
					CLOAK_ASSUME(state != nullptr);
					intptr_t* i = reinterpret_cast<intptr_t*>(API::Global::Memory::ThreadLocal::Get(m_id));
					if ((*i) != 0)
					{
						state->State32 = static_cast<int32_t>(*i);
						*i = 0;
						return true;
					}
					return false;
				}
				void CLOAK_CALL_THIS Semaphore::Recover(In const LOCK_STATE& state)
				{
					intptr_t* i = reinterpret_cast<intptr_t*>(API::Global::Memory::ThreadLocal::Get(m_id));
					CLOAK_ASSUME((*i) == 0);
					CLOAK_ASSUME(state.State32 != 0);
					*i = state.State32;
				}

				void* CLOAK_CALL Semaphore::operator new(In size_t size) { return API::Global::Memory::MemoryPool::Allocate(size); }
				void CLOAK_CALL Semaphore::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }

				Success(return == true) bool CLOAK_CALL_THIS Semaphore::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SavePtr, Helper::SyncSection_v1, Semaphore);
				}

				namespace Lock {
					void CLOAK_CALL UnlockAll(Out LOCK_DATA* data)
					{
						CLOAK_ASSUME(data != nullptr);
						data->clear();
						LOCK_STATE state;
#ifdef REGISTER_THREAD_LOCAL
						RegisterList** rl = reinterpret_cast<RegisterList**>(Impl::Global::Memory::LocalStorage::Get(g_registerID));
						if (*rl != nullptr)
						{
							for (auto a : **rl)
							{
								if (a->Unlock(&state) == true)
								{
									a->AddRef();
									data->push_back(std::make_pair(a, state));
								}
							}
						}
#else
						AcquireSRWLockExclusive(&g_srwObjects);
						for (auto a : *g_lockObjects)
						{
							if (a->Unlock(&state) == true)
							{
								a->AddRef();
								data->push_back(std::make_pair(a, state));
							}
						}
						ReleaseSRWLockExclusive(&g_srwObjects);
#endif
					}
					void CLOAK_CALL RecoverAll(In const LOCK_DATA& data)
					{
						for (auto a : data)
						{
							a.first->Recover(a.second);
							a.first->Release();
						}
					}
					void CLOAK_CALL Initiate()
					{
#ifdef REGISTER_THREAD_LOCAL
						g_registerID = Impl::Global::Memory::LocalStorage::Allocate(sizeof(RegisterList*));
#else
						g_lockObjects = new API::HashList<ILockObject*>();
#endif
					}
					void CLOAK_CALL Release()
					{
#ifdef REGISTER_THREAD_LOCAL
						Impl::Global::Memory::LocalStorage::ForAll(g_registerID, [](In void* p) {
							RegisterList** rl = reinterpret_cast<RegisterList**>(p);
							if (*rl != nullptr) { delete *rl; }
						});
						Impl::Global::Memory::LocalStorage::Release(g_registerID);
#else
						delete g_lockObjects;
#endif
					}
				}
			}
		}
	}
}