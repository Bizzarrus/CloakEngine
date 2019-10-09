#pragma once
#ifndef CE_API_HELPER_SYNCSECTION_H
#define CE_API_HELPER_SYNCSECTION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"

#include <functional>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Helper {
			inline namespace SyncSection_v1 {
				CLOAK_INTERFACE_ID("{6DE8C4C5-06C9-4F93-866A-7E5D1AF4B083}") ISyncSection : public virtual SavePtr_v1::ISavePtr {
					public:
						virtual bool CLOAK_CALL_THIS isLockedExclusive() = 0;
						virtual bool CLOAK_CALL_THIS isLockedShared() = 0;
						virtual void CLOAK_CALL_THIS lockExclusive() = 0;
						virtual void CLOAK_CALL_THIS unlockExclusive() = 0;
						virtual void CLOAK_CALL_THIS lockShared() = 0;
						virtual void CLOAK_CALL_THIS unlockShared() = 0;
				};
				CLOAK_INTERFACE_ID("{4469CB38-D23A-4A36-95CE-0B2C47128950}") IConditionSyncSection : public virtual ISyncSection{
					public:
						virtual void CLOAK_CALL_THIS lockExclusive(In const std::function<bool()>& condition) = 0;
						virtual void CLOAK_CALL_THIS lockShared(In const std::function<bool()>& condition) = 0;
						virtual void CLOAK_CALL_THIS signalOne() = 0;
						virtual void CLOAK_CALL_THIS signalAll() = 0;
				};
				CLOAK_INTERFACE_ID("{7509F6D1-C1EE-4414-A1E6-12CE399350F6}") ISemaphore : public virtual ISyncSection{
					public:
						virtual void CLOAK_CALL_THIS setSlotCount(In size_t count) = 0;
				};
				template class CLOAKENGINE_API CE::RefPointer<ISyncSection>;
				class ReadWriteLock;
				class CLOAKENGINE_API Lock {
					friend class ReadWriteLock;
					private:
						CE::RefPointer<ISyncSection> m_sync;
					public:
						explicit CLOAK_CALL_THIS Lock(In_opt const CE::RefPointer<SyncSection_v1::ISyncSection>& ptr = nullptr) : m_sync(ptr)
						{
							if (m_sync != nullptr) { m_sync->lockExclusive(); }
						}
						CLOAK_CALL_THIS Lock(In const CE::RefPointer<IConditionSyncSection>& ptr, In std::function<bool()> condition) : m_sync(ptr)
						{
							if (ptr != nullptr) { ptr->lockExclusive(condition); }
						}
						CLOAK_CALL_THIS Lock(In const Lock& lock) : Lock(lock.m_sync) {}
						CLOAK_CALL_THIS Lock(In Lock&& lock) : m_sync(lock.m_sync) { lock.m_sync = nullptr; }
						CLOAK_CALL_THIS ~Lock() { if (m_sync != nullptr) { m_sync->unlockExclusive(); } }
						inline void CLOAK_CALL_THIS unlock()
						{
							if (m_sync != nullptr) { m_sync->unlockExclusive(); }
							m_sync = nullptr;
						}
						inline void CLOAK_CALL_THIS lock(In const CE::RefPointer<SyncSection_v1::ISyncSection>& ptr)
						{
							if (m_sync != nullptr) { m_sync->unlockExclusive(); }
							m_sync = ptr;
							if (m_sync != nullptr) { m_sync->lockExclusive(); }
						}
						inline void CLOAK_CALL_THIS lock(In const CE::RefPointer<IConditionSyncSection>& ptr, In std::function<bool()> condition)
						{
							if (m_sync != nullptr) { m_sync->unlockExclusive(); }
							m_sync = ptr;
							if (ptr != nullptr) { ptr->lockExclusive(condition); }
						}
						inline Lock& CLOAK_CALL_THIS operator=(In const Lock& o)
						{
							if (m_sync != o.m_sync) 
							{ 
								if (o.m_sync == nullptr) { unlock(); }
								else { lock(o.m_sync); }
							}
							return *this;
						}
						inline Lock& CLOAK_CALL_THIS operator=(In Lock&& o)
						{
							if (m_sync != o.m_sync)
							{
								if (o.m_sync == nullptr) { unlock(); }
								else 
								{ 
									if (m_sync != nullptr) { m_sync->unlockExclusive(); }
									m_sync = o.m_sync;
								}
							}
							o.m_sync = nullptr;
							return *this;
						}
				};
				class CLOAKENGINE_API ReadLock {
					friend class ReadWriteLock;
					private:
						CE::RefPointer<ISyncSection> m_sync;
					public:
						explicit CLOAK_CALL_THIS ReadLock(In_opt const CE::RefPointer<SyncSection_v1::ISyncSection>& ptr = nullptr) : m_sync(ptr)
						{
							if (m_sync != nullptr) { m_sync->lockShared(); }
						}
						CLOAK_CALL_THIS ReadLock(In CE::RefPointer<IConditionSyncSection>& ptr, In std::function<bool()> condition) : m_sync(ptr)
						{
							if (ptr != nullptr) { ptr->lockShared(condition); }
						}
						CLOAK_CALL_THIS ReadLock(In const ReadLock& lock) : ReadLock(lock.m_sync) {}
						CLOAK_CALL_THIS ReadLock(In ReadLock&& lock) : m_sync(lock.m_sync) { lock.m_sync = nullptr; }
						CLOAK_CALL_THIS ~ReadLock()
						{
							if (m_sync != nullptr) { m_sync->unlockShared(); }
						}
						inline void CLOAK_CALL_THIS unlock()
						{
							if (m_sync != nullptr) { m_sync->unlockShared(); }
							m_sync = nullptr;
						}
						inline void CLOAK_CALL_THIS lock(In const CE::RefPointer<SyncSection_v1::ISyncSection>& ptr)
						{
							if (m_sync != nullptr) { m_sync->unlockShared(); }
							m_sync = ptr;
							if (m_sync != nullptr) { m_sync->lockShared(); }
						}
						inline void CLOAK_CALL_THIS lock(In const CE::RefPointer<IConditionSyncSection>& ptr, In std::function<bool()> condition)
						{
							if (m_sync != nullptr) { m_sync->unlockShared(); }
							m_sync = ptr;
							if (ptr != nullptr) { ptr->lockShared(condition); }
						}
						inline ReadLock& CLOAK_CALL_THIS operator=(In const ReadLock& o)
						{
							if (m_sync != o.m_sync)
							{
								if (o.m_sync == nullptr) { unlock(); }
								else { lock(o.m_sync); }
							}
							return *this;
						}
						inline ReadLock& CLOAK_CALL_THIS operator=(In ReadLock&& o)
						{
							if (m_sync != o.m_sync)
							{
								if (o.m_sync == nullptr) { unlock(); }
								else 
								{ 
									if (m_sync != nullptr) { m_sync->unlockShared(); }
									m_sync = o.m_sync;
								}
							}
							o.m_sync = nullptr;
							return *this;
						}
				};
				typedef Lock WriteLock;
				class CLOAKENGINE_API ReadWriteLock {
					private:
						typedef SyncSection_v1::ISyncSection section_t;
						static constexpr uintptr_t MODE_MASK = std::alignment_of<section_t*>::value - 1;
						static constexpr uintptr_t READ_MODE = 0 & MODE_MASK;
						static constexpr uintptr_t WRITE_MODE = 1 & MODE_MASK;

						uintptr_t m_sync;

						CLOAK_FORCEINLINE section_t* CLOAK_CALL_THIS GetSection() const { return reinterpret_cast<section_t*>(m_sync & ~MODE_MASK); }
						CLOAK_FORCEINLINE bool CLOAK_CALL_THIS IsWriteMode() const { return IsFlagSet(m_sync, WRITE_MODE); }
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS SetSection(In section_t* s, In bool write) 
						{
							if (s != nullptr)
							{
								s->AddRef();
								if (write == true) { s->lockExclusive(); }
								else { s->lockShared(); }
							}
							m_sync = reinterpret_cast<uintptr_t>(s) | (write == true ? WRITE_MODE : READ_MODE); 
						}
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS SetSection(In IConditionSyncSection* s, In bool write, In std::function<bool()> condition)
						{
							if (s != nullptr)
							{
								s->AddRef();
								if (write == true) { s->lockExclusive(condition); }
								else { s->lockShared(condition); }
							}
							m_sync = reinterpret_cast<uintptr_t>(static_cast<section_t*>(s)) | (write == true ? WRITE_MODE : READ_MODE);
						}
						CLOAK_FORCEINLINE void CLOAK_CALL_THIS iUnlock() const
						{
							section_t* s = GetSection();
							if (s != nullptr)
							{
								if (IsWriteMode() == true) { s->unlockExclusive(); }
								else { s->unlockShared(); }
								s->Release();
							}
						}
					public:
						CLOAK_CALL_THIS ReadWriteLock() : m_sync(0) {}
						explicit CLOAK_CALL_THIS ReadWriteLock(In const CE::RefPointer<section_t>& ptr) { SetSection(ptr.Get(), true); }
						CLOAK_CALL_THIS ReadWriteLock(In const CE::RefPointer<IConditionSyncSection>& ptr, In std::function<bool()> condition) { SetSection(ptr.Get(), true); }
						CLOAK_CALL_THIS ReadWriteLock(In const ReadWriteLock& lock) : m_sync(lock.m_sync) 
						{
							section_t* s = GetSection();
							if (s != nullptr)
							{
								s->AddRef();
								s->lockExclusive();
							}
						}
						CLOAK_CALL_THIS ReadWriteLock(In ReadWriteLock&& lock) : m_sync(lock.m_sync) { lock.m_sync = 0; }
						CLOAK_CALL_THIS ReadWriteLock(In const ReadLock& lock) { SetSection(lock.m_sync.Get(), false); }
						CLOAK_CALL_THIS ReadWriteLock(In const WriteLock& lock) { SetSection(lock.m_sync.Get(), true); }
						CLOAK_CALL_THIS ~ReadWriteLock() { iUnlock(); }
						inline void CLOAK_CALL_THIS unlock()
						{
							iUnlock();
							m_sync = 0;
						}
						inline void CLOAK_CALL_THIS lock(In const CE::RefPointer<section_t>& ptr)
						{
							iUnlock();
							SetSection(ptr.Get(), true);
						}
						inline void CLOAK_CALL_THIS lock(In const CE::RefPointer<IConditionSyncSection>& ptr, In std::function<bool()> condition)
						{
							iUnlock();
							SetSection(ptr.Get(), true, condition);
						}
						inline ReadWriteLock& CLOAK_CALL_THIS operator=(In const ReadWriteLock& o)
						{
							if (m_sync != o.m_sync)
							{
								section_t* s = o.GetSection();
								s->AddRef();
								iUnlock();
								SetSection(s, o.IsWriteMode());
								s->Release();
							}
							return *this;
						}
						inline ReadWriteLock& CLOAK_CALL_THIS operator=(In const ReadLock& o)
						{
							iUnlock();
							SetSection(o.m_sync.Get(), false);
							return *this;
						}
						inline ReadWriteLock& CLOAK_CALL_THIS operator=(In const WriteLock& o)
						{
							iUnlock();
							SetSection(o.m_sync.Get(), true);
							return *this;
						}
				};
			}
		}
	}
}

#endif
