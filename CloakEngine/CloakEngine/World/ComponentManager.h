#pragma once
#ifndef CE_API_WORLD_COMPONENTMANAGER_H
#define CE_API_WORLD_COMPONENTMANAGER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/World/Entity.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Helper/Types.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Helper/Reflection.h"

#include <iterator>

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace World {
			struct ComponentManager {
				public:
					typedef std::pair<void*, Component*> CacheData;
					CLOAK_INTERFACE ICache : public API::Helper::SavePtr_v1::IBasicPtr{
						public:
							virtual void CLOAK_CALL_THIS GetData(In size_t pos, In const size_t* order, Out CacheData* data) = 0;
							virtual size_t CLOAK_CALL_THIS Size() = 0;
					};
				private:
					static CLOAKENGINE_API ICache* CLOAK_CALL RequestCache(In size_t size, In_reads(size) const type_id* guids, In_reads(size) const bool* readOnly, Out_writes(size) size_t* order);

					template<bool Enable, typename... Cmps> class ComponentGroup {};
					template<typename... Cmps> class ComponentGroup<true, Cmps...> {
						private:
							struct IDHolder {
								type_id ID;
								bool ReadOnly;
							};

							template<const IDHolder*... ids> struct IDList {
								static constexpr type_id ID[] = { ids->ID ... };
								static constexpr bool ReadOnly[] = { ids->ReadOnly ... };
							};
							template<> struct IDList<> {};
							template<typename A> struct IDValue {
								static constexpr IDHolder value = { id_of_type<A>(), std::is_const<A>::value };
							};
							template<typename A> struct IDValue<A*> : public IDValue<A> {};
							template<typename A, typename B> struct IDListAppend {};
							template<const IDHolder*... A, const IDHolder*... B> struct IDListAppend<IDList<A...>, IDList<B...>> {
								typedef typename IDList<A..., B...> type;
							};
							template<const IDHolder*... A> struct IDListAppend<IDList<A...>, IDList<>> {
								typedef typename IDList<A...> type;
							};
							template<typename... A> struct IDListExpand {
								typedef IDList<> type;
							};
							template<typename A, typename... B> struct IDListExpand<A, B...> {
								typedef typename IDListAppend<IDList<&IDValue<A>::value>, typename IDListExpand<B...>::type>::type type;
							};

							template<typename A> struct AdjustConst { typedef A type; };
							template<typename A> struct AdjustConst<volatile A> { typedef typename AdjustConst<A>::type type; };
							template<typename A> struct AdjustConst<A*> { typedef typename AdjustConst<A>::type type; };
							template<typename A> struct AdjustConst<A&> { typedef typename AdjustConst<A>::type type; };
							template<typename C> struct is_valid_type {
								static constexpr bool value = index_of_type<C, Cmps...>::value < number_of_types<Cmps...>::value;
								typedef typename AdjustConst<typename nth_type<index_of_type<C, Cmps...>::value, Cmps...>::type>::type type;
							};
							typedef typename IDListExpand<Cmps...>::type ComponentIDList;
							static constexpr auto& ID = ComponentIDList::ID;
							static constexpr auto& ReadOnly = ComponentIDList::ReadOnly;
							static constexpr size_t ComponentCount = number_of_types<Cmps...>::value;

							ICache* m_cache;
							size_t m_order[ComponentCount];
							size_t m_cacheSize;
						public:
							struct iterator;
							struct value_type;

							struct value_type {
								friend class ComponentGroup;
								friend struct iterator;
								private:
									CacheData m_ptr[ComponentCount];

									CLOAK_CALL value_type() { for (size_t a = 0; a < ARRAYSIZE(m_ptr); a++) { m_ptr[a] = nullptr; } }
									CLOAK_CALL value_type(In ICache* cache, In size_t pos, In_reads(ComponentCount) const size_t* order)
									{
										cache->GetData(pos, order, m_ptr);
										for (size_t a = 0; a < ARRAYSIZE(m_ptr); a++)
										{
											if (m_ptr[a].second != nullptr) { m_ptr[a].second->AddRef(); }
										}
									}
								public:
									CLOAK_CALL value_type(In const value_type& o)
									{
										for (size_t a = 0; a < ARRAYSIZE(m_ptr); a++)
										{
											m_ptr[a] = o.m_ptr[a];
											if (m_ptr[a].second != nullptr) { m_ptr[a].second->AddRef(); }
										}
									}
									CLOAK_CALL ~value_type()
									{
										for (size_t a = 0; a < ARRAYSIZE(m_ptr); a++)
										{
											if (m_ptr[a].second != nullptr) { m_ptr[a].second->Release(); }
										}
									}

									value_type& CLOAK_CALL_THIS operator=(In const value_type& o)
									{
										if (&o == this) { return *this; }
										for (size_t a = 0; a < ARRAYSIZE(m_ptr); a++) 
										{
											if (m_ptr[a].second != nullptr) { m_ptr[a].second->Release(); }
											m_ptr[a] = o.m_ptr[a];
											if (m_ptr[a].second != nullptr) { m_ptr[a].second->AddRef(); }
										}
										return *this;
									}

									template<typename C, std::enable_if_t<is_valid_type<C>::value>* = nullptr> RefPointer<typename is_valid_type<C>::type> CLOAK_CALL_THIS Get() const
									{
										constexpr auto id = index_of_type<C, Cmps...>::value;
										typedef typename is_valid_type<C>::type T;
										CLOAK_ASSUME(id < ARRAYSIZE(m_ptr));
										T* r = reinterpret_cast<T*>(m_ptr[id].first);
										CLOAK_ASSUME(r == dynamic_cast<T*>(m_ptr[id].second));
										return r;
									}
							};
							struct iterator {
								friend class ComponentGroup;
								public:
									typedef std::random_access_iterator_tag iterator_category;
									typedef ComponentGroup::value_type value_type;
									typedef ptrdiff_t difference_type;
									typedef value_type* pointer;
									typedef value_type& reference;
								private:
									const ComponentGroup* m_parent;
									size_t m_pos;
									size_t m_cacheSize;
									value_type m_value;

									CLOAK_CALL iterator(In const ComponentGroup* parent, In size_t pos, In size_t cacheSize) : m_parent(parent), m_pos(pos), m_cacheSize(cacheSize), m_value(parent->m_cache, pos, parent->m_order) {}
								public:
									CLOAK_CALL iterator(In_opt nullptr_t = nullptr) : m_parent(nullptr), m_pos(0), m_cacheSize(0) {}

									iterator& CLOAK_CALL_THIS operator=(In const iterator& o)
									{
										if (&o == this) { return *this; }
										m_parent = o.m_parent;
										m_pos = o.m_pos;
										m_cacheSize = o.m_cacheSize;
										m_value = o.m_value;
										return *this;
									}
									iterator& CLOAK_CALL_THIS operator=(In nullptr_t)
									{
										m_parent = nullptr;
										m_pos = 0;
										m_cacheSize = 0;
										m_value = value_type();
										return *this;
									}

									iterator CLOAK_CALL_THIS operator++(In int)
									{
										iterator r = *this;
										if (m_pos < m_cacheSize)
										{
											m_pos++;
											m_value = value_type(m_parent->m_cache, m_pos, m_parent->m_order);
										}
										return r;
									}
									iterator& CLOAK_CALL_THIS operator++()
									{
										if (m_pos < m_cacheSize)
										{
											m_pos++;
											m_value = value_type(m_parent->m_cache, m_pos, m_parent->m_order);
										}
										return *this;
									}
									iterator CLOAK_CALL_THIS operator--(In int)
									{
										iterator r = *this;
										if (m_pos > 0)
										{
											m_pos--;
											m_value = value_type(m_parent->m_cache, m_pos, m_parent->m_order);
										}
										else
										{
											m_pos = m_cacheSize;
											m_value = value_type();
										}
										return r;
									}
									iterator& CLOAK_CALL_THIS operator--()
									{
										if (m_pos > 0)
										{
											m_pos--;
											m_value = value_type(m_parent->m_cache, m_pos, m_parent->m_order);
										}
										else
										{
											m_pos = m_cacheSize;
											m_value = value_type();
										}
										return *this;
									}
									iterator CLOAK_CALL_THIS operator+(In const long& i) const { return iterator(m_parent, m_pos + i, m_cacheSize); }
									iterator& CLOAK_CALL_THIS operator+=(In const long& i) 
									{
										m_pos += i;
										m_value = value_type(m_parent->m_cache, m_pos, m_parent->m_order);
										return *this;
									}
									iterator CLOAK_CALL_THIS operator-(In const long& i) const { return iterator(m_parent, m_pos - i, m_cacheSize); }
									iterator& CLOAK_CALL_THIS operator-=(In const long& i) 
									{ 
										if (m_pos >= i)
										{
											m_pos -= i;
											m_value = value_type(m_parent->m_cache, m_pos, m_parent->m_order);
										}
										else
										{
											m_pos = m_cacheSize;
											m_value = value_type();
										}
										return *this;
									}
									value_type CLOAK_CALL_THIS operator[](In const long& i) const { return value_type(m_parent->m_cache, m_pos + i, m_parent->m_order); }
									reference CLOAK_CALL_THIS operator*() { return m_value; }
									pointer CLOAK_CALL_THIS operator->() { return m_pos < m_cacheSize ? &m_value : nullptr; }
									bool CLOAK_CALL_THIS operator==(In const iterator& o) const
									{
										if (m_pos >= m_cacheSize) { return o.m_pos >= o.m_cacheSize; }
										return o.m_pos < o.m_cacheSize && m_pos == o.m_pos && m_parent->m_cache == o.m_parent->m_cache;
									}
									bool CLOAK_CALL_THIS operator!=(In const iterator& o) const
									{
										if (m_pos >= m_cacheSize) { return o.m_pos < o.m_cacheSize; }
										return o.m_pos >= o.m_cacheSize || m_pos != o.m_pos || m_parent->m_cache != o.m_parent->m_cache;
									}
									bool CLOAK_CALL_THIS operator<(In const iterator& o) const
									{
										if (m_pos >= m_cacheSize) { return false; }
										return o.m_pos >= o.m_cacheSize || (m_pos < o.m_pos && m_parent->m_cache == o.m_parent->m_cache);
									}
									bool CLOAK_CALL_THIS operator<=(In const iterator& o) const
									{
										if (m_pos >= m_cacheSize) { return o.m_pos >= o.m_cacheSize; }
										return o.m_pos >= o.m_cacheSize || (m_pos <= o.m_pos && m_parent->m_cache == o.m_parent->m_cache);
									}
									bool CLOAK_CALL_THIS operator>(In const iterator& o) const
									{
										if (m_pos >= m_cacheSize) { return o.m_pos < o.m_cacheSize; }
										return o.m_pos < o.m_cacheSize && (m_pos > o.m_pos && m_parent->m_cache == o.m_parent->m_cache);
									}
									bool CLOAK_CALL_THIS operator>=(In const iterator& o) const
									{
										if (m_pos >= m_cacheSize) { return true; }
										return o.m_pos < o.m_cacheSize && (m_pos >= o.m_pos && m_parent->m_cache == o.m_parent->m_cache);
									}
									bool CLOAK_CALL_THIS operator==(In nullptr_t) const { return m_pos >= m_cacheSize; }
									bool CLOAK_CALL_THIS operator!=(In nullptr_t) const { return m_pos < m_cacheSize; }
									bool CLOAK_CALL_THIS operator<(In nullptr_t) const { return m_pos < m_cacheSize; }
									bool CLOAK_CALL_THIS operator<=(In nullptr_t) const { return true; }
									bool CLOAK_CALL_THIS operator>(In nullptr_t) const { return false; }
									bool CLOAK_CALL_THIS operator>=(In nullptr_t) const { return m_pos >= m_cacheSize; }
							};
							typedef std::reverse_iterator<iterator> reverse_iterator;
							typedef size_t size_type;

							CLOAK_CALL ComponentGroup()
							{
								m_cache = RequestCache(ComponentCount, ID, ReadOnly, m_order);
								m_cacheSize = m_cache == nullptr ? 0 : m_cache->Size();
							}
							CLOAK_CALL ComponentGroup(In const ComponentGroup& g)
							{
								m_cache = g.m_cache;
								m_cacheSize = g.m_cacheSize;
								for (size_t a = 0; a < ARRAYSIZE(m_order); a++) { m_order[a] = g.m_order[a]; }
								if (m_cache != nullptr) { m_cache->AddRef(); }
							}
							virtual CLOAK_CALL ~ComponentGroup()
							{
								if (m_cache != nullptr) { m_cache->Release(); }
							}

							ComponentGroup& CLOAK_CALL_THIS operator=(In const ComponentGroup& g)
							{
								if (&g == this) { return *this; }
								SWAP(m_cache, g.m_cache);
								m_cacheSize = g.m_cacheSize;
								for (size_t a = 0; a < ARRAYSIZE(m_order); a++) { m_order[a] = g.m_order[a]; }
								return *this;
							}

							iterator CLOAK_CALL_THIS begin() const { return iterator(this, 0, m_cacheSize); }
							iterator CLOAK_CALL_THIS cbegin() const { return iterator(this, 0, m_cacheSize); }
							reverse_iterator CLOAK_CALL_THIS rbegin() const { return reverse_iterator(iterator(this, m_cacheSize > 0 ? m_cacheSize - 1 : 0, m_cacheSize)); }
							reverse_iterator CLOAK_CALL_THIS crbegin() const { return reverse_iterator(iterator(this, m_cacheSize > 0 ? m_cacheSize - 1 : 0, m_cacheSize)); }
							iterator CLOAK_CALL_THIS end() const { return iterator(this, m_cacheSize, m_cacheSize); }
							iterator CLOAK_CALL_THIS cend() const { return iterator(this, m_cacheSize, m_cacheSize); }
							reverse_iterator CLOAK_CALL_THIS rend() const { return reverse_iterator(iterator(this, m_cacheSize, m_cacheSize)); }
							reverse_iterator CLOAK_CALL_THIS crend() const { return reverse_iterator(iterator(this, m_cacheSize, m_cacheSize)); }

							size_type CLOAK_CALL_THIS size() const { return m_cacheSize; }
							value_type CLOAK_CALL_THIS at(In size_type i) const { return value_type(m_cache, i, m_order); }
							value_type CLOAK_CALL_THIS operator[](In size_type i) const { return value_type(m_cache, i, m_order); }
					};
				public:
					template<typename... C> using Group = ComponentGroup<Predicate::unique_list<C...>::value && Predicate::all_base_of<Component, C...>::value, C...>;
			};
			template<typename... C> using Group = ComponentManager::Group<C...>;
		}
	}
}

#endif