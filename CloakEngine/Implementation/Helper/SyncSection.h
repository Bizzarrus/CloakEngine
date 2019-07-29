#pragma once
#ifndef CE_IMPL_HELPER_SYNCSECTION_H
#define CE_IMPL_HELPER_SYNCSECTION_H

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Memory.h"
#include "Implementation/Helper/SavePtr.h"

#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Helper {
			inline namespace SyncSection_v1 {
				struct LOCK_STATE {
					union {
						int16_t State16[2];
						int32_t State32;
					};
				};
				class WAIT_EVENT {
					public:
						CLOAK_CALL WAIT_EVENT(In bool initState);
						CLOAK_CALL ~WAIT_EVENT();
						void CLOAK_CALL_THIS Set();
						void CLOAK_CALL_THIS Reset();
						void CLOAK_CALL_THIS Wait();
					private:
						SRWLOCK m_srw;
						CONDITION_VARIABLE m_cv;
						bool m_state;
				};
				CLOAK_INTERFACE_BASIC ILockObject : public virtual API::Helper::SavePtr_v1::IBasicPtr {
					public:
						virtual bool CLOAK_CALL_THIS Unlock(Out LOCK_STATE* state) = 0;
						virtual void CLOAK_CALL_THIS Recover(In const LOCK_STATE& state) = 0;
				};

				class CLOAK_UUID("{062AD427-E8EA-4BB8-B505-7D617E4DF3AB}") SyncSection : public virtual API::Helper::SyncSection_v1::ISyncSection, public virtual ILockObject, public SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL_THIS SyncSection();
						virtual CLOAK_CALL_THIS ~SyncSection();
						virtual API::Global::Debug::Error CLOAK_CALL_THIS Delete() override;

						virtual bool CLOAK_CALL_THIS isLockedExclusive() override;
						virtual bool CLOAK_CALL_THIS isLockedShared() override;
						virtual void CLOAK_CALL_THIS lockExclusive() override;
						virtual void CLOAK_CALL_THIS unlockExclusive() override;
						virtual void CLOAK_CALL_THIS lockShared() override;
						virtual void CLOAK_CALL_THIS unlockShared() override;

						virtual bool CLOAK_CALL_THIS Unlock(Out LOCK_STATE* state) override;
						virtual void CLOAK_CALL_THIS Recover(In const LOCK_STATE& state) override;

						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						SRWLOCK m_srw;
						API::Global::Memory::ThreadLocal::StorageID m_id;
				};
				class CLOAK_UUID("{062E31AA-94F7-42F1-A5B6-82A2D14D5627}") ConditionSyncSection : public virtual API::Helper::SyncSection_v1::IConditionSyncSection, public SyncSection{
					public:
						CLOAK_CALL ConditionSyncSection();
						virtual CLOAK_CALL ~ConditionSyncSection();
						virtual void CLOAK_CALL_THIS lockExclusive(In const std::function<bool()>& condition) override;
						virtual void CLOAK_CALL_THIS lockShared(In const std::function<bool()>& condition) override;
						virtual void CLOAK_CALL_THIS signalOne() override;
						virtual void CLOAK_CALL_THIS signalAll() override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						CONDITION_VARIABLE m_cv;
				};
				class CLOAK_UUID("{2FEEE01B-FF67-4AC7-A32C-53F79313CF6F}") Semaphore : public virtual API::Helper::SyncSection_v1::ISemaphore, public virtual ILockObject, public SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL Semaphore();
						virtual CLOAK_CALL ~Semaphore();
						virtual API::Global::Debug::Error CLOAK_CALL_THIS Delete() override;

						virtual bool CLOAK_CALL_THIS isLockedExclusive() override;
						virtual bool CLOAK_CALL_THIS isLockedShared() override;
						virtual void CLOAK_CALL_THIS lockExclusive() override;
						virtual void CLOAK_CALL_THIS unlockExclusive() override;
						virtual void CLOAK_CALL_THIS lockShared() override;
						virtual void CLOAK_CALL_THIS unlockShared() override;
						virtual void CLOAK_CALL_THIS setSlotCount(In size_t count) override;

						virtual bool CLOAK_CALL_THIS Unlock(Out LOCK_STATE* state) override;
						virtual void CLOAK_CALL_THIS Recover(In const LOCK_STATE& state) override;

						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						uint32_t m_slots;
						uint32_t m_regSlots;
						uint32_t m_usedSlots;
						uint32_t m_readSlots;
						WAIT_EVENT m_fullSlots;
						WAIT_EVENT m_emptySlots;
						WAIT_EVENT m_resize;
						bool m_tryResize;
						SRWLOCK m_srw;
						API::Global::Memory::ThreadLocal::StorageID m_id;
				};
				namespace Lock {
					typedef API::List<std::pair<ILockObject*, LOCK_STATE>> LOCK_DATA;
					void CLOAK_CALL UnlockAll(Out LOCK_DATA* data);
					void CLOAK_CALL RecoverAll(In const LOCK_DATA& data);
					void CLOAK_CALL Initiate();
					void CLOAK_CALL Release();
				}
			}
		}
	}
}

#pragma warning(pop)

#endif
