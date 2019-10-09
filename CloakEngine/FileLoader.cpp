#include "stdafx.h"
#include "Engine/FileLoader.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Memory.h"
#include "CloakEngine/Global/Task.h"

#include "Implementation/Global/Game.h"
#include "Implementation/Global/Task.h"

#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace FileLoader {
			namespace FileLoader_v2 {
				constexpr size_t PRIORITY_COUNT = 4;
				//Background-loading priority is hidden, as it is controled by the system automaticly
				//	Ressources with the priority BACKGROUND will be delayed-loaded if they are not yet fully loaded and there's nothing other to load
				constexpr API::Files::Priority PRIORITY_BACKGROUND = static_cast<API::Files::Priority>(0);
				constexpr LoadState DEFAULT_STATE = { LoadType::NONE, PRIORITY_BACKGROUND };
				constexpr LoadState PROCESSING_STATE = { LoadType::PROCESSING, PRIORITY_BACKGROUND };
				// If we requested an action but request another action before the first one is executed, this table decides what action
				//	will actuall be executed
				constexpr LoadType QUEUE_TRANSITION[6][6] = {
					//					NONE				LOAD				UNLOAD				RELOAD				DELAYED				PROCESSING
					/*	NONE	*/	{	LoadType::NONE,		LoadType::LOAD,		LoadType::UNLOAD,	LoadType::RELOAD,	LoadType::DELAYED,	LoadType::NONE	},
					/*	LOAD	*/	{	LoadType::NONE,		LoadType::LOAD,		LoadType::NONE,		LoadType::RELOAD,	LoadType::DELAYED,	LoadType::LOAD	},
					/*	UNLOAD	*/	{	LoadType::NONE,		LoadType::RELOAD,	LoadType::UNLOAD,	LoadType::RELOAD,	LoadType::UNLOAD,	LoadType::UNLOAD	},
					/*	RELOAD	*/	{	LoadType::NONE,		LoadType::RELOAD,	LoadType::UNLOAD,	LoadType::RELOAD,	LoadType::RELOAD,	LoadType::RELOAD	},
					/*	DELAYED	*/	{	LoadType::NONE,		LoadType::DELAYED,	LoadType::UNLOAD,	LoadType::RELOAD,	LoadType::DELAYED,	LoadType::DELAYED	},
					/*	PROCESSING*/{	LoadType::NONE,		LoadType::LOAD,		LoadType::UNLOAD,	LoadType::RELOAD,	LoadType::DELAYED,	LoadType::PROCESSING	},
				};

				template<typename A> inline void* CLOAK_CALL Align(In void* ptr, In_opt size_t offset = 0)
				{
					constexpr size_t ALIGNMENT = std::alignment_of<A>::value;
					uintptr_t p = reinterpret_cast<uintptr_t>(ptr) + offset;
					return reinterpret_cast<void*>(p + ((ALIGNMENT - (p % ALIGNMENT)) % ALIGNMENT));
				}

				//Simple wait-free multi-producer-single-consumer queue, based on moodycamel's lock-free single-producer-single-consumer queue:
				//	http://moodycamel.com/blog/2013/a-fast-lock-free-queue-for-c++
				template<typename T, size_t BlockSize = 512> class LoadingQueue {
					public:
						typedef T value_type;
					private:
						static constexpr size_t CACHE_LINE_SIZE = 64;
						static constexpr size_t BLOCK_SIZE = API::Global::Math::CompileTime::CeilPower2(max(BlockSize, 2));

						struct Block {
							public:
								struct {
									std::atomic<size_t> Head = 0;
									size_t LocalTail = 0;
								} Dequeue;
								uint8_t _cachelinePadding0[CACHE_LINE_SIZE - sizeof(Dequeue)];
								struct {
									std::atomic<size_t> Tail = 0;
									size_t LocalHead = 0;
								} Enqueue;
								uint8_t _cachelinePadding1[CACHE_LINE_SIZE - sizeof(Enqueue)];
								std::atomic<Block*> Next;
								void* const Data;
								const size_t SizeMask;
								void* const RawThis;
								CLOAK_CALL Block(In const size_t& size, void* rawPtr, void* data) : SizeMask(size - 1), RawThis(rawPtr), Next(nullptr), Data(data)
								{
									Dequeue.Head = 0;
									Dequeue.LocalTail = 0;
									Enqueue.Tail = 0;
									Enqueue.LocalHead = 0;
								}
								T* CLOAK_CALL_THIS At(In size_t i) { return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(Data) + (i * sizeof(T))); }
								const T* CLOAK_CALL_THIS At(In size_t i) const { return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(Data) + (i * sizeof(T))); }
							private:
								Block& CLOAK_CALL_THIS operator=(In const Block&) { return *this; }
						};
						class Producer {
							private:
								static constexpr size_t BASE_BLOCK_SIZE = sizeof(Block) + std::alignment_of<Block>::value + std::alignment_of<T>::value - 2;

								static Block* CLOAK_CALL CreateBlock(In size_t capacity)
								{
									const size_t size = BASE_BLOCK_SIZE + (sizeof(T) * capacity);
									void* nb = API::Global::Memory::MemoryHeap::Allocate(size);
									if (nb == nullptr) { return nullptr; }

									void* nba = Align<Block>(nb);
									void* nbd = Align<T>(nba, sizeof(Block));
									return ::new(nba)Block(capacity, nb, nbd);
								}

								std::atomic<Block*> m_head;
								uint8_t _cachelinePadding0[CACHE_LINE_SIZE - sizeof(m_head)];
								std::atomic<Block*> m_tail;

								size_t m_largestBlockSize;
							public:
								std::atomic<Producer*> Next;

								explicit CLOAK_CALL Producer(In_opt size_t size = 512) : Next(this)
								{
									CLOAK_ASSUME(size > 0);
									Block* start = nullptr;
									m_largestBlockSize = API::Global::Math::CeilPower2(size + 1);
									if (m_largestBlockSize > BLOCK_SIZE * 2)
									{
										const size_t bc = (size + BLOCK_SIZE * 2 - 3) / (BLOCK_SIZE - 1);
										m_largestBlockSize = BLOCK_SIZE;
										Block* end = nullptr;
										for (size_t a = 0; a < bc; a++)
										{
											Block* b = CreateBlock(m_largestBlockSize);
											if (b == nullptr) { throw std::bad_alloc(); }
											if (start == nullptr) { start = b; }
											else { end->Next = b; }
											end = b;
											end->Next = start;
										}
									}
									else
									{
										start = CreateBlock(m_largestBlockSize);
										if (start == nullptr) { throw std::bad_alloc(); }
										start->Next = start;
									}
									m_head = start;
									m_tail = start;
									std::atomic_thread_fence(std::memory_order_seq_cst);
								}
								CLOAK_CALL ~Producer()
								{
									std::atomic_thread_fence(std::memory_order_seq_cst);
									Block* start = m_head;
									Block* b = start;
									do {
										Block* nb = b->Next;
										size_t head = b->Dequeue.Head;
										size_t tail = b->Enqueue.Tail;

										for (size_t a = head; a != tail; a = (a + 1) & b->SizeMask)
										{
											T* e = b->At(a);
											e->~T();
											(void)e;
										}

										void* rb = b->RawThis;
										b->~Block();
										API::Global::Memory::MemoryHeap::Free(rb);
										b = nb;
									} while (b != start);
								}
								template<typename U> bool CLOAK_CALL_THIS try_dequeue(Inout U& result)
								{
									Block* block = m_head.load();
									size_t tail = block->Dequeue.LocalTail;
									size_t head = block->Dequeue.Head.load();

									if (head != tail || head != (block->Dequeue.LocalTail = block->Enqueue.Tail.load()))
									{ 
										std::atomic_thread_fence(std::memory_order_acquire); 
									non_empty_block:
										T* e = block->At(head);
										result = std::move(*e);
										e->~T();

										head = (head + 1) & block->SizeMask;
										std::atomic_thread_fence(std::memory_order_release);
										block->Dequeue.Head = head;
									}
									else if (block != m_tail.load())
									{
										std::atomic_thread_fence(std::memory_order_acquire);

										block = m_head.load();
										tail = block->Dequeue.LocalTail = block->Enqueue.Tail.load();
										head = block->Dequeue.Head.load();
										std::atomic_thread_fence(std::memory_order_acquire);

										if (head != tail) { goto non_empty_block; }

										Block* nb = block->Next;
										size_t nHead = nb->Dequeue.Head.load();
										size_t nTail = nb->Dequeue.LocalTail = nb->Enqueue.Tail.load();
										std::atomic_thread_fence(std::memory_order_acquire);

										CLOAK_ASSUME(nHead != nTail);
										(void)nTail;

										std::atomic_thread_fence(std::memory_order_release);
										m_head = block = nb;

										std::atomic_signal_fence(std::memory_order_release);

										T* e = block->At(nHead);
										result = std::move(*e);
										e->~T();

										nHead = (nHead + 1) & block->SizeMask;
										std::atomic_thread_fence(std::memory_order_release);
										block->Dequeue.Head = nHead;
									}
									else { return false; }
									return true;
								}
								template<typename... Args> void CLOAK_CALL_THIS enqueue(In Args&&... args)
								{
									Block* block = m_tail.load();
									size_t head = block->Enqueue.LocalHead;
									size_t tail = block->Enqueue.Tail.load();

									size_t nTail = (tail + 1) & block->SizeMask;
									if (nTail != head || nTail != (block->Enqueue.LocalHead = block->Dequeue.Head.load()))
									{
										std::atomic_thread_fence(std::memory_order_acquire);
										T* e = block->At(tail);
										::new(e)T(std::forward<Args>(args)...);
										std::atomic_thread_fence(std::memory_order_release);
										block->Enqueue.Tail = nTail;
									}
									else
									{
										std::atomic_thread_fence(std::memory_order_acquire);
										if (block->Next.load() != m_head.load())
										{
											Block* nb = block->Next.load();
											size_t nHead = nb->Enqueue.LocalHead = nb->Dequeue.Head.load();
											nTail = nb->Enqueue.Tail.load();
											std::atomic_thread_fence(std::memory_order_acquire);

											CLOAK_ASSUME(nHead == nTail);
											nb->Enqueue.LocalHead = nHead;

											T* e = nb->At(nTail);
											::new(e)T(std::forward<Args>(args)...);
											nb->Enqueue.Tail = (nTail + 1) & nb->SizeMask;
											std::atomic_thread_fence(std::memory_order_release);
											m_tail = nb;
										}
										else
										{
											const size_t nbs = m_largestBlockSize >= BLOCK_SIZE ? m_largestBlockSize : (m_largestBlockSize * 2);
											Block* nb = CreateBlock(nbs);
											if (nb == nullptr) { throw std::bad_alloc(); }
											m_largestBlockSize = nbs;
											T* e = nb->At(0);
											::new(e)T(std::forward<Args>(args)...);
											CLOAK_ASSUME(nb->Dequeue.Head == 0);
											nb->Enqueue.Tail = nb->Dequeue.LocalTail = 1;
											nb->Next = block->Next.load();
											block->Next = nb;

											std::atomic_thread_fence(std::memory_order_release);
											m_tail = nb;
										}
									}
								}
						};

						void* m_tls;
						std::atomic<Producer*> m_head;

						Producer* CLOAK_CALL_THIS GetProducer()
						{
							Producer* pr = reinterpret_cast<Producer*>(*API::Global::Memory::ThreadLocal::Get(m_tls));
							if (pr == nullptr)
							{
								void* p = API::Global::Memory::MemoryHeap::Allocate(sizeof(Producer));
								pr = ::new(p)Producer(BlockSize);
								*API::Global::Memory::ThreadLocal::Get(m_tls) = pr;

								Producer* op = m_head.load();
								do {
									pr->Next = op->Next.load();
								} while (!m_head.compare_exchange_weak(op, pr));
								op->Next = pr;
							}
							return pr;
						}
					public:
						class ConsumerToken {
							friend class LoadingQueue;
							private:
								Producer* const m_start;
								explicit CLOAK_CALL ConsumerToken(In Producer* producer) : m_start(producer) {}
							public:
								template<typename U> bool CLOAK_CALL_THIS try_dequeue(Inout U& result) const
								{
									Producer* p = m_start;
									do {
										if (p->try_dequeue(result) == true) { return true; }
										p = p->Next.load();
									} while (p != m_start);
									return false;
								}
						};

						CLOAK_CALL LoadingQueue()
						{
							m_tls = API::Global::Memory::ThreadLocal::Allocate();
							void* p = API::Global::Memory::MemoryHeap::Allocate(sizeof(Producer));
							Producer* pr = ::new(p)Producer(BlockSize);
							pr->Next = pr;
							m_head = pr;
							*API::Global::Memory::ThreadLocal::Get(m_tls) = pr;
						}
						CLOAK_CALL ~LoadingQueue()
						{
							std::atomic_thread_fence(std::memory_order_seq_cst);
							Producer* p = m_head.load();
							Producer* np = p;
							do {
								Producer* n = np->Next.load();
								np->~Producer();
								API::Global::Memory::MemoryHeap::Free(np);
								np = n;
							} while (np != p);
							API::Global::Memory::ThreadLocal::Free(m_tls);
						}

						ConsumerToken CLOAK_CALL_THIS GetConsumerToken() { return ConsumerToken(GetProducer()); }

						template<typename U> bool CLOAK_CALL_THIS try_dequeue(Inout U& result) { return GetConsumerToken().try_dequeue(result); }
						template<typename... Args> void CLOAK_CALL_THIS enqueue(In Args&&... args)
						{
							Producer* p = GetProducer();
							CLOAK_ASSUME(p != nullptr);
							p->enqueue(std::forward<Args>(args)...);
						}
				};

				typedef LoadingQueue<std::pair<ILoad*, uint64_t>, 512> queue_t;

				uint8_t g_queueMemory[(PRIORITY_COUNT * sizeof(queue_t)) + std::alignment_of<queue_t>::value - 1];
				queue_t* g_queues[PRIORITY_COUNT] = { nullptr, nullptr, nullptr, nullptr };
				CE::RefPointer<API::Helper::ISyncSection> g_syncLoaded = nullptr;
				std::atomic<int32_t> g_loadCount[PRIORITY_COUNT] = { 0, 0, 0, 0 };
				std::atomic<size_t> g_loadingSlots = 0;
				std::atomic<size_t> g_loadingState = 0;
				std::atomic<bool> g_hasTask = false;
				std::atomic<bool> g_reqTask = false;
				std::atomic<bool> g_checkLoading = false;
				std::atomic<bool> g_reqReschedule = false;
				Engine::LinkedList<ILoad*>* g_loaded = nullptr;
				API::Global::Task g_task = nullptr;

				void CLOAK_CALL ScheduleJobs(In size_t threadID);
				inline void CLOAK_CALL IncreasePriority(In API::Files::Priority p) 
				{ 
					CloakDebugLog("Increase Load Count[" + std::to_string(static_cast<uint8_t>(p)) + "] (+)");
					if (g_loadCount[static_cast<uint8_t>(p)].fetch_add(1, std::memory_order_acq_rel) <= 0) { g_checkLoading = true; }
				}
				inline void CLOAK_CALL DecreasePriority(In API::Files::Priority p) 
				{ 
					CloakDebugLog("Decrease Load Count[" + std::to_string(static_cast<uint8_t>(p)) + "] (-)");
					if (g_loadCount[static_cast<uint8_t>(p)].fetch_sub(1, std::memory_order_acq_rel) <= 1)
					{ 
						g_checkLoading = true; 
						Impl::Global::Game::wakeThreads(); 
					}
				}
				inline API::Global::Task CLOAK_CALL StartTask()
				{
					if (g_hasTask.exchange(true, std::memory_order_acq_rel) == false)
					{
						API::Global::Task t = [](In size_t threadID) { ScheduleJobs(threadID); };
						t.AddDependency(g_task);
						g_task = t;
						g_task.Schedule();
						return t;
					}
					return nullptr;
				}
				inline void CLOAK_CALL ScheduleJobs(In size_t threadID)
				{
					g_hasTask.store(false, std::memory_order_release);
					if (Impl::Global::Game::GetCurrentComLevel() == 0)
					{
						g_reqTask.store(false, std::memory_order_release);
						size_t slots = g_loadingSlots.exchange(0, std::memory_order_acq_rel);
						for (size_t p = PRIORITY_COUNT; p > 0 && slots > 0; p--)
						{
							const queue_t::ConsumerToken token = g_queues[p - 1]->GetConsumerToken();
							do {
								queue_t::value_type v;
								if (token.try_dequeue(v) == true)
								{
									if (v.first->CheckLoadAction(threadID, v.second) == true) { slots--; }
									v.first->Release();
								}
								else { break; }
							} while (slots > 0);
						}
						g_loadingSlots.fetch_add(slots, std::memory_order_acq_rel);
						if (g_reqTask == true) 
						{ 
							StartTask().Schedule(threadID);
							return; 
						}
						if (slots == 0) { g_reqReschedule.store(true, std::memory_order_release); }
					}
				}

				CLOAK_CALL ILoad::ILoad()
				{
					m_stateID = 0;
					m_canBeQueued = true;
					m_canDequeue = true;
					m_isLoaded = false;
					m_state = DEFAULT_STATE;
					m_ptr = nullptr;
				}
				CLOAK_CALL ILoad::~ILoad()
				{
					auto p = m_ptr.exchange(nullptr, std::memory_order_acq_rel);
					if (p != nullptr)
					{
						API::Helper::Lock lock(g_syncLoaded);
						g_loaded->erase(p);
					}
				}
				bool CLOAK_CALL_THIS ILoad::CanBeDequeued() const { return m_canDequeue.load(std::memory_order_consume); }
				bool CLOAK_CALL_THIS ILoad::CheckLoadAction(In size_t threadID, In uint64_t stateID)
				{
					if (m_canDequeue.load(std::memory_order_consume) == false)
					{
						Impl::Global::Task::IHelpWorker* worker = Impl::Global::Task::GetCurrentWorker(threadID);
						worker->HelpWorkWhile([this]() { return this->CanBeDequeued() == false; }, API::Global::Threading::Flag::All, true);
					}
					uint64_t csid = m_stateID.load(std::memory_order_acquire);
					if (csid == stateID)
					{
						m_canBeQueued.store(false, std::memory_order_release);
						if (m_stateID.compare_exchange_strong(csid, csid + 1, std::memory_order_acq_rel)) 
						{
							m_canDequeue.store(false, std::memory_order_release);
							if (iProcessLoadAction(threadID) == true) { return true; }
							m_canDequeue.store(true, std::memory_order_release);
						}
						m_canBeQueued.store(true, std::memory_order_release);
					}
					return false;
				}
				void CLOAK_CALL_THIS ILoad::ExecuteLoadAction(In size_t threadID, In const LoadState& state, In API::Global::Threading::Priority tp)
				{
					bool keepLoading = false;
					switch (state.Type)
					{
						case LoadType::DELAYED:
						{
							if (m_isLoaded == true)
							{
								CloakDebugLog("Start delayed loading: " + GetDebugName());
								iDelayedLoadFile(&keepLoading);
								CloakDebugLog("Finished loading: " + GetDebugName() + " Keep Loading: " + (keepLoading == true ? "true" : "false"));
								break;
							}
							//else: fall through
						}
						case LoadType::LOAD:
						{
							if (m_isLoaded == false)
							{
								CloakDebugLog("Start loading: " + GetDebugName());
								bool kl = false;
								if (iLoadFile(&kl) == true)
								{
									m_isLoaded = true;
									keepLoading = kl;
									CloakDebugLog("Finished loading: " + GetDebugName() + " Keep Loading: " + (kl == true ? "true" : "false"));
								}
								else
								{
									CloakDebugLog("Failed loading: " + GetDebugName());
								}
							}
							break;
						}
						case LoadType::UNLOAD:
						case LoadType::RELOAD:
						{
							if (m_isLoaded.exchange(false) == true)
							{
								iUnloadFile();
								CloakDebugLog("Unloaded: " + GetDebugName());
							}
							if (state.Type == LoadType::RELOAD)
							{
								API::Global::PushTask([this, state, tp](In size_t threadID) { this->ExecuteLoadAction(threadID, { LoadType::LOAD, state.Priority }, tp); }).Schedule(API::Global::Threading::Flag::IO, tp, threadID);
								return;
							}
							break;
						}
						default:
							break;
					}
					m_canBeQueued.store(true, std::memory_order_release);
					if (iProcessLoadAction(threadID) == false)
					{
						if (m_isLoaded.load() == false)
						{
							auto p = m_ptr.exchange(nullptr, std::memory_order_acq_rel);
							if (p != nullptr)
							{
								API::Helper::Lock lock(g_syncLoaded);
								g_loaded->pop(p);
							}
						}
						else if(m_ptr.load(std::memory_order_acquire) == nullptr)
						{
							API::Helper::Lock lock(g_syncLoaded);
							auto p = g_loaded->push(this);
							lock.unlock();
							m_ptr.store(p, std::memory_order_release);
						}
						if (keepLoading == true) { iRequestDelayedLoad(PRIORITY_BACKGROUND); }
						m_canDequeue.store(true, std::memory_order_release);
						g_loadingSlots.fetch_add(1, std::memory_order_relaxed);
						if (g_reqReschedule.exchange(false, std::memory_order_acq_rel) == true) { StartTask().Schedule(threadID); }
					}
					DecreasePriority(state.Priority);
					Release();
				}
				void CLOAK_CALL_THIS ILoad::CheckSettings(In const API::Global::Graphic::Settings& nset)
				{
					if (iCheckSetting(nset) == true) { iRequestReload(API::Files::Priority::NORMAL); }
				}

				void CLOAK_CALL_THIS ILoad::iRequestLoad(In_opt API::Files::Priority prio)
				{
					CloakDebugLog("Request load: " + GetDebugName());
					iQueueRequest(LoadType::LOAD, prio);
				}
				void CLOAK_CALL_THIS ILoad::iRequestUnload(In_opt API::Files::Priority prio)
				{
					CloakDebugLog("Request unload: " + GetDebugName());
					iQueueRequest(LoadType::UNLOAD, prio);
				}
				void CLOAK_CALL_THIS ILoad::iRequestReload(In_opt API::Files::Priority prio)
				{
					CloakDebugLog("Request reload: " + GetDebugName());
					iQueueRequest(LoadType::RELOAD, prio);
				}
				void CLOAK_CALL_THIS ILoad::iRequestDelayedLoad(In_opt API::Files::Priority prio)
				{
					CloakDebugLog("Request delayed load: " + GetDebugName());
					iQueueRequest(LoadType::DELAYED, prio);
				}
				void CLOAK_CALL_THIS ILoad::iSetPriority(In API::Files::Priority p)
				{
					LoadState cs = m_state.load(std::memory_order_acquire);
					LoadState ns;
					bool upd;
					do {
						upd = false;
						ns = cs;
						if (ns.Type != LoadType::NONE && ns.Priority < p)
						{
							ns.Priority = p;
							upd = true;
						}
					} while (!m_state.compare_exchange_weak(cs, ns, std::memory_order_acq_rel));
					if (upd == true)
					{
						IncreasePriority(ns.Priority);
						iPushInQueue(ns.Priority);
						DecreasePriority(cs.Priority);
					}
				}
				bool CLOAK_CALL_THIS ILoad::iCheckSetting(In const API::Global::Graphic::Settings& nset) const { return false; }

				void CLOAK_CALL_THIS ILoad::iQueueRequest(In LoadType type, In API::Files::Priority priority)
				{
					LoadState cs = m_state.load(std::memory_order_acquire);
					LoadState ns;
					bool upd;
					do {
						upd = false;
						ns = cs;
						CLOAK_ASSUME(static_cast<uint8_t>(type) < ARRAYSIZE(QUEUE_TRANSITION));
						CLOAK_ASSUME(static_cast<uint8_t>(cs.Type) < ARRAYSIZE(QUEUE_TRANSITION));
						ns.Type = QUEUE_TRANSITION[static_cast<uint8_t>(cs.Type)][static_cast<uint8_t>(type)];
						if (ns.Type != cs.Type)
						{
							ns.Priority = priority;
							upd = true;
						}
					} while (!m_state.compare_exchange_weak(cs, ns, std::memory_order_acq_rel));
					if (upd == true)
					{
						if (ns.Type != LoadType::NONE) 
						{ 
							IncreasePriority(priority); 
							iPushInQueue(priority);
						}
						if (cs.Type != LoadType::NONE) { DecreasePriority(cs.Priority); }
					}
				}
				bool CLOAK_CALL_THIS ILoad::iProcessLoadAction(In size_t threadID)
				{
					LoadState cs = m_state.exchange(PROCESSING_STATE, std::memory_order_acq_rel);
					IncreasePriority(PROCESSING_STATE.Priority);
				check_state:
					if (cs.Type != LoadType::NONE)
					{
						API::Global::Threading::Flag tf;
						API::Global::Threading::Priority tp;
						switch (cs.Type)
						{
							case LoadType::PROCESSING:
								DecreasePriority(cs.Priority);
								cs = m_state.exchange(DEFAULT_STATE, std::memory_order_acq_rel);
								if (cs.Type != LoadType::PROCESSING) { goto check_state; }
								DecreasePriority(cs.Priority);
								return false;
							case LoadType::LOAD:
							case LoadType::DELAYED:
								tf = API::Global::Threading::Flag::IO;
								break;
							default:
								tf = API::Global::Threading::Flag::None;
								break;
						}
						switch (cs.Priority)
						{
							case PRIORITY_BACKGROUND:
								tp = API::Global::Threading::Priority::Low;
								break;
							case API::Files::Priority::HIGH:
								tp = API::Global::Threading::Priority::High;
								break;
							default:
								tp = API::Global::Threading::Priority::Normal;
								break;
						}
						m_canBeQueued.store(false, std::memory_order_release);
						AddRef();
						API::Global::PushTask([this, cs, tp](In size_t threadID) { this->ExecuteLoadAction(threadID, cs, tp); }).Schedule(tf, tp, threadID);
						return true;
					}
					return false;
				}
				void CLOAK_CALL_THIS ILoad::iPushInQueue(In API::Files::Priority priority)
				{
					uint64_t csid = m_stateID.load(std::memory_order_acquire);
					uint64_t nsid;
					do {
						nsid = csid;
						if (m_canBeQueued.load(std::memory_order_acquire) == true) { nsid = csid + 1; }
					} while (!m_stateID.compare_exchange_weak(csid, nsid, std::memory_order_acq_rel));
					if (nsid != csid)
					{
						AddRef();
						g_queues[static_cast<uint8_t>(priority)]->enqueue(std::make_pair(this, nsid));
						g_reqTask.store(true, std::memory_order_release);
						StartTask();
					}
				}

				void CLOAK_CALL onStart()
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncLoaded));
					g_loaded = new Engine::LinkedList<ILoad*>();
					g_loadingSlots = API::Global::GetExecutionThreadCount();
					uintptr_t mempos = reinterpret_cast<uintptr_t>(Align<queue_t>(&g_queueMemory[0]));
					for (size_t a = 0; a < PRIORITY_COUNT; a++) 
					{
						g_queues[a] = ::new(reinterpret_cast<void*>(mempos + (sizeof(queue_t) * a)))queue_t();
					}
				}
				void CLOAK_CALL onStartLoading()
				{
					if (g_reqTask == true) { StartTask(); }
				}
				void CLOAK_CALL waitOnLoading()
				{
					Impl::Global::Task::IHelpWorker* hw = Impl::Global::Task::GetCurrentWorker();
					hw->HelpWorkWhile([]() {return isLoading(PRIORITY_BACKGROUND); }, API::Global::Threading::Flag::All, true);
				}
				void CLOAK_CALL onStop()
				{
					g_task = nullptr;
					for (size_t a = 0; a < PRIORITY_COUNT; a++) { g_queues[a]->~LoadingQueue(); }
					delete g_loaded;
					g_syncLoaded = nullptr;
				}
				void CLOAK_CALL updateSetting(In const API::Global::Graphic::Settings& nset)
				{
					API::Helper::Lock lock(g_syncLoaded);
					g_loaded->for_each([&nset](In ILoad* load) {
						load->CheckSettings(nset);
					});
				}
				bool CLOAK_CALL isLoading(In_opt API::Files::Priority prio)
				{
					while (g_checkLoading.exchange(false, std::memory_order_acq_rel) == true)
					{
						for (size_t a = PRIORITY_COUNT; a > 0; a--)
						{
							if (g_loadCount[a - 1].load(std::memory_order_consume) > 0) 
							{ 
								g_loadingState.store(a, std::memory_order_release); 
								goto updated_loading_state;
							}
						}
						g_loadingState.store(0, std::memory_order_release);
					updated_loading_state:;
					}
					return static_cast<uint8_t>(prio) < g_loadingState.load(std::memory_order_consume);
				}
			}
		}
	}
}