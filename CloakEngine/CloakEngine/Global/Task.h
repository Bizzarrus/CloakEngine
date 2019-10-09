#pragma once
#ifndef CE_API_GLOBAL_TASK_H
#define CE_API_GLOBAL_TASK_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"

#include <functional>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			class Task;
			namespace Threading {
				typedef std::function<void(In size_t threadID)> Kernel;
				enum class Flag {
					None			= 0,
					All				= ~None,						

					// Task may involve IO operations
					IO				= 1 << 0,	
					// Task may use components. This flag should be set on any task that calls AddComponent, GetComponent or RemoveComponent on an entity, or that 
					//	creates a component group. Otherwise, the task may cause race conditions and undefined behavior!
					Components		= 1 << 1,
				};
				DEFINE_ENUM_FLAG_OPERATORS(Flag);
				enum class Priority {
					Low			= 0,
					Normal		= 1,
					High		= 2,

					Lowest		= Low,
					Highest		= High,
				};
				CLOAK_INTERFACE IJob : public virtual API::Helper::SavePtr_v1::IBasicPtr{
					public:
						enum State {NOT_SCHEDULED, SCHEDULED, EXECUTING, FINISHED};
						virtual uint64_t CLOAK_CALL_THIS AddJobRef() = 0;
						virtual uint64_t CLOAK_CALL_THIS ScheduleAndRelease() = 0;
						virtual uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In Flag flags) = 0;
						virtual uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In Priority priority) = 0;
						virtual uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In Flag flags, In Priority priority) = 0;
						virtual uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In size_t threadID) = 0;
						virtual uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In Flag flags, In size_t threadID) = 0;
						virtual uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In Priority priority, In size_t threadID) = 0;
						virtual uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In Flag flags, In Priority priority, In size_t threadID) = 0;
						virtual IJob* CLOAK_CALL_THIS TryAddFollow(In IJob* job) = 0;
						virtual void CLOAK_CALL_THIS WaitForExecution(In Flag allowedFlags, In bool ignoreLocks) = 0;
						virtual State CLOAK_CALL_THIS GetState() const = 0;
						virtual Flag CLOAK_CALL_THIS GetFlags() const = 0;
				};
			}
			//A task represents an asynchonous executed job. Notice that the task class in itself is not thread safe.
			//Copying a not yet scheduled task will result in another not yet scheduled task, so both the original and the
			//copy will have to be scheduled before the job can be executed. Copying an already scheduled task will result in
			//another already scheduled task, so no additional scheduling is required.
			//Not yet scheduled tasks will schedule themself automaticly upon destruction. Same applies if the task gets overwritten
			//with another task, job or nullptr. Therefor, every created task will be scheduled eventually.
			class CLOAKENGINE_API Task {
				private:
					Threading::IJob* m_job;
					bool m_schedule;
					bool m_blocking;
				public:
					CLOAK_CALL Task(In Threading::IJob* job);
					CLOAK_CALL Task(In_opt nullptr_t = nullptr);
					CLOAK_CALL Task(In const Task& o);
					CLOAK_CALL Task(In Threading::Kernel&& kernel);
					CLOAK_CALL Task(In const Threading::Kernel& kernel);
					CLOAK_CALL ~Task();

					Task& CLOAK_CALL_THIS operator=(In const Task& o);
					Task& CLOAK_CALL_THIS operator=(In Threading::Kernel&& kernel);
					Task& CLOAK_CALL_THIS operator=(In const Threading::Kernel& kernel);
					Task& CLOAK_CALL_THIS operator=(In nullptr_t);

					bool CLOAK_CALL_THIS operator==(In const Task& o) const;
					bool CLOAK_CALL_THIS operator==(In nullptr_t) const;
					bool CLOAK_CALL_THIS operator!=(In const Task& o) const;
					bool CLOAK_CALL_THIS operator!=(In nullptr_t) const;

					CLOAK_CALL_THIS operator bool() const;

					//Schedule this task. Every task can only be scheduled once
					void CLOAK_CALL_THIS Schedule();
					void CLOAK_CALL_THIS Schedule(In Threading::Flag flags);
					void CLOAK_CALL_THIS Schedule(In Threading::Priority priority);
					void CLOAK_CALL_THIS Schedule(In Threading::Flag flags, In Threading::Priority priority);
					void CLOAK_CALL_THIS Schedule(In size_t threadID);
					void CLOAK_CALL_THIS Schedule(In Threading::Flag flags, In size_t threadID);
					void CLOAK_CALL_THIS Schedule(In Threading::Priority priority, In size_t threadID);
					void CLOAK_CALL_THIS Schedule(In Threading::Flag flags, In Threading::Priority priority, In size_t threadID);

					//This task will not be executed before 'other' finished executing. There is no guard to prevent dependency
					//loops, so tasks within a dependency loop will never be executed and will end up as leaked (allocated but
					//not usable) memory. Once a dependency is established, it can't be removed anymore.
					void CLOAK_CALL_THIS AddDependency(In const Task& other) const;
					//Blocks execution until this task has finished executing. The thread might help executing
					void CLOAK_CALL_THIS WaitForExecution(In_opt Threading::Flag allowedFlags = Threading::Flag::All, In_opt bool ignoreLocks = false) const;
					void CLOAK_CALL_THIS WaitForExecution(In bool ignoreLocks) const;
					//Returns true if the task has finished executing
					bool CLOAK_CALL_THIS Finished() const;
					bool CLOAK_CALL_THIS Task::IsScheduled() const;
			};
			CLOAKENGINE_API size_t CLOAK_CALL GetExecutionThreadCount();
			CLOAKENGINE_API Task CLOAK_CALL PushTask(In Threading::Kernel&& kernel);
			CLOAKENGINE_API Task CLOAK_CALL PushTask(In const Threading::Kernel& kernel);
		}
	}
}

#endif