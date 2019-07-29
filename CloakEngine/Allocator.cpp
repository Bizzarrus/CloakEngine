#include "stdafx.h"
#include "Implementation/Rendering/Allocator.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			CLOAK_CALL DynAlloc::DynAlloc()
			{
				Buffer = nullptr;
				Offset = 0;
				Size = 0;
				Data = nullptr;
				GPUAddress = GPU_ADDRESS_NULL;
			}
			CLOAK_CALL DynAlloc::~DynAlloc()
			{
				if (Buffer != nullptr) { Buffer->Unmap(); }
				SAVE_RELEASE(Buffer);
			}
			void CLOAK_CALL_THIS DynAlloc::Initialize(In IDynamicAllocatorPage* base, In size_t offset, In size_t size)
			{
				if (Buffer != nullptr) { Buffer->Unmap(); }
				SAVE_RELEASE(Buffer);
				Buffer = base;
				Offset = offset;
				Size = size;
				Data = nullptr;
				GPUAddress = GPU_ADDRESS_NULL;
				if (Buffer != nullptr)
				{
					Buffer->AddRef();
					Buffer->Map(&Data);
					Data = (reinterpret_cast<uint8_t*>(Data)) + Offset;
					GPUAddress = Buffer->GetGPUAddress().ptr + Offset;
				}
			}
		}
	}
	namespace API {
		namespace Rendering {
			namespace Allocator_v1 {
				CLOAK_CALL ReadBack::ReadBack(IReadBackPage* page, In size_t offset, In size_t size)
				{
					m_state = page == nullptr;
					m_page = page;
					m_offset = offset;
					m_size = size;
					m_data = nullptr;
					if (m_state == false) { m_page->AddReadCount(offset); }
				}
				CLOAK_CALL ReadBack::ReadBack(In const ReadBack& o)
				{
					m_state = o.m_state;
					m_page = o.m_page;
					m_offset = o.m_offset;
					m_size = o.m_size;
					if (m_state == false)
					{
						m_data = nullptr;
						m_page->AddReadCount(m_offset);
					}
					else
					{
						m_data = NewArray(uint8_t, m_size);
						memcpy(m_data, o.m_data, m_size);
					}
				}
				CLOAK_CALL ReadBack::ReadBack(In ReadBack&& o)
				{
					m_state = o.m_state;
					m_page = o.m_page;
					m_offset = o.m_offset;
					m_size = o.m_size;
					m_data = o.m_data;

					if (m_state == false) { m_page->AddReadCount(m_offset); }
					o.m_data = nullptr;
				}
				CLOAK_CALL ReadBack::~ReadBack()
				{
					if (m_state == false) { m_page->RemoveReadCount(m_offset); }
					DeleteArray(reinterpret_cast<uint8_t*>(m_data));
				}
				ReadBack& CLOAK_CALL_THIS ReadBack::operator=(In const ReadBack& o)
				{
					if (this != &o)
					{
						if (m_state == false) { m_page->RemoveReadCount(m_offset); }
						DeleteArray(reinterpret_cast<uint8_t*>(m_data));

						m_state = o.m_state;
						m_page = o.m_page;
						m_offset = o.m_offset;
						m_size = o.m_size;
						if (m_state == false)
						{
							m_data = nullptr;
							m_page->AddReadCount(m_offset);
						}
						else
						{
							m_data = NewArray(uint8_t, m_size);
							memcpy(m_data, o.m_data, m_size);
						}
					}
					return *this;
				}
				ReadBack& CLOAK_CALL_THIS ReadBack::operator=(In ReadBack&& o)
				{
					if (m_state == false) { m_page->RemoveReadCount(m_offset); }
					DeleteArray(reinterpret_cast<uint8_t*>(m_data));

					m_state = o.m_state;
					m_page = o.m_page;
					m_offset = o.m_offset;
					m_size = o.m_size;
					m_data = o.m_data;

					if (m_state == false) { m_page->AddReadCount(m_offset); }
					o.m_data = nullptr;

					return *this;
				}

				void* CLOAK_CALL_THIS ReadBack::GetData()
				{
					if (m_state == false)
					{
						if (m_size > 0)
						{
							m_data = NewArray(uint8_t, m_size);
							m_page->ReadData(m_offset, m_size, m_data);
						}
						m_page->RemoveReadCount(m_offset);
						m_page = nullptr;
						m_state = true;
					}
					return m_data;
				}
				size_t CLOAK_CALL_THIS ReadBack::GetSize() const { return m_size; }
				IReadBackPage* CLOAK_CALL_THIS ReadBack::GetResource() const { return m_page; }
				size_t CLOAK_CALL_THIS ReadBack::GetOffset() const { return m_offset; }
			}
		}
	}
}