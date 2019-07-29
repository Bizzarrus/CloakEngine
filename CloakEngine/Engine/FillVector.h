#pragma once
#ifndef CE_E_FILLVECTOR_H
#define CE_E_FILLVECTOR_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Memory.h"

#include <assert.h>

namespace CloakEngine {
	namespace Engine {
		template<typename T> class FillVector {
			private:
				class Element {
					private:
						bool m_used;
						T* m_data;
						BYTE m_dataSrc[sizeof(T) / sizeof(BYTE)];
					public:
						CLOAK_CALL Element()
						{
							m_data = nullptr;
							m_used = false;
						}
						CLOAK_CALL ~Element()
						{
							if (m_data != nullptr) { m_data->~T(); }
						}
						inline bool CLOAK_CALL_THIS IsUsed() { return m_used; }
						inline const T& CLOAK_CALL_THIS GetData() const { return *m_data; }
						inline T& CLOAK_CALL_THIS GetData() { return *m_data; }
						inline void CLOAK_CALL_THIS remove() 
						{
							if (m_data != nullptr) { m_data->~T(); }
							m_data = nullptr;
							m_used = false;
						}
						inline Element& CLOAK_CALL_THIS operator=(In const Element& e)
						{
							m_used = e.m_used;
							memcpy(m_dataSrc, e.m_dataSrc, sizeof(T));
							return *this;
						}
						inline Element& CLOAK_CALL_THIS operator=(In const T& e)
						{
							if (&e != m_data)
							{
								if (m_data != nullptr) { m_data->~T(); }
								else { m_data = reinterpret_cast<T*>(&m_dataSrc[0]); }
								new(m_data) T(e);
							}
							m_used = true;
							return *this;
						}
						void* CLOAK_CALL operator new[](In size_t size) { return API::Global::Memory::MemoryHeap::Allocate(size); }
						void CLOAK_CALL operator delete[](In void* ptr) { Memory::FreeFromFree(ptr); }
				};
				size_t m_size;
				size_t m_len;
				size_t m_freePos;
				size_t m_freeSize;
				Element* m_data;

				inline void CLOAK_CALL_THIS setElement(In const T& data, In size_t pos)
				{
					if (pos >= m_size)
					{
						const size_t os = m_size;
						do { m_size *= 2; } while (pos >= m_size);
						Element* nd = NewArray(Element, m_size);
						for (size_t a = 0; a < os; a++) { nd[a] = m_data[a]; }
						DeleteArray(m_data);
						m_data = nd;
					}
					m_data[pos] = data;
				}
				inline void CLOAK_CALL_THIS removeElement(In size_t pos)
				{
					if (pos < m_size)
					{
						m_data[pos].remove();
						if (pos < m_freePos) { m_freePos = pos; }
					}
				}
			public:
				CLOAK_CALL FillVector(In_opt size_t size = 16)
				{
					m_size = size;
					m_len = 0;
					m_freePos = 0;
					m_data = NewArray(Element, m_size);
				}
				CLOAK_CALL ~FillVector()
				{
					DeleteArray(m_data);
				}
				inline bool CLOAK_CALL_THIS exist(In size_t pos) const
				{
					return pos < m_len && m_data[pos].IsUsed();
				}
				inline size_t CLOAK_CALL_THIS push(In const T& data)
				{
					if (m_freeSize > 0)
					{
						while (m_freePos < m_len && m_data[m_freePos].IsUsed()) { m_freePos++; }
					}
					else
					{
						m_freePos = m_len;
					}
					const size_t r = m_freePos++;
					setElement(data, r);
					if (r >= m_len) { m_len++; }
					else { m_freeSize--; }
					return r;
				}
				inline size_t CLOAK_CALL_THIS getMaxSize() const { return m_len; }
				inline size_t CLOAK_CALL_THIS getNext(In size_t prev) const
				{
					size_t p = min(m_len, prev + 1);
					while (p < m_len && m_data[p].IsUsed() == false) { p++; }
					return p;
				}
				inline size_t CLOAK_CALL_THIS getFirst() const
				{
					size_t p = 0;
					if (m_data[p].IsUsed() == false) { p = getNext(0); }
					return p;
				}
				inline void CLOAK_CALL_THIS pop(In size_t pos)
				{
					assert(pos < m_len);
					removeElement(pos);
					if (pos + 1 == m_len) { m_len--; }
					else { m_freeSize++; }
				}
				inline const T& CLOAK_CALL_THIS operator[](In size_t pos) const
				{
					assert(pos < m_len);
					return m_data[pos].GetData();
				}
				inline T& CLOAK_CALL_THIS operator[](In size_t pos)
				{
					assert(pos < m_len);
					return m_data[pos].GetData();
				}
		};
	}
}

#endif