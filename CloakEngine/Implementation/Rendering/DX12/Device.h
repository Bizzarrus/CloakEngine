#pragma once
#ifndef CE_IMPL_RENDERING_DX12_DEVICE_H
#define CE_IMPL_RENDERING_DX12_DEVICE_H

#include "Implementation/Rendering/Device.h"
#include "Implementation/Helper/SavePtr.h"

#include <d3d12.h>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace Device_v1 {
					class alignas(void*) D3D12_PIPELINE_STATE_DESC {
						private:
							//Define Subobject class:
							template<typename T, D3D12_PIPELINE_STATE_SUBOBJECT_TYPE E, typename Default = T> class alignas(void*) Subobject {
								private:
									D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_type;
									T m_value;
								public:
									CLOAK_CALL Subobject() noexcept : m_type(E), m_value(Default()) {}
									CLOAK_CALL Subobject(In const T& o) : m_type(E), m_value(o) {}
									template<typename... Args> CLOAK_CALL Subobject(Args&& ... args) : m_type(E), m_value(std::forward<Args>(args)...) {}
									Subobject& CLOAK_CALL_THIS operator=(In const T & o) { m_value = o; return *this; }
									CLOAK_CALL_THIS operator T&() { return m_value; }
									CLOAK_CALL_THIS operator T() const { return m_value; }
									T* CLOAK_CALL_THIS operator->() { return &m_value; }
									const T* CLOAK_CALL_THIS operator->() const { return &m_value; }
									T* CLOAK_CALL_THIS operator&() { return &m_value; }
									const T* CLOAK_CALL_THIS operator&() const { return &m_value; }
							};

							//Define helpers for default initialisation:
							struct DefaultSampleMask { CLOAK_CALL operator UINT() { return UINT_MAX; } };
							struct DefaultSampleDesc { CLOAK_CALL operator DXGI_SAMPLE_DESC() { return DXGI_SAMPLE_DESC{ 1, 0 }; } };
							struct DefaultBlendDesc { CLOAK_CALL operator D3D12_BLEND_DESC() 
							{
								D3D12_BLEND_DESC r;
								r.AlphaToCoverageEnable = FALSE;
								r.IndependentBlendEnable = FALSE;
								for (UINT a = 0; a < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; a++)
								{
									r.RenderTarget[a].BlendEnable = FALSE;
									r.RenderTarget[a].LogicOpEnable = FALSE;
									r.RenderTarget[a].SrcBlend = D3D12_BLEND_ONE;
									r.RenderTarget[a].DestBlend = D3D12_BLEND_ZERO;
									r.RenderTarget[a].BlendOp = D3D12_BLEND_OP_ADD;
									r.RenderTarget[a].SrcBlendAlpha = D3D12_BLEND_ONE;
									r.RenderTarget[a].DestBlendAlpha = D3D12_BLEND_ZERO;
									r.RenderTarget[a].BlendOpAlpha = D3D12_BLEND_OP_ADD;
									r.RenderTarget[a].LogicOp = D3D12_LOGIC_OP_NOOP;
									r.RenderTarget[a].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
								}
								return r;
							} };
							struct DefaultDepthStencilDesc { CLOAK_CALL operator D3D12_DEPTH_STENCIL_DESC() {
								D3D12_DEPTH_STENCIL_DESC r;
								r.DepthEnable = TRUE;
								r.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
								r.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
								r.StencilEnable = FALSE;
								r.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
								r.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
								r.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
								r.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
								r.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
								r.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
								r.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
								r.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
								r.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
								r.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
								return r;
							} };
							struct DefaultDepthStencilDesc1 { CLOAK_CALL operator D3D12_DEPTH_STENCIL_DESC1() {
									D3D12_DEPTH_STENCIL_DESC1 r;
									r.DepthEnable = TRUE;
									r.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
									r.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
									r.StencilEnable = FALSE;
									r.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
									r.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
									r.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
									r.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
									r.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
									r.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
									r.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
									r.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
									r.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
									r.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
									r.DepthBoundsTestEnable = FALSE;
									return r;
							} };
							struct DefaultRasterizerDesc { CLOAK_CALL operator D3D12_RASTERIZER_DESC() {
								D3D12_RASTERIZER_DESC r;
								r.FillMode = D3D12_FILL_MODE_SOLID;
								r.CullMode = D3D12_CULL_MODE_BACK;
								r.FrontCounterClockwise = FALSE;
								r.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
								r.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
								r.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
								r.DepthClipEnable = TRUE;
								r.MultisampleEnable = FALSE;
								r.AntialiasedLineEnable = FALSE;
								r.ForcedSampleCount = 0;
								r.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
								return r;
							} };
							struct DefaultViewInstancingDesc { CLOAK_CALL operator D3D12_VIEW_INSTANCING_DESC() {
								D3D12_VIEW_INSTANCING_DESC r;
								r.Flags = D3D12_VIEW_INSTANCING_FLAG_NONE;
								r.pViewInstanceLocations = nullptr;
								r.ViewInstanceCount = 0;
								return r;
							} };

							//Extensions for construction out of old descs
							struct EXTENDED_DEPTH_STENCIL_DESC : public D3D12_DEPTH_STENCIL_DESC1 {
								CLOAK_CALL EXTENDED_DEPTH_STENCIL_DESC() = default;
								CLOAK_CALL EXTENDED_DEPTH_STENCIL_DESC(In const D3D12_DEPTH_STENCIL_DESC1& o) : D3D12_DEPTH_STENCIL_DESC1(o) {};
								CLOAK_CALL EXTENDED_DEPTH_STENCIL_DESC(In const D3D12_DEPTH_STENCIL_DESC& o)
								{
									DepthEnable = o.DepthEnable;
									DepthWriteMask = o.DepthWriteMask;
									DepthFunc = o.DepthFunc;
									StencilEnable = o.StencilEnable;
									StencilReadMask = o.StencilReadMask;
									StencilWriteMask = o.StencilWriteMask;
									FrontFace = o.FrontFace;
									BackFace = o.BackFace;
									DepthBoundsTestEnable = FALSE;
								}
							};
							struct EXTENDED_RT_FORMAT_ARRAY : public D3D12_RT_FORMAT_ARRAY {
								CLOAK_CALL EXTENDED_RT_FORMAT_ARRAY() = default;
								CLOAK_CALL EXTENDED_RT_FORMAT_ARRAY(In const D3D12_RT_FORMAT_ARRAY& o) : D3D12_RT_FORMAT_ARRAY(o) {};
								CLOAK_CALL EXTENDED_RT_FORMAT_ARRAY(In const DXGI_FORMAT* formats, In UINT count)
								{
									CLOAK_ASSUME(count <= ARRAYSIZE(RTFormats));
									NumRenderTargets = count;
									memcpy(RTFormats, formats, sizeof(DXGI_FORMAT) * ARRAYSIZE(RTFormats));
								}
							};

							//Define Subobject list:
							typedef Subobject<D3D12_PIPELINE_STATE_FLAGS,			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS>												SUBOBJECT_FLAGS;
							typedef Subobject<UINT,									D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK>											SUBOBJECT_NODE_MASK;
							typedef Subobject<ID3D12RootSignature*,					D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE>										SUBOBJECT_ROOT_SIGNATURE;
							typedef Subobject<D3D12_INPUT_LAYOUT_DESC,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT>										SUBOBJECT_INPUT_LAYOUT;
							typedef Subobject<D3D12_INDEX_BUFFER_STRIP_CUT_VALUE,	D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE>									SUBOBJECT_IB_STRIP_CUT_VALUE;
							typedef Subobject<D3D12_PRIMITIVE_TOPOLOGY_TYPE,		D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY>									SUBOBJECT_PRIMITIVE_TOPOLOGY;
							typedef Subobject<D3D12_SHADER_BYTECODE,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS>													SUBOBJECT_VS;
							typedef Subobject<D3D12_SHADER_BYTECODE,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS>													SUBOBJECT_GS;
							typedef Subobject<D3D12_STREAM_OUTPUT_DESC,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT>										SUBOBJECT_STREAM_OUTPUT;
							typedef Subobject<D3D12_SHADER_BYTECODE,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS>													SUBOBJECT_HS;
							typedef Subobject<D3D12_SHADER_BYTECODE,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS>													SUBOBJECT_DS;
							typedef Subobject<D3D12_SHADER_BYTECODE,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS>													SUBOBJECT_PS;
							typedef Subobject<D3D12_SHADER_BYTECODE,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS>													SUBOBJECT_CS;
							typedef Subobject<DXGI_FORMAT,							D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT>								SUBOBJECT_DEPTH_STENCIL_FORMAT;
							typedef Subobject<EXTENDED_RT_FORMAT_ARRAY,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS>								SUBOBJECT_RENDER_TARGET_FORMATS;
							typedef Subobject<D3D12_CACHED_PIPELINE_STATE,			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO>											SUBOBJECT_CACHED_PSO;
							typedef Subobject<DXGI_SAMPLE_DESC,						D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC,			DefaultSampleDesc>			SUBOBJECT_SAMPLE_DESC;
							typedef Subobject<UINT,									D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK,			DefaultSampleMask>			SUBOBJECT_SAMPLE_MASK;
							typedef Subobject<D3D12_BLEND_DESC,						D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND,					DefaultBlendDesc>			SUBOBJECT_BLEND_DESC;
							typedef Subobject<D3D12_DEPTH_STENCIL_DESC,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL,			DefaultDepthStencilDesc>	SUBOBJECT_DEPTH_STENCIL;
							typedef Subobject<EXTENDED_DEPTH_STENCIL_DESC,			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1,			DefaultDepthStencilDesc1>	SUBOBJECT_DEPTH_STENCIL1;
							typedef Subobject<D3D12_RASTERIZER_DESC,				D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER,				DefaultRasterizerDesc>		SUBOBJECT_RASTERIZER;
							typedef Subobject<D3D12_VIEW_INSTANCING_DESC,			D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING,		DefaultViewInstancingDesc>	SUBOBJECT_VIEW_INSTANCING;
						public:
							SUBOBJECT_FLAGS Flags;
							SUBOBJECT_NODE_MASK NodeMask;
							SUBOBJECT_ROOT_SIGNATURE RootSignature;
							SUBOBJECT_INPUT_LAYOUT InputLayout;
							SUBOBJECT_IB_STRIP_CUT_VALUE IBStripCutValue;
							SUBOBJECT_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
							SUBOBJECT_VS VS;
							SUBOBJECT_GS GS;
							SUBOBJECT_STREAM_OUTPUT StreamOutput;
							SUBOBJECT_HS HS;
							SUBOBJECT_DS DS;
							SUBOBJECT_PS PS;
							SUBOBJECT_CS CS;
							SUBOBJECT_BLEND_DESC BlendState;
							SUBOBJECT_DEPTH_STENCIL1 DepthStencilState;
							SUBOBJECT_DEPTH_STENCIL_FORMAT DSVFormat;
							SUBOBJECT_RASTERIZER RasterizerState;
							SUBOBJECT_RENDER_TARGET_FORMATS RTVFormats;
							SUBOBJECT_SAMPLE_DESC SampleDesc;
							SUBOBJECT_SAMPLE_MASK SampleMask;
							SUBOBJECT_CACHED_PSO CachedPSO;
							SUBOBJECT_VIEW_INSTANCING ViewInstancingDesc;

							CLOAK_CALL D3D12_PIPELINE_STATE_DESC() = default;
							CLOAK_CALL D3D12_PIPELINE_STATE_DESC(In const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) 
								: Flags(desc.Flags)
								, NodeMask(desc.NodeMask)
								, RootSignature(desc.pRootSignature)
								, InputLayout(desc.InputLayout)
								, IBStripCutValue(desc.IBStripCutValue)
								, PrimitiveTopologyType(desc.PrimitiveTopologyType)
								, VS(desc.VS)
								, GS(desc.GS)
								, StreamOutput(desc.StreamOutput)
								, HS(desc.HS)
								, DS(desc.DS)
								, PS(desc.PS)
								, BlendState(desc.BlendState)
								, DepthStencilState(desc.DepthStencilState)
								, DSVFormat(desc.DSVFormat)
								, RasterizerState(desc.RasterizerState)
								, RTVFormats(desc.RTVFormats, desc.NumRenderTargets)
								, SampleDesc(desc.SampleDesc)
								, CachedPSO(desc.CachedPSO) 
							{

							}
							CLOAK_CALL D3D12_PIPELINE_STATE_DESC(In const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
								: Flags(desc.Flags)
								, NodeMask(desc.NodeMask)
								, RootSignature(desc.pRootSignature)
								, CS(desc.CS)
								, CachedPSO(desc.CachedPSO) 
							{
								DepthStencilState->DepthEnable = false;
							}
					};

					struct GraphicDevice {
						CE::RefPointer<ID3D12Device> V0 = nullptr;
						CE::RefPointer<ID3D12Device1> V1 = nullptr;
						CE::RefPointer<ID3D12Device2> V2 = nullptr;
						CE::RefPointer<ID3D12Device3> V3 = nullptr;
						CE::RefPointer<ID3D12Device4> V4 = nullptr;
						CE::RefPointer<ID3D12Device5> V5 = nullptr;
						CE::RefPointer<ID3D12Device6> V6 = nullptr;
					};

					class CLOAK_UUID("{79914EA8-F2D8-4172-A48B-E7A1A5CE5BA5}") Device : public virtual IDevice, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							CLOAK_CALL Device(In const GraphicDevice& dev12);
							virtual CLOAK_CALL ~Device();

							uint32_t CLOAK_CALL_THIS GetDescriptorHandleIncrementSize(In HEAP_TYPE type) const override;
							void CLOAK_CALL_THIS CreateConstantBufferView(In const CBV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) const override;
							void CLOAK_CALL_THIS CreateShaderResourceView(In IResource* rsc, In const SRV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) override;
							void CLOAK_CALL_THIS CreateRenderTargetView(In IResource* rsc, In const RTV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) override;
							void CLOAK_CALL_THIS CreateUnorderedAccessView(In IResource* rsc, In IResource* byteAddress, In const UAV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) override;
							void CLOAK_CALL_THIS CreateDepthStencilView(In IResource* rsc, In const DSV_DESC& desc, In_opt const API::Rendering::CPU_DESCRIPTOR_HANDLE& cpuHandle = API::Rendering::CPU_HANDLE_INVALID) override;
							void CLOAK_CALL_THIS CreateSampler(In const API::Rendering::SAMPLER_DESC& desc, In API::Rendering::CPU_DESCRIPTOR_HANDLE handle) override;
							void CLOAK_CALL_THIS CopyDescriptorsSimple(In UINT numDescriptors, In const API::Rendering::CPU_DESCRIPTOR_HANDLE& dstStart, In const API::Rendering::CPU_DESCRIPTOR_HANDLE& srcStart, In HEAP_TYPE heap) override;
							void CLOAK_CALL_THIS CopyDescriptors(In UINT numDstRanges, In_reads(numDstRanges) const API::Rendering::CPU_DESCRIPTOR_HANDLE* dstRangeStarts, In_reads_opt(numDstRanges) const UINT* dstRangeSizes, In UINT numSrcRanges, In_reads(numSrcRanges) const API::Rendering::CPU_DESCRIPTOR_HANDLE* srcRangeStarts, In_reads_opt(numSrcRanges) const UINT* srcRangeSizes, In HEAP_TYPE heap) override;

							HRESULT CLOAK_CALL_THIS CreateRootSignature(In const void* data, In size_t dataSize, In REFIID riid, Out void** ppvObject) override;
							HRESULT CLOAK_CALL_THIS CreateCommandQueue(In const D3D12_COMMAND_QUEUE_DESC& desc, In REFIID riid, Out void** ppvObject);
							HRESULT CLOAK_CALL_THIS CreateFence(In uint64_t value, In D3D12_FENCE_FLAGS flags, In REFIID riid, Out void** ppvObject);
							HRESULT CLOAK_CALL_THIS CreateCommandList(In UINT node, In D3D12_COMMAND_LIST_TYPE type, In ID3D12CommandAllocator* alloc, In REFIID riid, Out void** ppvObject);
							HRESULT CLOAK_CALL_THIS CreateCommandAllocator(In D3D12_COMMAND_LIST_TYPE type, In REFIID riid, Out void** ppvObject);
							HRESULT CLOAK_CALL_THIS CreateCommittedResource(In const D3D12_HEAP_PROPERTIES& heap, In D3D12_HEAP_FLAGS heapFlags, In const D3D12_RESOURCE_DESC& desc, In D3D12_RESOURCE_STATES state, In_opt const D3D12_CLEAR_VALUE* clearValue, In REFIID riid, Out void** ppvObject);
							HRESULT CLOAK_CALL_THIS CreateDescriptorHeap(In const D3D12_DESCRIPTOR_HEAP_DESC& desc, REFIID riid, void** ppvObject);
							
							HRESULT CLOAK_CALL_THIS CreateComputePipelineState(In const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, REFIID riid, void** ppvObject);
							HRESULT CLOAK_CALL_THIS CreateGraphicsPipelineState(In const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, REFIID riid, void** ppvObject);

							HRESULT CLOAK_CALL_THIS CreatePipelineState(In D3D12_PIPELINE_STATE_DESC& desc, REFIID riid, void** ppvObject);
							
							HRESULT CLOAK_CALL_THIS CreateQueryHeap(In const D3D12_QUERY_HEAP_DESC& desc, REFIID riid, void** ppvObject);
							void CLOAK_CALL_THIS GetCopyableFootprints(In const D3D12_RESOURCE_DESC& resourceDesc, In UINT firstSubresource, In UINT numSubresources, In UINT64 baseOffset, Out_writes(numSubresources) D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts, Out_writes(numSubresources) UINT* numRows, Out_writes(numSubresources) UINT64* rowSizeInBytes, Out_opt UINT64* totalBytes = nullptr);
							Use_annotations virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
						protected:
							GraphicDevice m_dev12;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif