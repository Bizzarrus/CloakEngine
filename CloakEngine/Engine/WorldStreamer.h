#pragma once
#ifndef CE_E_WORLDSTREAMER_H
#define CE_E_WORLDSTREAMER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Rendering/RenderPass.h"

#include "Implementation/Files/Scene.h"

#include "Engine/LinkedList.h"

namespace CloakEngine {
	namespace Engine {
		namespace WorldStreamer {
			struct ActiveStreamer {
				Impl::Files::WorldNode* Node;
				float LoadDistance;
				float ViewDistance[4];
				float ViewRange;
				CLOAK_CALL ActiveStreamer(In Impl::Files::WorldNode* n, In float loadDist, In float viewDist, In float viewMaxDist);
				void CLOAK_CALL_THIS UpdateDistance(In float load, In float view, In float viewMaxDist);
			};
			struct DrawOffset {
				API::Global::Math::Vector Offset;
				size_t CamID;
			};
			typedef ActiveStreamer* streamAdr;

			class NodeList {
				private:
					struct Element {
						Element* Next;
						Impl::Files::WorldNode* Node;
					};
					struct Header {
						API::Rendering::WorldID ID;
						Element* Start;
					};
					struct BlockHeader {
						BlockHeader* Next;
					};
					static constexpr size_t MAX_HEADER_PAGE_SIZE = 1 KB;
					static constexpr size_t PAGE_HEADER_COUNT = MAX_HEADER_PAGE_SIZE / sizeof(Header);
					static constexpr size_t MAX_PAGE_SIZE = 1 MB;
					static constexpr size_t BLOCK_HEADER_SIZE = (sizeof(BlockHeader) + std::alignment_of<Element>::value - 1) & ~(std::alignment_of<Element>::value - 1);
					static constexpr size_t PAGE_ELEMENT_COUNT = (MAX_PAGE_SIZE - BLOCK_HEADER_SIZE) / sizeof(Element);
					static constexpr size_t PAGE_SIZE = BLOCK_HEADER_SIZE + (PAGE_ELEMENT_COUNT * sizeof(Element));
					API::Global::Memory::PageStackAllocator* const m_alloc;
					size_t m_idCount;
					size_t m_pageSize;
					Header* m_headers;
					BlockHeader* m_cur;

					Header* CLOAK_CALL GetHeader(In API::Rendering::WorldID id);
				public:
					CLOAK_CALL NodeList(In API::Global::Memory::PageStackAllocator* alloc);
					CLOAK_CALL ~NodeList();

					void CLOAK_CALL_THIS AddNode(In API::Rendering::WorldID id, In Impl::Files::WorldNode* node);
			};

			streamAdr CLOAK_CALL AddStreamer(In Impl::Files::WorldNode* node, In float loadDist, In float viewDist, In float viewMaxDist);
			void CLOAK_CALL UpdateStreamer(In const streamAdr& adr, In Impl::Files::WorldNode* node);
			void CLOAK_CALL UpdateStreamer(In const streamAdr& adr, In float loadDist, In float viewDist, In float viewMaxDist);
			void CLOAK_CALL RemoveStreamer(In const streamAdr& adr);

			void CLOAK_CALL SetLoadPortals(In bool load);
			bool CLOAK_CALL CheckVisible(In API::Rendering::WorldID worldID, In API::Files::IScene* scene);

			void CLOAK_CALL Initialize();
			void CLOAK_CALL Release();
			void CLOAK_CALL Update();
			void CLOAK_CALL UpdateNodeToWorld(In API::Global::Memory::PageStackAllocator* alloc, Out NodeList* list);
		}
	}
}

#endif