#include "stdafx.h"
#include "CloakEngine/Global/Memory.h"
#include "CloakEngine/Global/Debug.h"
#include "CloakEngine/Helper/Types.h"
#include "Implementation/Global/Memory.h"
#include "Implementation/Global/Task.h"
#include "Implementation/OS.h"

#include <atomic>

//Use direct (aligned) malloc for each allocation request
//#define USE_MALLOC

//Prevent automatic using of memory leak detection (MLD) implementation in debug mode
//#define NO_MLD

//Enforce using memory leak detection (MLD) even without debug mode
//#define USE_MLD

//Prevent automatic use of buffer overrun detection (BOD) in debug mode
//#define NO_BOD

//Enforce using buffer overrun detection (BOD) even without debug mode
//#define USE_BOD

//Prevent automatic use of backup values
#define NO_BACKUP

//Enforce using backup values
//#define USE_BACKUP

//Enforce only using pool allocator for each allocation request (disabled, if USE_MALLOC or ONLY_HEAP is defined)
//#define ONLY_POOL

//Enforce only using heap allocator for each allocation request (disabled, if USE_MALLOC or ONLY_POOL is defined)
//#define ONLY_HEAP

//Prints memory stats on every allocation event:
//#define PRINT_STATS

//Enables thread local cache coherency. This *should* reduce false sharing, as well as thread concurency, for small
//memory allocations ( <= 16 * Cache Line Size) by using thread local allocators with dedicated memory blocks.
#define USE_TLCC


#if (defined(PRINT_STATS) && !defined(_DEBUG))
#undef PRINT_STATS
#endif
//Buffer overrun detection:
#if (defined(_DEBUG) && !defined(NO_BOD)) || defined(USE_BOD)
#ifndef USE_BOD
#define USE_BOD 1
#endif
#include <assert.h>
namespace BOD {
	typedef uint64_t BOD_t;
	constexpr BOD_t START_VALUE = 0xAB64E5A3Ui64;
	constexpr BOD_t END_VALUE = 0xA24BAE34Ui64;
}

#define BOD_DEFINE(Name) BOD::BOD_t Name
#define BOD_SIZE sizeof(BOD::BOD_t)
#define BOD_SET(ptr, value) (*reinterpret_cast<uint64_t*>(ptr) = (value))
#define BOD_SET_START(ptr) BOD_SET(ptr, BOD::START_VALUE)
#define BOD_SET_END(ptr) BOD_SET(ptr, BOD::END_VALUE)
#define BOD_CHECK(ptr, value) CLOAK_ASSUME(*reinterpret_cast<uint64_t*>(ptr) == (value))
#define BOD_CHECK_START(ptr) BOD_CHECK(ptr, BOD::START_VALUE)
#define BOD_CHECK_END(ptr) BOD_CHECK(ptr, BOD::END_VALUE)
#else
#define BOD_DEFINE(Name)
#define BOD_SIZE 0
#define BOD_BLOCK_START_VALUE 0
#define BOD_BLOCK_END_VALUE 0
#define BOD_SET(ptr, value)
#define BOD_SET_START(ptr)
#define BOD_SET_END(ptr)
#define BOD_CHECK(ptr, value)
#define BOD_CHECK_START(ptr)
#define BOD_CHECK_END(ptr)
#endif

typedef std::stringstream StringStream;
typedef std::string String;
void CLOAK_CALL Print(In String s)
{
	s = s + "\n";
	OutputDebugStringA(s.c_str());
}

//Memory leak detection:
#if (defined(_DEBUG) && !defined(NO_MLD)) || defined(USE_MLD)
#ifndef USE_MLD
#define USE_MLD 1
#endif
#include <unordered_map>
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "CloakEngine/Global/Log.h"
#include "Implementation/Global/Game.h"
#pragma push_macro("max")
#undef max

template<typename A> class NormAlloc;
template<> class NormAlloc<void> {
public:
	typedef void* pointer;
	typedef const void* const_pointer;
	typedef void value_type;

	template <typename U> struct rebind { typedef NormAlloc<U> other; };
};
template<typename A> class NormAlloc {
public:
	typedef A value_type;
	typedef A* pointer;
	typedef const A* const_pointer;
	typedef A& reference;
	typedef const A& const_reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef std::true_type propagate_on_container_move_assignment;

	template<typename U> struct rebind { typedef NormAlloc<U> other; };
	CLOAK_CALL NormAlloc() noexcept {}
	template<typename U> CLOAK_CALL NormAlloc(In const NormAlloc<U>& a)noexcept {}

	size_t CLOAK_CALL max_size() const noexcept { return std::numeric_limits<size_t>::max() / sizeof(A); }
	pointer CLOAK_CALL address(In reference x) const noexcept { return std::addressof(x); }
	const_pointer CLOAK_CALL address(In const_reference x) const noexcept { return std::addressof(x); }
	pointer CLOAK_CALL allocate(In size_type n, In_opt typename NormAlloc<void>::const_pointer cvptr = nullptr)
	{
		void* ptr = malloc(n * sizeof(A));
		if (ptr == nullptr) { throw std::bad_alloc(); }
		return reinterpret_cast<pointer>(ptr);
	}
	void CLOAK_CALL deallocate(In pointer p, In size_type n) noexcept { return free(p); }
	template<typename U, typename... Args> void CLOAK_CALL construct(In U* p, In Args&& ... args) { ::new(reinterpret_cast<void*>(p))U(std::forward<Args>(args)...); }
	void CLOAK_CALL destroy(In pointer p) { p->~A(); }
};
template<typename A> class NormAlloc<const A> : public NormAlloc<A> {};
template<typename A, typename B> bool CLOAK_CALL operator==(In const NormAlloc<A>& a, In const NormAlloc<B>& b) { return true; }
template<typename A, typename B> bool CLOAK_CALL operator!=(In const NormAlloc<A>& a, In const NormAlloc<B>& b) { return false; }

template<typename T> String toString(In T val)
{
	static_assert(std::is_integral<T>::value, "No integral");
	using UT = std::make_unsigned_t<T>;
	char buf[21];
	char* const end = std::end(buf);
	char* next = end;
	auto UVal = static_cast<UT>(val);
	if (val < 0)
	{
		next = std::_UIntegral_to_buff(next, 0 - UVal);
		*--next = '-';
	}
	else { next = std::_UIntegral_to_buff(next, UVal); }
	return String(next, end);
}

template<typename T> String to_hex(In T t)
{
	StringStream s;
	s << "0x" << std::setfill('0') << std::setw(2 * sizeof(T)) << std::hex << t;
	return s.str();
}

class MLDTable {
	private:
		std::unordered_map<uintptr_t, size_t, std::hash<uintptr_t>, std::equal_to<uintptr_t>, NormAlloc<std::pair<const uintptr_t, size_t>>> m_table;
	public:
		void CLOAK_CALL push(In size_t size, In uintptr_t ptr)
		{
			CLOAK_ASSUME(m_table.find(ptr) == m_table.end());
			m_table[ptr] = size;

			for (auto a : m_table) { CLOAK_ASSUME(ptr + size <= a.first || a.first + a.second <= ptr || a.first == ptr); }
		}
		template<typename T> void CLOAK_CALL push(In size_t size, In T* ptr) { push(size, reinterpret_cast<uintptr_t>(ptr)); }
		void CLOAK_CALL pop(In size_t size, In uintptr_t ptr)
		{
			auto& it = m_table.find(ptr);
			CLOAK_ASSUME(it != m_table.end());
			CLOAK_ASSUME(it->second >= size);
			if (it->second == size) { m_table.erase(it); }
			else { it->second -= size; }
		}
		template<typename T> void CLOAK_CALL pop(In size_t size, In T* ptr) { pop(size, reinterpret_cast<uintptr_t>(ptr)); }
		void CLOAK_CALL resize(In size_t size, In uintptr_t ptr)
		{
			auto& it = m_table.find(ptr);
			CLOAK_ASSUME(it != m_table.end());
			it->second = size;

			for (auto a : m_table) { CLOAK_ASSUME(ptr + size <= a.first || a.first + a.second <= ptr || a.first == ptr); }
		}
		template<typename T> void CLOAK_CALL resize(In size_t size, In T* ptr) { resize(size, reinterpret_cast<uintptr_t>(ptr)); }
		void CLOAK_CALL check()
		{
			for (const auto& it : m_table)
			{
				if (it.second > 0)
				{
					Print("\t\t" + toString(it.second) + " Bytes at " + to_hex(it.first));
				}
			}
		}
		bool CLOAK_CALL hasAny() { return m_table.size() > 0; }
};

#define MLD_ALLOC(size, ptr) m_table.push(size,ptr)
#define MLD_REALLOC(size, ptr) m_table.resize(size,ptr)
#define MLD_FREE(size, ptr) m_table.pop(size,ptr)
#define MLD_DEFINE() MLDTable m_table
#define MLD_HASANY() bool CLOAK_CALL HasAnyMLD(){return m_table.hasAny();}
#define MLD_CHECK() void CLOAK_CALL CheckMLD(){m_table.check();} MLD_HASANY()

#pragma pop_macro("max")
#else
#define MLD_ALLOC(size, ptr)
#define MLD_REALLOC(size, ptr) 
#define MLD_FREE(size, ptr)
#define MLD_DEFINE()
#define MLD_HASANY()
#define MLD_CHECK()
#endif

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Global {
			namespace Memory {
				typedef size_t Alignment;
				constexpr Alignment DEFAULT_ALIGNMENT = 16;

				uint32_t g_cacheBorder = 0;
				uint32_t g_cacheLineSize = 0;

				template<typename A> constexpr CLOAK_FORCEINLINE A CLOAK_CALL Align(In A size, In_opt Alignment align = DEFAULT_ALIGNMENT)
				{
					const Alignment m = align - 1;
					CLOAK_ASSUME((align & m) == 0); //given alignment must be a power of 2
					return static_cast<A>(size + m) & ~static_cast<A>(m);
				}
				constexpr inline size_t CLOAK_CALL AlignAdjust(In uintptr_t ptr, In size_t headSize, In_opt Alignment align = DEFAULT_ALIGNMENT)
				{
					const Alignment m = align - 1;
					CLOAK_ASSUME((align & m) == 0); //given alignment must be a power of 2
					const size_t adj = static_cast<size_t>(align - (ptr & m)) & static_cast<size_t>(m);
					if (headSize > adj) { return adj + Align(headSize - adj, align); }
					return adj;
				}
				template<typename A> constexpr CLOAK_FORCEINLINE size_t CLOAK_CALL AlignAdjust(In A* ptr, In size_t headSize, In_opt Alignment align = DEFAULT_ALIGNMENT) { return AlignAdjust(reinterpret_cast<uintptr_t>(ptr), headSize, align); }
				template<typename A, typename B> CLOAK_FORCEINLINE A* CLOAK_CALL Move(In A* ptr, In B s) { return reinterpret_cast<A*>(reinterpret_cast<uintptr_t>(ptr) + s); }

				template<typename T> struct BackupWrapper {
					T m_first;
					T m_second;
					
					CLOAK_CALL BackupWrapper() : m_first(), m_second() {}
					CLOAK_CALL BackupWrapper(In const T& o) : m_first(o), m_second(o) {}

					bool CLOAK_CALL_THIS operator==(In const BackupWrapper<T>& o) const { return m_first == o.m_first; }
					bool CLOAK_CALL_THIS operator==(In const T& t) const { return m_first == t; }
					friend inline bool CLOAK_CALL operator==(In const T& t, In const BackupWrapper<T>& o) { return o == t; }

					CLOAK_CALL_THIS operator T() { return m_first; }
					BackupWrapper<T>& CLOAK_CALL_THIS operator=(In const T& t)
					{
						m_first = t;
						m_second = t;
						return *this;
					}
					T CLOAK_CALL_THIS operator->() { return m_first; }

					BackupWrapper<T>& CLOAK_CALL_THIS operator+=(In const T& o) { m_first += o; m_second += o; return *this; }
					BackupWrapper<T>& CLOAK_CALL_THIS operator-=(In const T& o) { m_first -= o; m_second -= o; return *this; }
					BackupWrapper<T>& CLOAK_CALL_THIS operator*=(In const T& o) { m_first *= o; m_second *= o; return *this; }
					BackupWrapper<T>& CLOAK_CALL_THIS operator/=(In const T& o) { m_first /= o; m_second /= o; return *this; }
					T CLOAK_CALL_THIS operator+(In const T& o) const { return m_first + o; }
					T CLOAK_CALL_THIS operator-(In const T& o) const { return m_first - o; }
					T CLOAK_CALL_THIS operator*(In const T& o) const { return m_first * o; }
					T CLOAK_CALL_THIS operator/(In const T& o) const { return m_first / o; }
				};
				template<typename T> struct Backup {
#if (defined(_DEBUG) && !defined(NO_BACKUP)) || defined(USE_BACKUP)
					typedef BackupWrapper<T> type;
#else
					typedef T type;
#endif
				};

				//Base classes
				class Lock;
				class Alloc {
					friend class Lock;
					private:
						SRWLOCK m_cs;
					public:
						CLOAK_CALL Alloc() : m_cs(SRWLOCK_INIT) { }
						CLOAK_CALL ~Alloc() { }
						virtual void* CLOAK_CALL_THIS Allocate(In size_t s) = 0;
				};
				class Lock {
					private:
						SRWLOCK * const m_cs;
					public:
						CLOAK_CALL Lock(In Alloc* alloc) : m_cs(&alloc->m_cs) { AcquireSRWLockExclusive(m_cs); }
						CLOAK_CALL ~Lock() { ReleaseSRWLockExclusive(m_cs); }
						static void CLOAK_CALL StaticLock(In Alloc* alloc) { AcquireSRWLockExclusive(&alloc->m_cs); }
						static void CLOAK_CALL StaticUnlock(In Alloc* alloc) { ReleaseSRWLockExclusive(&alloc->m_cs); }
						static void CLOAK_CALL StaticReadLock(In Alloc* alloc) { AcquireSRWLockShared(&alloc->m_cs); }
						static void CLOAK_CALL StaticReadUnlock(In Alloc* alloc) { ReleaseSRWLockShared(&alloc->m_cs); }
				};
				class DynamicAlloc : public Alloc {
					public:
						virtual void CLOAK_CALL_THIS Free(In void* ptr) = 0;
						virtual void* CLOAK_CALL_THIS Reallocate(In void* ptr, In size_t size, In bool moveData) = 0;
						virtual void* CLOAK_CALL_THIS TryReallocateNoFree(In void* ptr, In size_t size) = 0;
						virtual MemoryStats CLOAK_CALL_THIS GetStats() const = 0;
				};
				class RBTree {
					protected:
						static constexpr uint32_t BLACK = 0;
						static constexpr uint32_t RED = 1;
					public:
						struct TreeHead {
#ifdef _DEBUG
							Backup<uint32_t>::type Color;
							Backup<uint32_t>::type Size;
#else
							uint32_t Color : 1;
							uint32_t Size : 31;
#endif
							struct {
								Backup<TreeHead*>::type Left; //Smaller Block
								Backup<TreeHead*>::type Right; //Larger Block
								Backup<TreeHead*>::type Parent;
							} Tree;
						};
						struct ListTreeHead : public TreeHead {
							struct {
								Backup<ListTreeHead*>::type Next;
								Backup<ListTreeHead*>::type Previous;
							} List;
						};
					private:
						static CLOAK_FORCEINLINE TreeHead* CLOAK_CALL Grandparent(In TreeHead* h)
						{
							TreeHead* p = h->Tree.Parent;
							if (p != nullptr) { return p->Tree.Parent; }
							return nullptr;
						}
						static CLOAK_FORCEINLINE TreeHead* CLOAK_CALL Sibling(In TreeHead* h)
						{
							TreeHead* p = h->Tree.Parent;
							if (p != nullptr) { return h == p->Tree.Left ? p->Tree.Right : p->Tree.Left; }
							return nullptr;
						}
						static CLOAK_FORCEINLINE TreeHead* CLOAK_CALL Uncle(In TreeHead* h)
						{
							TreeHead* p = h->Tree.Parent;
							if (p != nullptr) { return Sibling(p); }
							return nullptr;
						}
						static CLOAK_FORCEINLINE void CLOAK_CALL ReplaceNode(In TreeHead* n, In TreeHead* c)
						{
							TreeHead* p = n->Tree.Parent;
							if (n == p->Tree.Left) { p->Tree.Left = c; }
							else { p->Tree.Right = c; }
							if (c != nullptr) { c->Tree.Parent = p; }
						}
						static CLOAK_FORCEINLINE void CLOAK_CALL RotateLeft(In TreeHead* h, Inout TreeHead** root)
						{
							TreeHead* n = h->Tree.Right;
							TreeHead* p = h->Tree.Parent;
							CLOAK_ASSUME(n != nullptr);
							h->Tree.Right = n->Tree.Left;
							n->Tree.Left = h;
							h->Tree.Parent = n;

							if (h->Tree.Right != nullptr) { h->Tree.Right->Tree.Parent = h; }
							if (p != nullptr)
							{
								if (p->Tree.Left == h) { p->Tree.Left = n; }
								else { p->Tree.Right = n; }
							}
							n->Tree.Parent = p;
							if (*root == h) { *root = n; }
						}
						static CLOAK_FORCEINLINE void CLOAK_CALL RotateRight(In TreeHead* h, Inout TreeHead** root)
						{
							TreeHead* n = h->Tree.Left;
							TreeHead* p = h->Tree.Parent;
							CLOAK_ASSUME(n != nullptr);
							h->Tree.Left = n->Tree.Right;
							n->Tree.Right = h;
							h->Tree.Parent = n;
							if (h->Tree.Left != nullptr) { h->Tree.Left->Tree.Parent = h; }
							if (p != nullptr)
							{
								if (p->Tree.Left == h) { p->Tree.Left = n; }
								else { p->Tree.Right = n; }
							}
							n->Tree.Parent = p;
							if (*root == h) { *root = n; }
						}
						static CLOAK_FORCEINLINE void CLOAK_CALL SwapNodes(In TreeHead* a, In TreeHead* b, Inout TreeHead** root)
						{
							std::swap(a->Tree.Parent, b->Tree.Parent);
							std::swap(a->Tree.Left, b->Tree.Left);
							std::swap(a->Tree.Right, b->Tree.Right);
							//Fix if a and b were closely related:
							if (a->Tree.Parent == a) { a->Tree.Parent = b; }
							if (b->Tree.Parent == b) { b->Tree.Parent = a; }
							if (a->Tree.Left == a) { a->Tree.Left = b; }
							if (b->Tree.Left == b) { b->Tree.Left = a; }
							if (a->Tree.Right == a) { a->Tree.Right = b; }
							if (b->Tree.Right == b) { b->Tree.Right = a; }

							if (a->Tree.Left != nullptr) { a->Tree.Left->Tree.Parent = a; }
							if (a->Tree.Right != nullptr) { a->Tree.Right->Tree.Parent = a; }
							if (b->Tree.Left != nullptr) { b->Tree.Left->Tree.Parent = b; }
							if (b->Tree.Right != nullptr) { b->Tree.Right->Tree.Parent = b; }
							if (a->Tree.Parent != nullptr)
							{
								if (a->Tree.Parent->Tree.Left == b) { a->Tree.Parent->Tree.Left = a; }
								else if (a->Tree.Parent->Tree.Right == b) { a->Tree.Parent->Tree.Right = a; } //We need additional test, as the parent might already have changed
							}
							if (b->Tree.Parent != nullptr)
							{
								if (b->Tree.Parent->Tree.Left == a) { b->Tree.Parent->Tree.Left = b; }
								else if (b->Tree.Parent->Tree.Right == a) { b->Tree.Parent->Tree.Right = b; } //We need additional test, as the parent might already have changed
							}
							auto tc = a->Color;
							a->Color = b->Color;
							b->Color = tc;

							if (*root == a) { *root = b; }
							else if (*root == b) { *root = a; }
						}
					protected:
						static inline void CLOAK_CALL RemoveNode(In TreeHead* h, Inout Backup<TreeHead*>::type* root)
						{
							CLOAK_ASSUME(root != nullptr);
							TreeHead* rr = *root;
							if (h->Tree.Left != nullptr && h->Tree.Right != nullptr)
							{
								TreeHead* c = h->Tree.Right;
								while (c->Tree.Left != nullptr) { c = c->Tree.Left; }
								SwapNodes(h, c, &rr);
							}
							CLOAK_ASSUME(h->Tree.Left == nullptr || h->Tree.Right == nullptr);
							CLOAK_ASSUME(h->Tree.Parent == nullptr || h->Tree.Parent->Tree.Left == h || h->Tree.Parent->Tree.Right == h);
							TreeHead* const c = h->Tree.Left == nullptr ? h->Tree.Right : h->Tree.Left;
							TreeHead* const td = h;
							//rebalance tree:
							if (h == rr)
							{
								rr = c;
								if (c != nullptr) { c->Tree.Parent = nullptr; c->Color = BLACK; }
							}
							else
							{
								if (h->Color == BLACK)
								{
								rb_remove_rebalance:
									if (h != rr)
									{
										CLOAK_ASSUME(h->Tree.Parent != nullptr);
										TreeHead * s = Sibling(h);
										CLOAK_ASSUME(s != nullptr); //h is a black node, and black nodes allways have siblings
										if (s->Color == RED)
										{
											h->Tree.Parent->Color = RED;
											s->Color = BLACK;
											if (h == h->Tree.Parent->Tree.Left) { RotateLeft(h->Tree.Parent, &rr); }
											else { RotateRight(h->Tree.Parent, &rr); }
											s = Sibling(h);
											CLOAK_ASSUME(s != nullptr); //h is a black node, and black nodes allways have siblings
										}
										CLOAK_ASSUME(s->Color == BLACK); //The above rotation ensures (as a side effect) that our current sibling is black, because red nodes can't have red children
										if ((s->Tree.Left == nullptr || s->Tree.Left->Color == BLACK) && (s->Tree.Right == nullptr || s->Tree.Right->Color == BLACK))
										{
											if (h->Tree.Parent->Color == BLACK)
											{
												s->Color = RED;
												h = h->Tree.Parent;
												goto rb_remove_rebalance;
											}
											else
											{
												s->Color = RED;
												h->Tree.Parent->Color = BLACK;
											}
										}
										else
										{
											if (h == h->Tree.Parent->Tree.Left && (s->Tree.Right == nullptr || s->Tree.Right->Color == BLACK) && (s->Tree.Left != nullptr && s->Tree.Left->Color == RED))
											{
												s->Color = RED;
												s->Tree.Left->Color = BLACK;
												RotateRight(s, &rr);
												s = Sibling(h);
												CLOAK_ASSUME(s != nullptr); //h is a black node, and black nodes allways have siblings
											}
											else if (h == h->Tree.Parent->Tree.Right && (s->Tree.Left == nullptr || s->Tree.Left->Color == BLACK) && (s->Tree.Right != nullptr && s->Tree.Right->Color == RED))
											{
												s->Color = RED;
												s->Tree.Right->Color = BLACK;
												RotateLeft(s, &rr);
												s = Sibling(h);
												CLOAK_ASSUME(s != nullptr); //h is a black node, and black nodes allways have siblings
											}
											CLOAK_ASSUME(s->Color == BLACK);
											s->Color = h->Tree.Parent->Color;
											h->Tree.Parent->Color = BLACK;
											if (h == h->Tree.Parent->Tree.Left)
											{
												CLOAK_ASSUME(s->Tree.Right != nullptr);
												s->Tree.Right->Color = BLACK;
												RotateLeft(h->Tree.Parent, &rr);
											}
											else
											{
												CLOAK_ASSUME(s->Tree.Left != nullptr);
												s->Tree.Left->Color = BLACK;
												RotateRight(h->Tree.Parent, &rr);
											}
										}
									}
								}
								ReplaceNode(td, c); //Finally remove node, replacing it with it's child
							}
							*root = rr;
						}
						static inline void CLOAK_CALL RemoveNode(In ListTreeHead* h, Inout Backup<TreeHead*>::type* root, Inout Backup<ListTreeHead*>::type* listStart)
						{
							CLOAK_ASSUME(listStart != nullptr);
							if (h == *listStart) { *listStart = h->List.Next; }
							if (h->List.Previous != nullptr) { h->List.Previous->List.Next = h->List.Next; }
							if (h->List.Next != nullptr) { h->List.Next->List.Previous = h->List.Previous; }
							RemoveNode(static_cast<TreeHead*>(h), root);
						}
						static inline void CLOAK_CALL InsertNode(In TreeHead* h, Inout Backup<TreeHead*>::type* root)
						{
							CLOAK_ASSUME(root != nullptr);
							TreeHead* rr = *root;
							//Insert into binary tree:
							h->Tree.Parent = nullptr;
							h->Tree.Left = nullptr;
							h->Tree.Right = nullptr;
							h->Color = RED;
							if (rr == nullptr) { rr = h; }
							else
							{
								TreeHead* r = rr;
								while (true)
								{
									if (h->Size <= r->Size)
									{
										if (r->Tree.Left != nullptr) { r = r->Tree.Left; }
										else
										{
											r->Tree.Left = h;
											h->Tree.Parent = r;
											break;
										}
									}
									else
									{
										if (r->Tree.Right != nullptr) { r = r->Tree.Right; }
										else
										{
											r->Tree.Right = h;
											h->Tree.Parent = r;
											break;
										}
									}
								}
							}
							//Rebalance:
						rb_insert_rebalance:
							if (h->Tree.Parent == nullptr) { h->Color = BLACK; }
							else if (h->Tree.Parent->Color == RED)
							{
								TreeHead* g = Grandparent(h);
								TreeHead* u = Uncle(h);
								if (u != nullptr && u->Color == RED)
								{
									h->Tree.Parent->Color = BLACK;
									u->Color = BLACK;
									g->Color = RED;
									h = g;
									//Continue to loop
									goto rb_insert_rebalance;
								}
								else
								{
									if (h == h->Tree.Parent->Tree.Right && h->Tree.Parent == g->Tree.Left) { RotateLeft(h->Tree.Parent, &rr); h = h->Tree.Left; }
									else if (h == h->Tree.Parent->Tree.Left && h->Tree.Parent == g->Tree.Right) { RotateRight(h->Tree.Parent, &rr); h = h->Tree.Right; }
									TreeHead* p = h->Tree.Parent;
									g = p->Tree.Parent;
									if (h == h->Tree.Parent->Tree.Left) { RotateRight(g, &rr); }
									else { RotateLeft(g, &rr); }
									g->Color = RED;
									p->Color = BLACK;
								}
							}
							*root = rr;
						}
						static inline void CLOAK_CALL InsertNode(In ListTreeHead* h, Inout Backup<TreeHead*>::type* root, Inout Backup<ListTreeHead*>::type* listStart)
						{
							CLOAK_ASSUME(listStart != nullptr);
							ListTreeHead* lr = *listStart;
							h->List.Next = lr;
							h->List.Previous = nullptr;
							if (lr != nullptr) { lr->List.Previous = h; }
							*listStart = h;
							InsertNode(static_cast<TreeHead*>(h), root);
						}
						static inline TreeHead* CLOAK_CALL Find(In uint32_t size, In TreeHead* root)
						{
							if (root == nullptr) { return nullptr; }
							TreeHead* bestFit = nullptr;
							TreeHead* cur = root;
							do {
								if (cur->Size >= size) { bestFit = cur; cur = cur->Tree.Left; }
								else { cur = cur->Tree.Right; }
							} while (cur != nullptr);
							return bestFit;
						}
						static inline std::pair<ListTreeHead*, ListTreeHead*> CLOAK_CALL FindAdjacent(In void* pos, In size_t size, In ListTreeHead* listStart)
						{
							const uintptr_t hp = reinterpret_cast<uintptr_t>(pos);
							ListTreeHead* l = nullptr;
							ListTreeHead* r = nullptr;
							while (listStart != nullptr && (l == nullptr || r == nullptr))
							{
								const uintptr_t s = reinterpret_cast<uintptr_t>(listStart);
								if (s + listStart->Size == hp) { l = listStart; }
								else if (hp + size == s) { r = listStart; }
								listStart = listStart->List.Next;
							}
							return std::make_pair(l, r);
						}
						static inline std::pair<ListTreeHead*, ListTreeHead*> CLOAK_CALL FindAdjacent(In ListTreeHead* h, In ListTreeHead* listStart) { return FindAdjacent(h, h->Size, listStart); }
					public:
						CLOAK_CALL RBTree() {}
				};

				//Primary Memory manager
				class LinearAllocator : public Alloc, private RBTree {
					private:
						struct Header {
							Backup<Header*>::type Next;
							Backup<size_t>::type Size;
						};
					public:
						static constexpr size_t MAX_ALLOC = 256 MB;
						static constexpr size_t MIN_ALLOC = 1 KB;
						static constexpr size_t HEADER_SIZE = sizeof(Header);
					private:
						Backup<Header*>::type m_first;
						Backup<Header*>::type m_last;
						Backup<TreeHead*>::type m_root;

						Header* CLOAK_CALL_THIS CreateBlock()
						{
							const size_t blockSize = Align(MAX_ALLOC + HEADER_SIZE, g_cacheLineSize);
							uintptr_t r = reinterpret_cast<uintptr_t>(malloc(blockSize));
							CLOAK_ASSUME(r != 0);
							size_t adj = AlignAdjust(r, sizeof(Header), g_cacheLineSize);
							Header* h = reinterpret_cast<Header*>(r);
							h->Next = nullptr;
							h->Size = blockSize - adj;
							CLOAK_ASSUME(blockSize >= h->Size);
							CLOAK_ASSUME(((r + blockSize - h->Size) & (g_cacheLineSize - 1)) == 0);
							return h;
						}
					public:
						CLOAK_CALL LinearAllocator()
						{
							m_first = nullptr;
							m_last = nullptr;
							m_root = nullptr;
						}
						CLOAK_CALL ~LinearAllocator()
						{

						}
						void* CLOAK_CALL_THIS Allocate(In size_t s) override
						{
							if (s == 0) { return nullptr; }
							CLOAK_ASSUME(g_cacheLineSize > 0);
							const size_t blockSize = Align(MAX_ALLOC + HEADER_SIZE, g_cacheLineSize);
							s = Align(s, g_cacheLineSize);
							CLOAK_ASSUME(s <= MAX_ALLOC);
							Lock lock(this);
							TreeHead* h = Find(static_cast<uint32_t>(s), m_root);
							if (h == nullptr)
							{
								Header* b = CreateBlock();
								if (m_last != nullptr) { m_last->Next = b; }
								else { m_first = b; }
								m_last = b;
								h = reinterpret_cast<TreeHead*>(Move(b, blockSize - b->Size));
								h->Size = b->Size;
							}
							else { RemoveNode(h, &m_root); }
							//Cut off unused part:
							if (h->Size >= s + MIN_ALLOC)
							{
								TreeHead* nh = Move(h, s);
								nh->Size = h->Size - s;
								InsertNode(nh, &m_root);
							}
							return h;
						}
						void CLOAK_CALL Release()
						{
							Lock lock(this);
							Header* r = m_first;
							while (r != nullptr) 
							{
								Header* n = r->Next;
								free(r);
								r = n;
							}
							m_first = m_last = nullptr;
						}
				};
				class TreeAllocator : public DynamicAlloc, public RBTree {
					private:
						struct Block {
							Backup<DynamicAlloc*>::type Owner;
							Backup<uint32_t>::type Size;
						};
						static constexpr size_t BLOCK_SIZE = Align(sizeof(Block) + BOD_SIZE);
						static constexpr size_t MIN_ALLOC = max(BLOCK_SIZE + BOD_SIZE, sizeof(ListTreeHead));
						static constexpr size_t MIN_BLOCK_SIZE = max(BLOCK_SIZE + DEFAULT_ALIGNMENT + BOD_SIZE, sizeof(ListTreeHead));

						template<size_t S, size_t M = S, bool ReqLock = true> class ListTree : public DynamicAlloc {
							public:
								static constexpr size_t PAGE_SIZE = S;
								static constexpr size_t MAX_ALLOC = min(M, S) - BLOCK_SIZE;
							private:
								Backup<ListTreeHead*>::type m_list;
								Backup<TreeHead*>::type m_root;
								Alloc* const m_alloc;
								std::atomic<uint64_t> m_available;
								std::atomic<uint64_t> m_used;
							protected:
								ListTreeHead* CLOAK_CALL_THIS CreateBlock()
								{
									ListTreeHead* h = reinterpret_cast<ListTreeHead*>(m_alloc->Allocate(PAGE_SIZE));
									h->Color = BLACK;
									h->Size = PAGE_SIZE;
									h->Tree.Left = nullptr;
									h->Tree.Right = nullptr;
									h->Tree.Parent = nullptr;
									h->List.Next = nullptr;
									h->List.Previous = nullptr;
									m_available += PAGE_SIZE;
#ifdef PRINT_STATS
									Print("Memory Heap requested new page of size " + std::to_string(PAGE_SIZE));
#endif
									return h;
								}
							public:
								CLOAK_CALL ListTree(In Alloc* alloc) : m_alloc(alloc), m_list(nullptr), m_root(nullptr), m_available(0), m_used(0) {}
								CLOAK_CALL ListTree(In Alloc* alloc, In void* startMem, In size_t startMemSize) : ListTree(alloc)
								{
									const size_t adj = AlignAdjust(startMem, 0);
									ListTreeHead* h = reinterpret_cast<ListTreeHead*>(Move(startMem, adj));
									h->Color = BLACK;
									h->Size = startMemSize - adj;
									h->Tree.Left = nullptr;
									h->Tree.Right = nullptr;
									h->Tree.Parent = nullptr;
									h->List.Next = nullptr;
									h->List.Previous = nullptr;
									m_available = startMemSize;
									InsertNode(h, &m_root, &m_list);
								}
								//Returns Block*
								void* CLOAK_CALL_THIS Allocate(In size_t s) override
								{
									if constexpr (ReqLock == true) { Lock::StaticLock(this); }
									else { Lock::StaticReadLock(this); }
									ListTreeHead* bestFit = reinterpret_cast<ListTreeHead*>(Find(static_cast<uint32_t>(s), m_root));
									if (bestFit == nullptr) { bestFit = CreateBlock(); }
									else { RemoveNode(bestFit, &m_root, &m_list); }

									uintptr_t r = reinterpret_cast<uintptr_t>(bestFit);

									//Cut off parts of the best fit block:
									if (bestFit->Size >= s + MIN_BLOCK_SIZE)
									{
										ListTreeHead* nh = reinterpret_cast<ListTreeHead*>(r + s);
										nh->Size = bestFit->Size - s;
										bestFit->Size = s;
										InsertNode(nh, &m_root, &m_list);
									}
									const uint32_t fs = bestFit->Size;

									Block* rb = reinterpret_cast<Block*>(r);
									rb->Size = fs;
									rb->Owner = this;
									if constexpr (ReqLock == true) { Lock::StaticUnlock(this); }
									else { Lock::StaticReadUnlock(this); }
									m_used += fs;
									return rb;
								}
								//Gets Block*, returns Block*
								void* CLOAK_CALL_THIS TryReallocateNoFree(In void* ptr, In size_t ns) override
								{
									Block* b = reinterpret_cast<Block*>(ptr);
									CLOAK_ASSUME(b->Owner == this);
									const uint32_t size = b->Size;

									//Check if memory block is large enouth for resize:
									const uint32_t nrs = static_cast<uint32_t>(Align(max(ns + BLOCK_SIZE + BOD_SIZE, MIN_ALLOC)));
									if (nrs > MAX_ALLOC) { return nullptr; }
									if (size >= nrs) { return ptr; }

									Lock lock(this);
									//Find left and right adjacent blocks:
									auto f = RBTree::FindAdjacent(b, size, m_list);
									uint32_t fhs = size;
									if (f.second != nullptr && f.second->Size + fhs >= nrs)
									{
										RemoveNode(f.second, &m_root, &m_list);
										b->Size = fhs + f.second->Size;
										//Cut off unused part
										if (b->Size >= nrs + MIN_BLOCK_SIZE)
										{
											ListTreeHead* nh = reinterpret_cast<ListTreeHead*>(Move(b, nrs));
											nh->Size = b->Size - nrs;
											b->Size = nrs;
											InsertNode(nh, &m_root, &m_list);
										}
										m_used += b->Size - size;
										return ptr;
									}
									return nullptr;
								}
								//Gets Block*, returns Block*
								void* CLOAK_CALL_THIS Reallocate(In void* ptr, In size_t ns, In bool moveData) override
								{
									Block* b = reinterpret_cast<Block*>(ptr);
									CLOAK_ASSUME(b->Owner == this);
									const uint32_t size = b->Size;

									//Check if memory block is large enouth for resize:
									const uint32_t nrs = static_cast<uint32_t>(Align(max(ns + BLOCK_SIZE + BOD_SIZE, MIN_ALLOC)));
									if (nrs > MAX_ALLOC) { return nullptr; }
									if (size >= nrs) { return ptr; }

									Lock lock(this);
									//Find left and right adjacent blocks:
									auto f = RBTree::FindAdjacent(b, size, m_list);
									uint32_t fhs = size;
									if (f.second != nullptr)
									{
										RemoveNode(f.second, &m_root, &m_list);
										if (f.second->Size + fhs >= nrs)
										{
											b->Size = fhs + f.second->Size;
											//Cut off unused part
											if (b->Size >= nrs + MIN_BLOCK_SIZE)
											{
												ListTreeHead* nh = reinterpret_cast<ListTreeHead*>(Move(b, nrs));
												nh->Size = b->Size - nrs;
												b->Size = nrs;
												InsertNode(nh, &m_root, &m_list);
											}
											m_used += b->Size - size;
											return ptr;
										}
										fhs += f.second->Size;
									}
									if (f.first != nullptr)
									{
										RemoveNode(f.first, &m_root, &m_list);
										b = reinterpret_cast<Block*>(f.first);
										if (f.first->Size + fhs >= nrs)
										{
											const uintptr_t np = reinterpret_cast<uintptr_t>(f.first);
											b->Size = fhs + f.first->Size;
											b->Owner = this;
											//Move payload data:
											if (moveData == true)
											{
												const uint8_t* src = reinterpret_cast<uint8_t*>(Move(b, BLOCK_SIZE));
												uint8_t* dst = reinterpret_cast<uint8_t*>(np + BLOCK_SIZE);
												for (size_t a = 0; a < size - (BLOCK_SIZE + BOD_SIZE); a++) { dst[a] = src[a]; }
											}
											//Cut off unused part
											if (b->Size >= nrs + MIN_BLOCK_SIZE)
											{
												ListTreeHead* nh = reinterpret_cast<ListTreeHead*>(np + nrs);
												nh->Size = b->Size - nrs;
												b->Size = nrs;
												InsertNode(nh, &m_root, &m_list);
											}
											m_used += b->Size - size;
											return b;
										}
										fhs += f.first->Size;
										b->Size = fhs;
										b->Owner = this;
									}
									//merge could not result in large enouth block, allocate new one:
									ListTreeHead* s = reinterpret_cast<ListTreeHead*>(b);
									void* np = nullptr;
									if (moveData == true) 
									{
										np = Allocate(ns);
										//Move payload data:
										memcpy(Move(np, BLOCK_SIZE), Move(ptr, BLOCK_SIZE), size - (BLOCK_SIZE + BOD_SIZE));
										//Free old block:
										s->Size = fhs;
										InsertNode(s, &m_root, &m_list);
									}
									else
									{
										//Free old block:
										s->Size = fhs;
										InsertNode(s, &m_root, &m_list);
										//Create new one
										np = Allocate(ns);
									}
									return np;
								}
								//Gets Block*
								void CLOAK_CALL_THIS Free(In void* ptr) override
								{
									Block* b = reinterpret_cast<Block*>(ptr);
									CLOAK_ASSUME(b->Owner == this);
									const size_t size = b->Size;

									Lock lock(this);
									//Find left and right adjacent blocks:
									ListTreeHead* s = reinterpret_cast<ListTreeHead*>(b);
									s->Size = size;
									auto f = RBTree::FindAdjacent(s, m_list);

									//Merge with adjacent blocks:
									if (f.second != nullptr)
									{
										RemoveNode(f.second, &m_root, &m_list);
										s->Size += f.second->Size;
									}
									if (f.first != nullptr)
									{
										RemoveNode(f.first, &m_root, &m_list);
										f.first->Size += s->Size;
										s = f.first;
									}
									InsertNode(s, &m_root, &m_list);
									m_used -= size;
								}
								MemoryStats CLOAK_CALL_THIS GetStats() const override
								{
									MemoryStats r;
									r.Used = m_used;
									r.Available = m_available;
									return r;
								}
						};

#ifdef USE_TLCC
						typedef ListTree<512KB, 128KB, false> CacheCoherentAllocator;
#endif
						typedef ListTree<32MB> GlobalAllocator;
					public:
						static constexpr size_t MAX_ALLOC = GlobalAllocator::MAX_ALLOC;
					private:
						GlobalAllocator m_bigList;
#ifdef USE_TLCC
						DWORD m_storageID;
#endif
						Alloc* const m_alloc;
						MLD_DEFINE();
					public:
						CLOAK_CALL TreeAllocator(In Alloc* alloc) : m_bigList(alloc), m_alloc(alloc) 
						{
#ifdef USE_TLCC
							m_storageID = TlsAlloc(); 
#endif
						}
						CLOAK_CALL ~TreeAllocator() 
						{
#ifdef USE_TLCC
							TlsFree(m_storageID); 
#endif
						}
						void* CLOAK_CALL_THIS Allocate(In size_t s) override
						{
							if (s == 0) { return nullptr; }
							s = Align(max(s + BLOCK_SIZE + BOD_SIZE, MIN_ALLOC));
							CLOAK_ASSUME(s <= MAX_ALLOC);

							Block* rb = nullptr;
#ifdef USE_TLCC
							if (s <= g_cacheBorder && s <= CacheCoherentAllocator::MAX_ALLOC)
							{
								CacheCoherentAllocator* l = reinterpret_cast<CacheCoherentAllocator*>(TlsGetValue(m_storageID));
								if (l == nullptr)
								{
									void* p = m_alloc->Allocate(CacheCoherentAllocator::PAGE_SIZE);
									l = ::new(p)CacheCoherentAllocator(m_alloc, Move(p, sizeof(CacheCoherentAllocator)), CacheCoherentAllocator::PAGE_SIZE - sizeof(CacheCoherentAllocator));
#ifdef PRINT_STATS
									Print("Memory Heap requested new page of size " + std::to_string(CacheCoherentAllocator::PAGE_SIZE));
#endif
									TlsSetValue(m_storageID, l);
								}
								rb = reinterpret_cast<Block*>(l->Allocate(s));
#ifdef PRINT_STATS
								MemoryStats ms = l->GetStats();
								Print("Memory Heap Current Thread: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available));
#endif
							}
							else
							{
#endif
								rb = reinterpret_cast<Block*>(m_bigList.Allocate(s));
#ifdef PRINT_STATS
								MemoryStats ms = m_bigList.GetStats();
								Print("Memory Heap: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available));
#endif
#ifdef USE_TLCC
							}
#endif

							const uintptr_t r = reinterpret_cast<uintptr_t>(rb);
#ifdef USE_MLD
							Lock lock(this);
							MLD_ALLOC(rb->Size, r);
#endif
							BOD_SET_START(r + sizeof(Block));
							BOD_SET_END(r + rb->Size - BOD_SIZE);
							CLOAK_ASSUME(rb->Size >= s);
							CLOAK_ASSUME(rb->Size - s < MIN_BLOCK_SIZE);
							return reinterpret_cast<void*>(r + BLOCK_SIZE);
						}
						void* CLOAK_CALL_THIS TryReallocateNoFree(In void* ptr, In size_t ns) override
						{
							if (ptr == nullptr || ns == 0) { return nullptr; }
							const uintptr_t r = reinterpret_cast<uintptr_t>(ptr) - BLOCK_SIZE;
							CLOAK_ASSUME(r + BLOCK_SIZE > BLOCK_SIZE);
							Block* b = reinterpret_cast<Block*>(r);
							const size_t size = b->Size;
							BOD_CHECK_START(r + sizeof(Block));
							BOD_CHECK_END(r + size - BOD_SIZE);

							void* res = b->Owner->TryReallocateNoFree(b, ns);
#ifdef PRINT_STATS
							MemoryStats ms = b->Owner->GetStats();
							if (b->Owner == &m_bigList) { Print("Memory Heap: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
							else { Print("Memory Heap Current Thread: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
#endif
							if (res != nullptr) { return Move(res, BLOCK_SIZE); }
							return nullptr;
						}
						void* CLOAK_CALL_THIS Reallocate(In void* ptr, In size_t ns, In bool moveData) override
						{
							if (ptr == nullptr) { return Allocate(ns); }
							if (ns == 0) { Free(ptr); return nullptr; }
							const uintptr_t r = reinterpret_cast<uintptr_t>(ptr) - BLOCK_SIZE;
							CLOAK_ASSUME(r + BLOCK_SIZE > BLOCK_SIZE);
							Block* b = reinterpret_cast<Block*>(r);
							const size_t size = b->Size;
							BOD_CHECK_START(r + sizeof(Block));
							BOD_CHECK_END(r + size - BOD_SIZE);

							Block* nb = reinterpret_cast<Block*>(b->Owner->Reallocate(b, ns, moveData));
#ifdef USE_TLCC
							if (nb == nullptr && b->Owner != &m_bigList)
							{
								nb = reinterpret_cast<Block*>(m_bigList.Allocate(ns));
								if (moveData == true) { memcpy(Move(nb, BLOCK_SIZE), Move(b, BLOCK_SIZE), size); }
								b->Owner->Free(b);
							}
#endif
#ifdef PRINT_STATS
							MemoryStats ms = b->Owner->GetStats();
							if (b->Owner == &m_bigList) { Print("Memory Heap: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
							else { Print("Memory Heap Current Thread: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
#endif

							CLOAK_ASSUME(nb != nullptr);
							uintptr_t np = reinterpret_cast<uintptr_t>(nb);
#ifdef USE_MLD
							Lock lock(this);
							if (b == nb) { MLD_REALLOC(nb->Size, nb); }
							else { MLD_FREE(size, b); MLD_ALLOC(nb->Size, nb); }
#endif
							BOD_SET_START(np + sizeof(Block));
							BOD_SET_END(np + nb->Size - BOD_SIZE);
							return reinterpret_cast<void*>(np + BLOCK_SIZE);
						}
						void CLOAK_CALL_THIS Free(In void* ptr) override
						{
							if (ptr == nullptr) { return; }
							uintptr_t r = reinterpret_cast<uintptr_t>(ptr) - BLOCK_SIZE;
							CLOAK_ASSUME(r + BLOCK_SIZE > BLOCK_SIZE);
							Block* b = reinterpret_cast<Block*>(r);
							const size_t size = b->Size;

							BOD_CHECK_START(r + sizeof(Block));
							BOD_CHECK_END(r + size - BOD_SIZE);
#ifdef USE_MLD
							Lock lock(this);
							MLD_FREE(size, r);
#endif
#ifdef PRINT_STATS
							const DynamicAlloc* own = b->Owner;
#endif
							b->Owner->Free(b);
#ifdef PRINT_STATS
							MemoryStats ms = own->GetStats();
							if (own == &m_bigList) { Print("Memory Heap: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
							else { Print("Memory Heap Current Thread: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
#endif
						}
						MemoryStats CLOAK_CALL_THIS GetStats() const override
						{
							MemoryStats ms = m_bigList.GetStats();
#ifdef USE_TLCC
							CacheCoherentAllocator* l = reinterpret_cast<CacheCoherentAllocator*>(TlsGetValue(m_storageID));
							if (l != nullptr)
							{
								MemoryStats ls = l->GetStats();
								ms.Available += ls.Available;
								ms.Used += ls.Used;
							}
#endif
							return ms;
						}
						MLD_CHECK()
				};
				class PoolAllocator : public DynamicAlloc {
					private:
						static constexpr size_t POOL_SIZES[] = { 16, 32, 64, 128, 256, 512, 1 KB, 2 KB, 4 KB, 8 KB };
					public:
						static constexpr size_t PAGE_SIZE = 32 KB;
					private:
						struct Head {
							Backup<DynamicAlloc*>::type Alloc;
							Backup<size_t>::type Size;
						};
						class BlockAllocator : public DynamicAlloc {
							private:
								struct Block {
									Backup<Block*>::type Next;
								};

								static constexpr size_t BLOCK_SIZE = Align(sizeof(Block));

								Alloc* const m_alloc;
								const size_t m_elementSize;
								const size_t m_pageSize;
								const bool m_shouldLock;
								Block* m_free;

								inline Block* CreateBlocks(In void* ptr, In size_t size)
								{
									CLOAK_ASSUME(Align(reinterpret_cast<uintptr_t>(ptr)) == reinterpret_cast<uintptr_t>(ptr));
									if (size < m_elementSize) { return nullptr; }
									Block* res = reinterpret_cast<Block*>(ptr);
									Block* p = nullptr;
									for (size_t a = 0; a < size; a += m_elementSize)
									{
										Block* n = reinterpret_cast<Block*>(Move(ptr, a));
										if (p != nullptr) { p->Next = n; }
										p = n;
									}
									if (p != nullptr) { p->Next = nullptr; }
									return res;
								}
							public:
								CLOAK_CALL BlockAllocator(In Alloc* alloc, In size_t elementSize, In bool shouldLock) : m_elementSize(max(Align(elementSize), BLOCK_SIZE)), m_shouldLock(shouldLock), m_free(nullptr), m_alloc(alloc), m_pageSize(elementSize * (PAGE_SIZE / elementSize)) {}
								CLOAK_CALL BlockAllocator(In Alloc* alloc, In size_t elementSize, In bool shouldLock, In void* startMemory, In size_t startMemorySize) : BlockAllocator(alloc, elementSize, shouldLock)
								{
									m_free = CreateBlocks(startMemory, startMemorySize);
								}
								void* CLOAK_CALL_THIS Allocate(In size_t s) override
								{
									CLOAK_ASSUME(s <= m_elementSize);
									if (m_shouldLock == true) { Lock::StaticLock(this); }
									else { Lock::StaticReadLock(this); }
									if (m_free == nullptr) 
									{ 
										m_free = CreateBlocks(m_alloc->Allocate(m_pageSize), m_pageSize); 
#ifdef PRINT_STATS
										Print("Memory Pool requested new page of size " + std::to_string(m_pageSize));
#endif
									}
									Block* res = m_free;
									m_free = m_free->Next;
									if (m_shouldLock == true) { Lock::StaticUnlock(this); }
									else { Lock::StaticReadUnlock(this); }
									return res;
								}
								void CLOAK_CALL_THIS Free(In void* ptr) override
								{
									Lock::StaticLock(this);
									Block* res = reinterpret_cast<Block*>(ptr);
									res->Next = m_free;
									m_free = res;
									Lock::StaticUnlock(this);
								}
								void* CLOAK_CALL_THIS Reallocate(In void* ptr, In size_t size, In bool moveData) override
								{
									if (size <= m_elementSize) { return ptr; }
									return nullptr;
								}
								void* CLOAK_CALL_THIS TryReallocateNoFree(In void* ptr, In size_t size) override
								{
									if (size <= m_elementSize) { return ptr; }
									return nullptr;
								}
								MemoryStats CLOAK_CALL_THIS GetStats() const override { return { 0,0 }; } //Not implemented
						};
						struct AllocList {
							BlockAllocator* Allocator[ARRAYSIZE(POOL_SIZES)];
						};
						static constexpr size_t ALLOC_LIST_SIZE = Align(sizeof(AllocList));
						static constexpr size_t BLOCK_ALLOCATOR_SIZE = Align(sizeof(BlockAllocator));
					public:
						static constexpr size_t HEAD_SIZE = Align(sizeof(Head));
					private:
						Alloc* const m_alloc;
						TreeAllocator* const m_poolFree;
						AllocList m_list;
						DWORD m_storageID;
						std::atomic<uint64_t> m_used;
						std::atomic<uint64_t> m_available;
						MLD_DEFINE();

						inline size_t CLOAK_CALL_THIS FindPoolIndex(In size_t size)
						{
							size_t l = 0;
							size_t r = ARRAYSIZE(POOL_SIZES);
							while (true)
							{
								const size_t h = (l + r) / 2;
								if (h == l) { return r; }
								if (size > POOL_SIZES[h]) { l = h; }
								else if (size == POOL_SIZES[h]) { return h; }
								else { r = h; }
							}
							return ARRAYSIZE(POOL_SIZES);
						}
					public:
						static constexpr size_t MAX_ALLOC = TreeAllocator::MAX_ALLOC - HEAD_SIZE;
						CLOAK_CALL PoolAllocator(In Alloc* alloc, In TreeAllocator* poolFree) : m_alloc(alloc), m_poolFree(poolFree), m_used(0), m_available(0)
						{
							for (size_t a = 0; a < ARRAYSIZE(m_list.Allocator); a++) { m_list.Allocator[a] = nullptr; }
#ifdef USE_TLCC
							m_storageID = TlsAlloc();
#endif
						}
						CLOAK_CALL ~PoolAllocator()
						{
#ifdef USE_TLCC
							TlsFree(m_storageID);
#endif
						}
						void* CLOAK_CALL_THIS Allocate(In size_t s) override
						{
							if (s == 0) { return nullptr; }
							const size_t i = FindPoolIndex(s);
							DynamicAlloc* alloc = nullptr;
							size_t fullSize = HEAD_SIZE + s;
							if (i >= ARRAYSIZE(POOL_SIZES)) { alloc = m_poolFree; }
							else
							{
								fullSize = HEAD_SIZE + POOL_SIZES[i];
#ifdef USE_TLCC
								if (fullSize <= g_cacheBorder)
								{
									AllocList* al = reinterpret_cast<AllocList*>(TlsGetValue(m_storageID));
									if (al == nullptr)
									{
										const size_t elemCount = (PAGE_SIZE - (ALLOC_LIST_SIZE + BLOCK_ALLOCATOR_SIZE)) / fullSize;
										const size_t usageSize = fullSize * elemCount;
										const size_t pageSize = ALLOC_LIST_SIZE + BLOCK_ALLOCATOR_SIZE + usageSize;
										al = reinterpret_cast<AllocList*>(m_alloc->Allocate(pageSize));
#ifdef PRINT_STATS
										Print("Memory Pool requested new page of size " + std::to_string(pageSize));
#endif
										for (size_t a = 0; a < ARRAYSIZE(al->Allocator); a++) { al->Allocator[a] = nullptr; }
										al->Allocator[i] = ::new(Move(al, ALLOC_LIST_SIZE))BlockAllocator(m_alloc, fullSize, false, Move(al, ALLOC_LIST_SIZE + BLOCK_ALLOCATOR_SIZE), usageSize);
										TlsSetValue(m_storageID, al);
										m_available += usageSize;
									}
									else if (al->Allocator[i] == nullptr)
									{
										const size_t elemCount = (PAGE_SIZE - BLOCK_ALLOCATOR_SIZE) / fullSize;
										const size_t usageSize = fullSize * elemCount;
										const size_t pageSize = BLOCK_ALLOCATOR_SIZE + usageSize;
										void* r = m_alloc->Allocate(pageSize);
#ifdef PRINT_STATS
										Print("Memory Pool requested new page of size " + std::to_string(pageSize));
#endif
										al->Allocator[i] = ::new(r)BlockAllocator(m_alloc, fullSize, false, Move(r, BLOCK_ALLOCATOR_SIZE), usageSize);
										m_available += usageSize;
									}
									alloc = al->Allocator[i];
								}
								else
								{
#endif
									if (m_list.Allocator[i] == nullptr)
									{
										const size_t elemCount = (PAGE_SIZE - BLOCK_ALLOCATOR_SIZE) / fullSize;
										const size_t usageSize = fullSize * elemCount;
										const size_t pageSize = BLOCK_ALLOCATOR_SIZE + usageSize;
										void* r = m_alloc->Allocate(pageSize);
#ifdef PRINT_STATS
										Print("Memory Pool requested new page of size " + std::to_string(pageSize));
#endif
										m_list.Allocator[i] = ::new(r)BlockAllocator(m_alloc, fullSize, false, Move(r, BLOCK_ALLOCATOR_SIZE), usageSize);
										m_available += usageSize;
									}
									alloc = m_list.Allocator[i];
#ifdef USE_TLCC
								}
#endif
							}
							Head* res = reinterpret_cast<Head*>(alloc->Allocate(fullSize));
							res->Alloc = alloc;
							res->Size = fullSize;
							m_used += fullSize;
#ifdef PRINT_STATS
							MemoryStats ms = GetStats();
							if (alloc == m_poolFree) { Print("Memory Pool: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
							else { Print("Memory Pool Current Thread: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
#endif
#ifdef USE_MLD
							{
								Lock lock(this);
								MLD_ALLOC(fullSize, reinterpret_cast<uintptr_t>(res));
							}
#endif
							return Move(res, HEAD_SIZE);
						}
						void* CLOAK_CALL_THIS TryReallocateNoFree(In void* ptr, In size_t ns) override
						{
							if (ptr == nullptr || ns == 0) { return nullptr; }
							Head* head = reinterpret_cast<Head*>(reinterpret_cast<uintptr_t>(ptr) - HEAD_SIZE);
							void* r = head->Alloc->TryReallocateNoFree(head, ns);
							if (r != nullptr) { return Move(r, HEAD_SIZE); }
							return nullptr;
						}
						void* CLOAK_CALL_THIS Reallocate(In void* ptr, In size_t size, In bool moveData) override
						{
							if (ptr == nullptr) { return Allocate(size); }
							if (size == 0) { Free(ptr); return nullptr; }
							Head* head = reinterpret_cast<Head*>(reinterpret_cast<uintptr_t>(ptr) - HEAD_SIZE);
							const size_t oldSize = head->Size;
							DynamicAlloc* alloc = head->Alloc;
							void* res = alloc->Reallocate(head, HEAD_SIZE + size, moveData);
							if (res == nullptr)
							{
								m_used -= oldSize;
#ifdef USE_MLD
								{
									Lock lock(this);
									MLD_FREE(oldSize, reinterpret_cast<uintptr_t>(head));
								}
#endif
								if (moveData == true)
								{
									res = Allocate(size);
									memcpy(res, ptr, oldSize - HEAD_SIZE);
									alloc->Free(head);
								}
								else
								{
									alloc->Free(head);
									res = Allocate(size);
								}
#ifdef PRINT_STATS
								MemoryStats ms = GetStats();
								if (alloc == m_poolFree) { Print("Memory Pool: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
								else { Print("Memory Pool Current Thread: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
#endif
								return res;
							}
							else if (alloc == m_poolFree)
							{
#ifdef USE_MLD
								{
									Lock lock(this);
									if (head == res) { MLD_REALLOC(HEAD_SIZE + size, reinterpret_cast<uintptr_t>(res)); }
									else
									{
										MLD_FREE(oldSize, reinterpret_cast<uintptr_t>(res));
										MLD_ALLOC(HEAD_SIZE + size, reinterpret_cast<uintptr_t>(res));
									}
								}
								head = reinterpret_cast<Head*>(res);
								head->Size = HEAD_SIZE + size;
								head->Alloc = alloc;
#endif
							}
							else { CLOAK_ASSUME(res == head); }
							m_used += head->Size - oldSize;
#ifdef PRINT_STATS
							MemoryStats ms = GetStats();
							if (alloc == m_poolFree) { Print("Memory Pool: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
							else { Print("Memory Pool Current Thread: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
#endif
							return Move(head, HEAD_SIZE);
						}
						void CLOAK_CALL_THIS Free(In void* ptr) override
						{
							if (ptr == nullptr) { return; }
							Head* head = reinterpret_cast<Head*>(reinterpret_cast<uintptr_t>(ptr) - HEAD_SIZE);
#ifdef USE_MLD
							{
								Lock lock(this);
								MLD_FREE(head->Size, reinterpret_cast<uintptr_t>(head));
							}
#endif
#ifdef PRINT_STATS
							DynamicAlloc* alloc = head->Alloc;
#endif
							m_used -= head->Size;
							head->Alloc->Free(head);
#ifdef PRINT_STATS
							MemoryStats ms = GetStats();
							if (alloc == m_poolFree) { Print("Memory Pool: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
							else { Print("Memory Pool Current Thread: " + std::to_string(ms.Used) + " / " + std::to_string(ms.Available)); }
#endif
						}
						MemoryStats CLOAK_CALL_THIS GetStats() const override 
						{ 
							MemoryStats r;
							r.Used = m_used;
							r.Available = m_available;
							return r;
						}
						MLD_CHECK()
				};
				class PageStackManager : public DynamicAlloc {
					private:
						static constexpr uint32_t SMALL_BLOCK_SIZE = 4 MB;
						static constexpr uint32_t LARGE_BLOCK_SIZE = 64 MB;

						struct Header {
							Header* Next;
						};

						Alloc* const m_alloc;
						Backup<Header*>::type m_small;
						Backup<Header*>::type m_large;
					public:
						struct Page {
							Backup<uint32_t>::type Size;
							Backup<uint32_t>::type Offset;
							Backup<Page*>::type Next;
						};
						struct StackHeader {
							Backup<size_t>::type Size;
						};

						static constexpr size_t PAGE_HEAD_SIZE = Align(sizeof(Page));
						static constexpr size_t STACK_HEAD_SIZE = Align(sizeof(StackHeader));
						static constexpr size_t MAX_ALLOC = LARGE_BLOCK_SIZE - (STACK_HEAD_SIZE + PAGE_HEAD_SIZE);
						static constexpr size_t SMALL_PAGE_SIZE = SMALL_BLOCK_SIZE - PAGE_HEAD_SIZE;
						static constexpr size_t LARGE_PAGE_SIZE = LARGE_BLOCK_SIZE - PAGE_HEAD_SIZE;
					private:
						Header* CLOAK_CALL_THIS CreateSmallBlock()
						{
							Header* h = reinterpret_cast<Header*>(m_alloc->Allocate(SMALL_BLOCK_SIZE));
#ifdef PRINT_STATS
							Print("Memory Page Stack requested new page of size " + std::to_string(SMALL_BLOCK_SIZE));
#endif
							h->Next = nullptr;
							return h;
						}
						Header* CLOAK_CALL_THIS CreateLargeBlock()
						{
							Header* h = reinterpret_cast<Header*>(m_alloc->Allocate(LARGE_BLOCK_SIZE));
#ifdef PRINT_STATS
							Print("Memory Page Stack requested new page of size " + std::to_string(LARGE_BLOCK_SIZE));
#endif
							h->Next = nullptr;
							return h;
						}
					public:
						CLOAK_CALL PageStackManager(In Alloc* alloc) : m_alloc(alloc), m_small(nullptr), m_large(nullptr) {}
						void* CLOAK_CALL_THIS Allocate(In size_t s) override
						{
							CLOAK_ASSUME(s > 0);
							Lock lock(this);
							if (s <= SMALL_PAGE_SIZE && (m_small != nullptr || m_large == nullptr))
							{
								Header* h = m_small;
								if (h == nullptr) { h = CreateSmallBlock(); }
								m_small = h->Next;
								Page* res = reinterpret_cast<Page*>(h);
								res->Next = nullptr;
								res->Offset = 0;
								res->Size = SMALL_PAGE_SIZE;
								return res;
							}
							CLOAK_ASSUME(s <= LARGE_PAGE_SIZE);
							Header* h = m_large;
							if (h == nullptr) { h = CreateLargeBlock(); }
							m_large = h->Next;
							Page* res = reinterpret_cast<Page*>(h);
							res->Next = nullptr;
							res->Offset = 0;
							res->Size = LARGE_PAGE_SIZE;
							return res;
						}
						void* CLOAK_CALL_THIS TryReallocateNoFree(In void* ptr, In size_t ns) override
						{
							//Not implemented
							CLOAK_ASSUME(false);
							return nullptr;
						}
						void* CLOAK_CALL_THIS Reallocate(In void* ptr, In size_t size, In bool moveData) override
						{
							//Not implemented
							CLOAK_ASSUME(false);
							return nullptr;
						}
						void CLOAK_CALL_THIS Free(In void* ptr) override
						{
							CLOAK_ASSUME(ptr != nullptr);
							Page * res = reinterpret_cast<Page*>(ptr);
							Lock lock(this);
							if (res->Size == SMALL_PAGE_SIZE)
							{
								Header* h = reinterpret_cast<Header*>(ptr);
								h->Next = m_small;
								m_small = h;
							}
							else
							{
								Header* h = reinterpret_cast<Header*>(ptr);
								h->Next = m_large;
								m_large = h;
							}
						}
						MemoryStats CLOAK_CALL_THIS GetStats() const override { return { 0,0 }; } //TODO: Currently not implemented
				};
				class ThreadLocalAllocator : public DynamicAlloc {
					private:
						static constexpr size_t PAGE_SIZE = LinearAllocator::MIN_ALLOC;

						struct GlobalHeader {
							std::atomic<uint64_t> Allocations;
							Backup<uint32_t>::type ID;
							Backup<GlobalHeader*>::type Next;
						};
						class LocalAllocator : RBTree {
							private:
								struct Entry : public TreeHead {
									Backup<uint64_t>::type AllocationState;
									void* Payload;
								};
								
								static constexpr size_t ENTRY_SIZE = sizeof(Entry);
								static constexpr size_t ENTRY_COUNT = (LinearAllocator::MIN_ALLOC + ENTRY_SIZE - 1) / ENTRY_SIZE;
								static constexpr size_t ENTRY_PAGE_SIZE = ENTRY_SIZE * ENTRY_COUNT;

								SRWLOCK m_srw;
								Backup<TreeHead*>::type m_root;
								Alloc* const m_alloc;

								Entry* CLOAK_CALL_THIS CreateEntry()
								{
									Entry* r = reinterpret_cast<Entry*>(m_alloc->Allocate(ENTRY_PAGE_SIZE));
									Entry* n = r;
									for (size_t a = 1; a < ENTRY_COUNT; a++)
									{
										n = Move(n, ENTRY_SIZE);
										n->Size = 0;
										n->AllocationState = 0;
										n->Payload = nullptr;
										InsertNode(n, &m_root);
									}
									return r;
								}
							public:
								LocalAllocator* Next;
								CLOAK_CALL LocalAllocator(In Alloc* alloc, In void* startMemory, In size_t startMemorySize) : m_alloc(alloc), m_srw(SRWLOCK_INIT), m_root(nullptr)
								{
									if (startMemory != nullptr)
									{
										uintptr_t p = reinterpret_cast<uintptr_t>(startMemory);
										const size_t adj = AlignAdjust(p, 0, std::alignment_of<Entry>::value);
										size_t off = adj;
										size_t no = off + ENTRY_SIZE;
										while (no <= startMemorySize)
										{
											Entry* n = reinterpret_cast<Entry*>(p + off);
											n->Size = 0;
											n->AllocationState = 0;
											n->Payload = nullptr;
											InsertNode(n, &m_root);
											off = no;
											no += ENTRY_SIZE;
										}
									}
								}
								void** CLOAK_CALL_THIS Get(In const GlobalHeader* header)
								{
									const uint64_t als = header->Allocations.load(std::memory_order_consume);
									const uint32_t id = header->ID + 1;
									CLOAK_ASSUME(id > 0);
									AcquireSRWLockExclusive(&m_srw);
									Entry* e = reinterpret_cast<Entry*>(Find(id, m_root));
									if (e == nullptr || e->Size != id)
									{
										e = reinterpret_cast<Entry*>(Find(0, m_root));
										if (e == nullptr || e->Size != 0) { e = CreateEntry(); }
										else { RemoveNode(e, &m_root); }
										CLOAK_ASSUME(e != nullptr);
										e->Size = id;
										e->Payload = nullptr;
										e->AllocationState = als;
										InsertNode(e, &m_root);
									}
									else if (e->AllocationState != als) 
									{ 
										e->Payload = nullptr;
										e->AllocationState = als;
									}
									ReleaseSRWLockExclusive(&m_srw);
									return &e->Payload;
								}
								void CLOAK_CALL_THIS Free(In const GlobalHeader* header, In std::function<void(void*)> func)
								{
									const uint64_t als = header->Allocations.load(std::memory_order_consume);
									const uint32_t id = header->ID + 1;
									CLOAK_ASSUME(id > 0);
									AcquireSRWLockExclusive(&m_srw);
									Entry* e = reinterpret_cast<Entry*>(Find(id, m_root));
									if (e->Size == id && e->AllocationState == als) { func(e->Payload); }
									ReleaseSRWLockExclusive(&m_srw);
								}
						};

						static constexpr size_t GLOBAL_ELEMENT_SIZE = sizeof(GlobalHeader);
						static constexpr size_t GLOBAL_ELEMENT_COUNT = (LinearAllocator::MIN_ALLOC + GLOBAL_ELEMENT_SIZE - 1) / GLOBAL_ELEMENT_SIZE;
						static constexpr size_t GLOBAL_PAGE_SIZE = GLOBAL_ELEMENT_SIZE * GLOBAL_ELEMENT_COUNT;

						static constexpr size_t LOCAL_ELEMENT_SIZE = sizeof(LocalAllocator);
						static constexpr size_t LOCAL_PAGE_SIZE = max(LOCAL_ELEMENT_SIZE, LinearAllocator::MIN_ALLOC);
						static constexpr size_t LOCAL_REM_SIZE = LOCAL_PAGE_SIZE - LOCAL_ELEMENT_SIZE;

						DWORD m_storageID;
						GlobalHeader* m_freeGlobal;
						LocalAllocator* m_locals;
						uint32_t m_ID;
						MLD_DEFINE();
						Alloc* const m_alloc;

						GlobalHeader* CLOAK_CALL_THIS CreateGlobalBlock()
						{
							GlobalHeader* r = reinterpret_cast<GlobalHeader*>(m_alloc->Allocate(GLOBAL_PAGE_SIZE));
							GlobalHeader* n = r;
							GlobalHeader* l = nullptr;
							for (size_t a = 0; a < GLOBAL_ELEMENT_COUNT; a++)
							{
								if (l != nullptr) { l->Next = n; }
								n->Allocations = 0;
								n->ID = m_ID++;
								l = n;
								n = Move(n, GLOBAL_ELEMENT_SIZE);
							}
							if (l != nullptr) { l->Next = nullptr; }
							return r;
						}
					public:
						CLOAK_CALL ThreadLocalAllocator(In Alloc* alloc) : m_alloc(alloc)
						{
							m_storageID = TlsAlloc();
							m_freeGlobal = nullptr;
							m_locals = nullptr;
							m_ID = 0;
						}
						CLOAK_CALL ~ThreadLocalAllocator() { TlsFree(m_storageID); }
						void* CLOAK_CALL_THIS Allocate(In size_t s) override
						{
							Lock lock(this);
							if (m_freeGlobal == nullptr) { m_freeGlobal = CreateGlobalBlock(); }
							CLOAK_ASSUME(m_freeGlobal != nullptr);
							GlobalHeader* h = m_freeGlobal;
							MLD_ALLOC(sizeof(void*), h);
							m_freeGlobal = m_freeGlobal->Next;
							return h;
						}
						void* CLOAK_CALL_THIS TryReallocateNoFree(In void* ptr, In size_t ns) override
						{
							//Not implemented
							CLOAK_ASSUME(false);
							return nullptr;
						}
						void* CLOAK_CALL_THIS Reallocate(In void* ptr, In size_t size, In bool moveData) override
						{
							//Not implemented
							CLOAK_ASSUME(false);
							return nullptr;
						}
						void CLOAK_CALL_THIS Free(In void* ptr) override
						{
							Lock lock(this);
							GlobalHeader* h = reinterpret_cast<GlobalHeader*>(ptr);
							MLD_FREE(sizeof(void*), h);
							h->Allocations.fetch_add(1, std::memory_order_acq_rel);
							h->Next = m_freeGlobal;
							m_freeGlobal = h;
						}
						void CLOAK_CALL_THIS Free(In void* ptr, In std::function<void(void*)> func)
						{
							Lock lock(this);
							GlobalHeader* h = reinterpret_cast<GlobalHeader*>(ptr);
							LocalAllocator* la = m_locals;
							while (la != nullptr)
							{
								la->Free(h, func);
								la = la->Next;
							}
							MLD_FREE(sizeof(void*), h);
							h->Allocations.fetch_add(1, std::memory_order_acq_rel);
							h->Next = m_freeGlobal;
							m_freeGlobal = h;
						}
						void** CLOAK_CALL_THIS Get(In void* ptr)
						{
							GlobalHeader* h = reinterpret_cast<GlobalHeader*>(ptr);
							LocalAllocator* a = reinterpret_cast<LocalAllocator*>(TlsGetValue(m_storageID));
							if (a == nullptr)
							{
								//We create one allocator per page and use the rest of that page as entries of that allocator. This
								//will allow way better cache behaviour then creating a array of allocators in the same page
								Lock lock(this);
								void* p = m_alloc->Allocate(LOCAL_PAGE_SIZE);
								a = ::new(p)LocalAllocator(m_alloc, Move(p, LOCAL_ELEMENT_SIZE), LOCAL_REM_SIZE);
								a->Next = m_locals;
								m_locals = a;
								TlsSetValue(m_storageID, a);
							}
							return a->Get(h);
						}
						MemoryStats CLOAK_CALL_THIS GetStats() const override { return { 0,0 }; } //TODO: Currently not implemented
						MLD_CHECK();
				};

				LinearAllocator g_mainAlloc;
				TreeAllocator g_heapAlloc(&g_mainAlloc);
				PoolAllocator g_poolAlloc(&g_mainAlloc, &g_heapAlloc);
				PageStackManager g_pageStack(&g_mainAlloc);
				ThreadLocalAllocator g_threadLocal(&g_mainAlloc);

#ifdef USE_MALLOC
				struct MalHead {
					size_t Adjust;
				};
				inline void* CLOAK_CALL AllocateMalloc(In size_t s)
				{
					if (s == 0) { return nullptr; }
					const size_t size = s + sizeof(MalHead) + MASK;
					uintptr_t adr = reinterpret_cast<uintptr_t>(malloc(size));
					size_t adj = AlignAdjust(adr, sizeof(MalHead));
					adr = adr + adj;
					MalHead* head = reinterpret_cast<MalHead*>(adr - sizeof(MalHead));
					head->Adjust = adj;
					return reinterpret_cast<void*>(adr);
				}
				inline void CLOAK_CALL FreeMalloc(In void* ptr)
				{
					if (ptr != nullptr)
					{
						uintptr_t adr = reinterpret_cast<uintptr_t>(ptr);
						MalHead* head = reinterpret_cast<MalHead*>(adr - sizeof(MalHead));
						void* p = reinterpret_cast<void*>(adr - head->Adjust);
						free(p);
					}
				}
				inline void* CLOAK_CALL ReallocateMalloc(In void* ptr, In size_t s, In bool moveData)
				{
					if (ptr == nullptr) { return AllocateMalloc(s); }
					if (s == 0) { FreeMalloc(ptr); return nullptr; }
					if (moveData == false)
					{
						FreeMalloc(ptr);
						return AllocateMalloc(s);
					}
					uintptr_t adr = reinterpret_cast<uintptr_t>(ptr);
					MalHead* head = reinterpret_cast<MalHead*>(adr - sizeof(MalHead));
					void* p = reinterpret_cast<void*>(adr - head->Adjust);
					const size_t size = s + sizeof(MalHead) + MASK;
					adr = reinterpret_cast<uintptr_t>(realloc(p, size));
					if (adr == 0) { FreeMalloc(ptr); return nullptr; }
					size_t adj = AlignAdjust(adr, sizeof(MalHead));
					adr = adr + adj;
					head = reinterpret_cast<MalHead*>(adr - sizeof(MalHead));
					head->Adjust = adj;
					return reinterpret_cast<void*>(adr);
				}
#endif

				void* CLOAK_CALL MemoryPool::Allocate(In size_t size)
				{
#ifdef USE_MALLOC
					return AllocateMalloc(size);
#elif defined(ONLY_HEAP) && !defined(ONLY_POOL)
					return g_heapAlloc.Allocate(size);
#else
					return g_poolAlloc.Allocate(size);
#endif
				}
				void* CLOAK_CALL MemoryPool::TryResizeBlock(In void* ptr, In size_t size)
				{
#ifdef USE_MALLOC
					return nullptr;
#elif defined(ONLY_HEAP) && !defined(ONLY_POOL)
					return g_heapAlloc.TryReallocateNoFree(ptr, size);
#else
					return g_poolAlloc.TryReallocateNoFree(ptr, size);
#endif
				}
				void* CLOAK_CALL MemoryPool::Reallocate(In void* ptr, In size_t size, In_opt bool moveData)
				{
#ifdef USE_MALLOC
					return ReallocateMalloc(ptr, size, moveData);
#elif defined(ONLY_HEAP) && !defined(ONLY_POOL)
					return g_heapAlloc.Reallocate(ptr, size, moveData);
#else
					return g_poolAlloc.Reallocate(ptr, size, moveData);
#endif
				}
				void CLOAK_CALL MemoryPool::Free(In void* ptr)
				{
#ifdef USE_MALLOC
					FreeMalloc(ptr);
#elif defined(ONLY_HEAP) && !defined(ONLY_POOL)
					g_heapAlloc.Free(ptr);
#else
					g_poolAlloc.Free(ptr);
#endif
				}
				size_t CLOAK_CALL MemoryPool::GetMaxAlloc() 
				{
#if defined(ONLY_HEAP) && !defined(ONLY_POOL)
					return TreeAllocator::MAX_ALLOC;
#else
					return PoolAllocator::MAX_ALLOC; 
#endif
				}

				void* CLOAK_CALL MemoryHeap::Allocate(In size_t size)
				{
#ifdef USE_MALLOC
					return AllocateMalloc(size);
#elif defined(ONLY_POOL) && !defined(ONLY_HEAP)
					return g_poolAlloc.Allocate(size);
#else
					return g_heapAlloc.Allocate(size);
#endif
				}
				void* CLOAK_CALL MemoryHeap::TryResizeBlock(In void* ptr, In size_t size)
				{
#ifdef USE_MALLOC
					return nullptr;
#elif defined(ONLY_POOL) && !defined(ONLY_HEAP)
					return g_poolAlloc.TryReallocateNoFree(ptr, size);
#else
					return g_heapAlloc.TryReallocateNoFree(ptr, size);
#endif
				}
				void* CLOAK_CALL MemoryHeap::Reallocate(In void* ptr, In size_t size, In_opt bool moveData)
				{
#ifdef USE_MALLOC
					return ReallocateMalloc(ptr, size, moveData);
#elif defined(ONLY_POOL) && !defined(ONLY_HEAP)
					return g_poolAlloc.Reallocate(ptr, size, moveData);
#else
					return g_heapAlloc.Reallocate(ptr, size, moveData);
#endif
				}
				void CLOAK_CALL MemoryHeap::Free(In void* ptr)
				{
#ifdef USE_MALLOC
					FreeMalloc(ptr);
#elif defined(ONLY_POOL) && !defined(ONLY_HEAP)
					g_poolAlloc.Free(ptr);
#else
					g_heapAlloc.Free(ptr);
#endif
				}
				size_t CLOAK_CALL MemoryHeap::GetMaxAlloc() 
				{
#if defined(ONLY_POOL) && !defined(ONLY_HEAP)
					return PoolAllocator::MAX_ALLOC;
#else
					return TreeAllocator::MAX_ALLOC;
#endif
				}
				ThreadLocal::StorageID CLOAK_CALL ThreadLocal::Allocate() { return reinterpret_cast<ThreadLocal::StorageID>(g_threadLocal.Allocate(0)); }
				void CLOAK_CALL ThreadLocal::Free(In StorageID id) { g_threadLocal.Free(reinterpret_cast<void*>(id)); }
				void CLOAK_CALL ThreadLocal::Free(In StorageID id, In std::function<void(void* ptr)> func) 
				{ 
					if (func) { g_threadLocal.Free(reinterpret_cast<void*>(id), func); } 
					else { g_threadLocal.Free(reinterpret_cast<void*>(id)); }
				}
				void** CLOAK_CALL ThreadLocal::Get(In StorageID id) { return g_threadLocal.Get(reinterpret_cast<void*>(id)); }

				void* CLOAK_CALL MemoryLocal::Allocate(In size_t size) { return Impl::Global::Task::Allocate(size); }
				void* CLOAK_CALL MemoryLocal::TryResizeBlock(In void* ptr, In size_t size) { return Impl::Global::Task::TryResizeBlock(ptr, size); }
				void* CLOAK_CALL MemoryLocal::Reallocate(In void* ptr, In size_t size, In_opt bool moveData) { return Impl::Global::Task::Reallocate(ptr, size, moveData); }
				void CLOAK_CALL MemoryLocal::Free(In void* ptr) { Impl::Global::Task::Free(ptr); }
				size_t CLOAK_CALL MemoryLocal::GetMaxAlloc() { return Impl::Global::Task::GetMaxAlloc(); }

				size_t CLOAK_CALL PageStackAllocator::GetMaxAlloc()
				{
					return PageStackManager::MAX_ALLOC;
				}

				CLOAK_CALL PageStackAllocator::PageStackAllocator() : m_ptr(nullptr), m_empty(nullptr) {}
				CLOAK_CALL PageStackAllocator::~PageStackAllocator() { Clear(); }
				void* CLOAK_CALL_THIS PageStackAllocator::Allocate(In size_t size)
				{
					size = Align(size);
					const uint32_t fullSize = static_cast<uint32_t>(size + PageStackManager::STACK_HEAD_SIZE);
					PageStackManager::Page* p = reinterpret_cast<PageStackManager::Page*>(m_ptr);
					if (p == nullptr || p->Size - p->Offset < fullSize)
					{
						PageStackManager::Page* ep = reinterpret_cast<PageStackManager::Page*>(m_empty);
						if (ep != nullptr)
						{
							m_empty = ep->Next;
							ep->Offset = 0;
							ep->Next = p;
							m_ptr = ep;
							p = ep;
						}
						else
						{
							PageStackManager::Page* np = reinterpret_cast<PageStackManager::Page*>(g_pageStack.Allocate(fullSize));
							np->Next = p;
							m_ptr = np;
							p = np;
						}
					}
					PageStackManager::StackHeader* h = reinterpret_cast<PageStackManager::StackHeader*>(Move(p, PageStackManager::PAGE_HEAD_SIZE + p->Offset));
					h->Size = size;
					void* res = reinterpret_cast<void*>(Move(h, PageStackManager::STACK_HEAD_SIZE));
					p->Offset += fullSize;
					CLOAK_ASSUME(Align(reinterpret_cast<uintptr_t>(res)) == reinterpret_cast<uintptr_t>(res));
					return res;
				}
				void* CLOAK_CALL_THIS PageStackAllocator::TryResizeBlock(In void* ptr, In size_t size)
				{
					if (ptr == nullptr || size == 0) { return nullptr; }
					size = Align(size);
					PageStackManager::StackHeader* h = reinterpret_cast<PageStackManager::StackHeader*>(reinterpret_cast<uintptr_t>(ptr) - PageStackManager::STACK_HEAD_SIZE);
					PageStackManager::Page* p = reinterpret_cast<PageStackManager::Page*>(m_ptr);
					const uint32_t fullSize = static_cast<uint32_t>(PageStackManager::STACK_HEAD_SIZE + h->Size);
					if (reinterpret_cast<uintptr_t>(h) + fullSize == reinterpret_cast<uintptr_t>(p) + PageStackManager::PAGE_HEAD_SIZE + p->Offset)
					{
						if (p->Size + fullSize - p->Offset >= size + PageStackManager::STACK_HEAD_SIZE)
						{
							h->Size = size;
							p->Offset += static_cast<uint32_t>(size + PageStackManager::STACK_HEAD_SIZE - fullSize);
							return ptr;
						}
					}
					return nullptr;
				}
				void* CLOAK_CALL_THIS PageStackAllocator::Reallocate(In void* ptr, In size_t size, In_opt bool moveData)
				{
					if (ptr == nullptr) { return Allocate(size); }
					if (size == 0) { Free(ptr); return nullptr; }
					size = Align(size);
					PageStackManager::StackHeader* h = reinterpret_cast<PageStackManager::StackHeader*>(reinterpret_cast<uintptr_t>(ptr) - PageStackManager::STACK_HEAD_SIZE);
					PageStackManager::Page* p = reinterpret_cast<PageStackManager::Page*>(m_ptr);
					const uint32_t fullSize = static_cast<uint32_t>(PageStackManager::STACK_HEAD_SIZE + h->Size);
					if (reinterpret_cast<uintptr_t>(h) + fullSize == reinterpret_cast<uintptr_t>(p) + PageStackManager::PAGE_HEAD_SIZE + p->Offset)
					{
						CLOAK_ASSUME(p->Offset >= fullSize);
						p->Offset -= fullSize;
						if (p->Size - p->Offset >= size + PageStackManager::STACK_HEAD_SIZE)
						{
							h->Size = size;
							p->Offset += static_cast<uint32_t>(size + PageStackManager::STACK_HEAD_SIZE);
							return ptr;
						}
						void* n = Allocate(size);
						if (moveData == true) { memcpy(n, ptr, h->Size); }
						if (p->Offset == static_cast<uint32_t>(0))
						{
							m_ptr = p->Next;
							p->Next = reinterpret_cast<PageStackManager::Page*>(m_empty);
							m_empty = p;
						}
						return n;
					}
					void* n = Allocate(size);
					if (moveData == true) { memcpy(n, ptr, h->Size); }
					return n;
				}
				void CLOAK_CALL_THIS PageStackAllocator::Free(In void* ptr)
				{
					if (ptr == nullptr) { return; }
					PageStackManager::StackHeader* h = reinterpret_cast<PageStackManager::StackHeader*>(reinterpret_cast<uintptr_t>(ptr) - PageStackManager::STACK_HEAD_SIZE);
					PageStackManager::Page* p = reinterpret_cast<PageStackManager::Page*>(m_ptr);
					const uint32_t fullSize = static_cast<uint32_t>(PageStackManager::STACK_HEAD_SIZE + h->Size);
					if (reinterpret_cast<uintptr_t>(h) + fullSize == reinterpret_cast<uintptr_t>(p) + PageStackManager::PAGE_HEAD_SIZE + p->Offset)
					{
						CLOAK_ASSUME(p->Offset >= fullSize);
						p->Offset -= fullSize;
						if (p->Offset == static_cast<uint32_t>(0))
						{
							m_ptr = p->Next;
							p->Next = reinterpret_cast<PageStackManager::Page*>(m_empty);
							m_empty = p;
						}
					}
				}
				void CLOAK_CALL_THIS PageStackAllocator::Clear()
				{
					PageStackManager::Page* p = reinterpret_cast<PageStackManager::Page*>(m_ptr);
					while (p != nullptr)
					{
						PageStackManager::Page* lp = p->Next;
						g_pageStack.Free(p);
						p = lp;
					}
					p = reinterpret_cast<PageStackManager::Page*>(m_empty);
					while (p != nullptr)
					{
						PageStackManager::Page* lp = p->Next;
						g_pageStack.Free(p);
						p = lp;
					}
					m_ptr = nullptr;
					m_empty = nullptr;
				}

				//TODO
				//	- Replace most NewArray and DeleteArray calls with NewLocalArray and DeleteLocalArray, which should use the
				//		local memory allocator. This will allow memory allocation and freeing for these arrays to be much
				//		faster
				//		- Might also need to replace some normal MemoryHeap::Allocation calls with MemoryLocal::Allocation calls
				//			for the same reason
				//	- Update new container classes
				//		- Current implementation does not use ::Reallocation to it's full potential
				//		- Might need to think of a way to move as much code as possible from typed containers to untyped ones,
				//			and still be able to use Allocator::reallocate, as that requires the correct type
				//			- Possible solution might be that untyped container do not allocate memory themself at all, their
				//				code only manages allready allocated memory and calculate, when and how new memory should be
				//				allocated. 
				//			- This problem also only arises if the container-type actually requires the usage of reallocate
			}
		}
	}
	namespace Impl {
		namespace Global {
			namespace Memory {
				void CLOAK_CALL OnStart()
				{
					const Impl::OS::CPU_INFORMATION& cpu = Impl::OS::GetCPUInformation();
					API::Global::Memory::g_cacheLineSize = cpu.GetCache(Impl::OS::CacheType::Data, Impl::OS::CacheLevel::L1).LineSize;
					//If we failed to read cache line size, set to default
					if (API::Global::Memory::g_cacheLineSize == 0) { API::Global::Memory::g_cacheLineSize = 64; }
					API::Global::Memory::g_cacheBorder = API::Global::Memory::g_cacheLineSize << 4;
				}
				void CLOAK_CALL OnStop()
				{
#ifdef USE_MLD
					if (API::Global::Memory::g_heapAlloc.HasAnyMLD() || API::Global::Memory::g_poolAlloc.HasAnyMLD() || API::Global::Memory::g_threadLocal.HasAnyMLD())
					{
						Print("[CloakEngine] Detected Memoy Leaks:");
						if (API::Global::Memory::g_heapAlloc.HasAnyMLD())
						{
							Print("\tHeap Allocator:");
							API::Global::Memory::g_heapAlloc.CheckMLD();
						}
						if (API::Global::Memory::g_poolAlloc.HasAnyMLD())
						{
							Print("\tPool Allocator:");
							API::Global::Memory::g_poolAlloc.CheckMLD();
						}
						if (API::Global::Memory::g_threadLocal.HasAnyMLD())
						{
							Print("\tThread Local Storage:");
							API::Global::Memory::g_threadLocal.CheckMLD();
						}
					}
#endif
					API::Global::Memory::g_mainAlloc.Release();
				}
				void* CLOAK_CALL AllocateStaticMemory(In size_t size)
				{
					return API::Global::Memory::g_mainAlloc.Allocate(size);
				}
			}
		}
	}
}