#include "stdafx.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Implementation/Global/Connection.h"
#include "Implementation/Global/Game.h"

#include "CloakEngine/Global/Debug.h"

#include "Engine/WindowHandler.h"
#include "Engine/FlipVector.h"
#include "Engine/Compress/NetBuffer/UDPBuffer.h"
#include "Engine/Compress/NetBuffer/TCPBuffer.h"
#include "Engine/Compress/NetBuffer/SecureNet.h"

#include <atomic>
#include <unordered_map>

#define DEFAULT_SERVER "127.0.0.1"

#define CLOSE_CONST(sock) if(sock!=INVALID_SOCKET){closesocket(sock);}
#define CLOSE(sock) if(sock!=INVALID_SOCKET){closesocket(sock); sock=INVALID_SOCKET;}
#define CLOSE_ASYNC(sock) {SOCKET o=sock.exchange(INVALID_SOCKET); if(o!=INVALID_SOCKET){closesocket(o);}}

#define SIMPLE_QUERY(riid,ptr,clas) if (riid == __uuidof(clas)) { *ppvObject = (clas*)this; AddRef(); return S_OK; }


//See https://de.wikipedia.org/wiki/Diffie-Hellman-Schl%C3%BCsselaustausch for key exchange/crypt

#define OLD_CONNECTION_IMPLEMENTATION

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Lobby {
				/*
				TODO: This needs a fully rework using overlapping structures, similar to how file reads/writes work. Also, don't
				use hardcoded IPs or Ports, instead use parameters set by the application/the user (this also is true for server/
				lobby providing sockets!)
				Also handle the case that socket creation failes (which might happen due to network driver/connection issues?) by
				given back an error message to the application instead of shutting the game down. This allows to display error
				messages to the player or enter a wait-and-retry loop
				*/

				struct Message {
					LPARAM lParam;
					SOCKET sock;
				};
				struct AddrStorage {
					bool Lobby;
					SOCKADDR_STORAGE Addr;
				};

				std::atomic<SOCKET> g_listenTCP = INVALID_SOCKET;
				std::atomic<SOCKET> g_listenUDP = INVALID_SOCKET;
				SOCKET g_globalUDP = INVALID_SOCKET;
				API::HashMap<SOCKET, Impl::Global::Connection_v1::Connection*>* g_curConn;
				Engine::LinkedList<std::pair<AddrStorage, Impl::Global::Connection_v1::Connection*>>* g_curUdpCon;
				Engine::LinkedList<Impl::Global::Connection_v1::Connection*>* g_lobbyCon;
				Engine::FlipVector<Message>* g_MessageQueue;
				API::Helper::ISyncSection* g_sync = nullptr;
				API::Helper::ISyncSection* g_syncMQ = nullptr;
				std::atomic<bool> g_init = false;
				size_t g_lobbySize = 0;
				API::Files::UGID g_lobbyGameID;
				std::atomic<BYTE> g_hasLobby = 0;
				API::Global::ILobbyEvent* g_lobbyEvent = nullptr;

				inline void CLOAK_CALL AddConnection(In Impl::Global::Connection_v1::Connection* con, In bool lobby)
				{
					if (con != nullptr)
					{
						const SOCKET s = con->GetSocket();
						API::Helper::Lock lock(g_sync);
						if (g_curConn->find(s) == g_curConn->end())
						{
							con->AddRef();
							(*g_curConn)[s] = con;
						}
						if (lobby) { con->SetLobbyPtr(g_lobbyCon->push(con)); }
					}
				}
				inline bool CLOAK_CALL AddConnectionUDP(In Impl::Global::Connection_v1::Connection* con, In bool lobby, In const SOCKADDR_STORAGE& addr, In size_t addrSize)
				{
					bool res = false;
					if (con != nullptr)
					{
						API::Helper::Lock lock(g_sync);
						for (auto e = g_curUdpCon->begin(); e != nullptr; e++)
						{
							const std::pair<AddrStorage, Impl::Global::Connection_v1::Connection*> c = *e;
							if ((memcmp(&c.first.Addr, &addr, addrSize) == 0 && c.first.Lobby == lobby) || c.second == con) 
							{ 
								goto add_connection_udp_found;
							}
						}
						con->AddRef();
						AddrStorage s;
						s.Addr = addr;
						s.Lobby = lobby;
						g_curUdpCon->push(std::make_pair(s, con));
						res = true;

					add_connection_udp_found:
						if (lobby) { con->SetLobbyPtr(g_lobbyCon->push(con)); }
					}
					return res;
				}
				inline void CLOAK_CALL RemoveConnection(In Impl::Global::Connection_v1::Connection* con, In bool lobby)
				{
					if (con != nullptr)
					{
						const SOCKET s = con->GetSocket();
						API::Helper::Lock lock(g_sync);
						if (s != g_globalUDP && s != g_listenUDP)
						{
							if (lobby)
							{
								auto p = con->GetLobbyPtr();
								if (p != nullptr) { g_lobbyCon->remove(p); }
							}
							const bool f = g_curConn->find(s) != g_curConn->end();
							if (f)
							{
								g_curConn->erase(s);
								shutdown(s, SD_BOTH);
								CLOSE_CONST(s);
								con->Release();
							}
						}
						else
						{
							for (auto e = g_curUdpCon->begin(); e != nullptr; e++)
							{
								if (con == e->second) 
								{ 
									g_curUdpCon->remove(e); 
									con->Release();
									break;
								}
							}
						}
					}
				}
				inline Impl::Global::Connection_v1::Connection* CLOAK_CALL GetConnection(In SOCKET sock)
				{
					API::Helper::Lock lock(g_sync);
					auto f = g_curConn->find(sock);
					if (f != g_curConn->end()) { return f->second; }
					return nullptr;
				}
				inline SOCKET createGlobalSocket(In int type, In size_t size, In bool server)
				{
					SOCKET sock = socket(AF_INET, type, type == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP);
					if (CloakCheckOK(sock != INVALID_SOCKET, API::Global::Debug::Error::CONNECT_SOCKET, true, "Socket Error " + std::to_string(WSAGetLastError())))
					{
						long events = FD_CLOSE;
						if (type == SOCK_STREAM) { events |= FD_ACCEPT; }
						else { events |= FD_READ | FD_WRITE; }
						if (CloakCheckOK(WSAAsyncSelect(sock, Engine::WindowHandler::getWindow(), WM_SOCKET, events) == 0, API::Global::Debug::Error::CONNECT_EVENT_REGISTRATION, false))
						{
							SOCKADDR_IN serv;
							serv.sin_addr.s_addr = inet_addr(DEFAULT_SERVER);
							serv.sin_family = AF_INET;
							serv.sin_port = htons(server ? Impl::Files::DEFAULT_PORT_SERVER : Impl::Files::DEFAULT_PORT_CLIENT);

							int res = bind(sock, (SOCKADDR*)&serv, sizeof(serv));
							if (CloakCheckOK(res == 0, API::Global::Debug::Error::CONNECT_BIND, true))
							{
								if (type == SOCK_DGRAM) { return sock; }
								res = listen(sock, static_cast<int>(size));
								if (CloakCheckOK(res == 0, API::Global::Debug::Error::CONNECT_LISTEN, true))
								{
									return sock;
								}
							}
						}
						CLOSE(sock);
					}
					return INVALID_SOCKET;
				}
				inline Impl::Global::Connection_v1::Connection* CLOAK_CALL AcceptSocket(In SOCKET sock, In SOCKET listenSock)
				{
					if (CloakCheckOK(WSAAsyncSelect(sock, Engine::WindowHandler::getWindow(), WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE) == 0, API::Global::Debug::Error::CONNECT_EVENT_REGISTRATION, true))
					{
						API::Global::Lobby::TPType tp = API::Global::Lobby::TPType::UNKNOWN;
						if (listenSock == g_listenTCP) { tp = API::Global::Lobby::TPType::TCP; }
						SOCKADDR_STORAGE sadd;
						int saddS = sizeof(sadd);
						getpeername(sock, reinterpret_cast<SOCKADDR*>(&sadd), &saddS);
						Impl::Global::Connection_v1::Connection* c = new Impl::Global::Connection_v1::Connection(true, sock, tp, g_lobbyGameID, sadd);
						AddConnection(c, true);
						c->Release();
						return c;
					}
					return nullptr;
				}
				inline void CLOAK_CALL OpenLobby()
				{
					API::Helper::Lock lock(g_sync);
					BYTE n = 2;
					if (g_hasLobby.compare_exchange_weak(n, 3))
					{
						size_t size = g_lobbySize;
						if (size == 0) { size = SOMAXCONN; }
						g_listenTCP = createGlobalSocket(SOCK_STREAM, size, true);
						g_listenUDP = createGlobalSocket(SOCK_DGRAM, size, true);
						g_hasLobby = 4;
					}
				}
				inline void CLOAK_CALL CloseLobby()
				{
					API::Helper::Lock lock(g_sync);
					CLOSE_ASYNC(g_listenTCP);
					CLOSE_ASYNC(g_listenUDP);
					g_listenUDP = INVALID_SOCKET;
					if (g_lobbyCon != nullptr)
					{
						const size_t ls = g_lobbyCon->size();
						if (ls > 0)
						{
							Impl::Global::Connection_v1::Connection** toRem = NewArray(Impl::Global::Connection_v1::Connection*, ls);
							size_t p = 0;
							size_t* lp = &p;
							g_lobbyCon->for_each([=](Impl::Global::Connection_v1::Connection* c) {
								c->SetLobbyPtr(nullptr);
								toRem[(*lp)++] = c;
							});
							g_lobbyCon->clear();
							for (size_t a = 0; a < p; a++) { RemoveConnection(toRem[a], false); }
							DeleteArray(toRem);
						}
					}
				}
				inline size_t storeIPv4(In const API::Global::Lobby::IPAddress& addr, Out SOCKADDR_IN* store)
				{
					store->sin_family = AF_INET;
					store->sin_addr.S_un.S_un_b.s_b1 = addr.IPv4.addr[0];
					store->sin_addr.S_un.S_un_b.s_b2 = addr.IPv4.addr[1];
					store->sin_addr.S_un.S_un_b.s_b3 = addr.IPv4.addr[2];
					store->sin_addr.S_un.S_un_b.s_b4 = addr.IPv4.addr[3];
					store->sin_port = htons(addr.Port);
					return sizeof(SOCKADDR_IN);
				}
				inline size_t storeIPv6(In const API::Global::Lobby::IPAddress& addr, Out SOCKADDR_IN6* store)
				{
					store->sin6_family = AF_INET6;
					for (size_t a = 0; a < 8; a++) { store->sin6_addr.u.Word[a] = addr.IPv6.addr[a]; }
					store->sin6_port = htons(addr.Port);
					return sizeof(SOCKADDR_IN6);
				}
				inline void CLOAK_CALL Connect(In const API::Global::Lobby::IPAddress& addr, In API::Global::Lobby::TPType prot, In const API::Files::UGID& cgid, In_opt API::Global::IConnectionCallback* callback = nullptr)
				{
					SOCKET sock;
					bool suc = false;
					if (prot == API::Global::Lobby::TPType::UDP) 
					{ 
						sock = g_globalUDP;
						suc = true;
					}
					else
					{
						sock = socket(addr.Type == API::Global::Lobby::IPType::IPv4 ? AF_INET : AF_INET6, prot == API::Global::Lobby::TPType::TCP ? SOCK_STREAM : SOCK_DGRAM, prot == API::Global::Lobby::TPType::TCP ? IPPROTO_TCP : IPPROTO_UDP);
						if (CloakCheckOK(sock != INVALID_SOCKET, API::Global::Debug::Error::CONNECT_SOCKET, true))
						{
							if (CloakCheckOK(WSAAsyncSelect(sock, Engine::WindowHandler::getWindow(), WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE | FD_CONNECT) == 0, API::Global::Debug::Error::CONNECT_EVENT_REGISTRATION, true))
							{
								suc = true;
							}
						}
						if (suc == false) { CLOSE(sock); }
					}
					if (suc)
					{
						size_t slen = 0;
						SOCKADDR_STORAGE sadd;
						ZeroMemory(&sadd, sizeof(sadd));
						switch (addr.Type)
						{
							case API::Global::Lobby::IPType::IPv4:
								slen = storeIPv4(addr, reinterpret_cast<SOCKADDR_IN*>(&sadd));
								break;
							case API::Global::Lobby::IPType::IPv6:
								slen = storeIPv6(addr, reinterpret_cast<SOCKADDR_IN6*>(&sadd));
								break;
							default:
								CloakCheckOK(false, API::Global::Debug::Error::CONNECT_INVALID_IPTYPE, false);
								if (sock != g_globalUDP && sock != g_listenUDP) { CLOSE(sock); }
								return;
						}
						Impl::Global::Connection_v1::Connection* r = new Impl::Global::Connection_v1::Connection(false, sock, prot, cgid, sadd, callback);
						bool res = true;
						if (prot == API::Global::Lobby::TPType::UDP) { res = AddConnectionUDP(r, false, sadd, slen); }
						else { AddConnection(r, false); }
						r->Release();
						if (res)
						{
							res = connect(sock, reinterpret_cast<sockaddr*>(&sadd), static_cast<int>(slen)) == 0;
							if (CloakCheckOK(res || WSAGetLastError() == WSAEWOULDBLOCK, API::Global::Debug::Error::CONNECT_FAILED, false)) { return; }
							RemoveConnection(r, false);
						}
					}
				}
				inline void CLOAK_CALL Connect(In const API::Global::Lobby::IPAddress& addr, In API::Global::Lobby::TPType prot, In_opt API::Global::IConnectionCallback* callback = nullptr)
				{
					SOCKET sock;
					bool suc = false;
					if (prot == API::Global::Lobby::TPType::UDP)
					{
						sock = g_globalUDP;
						suc = true;
					}
					else
					{
						sock = socket(addr.Type == API::Global::Lobby::IPType::IPv4 ? AF_INET : AF_INET6, prot == API::Global::Lobby::TPType::TCP ? SOCK_STREAM : SOCK_DGRAM, prot == API::Global::Lobby::TPType::TCP ? IPPROTO_TCP : IPPROTO_UDP);
						if (CloakCheckOK(sock != INVALID_SOCKET, API::Global::Debug::Error::CONNECT_SOCKET, true))
						{
							if (CloakCheckOK(WSAAsyncSelect(sock, Engine::WindowHandler::getWindow(), WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE | FD_CONNECT) == 0, API::Global::Debug::Error::CONNECT_EVENT_REGISTRATION, true))
							{
								suc = true;
							}
						}
						if (suc == false) { CLOSE(sock); }
					}
					if (suc)
					{
						size_t slen = 0;
						SOCKADDR_STORAGE sadd;
						ZeroMemory(&sadd, sizeof(sadd));
						switch (addr.Type)
						{
							case API::Global::Lobby::IPType::IPv4:
								slen = storeIPv4(addr, reinterpret_cast<SOCKADDR_IN*>(&sadd));
								break;
							case API::Global::Lobby::IPType::IPv6:
								slen = storeIPv6(addr, reinterpret_cast<SOCKADDR_IN6*>(&sadd));
								break;
							default:
								CloakCheckOK(false, API::Global::Debug::Error::CONNECT_INVALID_IPTYPE, false);
								if (sock != g_globalUDP && sock != g_listenUDP) { CLOSE(sock); }
								return;
						}
						Impl::Global::Connection_v1::Connection* r = new Impl::Global::Connection_v1::Connection(false, sock, prot, sadd, callback);
						bool res = true;
						if (prot == API::Global::Lobby::TPType::UDP) { res = AddConnectionUDP(r, false, sadd, slen); }
						else { AddConnection(r, false); }
						r->Release();
						if (res)
						{
							res = connect(sock, reinterpret_cast<sockaddr*>(&sadd), static_cast<int>(slen)) == 0;
							if (CloakCheckOK(res || WSAGetLastError() == WSAEWOULDBLOCK, API::Global::Debug::Error::CONNECT_FAILED, false)) { return; }
							RemoveConnection(r, false);
						}
					}
				}
				inline void CLOAK_CALL UpdateConnections()
				{
					API::Helper::Lock lock(g_sync);
					if (g_curConn->size() > 0)
					{
						Impl::Global::Connection_v1::Connection** toUpd = NewArray(Impl::Global::Connection_v1::Connection*, g_curConn->size());
						size_t updP = 0;
						for (const auto& it : (*g_curConn)) { toUpd[updP++] = it.second; }
						for (size_t a = 0; a < updP; a++) { toUpd[a]->OnUpdate(); }
						DeleteArray(toUpd);
					}
				}

				void CLOAK_CALL OnStart(In size_t threadID)
				{
					g_MessageQueue = new Engine::FlipVector<Message>();
					g_curConn = new API::HashMap<SOCKET, Impl::Global::Connection_v1::Connection*>();
					g_curUdpCon = new Engine::LinkedList<std::pair<AddrStorage, Impl::Global::Connection_v1::Connection*>>();
					g_lobbyCon = new Engine::LinkedList<Connection*>();
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_sync));
					CREATE_INTERFACE(CE_QUERY_ARGS(&g_syncMQ));
					WSADATA wsDat;
					int res = WSAStartup(MAKEWORD(2, 2), &wsDat);
					if (CloakCheckOK(res == 0, API::Global::Debug::Error::CONNECT_STARTUP, true))
					{
						g_globalUDP = createGlobalSocket(SOCK_DGRAM, SOMAXCONN, false);
						g_init = true;
						OpenLobby();
					}
					
				}
				void CLOAK_CALL OnUpdate(In size_t threadID, In API::Global::Time etime)
				{
					UpdateConnections();
					API::Helper::Lock lock(g_syncMQ);
					API::List<Message>& v = g_MessageQueue->flip();
					lock.unlock();
					for (size_t a = 0; a < v.size(); a++)
					{
						const Message& m = v[a];
						switch (WSAGETSELECTEVENT(m.lParam))
						{
							case FD_ACCEPT:
							{
								if (g_hasLobby > 0)
								{
									SOCKET n = accept(m.sock, nullptr, nullptr);
									if (CloakCheckOK(n != INVALID_SOCKET, API::Global::Debug::Error::CONNECT_ACCEPT, false)) { AcceptSocket(n, m.sock); }
								}
								break;
							}
							case FD_CONNECT:
							{
								Impl::Global::Connection_v1::Connection* c = GetConnection(m.sock);
								if (c != nullptr) { c->OnConnect(); }
								break;
							}
							case FD_READ:
							{
								Impl::Global::Connection_v1::Connection* c = nullptr;
								if (m.sock == g_globalUDP || m.sock == g_listenUDP)
								{ 
									bool isInLobby = m.sock == g_listenUDP;
									API::Helper::Lock lock(g_sync);
									u_long dts = 0;
									int r = ioctlsocket(m.sock, FIONREAD, &dts);
									if (dts > 0)
									{
										uint8_t* data = NewArray(uint8_t, dts);
										while (dts > 0 && r != SOCKET_ERROR)
										{
											SOCKADDR_STORAGE addr;
											ZeroMemory(&addr, sizeof(addr));
											int addrS = sizeof(addr);
											r = recvfrom(m.sock, reinterpret_cast<char*>(data), dts, 0, reinterpret_cast<SOCKADDR*>(&addr), &addrS);
											if (r != SOCKET_ERROR)
											{
												dts -= r;
												Impl::Global::Connection_v1::Connection* res = nullptr;
												for (auto e = g_curUdpCon->begin(); e != nullptr; e++)
												{
													const std::pair<AddrStorage, Impl::Global::Connection_v1::Connection*>& c = *e;
													if (memcmp(&c.first.Addr, &addr, addrS) == 0 && c.first.Lobby == isInLobby) 
													{ 
														res = c.second; 
														goto update_connection_found;
													}
												}
												res = new Impl::Global::Connection_v1::Connection(isInLobby, m.sock, API::Global::Lobby::TPType::UDP, g_lobbyGameID, addr);
												AddConnectionUDP(res, isInLobby, addr, addrS);
												res->OnWrite();
												res->Release();

											update_connection_found:
												res->OnRead(data, r);
											}
										}
										DeleteArray(data);
									}
								}
								else 
								{ 
									Impl::Global::Connection_v1::Connection* c = GetConnection(m.sock); 
									if (c != nullptr) { c->OnRead(); }
								}
								break;
							}
							case FD_WRITE:
							{
								if (m.sock == g_globalUDP || m.sock == g_listenUDP)
								{
									API::Helper::Lock rlock(g_sync);
									g_curUdpCon->for_each([=](std::pair<AddrStorage, Impl::Global::Connection_v1::Connection*> c) {c.second->OnWrite(); });
								}
								else
								{
									Impl::Global::Connection_v1::Connection* c = GetConnection(m.sock);
									if (c != nullptr) { c->OnWrite(); }
								}
								break;
							}
							case FD_CLOSE:
							{
								if (m.sock == g_listenTCP) { CLOSE_ASYNC(g_listenTCP); }
								else if (m.sock == g_globalUDP || m.sock == g_listenUDP)
								{ 
									CLOSE_ASYNC(g_listenUDP);
									API::Helper::Lock rlock(g_sync);
									g_curUdpCon->for_each([=](std::pair<AddrStorage, Impl::Global::Connection_v1::Connection*> c) 
									{
										c.second->OnClose();
										SAVE_RELEASE(c.second); 
									});
									g_curUdpCon->clear();
								}
								else
								{
									Impl::Global::Connection_v1::Connection* c = GetConnection(m.sock);
									if (c != nullptr) { c->OnClose(); }
								}
								break;
							}
							default:
								break;
						}
					}
					v.clear();
				}
				void CLOAK_CALL OnStop(In size_t threadID)
				{
					CloseLobby();
					CLOSE(g_globalUDP);
					SAVE_RELEASE(g_sync);
					SAVE_RELEASE(g_syncMQ);
					if (g_curConn->size() > 0)
					{
						Impl::Global::Connection_v1::Connection** toDelCon = NewArray(Impl::Global::Connection_v1::Connection*, g_curConn->size());
						size_t a = 0;
						for (const auto& it : (*g_curConn))
						{
							CLOSE_CONST(it.first);
							toDelCon[a++] = it.second;
						}
						g_curConn->clear();
						for (size_t b = 0; b < a; b++)
						{
							SAVE_RELEASE(toDelCon[b]);
						}
						DeleteArray(toDelCon);
					}
					g_curUdpCon->for_each([=](std::pair<AddrStorage, Impl::Global::Connection_v1::Connection*> c) {SAVE_RELEASE(c.second); });
					g_curUdpCon->clear();
					WSACleanup();
					delete g_lobbyCon;
					delete g_curUdpCon;
					delete g_MessageQueue;
					delete g_curConn;
					g_MessageQueue = nullptr;
				}
				void CLOAK_CALL OnMessage(In WPARAM wParam, In LPARAM lParam)
				{
					SOCKET sock = (SOCKET)wParam;
					if (WSAGETSELECTERROR(lParam) != 0)
					{
						if (sock == g_listenTCP) { CLOSE_ASYNC(g_listenTCP); }
						else if (sock == g_listenUDP) { CLOSE_ASYNC(g_listenUDP); }
						else if (sock != g_globalUDP)
						{
							Impl::Global::Connection_v1::Connection* c = GetConnection(sock);
							if (c != nullptr) { c->Close(); }
						}
					}
					else
					{
						Message m;
						m.lParam = lParam;
						m.sock = sock;
						API::Helper::Lock lock(g_syncMQ);
						g_MessageQueue->get()->push_back(m);
					}
				}
				void CLOAK_CALL SetLobbyEventHandler(In API::Global::ILobbyEvent* ev) { g_lobbyEvent = ev; }
			}
			namespace Connection_v1 {

				CLOAK_CALL NetWriteBuffer::NetWriteBuffer(In INetBuffer* par) : m_par(par), m_srcFile(__FILE__)
				{

				}
				CLOAK_CALL NetWriteBuffer::~NetWriteBuffer()
				{

				}

				API::Global::Debug::Error CLOAK_CALL_THIS NetWriteBuffer::Delete()
				{
					return m_par->Delete();
				}
				Use_annotations HRESULT STDMETHODCALLTYPE NetWriteBuffer::QueryInterface(REFIID riid, void** ppvObject)
				{
					SIMPLE_QUERY(riid, ppvObject, API::Helper::SavePtr_v1::ISavePtr)
					else SIMPLE_QUERY(riid, ppvObject, API::Files::Buffer_v1::IBuffer)
					else SIMPLE_QUERY(riid, ppvObject, API::Files::Buffer_v1::IWriteBuffer)
					return E_NOINTERFACE;
				}
				uint64_t CLOAK_CALL_THIS NetWriteBuffer::AddRef()
				{
					return m_par->AddRef();
				}
				uint64_t CLOAK_CALL_THIS NetWriteBuffer::Release()
				{
					return m_par->Release();
				}
				uint64_t CLOAK_CALL_THIS NetWriteBuffer::GetRefCount()
				{
					return m_par->GetRefCount();
				}
				void CLOAK_CALL_THIS NetWriteBuffer::SetDebugName(In const std::string& s)
				{
					m_dbgName = s;
				}
				void CLOAK_CALL_THIS NetWriteBuffer::SetSourceFile(In const std::string& s)
				{
					m_srcFile = s;
				}
				std::string CLOAK_CALL_THIS NetWriteBuffer::GetDebugName() const
				{
					return m_dbgName;
				}
				std::string CLOAK_CALL_THIS NetWriteBuffer::GetSourceFile() const
				{
					return m_srcFile;
				}

				unsigned long long CLOAK_CALL_THIS NetWriteBuffer::GetPosition() const
				{
					return m_par->GetWritePos();
				}
				void CLOAK_CALL_THIS NetWriteBuffer::WriteByte(In uint8_t b)
				{
					m_par->WriteByte(b);
				}
				void CLOAK_CALL_THIS NetWriteBuffer::Save()
				{
					m_par->SendData();
				}


				CLOAK_CALL NetReadBuffer::NetReadBuffer(In INetBuffer* par) : m_par(par), m_srcFile(__FILE__)
				{

				}
				CLOAK_CALL NetReadBuffer::~NetReadBuffer()
				{

				}

				API::Global::Debug::Error CLOAK_CALL_THIS NetReadBuffer::Delete()
				{
					return m_par->Delete();
				}
				Use_annotations HRESULT STDMETHODCALLTYPE NetReadBuffer::QueryInterface(REFIID riid, void** ppvObject)
				{
					SIMPLE_QUERY(riid, ppvObject, API::Helper::SavePtr_v1::ISavePtr)
					else SIMPLE_QUERY(riid, ppvObject, API::Files::Buffer_v1::IBuffer)
					else SIMPLE_QUERY(riid, ppvObject, API::Files::Buffer_v1::IReadBuffer)
					return E_NOINTERFACE;
				}
				uint64_t CLOAK_CALL_THIS NetReadBuffer::AddRef()
				{
					return m_par->AddRef();
				}
				uint64_t CLOAK_CALL_THIS NetReadBuffer::Release()
				{
					return m_par->Release();
				}
				uint64_t CLOAK_CALL_THIS NetReadBuffer::GetRefCount()
				{
					return m_par->GetRefCount();
				}
				void CLOAK_CALL_THIS NetReadBuffer::SetDebugName(In const std::string& s)
				{
					m_dbgName = s;
				}
				void CLOAK_CALL_THIS NetReadBuffer::SetSourceFile(In const std::string& s)
				{
					m_srcFile = s;
				}
				std::string CLOAK_CALL_THIS NetReadBuffer::GetDebugName() const
				{
					return m_dbgName;
				}
				std::string CLOAK_CALL_THIS NetReadBuffer::GetSourceFile() const
				{
					return m_srcFile;
				}

				unsigned long long CLOAK_CALL_THIS NetReadBuffer::GetPosition() const
				{
					return m_par->GetReadPos();
				}
				uint8_t CLOAK_CALL_THIS NetReadBuffer::ReadByte()
				{
					return m_par->ReadByte();
				}
				bool CLOAK_CALL_THIS NetReadBuffer::HasNextByte()
				{
					return m_par->HasData();
				}


				CLOAK_CALL Impl::Global::Connection_v1::Connection::Connection(In bool lobby, In SOCKET sock, In API::Global::Lobby::TPType p, In const API::Files::UGID& cgid, In const SOCKADDR_STORAGE& addr, In_opt API::Global::Connection_v1::IConnectionCallback* callback) : m_lobby(lobby), m_socket(sock), m_tpp(p), m_callback(callback)
				{
					if (m_callback != nullptr) { m_callback->AddRef(); }
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					DEBUG_NAME(Impl::Global::Connection_v1::Connection);
					m_closed = false;
					m_connected = false;
					m_recConnected = false;
					m_data = nullptr;
					if (p == API::Global::Lobby::TPType::UDP) { m_net = new Engine::Compress::NetBuffer::UDPBuffer(sock, addr); }
					else if (p == API::Global::Lobby::TPType::TCP) { m_net = new Engine::Compress::NetBuffer::TCPBuffer(sock); }
					m_net = new Engine::Compress::NetBuffer::SecureNetBuffer(m_net, cgid, m_lobby);
					m_net->SetCallback([=](CallbackType t) {
						if (t == CallbackType::Connected) { OnNetConnected(); }
						else if (t == CallbackType::Disconnected) { Close(); }
						else if (t == CallbackType::ReceivedData) { OnRecData(); }
					});
				}
				CLOAK_CALL Impl::Global::Connection_v1::Connection::Connection(In bool lobby, In SOCKET sock, In API::Global::Lobby::TPType p, In const SOCKADDR_STORAGE& addr, In_opt API::Global::Connection_v1::IConnectionCallback* callback) : m_lobby(lobby), m_socket(sock), m_tpp(p), m_callback(callback)
				{
					if (m_callback != nullptr) { m_callback->AddRef(); }
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					DEBUG_NAME(Impl::Global::Connection_v1::Connection);
					m_closed = false;
					m_connected = false;
					m_recConnected = false;
					m_data = nullptr;
					if (p == API::Global::Lobby::TPType::UDP) { m_net = new Engine::Compress::NetBuffer::UDPBuffer(sock, addr); }
					else if (p == API::Global::Lobby::TPType::TCP) { m_net = new Engine::Compress::NetBuffer::TCPBuffer(sock); }
					m_net->SetCallback([=](CallbackType t) {
						if (t == CallbackType::Connected) { OnNetConnected(); }
						else if (t == CallbackType::Disconnected) { Close(); }
						else if (t == CallbackType::ReceivedData) { OnRecData(); }
					});
				}
				CLOAK_CALL Impl::Global::Connection_v1::Connection::~Connection()
				{
					Close();
					SAVE_RELEASE(m_net);
					SAVE_RELEASE(m_sync);
					if (m_callback != nullptr) { m_callback->Release(); }
					Lobby::RemoveConnection(this, m_lobby);
				}

				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::Close()
				{
					API::Helper::Lock lock(m_sync);
					if (m_closed == false)
					{
						m_closed = true;
						if (m_socket != Lobby::g_globalUDP && m_socket != Lobby::g_listenUDP) { shutdown(m_socket, SD_SEND); }
						if (m_callback != nullptr) { m_callback->OnDisconnect(this); }
						else if (Lobby::g_lobbyEvent != nullptr)
						{
							if (m_recConnected) { Lobby::g_lobbyEvent->OnDisconnect(this); }
							else if (m_lobby == false) { Lobby::g_lobbyEvent->OnConnectionFailed(this); }
						}
					}
				}
				bool CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::IsConnected() const
				{
					API::Helper::Lock lock(m_sync);
					return m_recConnected && !m_closed;
				}
				API::Global::Lobby::TPType CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::GetProtocoll() const { return m_tpp; }
				API::Global::Lobby::ConnectionType CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::GetType() const { return m_lobby ? API::Global::Lobby::ConnectionType::Server : API::Global::Lobby::ConnectionType::Client; }
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::SetData(In LPVOID data)
				{
					API::Helper::Lock lock(m_sync);
					m_data = data;
				}
				LPVOID CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::GetData() const
				{
					API::Helper::Lock lock(m_sync);
					return m_data;
				}
				API::Files::IReadBuffer* CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::GetReadBuffer() const
				{
					API::Helper::Lock lock(m_sync);
					return m_net->GetReadBuffer();
				}
				API::Files::IWriteBuffer* CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::GetWriteBuffer() const
				{
					API::Helper::Lock lock(m_sync);
					return m_net->GetWriteBuffer();
				}

				SOCKET CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::GetSocket() { return m_socket; }
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::OnClose() 
				{ 
					Close(); 
				}
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::OnRead(In_opt uint8_t* data, In_opt size_t dataSize)
				{
					if (m_connected.exchange(true) == false) { OnConnect(); }
					m_net->OnRead(data, dataSize);
				}
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::OnWrite()
				{
					if (m_connected.exchange(true) == false) { OnConnect(); }
					m_net->OnWrite();
				}
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::OnConnect()
				{
					API::Helper::Lock lock(m_sync);
					m_connected = true;
					m_net->OnConnect();
				}
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::OnUpdate()
				{
					m_net->OnUpdate();
					if (m_connected == false) { m_connected = m_net->IsConnected(); }
				}
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::OnNetConnected()
				{
					if (m_recConnected.exchange(true) == false) 
					{
						if (m_callback != nullptr) { m_callback->OnConnect(this); }
						else if (Lobby::g_lobbyEvent != nullptr) { Lobby::g_lobbyEvent->OnConnect(this); }
					}
				}
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::OnRecData()
				{
					if (m_callback != nullptr) { m_callback->OnReceiveData(this); }
					else if (Lobby::g_lobbyEvent != nullptr) { Lobby::g_lobbyEvent->OnReceivedData(this); }
				}
				void CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::SetLobbyPtr(In_opt Engine::LinkedList<Connection*>::const_iterator ptr)
				{
					API::Helper::Lock lock(m_sync);
					m_lobbyPos = ptr;
				}
				Engine::LinkedList<Connection*>::const_iterator CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::GetLobbyPtr() const
				{
					API::Helper::Lock lock(m_sync);
					return m_lobbyPos;
				}

				Success(return == true) bool CLOAK_CALL_THIS Impl::Global::Connection_v1::Connection::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					CLOAK_QUERY(riid, ptr, SavePtr, Global::Connection_v1, Connection);
				}
			}
		}
	}
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Lobby {
				CLOAKENGINE_API CLOAK_CALL IPAddress::IPAddress() : Type(IPType::IPv6)
				{
					ZeroMemory(&IPv6, sizeof(IPv6));
					Port = 0;
				}
				CLOAKENGINE_API CLOAK_CALL IPAddress::IPAddress(In const std::string& addrStr) : IPAddress(addrStr, Impl::Files::DEFAULT_PORT_SERVER)
				{

				}
				CLOAKENGINE_API CLOAK_CALL IPAddress::IPAddress(In const std::string& addrStr, In uint16_t port)
				{
					ADDRINFOA ai;
					ai.ai_flags = 0;
					ai.ai_family = AF_UNSPEC;
					ai.ai_socktype = 0;
					ai.ai_protocol = 0;
					ai.ai_addrlen = 0;
					ai.ai_canonname = nullptr;
					ai.ai_addr = nullptr;
					ai.ai_next = nullptr;
					PADDRINFOA info = nullptr;
					std::string p = std::to_string(port);
					Port = port;
					int err = 0;
					do {
						err = GetAddrInfoA(addrStr.c_str(), p.c_str(), &ai, &info);
					} while (err == EAI_AGAIN);
#ifdef _DEBUG
					if (err == EAI_NONAME) { CloakDebugLog("Host not found!"); }
#endif
					if (CloakCheckOK(err == 0 && info != nullptr, API::Global::Debug::Error::CONNECT_ADDRESS_RESOLVE, false))
					{
						//bool f = false;
						for (PADDRINFOA addr = info; addr != nullptr; addr = addr->ai_next)
						{
							switch (addr->ai_family)
							{
								case AF_INET:
								{
									Type = IPType::IPv4;
									sockaddr_in* ad = reinterpret_cast<sockaddr_in*>(addr->ai_addr);
									IPv4.addr[0] = ad->sin_addr.S_un.S_un_b.s_b1;
									IPv4.addr[1] = ad->sin_addr.S_un.S_un_b.s_b2;
									IPv4.addr[2] = ad->sin_addr.S_un.S_un_b.s_b3;
									IPv4.addr[3] = ad->sin_addr.S_un.S_un_b.s_b4;
									break;
								}
								case AF_INET6:
								{
									Type = IPType::IPv6;
									sockaddr_in6* ad = reinterpret_cast<sockaddr_in6*>(addr->ai_addr);
									for (size_t a = 0; a < 8; a++) { IPv6.addr[a] = ad->sin6_addr.u.Word[a]; }
									break;
								}
								default:
									continue;
							}
							break;
						}
					}
					freeaddrinfo(info);
				}
				CLOAKENGINE_API bool CLOAK_CALL_THIS IPAddress::operator==(In const IPAddress& o) const
				{
					if (Type == o.Type)
					{
						switch (Type)
						{
							case CloakEngine::API::Global::Lobby::IPType::IPv4:
								for (size_t a = 0; a < ARRAYSIZE(IPv4.addr); a++) 
								{ 
									if (IPv4.addr[a] != o.IPv4.addr[a]) { return false; }
								}
								return true;
							case CloakEngine::API::Global::Lobby::IPType::IPv6:
								for (size_t a = 0; a < ARRAYSIZE(IPv6.addr); a++)
								{
									if (IPv6.addr[a] != o.IPv6.addr[a]) { return false; }
								}
								return true;
							default:
								return true;
						}
					}
					return false;
				}

				CLOAKENGINE_API void CLOAK_CALL Connect(In const API::Global::Lobby::IPAddress& ad, In TPType prot, In const API::Files::UGID& cgid, In_opt IConnectionCallback* callback)
				{
					Impl::Global::Lobby::Connect(ad, prot, cgid, callback);
				}
				CLOAKENGINE_API void CLOAK_CALL Connect(In const API::Global::Lobby::IPAddress& ad, In TPType prot, In_opt IConnectionCallback* callback)
				{
					Impl::Global::Lobby::Connect(ad, prot, callback);
				}
				CLOAKENGINE_API void CLOAK_CALL Open(In const API::Files::UGID& cgid, In_opt size_t maxSize)
				{
					BYTE n = 0;
					if (CloakCheckOK(Impl::Global::Lobby::g_hasLobby.compare_exchange_weak(n, 1), Debug::Error::CONNECT_LOBBY_IS_OPEN, false))
					{
						Impl::Global::Lobby::g_lobbySize = maxSize;
						Impl::Global::Lobby::g_lobbyGameID = cgid;
						Impl::Global::Lobby::g_hasLobby = 2;
						if (Impl::Global::Lobby::g_init) { Impl::Global::Lobby::OpenLobby(); }
					}
				}
				CLOAKENGINE_API void CLOAK_CALL Close()
				{
					if (Impl::Global::Lobby::g_hasLobby.exchange(0) > 0) { Impl::Global::Lobby::CloseLobby(); }
				}
				CLOAKENGINE_API bool CLOAK_CALL IsOpen()
				{
					return Impl::Global::Lobby::g_hasLobby >= 2;
				}
			}
		}
	}
}