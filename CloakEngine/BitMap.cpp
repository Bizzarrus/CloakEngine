#include "stdafx.h"
#include "Engine/BitMap.h"
#include "CloakEngine/Global/Memory.h"

#include "CloakEngine/Global/Debug.h"

#include <cmath>
#include <stdexcept>
#include <string>

#define BYTE_SIZE (sizeof(BYTE)*8)

namespace CloakEngine {
	namespace Engine {
		UsageMap::const_iterator CLOAK_CALL_THIS UsageMap::const_iterator::operator++(In int)
		{
			const_iterator o = *this;
			operator++();
			return o;
		}
		UsageMap::const_iterator& CLOAK_CALL_THIS UsageMap::const_iterator::operator++()
		{
			if (m_map != nullptr && m_pos < m_map->m_size)
			{
				do {
					m_pos++;
				} while (m_pos < m_map->m_size && m_map->Contains(m_pos) == false);
				if (m_pos >= m_map->m_size) { m_pos = static_cast<size_t>(-1); }
			}
			return *this;
		}
		UsageMap::const_iterator CLOAK_CALL_THIS UsageMap::const_iterator::operator--(In int)
		{
			const_iterator o = *this;
			operator--();
			return o;
		}
		UsageMap::const_iterator& CLOAK_CALL_THIS UsageMap::const_iterator::operator--()
		{
			if (m_map != nullptr)
			{
				size_t p = min(m_pos, m_map->m_size);
				const size_t lp = p;
				while (p > 0 && m_map->Contains(p - 1) == false) { p--; }
				if (p == 0) { m_pos = lp; }
				else { m_pos = p - 1; }
			}
			return *this;
		}
		UsageMap::const_iterator::reference CLOAK_CALL_THIS UsageMap::const_iterator::operator*() const
		{
			return m_pos;
		}
		UsageMap::const_iterator::pointer CLOAK_CALL_THIS UsageMap::const_iterator::operator->() const
		{
			return &m_pos;
		}
		bool CLOAK_CALL_THIS UsageMap::const_iterator::operator==(In const UsageMap::const_iterator& o) const
		{
			return m_map == o.m_map && m_pos == o.m_pos;
		}
		bool CLOAK_CALL_THIS UsageMap::const_iterator::operator!=(In const UsageMap::const_iterator& o) const
		{
			return m_map != o.m_map || m_pos != o.m_pos;
		}

		CLOAK_CALL UsageMap::const_iterator::const_iterator(In size_t p, In const UsageMap* map) : m_pos(p), m_map(map) {}

		CLOAK_CALL UsageMap::UsageMap()
		{
			m_size = 0;
			m_base = 0;
		}
		CLOAK_CALL UsageMap::UsageMap(In const UsageMap& o)
		{
			m_size = o.m_size;
			if (m_size <= 64) { m_base = o.m_base; }
			else 
			{
				const size_t s = (m_size >> 3) + 1;
				m_extended = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryPool::Allocate(sizeof(uint8_t)*s));
				for (size_t a = 0; a < s; a++) { m_extended[a] = o.m_extended[a]; }
			}
		}
		CLOAK_CALL UsageMap::~UsageMap()
		{
			if (m_size > 64) { API::Global::Memory::MemoryPool::Free(m_extended); }
		}
		void CLOAK_CALL_THIS UsageMap::Add(In size_t pos)
		{
			const size_t nps = pos + 1;
			if (m_size <= 64 && nps <= 64) { m_base |= 1Ui64 << pos; }
			else if (m_size <= 64 && nps > 64)
			{
				const size_t a = pos >> 3;
				const size_t b = pos & 0x7;
				const uint64_t l = m_base;
				CLOAK_ASSUME(a > 7);
				m_extended = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryPool::Allocate(sizeof(uint8_t)*(a + 1)));
				m_extended[0] = static_cast<uint8_t>((l >>  0) & 0xFF);
				m_extended[1] = static_cast<uint8_t>((l >>  8) & 0xFF);
				m_extended[2] = static_cast<uint8_t>((l >> 16) & 0xFF);
				m_extended[3] = static_cast<uint8_t>((l >> 24) & 0xFF);
				m_extended[4] = static_cast<uint8_t>((l >> 32) & 0xFF);
				m_extended[5] = static_cast<uint8_t>((l >> 40) & 0xFF);
				m_extended[6] = static_cast<uint8_t>((l >> 48) & 0xFF);
				m_extended[7] = static_cast<uint8_t>((l >> 56) & 0xFF);
				for (size_t c = 8; c < a; c++) { m_extended[c] = 0; }
				m_extended[a] = 1Ui8 << b;
			}
			else if (m_size > 64) 
			{
				const size_t a = pos >> 3;
				const size_t b = pos & 0x7;
				const size_t os = (m_size >> 3) + 1;
				const size_t ns = max(os, a + 1);
				if (ns > os) 
				{
					uint8_t* l = m_extended;
					m_extended = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryPool::Allocate(sizeof(uint8_t)*ns));
					for (size_t c = 0; c < os; c++) { m_extended[c] = l[c]; }
					m_extended[a] = 1Ui8 << b;
				}
				else { m_extended[a] |= 1Ui8 << b; }
			}
			m_size = max(m_size, nps);
		}
		void CLOAK_CALL_THIS UsageMap::Remove(In size_t pos)
		{
			if (pos < m_size)
			{
				if (m_size <= 64) { m_base &= ~(1Ui64 << pos); }
				else
				{
					const size_t a = pos >> 3;
					const size_t b = pos & 0x7;
					m_extended[a] &= ~(1Ui8 << b);
				}
			}
		}
		void CLOAK_CALL_THIS UsageMap::Clear()
		{
			if (m_size > 64) { API::Global::Memory::MemoryPool::Free(m_extended); }
			m_size = 0;
			m_base = 0;
		}
		bool CLOAK_CALL_THIS UsageMap::Contains(In size_t pos) const
		{
			if (m_size <= 64) { return (m_base & (1Ui64 << pos)) != 0; }
			else if (pos < m_size) { return (m_extended[pos >> 3] & (1Ui8 << (pos & 0x7))) != 0; }
			return false;
		}
		UsageMap::const_iterator CLOAK_CALL_THIS UsageMap::begin() const
		{
			size_t p = 0;
			while (p < m_size && Contains(p) == false) { p++; }
			if (p >= m_size) { p = static_cast<size_t>(-1); }
			return const_iterator(p, this);
		}
		UsageMap::const_iterator CLOAK_CALL_THIS UsageMap::end() const
		{
			return const_iterator(static_cast<size_t>(-1), this);
		}
		UsageMap& CLOAK_CALL_THIS UsageMap::operator=(In const UsageMap& o)
		{
			if (&o != this)
			{
				if (m_size > 64) { API::Global::Memory::MemoryPool::Free(m_extended); }
				m_size = o.m_size;
				if (m_size <= 64) { m_base = o.m_base; }
				else
				{
					const size_t s = (m_size >> 3) + 1;
					m_extended = reinterpret_cast<uint8_t*>(API::Global::Memory::MemoryPool::Allocate(sizeof(uint8_t)*s));
					for (size_t a = 0; a < s; a++) { m_extended[a] = o.m_extended[a]; }
				}
			}
			return *this;
		}


		CLOAK_CALL_THIS BitMap::BitBool::BitBool(Inout BYTE& d, In size_t pos)
		{
			m_data = &d;
			m_mask = 1 << pos;
		}
		CLOAK_CALL BitMap::BitBool::BitBool()
		{
			m_data = nullptr;
			m_mask = 0;
		}
		CLOAK_CALL_THIS BitMap::BitBool::~BitBool() {}
		bool CLOAK_CALL_THIS BitMap::BitBool::operator=(In bool b)
		{
			if (m_data == nullptr) { return b; }
			if (b == true) { *m_data |= m_mask; }
			else { *m_data &= ~m_mask; }
			return b;
		}
		CLOAK_CALL_THIS BitMap::BitBool::operator bool()
		{
			if (m_data == nullptr) { return false; }
			return (*m_data & m_mask) != 0;
		}


		CLOAK_CALL_THIS BitMap::BitMap(In size_t size)
		{
			m_size = size;
			size_t bytes = (size_t)std::ceil((double)m_size / BYTE_SIZE);
			if (bytes > 0) { m_data = NewArray(BYTE, bytes); }
			else { m_data = nullptr; }
			for (size_t a = 0; a < bytes; a++) { m_data[a] = 0; }
		}
		CLOAK_CALL_THIS BitMap::BitMap(In const BitMap& bm)
		{
			m_size = bm.m_size;
			size_t bytes = (size_t)std::ceil((double)m_size / BYTE_SIZE);
			if (bytes > 0) { m_data = NewArray(BYTE, bytes); }
			else { m_data = nullptr; }
			for (size_t a = 0; a < bytes; a++) { m_data[a] = bm.m_data[a]; }
		}
		CLOAK_CALL_THIS BitMap::~BitMap()
		{
			DeleteArray(m_data);
		}
		size_t CLOAK_CALL_THIS BitMap::size() { return m_size; }
		BitMap::BitBool CLOAK_CALL_THIS BitMap::operator[](In size_t a)
		{
			if (CloakDebugCheckOK(a < m_size, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true))
			{
				const size_t p = static_cast<size_t>(std::floor(static_cast<double>(a) / BYTE_SIZE));
				return BitBool(m_data[p], a % BYTE_SIZE);
			}
			return BitBool();
		}
		bool CLOAK_CALL_THIS BitMap::operator[](In size_t a) const
		{
			if (CloakDebugCheckOK(a < m_size, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true))
			{
				const size_t p = static_cast<size_t>(std::floor(static_cast<double>(a) / BYTE_SIZE));
				return (m_data[p] & (1 << (a & BYTE_SIZE))) != 0;
			}
			return false;
		}
		bool CLOAK_CALL_THIS BitMap::isAt(In size_t a) const
		{
			if (CloakDebugCheckOK(a < m_size, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true, std::to_string(a) + " is out of range (" + std::to_string(m_size) + ")"))
			{
				size_t p = static_cast<size_t>(std::floor(static_cast<double>(a) / BYTE_SIZE));
				size_t m = static_cast<size_t>(1ULL << (a % BYTE_SIZE));
				return (m_data[p] & m) != 0;
			}
			return false;
		}
		void CLOAK_CALL_THIS BitMap::set(In size_t p, In bool s)
		{
			if (CloakDebugCheckOK(p < m_size, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true, std::to_string(p) + " is out of range (" + std::to_string(m_size) + ")"))
			{
				const size_t bp= static_cast<size_t>(std::floor(static_cast<double>(p) / BYTE_SIZE));
				const size_t bi = p % BYTE_SIZE;
				const uint8_t mask = 1 << bi;
				if (s) { m_data[bp] |= mask; }
				else { m_data[bp] &= ~mask; }
			}
		}
		void* CLOAK_CALL BitMap::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
		void CLOAK_CALL BitMap::operator delete(In void* ptr) { API::Global::Memory::MemoryPool::Free(ptr); }
	}
}