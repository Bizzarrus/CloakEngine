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
				class CLOAKENGINE_API Lock {
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
			}
		}
	}
}

#endif
