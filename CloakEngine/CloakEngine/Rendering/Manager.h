#pragma once
#ifndef CE_API_RENDERING_MANAGER_H
#define CE_API_RENDERING_MANAGER_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/Defines.h"
#include "CloakEngine/Rendering/ColorBuffer.h"
#include "CloakEngine/Rendering/DepthBuffer.h"
#include "CloakEngine/Rendering/StructuredBuffer.h"
#include "CloakEngine/Rendering/DataTypes.h"
#include "CloakEngine/Rendering/PipelineState.h"
#include "CloakEngine/Rendering/Query.h"
#include "CloakEngine/Helper/SavePtr.h"
#include "CloakEngine/Helper/Color.h"
#include "CloakEngine/Files/Shader.h"

#ifdef _DEBUG
#define BeginContext(t,n,r) BeginContextFile(t,n,r,std::string(__FILE__)+" @ "+std::to_string(__LINE__))
#endif

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace Manager_v1 {
				CLOAK_INTERFACE_ID("{351113F7-93EB-458A-A301-FF113A7BA2AA}") IManager : public virtual API::Helper::SavePtr_v1::ISavePtr{
					public:
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IResource* buffer[], In_opt size_t bufferSize = 1, In_opt VIEW_TYPE Type = VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IPixelBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt VIEW_TYPE Type = VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IColorBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt VIEW_TYPE Type = VIEW_TYPE::ALL, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::WORLD) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IDepthBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt VIEW_TYPE Type = VIEW_TYPE::ALL) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IGraphicBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IStructuredBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual void CLOAK_CALL_THIS GenerateViews(In_reads(bufferSize) IConstantBuffer* buffer[], In_opt size_t bufferSize = 1, In_opt API::Rendering::DescriptorPageType page = API::Rendering::DescriptorPageType::UTILITY) = 0;
						virtual IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const TEXTURE_DESC& desc, In const Helper::Color::RGBA* const* colorData) const = 0;
						virtual IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const TEXTURE_DESC& desc, In Helper::Color::RGBA** colorData) const = 0;
						virtual IColorBuffer* CLOAK_CALL_THIS CreateColorBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const TEXTURE_DESC& desc) const = 0;
						virtual IDepthBuffer* CLOAK_CALL_THIS CreateDepthBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In const DEPTH_DESC& desc) const = 0;
						virtual IVertexBuffer* CLOAK_CALL_THIS CreateVertexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT vertexCount, In UINT vertexSize, In_reads(vertexCount*vertexSize) const void* data) const = 0;
						virtual IIndexBuffer* CLOAK_CALL_THIS CreateIndexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT size, In_reads(size) const uint16_t* data) const = 0;
						virtual IIndexBuffer* CLOAK_CALL_THIS CreateIndexBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT size, In_reads(size) const uint32_t* data) const = 0;
						virtual IConstantBuffer* CLOAK_CALL_THIS CreateConstantBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementSize, In_reads_opt(elementSize) const void* data = nullptr) const = 0;
						virtual IStructuredBuffer* CLOAK_CALL_THIS CreateStructuredBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementCount, In UINT elementSize, In_reads(elementSize) const void* data, In_opt bool counterBuffer = true) const = 0;
						virtual IStructuredBuffer* CLOAK_CALL_THIS CreateStructuredBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In UINT elementCount, In UINT elementSize, In_opt bool counterBuffer = true, In_reads_opt(elementSize) const void* data = nullptr) const = 0;
						virtual IByteAddressBuffer* CLOAK_CALL_THIS CreateByteAddressBuffer(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In_opt UINT count = 1) const = 0;
						virtual IPipelineState* CLOAK_CALL_THIS CreatePipelineState() const = 0;
						virtual IQuery* CLOAK_CALL_THIS CreateQuery(In API::Rendering::CONTEXT_TYPE contextType, In uint32_t nodeID, In QUERY_TYPE type) const = 0;
						virtual size_t CLOAK_CALL_THIS GetDataPos(In const TEXTURE_DESC& desc, In uint32_t MipMap, In_opt uint32_t Image = 0) const = 0;
#ifdef _DEBUG
						virtual void CLOAK_CALL_THIS BeginContextFile(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In REFIID riid, Outptr void** ptr, In const std::string& file) const = 0;
#else
						virtual void CLOAK_CALL_THIS BeginContext(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In REFIID riid, Outptr void** ptr) const = 0;
#endif
						virtual bool CLOAK_CALL_THIS WaitForFence(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In uint64_t val) = 0;
						virtual uint64_t CLOAK_CALL_THIS FlushExecutions(In API::Rendering::CONTEXT_TYPE type, In uint32_t nodeID, In_opt bool wait = false) = 0;
						virtual SHADER_MODEL CLOAK_CALL_THIS GetShaderModel() const = 0;
						virtual bool CLOAK_CALL_THIS EnumerateAdapter(In size_t id, Out std::string* name) const = 0;
						virtual size_t CLOAK_CALL_THIS GetNodeCount() const = 0;
						virtual const Hardware& CLOAK_CALL_THIS GetHardwareSetting() const = 0;
						virtual void CLOAK_CALL_THIS ReadBuffer(In size_t dstByteSize, Out_writes(dstByteSize) void* dst, In API::Rendering::IPixelBuffer* src, In_opt UINT subresource = 0) const = 0;
						virtual void CLOAK_CALL_THIS ReadBuffer(In size_t dstByteSize, Out_writes(dstByteSize) void* dst, In API::Rendering::IGraphicBuffer* src) const = 0;
						virtual void CLOAK_CALL_THIS ReadBuffer(In size_t dstSize, Out_writes(dstSize) Helper::Color::RGBA* color, In API::Rendering::IColorBuffer* src, In_opt UINT subresource = 0) const = 0;
				};
			}
		}
	}
}

#endif