#pragma once
#ifndef CE_API_FILES_EXTENDEDBUFFERS_H
#define CE_API_FILES_EXTENDEDBUFFERS_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Buffer.h"
#include "CloakEngine/Files/Compressor.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Buffer_v1 {
				CLOAK_INTERFACE_ID("{46DB2C4D-4CE0-4383-9726-D9C186CDE5DD}") IMultithreadedWriteBuffer : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual CE::RefPointer<IWriteBuffer> CLOAK_CALL_THIS CreateLocalWriteBuffer(In_opt void* userData = nullptr) = 0;
						virtual CE::RefPointer<CE::Files::IWriter> CLOAK_CALL_THIS CreateLocalWriter(In_opt void* userData = nullptr) = 0;
						virtual void CLOAK_CALL_THIS SetFinishTask(In const API::Global::Task& task) = 0;
				};
				CLOAK_INTERFACE_ID("{144F0DFC-FCCA-4EC6-AF91-17DDD902ED04}") IHTTPReadBuffer : public virtual Buffer_v1::IReadBuffer{
					public:
						virtual uint16_t CLOAK_CALL_THIS GetStatusCode() = 0;
						virtual RequestID CLOAK_CALL_THIS Open(In const std::string& address, In_opt HTTPMethod method = HTTPMethod::GET, In_opt uint16_t port = 80) = 0;
						virtual RequestID CLOAK_CALL_THIS Open(In const std::string& address, In uint16_t port, In_opt HTTPMethod method = HTTPMethod::GET) = 0;
						virtual bool CLOAK_CALL_THIS WaitForNextData() = 0;
						virtual RequestID CLOAK_CALL_THIS GetRequestID() const = 0;
						virtual const std::string& CLOAK_CALL_THIS GetHostName() const = 0;
				};

				CLOAKENGINE_API CE::RefPointer<IHTTPReadBuffer> CLOAK_CALL CreateHTTPRead(In const std::string& host, In_opt HTTPMethod method = HTTPMethod::GET, In_opt uint16_t port = 80);
				CLOAKENGINE_API CE::RefPointer<IHTTPReadBuffer> CLOAK_CALL CreateHTTPRead(In const std::string& host, In uint16_t port, In_opt HTTPMethod method = HTTPMethod::GET);
				CLOAKENGINE_API CE::RefPointer<IHTTPReadBuffer> CLOAK_CALL CreateHTTPRead();
				CLOAKENGINE_API CE::RefPointer<IMultithreadedWriteBuffer> CLOAK_CALL CreateMultithreadedWriteBuffer(In const CE::RefPointer<IWriteBuffer>& dst, In_opt std::function<void(In void* userData, In uint64_t size)> onWrite = nullptr, In_opt API::Global::Threading::ScheduleHint writingHint = API::Global::Threading::ScheduleHint::None);
				CLOAKENGINE_API CE::RefPointer<IMultithreadedWriteBuffer> CLOAK_CALL CreateMultithreadedWriteBuffer(In const CE::RefPointer<IWriteBuffer>& dst, In API::Global::Threading::ScheduleHint writingHint);
			}
		}
	}
}

#endif