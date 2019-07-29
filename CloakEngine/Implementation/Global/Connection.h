#pragma once
#ifndef CE_IMPL_GLOBAL_CONNECTION_H
#define CE_IMPL_GLOBAL_CONNECTION_H

#include "CloakEngine/Global/Connection.h"
#include "CloakEngine/Helper/SyncSection.h"

#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Files/Buffer.h"

#include "Engine/LinkedList.h"

#include <unordered_map>
#include <functional>
#include <atomic>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <in6addr.h>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Global {
			namespace Lobby {
				void CLOAK_CALL OnStart(In size_t threadID);
				void CLOAK_CALL OnUpdate(In size_t threadID, In API::Global::Time etime);
				void CLOAK_CALL OnStop(In size_t threadID);
				void CLOAK_CALL OnMessage(In WPARAM wParam, In LPARAM lParam);
				void CLOAK_CALL SetLobbyEventHandler(In API::Global::ILobbyEvent* ev);
			}
			inline namespace Connection_v1 {	
				enum class CallbackType {
					ReceivedData,
					Connected,
					Disconnected,
				};
				typedef std::function<void(CallbackType)> ConnectionCallback;
				CLOAK_INTERFACE_BASIC INetBuffer : public virtual API::Helper::SavePtr_v1::ISavePtr {
					public:
						virtual void CLOAK_CALL_THIS OnWrite() = 0;
						virtual void CLOAK_CALL_THIS OnRead(In_opt uint8_t* data = nullptr, In_opt size_t dataSize = 0) = 0;
						virtual void CLOAK_CALL_THIS OnUpdate() = 0;
						virtual void CLOAK_CALL_THIS OnConnect() = 0;
						virtual void CLOAK_CALL_THIS SetCallback(In ConnectionCallback cb) = 0;
						virtual bool CLOAK_CALL_THIS IsConnected() = 0;
						virtual API::Files::Buffer_v1::IWriteBuffer* CLOAK_CALL_THIS GetWriteBuffer() = 0;
						virtual API::Files::Buffer_v1::IReadBuffer* CLOAK_CALL_THIS GetReadBuffer() = 0;
						virtual unsigned long long CLOAK_CALL_THIS GetWritePos() const = 0;
						virtual unsigned long long CLOAK_CALL_THIS GetReadPos() const = 0;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) = 0;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() = 0;
						virtual void CLOAK_CALL_THIS SendData(In_opt bool FragRef = false) = 0;
						virtual bool CLOAK_CALL_THIS HasData() = 0;
				};
				class NetWriteBuffer : public virtual API::Files::Buffer_v1::IWriteBuffer {
					public:
						CLOAK_CALL NetWriteBuffer(In INetBuffer* par);
						virtual CLOAK_CALL ~NetWriteBuffer();

						virtual API::Global::Debug::Error CLOAK_CALL_THIS Delete() override;
						Success(return == S_OK) Post_satisfies((*ppvObject) != nullptr) virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
						virtual uint64_t CLOAK_CALL_THIS AddRef() override;
						virtual uint64_t CLOAK_CALL_THIS Release() override;
						virtual uint64_t CLOAK_CALL_THIS GetRefCount() override;
						virtual void CLOAK_CALL_THIS SetDebugName(In const std::string& s) override;
						virtual void CLOAK_CALL_THIS SetSourceFile(In const std::string& s) override;
						virtual std::string CLOAK_CALL_THIS GetDebugName() const override;
						virtual std::string CLOAK_CALL_THIS GetSourceFile() const override;

						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual void CLOAK_CALL_THIS WriteByte(In uint8_t b) override;
						virtual void CLOAK_CALL_THIS Save() override;
					protected:
						INetBuffer* m_par;
						std::string m_dbgName;
						std::string m_srcFile;
				};
				class NetReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer {
					public:
						CLOAK_CALL NetReadBuffer(In INetBuffer* par);
						virtual CLOAK_CALL ~NetReadBuffer();

						virtual API::Global::Debug::Error CLOAK_CALL_THIS Delete() override;
						Success(return == S_OK) Post_satisfies((*ppvObject) != nullptr) virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
						virtual uint64_t CLOAK_CALL_THIS AddRef() override;
						virtual uint64_t CLOAK_CALL_THIS Release() override;
						virtual uint64_t CLOAK_CALL_THIS GetRefCount() override;
						virtual void CLOAK_CALL_THIS SetDebugName(In const std::string& s) override;
						virtual void CLOAK_CALL_THIS SetSourceFile(In const std::string& s) override;
						virtual std::string CLOAK_CALL_THIS GetDebugName() const override;
						virtual std::string CLOAK_CALL_THIS GetSourceFile() const override;

						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
					protected:
						INetBuffer* m_par;
						std::string m_dbgName;
						std::string m_srcFile;
				};
				class CLOAK_UUID("{3FCBFCBD-EA75-47DE-AC17-451B086996A0}") Connection : public virtual API::Global::Connection_v1::IConnection, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL Connection(In bool lobby, In SOCKET sock, In API::Global::Lobby::TPType p, In const API::Files::UGID& cgid, In const SOCKADDR_STORAGE& addr, In_opt API::Global::Connection_v1::IConnectionCallback* callback = nullptr);
						CLOAK_CALL Connection(In bool lobby, In SOCKET sock, In API::Global::Lobby::TPType p, In const SOCKADDR_STORAGE& addr, In_opt API::Global::Connection_v1::IConnectionCallback* callback = nullptr);
						virtual CLOAK_CALL ~Connection();

						virtual void CLOAK_CALL_THIS Close() override;
						virtual bool CLOAK_CALL_THIS IsConnected() const override;
						virtual API::Global::Lobby::TPType CLOAK_CALL_THIS GetProtocoll() const override;
						virtual API::Global::Lobby::ConnectionType CLOAK_CALL_THIS GetType() const override;
						virtual void CLOAK_CALL_THIS SetData(In LPVOID data) override;
						virtual LPVOID CLOAK_CALL_THIS GetData() const override;
						virtual API::Files::IReadBuffer* CLOAK_CALL_THIS GetReadBuffer() const override;
						virtual API::Files::IWriteBuffer* CLOAK_CALL_THIS GetWriteBuffer() const override;

						virtual SOCKET CLOAK_CALL_THIS GetSocket();
						virtual void CLOAK_CALL_THIS OnClose();
						virtual void CLOAK_CALL_THIS OnRead(In_opt uint8_t* data = nullptr, In_opt size_t dataSize = 0);
						virtual void CLOAK_CALL_THIS OnWrite();
						virtual void CLOAK_CALL_THIS OnConnect();
						virtual void CLOAK_CALL_THIS OnUpdate();
						virtual void CLOAK_CALL_THIS OnNetConnected();
						virtual void CLOAK_CALL_THIS OnRecData();
						virtual void CLOAK_CALL_THIS SetLobbyPtr(In_opt Engine::LinkedList<Connection*>::const_iterator ptr);
						virtual Engine::LinkedList<Connection*>::const_iterator CLOAK_CALL_THIS GetLobbyPtr() const;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						API::Helper::ISyncSection* m_sync;
						API::Global::Connection_v1::IConnectionCallback* const m_callback;
						const SOCKET m_socket;
						const bool m_lobby;
						const API::Global::Lobby::TPType m_tpp;
						bool m_identified;
						bool m_closed;
						std::atomic<bool> m_connected;
						std::atomic<bool> m_recConnected;
						INetBuffer* m_net;
						Engine::LinkedList<Connection*>::const_iterator m_lobbyPos;
						LPVOID m_data;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif