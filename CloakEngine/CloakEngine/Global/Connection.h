#pragma once
#ifndef CE_API_GLOBAL_CONNECTION_H
#define CE_API_GLOBAL_CONNECTION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Files/Buffer.h"

#include <string>

#define LOBBY_MAX_SIZE 0

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Lobby {
				enum class ConnectionType {
					Client,
					Server,
				};
				enum class TPType {
					TCP,
					UDP,
					UNKNOWN,
				};
				enum class IPType {
					IPv4,
					IPv6,
				};
				struct IPAddress {
					IPType Type;
					union {
						struct {
							uint8_t addr[4];
						} IPv4;
						struct {
							uint16_t addr[8];
						} IPv6;
					};
					uint16_t Port;
					const char* Name;
					CLOAKENGINE_API CLOAK_CALL IPAddress();
					CLOAKENGINE_API CLOAK_CALL IPAddress(In const std::string& addrStr);
					CLOAKENGINE_API CLOAK_CALL IPAddress(In const std::string& addrStr, In uint16_t port);
					CLOAKENGINE_API bool CLOAK_CALL_THIS operator==(In const IPAddress& o) const;
				};
			}
			inline namespace Connection_v1 {
				CLOAK_INTERFACE_ID("{024055E0-E389-41D8-9FC6-ACF2D8DB9583}") IConnection : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS Close() = 0;
						virtual bool CLOAK_CALL_THIS IsConnected() const = 0;
						virtual Lobby::TPType CLOAK_CALL_THIS GetProtocoll() const = 0;
						virtual Lobby::ConnectionType CLOAK_CALL_THIS GetType() const = 0;
						virtual void CLOAK_CALL_THIS SetData(In LPVOID data) = 0;
						virtual LPVOID CLOAK_CALL_THIS GetData() const = 0;
						virtual API::Files::IReadBuffer* CLOAK_CALL_THIS GetReadBuffer() const = 0;
						virtual API::Files::IWriteBuffer* CLOAK_CALL_THIS GetWriteBuffer() const = 0;
				};
				CLOAK_INTERFACE IConnectionCallback : public virtual API::Helper::SavePtr_v1::IBasicPtr{
					public:
						virtual void CLOAK_CALL_THIS OnConnect(In IConnection* connection) = 0;
						virtual void CLOAK_CALL_THIS OnReceiveData(In IConnection* connection) = 0;
						virtual void CLOAK_CALL_THIS OnDisconnect(In IConnection* connection) = 0;
				};
			}
			namespace Lobby {
				CLOAKENGINE_API void CLOAK_CALL Connect(In const API::Global::Lobby::IPAddress& ad, In TPType prot, In const Files::UGID& cgid, In_opt IConnectionCallback* callback = nullptr);
				CLOAKENGINE_API void CLOAK_CALL Connect(In const API::Global::Lobby::IPAddress& ad, In TPType prot, In_opt IConnectionCallback* callback = nullptr);
				CLOAKENGINE_API void CLOAK_CALL Open(In const Files::UGID& cgid, In_opt size_t maxSize = LOBBY_MAX_SIZE);
				CLOAKENGINE_API void CLOAK_CALL Close();
				CLOAKENGINE_API bool CLOAK_CALL IsOpen();
			}
		}
	}
}

#endif