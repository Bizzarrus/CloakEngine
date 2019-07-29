#pragma once
#ifndef CE_IMPL_RENDEIRNG_MANAGER_H
#define CE_IMPL_RENDERING_MANAGER_H

#include "CloakEngine/Rendering/Manager.h"

#include "Implementation/Rendering/Defines.h"
#include "Implementation/Rendering/Device.h"
#include "Implementation/Rendering/Context.h"
#include "Implementation/Rendering/ColorBuffer.h"
#include "Implementation/Rendering/DepthBuffer.h"
#include "Implementation/Rendering/Allocator.h"
#include "Implementation/Rendering/StructuredBuffer.h"
#include "Implementation/Rendering/Queue.h"

#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Global/Graphic.h"
#include "CloakEngine/Global/Math.h"
#include "CloakEngine/Rendering/PipelineState.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			inline namespace Manager_v1 {
				struct CLOAK_ALIGN MESH_DATA {
					API::Rendering::MATRIX LocalToWorld;
					API::Rendering::MATRIX WorldToLocal;
					API::Rendering::FLOAT1 Wetness;
				};
				RENDERING_INTERFACE CLOAK_UUID("{F63EDC97-5C73-4C9A-BCDB-1FFDAD3BF7BE}") IManager : public virtual API::Rendering::IManager {
					public:
						static void CLOAK_CALL GetSupportedModes(Out API::List<API::Global::Graphic::RenderMode>* modes);
						static IManager* CLOAK_CALL Create(In API::Global::Graphic::RenderMode Type);

						virtual HRESULT CLOAK_CALL_THIS Create(In UINT adapterID) = 0;
						virtual HMONITOR CLOAK_CALL_THIS GetMonitor() const = 0;
						virtual void CLOAK_CALL_THIS ShutdownAdapters() = 0;
						virtual void CLOAK_CALL_THIS ShutdownMemory() = 0;
						virtual void CLOAK_CALL_THIS ShutdownDevice() = 0;
						virtual void CLOAK_CALL_THIS IdleGPU() = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IResource* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IPixelBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IColorBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::WORLD) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IDepthBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IGraphicBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IStructuredBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) API::Rendering::IConstantBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IColorBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::VIEW_TYPE Type = API::Rendering::VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::WORLD) = 0;
						virtual void CLOAK_CALL_THIS InitializeBuffer(In API::Rendering::IResource* dst, In size_t size, In_reads_bytes(size) const void* data, In_opt bool wait = false) = 0;
						virtual void CLOAK_CALL_THIS InitializeTexture(In API::Rendering::IResource* dst, In UINT numSubresources, In_reads(numSubresources) API::Rendering::SUBRESOURCE_DATA data[], In_opt bool wait = false) const = 0;
						virtual void CLOAK_CALL_THIS UpdateTextureSubresource(In API::Rendering::IResource* dst, In UINT firstSubresource, In UINT numSubresources, In_reads(numSubresources) API::Rendering::SUBRESOURCE_DATA data[], In_opt bool wait = false) const = 0;
						virtual uint64_t CLOAK_CALL_THIS IncrementFence(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID) = 0;
						virtual bool CLOAK_CALL_THIS IsReady() const = 0;
						virtual bool CLOAK_CALL_THIS IsFenceComplete(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In uint64_t val) = 0;
						virtual void CLOAK_CALL_THIS WaitForGPU() = 0;
						virtual bool CLOAK_CALL_THIS SupportMultiThreadedRendering() const = 0;
						virtual IDevice* CLOAK_CALL_THIS GetDevice() const = 0;
						virtual IRootSignature* CLOAK_CALL_THIS CreateRootSignature() const = 0;
						virtual IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In ISwapChain* base, In uint32_t textureNum) const = 0;
						virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::TEXTURE_DESC& desc, In const API::Helper::Color::RGBA* const* colorData) const = 0;
						virtual API::Rendering::IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::TEXTURE_DESC& desc) const = 0;
						virtual API::Rendering::IDepthBuffer* CLOAK_CALL_THIS CreateDepthBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const API::Rendering::DEPTH_DESC& desc) const = 0;
						virtual IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In const API::Rendering::TEXTURE_DESC& desc, In size_t nodeID, In UINT nodeMask) const = 0;
						virtual IDepthBuffer* CLOAK_CALL_THIS CreateDepthBuffer(In const API::Rendering::DEPTH_DESC& desc, In size_t nodeID, In UINT nodeMask) const = 0;
						virtual ISwapChain* CLOAK_CALL_THIS CreateSwapChain(In const API::Global::Graphic::Settings& gset, Out_opt HRESULT* hRet = nullptr) = 0;
						virtual size_t CLOAK_CALL_THIS GetNodeID(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID) const = 0;
						virtual IQueue* CLOAK_CALL_THIS GetQueueByNodeID(In size_t nodeID) const = 0;
						virtual ISingleQueue* CLOAK_CALL_THIS GetQueue(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID) const = 0;
						virtual void CLOAK_CALL_THIS Update() = 0;
				};
			}
		}
	}
}

#endif