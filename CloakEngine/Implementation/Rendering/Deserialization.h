#pragma once
#ifndef CE_IMPL_RENDERING_DESERIALIZATION_H
#define CE_IMPL_RENDERING_DESERIALIZATION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Helper/Predicate.h"
#include "CloakEngine/Helper/Types.h"

#include <string>

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			constexpr size_t PS_SERIIALIZATION_HEADER_SIZE = 17;
			class Deserializer {
				private:
					API::List<char*> m_strAlloc;
					size_t m_pos = 0;
					const void* const m_data;
				public:
					CLOAK_CALL Deserializer(In const void* data) : m_data(data)
					{

					}
					CLOAK_CALL ~Deserializer()
					{
						for (size_t a = 0; a < m_strAlloc.size(); a++)
						{
							DeleteArray(m_strAlloc[a]);
						}
					}
					template<typename A, std::enable_if_t<std::is_pointer_v<A> == false>* = nullptr> inline void CLOAK_CALL DeserializeData(Out A* res)
					{
						typename int_by_size<sizeof(A)>::unsigned_t b = 0;
						for (size_t c = 0; c < sizeof(A); c++)
						{
							const typename int_by_size<sizeof(A)>::unsigned_t d = reinterpret_cast<const uint8_t*>(m_data)[m_pos + c];
							b |= d << ((sizeof(A) - (c + 1)) << 3);
						}
						*res = *reinterpret_cast<A*>(&b);
						m_pos += sizeof(A);
					}
					inline void CLOAK_CALL DeserializeData(Out LPCSTR* res)
					{
						uint32_t l = 0;
						DeserializeData(&l);
						char* d = NewArray(char, l + 1);
						m_strAlloc.push_back(d);
						for (size_t c = 0; c < l; c++) { d[c] = static_cast<char>(reinterpret_cast<const uint8_t*>(m_data)[m_pos + c]); }
						*res = d;
						m_pos += l;
					}
					size_t CLOAK_CALL GetPos() const { return m_pos; }
					void CLOAK_CALL SetPos(In size_t pos) { m_pos = pos; }
				private:
					CLOAK_CALL Deserializer(In const Deserializer& a) : m_data(nullptr) {}
					Deserializer & CLOAK_CALL_THIS operator=(In const Deserializer& a) { return *this; }
			};
		}
	}
}

#endif