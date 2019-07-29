#pragma once
#ifndef CE_E_BITMAP_H
#define CE_E_BITMAP_H

#include "CloakEngine/Defines.h"

#include <Windows.h>

namespace CloakEngine {
	namespace Engine {
		class UsageMap {
			public:
				class const_iterator {
					friend class UsageMap;
					public:
						typedef std::bidirectional_iterator_tag iterator_category;
						typedef size_t value_type;
						typedef ptrdiff_t difference_type;
						typedef const value_type* pointer;
						typedef const value_type& reference;
					public:
						const_iterator CLOAK_CALL_THIS operator++(In int);
						const_iterator& CLOAK_CALL_THIS operator++();
						const_iterator CLOAK_CALL_THIS operator--(In int);
						const_iterator& CLOAK_CALL_THIS operator--();
						reference CLOAK_CALL_THIS operator*() const;
						pointer CLOAK_CALL_THIS operator->() const;
						bool CLOAK_CALL_THIS operator==(In const const_iterator& o) const;
						bool CLOAK_CALL_THIS operator!=(In const const_iterator& o) const;
					private:
						CLOAK_CALL const_iterator(In size_t p, In const UsageMap* map);

						size_t m_pos;
						const UsageMap* const m_map;
				};
				CLOAK_CALL UsageMap();
				CLOAK_CALL UsageMap(In const UsageMap& o);
				CLOAK_CALL ~UsageMap();
				void CLOAK_CALL_THIS Add(In size_t pos);
				void CLOAK_CALL_THIS Remove(In size_t pos);
				void CLOAK_CALL_THIS Clear();
				bool CLOAK_CALL_THIS Contains(In size_t pos) const;
				const_iterator CLOAK_CALL_THIS begin() const;
				const_iterator CLOAK_CALL_THIS end() const;
				UsageMap& CLOAK_CALL_THIS operator=(In const UsageMap& o);
			private:
				size_t m_size;
				union {
					uint64_t m_base;
					uint8_t* m_extended;
				};
		};
		class BitMap {
			public:
				class BitBool {
					friend class BitMap;
					public:
						bool CLOAK_CALL_THIS operator=(In bool);
						CLOAK_CALL_THIS operator bool();
						CLOAK_CALL ~BitBool();
					protected:
						CLOAK_CALL BitBool(Inout BYTE& d, In size_t pos);
						CLOAK_CALL BitBool();
					private:
						BYTE* m_data;
						BYTE m_mask;
				};
				CLOAK_CALL_THIS BitMap(In size_t size);
				CLOAK_CALL_THIS BitMap(In const BitMap& bm);
				CLOAK_CALL_THIS ~BitMap();
				size_t CLOAK_CALL_THIS size();
				BitBool CLOAK_CALL_THIS operator[](In size_t a);
				bool CLOAK_CALL_THIS operator[](In size_t a) const;
				bool CLOAK_CALL_THIS isAt(In size_t a) const;
				void CLOAK_CALL_THIS set(In size_t p, In bool s);
				void* CLOAK_CALL operator new(In size_t s);
				void CLOAK_CALL operator delete(In void* ptr);
			private:
				BYTE* m_data;
				size_t m_size;
		};
	}
}

#endif