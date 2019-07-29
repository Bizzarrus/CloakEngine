#pragma once
#ifndef CE_E_COMPRESS_UTF_H
#define CE_E_COMPRESS_UTF_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/Compressor.h"
#include "Implementation/Files/Buffer.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Engine {
		namespace Compress {
			namespace UTF {
				class UTFReadBuffer : public virtual API::Files::Buffer_v1::IReadBuffer, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL UTFReadBuffer(In const CE::RefPointer<API::Files::Buffer_v1::IReadBuffer>& input);
						virtual CLOAK_CALL ~UTFReadBuffer();
						virtual unsigned long long CLOAK_CALL_THIS GetPosition() const override;
						virtual uint8_t CLOAK_CALL_THIS ReadByte() override;
						virtual bool CLOAK_CALL_THIS HasNextByte() override;
						void* CLOAK_CALL operator new(In size_t size);
						void CLOAK_CALL operator delete(In void* ptr);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						enum class Input
						{
							BOM_00 = 0,
							FF = 1,
							FE = 2,
							EF = 3,
							BB = 4,
							BF = 5,
							U8_1 = 6,
							U8_2 = 7,
							U8_3 = 8,
							U8_PART = 9,
							ASCII = 10,
							U16_HS = 11,
							U16_LS = 12,
							INVALID = 13
						};
						enum State {
							RDY_U8_C = 0,
							WAT_U8_C1 = 1,
							WAT_U8_C2 = 2,
							RDY_U8_S = 3,
							RDY_U8 = 4,
							WAT_U8_1 = 5,
							WAT_U8_2 = 6,
							WAT_U8_3 = 7,

							RDY_U16_BE_C = 8,
							RDY_U16_BE_S = 9,
							RDY_U16_BE = 10,
							WAT_U16_BE = 11,
							WAT_U16_BE_HS = 12,
							RDY_U16_BE_HS = 13,
							WAT_U16_BE_LS = 14,

							RDY_U16_LE_C = 15,
							RDY_U16_LE_S = 16,
							RDY_U16_LE = 17,
							WAT_U16_LE = 18,
							RDY_U16_LE_HS = 19,
							WAT_U16_LE_LS = 20,

							WAT_U32_BE_C1 = 21,
							WAT_U32_BE_C2 = 22,
							RDY_U32_BE_C = 23,
							RDY_U32_BE_S = 24,
							RDY_U32_BE = 25,
							WAT_U32_BE_1 = 26,
							WAT_U32_BE_2 = 27,
							WAT_U32_BE_3 = 28,

							RDY_U32_LE_C = 29,
							RDY_U32_LE_S = 30,
							RDY_U32_LE = 31,
							WAT_U32_LE_1 = 32,
							WAT_U32_LE_2 = 33,
							WAT_U32_LE_3 = 34,

							UNKNOWN = 35,
						};

						static constexpr size_t g_typeSize = static_cast<size_t>(Input::INVALID);

						static constexpr State g_machine[][g_typeSize] = {
							/*					 BOM_00			FF				FE				EF				BB				BF				U8_1			U8_2			U8_3			U8_PART			ASCII			U16_HS			U16_LS*/
							/*RDY_U8_C*/		{ WAT_U32_BE_C1,RDY_U16_LE_C,	RDY_U16_BE_C,	WAT_U8_C1,		UNKNOWN,		UNKNOWN,		WAT_U8_1,		WAT_U8_2,		WAT_U8_3,		UNKNOWN,		RDY_U8,			UNKNOWN,		UNKNOWN },
							/*WAT_U8_C1*/		{ UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		WAT_U8_C2,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*WAT_U8_C2*/		{ UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		RDY_U8_S,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*RDY_U8_S*/		{ UNKNOWN,		WAT_U8_3,		WAT_U8_3,		WAT_U8_2,		UNKNOWN,		UNKNOWN,		WAT_U8_1,		WAT_U8_2,		WAT_U8_3,		UNKNOWN,		RDY_U8,			WAT_U8_1,		WAT_U8_1 },
							/*RDY_U8*/			{ UNKNOWN,		WAT_U8_3,		WAT_U8_3,		WAT_U8_2,		UNKNOWN,		UNKNOWN,		WAT_U8_1,		WAT_U8_2,		WAT_U8_3,		UNKNOWN,		RDY_U8,			WAT_U8_1,		WAT_U8_1 },
							/*WAT_U8_1*/		{ UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		RDY_U8,			RDY_U8,			UNKNOWN,		UNKNOWN,		UNKNOWN,		RDY_U8,			UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*WAT_U8_2*/		{ UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		WAT_U8_1,		WAT_U8_1,		UNKNOWN,		UNKNOWN,		UNKNOWN,		WAT_U8_1,		UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*WAT_U8_3*/		{ UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		WAT_U8_2,		WAT_U8_2,		UNKNOWN,		UNKNOWN,		UNKNOWN,		WAT_U8_2,		UNKNOWN,		UNKNOWN,		UNKNOWN },

							/*RDY_U16_BE_C*/	{ UNKNOWN,		RDY_U16_BE_S,	UNKNOWN,		UNKNOWN,		WAT_U8_2,		WAT_U8_2,		UNKNOWN,		UNKNOWN,		UNKNOWN,		WAT_U8_2,		UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*RDY_U16_BE_S*/	{ WAT_U16_BE,	WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE_HS,	UNKNOWN },
							/*RDY_U16_BE*/		{ WAT_U16_BE,	WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE,		WAT_U16_BE_HS,	UNKNOWN },
							/*WAT_U16_BE*/		{ RDY_U16_BE,	RDY_U16_BE,		RDY_U16_BE, 	RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE },
							/*WAT_U16_BE_HS*/	{ RDY_U16_BE_HS,RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS,	RDY_U16_BE_HS },
							/*RDY_U16_BE_HS*/	{ UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		WAT_U16_BE_LS },
							/*WAT_U16_BE_LS*/	{ RDY_U16_BE,	RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE,		RDY_U16_BE, 	RDY_U16_BE, 	RDY_U16_BE, 	RDY_U16_BE, 	RDY_U16_BE, 	RDY_U16_BE, 	RDY_U16_BE, 	RDY_U16_BE, 	RDY_U16_BE },

							/*RDY_U16_LE_C*/	{ UNKNOWN,		UNKNOWN,		RDY_U16_LE_S,	UNKNOWN,		WAT_U8_2,		WAT_U8_2,		UNKNOWN,		UNKNOWN,		UNKNOWN,		WAT_U8_2,		UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*RDY_U16_LE_S*/	{ RDY_U32_LE_C, WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE },
							/*RDY_U16_LE*/		{ WAT_U16_LE,	WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE,		WAT_U16_LE },
							/*WAT_U16_LE*/		{ RDY_U16_LE,	RDY_U16_LE,		RDY_U16_LE,		RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE_HS, 	RDY_U16_LE },
							/*RDY_U16_LE_HS*/	{ WAT_U16_LE_LS,WAT_U16_LE_LS,	WAT_U16_LE_LS,	WAT_U16_LE_LS, 	WAT_U16_LE_LS, 	WAT_U16_LE_LS, 	WAT_U16_LE_LS, 	WAT_U16_LE_LS, 	WAT_U16_LE_LS, 	WAT_U16_LE_LS, 	WAT_U16_LE_LS, 	WAT_U16_LE_LS, 	WAT_U16_LE_LS },
							/*WAT_U16_LE_LS*/	{ UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN, 		UNKNOWN, 		UNKNOWN, 		UNKNOWN, 		UNKNOWN, 		UNKNOWN, 		UNKNOWN, 		UNKNOWN, 		UNKNOWN, 		RDY_U16_LE },

							/*WAT_U32_BE_C1*/	{ WAT_U32_BE_C2,UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*WAT_U32_BE_C2*/	{ UNKNOWN,		UNKNOWN,		RDY_U32_BE_C,	UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*RDY_U32_BE_C*/	{ UNKNOWN,		RDY_U32_BE_S,	UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN,		UNKNOWN },
							/*RDY_U32_BE_S*/	{ WAT_U32_BE_3,	WAT_U32_BE_3,	WAT_U32_BE_3,	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3 },
							/*RDY_U32_BE*/		{ WAT_U32_BE_3,	WAT_U32_BE_3,	WAT_U32_BE_3,	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3, 	WAT_U32_BE_3 },
							/*WAT_U32_BE_1*/	{ RDY_U32_BE,	RDY_U32_BE,		RDY_U32_BE,		RDY_U32_BE, 	RDY_U32_BE, 	RDY_U32_BE, 	RDY_U32_BE, 	RDY_U32_BE, 	RDY_U32_BE, 	RDY_U32_BE, 	RDY_U32_BE, 	RDY_U32_BE, 	RDY_U32_BE },
							/*WAT_U32_BE_2*/	{ WAT_U32_BE_1,	WAT_U32_BE_1,	WAT_U32_BE_1,	WAT_U32_BE_1, 	WAT_U32_BE_1, 	WAT_U32_BE_1, 	WAT_U32_BE_1, 	WAT_U32_BE_1, 	WAT_U32_BE_1, 	WAT_U32_BE_1, 	WAT_U32_BE_1, 	WAT_U32_BE_1, 	WAT_U32_BE_1 },
							/*WAT_U32_BE_3*/	{ WAT_U32_BE_2,	WAT_U32_BE_2,	WAT_U32_BE_2,	WAT_U32_BE_2, 	WAT_U32_BE_2, 	WAT_U32_BE_2, 	WAT_U32_BE_2, 	WAT_U32_BE_2, 	WAT_U32_BE_2, 	WAT_U32_BE_2, 	WAT_U32_BE_2, 	WAT_U32_BE_2, 	WAT_U32_BE_2 },

							/*RDY_U32_LE_C*/	{ RDY_U32_LE_S,	RDY_U16_LE,		RDY_U16_LE,		RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE, 	RDY_U16_LE_HS, 	RDY_U16_LE },
							/*RDY_U32_LE_S*/	{ WAT_U32_LE_3,	WAT_U32_LE_3,	WAT_U32_LE_3,	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3 },
							/*RDY_U32_LE*/		{ WAT_U32_LE_3,	WAT_U32_LE_3,	WAT_U32_LE_3,	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3, 	WAT_U32_LE_3 },
							/*WAT_U32_LE_1*/	{ RDY_U32_LE,	RDY_U32_LE,		RDY_U32_LE,		RDY_U32_LE, 	RDY_U32_LE, 	RDY_U32_LE, 	RDY_U32_LE, 	RDY_U32_LE, 	RDY_U32_LE, 	RDY_U32_LE, 	RDY_U32_LE, 	RDY_U32_LE, 	RDY_U32_LE },
							/*WAT_U32_LE_2*/	{ WAT_U32_LE_1,	WAT_U32_LE_1,	WAT_U32_LE_1,	WAT_U32_LE_1, 	WAT_U32_LE_1, 	WAT_U32_LE_1, 	WAT_U32_LE_1, 	WAT_U32_LE_1, 	WAT_U32_LE_1, 	WAT_U32_LE_1, 	WAT_U32_LE_1, 	WAT_U32_LE_1, 	WAT_U32_LE_1 },
							/*WAT_U32_LE_3*/	{ WAT_U32_LE_2,	WAT_U32_LE_2,	WAT_U32_LE_2,	WAT_U32_LE_2, 	WAT_U32_LE_2, 	WAT_U32_LE_2, 	WAT_U32_LE_2, 	WAT_U32_LE_2, 	WAT_U32_LE_2, 	WAT_U32_LE_2, 	WAT_U32_LE_2, 	WAT_U32_LE_2, 	WAT_U32_LE_2 },
						};

						CE::RefPointer<API::Files::IReadBuffer> m_read;
						State m_state;
						char32_t m_curChar;
						uint8_t m_charPos;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif