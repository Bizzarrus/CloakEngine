#pragma once
#ifndef CE_E_COMPRESS_NETBUFFER_HTTPBUFFER_H
#define CE_E_COMPRESS_NETBUFFER_HTTPBUFFER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Buffer.h"
#include "CloakEngine/Files/ExtendedBuffers.h"

#include "Implementation/Helper/SavePtr.h"

#include <zlib.h> //from vcpkg

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace NetBuffer {
				namespace HTTP {
					enum class Protocoll {
						HTTP,
					};
					struct Request {
						Protocoll Protocoll;
						std::string Host;
						std::string File;
						std::string Query;
						API::Global::Lobby::IPAddress Address;
						API::Files::HTTPMethod Method;
					};
				}
				class HTTPBuffer : public virtual API::Files::Buffer_v1::IHTTPReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL HTTPBuffer();
						CLOAK_CALL HTTPBuffer(In const std::string& address, In_opt uint16_t port = 80, In_opt API::Files::HTTPMethod method = API::Files::HTTPMethod::GET);
						virtual CLOAK_CALL ~HTTPBuffer();

						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;

						virtual uint16_t CLOAK_CALL_THIS GetStatusCode() override;
						virtual API::Files::RequestID CLOAK_CALL_THIS Open(In const std::string& address, In_opt API::Files::HTTPMethod method = API::Files::HTTPMethod::GET, In_opt uint16_t port = 80) override;
						virtual API::Files::RequestID CLOAK_CALL_THIS Open(In const std::string& address, In uint16_t port, In_opt API::Files::HTTPMethod method = API::Files::HTTPMethod::GET) override;
						virtual bool CLOAK_CALL_THIS WaitForNextData() override;
						virtual API::Files::RequestID CLOAK_CALL_THIS GetRequestID() const override;
						virtual const std::string& CLOAK_CALL_THIS GetHostName() const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
						virtual void CLOAK_CALL_THIS iReadHeader();
						virtual void CLOAK_CALL_THIS iReadData();
						virtual void CLOAK_CALL_THIS iRequest(In const HTTP::Request& request);
						virtual void CLOAK_CALL_THIS iRequestFile(In API::Global::IConnection* con, In const HTTP::Request& request, In const std::string& host);

						static constexpr size_t BUFFER_IN_SIZE = 32 KB;
						static constexpr size_t BUFFER_OUT_SIZE = 32 KB;

						API::Queue<HTTP::Request> m_requests;
						size_t m_pending;
						HTTP::Request m_curRequest;
						API::Helper::IConditionSyncSection* m_sync;
						API::Global::IConnection* m_curCon;
						API::Files::IReadBuffer* m_net;
						API::Files::RequestID m_curReqID;
						API::Files::RequestID m_nextReqID;
						size_t m_msgLength;
						uint64_t m_pos;
						uint16_t m_lastStatus;
						uint8_t m_inBuffer[BUFFER_IN_SIZE];
						uint8_t m_outBuffer[BUFFER_OUT_SIZE];
						size_t m_outLength;
						size_t m_inPos;
						size_t m_outPos;
						z_stream m_gzip;
						bool m_initEncoding;
						bool m_updReqID;
						enum class Encoding { Identity, GZIP } m_encoding;

						class Callback : public virtual API::Global::IConnectionCallback {
							public:
								CLOAK_CALL Callback(In HTTPBuffer* parent);
								void CLOAK_CALL_THIS OnConnect(In API::Global::IConnection* connection) override;
								void CLOAK_CALL_THIS OnReceiveData(In API::Global::IConnection* connection) override;
								void CLOAK_CALL_THIS OnDisconnect(In API::Global::IConnection* connection) override;

								uint64_t CLOAK_CALL_THIS AddRef() override;
								uint64_t CLOAK_CALL_THIS Release() override;
							protected:
								HTTPBuffer* const m_parent;
						} m_callback;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif