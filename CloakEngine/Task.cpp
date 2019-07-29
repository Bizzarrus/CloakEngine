#include "stdafx.h"
#include "CloakEngine/Global/Task.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Memory.h"
#include "Implementation/Global/Memory.h"
#include "Implementation/Global/Game.h"
#include "Implementation/Global/Task.h"
#include "Implementation/OS.h"
#include "Implementation/Helper/SyncSection.h"
#include "Engine/FileLoader.h"

#include <concurrentqueue/concurrentqueue.h> //from vcpkg
#include <atomic>

#define HAS_HELP_FLAG(var, flag) ((var & CloakEngine::API::Global::Threading::Help::flag) == CloakEngine::API::Global::Threading::Help::flag)

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Threading {
				constexpr DWORD MAX_WAIT = 1000;
				constexpr size_t MAX_IO_THREADS = 4;
				constexpr uint32_t PRIORITY_WAIT_RESCHEDULE = 8;
				typedef API::Global::Memory::PageStackAllocator FrameAllocator;

				class Job;
				class GlobalWorker;
				class ThreadWorker;

				CLOAK_INTERFACE_BASIC IWorker{
					public:
						virtual void CLOAK_CALL_THIS Push(In Job * job, In uint32_t priority, In ScheduleHint hints) = 0;
						virtual size_t CLOAK_CALL_THIS GetID() const = 0;
				};

				class Job : public Threading::IJob {
					private:
						static constexpr size_t MAX_DEPENDENCY_COUNT = 16;

						std::atomic<State> m_curState = State::NOT_SCHEDULED;
						std::atomic<uint64_t> m_refCount = 0;
						std::atomic<uint64_t> m_scheduleCount = 0;
						Threading::IJob* m_following[MAX_DEPENDENCY_COUNT];
						size_t m_followingCount = 0;
						SRWLOCK m_srw;
						CONDITION_VARIABLE m_cv;
						std::atomic<ScheduleHint> m_hints = ScheduleHint::None;
						std::atomic<Priority> m_priority = Priority::Lowest;
						std::atomic<bool> m_setPriority = false;
						Threading::Kernel m_kernel;
					public:
						CLOAK_CALL Job(In Threading::Kernel&& kernel);
						CLOAK_CALL Job(In const Threading::Kernel& kernel);
						CLOAK_CALL ~Job();

						uint64_t CLOAK_CALL_THIS AddRef() override;
						uint64_t CLOAK_CALL_THIS Release() override;
						uint64_t CLOAK_CALL_THIS AddJobRef() override;
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease() override;
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In ScheduleHint hints) override;
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In Priority priority) override;
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In ScheduleHint hints, In Priority priority) override;
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In size_t threadID) override;
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In ScheduleHint hints, In size_t threadID) override;
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In Priority priority, In size_t threadID) override;
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In ScheduleHint hints, In Priority priority, In size_t threadID) override;
						IJob* CLOAK_CALL_THIS TryAddFollow(In IJob* job) override;
						void CLOAK_CALL_THIS WaitForExecution(In Threading::Help help, In bool ignoreLocks) override;
						State CLOAK_CALL_THIS GetState() const override;
					private:
						void CLOAK_CALL_THIS UpdateHints(In ScheduleHint hints);
						void CLOAK_CALL_THIS UpdatePriority(In Priority priority);
					public:
						uint64_t CLOAK_CALL_THIS ScheduleAndRelease(In IWorker* caller);
						void CLOAK_CALL_THIS Execute(In IWorker* caller);

						void* CLOAK_CALL operator new(In size_t s);
						void CLOAK_CALL operator delete(In void* p);
				};

				typedef Impl::Global::Memory::TypedPool<Job> JobPool;
				struct WaitingJob {
					API::Global::Time Time;
					Job* Job;
					bool CLOAK_CALL_THIS operator==(In const WaitingJob& o) const { return Time == o.Time && Job == o.Job; }
					bool CLOAK_CALL_THIS operator!=(In const WaitingJob& o) const { return Time != o.Time || Job != o.Job; }
					bool CLOAK_CALL_THIS operator<(In const WaitingJob& o) const { return Time < o.Time; }
					bool CLOAK_CALL_THIS operator<=(In const WaitingJob& o) const { return Time < o.Time || operator==(o); }
					bool CLOAK_CALL_THIS operator>(In const WaitingJob& o) const { return Time > o.Time; }
					bool CLOAK_CALL_THIS operator>=(In const WaitingJob& o) const { return Time > o.Time || operator==(o); }
				};

				struct ConcurrentQueueTraits : public moodycamel::ConcurrentQueueDefaultTraits {
					static inline void* malloc(size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
					static inline void free(void* ptr) { return API::Global::Memory::MemoryHeap::Free(ptr); }
				};
				struct StealableQueue : public moodycamel::ConcurrentQueue<Job*, ConcurrentQueueTraits>
				{
					CLOAK_CALL StealableQueue() {}
					CLOAK_CALL StealableQueue(In size_t capacity) : ConcurrentQueue(capacity) {}
					CLOAK_CALL StealableQueue(In size_t minCapacity, In size_t maxExplicitProducers, In size_t maxImplicitProducers) : ConcurrentQueue(minCapacity, maxExplicitProducers, maxImplicitProducers) {}
				};

				template<uint64_t BitMask, typename = void> struct CombinedLUT {
					static constexpr bool ENABLED = false;
					constexpr CLOAK_CALL CombinedLUT() {}
				};
				//Memory consumption with a bit mask of 0b111111111 (9 bits) is 1MB and 2 KB. Another bit would increase this
				//value up to 4 MB and 4 KB!
				template<uint64_t BitMask> struct CombinedLUT<BitMask, std::enable_if_t<BitMask <= 0b111111111>> {
					public:
						static constexpr bool ENABLED = true;
						static constexpr size_t TABLE_SIZE = min(CE::Global::Math::CompileTime::CeilPower2(BitMask + 1), CE::Global::Math::CompileTime::CeilPower2(BitMask) + 1);

						struct LUT {
							static constexpr size_t MAX_SIZE = 1 << CE::Global::Math::CompileTime::popcount(BitMask);
							uint16_t Size = 0;
							uint16_t List[MAX_SIZE] = { 0 };
						} Table[TABLE_SIZE];
					private:
						constexpr void CLOAK_CALL Generate(In uint16_t mask)
						{
							if (mask < TABLE_SIZE)
							{
								Table[mask].List[0] = 0;
								Table[mask].Size = 1;
								for (uint32_t a = 1; a <= mask; a <<= 1)
								{
									if ((a & mask) != 0)
									{
										const size_t s = Table[mask].Size;
										for (uint16_t b = 0; b < s; b++)
										{
											if (Table[mask].Size < LUT::MAX_SIZE)
											{
												Table[mask].List[Table[mask].Size++] = static_cast<uint16_t>(Table[mask].List[b] | a);
											}
										}
									}
								}
							}
						}
					public:
						constexpr CLOAK_CALL CombinedLUT()
						{
							Generate(0);
							uint16_t s = 1;
							for (uint64_t a = 1; a <= BitMask; a <<= 1)
							{
								if ((a & BitMask) != 0)
								{
									for (uint16_t b = 0; b < s; b++) { Generate(b | static_cast<uint16_t>(a)); }
									s <<= 1;
								}
							}
						}
				};

				template<uint32_t HighestPriority, ScheduleHint AllowedHints> class GlobalQueue {
					private:
						static_assert((static_cast<uint64_t>(AllowedHints) & static_cast<uint64_t>(Help::BasicTasks)) == 0, "Help::BasicTasks must not be a valid ScheduleHint!");
						static constexpr uint32_t HINT_BIT_COUNT = CE::Global::Math::CompileTime::popcount(static_cast<uint64_t>(AllowedHints));
					public:
						static constexpr uint64_t HINT_COMBINATIONS = 1Ui64 << HINT_BIT_COUNT;
						static constexpr uint32_t ALLOWED_HINTS = static_cast<uint32_t>(AllowedHints);
						static constexpr uint32_t MAX_PRIORITY = HighestPriority;
						static constexpr CombinedLUT<static_cast<uint64_t>(AllowedHints)> LookUp = CombinedLUT<static_cast<uint64_t>(AllowedHints)>();
					private:
						static constexpr uint64_t QUEUE_COUNT = (HighestPriority + 1) * HINT_COMBINATIONS;
						uint8_t m_queue[QUEUE_COUNT * sizeof(StealableQueue)];

						StealableQueue* CLOAK_CALL_THIS Queue(In size_t id)
						{
							return reinterpret_cast<StealableQueue*>(reinterpret_cast<uintptr_t>(m_queue) + (id * sizeof(StealableQueue)));
						}
						const StealableQueue* CLOAK_CALL_THIS Queue(In size_t id) const
						{
							return reinterpret_cast<const StealableQueue*>(reinterpret_cast<uintptr_t>(m_queue) + (id * sizeof(StealableQueue)));
						}
						size_t CLOAK_CALL_THIS GetQueueID(In uint32_t priority, In ScheduleHint hints) const
						{
							priority = min(priority, HighestPriority);
							hints &= AllowedHints;
							uint32_t ch = static_cast<uint32_t>(hints);
							if constexpr (HINT_COMBINATIONS != 1 + static_cast<uint64_t>(AllowedHints))
							{
								//shrink hints:
								uint32_t sh = static_cast<uint32_t>(AllowedHints);
								for (uint32_t a = 0, b = 1; sh >> a != 0;)
								{
									if ((sh & b) == 0)
									{
										ch = (ch & (b - 1)) | ((ch & ~(b - 1)) >> 1);
										sh = (sh & (b - 1)) | ((ch & ~(b - 1)) >> 1);
									}
									else
									{
										a++;
										b <<= 1;
									}
								}
							}
							const size_t res = static_cast<size_t>((HINT_COMBINATIONS * priority) + ch);
							CLOAK_ASSUME(res < QUEUE_COUNT);
							return res;
						}
					public:
						class Token {
							friend class GlobalQueue;
							private:
								static constexpr size_t ITEM_SIZE = sizeof(StealableQueue::producer_token_t) + sizeof(StealableQueue::consumer_token_t);
								uint8_t m_data[QUEUE_COUNT * ITEM_SIZE];

								StealableQueue::producer_token_t* CLOAK_CALL_THIS Producer(In size_t id)
								{
									return reinterpret_cast<StealableQueue::producer_token_t*>(reinterpret_cast<uintptr_t>(m_data) + (id * ITEM_SIZE));
								}
								const StealableQueue::producer_token_t* CLOAK_CALL_THIS Producer(In size_t id) const
								{
									return reinterpret_cast<const StealableQueue::producer_token_t*>(reinterpret_cast<uintptr_t>(m_data) + (id * ITEM_SIZE));
								}
								StealableQueue::consumer_token_t* CLOAK_CALL_THIS Consumer(In size_t id)
								{
									return reinterpret_cast<StealableQueue::consumer_token_t*>(reinterpret_cast<uintptr_t>(m_data) + (id * ITEM_SIZE) + sizeof(StealableQueue::producer_token_t));
								}
								const StealableQueue::consumer_token_t* CLOAK_CALL_THIS Consumer(In size_t id) const
								{
									return reinterpret_cast<const StealableQueue::consumer_token_t*>(reinterpret_cast<uintptr_t>(m_data) + (id * ITEM_SIZE) + sizeof(StealableQueue::producer_token_t));
								}
							public:
								CLOAK_CALL Token(In GlobalQueue* queue)
								{
									for (size_t a = 0; a < QUEUE_COUNT; a++)
									{
										::new(Producer(a))StealableQueue::producer_token_t(*queue->Queue(a));
										::new(Consumer(a))StealableQueue::consumer_token_t(*queue->Queue(a));
									}
								}
								CLOAK_CALL ~Token()
								{
									for (size_t a = 0; a < QUEUE_COUNT; a++)
									{
										Producer(a)->~ProducerToken();
										Consumer(a)->~ConsumerToken();
									}
								}
						};

						CLOAK_CALL GlobalQueue(In size_t poolSize)
						{
							for (size_t a = 0; a < QUEUE_COUNT; a++)
							{
								::new(Queue(a))StealableQueue(64, poolSize, 0);
							}
						}
						CLOAK_CALL ~GlobalQueue()
						{
							for (size_t a = 0; a < QUEUE_COUNT; a++)
							{
								Queue(a)->~StealableQueue();
							}
						}

						bool CLOAK_CALL_THIS Enqueue(In uint32_t priority, In ScheduleHint hints, In Job* job)
						{
							const size_t id = GetQueueID(priority, hints);
							return Queue(id)->enqueue(job);
						}
						bool CLOAK_CALL_THIS Enqueue(In uint32_t priority, In ScheduleHint hints, In Job* job, Inout Token& token)
						{
							const size_t id = GetQueueID(priority, hints);
							return Queue(id)->enqueue(*token.Producer(id), job);
						}
						bool CLOAK_CALL_THIS TryDequeue(In uint32_t priority, In ScheduleHint hints, Inout Token& token, Out Job** job)
						{
							*job = nullptr;
							const size_t id = GetQueueID(priority, hints);
							return Queue(id)->try_dequeue(*token.Consumer(id), *job);
						}
				};
				typedef GlobalQueue<static_cast<uint32_t>(Priority::Highest), ScheduleHint::IO> Queue;

				class GlobalWorker : public IWorker {
					private:
						API::Heap<WaitingJob, 4> m_waitingQueue;
						std::atomic<bool> m_hasWaiting = false;
						std::atomic<bool> m_checkingWaiting = false;
						std::atomic<API::Global::Time> m_nextWaitCheck = 0;
						API::Helper::ISyncSection* m_sync = nullptr;
					public:
						CLOAK_CALL GlobalWorker();
						CLOAK_CALL ~GlobalWorker();
						void CLOAK_CALL_THIS Push(In Job* job, In uint32_t priority, In ScheduleHint hints) override;
						size_t CLOAK_CALL_THIS GetID() const override;

						bool CLOAK_CALL_THIS CheckWaiting(In IWorker* caller);
						void CLOAK_CALL_THIS Sleep(In API::Global::Time milliSeconds, In Job* job);
						API::Global::Time CLOAK_CALL_THIS GetNextWaitTime() const;
						void CLOAK_CALL_THIS WakeAllThreads() const;

						void* CLOAK_CALL operator new(In size_t s);
						void CLOAK_CALL operator delete(In void* p);
				};
				class ThreadWorker : public IWorker, public Impl::Global::Task::IHelpWorker, public Impl::Global::Task::IRunWorker {
					protected:
						Queue::Token m_token;
						HANDLE m_thread;
						HANDLE m_evEnded;
						HANDLE m_evWork;
						Threading::Help m_curHelp;
						std::atomic<DWORD> m_sleepTime = 0;
						bool m_loadFiles;
						FrameAllocator m_alloc;
						uint8_t m_priorityCounts[Queue::MAX_PRIORITY];
						const size_t m_id;

						void CLOAK_CALL_THIS Wait(In_opt DWORD maxWait = MAX_WAIT);
					public:
						CLOAK_CALL ThreadWorker(In size_t id, In Threading::Help allowedMaxHelp); //Thread worker for current thread, no new thread is created!
						CLOAK_CALL ThreadWorker(In size_t id, In Threading::Help allowedMaxHelp, In DWORD_PTR affinityMask);
						virtual CLOAK_CALL ~ThreadWorker();
						void CLOAK_CALL_THIS Push(In Job* job, In uint32_t priority, In ScheduleHint hints) override;
						size_t CLOAK_CALL_THIS GetID() const override;

						bool CLOAK_CALL_THIS HelpWork(In_opt Threading::Help help = Threading::Help::Allways) override;
						void CLOAK_CALL_THIS HelpWorkUntil(In std::function<bool()> func, In_opt API::Global::Threading::Help help = API::Global::Threading::Help::Allways, In_opt bool ignoreLocks = false) override;
						void CLOAK_CALL_THIS HelpWorkWhile(In std::function<bool()> func, In_opt API::Global::Threading::Help help = API::Global::Threading::Help::Allways, In_opt bool ignoreLocks = false) override;
						bool CLOAK_CALL_THIS RunWork(In_opt Threading::Help help = API::Global::Threading::Help::Allways) override;
						bool CLOAK_CALL_THIS RunWorkOrWait(In_opt API::Global::Threading::Help help = API::Global::Threading::Help::Allways) override;
						bool CLOAK_CALL_THIS RunWorkOrWait(In API::Global::Threading::Help help, In API::Global::Time waitEndMilliSeconds) override;
						bool CLOAK_CALL_THIS RunWorkOrWait(In API::Global::Time waitEndMilliSeconds) override;

						FrameAllocator* CLOAK_CALL_THIS GetAllocator();

						//Job* CLOAK_CALL_THIS Pop();
						//Job* CLOAK_CALL_THIS PopImportant();
						//Job* CLOAK_CALL_THIS PopLoading();

						void CLOAK_CALL_THIS Run();
						void CLOAK_CALL_THIS Awake();
						void CLOAK_CALL_THIS WaitForFinish();
				};


				API::Helper::ISyncSection* g_syncJobPool = nullptr;
				JobPool* g_jobPool = nullptr;
				size_t g_poolSize = 0;
				size_t g_activePoolSize = 0;
				size_t g_IOThreads = 0;
				Queue* g_queue = nullptr;
				ThreadWorker* g_workers = nullptr;
				GlobalWorker* g_globalWorker = nullptr;
				std::atomic<bool> g_running = true;
				std::atomic<bool> g_initialized = false;
				std::atomic<size_t> g_runningThreads = 0;
				DWORD g_tlWorker = 0;

				DWORD WINAPI runThreadFunc(LPVOID p)
				{
					ThreadWorker* w = reinterpret_cast<ThreadWorker*>(p);
					w->Run();
					return 0;
				}

				typedef ThreadWorker ThreadLocalWorker;
				CLOAK_FORCEINLINE void CLOAK_CALL SetCurrentWorker(In ThreadLocalWorker* worker)
				{
					TlsSetValue(g_tlWorker, worker);
				}
				CLOAK_FORCEINLINE ThreadLocalWorker* CLOAK_CALL GetCurrentWorker()
				{
					if (g_runningThreads >= g_poolSize) { return reinterpret_cast<ThreadLocalWorker*>(TlsGetValue(g_tlWorker)); }
					return nullptr;
				}

				CLOAK_CALL GlobalWorker::GlobalWorker()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
				}
				CLOAK_CALL GlobalWorker::~GlobalWorker()
				{
					SAVE_RELEASE(m_sync);
					while (m_waitingQueue.empty() == false)
					{
						const auto& j = m_waitingQueue.front();
						j.Job->Release();
						m_waitingQueue.pop();
					}
				}
				bool CLOAK_CALL_THIS GlobalWorker::CheckWaiting(In IWorker* caller)
				{
					if (m_hasWaiting == true && m_checkingWaiting.exchange(true) == false)
					{
						API::Helper::Lock lock(m_sync);
						bool res = false;
						API::Global::Time cur = Impl::Global::Game::getCurrentTimeMilliSeconds();
						if (cur >= m_nextWaitCheck)
						{
							while (m_waitingQueue.empty() == false)
							{
								const auto& j = m_waitingQueue.front();
								if (j.Time > cur) { m_nextWaitCheck = j.Time; break; }
								j.Job->ScheduleAndRelease(caller);
								m_waitingQueue.pop();
								res = true;
							}
							m_hasWaiting = !m_waitingQueue.empty();
						}
						m_checkingWaiting = false;
						return res;
					}
					return false;
				}
				void CLOAK_CALL_THIS GlobalWorker::Push(In Job* job, In uint32_t priority, In ScheduleHint hints)
				{
					job->AddRef();
					const bool r = g_queue->Enqueue(priority, hints, job);
					CLOAK_ASSUME(r == true);
					WakeAllThreads();
				}
				size_t CLOAK_CALL_THIS GlobalWorker::GetID() const { return g_poolSize; }

				void CLOAK_CALL_THIS GlobalWorker::Sleep(In API::Global::Time milliSeconds, In Job * job)
				{
					if (milliSeconds > 0)
					{
						API::Helper::Lock lock(m_sync);
						const API::Global::Time ct = Impl::Global::Game::getCurrentTimeMilliSeconds();
						if (ct >= m_nextWaitCheck && m_hasWaiting == true)
						{
							CLOAK_ASSUME(m_waitingQueue.empty() == false);
							do {
								const auto& p = m_waitingQueue.front();
								if (p.Time > ct) { m_nextWaitCheck = p.Time; break; }
								p.Job->ScheduleAndRelease();
								m_waitingQueue.pop();
							} while (m_waitingQueue.empty() == false);
						}
						const API::Global::Time t = ct + milliSeconds;
						if (m_waitingQueue.empty() || m_nextWaitCheck > t)
						{
							m_nextWaitCheck = t;
							CLOAK_ASSUME(g_poolSize >= 2);
							for (size_t a = 0; a < g_poolSize; a++) { g_workers[a].Awake(); }
						}
						else { CLOAK_ASSUME(t >= m_waitingQueue.front().Time); }
						job->AddJobRef();
						m_waitingQueue.push({ t,job });
						m_hasWaiting = true;
					}
				}
				API::Global::Time CLOAK_CALL_THIS GlobalWorker::GetNextWaitTime() const
				{
					if (m_hasWaiting == false) { return MAX_WAIT; }
					const API::Global::Time cur = Impl::Global::Game::getCurrentTimeMilliSeconds();
					const API::Global::Time tar = m_nextWaitCheck.load();
					if (cur >= tar) { return 0; }
					return tar - cur;
				}
				void CLOAK_CALL_THIS GlobalWorker::WakeAllThreads() const
				{
					for (size_t a = 0; a < g_activePoolSize; a++) { g_workers[a].Awake(); }
				}
				void* CLOAK_CALL GlobalWorker::operator new(In size_t s)
				{
					return API::Global::Memory::MemoryHeap::Allocate(s);
				}
				void CLOAK_CALL GlobalWorker::operator delete(In void* p)
				{
					API::Global::Memory::MemoryHeap::Free(p);
				}

				CLOAK_CALL ThreadWorker::ThreadWorker(In size_t id, In Threading::Help allowedMaxHelp, In DWORD_PTR affinityMask) : m_id(id), m_token(g_queue)
				{
					m_curHelp = allowedMaxHelp;
					m_evEnded = CreateEvent(nullptr, TRUE, FALSE, nullptr);
					m_evWork = CreateEvent(nullptr, FALSE, FALSE, nullptr);
					m_thread = CreateThread(nullptr, 0, runThreadFunc, this, 0, nullptr);
					m_loadFiles = false;
					if (affinityMask != 0) { CloakCheckOK(SetThreadAffinityMask(m_thread, affinityMask) != 0, API::Global::Debug::Error::THREAD_AFFINITY, false); }
				}
				CLOAK_CALL ThreadWorker::ThreadWorker(In size_t id, In Threading::Help allowedMaxHelp) : m_id(id), m_token(g_queue)
				{
					for (size_t a = 0; a < ARRAYSIZE(m_priorityCounts); a++) { m_priorityCounts[a] = 0; }
					m_curHelp = allowedMaxHelp;
					m_evWork = CreateEvent(nullptr, FALSE, FALSE, nullptr);
					m_evEnded = INVALID_HANDLE_VALUE;
					m_thread = INVALID_HANDLE_VALUE;
					m_loadFiles = false;

					SetCurrentWorker(this);
					MemoryBarrier();
					g_runningThreads++;
				}
				CLOAK_CALL ThreadWorker::~ThreadWorker()
				{
					Awake();
					WaitForFinish();
					if (m_evEnded != INVALID_HANDLE_VALUE) { CloseHandle(m_evEnded); }
					if (m_thread != INVALID_HANDLE_VALUE) { CloseHandle(m_thread); }
					CloseHandle(m_evWork);
				}
				size_t CLOAK_CALL_THIS ThreadWorker::GetID() const { return m_id; }
				void CLOAK_CALL_THIS ThreadWorker::Push(In Job* job, In uint32_t priority, In ScheduleHint hints)
				{
					job->AddRef();
					const bool r = g_queue->Enqueue(priority, hints, job, m_token);
					CLOAK_ASSUME(r == true);
				}

				bool CLOAK_CALL_THIS ThreadWorker::HelpWork(In_opt Threading::Help help)
				{
					help &= m_curHelp;
					Job* n = nullptr;
					if (help != Help::Never)
					{
						uint64_t h = static_cast<uint64_t>(help & ~Help::BasicTasks) & Queue::ALLOWED_HINTS;

						//Find priority offset
						uint32_t poff = 0;
						for (uint32_t p = Queue::MAX_PRIORITY; true; p--)
						{
							if (p > 0 && m_priorityCounts[p - 1] >= PRIORITY_WAIT_RESCHEDULE) { poff++; }
							else { break; }
						}

						//Find all schedule hints we should try:
						void* hintMemPos = nullptr;
						const uint16_t* hints = nullptr;
						size_t hintCount = 0;
						const size_t startHint = IsFlagSet(help, Help::BasicTasks) ? 0 : 1;
						if constexpr (Queue::LookUp.ENABLED == true)
						{
							//Since computation of hints to try runs in O(2^n), we use compile-time computed look up tables,
							//as long as these tables require less then 2 MB memory in total
							CLOAK_ASSUME(h < ARRAYSIZE(Queue::LookUp.Table));
							hints = Queue::LookUp.Table[h].List;
							hintCount = Queue::LookUp.Table[h].Size;
						}
						else
						{
							hintCount = 1 << CE::Global::Math::popcount(h);
							hintMemPos = m_alloc.Allocate(sizeof(uint16_t) * hintCount);
							uint16_t* rh = reinterpret_cast<uint16_t*>(hintMemPos);
							rh[0] = 0;
							size_t rs = 1;
							for (uint64_t a = 1; a <= h; a <<= 1)
							{
								if ((a & h) != 0)
								{
									const size_t s = rs;
									for (size_t b = 0; b < s; b++) { rh[rs++] = rh[b] | static_cast<uint16_t>(a); }
									rs <<= 1;
								}
							}
							CLOAK_ASSUME(rs == hintCount);
							hints = rh;
						}

						//Find task to execute:
						for (uint32_t p = Queue::MAX_PRIORITY; true; p--)
						{
							uint32_t priority = p;
							if (poff > priority) { priority += Queue::MAX_PRIORITY + 1; }
							priority -= poff;

							for (size_t a = startHint; a < hintCount; a++)
							{
								if (g_queue->TryDequeue(priority, static_cast<ScheduleHint>(hints[a]), m_token, &n) == true)
								{
									CLOAK_ASSUME(n != nullptr);
									if (priority > 0) { m_priorityCounts[priority - 1]++; }
									if constexpr (Queue::LookUp.ENABLED == false) { m_alloc.Free(hintMemPos); }
									goto threadworker_help_found;
								}
							}
							if (priority > 0) { m_priorityCounts[priority - 1] = 0; }
							if (p == 0) { break; }
						}

						if constexpr (Queue::LookUp.ENABLED == false) { m_alloc.Free(hintMemPos); }
					}
					CLOAK_ASSUME(n == nullptr);
					m_loadFiles = Engine::FileLoader::isLoading(API::Files::Priority::NORMAL);
					return false;
				threadworker_help_found:
					const Threading::Help lh = m_curHelp;
					m_curHelp = help;
					n->Execute(this);
					n->Release();
					m_curHelp = lh;
					return true;
				}
				void CLOAK_CALL_THIS ThreadWorker::HelpWorkUntil(In std::function<bool()> func, In_opt API::Global::Threading::Help help, In_opt bool ignoreLocks)
				{
					if (func && func() == false)
					{
						Impl::Helper::Lock::LOCK_DATA lockData;
						if (ignoreLocks == true) { Impl::Helper::Lock::UnlockAll(&lockData); }
						do {
							if (HelpWork(help) == false) { Wait(); }
						} while (func() == false);
						if (ignoreLocks == true) { Impl::Helper::Lock::RecoverAll(lockData); }
					}
				}
				void CLOAK_CALL_THIS ThreadWorker::HelpWorkWhile(In std::function<bool()> func, In_opt API::Global::Threading::Help help, In_opt bool ignoreLocks)
				{
					if (func && func() == true)
					{
						Impl::Helper::Lock::LOCK_DATA lockData;
						if (ignoreLocks == true) { Impl::Helper::Lock::UnlockAll(&lockData); }
						do {
							if (HelpWork(help) == false) { Wait(); }
						} while (func() == true);
						if (ignoreLocks == true) { Impl::Helper::Lock::RecoverAll(lockData); }
					}
				}
				bool CLOAK_CALL_THIS ThreadWorker::RunWork(In_opt Threading::Help help)
				{
					if (m_loadFiles == false && g_running == true) { help &= ~Threading::Help::IO; }
					const bool r = HelpWork(help);
					m_alloc.Clear();
					return r;
				}
				bool CLOAK_CALL_THIS ThreadWorker::RunWorkOrWait(In_opt API::Global::Threading::Help help)
				{
					return RunWorkOrWait(help, MAX_WAIT);
				}
				bool CLOAK_CALL_THIS ThreadWorker::RunWorkOrWait(In API::Global::Threading::Help help, In API::Global::Time waitEndMilliSeconds)
				{
					if (RunWork(help) == true) { if (m_sleepTime.exchange(0) != 0) { SetEvent(m_evWork); } return true; }
					Wait(static_cast<DWORD>(waitEndMilliSeconds - Impl::Global::Game::getCurrentTimeMilliSeconds()));
					return false;
				}
				bool CLOAK_CALL_THIS ThreadWorker::RunWorkOrWait(In API::Global::Time waitEndMilliSeconds)
				{
					return RunWorkOrWait(API::Global::Threading::Help::Allways, waitEndMilliSeconds);
				}

				FrameAllocator* CLOAK_CALL_THIS ThreadWorker::GetAllocator()
				{
					return &m_alloc;
				}

				void CLOAK_CALL_THIS ThreadWorker::Run()
				{
					SetCurrentWorker(this);
					MemoryBarrier();
					g_runningThreads++;
					while ((g_runningThreads < g_poolSize || g_initialized == false) && g_running == true) { Sleep(100); }
					const Threading::Help help = Threading::Help::BasicTasks | (m_id < g_IOThreads ? Threading::Help::IO : Threading::Help::Never);
					while (true)
					{
						bool r = false;
						if (m_loadFiles == false) { r = HelpWork(help); }
						else { r = HelpWork(help | Threading::Help::IO); }
						m_alloc.Clear();
						if (r == true) { if (m_sleepTime.exchange(0) != 0) { SetEvent(m_evWork); } }
						else if (g_running == false) { break; }
						else { Wait(); }
					}
					SetEvent(m_evEnded);
				}
				void CLOAK_CALL_THIS ThreadWorker::Wait(In_opt DWORD maxWait)
				{
					DWORD ms = static_cast<DWORD>(g_globalWorker->GetNextWaitTime());
					ms = min(ms, maxWait);
					if (ms > 0)
					{
						DWORD sleep = m_sleepTime;
						DWORD nsleep;
						do {
							const DWORD s = min(ms, sleep);
							const DWORD n = 1 + (s << 1);
							nsleep = min(n, MAX_WAIT);
						} while (m_sleepTime.compare_exchange_strong(sleep, nsleep) == false);
						if (sleep > 0) { WaitForSingleObject(m_evWork, min(ms, sleep)); }
					}
					else { m_sleepTime = 0; }
				}
				void CLOAK_CALL_THIS ThreadWorker::Awake()
				{
					if (m_sleepTime.exchange(0) != 0) { SetEvent(m_evWork); }
				}
				void CLOAK_CALL_THIS ThreadWorker::WaitForFinish()
				{
					if (m_evEnded != INVALID_HANDLE_VALUE)
					{
						DWORD hR = WaitForSingleObject(m_evEnded, MAX_WAIT << 1);
						while (hR == WAIT_TIMEOUT)
						{
							hR = WaitForSingleObject(m_evEnded, MAX_WAIT);
#ifdef _DEBUG
							if (hR == WAIT_TIMEOUT) { OutputDebugStringA("ThreadWorker Destructor: Thread still working...\n"); }
#endif
						}
					}
				}

				CLOAK_CALL Job::Job(In Threading::Kernel && kernel) : m_kernel(kernel), m_srw(SRWLOCK_INIT), m_cv(CONDITION_VARIABLE_INIT) {}
				CLOAK_CALL Job::Job(In const Threading::Kernel & kernel) : m_kernel(kernel), m_srw(SRWLOCK_INIT), m_cv(CONDITION_VARIABLE_INIT) {}
				CLOAK_CALL Job::~Job()
				{
					for (size_t a = 0; a < m_followingCount; a++)
					{
						CLOAK_ASSUME(m_following[a] != nullptr);
						m_following[a]->ScheduleAndRelease();
					}
				}

				uint64_t CLOAK_CALL_THIS Job::AddRef() { return static_cast<ULONG>(m_refCount.fetch_add(1) + 1); }
				uint64_t CLOAK_CALL_THIS Job::Release()
				{
					const uint64_t r = m_refCount.fetch_sub(1);
					if (r == 1) { delete this; }
					else if (r == 0) { m_refCount = 0; return 0; }
					return r - 1;
				}
				uint64_t CLOAK_CALL_THIS Job::AddJobRef()
				{
					AddRef();
					if (m_curState != State::NOT_SCHEDULED) { return 0; }
					return m_scheduleCount.fetch_add(1) + 1;
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease()
				{
					return ScheduleAndRelease(nullptr);
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease(In ScheduleHint hints)
				{
					UpdateHints(hints);
					return ScheduleAndRelease(nullptr);
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease(In Priority priority)
				{
					UpdatePriority(priority);
					return ScheduleAndRelease(nullptr);
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease(In ScheduleHint hints, In Priority priority)
				{
					UpdateHints(hints);
					UpdatePriority(priority);
					return ScheduleAndRelease(nullptr);
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease(In size_t threadID)
				{
#ifdef _DEBUG
					Threading::ThreadLocalWorker* tlw = Threading::GetCurrentWorker();
					CLOAK_ASSUME(threadID < Threading::g_poolSize && tlw == &g_workers[threadID]);
#endif
					return ScheduleAndRelease(&g_workers[threadID]);
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease(In ScheduleHint hints, In size_t threadID)
				{
#ifdef _DEBUG
					Threading::ThreadLocalWorker* tlw = Threading::GetCurrentWorker();
					CLOAK_ASSUME(threadID < Threading::g_poolSize && tlw == &g_workers[threadID]);
#endif
					UpdateHints(hints);
					return ScheduleAndRelease(&g_workers[threadID]);
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease(In Priority priority, In size_t threadID)
				{
#ifdef _DEBUG
					Threading::ThreadLocalWorker* tlw = Threading::GetCurrentWorker();
					CLOAK_ASSUME(threadID < Threading::g_poolSize && tlw == &g_workers[threadID]);
#endif
					UpdatePriority(priority);
					return ScheduleAndRelease(&g_workers[threadID]);
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease(In ScheduleHint hints, In Priority priority, In size_t threadID)
				{
#ifdef _DEBUG
					Threading::ThreadLocalWorker* tlw = Threading::GetCurrentWorker();
					CLOAK_ASSUME(threadID < Threading::g_poolSize && tlw == &g_workers[threadID]);
#endif
					UpdateHints(hints);
					UpdatePriority(priority);
					return ScheduleAndRelease(&g_workers[threadID]);
				}
				Threading::IJob* CLOAK_CALL_THIS Job::TryAddFollow(In IJob * job)
				{
					CLOAK_ASSUME(job != nullptr);
					if (m_curState == State::FINISHED) { return nullptr; }
					AcquireSRWLockExclusive(&m_srw);
					CLOAK_ASSUME(m_followingCount <= MAX_DEPENDENCY_COUNT);
					if (m_followingCount == MAX_DEPENDENCY_COUNT)
					{
						ReleaseSRWLockExclusive(&m_srw);
						return m_following[MAX_DEPENDENCY_COUNT - 1];
					}
					const size_t id = m_followingCount++;
					if (m_followingCount == MAX_DEPENDENCY_COUNT)
					{
						Job* n = new Job(false);
						n->AddJobRef();
						m_following[MAX_DEPENDENCY_COUNT - 1] = n;
						ReleaseSRWLockExclusive(&m_srw);
						return n;
					}
					m_following[id] = job;
					job->AddJobRef();
					ReleaseSRWLockExclusive(&m_srw);
					return nullptr;
				}
				void CLOAK_CALL_THIS Job::WaitForExecution(In Threading::Help help, In bool ignoreLocks)
				{
					if (m_curState == State::FINISHED) { return; }
					if (help != Threading::Help::Never)
					{
						ThreadLocalWorker* caller = GetCurrentWorker();
						if (caller != nullptr)
						{
							Impl::Helper::Lock::LOCK_DATA lockData;
							if (ignoreLocks == true) { Impl::Helper::Lock::UnlockAll(&lockData); }
							while (m_curState != State::FINISHED) { caller->HelpWork(help); }
							if (ignoreLocks == true) { Impl::Helper::Lock::RecoverAll(lockData); }
							return;
						}
					}
					AcquireSRWLockExclusive(&m_srw);
					do {
						SleepConditionVariableSRW(&m_cv, &m_srw, MAX_WAIT, 0);
					} while (m_curState != State::FINISHED);
					ReleaseSRWLockExclusive(&m_srw);
				}
				Threading::IJob::State CLOAK_CALL_THIS Job::GetState() const { return m_curState; }

				void CLOAK_CALL_THIS Job::UpdateHints(In ScheduleHint hints)
				{
					ScheduleHint expH = m_hints;
					ScheduleHint desH;
					do { desH = expH | hints; } while (!m_hints.compare_exchange_weak(expH, desH));
				}
				void CLOAK_CALL_THIS Job::UpdatePriority(In Priority priority)
				{
					m_setPriority = true;
					Priority p = m_priority;
					while (p < priority && !m_priority.compare_exchange_weak(p, priority)) {}
				}
				uint64_t CLOAK_CALL_THIS Job::ScheduleAndRelease(In IWorker* caller)
				{
					const uint64_t r = m_scheduleCount.fetch_sub(1);
					if (r == 1)
					{
						State exp = State::NOT_SCHEDULED;
						if (m_curState.compare_exchange_strong(exp, State::SCHEDULED))
						{
							if (caller == nullptr)
							{
								caller = g_globalWorker;
								ThreadLocalWorker* tlw = GetCurrentWorker();
								if (tlw != nullptr) { caller = tlw; }
							}
							CLOAK_ASSUME(caller != nullptr);
							Priority p = m_priority.load();
							if (m_setPriority.load() == false) { p = Priority::Normal; }
							caller->Push(this, static_cast<uint32_t>(p), m_hints);
						}
					}
					else if (r == 0) { m_scheduleCount = 0; }
					Release();
					return r > 0 ? r - 1 : 0;
				}
				void CLOAK_CALL_THIS Job::Execute(In IWorker* caller)
				{
					const State o = m_curState.exchange(State::EXECUTING);
					CLOAK_ASSUME(o == State::SCHEDULED);
					if (m_kernel) { m_kernel(caller->GetID()); }
					AcquireSRWLockExclusive(&m_srw);
					m_curState = State::FINISHED;
					WakeAllConditionVariable(&m_cv);
					//If the kernel object was holding any references, this should release them. However, this should only be done after changeing the current state to ensure assertions
					//during reference releases will succeed
					m_kernel = nullptr;
					for (size_t a = 0; a < m_followingCount; a++)
					{
						CLOAK_ASSUME(m_following[a] != nullptr);
						Job * j = dynamic_cast<Job*>(m_following[a]);
						if (j != nullptr) { j->ScheduleAndRelease(caller); }
						else { m_following[a]->ScheduleAndRelease(caller->GetID()); }
					}
					m_followingCount = 0;
					ReleaseSRWLockExclusive(&m_srw);
				}

				void* CLOAK_CALL Job::operator new(In size_t s)
				{
					API::Helper::Lock lock(g_syncJobPool);
					return g_jobPool->Allocate();
				}
				void CLOAK_CALL Job::operator delete(In void* p)
				{
					API::Helper::Lock lock(g_syncJobPool);
					g_jobPool->Free(p);
				}
			}

			CLOAK_CALL Task::Task(In Threading::IJob* job) : m_job(job), m_schedule(true), m_blocking(false) 
			{
				CLOAK_ASSUME(job != nullptr);
				job->AddJobRef();
			}
			CLOAK_CALL Task::Task(In_opt nullptr_t) : m_job(nullptr), m_schedule(false), m_blocking(false) {}
			CLOAK_CALL Task::Task(In const Task& o) : m_job(o.m_job), m_schedule(o.m_schedule), m_blocking(o.m_blocking)
			{
				if (m_job != nullptr)
				{
					if (m_schedule == true) { m_job->AddJobRef(); }
					else { m_job->AddRef(); }
				}
			}
			CLOAK_CALL Task::Task(In Threading::Kernel&& kernel) : Task(new Threading::Job(kernel)) {}
			CLOAK_CALL Task::Task(In const Threading::Kernel& kernel) : Task(new Threading::Job(kernel)) {}
			CLOAK_CALL Task::~Task()
			{
				if (m_job != nullptr)
				{
					if (m_schedule == true) { m_job->ScheduleAndRelease(); }
					else { m_job->Release(); }
				}
			}
			Task& CLOAK_CALL_THIS Task::operator=(In const Task& o)
			{
				if (o.m_job != m_job)
				{
					if (m_job != nullptr)
					{
						if (m_schedule == true) { m_job->ScheduleAndRelease(); }
						else { m_job->Release(); }
					}
					m_job = o.m_job;
					m_schedule = o.m_schedule;
					m_blocking = o.m_blocking;
					if (m_job != nullptr)
					{
						if (m_schedule == true) { m_job->AddJobRef(); }
						else { m_job->AddRef(); }
					}
				}
				return *this;
			}
			Task& CLOAK_CALL_THIS Task::operator=(In Threading::Kernel&& kernel)
			{
				Threading::IJob* n = new Threading::Job(kernel);
				if (m_job != nullptr)
				{
					if (m_schedule == true) { m_job->ScheduleAndRelease(); }
					else { m_job->Release(); }
				}
				m_job = n;
				m_schedule = true;
				m_blocking = false;
				if (m_job != nullptr)
				{
					m_job->AddJobRef();
				}
				return *this;
			}
			Task& CLOAK_CALL_THIS Task::operator=(In const Threading::Kernel& kernel)
			{
				Threading::IJob* n = new Threading::Job(kernel);
				if (m_job != nullptr)
				{
					if (m_schedule == true) { m_job->ScheduleAndRelease(); }
					else { m_job->Release(); }
				}
				m_job = n;
				m_schedule = true;
				m_blocking = false;
				if (m_job != nullptr)
				{
					m_job->AddJobRef();
				}
				return *this;
			}
			Task& CLOAK_CALL_THIS Task::operator=(In nullptr_t)
			{
				if (m_job != nullptr)
				{
					if (m_schedule == true) { m_job->ScheduleAndRelease(); }
					else { m_job->Release(); }
				}
				m_job = nullptr;
				m_schedule = false;
				m_blocking = false;
				return *this;
			}

			bool CLOAK_CALL_THIS Task::operator==(In const Task& o) const { return m_job == o.m_job; }
			bool CLOAK_CALL_THIS Task::operator==(In nullptr_t) const { return m_job == nullptr; }
			bool CLOAK_CALL_THIS Task::operator!=(In const Task& o) const { return m_job != o.m_job; }
			bool CLOAK_CALL_THIS Task::operator!=(In nullptr_t) const { return m_job != nullptr; }

			CLOAK_CALL_THIS Task::operator bool() const { return m_job != nullptr; }

			void CLOAK_CALL_THIS Task::Schedule()
			{
				if (m_job != nullptr && m_schedule == true)
				{
					m_schedule = false;
					m_job->AddRef();
					m_job->ScheduleAndRelease();
				}
			}
			void CLOAK_CALL_THIS Task::Schedule(In Threading::ScheduleHint hint)
			{
				if (m_job != nullptr && m_schedule == true)
				{
					m_schedule = false;
					m_job->AddRef();
					m_job->ScheduleAndRelease(hint);
				}
			}
			void CLOAK_CALL_THIS Task::Schedule(In Threading::Priority priority)
			{
				if (m_job != nullptr && m_schedule == true)
				{
					m_schedule = false;
					m_job->AddRef();
					m_job->ScheduleAndRelease(priority);
				}
			}
			void CLOAK_CALL_THIS Task::Schedule(In Threading::ScheduleHint hint, In Threading::Priority priority)
			{
				if (m_job != nullptr && m_schedule == true)
				{
					m_schedule = false;
					m_job->AddRef();
					m_job->ScheduleAndRelease(hint, priority);
				}
			}
			void CLOAK_CALL_THIS Task::Schedule(In size_t threadID)
			{
				if (m_job != nullptr && m_schedule == true)
				{
					m_schedule = false;
					m_job->AddRef();
					m_job->ScheduleAndRelease(threadID);
				}
			}
			void CLOAK_CALL_THIS Task::Schedule(In Threading::ScheduleHint hint, In size_t threadID)
			{
				if (m_job != nullptr && m_schedule == true)
				{
					m_schedule = false;
					m_job->AddRef();
					m_job->ScheduleAndRelease(hint, threadID);
				}
			}
			void CLOAK_CALL_THIS Task::Schedule(In Threading::Priority priority, In size_t threadID)
			{
				if (m_job != nullptr && m_schedule == true)
				{
					m_schedule = false;
					m_job->AddRef();
					m_job->ScheduleAndRelease(priority, threadID);
				}
			}
			void CLOAK_CALL_THIS Task::Schedule(In Threading::ScheduleHint hint, In Threading::Priority priority, In size_t threadID)
			{
				if (m_job != nullptr && m_schedule == true)
				{
					m_schedule = false;
					m_job->AddRef();
					m_job->ScheduleAndRelease(hint, priority, threadID);
				}
			}
			void CLOAK_CALL_THIS Task::AddDependency(In const Task& other) const
			{
				if (m_job != nullptr && other.m_job != nullptr)
				{
					Threading::IJob* j = other.m_job;
					do { j = j->TryAddFollow(m_job); } while (j != nullptr);
				}
			}
			void CLOAK_CALL_THIS Task::WaitForExecution(In_opt Threading::Help helpExecuting, In_opt bool ignoreLocks) const
			{
				if (m_job != nullptr)
				{
					m_job->WaitForExecution(helpExecuting, ignoreLocks);
				}
			}
			void CLOAK_CALL_THIS Task::WaitForExecution(In bool ignoreLocks) const { WaitForExecution(Threading::Help::Allways, ignoreLocks); }
			bool CLOAK_CALL_THIS Task::Finished() const
			{
				if (m_job != nullptr) { return m_job->GetState() == Threading::Job::State::FINISHED; }
				return true;
			}
			bool CLOAK_CALL_THIS Task::IsScheduled() const
			{
				if (m_job != nullptr) { return m_job->GetState() != Threading::Job::State::NOT_SCHEDULED; }
				return false;
			}

			CLOAKENGINE_API size_t CLOAK_CALL GetExecutionThreadCount() { return Threading::g_activePoolSize; }
			CLOAKENGINE_API Task CLOAK_CALL PushTask(In Threading::Kernel&& kernel) { return Task(kernel); }
			CLOAKENGINE_API Task CLOAK_CALL PushTask(In const Threading::Kernel& kernel) { return Task(kernel); }
		}
	}
	namespace Impl {
		namespace Global {
			namespace Task {
				API::Global::Task CLOAK_CALL Sleep(In API::Global::Time milliSeconds, In API::Global::Threading::Kernel&& kernel)
				{
					API::Global::Threading::Job* j = new API::Global::Threading::Job(kernel);
					j->AddJobRef();
					API::Global::Threading::g_globalWorker->Sleep(milliSeconds, j);
					API::Global::Task r(j);
					j->ScheduleAndRelease();
					return r;
				}
				API::Global::Task CLOAK_CALL Sleep(In API::Global::Time milliSeconds, In const API::Global::Threading::Kernel& kernel)
				{
					API::Global::Threading::Job* j = new API::Global::Threading::Job(kernel);
					j->AddJobRef();
					API::Global::Threading::g_globalWorker->Sleep(milliSeconds, j);
					API::Global::Task r(j);
					j->ScheduleAndRelease();
					return r;
				}

				void* CLOAK_CALL Allocate(In size_t size)
				{
					return API::Global::Threading::GetCurrentWorker()->GetAllocator()->Allocate(size);
				}
				void* CLOAK_CALL TryResizeBlock(In void* ptr, In size_t size)
				{
					return API::Global::Threading::GetCurrentWorker()->GetAllocator()->TryResizeBlock(ptr, size);
				}
				void* CLOAK_CALL Reallocate(In void* ptr, In size_t size, In_opt bool moveData)
				{
					return API::Global::Threading::GetCurrentWorker()->GetAllocator()->Reallocate(ptr, size, moveData);
				}
				void CLOAK_CALL Free(In void* ptr)
				{
					API::Global::Threading::GetCurrentWorker()->GetAllocator()->Free(ptr);
				}
				size_t CLOAK_CALL GetMaxAlloc()
				{
					return API::Global::Threading::FrameAllocator::GetMaxAlloc();
				}

				IHelpWorker* CLOAK_CALL GetCurrentWorker()
				{
					API::Global::Threading::ThreadLocalWorker* w = API::Global::Threading::GetCurrentWorker();
					return w;
				}

				IRunWorker* CLOAK_CALL Initialize(In bool windowThread)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&API::Global::Threading::g_syncJobPool));

					const size_t tc = Impl::OS::GetLogicalProcessorCount();

					const size_t wtc = windowThread == true ? 1 : 0;
					API::Global::Threading::g_activePoolSize = max(tc, 2);
					API::Global::Threading::g_poolSize = API::Global::Threading::g_activePoolSize + wtc;
					API::Global::Threading::g_IOThreads = min(API::Global::Threading::g_poolSize / 2, API::Global::Threading::MAX_IO_THREADS);
					API::Global::Threading::g_tlWorker = TlsAlloc();

					const size_t mainWorkerID = API::Global::Threading::g_poolSize - 1;
					if (windowThread == false) { SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(1Ui64 << mainWorkerID)); }

					CloakDebugLog("Task Core: " + std::to_string(API::Global::Threading::g_poolSize) + " (Active: " + std::to_string(API::Global::Threading::g_activePoolSize) + ") Loading: " + std::to_string(API::Global::Threading::g_IOThreads));

					//Initializing Memory:
					API::Global::Threading::g_jobPool = new API::Global::Threading::JobPool();
					const size_t poolSize = (API::Global::Threading::g_poolSize * sizeof(API::Global::Threading::ThreadWorker)) + sizeof(API::Global::Threading::Queue) + sizeof(API::Global::Threading::GlobalWorker);
					const uintptr_t p = reinterpret_cast<uintptr_t>(API::Global::Memory::MemoryHeap::Allocate(poolSize));
					API::Global::Threading::g_globalWorker = reinterpret_cast<API::Global::Threading::GlobalWorker*>(p);
					API::Global::Threading::g_queue = reinterpret_cast<API::Global::Threading::Queue*>(p + sizeof(API::Global::Threading::GlobalWorker));
					API::Global::Threading::g_workers = reinterpret_cast<API::Global::Threading::ThreadWorker*>(p + (sizeof(API::Global::Threading::GlobalWorker) + sizeof(API::Global::Threading::Queue)));

					std::atomic_thread_fence(std::memory_order_acquire);
					CLOAK_ASSUME(API::Global::Threading::g_tlWorker != 0);

					//Initialize worker queue:
					::new(&API::Global::Threading::g_queue)API::Global::Threading::Queue(API::Global::Threading::g_poolSize);

					//Initialize worker for current (main) thread
					::new(&API::Global::Threading::g_workers[mainWorkerID])API::Global::Threading::ThreadWorker(mainWorkerID, windowThread == true ? API::Global::Threading::Help::Never : API::Global::Threading::Help::Allways);

					//Initialize core worker
					for (size_t a = 0; a < mainWorkerID; a++) { ::new(&API::Global::Threading::g_workers[a])API::Global::Threading::ThreadWorker(a, API::Global::Threading::Help::Allways, static_cast<DWORD_PTR>(1Ui64 << a)); }

					//Initialize global worker
					::new(&API::Global::Threading::g_globalWorker[0])API::Global::Threading::GlobalWorker();

					//Everything is set up now
					API::Global::Threading::g_initialized = true;
					return &API::Global::Threading::g_workers[mainWorkerID];
				}
				void CLOAK_CALL Release()
				{
					API::Global::Threading::g_running = false;
					CLOAK_ASSUME(API::Global::Threading::g_poolSize >= 2);
					//Wait for all threads to shut down, so we are single threaded
					for (size_t a = 0; a < API::Global::Threading::g_poolSize; a++) { API::Global::Threading::g_workers[a].WaitForFinish(); }
					SAVE_RELEASE(API::Global::Threading::g_syncJobPool);
				}
				void CLOAK_CALL Finish()
				{
					//If anything was still pushing tasks, we are now executing them:
					API::Global::Threading::ThreadLocalWorker* tlw = API::Global::Threading::GetCurrentWorker();
					CLOAK_ASSUME(tlw != nullptr);
					while (tlw->RunWork(API::Global::Threading::Help::Allways) == true) {}

					//Free all allocated memory:
					CLOAK_ASSUME(API::Global::Threading::g_poolSize >= 2);
					for (size_t a = 0; a < API::Global::Threading::g_poolSize; a++) { API::Global::Threading::g_workers[a].~ThreadWorker(); }
					API::Global::Threading::g_globalWorker->~GlobalWorker();
					API::Global::Threading::g_queue->~GlobalQueue();
					API::Global::Memory::MemoryHeap::Free(API::Global::Threading::g_globalWorker);
					TlsFree(API::Global::Threading::g_tlWorker);
					delete API::Global::Threading::g_jobPool;
				}
			}
		}
	}
}
