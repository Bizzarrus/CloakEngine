#include "stdafx.h"
#include "Implementation/Rendering/VertexData.h"
#include "CloakEngine/Global/Memory.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			void* CLOAK_CALL VERTEX_2D::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
			void* CLOAK_CALL VERTEX_2D::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
			void CLOAK_CALL VERTEX_2D::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
			void CLOAK_CALL VERTEX_2D::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

			void* CLOAK_CALL VERTEX_3D_POSITION::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
			void* CLOAK_CALL VERTEX_3D_POSITION::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
			void CLOAK_CALL VERTEX_3D_POSITION::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
			void CLOAK_CALL VERTEX_3D_POSITION::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

			void* CLOAK_CALL VERTEX_3D::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
			void* CLOAK_CALL VERTEX_3D::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
			void CLOAK_CALL VERTEX_3D::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
			void CLOAK_CALL VERTEX_3D::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }

			void* CLOAK_CALL VERTEX_TERRAIN::operator new(In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
			void* CLOAK_CALL VERTEX_TERRAIN::operator new[](In size_t s) { return API::Global::Memory::MemoryPool::Allocate(s); }
			void CLOAK_CALL VERTEX_TERRAIN::operator delete(In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
			void CLOAK_CALL VERTEX_TERRAIN::operator delete[](In void* s) { return API::Global::Memory::MemoryPool::Free(s); }
		}
	}
}