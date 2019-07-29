#include "stdafx.h"
#include "Engine/WorldStreamer.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Global/Task.h"
#include "CloakEngine/Files/Scene.h"
#include "Implementation/Global/Memory.h"

#include <atomic>

namespace CloakEngine {
	namespace Engine {
		namespace WorldStreamer {
			constexpr uint32_t MAX_FLOOD_COUNT = 1 << 30;
			//Loadrange-Offset is set to sqrt(0.5) to ensure all nodes that are in range are loaded (not only nodes with centers in range)
			constexpr float LOADRANGE_OFFSET = 0.70710678118f;
			//Noderange-Offset is set to 2*sqrt(2) to ensure there is enough space between 2 cams to not interfer with each other
			constexpr float NODERANGE_OFFSET = 2.82842712474f;
			constexpr float NODERANGE_OFFSET_SQ = NODERANGE_OFFSET*NODERANGE_OFFSET;

			//LOD precalculated factors:
			constexpr float LOD_RANGE_LOW = 1;
			constexpr float LOD_RANGE_MID = LOD_RANGE_LOW / 2;
			constexpr float LOD_RANGE_HIGH = LOD_RANGE_MID / 2;
			constexpr float LOD_RANGE_ULTRA = LOD_RANGE_HIGH / 2;
			constexpr float NODE_RESIZE = 1 / Impl::Files::WORLDNODE_SIZE;

			constexpr float LOD_RANGE[] = {
				LOD_RANGE_ULTRA,
				LOD_RANGE_HIGH,
				LOD_RANGE_MID,
				LOD_RANGE_LOW,
			};


			struct UnionElement {
				UnionElement* Parent;
				API::Files::IScene* Scene;
				size_t Rank;
				float ViewRange;
				API::Global::Math::IntVector Position;
				float Range;
				API::Rendering::WorldID ID;
			};
			struct UnionLink {
				Impl::Files::WorldNode* Node;
				UnionElement* Parent;
				API::Global::Math::IntVector Position;
			};
			struct NodeWalk {
				Impl::Files::WorldNode* Node;
				API::Global::Math::IntVector RootPos;
				float MaxDistance;
				CLOAK_CALL NodeWalk(In Impl::Files::WorldNode* node, In float maxDist)
				{
					Node = node;
					RootPos = node->GetPosition();
					MaxDistance = maxDist;
				}
				CLOAK_CALL NodeWalk(In Impl::Files::WorldNode* node, In const NodeWalk& w)
				{
					Node = node;
					RootPos = w.RootPos;
					MaxDistance = w.MaxDistance;
				}
			};

			API::Helper::ISyncSection* g_sync;
			API::Helper::ISyncSection* g_syncLoaded;
			uint32_t g_floodFill = 0;
			std::atomic<bool> g_reqUpdate = false;
			std::atomic<bool> g_loadPorts = false;
			std::atomic<bool> g_canUpdate = false;
			API::List<Impl::Files::WorldNode*>* g_loaded;
			API::List<Impl::Files::WorldNode*>* g_toLoad;
			API::List<Impl::Files::PortalInfo>* g_portalBuffer;
			API::FlatMap<API::Rendering::WorldID, API::Files::IScene*>* g_idToScene;
			Impl::Global::Memory::TypedPool<ActiveStreamer>* g_streamer;
			API::Global::Memory::PageStackAllocator* g_alloc;
			API::PagedStack<NodeWalk>* g_stack[2];
			API::Global::Task g_updateTask;

			CLOAK_CALL NodeList::NodeList(In API::Global::Memory::PageStackAllocator* alloc) : m_alloc(alloc), m_idCount(0), m_pageSize(0), m_headers(nullptr), m_cur(nullptr)
			{
				
			}
			CLOAK_CALL NodeList::~NodeList()
			{
				BlockHeader* c = m_cur;
				while (c != nullptr) { BlockHeader* n = c->Next; m_alloc->Free(c); c = n; }
				if (m_headers != nullptr) { m_alloc->Free(m_headers); }
			}

			NodeList::Header* CLOAK_CALL NodeList::GetHeader(In API::Rendering::WorldID id)
			{
				if (m_headers == nullptr) { m_headers = reinterpret_cast<Header*>(m_alloc->Allocate(MAX_HEADER_PAGE_SIZE)); }
				size_t l = 0;
				size_t m = m_idCount;
				while (m > l)
				{
					const size_t p = (l + m) / 2;
					CLOAK_ASSUME(p < m);
					if (m_headers[p].ID == id) { return &m_headers[p]; }
					else if (m_headers[p].ID < id) { l = p + 1; }
					else { m = p; }
				}
				CLOAK_ASSUME(m_idCount < PAGE_HEADER_COUNT);
				Header t = m_headers[m_idCount++];
				for (size_t a = l; a < m_idCount; a++)
				{
					Header x = m_headers[a];
					m_headers[a] = t;
					t = x;
				}
				m_headers[l].ID = id;
				m_headers[l].Start = nullptr;
				return &m_headers[l];
			}

			void CLOAK_CALL_THIS NodeList::AddNode(In API::Rendering::WorldID id, In Impl::Files::WorldNode* node)
			{
				Header* h = GetHeader(id);
				if (m_cur == nullptr || m_pageSize == PAGE_ELEMENT_COUNT)
				{
					BlockHeader* n = reinterpret_cast<BlockHeader*>(m_alloc->Allocate(PAGE_SIZE));
					n->Next = m_cur;
					m_cur = n;
				}
				Element* e = &reinterpret_cast<Element*>(reinterpret_cast<uintptr_t>(m_cur) + BLOCK_HEADER_SIZE)[m_pageSize++];
				e->Next = h->Start;
				e->Node = node;
				h->Start = e;
			}

			inline int32_t CLOAK_CALL GetNodeDistance(In const API::Global::Math::IntVector& orgP, In const API::Global::Math::IntVector& p)
			{
				API::Global::Math::IntVector v = p - orgP;
				return v.Dot(v);
			}
			inline float CLOAK_CALL GetNodeDistance(In const API::Global::Math::Vector& orgP, In const API::Global::Math::Vector& p)
			{
				API::Global::Math::Vector v = orgP - p;
				return v.Dot(v);
			}
			CLOAK_FORCEINLINE void CLOAK_CALL QueueUpdate()
			{
				if (g_canUpdate == true && g_reqUpdate.exchange(true) == false)
				{ 
					API::Global::Task ut([](In size_t id) { Update(); }); 
					ut.AddDependency(g_updateTask);
					ut.Schedule();
					g_updateTask = ut;
				}
			}

			CLOAK_CALL ActiveStreamer::ActiveStreamer(In Impl::Files::WorldNode* n, In float loadDist, In float viewDist, In float viewMaxDist) : Node(n)
			{
				UpdateDistance(loadDist, viewDist, viewMaxDist);
			}
			void CLOAK_CALL_THIS ActiveStreamer::UpdateDistance(In float load, In float view, In float viewMaxDist)
			{
				const float vD = view*NODE_RESIZE;
				const float lD = (load*NODE_RESIZE) + LOADRANGE_OFFSET;
				const float rD = (viewMaxDist * NODE_RESIZE);
				ViewRange = rD;
				LoadDistance = lD * lD;
				for (size_t a = 0; a < 4; a++)
				{
					const float vDd = (vD*LOD_RANGE[a]) + LOADRANGE_OFFSET;
					ViewDistance[a] = vDd*vDd;
				}
			}

			streamAdr CLOAK_CALL AddStreamer(In Impl::Files::WorldNode* node, In float loadDist, In float viewDist, In float viewMaxDist)
			{
				API::Helper::Lock lock(g_sync);
				streamAdr r = ::new(g_streamer->Allocate())ActiveStreamer(node, loadDist, viewDist, viewMaxDist);
				QueueUpdate();
				return r;
			}
			void CLOAK_CALL UpdateStreamer(In const streamAdr& adr, In Impl::Files::WorldNode* node)
			{
				API::Helper::Lock lock(g_sync);
				if (adr != nullptr)
				{
					adr->Node = node;
					QueueUpdate();
				}
			}
			void CLOAK_CALL UpdateStreamer(In const streamAdr& adr, In float loadDist, In float viewDist, In float viewMaxDist)
			{
				API::Helper::Lock lock(g_sync);
				if (adr != nullptr)
				{
					adr->UpdateDistance(loadDist, viewDist, viewMaxDist);
					QueueUpdate();
				}
			}
			void CLOAK_CALL RemoveStreamer(In const streamAdr& adr)
			{
				API::Helper::Lock lock(g_sync);
				adr->~ActiveStreamer();
				g_streamer->Free(adr);
				QueueUpdate();
			}

			void CLOAK_CALL SetLoadPortals(In bool load)
			{
				g_loadPorts = load;
			}
			bool CLOAK_CALL CheckVisible(In API::Rendering::WorldID worldID, In API::Files::IScene* scene)
			{
				API::Helper::Lock lock(g_sync);
				auto f = g_idToScene->find(worldID);
				if (f != g_idToScene->end()) { return f->second == scene; }
				return false;
			}

			void CLOAK_CALL Initialize()
			{
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync));
				CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncLoaded));
				g_alloc = new API::Global::Memory::PageStackAllocator();
				g_streamer = new Impl::Global::Memory::TypedPool<ActiveStreamer>();
				g_loaded = new API::List<Impl::Files::WorldNode*>();
				g_toLoad = new API::List<Impl::Files::WorldNode*>();
				g_stack[0] = new API::PagedStack<NodeWalk>(g_alloc);
				g_stack[1] = new API::PagedStack<NodeWalk>(g_alloc);
				g_portalBuffer = new API::List<Impl::Files::PortalInfo>();
				g_idToScene = new API::FlatMap<API::Rendering::WorldID, API::Files::IScene*>();
				g_canUpdate = true;
			}
			void CLOAK_CALL Release()
			{
				g_canUpdate = false;
				g_updateTask.WaitForExecution();
				for (size_t a = 0; a < g_loaded->size(); a++)
				{
					Impl::Files::WorldNode* n = g_loaded->at(a);
					n->unload();
					n->Release();
				}
				for (size_t a = 0; a < g_toLoad->size(); a++)
				{
					Impl::Files::WorldNode* n = g_toLoad->at(a);
					n->unload();
					n->Release();
				}
				g_updateTask = nullptr;
				delete g_idToScene;
				delete g_portalBuffer;
				delete g_stack[0];
				delete g_stack[1];
				delete g_loaded;
				delete g_toLoad;
				delete g_streamer;
				delete g_alloc;
				SAVE_RELEASE(g_sync);
				SAVE_RELEASE(g_syncLoaded);
			}
			void CLOAK_CALL Update()
			{
				if (g_reqUpdate.exchange(false) == true)
				{
					API::Helper::Lock lock(g_sync);
					const auto sbeg = g_streamer->begin();
					const auto send = g_streamer->end();
					if (sbeg != send)
					{
						const uint32_t fFs = g_floodFill + 1;
						uint32_t fF = fFs;
						for (auto a = sbeg; a != send; ++a)
						{
							if (a->Node != nullptr)
							{
								API::PagedStack<NodeWalk>* curStack = g_stack[0];
								API::PagedStack<NodeWalk>* nexStack = g_stack[1];

								curStack->emplace(a->Node, a->LoadDistance);
								a->Node->UpdateFloodFillCounter(fF);
								a->Node->RequestLOD(API::Files::LOD::ULTRA);
								do {
									do {
										const NodeWalk w = curStack->top();
										curStack->pop();
										if (w.Node->setStreamFlag(true) == false)
										{
											g_toLoad->push_back(w.Node);
											w.Node->AddRef();
											w.Node->load();
										}
										for (size_t b = 0; b < 6; b++)
										{
											Impl::Files::WorldNode* n = w.Node->GetAdjacentWorldNode(static_cast<API::Files::AdjacentPos>(b));
											if (n != nullptr)
											{
												const API::Global::Math::IntVector& nPos = n->GetPosition();
												const int32_t dis = GetNodeDistance(w.RootPos, nPos);
												if (dis < w.MaxDistance && n->UpdateFloodFillCounter(fF))
												{
													nexStack->emplace(n, w);
													API::Files::LOD flod = API::Files::LOD::LOW;
													for (size_t l = 0; l < 3; l++)
													{
														if (dis <= a->ViewDistance[l])
														{
															flod = static_cast<API::Files::LOD>(l);
															break;
														}
													}
													n->RequestLOD(flod);
												}
											}
										}
										if (g_loadPorts)
										{
											w.Node->GetPortals(g_portalBuffer);
											if (g_portalBuffer->size() > 0)
											{
												const int32_t cd = GetNodeDistance(w.RootPos, w.Node->GetPosition());
												for (size_t b = 0; b < g_portalBuffer->size(); b++)
												{
													const Impl::Files::PortalInfo& p = g_portalBuffer->at(b);
													if (p.Node != nullptr && cd + p.Distance <= w.MaxDistance && p.Node->UpdateFloodFillCounter(fF))
													{
														nexStack->emplace(p.Node, w.MaxDistance - (cd + p.Distance));
														API::Files::LOD flod = API::Files::LOD::LOW;
														for (size_t l = 0; l < 3; l++)
														{
															if (cd + p.Distance <= a->ViewDistance[l])
															{
																flod = static_cast<API::Files::LOD>(l);
																break;
															}
														}
														p.Node->RequestLOD(flod);
													}
												}
												g_portalBuffer->clear();
											}
										}
									} while (curStack->size() > 0);
									API::PagedStack<NodeWalk>* t = curStack;
									curStack = nexStack;
									nexStack = t;
								} while (curStack->size() > 0);
								g_floodFill = (g_floodFill + 1) % MAX_FLOOD_COUNT;
								fF = g_floodFill + 1;
							}
						}
						API::Helper::WriteLock wlock(g_syncLoaded);
						const size_t ols = g_loaded->size();
						size_t ms = ols;
						for (size_t a = 0; a < ms; a++)
						{
							Impl::Files::WorldNode* n = g_loaded->at(a);
							const uint32_t f = n->GetFloodFillCounter();
							if (f == 0 || (fFs < fF && (f >= fF || f < fFs)) || (fFs > fF && f >= fF && f < fFs))
							{
								n->setStreamFlag(false);
								n->unload();
								n->Release();
								g_loaded->at(a) = g_loaded->at(ms - 1);
								ms -= 1;
								a -= 1;
							}
						}
						const size_t tlc = g_toLoad->size();
						g_loaded->resize(ms + tlc, nullptr);
						for (size_t a = 0; a < tlc; a++) { g_loaded->at(a + ms) = g_toLoad->at(a); }
						g_toLoad->clear();
					}
					else
					{
						API::Helper::WriteLock wlock(g_syncLoaded);
						for (size_t a = 0; a < g_loaded->size(); a++)
						{
							Impl::Files::WorldNode* n = g_loaded->at(a);
							n->unload();
							n->Release();
						}
						g_loaded->clear();
					}
					g_alloc->Clear();
				}
			}
			inline UnionElement* CLOAK_CALL FindParent(In UnionElement* x)
			{
				while (x != x->Parent)
				{
					UnionElement* p = x;
					x = x->Parent;
					p->Parent = x->Parent;
				}
				return x;
			}
			void CLOAK_CALL UpdateNodeToWorld(In API::Global::Memory::PageStackAllocator* alloc, Out NodeList* list)
			{
				CLOAK_ASSUME(list != nullptr);
				API::Helper::Lock lock(g_sync);
				const auto sbeg = g_streamer->begin();
				const auto send = g_streamer->end();
				if (sbeg != send)
				{
					g_idToScene->clear();
					//Using Union-Find (Disjoint-Set) algorithm to group streamers:
					API::PagedList<UnionElement> tree(alloc);
					size_t pi = 0;
					//Fill disjoint set with data:
					for (auto a = sbeg; a != send; ++a, ++pi)
					{
						const API::Global::Math::IntVector& pos = a->Node->GetPosition();
						auto pt = tree.emplace_back();
						pt->Parent = &(*pt);
						pt->Scene = a->Node->GetParentScene();
						pt->Rank = 0;
						pt->ViewRange = a->ViewRange;
						pt->Position = pos;
						pt->Range = a->ViewRange;
						pt->ID = static_cast<API::Rendering::WorldID>(pi + 1);
						g_idToScene->insert(pt->ID, pt->Scene);
					}
					lock.unlock();
					lock.lock(g_syncLoaded);
					const size_t numNodes = g_loaded->size();
					API::PagedList<UnionLink> links(alloc);
					links.resize(numNodes);
					CLOAK_ASSUME(links.size() == numNodes);
					auto pl = links.begin();
					//Expand tree elements according to node sizes:
					for (auto a = g_loaded->begin(); a != g_loaded->end(); ++a, ++pl)
					{
						pl->Node = *a;
						pl->Parent = nullptr;
						const float r = pl->Node->GetRadius(alloc)*NODE_RESIZE;
						const API::Global::Math::IntVector& np = pl->Node->GetPosition();
						pl->Position = np;
						API::Files::IScene* scene = pl->Node->GetParentScene();
						for (auto& b : tree)
						{
							if (scene == b.Scene)
							{
								const float mr = r + b.ViewRange;
								const int32_t d = GetNodeDistance(pl->Position, b.Position);
								if (d <= mr * mr)
								{
									const float dr = std::sqrt(static_cast<float>(d));
									pl->Parent = &b;
									if (dr > b.Range) { b.Range = dr; }
								}
							}
						}
					}
					lock.unlock();
					//Unite:
					for (auto a = tree.begin() + 1; a != tree.end(); ++a)
					{
						for (auto b = tree.begin(); b != a; ++b)
						{
							const float mr = a->Range + b->Range;
							if (a->Scene == b->Scene && GetNodeDistance(a->Position, b->Position) <= mr * mr)
							{
								//Find Parents:
								UnionElement* pa = FindParent(&(*a));
								UnionElement* pb = FindParent(&(*b));
								//Unite:
								if (pa != pb)
								{
									if (pa->Rank < pb->Rank)
									{
										UnionElement* t = pa;
										pa = pb;
										pb = t;
									}
									pb->Parent = pa;
									if (pa->Rank == pb->Rank) { pa->Rank++; }
								}
							}
						}
					}
					//Update NTW:
					for (const auto& a : links)
					{
						if (a.Parent != nullptr)
						{
							UnionElement* p = FindParent(a.Parent);
							const API::Global::Math::Vector offset = (a.Position - p->Position)*Impl::Files::WORLDNODE_SIZE;
							a.Node->SetNodeToWorld(API::Global::Math::Matrix::Translate(offset), p->ID);
							list->AddNode(p->ID, a.Node);
						}
						else
						{
							a.Node->SetNodeToWorld(API::Global::Math::Matrix::Identity, 0);
						}
					}
				}
			}
		}
	}
}