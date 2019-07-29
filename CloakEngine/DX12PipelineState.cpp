#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/PipelineState.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/RootSignature.h"
#include "Implementation/Rendering/DX12/Device.h"
#include "Implementation/Files/Shader.h"
#include "Implementation/Rendering/Deserialization.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Rendering/RootDescription.h"
#include "CloakEngine/Global/Game.h"

#include "Engine/Graphic/Core.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace PipelineState_v1 {
					constexpr size_t PACKING_SIZE = 16;
					constexpr size_t MAX_RENDER_TARGETS = min(8, API::Rendering::MAX_RENDER_TARGET);
					/*
					HRESULT CLOAK_CALL CreateComputePSO(In Device* device, In UINT nodeMask, In const API::Rendering::COMPUTE_PIPELINE_STATE_DESC& desc, In ID3D12RootSignature* root, In API::Rendering::SHADER_MODEL shader, Out ID3D12PipelineState** pso)
					{
						if (root == nullptr) { return E_FAIL; }

						CloakDebugLog("Create Compute PSO");

						D3D12_COMPUTE_PIPELINE_STATE_DESC pd;
						pd.CachedPSO.CachedBlobSizeInBytes = 0;
						pd.CachedPSO.pCachedBlob = nullptr;
						pd.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
						pd.NodeMask = nodeMask;
						pd.pRootSignature = root;

						pd.CS.BytecodeLength = desc.CS.Model[shader].Length;
						pd.CS.pShaderBytecode = desc.CS.Model[shader].Code;

						return device->CreateComputePipelineState(pd, CE_QUERY_ARGS(pso));
					}
					HRESULT CLOAK_CALL CreateGraphicPSO(In Device* device, In UINT nodeMask, In const API::Rendering::GRAPHIC_PIPELINE_STATE_DESC& desc, In ID3D12RootSignature* root, In API::Rendering::SHADER_MODEL shader, Out ID3D12PipelineState** pso)
					{
						if (root == nullptr || desc.NumRenderTargets > MAX_RENDER_TARGETS) { return E_FAIL; }

						CloakDebugLog("Create Graphic PSO");

						D3D12_GRAPHICS_PIPELINE_STATE_DESC pd;
						CLOAK_ASSUME(MAX_RENDER_TARGETS <= ARRAYSIZE(pd.BlendState.RenderTarget));

						pd.CachedPSO.CachedBlobSizeInBytes = 0;
						pd.CachedPSO.pCachedBlob = nullptr;
						pd.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
						pd.NodeMask = nodeMask;
						pd.pRootSignature = root;

						pd.NumRenderTargets = desc.NumRenderTargets;

						pd.BlendState.AlphaToCoverageEnable = desc.BlendState.AlphaToCoverage ? TRUE : FALSE;
						pd.BlendState.IndependentBlendEnable = desc.BlendState.IndependentBlend ? TRUE : FALSE;
						for (size_t a = 0; a < MAX_RENDER_TARGETS && a < desc.NumRenderTargets; a++)
						{
							pd.BlendState.RenderTarget[a].BlendEnable = desc.BlendState.RenderTarget[a].BlendEnable ? TRUE : FALSE;
							pd.BlendState.RenderTarget[a].LogicOpEnable = desc.BlendState.RenderTarget[a].LogicOperationEnable ? TRUE : FALSE;
							pd.BlendState.RenderTarget[a].SrcBlend = Casting::CastForward(desc.BlendState.RenderTarget[a].SrcBlend);
							pd.BlendState.RenderTarget[a].DestBlend = Casting::CastForward(desc.BlendState.RenderTarget[a].DstBlend);
							pd.BlendState.RenderTarget[a].SrcBlendAlpha = Casting::CastForward(desc.BlendState.RenderTarget[a].SrcBlendAlpha);
							pd.BlendState.RenderTarget[a].DestBlendAlpha = Casting::CastForward(desc.BlendState.RenderTarget[a].DstBlendAlpha);
							pd.BlendState.RenderTarget[a].BlendOp = Casting::CastForward(desc.BlendState.RenderTarget[a].BlendOperation);
							pd.BlendState.RenderTarget[a].BlendOpAlpha = Casting::CastForward(desc.BlendState.RenderTarget[a].BlendOperationAlpha);
							pd.BlendState.RenderTarget[a].LogicOp = Casting::CastForward(desc.BlendState.RenderTarget[a].LogicOperation);
							pd.BlendState.RenderTarget[a].RenderTargetWriteMask = Casting::CastForward(desc.BlendState.RenderTarget[a].WriteMask);

							pd.RTVFormats[a] = Casting::CastForward(desc.RTVFormat[a]);
						}
						for (size_t a = desc.NumRenderTargets; a < ARRAYSIZE(pd.BlendState.RenderTarget); a++)
						{
							pd.BlendState.RenderTarget[a].BlendEnable = FALSE;
							pd.BlendState.RenderTarget[a].LogicOpEnable = FALSE;
							pd.BlendState.RenderTarget[a].SrcBlend = D3D12_BLEND_ZERO;
							pd.BlendState.RenderTarget[a].DestBlend = D3D12_BLEND_ZERO;
							pd.BlendState.RenderTarget[a].SrcBlendAlpha = D3D12_BLEND_ZERO;
							pd.BlendState.RenderTarget[a].DestBlendAlpha = D3D12_BLEND_ZERO;
							pd.BlendState.RenderTarget[a].BlendOp = D3D12_BLEND_OP_ADD;
							pd.BlendState.RenderTarget[a].BlendOpAlpha = D3D12_BLEND_OP_ADD;
							pd.BlendState.RenderTarget[a].LogicOp = D3D12_LOGIC_OP_NOOP;
							pd.BlendState.RenderTarget[a].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

							pd.RTVFormats[a] = DXGI_FORMAT_UNKNOWN;
						}

						pd.DepthStencilState.BackFace.StencilDepthFailOp = Casting::CastForward(desc.DepthStencilState.BackFace.DepthFailOperation);
						pd.DepthStencilState.BackFace.StencilFailOp = Casting::CastForward(desc.DepthStencilState.BackFace.StencilFailOperation);
						pd.DepthStencilState.BackFace.StencilFunc = Casting::CastForward(desc.DepthStencilState.BackFace.StencilFunction);
						pd.DepthStencilState.BackFace.StencilPassOp = Casting::CastForward(desc.DepthStencilState.BackFace.PassOperation);

						pd.DepthStencilState.FrontFace.StencilDepthFailOp = Casting::CastForward(desc.DepthStencilState.FrontFace.DepthFailOperation);
						pd.DepthStencilState.FrontFace.StencilFailOp = Casting::CastForward(desc.DepthStencilState.FrontFace.StencilFailOperation);
						pd.DepthStencilState.FrontFace.StencilFunc = Casting::CastForward(desc.DepthStencilState.FrontFace.StencilFunction);
						pd.DepthStencilState.FrontFace.StencilPassOp = Casting::CastForward(desc.DepthStencilState.FrontFace.PassOperation);

						pd.DepthStencilState.DepthEnable = desc.DepthStencilState.DepthEnable ? TRUE : FALSE;
						pd.DepthStencilState.DepthFunc = Casting::CastForward(desc.DepthStencilState.DepthFunction);
						pd.DepthStencilState.DepthWriteMask = Casting::CastForward(desc.DepthStencilState.DepthWriteMask);
						pd.DepthStencilState.StencilEnable = desc.DepthStencilState.StencilEnable ? TRUE : FALSE;
						pd.DepthStencilState.StencilReadMask = static_cast<UINT8>(desc.DepthStencilState.StencilReadMask);
						pd.DepthStencilState.StencilWriteMask = static_cast<UINT8>(desc.DepthStencilState.StencilWriteMask);

						pd.VS.BytecodeLength = desc.VS.Model[shader].Length;
						pd.VS.pShaderBytecode = desc.VS.Model[shader].Code;
						pd.GS.BytecodeLength = desc.GS.Model[shader].Length;
						pd.GS.pShaderBytecode = desc.GS.Model[shader].Code;
						pd.PS.BytecodeLength = desc.PS.Model[shader].Length;
						pd.PS.pShaderBytecode = desc.PS.Model[shader].Code;
						pd.HS.BytecodeLength = desc.HS.Model[shader].Length;
						pd.HS.pShaderBytecode = desc.HS.Model[shader].Code;
						pd.DS.BytecodeLength = desc.DS.Model[shader].Length;
						pd.DS.pShaderBytecode = desc.DS.Model[shader].Code;

						pd.DSVFormat = Casting::CastForward(desc.DSVFormat);
						pd.IBStripCutValue = Casting::CastForward(desc.IndexBufferStripCutValue);
						pd.PrimitiveTopologyType = Casting::CastForward(desc.PrimitiveTopologyType);

						const API::Rendering::INPUT_ELEMENT_DESC* input = nullptr;
						size_t inputSize = 0;
						switch (desc.InputLayout.Type)
						{
							case API::Rendering::INPUT_LAYOUT_TYPE_NONE:
								input = nullptr;
								inputSize = 0;
								break;
							case API::Rendering::INPUT_LAYOUT_TYPE_MESH:
								input = Impl::Rendering::InputElementDescription::MESH;
								inputSize = ARRAYSIZE(Impl::Rendering::InputElementDescription::MESH);
								break;
							case API::Rendering::INPUT_LAYOUT_TYPE_MESH_POS:
								input = Impl::Rendering::InputElementDescription::MESH_POS;
								inputSize = ARRAYSIZE(Impl::Rendering::InputElementDescription::MESH_POS);
								break;
							case API::Rendering::INPUT_LAYOUT_TYPE_GUI:
								input = Impl::Rendering::InputElementDescription::GUI;
								inputSize = ARRAYSIZE(Impl::Rendering::InputElementDescription::GUI);
								break;
							case API::Rendering::INPUT_LAYOUT_TYPE_TERRAIN:
								input = Impl::Rendering::InputElementDescription::TERRAIN;
								inputSize = ARRAYSIZE(Impl::Rendering::InputElementDescription::TERRAIN);
								break;
							case API::Rendering::INPUT_LAYOUT_TYPE_CUSTOM:
								input = desc.InputLayout.Custom.Elements;
								inputSize = desc.InputLayout.Custom.NumElements;
								break;
							default:
								return E_FAIL;
						}

						D3D12_INPUT_ELEMENT_DESC* inp = NewArray(D3D12_INPUT_ELEMENT_DESC, inputSize);
						UINT inOffset[16];
						for (size_t a = 0; a < ARRAYSIZE(inOffset); a++) { inOffset[a] = 0; }
						for (UINT a = 0; a < inputSize; a++)
						{
							if (input[a].InputSlot >= ARRAYSIZE(inOffset)) { DeleteArray(inp); return E_FAIL; }
							const size_t is = GetFormatSize(input[a].Format);
							const UINT offset = inOffset[input[a].InputSlot] + input[a].Offset;
							UINT remSize = offset % PACKING_SIZE;
							if (remSize == 0) { remSize = PACKING_SIZE; }
							if (is > remSize) 
							{ 
								API::Global::Log::WriteToLog("Packing would require unspecified padding!", API::Global::Log::Type::Warning);
								DeleteArray(inp); 
								return E_FAIL; 
							}

							inp[a].AlignedByteOffset = offset;
							inp[a].Format = Casting::CastForward(input[a].Format);
							inp[a].InputSlot = input[a].InputSlot;
							inp[a].InputSlotClass = Casting::CastForward(input[a].InputSlotClass);
							inp[a].InstanceDataStepRate = input[a].InstanceDataStepRate;
							inp[a].SemanticIndex = input[a].SemanticIndex;
							inp[a].SemanticName = input[a].SemanticName;

							inOffset[input[a].InputSlot] = static_cast<UINT>(offset + is);
						}

						pd.InputLayout.NumElements = static_cast<UINT>(inputSize);
						pd.InputLayout.pInputElementDescs = inp;

						pd.RasterizerState.AntialiasedLineEnable = desc.RasterizerState.AntialiasedLine ? TRUE : FALSE;
						pd.RasterizerState.ConservativeRaster = Casting::CastForward(desc.RasterizerState.ConservativeRasterization);
						pd.RasterizerState.CullMode = Casting::CastForward(desc.RasterizerState.CullMode);
						pd.RasterizerState.FillMode = Casting::CastForward(desc.RasterizerState.FillMode);
						pd.RasterizerState.DepthBias = desc.RasterizerState.DepthBias;
						pd.RasterizerState.DepthBiasClamp = desc.RasterizerState.DepthBiasClamp;
						pd.RasterizerState.DepthClipEnable = desc.RasterizerState.DepthClip ? TRUE : FALSE;
						pd.RasterizerState.ForcedSampleCount = desc.RasterizerState.ForcedSampleCount;
						pd.RasterizerState.FrontCounterClockwise = desc.RasterizerState.FrontCounterClockwise ? TRUE : FALSE;
						pd.RasterizerState.MultisampleEnable = desc.RasterizerState.Multisample ? TRUE : FALSE;
						pd.RasterizerState.SlopeScaledDepthBias = desc.RasterizerState.SlopeScaledDepthBias;

						pd.SampleDesc.Count = desc.SampleDesc.Count;
						pd.SampleDesc.Quality = desc.SampleDesc.Quality;
						pd.SampleMask = desc.SampleMask;

						D3D12_SO_DECLARATION_ENTRY* soe = NewArray(D3D12_SO_DECLARATION_ENTRY, desc.StreamOutput.NumEntries);
						for (size_t a = 0; a < desc.StreamOutput.NumEntries; a++)
						{
							soe[a].ComponentCount = desc.StreamOutput.Entries[a].ComponentCount;
							soe[a].OutputSlot = desc.StreamOutput.Entries[a].OutputSlot;
							soe[a].SemanticIndex = desc.StreamOutput.Entries[a].SemanticIndex;
							soe[a].SemanticName = desc.StreamOutput.Entries[a].SemanticName;
							soe[a].StartComponent = desc.StreamOutput.Entries[a].StartComponent;
							soe[a].Stream = desc.StreamOutput.Entries[a].Stream;
						}

						pd.StreamOutput.NumEntries = desc.StreamOutput.NumEntries;
						pd.StreamOutput.NumStrides = desc.StreamOutput.NumStrides;
						pd.StreamOutput.pSODeclaration = soe;
						pd.StreamOutput.pBufferStrides = desc.StreamOutput.BufferStrides;
						pd.StreamOutput.RasterizedStream = desc.StreamOutput.RasterizedStreamIndex;

						HRESULT hRet = device->CreateGraphicsPipelineState(pd, CE_QUERY_ARGS(pso));
						DeleteArray(soe);
						DeleteArray(inp);
						return hRet;
					}
					*/
					class TempAllocator {
						private:
							API::List<void*> m_mem;
						public:
							CLOAK_CALL TempAllocator() {}
							CLOAK_CALL ~TempAllocator()
							{
								for (size_t a = 0; a < m_mem.size(); a++) { API::Global::Memory::MemoryPool::Free(m_mem[a]); }
							}
							void* CLOAK_CALL_THIS Allocate(In size_t size)
							{
								void* r = API::Global::Memory::MemoryPool::Allocate(size);
								m_mem.push_back(r);
								return r;
							}
					};
					template<API::Rendering::PSO_SUBTYPE T> struct ConditionalFill { };
					template<API::Rendering::PSO_SUBTYPE T> struct FillShader
					{
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_SHADER_BYTECODE* pd)
						{
							if (desc.Contains<T>())
							{
								const auto& b = desc.Get<T>();
								if (b.Length > 0)
								{
									pd->BytecodeLength = b.Length;
									pd->pShaderBytecode = b.Code;
								}
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_CS> : private FillShader<API::Rendering::PSO_SUBTYPE_CS> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd) { FillShader::Fill(desc, &pd->CS); }
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_PS> : private FillShader<API::Rendering::PSO_SUBTYPE_PS> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd) { FillShader::Fill(desc, &pd->PS); }
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_DS> : private FillShader<API::Rendering::PSO_SUBTYPE_DS> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd) { FillShader::Fill(desc, &pd->DS); }
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_HS> : private FillShader<API::Rendering::PSO_SUBTYPE_HS> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd) { FillShader::Fill(desc, &pd->HS); }
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_GS> : private FillShader<API::Rendering::PSO_SUBTYPE_GS> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd) { FillShader::Fill(desc, &pd->GS); }
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_VS> : private FillShader<API::Rendering::PSO_SUBTYPE_VS> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd) { FillShader::Fill(desc, &pd->VS); }
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_STREAM_OUTPUT> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd, In TempAllocator* alloc)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_STREAM_OUTPUT>())
							{
								const auto& b = desc.Get<API::Rendering::PSO_SUBTYPE_STREAM_OUTPUT>();
								
								D3D12_SO_DECLARATION_ENTRY* soe = reinterpret_cast<D3D12_SO_DECLARATION_ENTRY*>(alloc->Allocate(sizeof(D3D12_SO_DECLARATION_ENTRY) * b.NumEntries));
								for (size_t a = 0; a < b.NumEntries; a++)
								{
									soe[a].ComponentCount	= b.Entries[a].ComponentCount;
									soe[a].OutputSlot		= b.Entries[a].OutputSlot;
									soe[a].SemanticIndex	= b.Entries[a].SemanticIndex;
									soe[a].SemanticName		= b.Entries[a].SemanticName;
									soe[a].StartComponent	= b.Entries[a].StartComponent;
									soe[a].Stream			= b.Entries[a].Stream;
								}
								pd->StreamOutput->NumEntries = b.NumEntries;
								pd->StreamOutput->NumStrides = b.NumStrides;
								pd->StreamOutput->pSODeclaration = soe;
								pd->StreamOutput->pBufferStrides = b.BufferStrides;
								pd->StreamOutput->RasterizedStream = b.RasterizedStreamIndex;
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_BLEND> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd, In size_t numRenderTargets)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_BLEND>())
							{
								const auto& b = desc.Get<API::Rendering::PSO_SUBTYPE_BLEND>();
								pd->BlendState->AlphaToCoverageEnable = b.AlphaToCoverage ? TRUE : FALSE;
								pd->BlendState->IndependentBlendEnable = b.IndependentBlend ? TRUE : FALSE;
								for (size_t c = 0; c < numRenderTargets; c++)
								{
									pd->BlendState->RenderTarget[c].BlendEnable				= b.RenderTarget[c].BlendEnable ? TRUE : FALSE;
									pd->BlendState->RenderTarget[c].LogicOpEnable			= b.RenderTarget[c].LogicOperationEnable ? TRUE : FALSE;
									pd->BlendState->RenderTarget[c].SrcBlend				= Casting::CastForward(b.RenderTarget[c].SrcBlend);
									pd->BlendState->RenderTarget[c].DestBlend				= Casting::CastForward(b.RenderTarget[c].DstBlend);
									pd->BlendState->RenderTarget[c].SrcBlendAlpha			= Casting::CastForward(b.RenderTarget[c].SrcBlendAlpha);
									pd->BlendState->RenderTarget[c].DestBlendAlpha			= Casting::CastForward(b.RenderTarget[c].DstBlendAlpha);
									pd->BlendState->RenderTarget[c].BlendOp					= Casting::CastForward(b.RenderTarget[c].BlendOperation);
									pd->BlendState->RenderTarget[c].BlendOpAlpha			= Casting::CastForward(b.RenderTarget[c].BlendOperationAlpha);
									pd->BlendState->RenderTarget[c].LogicOp					= Casting::CastForward(b.RenderTarget[c].LogicOperation);
									pd->BlendState->RenderTarget[c].RenderTargetWriteMask	= Casting::CastForward(b.RenderTarget[c].WriteMask);
								}
								for (size_t c = numRenderTargets; c < ARRAYSIZE(pd->BlendState->RenderTarget); c++)
								{
									pd->BlendState->RenderTarget[c].BlendEnable = FALSE;
									pd->BlendState->RenderTarget[c].LogicOpEnable = FALSE;
									pd->BlendState->RenderTarget[c].SrcBlend = D3D12_BLEND_ZERO;
									pd->BlendState->RenderTarget[c].DestBlend = D3D12_BLEND_ZERO;
									pd->BlendState->RenderTarget[c].SrcBlendAlpha = D3D12_BLEND_ZERO;
									pd->BlendState->RenderTarget[c].DestBlendAlpha = D3D12_BLEND_ZERO;
									pd->BlendState->RenderTarget[c].BlendOp = D3D12_BLEND_OP_ADD;
									pd->BlendState->RenderTarget[c].BlendOpAlpha = D3D12_BLEND_OP_ADD;
									pd->BlendState->RenderTarget[c].LogicOp = D3D12_LOGIC_OP_NOOP;
									pd->BlendState->RenderTarget[c].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
								}
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_SAMPLE_MASK> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_SAMPLE_MASK>())
							{
								pd->SampleMask = desc.Get<API::Rendering::PSO_SUBTYPE_SAMPLE_MASK>();
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_RASTERIZER> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_RASTERIZER>())
							{
								const auto& b = desc.Get< API::Rendering::PSO_SUBTYPE_RASTERIZER>();

								pd->RasterizerState->AntialiasedLineEnable		= b.AntialiasedLine ? TRUE : FALSE;
								pd->RasterizerState->ConservativeRaster			= Casting::CastForward(b.ConservativeRasterization);
								pd->RasterizerState->CullMode					= Casting::CastForward(b.CullMode);
								pd->RasterizerState->FillMode					= Casting::CastForward(b.FillMode);
								pd->RasterizerState->DepthBias					= b.DepthBias;
								pd->RasterizerState->DepthBiasClamp				= b.DepthBiasClamp;
								pd->RasterizerState->DepthClipEnable			= b.DepthClip ? TRUE : FALSE;
								pd->RasterizerState->ForcedSampleCount			= b.ForcedSampleCount;
								pd->RasterizerState->FrontCounterClockwise		= b.FrontCounterClockwise ? TRUE : FALSE;
								pd->RasterizerState->MultisampleEnable			= b.Multisample ? TRUE : FALSE;
								pd->RasterizerState->SlopeScaledDepthBias		= b.SlopeScaledDepthBias;
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_DEPTH_STENCIL> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd, In const API::Rendering::Hardware& hardware)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_DEPTH_STENCIL>())
							{
								const auto& b = desc.Get<API::Rendering::PSO_SUBTYPE_DEPTH_STENCIL>();

								CloakCheckOK(b.DepthBoundsTests == false || hardware.AllowDepthBoundsTests == true, API::Global::Debug::Error::GRAPHIC_PSO_CREATION, false, "The Hardware does not support Depth Bounds Tests");

								pd->DepthStencilState->BackFace.StencilDepthFailOp		= Casting::CastForward(b.BackFace.DepthFailOperation);
								pd->DepthStencilState->BackFace.StencilFailOp			= Casting::CastForward(b.BackFace.StencilFailOperation);
								pd->DepthStencilState->BackFace.StencilFunc				= Casting::CastForward(b.BackFace.StencilFunction);
								pd->DepthStencilState->BackFace.StencilPassOp			= Casting::CastForward(b.BackFace.PassOperation);
								
								pd->DepthStencilState->FrontFace.StencilDepthFailOp		= Casting::CastForward(b.FrontFace.DepthFailOperation);
								pd->DepthStencilState->FrontFace.StencilFailOp			= Casting::CastForward(b.FrontFace.StencilFailOperation);
								pd->DepthStencilState->FrontFace.StencilFunc			= Casting::CastForward(b.FrontFace.StencilFunction);
								pd->DepthStencilState->FrontFace.StencilPassOp			= Casting::CastForward(b.FrontFace.PassOperation);
								
								pd->DepthStencilState->DepthEnable						= b.DepthEnable ? TRUE : FALSE;
								pd->DepthStencilState->DepthFunc						= Casting::CastForward(b.DepthFunction);
								pd->DepthStencilState->DepthWriteMask					= Casting::CastForward(b.DepthWriteMask);
								pd->DepthStencilState->StencilEnable					= b.StencilEnable ? TRUE : FALSE;
								pd->DepthStencilState->StencilReadMask					= static_cast<UINT8>(b.StencilReadMask);
								pd->DepthStencilState->StencilWriteMask					= static_cast<UINT8>(b.StencilWriteMask);

								pd->DepthStencilState->DepthBoundsTestEnable			= hardware.AllowDepthBoundsTests == true && b.DepthBoundsTests == true ? TRUE : FALSE;

								pd->DSVFormat = Casting::CastForward(b.Format);
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_INPUT_LAYOUT> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd, In TempAllocator* alloc)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_INPUT_LAYOUT>())
							{
								const auto& b = desc.Get< API::Rendering::PSO_SUBTYPE_INPUT_LAYOUT>();

								const API::Rendering::INPUT_ELEMENT_DESC* input = nullptr;
								UINT inputSize = 0;
								switch (b.Type)
								{
									case API::Rendering::INPUT_LAYOUT_TYPE_NONE:
										input = nullptr;
										inputSize = 0;
										break;
									case API::Rendering::INPUT_LAYOUT_TYPE_MESH:
										input = Impl::Rendering::InputElementDescription::MESH;
										inputSize = ARRAYSIZE(Impl::Rendering::InputElementDescription::MESH);
										break;
									case API::Rendering::INPUT_LAYOUT_TYPE_MESH_POS:
										input = Impl::Rendering::InputElementDescription::MESH_POS;
										inputSize = ARRAYSIZE(Impl::Rendering::InputElementDescription::MESH_POS);
										break;
									case API::Rendering::INPUT_LAYOUT_TYPE_GUI:
										input = Impl::Rendering::InputElementDescription::GUI;
										inputSize = ARRAYSIZE(Impl::Rendering::InputElementDescription::GUI);
										break;
									case API::Rendering::INPUT_LAYOUT_TYPE_TERRAIN:
										input = Impl::Rendering::InputElementDescription::TERRAIN;
										inputSize = ARRAYSIZE(Impl::Rendering::InputElementDescription::TERRAIN);
										break;
									case API::Rendering::INPUT_LAYOUT_TYPE_CUSTOM:
										input = b.Custom.Elements;
										inputSize = b.Custom.NumElements;
										break;
									default:
										CloakError(API::Global::Debug::Error::GRAPHIC_PSO_CREATION, false, "No corret input layout type was given");
										return;
								}
								
								D3D12_INPUT_ELEMENT_DESC* inp = reinterpret_cast<D3D12_INPUT_ELEMENT_DESC*>(alloc->Allocate(sizeof(D3D12_INPUT_ELEMENT_DESC) * inputSize));
								UINT slotOffset[16];
								for (size_t a = 0; a < ARRAYSIZE(slotOffset); a++) { slotOffset[a] = 0; }
								for (UINT a = 0; a < inputSize; a++)
								{
									if (input[a].InputSlot >= ARRAYSIZE(slotOffset)) { CloakError(API::Global::Debug::Error::GRAPHIC_PSO_CREATION, false, "Input Slot must not be larger then 15"); return; }
									const size_t is = GetFormatSize(input[a].Format);
									const UINT offset = slotOffset[input[a].InputSlot] + input[a].Offset;
									UINT remSize = offset % PACKING_SIZE;
									if (remSize == 0) { remSize = PACKING_SIZE; }
									if (is > remSize)
									{
										CloakError(API::Global::Debug::Error::GRAPHIC_PSO_CREATION, false, "Packing would require unspecified padding!");
										return;
									}

									inp[a].AlignedByteOffset = offset;
									inp[a].Format = Casting::CastForward(input[a].Format);
									inp[a].InputSlot = input[a].InputSlot;
									inp[a].InputSlotClass = Casting::CastForward(input[a].InputSlotClass);
									inp[a].InstanceDataStepRate = input[a].InstanceDataStepRate;
									inp[a].SemanticIndex = input[a].SemanticIndex;
									inp[a].SemanticName = input[a].SemanticName;

									slotOffset[input[a].InputSlot] = static_cast<UINT>(offset + is);
								}

								pd->InputLayout->pInputElementDescs = inp;
								pd->InputLayout->NumElements = static_cast<UINT>(inputSize);
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT>())
							{
								pd->IBStripCutValue = Casting::CastForward(desc.Get<API::Rendering::PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT>());
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>())
							{
								pd->PrimitiveTopologyType = Casting::CastForward(desc.Get<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>());
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_RENDER_TARGETS> {
						CLOAK_FORCEINLINE static size_t CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_RENDER_TARGETS>())
							{
								const auto& b = desc.Get<API::Rendering::PSO_SUBTYPE_RENDER_TARGETS>();
								if (b.NumRenderTargets > API::Rendering::MAX_RENDER_TARGET || b.NumRenderTargets > ARRAYSIZE(pd->RTVFormats->RTFormats))
								{
									CloakError(API::Global::Debug::Error::GRAPHIC_PSO_CREATION, false, "Given number of render targets exeeds maximum");
									return 0;
								}
								pd->RTVFormats->NumRenderTargets = b.NumRenderTargets;
								for (size_t c = 0; c < API::Rendering::MAX_RENDER_TARGET && c < b.NumRenderTargets; c++) { pd->RTVFormats->RTFormats[c] = Casting::CastForward(b.RTVFormat[c]); }
								for (size_t c = b.NumRenderTargets; c < ARRAYSIZE(pd->RTVFormats->RTFormats); c++) { pd->RTVFormats->RTFormats[c] = DXGI_FORMAT_UNKNOWN; }
								return b.NumRenderTargets;
							}
							return 0;
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_SAMPLE_DESC> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_SAMPLE_DESC>())
							{
								const auto& b = desc.Get<API::Rendering::PSO_SUBTYPE_SAMPLE_DESC>();
								pd->SampleDesc->Count = b.Count;
								pd->SampleDesc->Quality = b.Quality;
							}
						}
					};
					template<> struct ConditionalFill<API::Rendering::PSO_SUBTYPE_VIEW_INSTANCING> {
						CLOAK_FORCEINLINE static void CLOAK_CALL Fill(In const API::Rendering::PIPELINE_STATE_DESC& desc, Out D3D12_PIPELINE_STATE_DESC* pd, In const API::Rendering::Hardware& hardware, In TempAllocator* alloc)
						{
							if (desc.Contains<API::Rendering::PSO_SUBTYPE_VIEW_INSTANCING>())
							{
								const auto& b = desc.Get<API::Rendering::PSO_SUBTYPE_VIEW_INSTANCING>();
								if (hardware.ViewInstanceSupport != API::Rendering::VIEW_INSTANCE_NOT_SUPPORTED)
								{
									if (b.NumInstances > D3D12_MAX_VIEW_INSTANCE_COUNT)
									{
										CloakError(API::Global::Debug::Error::GRAPHIC_PSO_CREATION, false, "Number of View Instances exeeds limits");
										return;
									}
									if (b.NumInstances > 1)
									{
										D3D12_VIEW_INSTANCE_LOCATION* loc = reinterpret_cast<D3D12_VIEW_INSTANCE_LOCATION*>(alloc->Allocate(sizeof(D3D12_VIEW_INSTANCE_LOCATION) * b.NumInstances));
										for (size_t a = 0; a < b.NumInstances; a++)
										{
											loc[a].RenderTargetArrayIndex = b.Location[a].RenderTargetIndex;
											loc[a].ViewportArrayIndex = b.Location[a].ViewportIndex;
										}
										pd->ViewInstancingDesc->pViewInstanceLocations = loc;
										pd->ViewInstancingDesc->ViewInstanceCount = b.NumInstances;
										pd->ViewInstancingDesc->Flags = Casting::CastForward(b.Flags);
									}
								}
								else if (b.NumInstances > 1)
								{
									CloakError(API::Global::Debug::Error::GRAPHIC_PSO_CREATION, false, "The Hardware does not support View Instancing");
									return;
								}
							}
						}
					};
					HRESULT CLOAK_CALL CreatePSO(In Device* device, In UINT nodeMask, In const API::Rendering::PIPELINE_STATE_DESC& desc, In ID3D12RootSignature* root, Out ID3D12PipelineState** pso, In const API::Rendering::Hardware& hardware)
					{
						CLOAK_ASSUME(pso != nullptr);
						if (root == nullptr) { return E_FAIL; }

						TempAllocator alloc;

						D3D12_PIPELINE_STATE_DESC pd;
						pd.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
						pd.NodeMask = nodeMask;
						pd.RootSignature = root;

						ConditionalFill<API::Rendering::PSO_SUBTYPE_CS>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_VS>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_HS>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_DS>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_GS>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_PS>::Fill(desc, &pd);

						const size_t NumRenderTargets = ConditionalFill<API::Rendering::PSO_SUBTYPE_RENDER_TARGETS>::Fill(desc, &pd);
						
						ConditionalFill<API::Rendering::PSO_SUBTYPE_BLEND>::Fill(desc, &pd, NumRenderTargets);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_DEPTH_STENCIL>::Fill(desc, &pd, hardware);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_INDEX_BUFFER_STRIP_CUT>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_PRIMITIVE_TOPOLOGY>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_INPUT_LAYOUT>::Fill(desc, &pd, &alloc);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_RASTERIZER>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_SAMPLE_DESC>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_SAMPLE_MASK>::Fill(desc, &pd);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_STREAM_OUTPUT>::Fill(desc, &pd, &alloc);
						ConditionalFill<API::Rendering::PSO_SUBTYPE_VIEW_INSTANCING>::Fill(desc, &pd, hardware, &alloc);

						HRESULT hRet = device->CreatePipelineState(pd, CE_QUERY_ARGS(pso));
						CLOAK_ASSUME(SUCCEEDED(hRet));
						return hRet;
					}

					HRESULT CLOAK_CALL CreatePSO(In IManager* clm, In const API::Rendering::PIPELINE_STATE_DESC& desc, In IRootSignature* root, Out ID3D12PipelineState** pso)
					{
						HRESULT hRet = E_FAIL;
						if (pso != nullptr)
						{
							*pso = nullptr;
							RootSignature* r = nullptr;
							if (root != nullptr && SUCCEEDED(root->QueryInterface(CE_QUERY_ARGS(&r))))
							{
								const size_t nodeCount = clm->GetNodeCount();
								const UINT nodeMask = nodeCount == 1 ? 0 : ((1 << nodeCount) - 1);
								IDevice* dev = clm->GetDevice();
								Device* dxDev = nullptr;
								if (SUCCEEDED(dev->QueryInterface(CE_QUERY_ARGS(&dxDev))))
								{
									hRet = CreatePSO(dxDev, nodeMask, desc, r->GetSignature(), pso, clm->GetHardwareSetting());
									//if (desc.Type == API::Rendering::PIPELINE_STATE_TYPE_COMPUTE) { hRet = CreateComputePSO(dxDev, nodeMask, desc.Compute, r->GetSignature(), shader, pso); }
									//else { hRet = CreateGraphicPSO(dxDev, nodeMask, desc.Graphic, r->GetSignature(), shader, pso); }
								}
								RELEASE(dxDev);
								SAVE_RELEASE(dev);
							}
							SAVE_RELEASE(r);
						}
						return hRet;
					}

					Ret_notnull PipelineState* CLOAK_CALL PipelineState::Create(In const IManager* manager)
					{
						return new PipelineState(manager);
					}

					CLOAK_CALL PipelineState::PipelineState(In const IManager* manager)
					{
						m_pso = nullptr;
						m_root = manager->CreateRootSignature();
						m_useTess = false;
						DEBUG_NAME(PipelineState);
					}
					CLOAK_CALL PipelineState::~PipelineState()
					{
						SAVE_RELEASE(m_root);
						SAVE_RELEASE(m_pso);
					}

					bool CLOAK_CALL_THIS PipelineState::Reset(In const API::Rendering::PIPELINE_STATE_DESC& desc, In const API::Rendering::RootDescription& root)
					{
						ID3D12PipelineState* nPSO = nullptr;
						HRESULT hRet = S_OK;
						m_useTess = false;

						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						if (SUCCEEDED(hRet))
						{
							const bool vHD = (desc.Contains<API::Rendering::PSO_SUBTYPE_DS>() == true) == (desc.Contains<API::Rendering::PSO_SUBTYPE_HS>() == true);
							const bool vCP = (desc.Contains<API::Rendering::PSO_SUBTYPE_CS>() == true) != (desc.Contains<API::Rendering::PSO_SUBTYPE_PS>() == true);
							const bool vVP = (desc.Contains<API::Rendering::PSO_SUBTYPE_PS>() == true) == (desc.Contains<API::Rendering::PSO_SUBTYPE_VS>() == true);
							const bool vGP =  desc.Contains<API::Rendering::PSO_SUBTYPE_GS>() == false ||  desc.Contains<API::Rendering::PSO_SUBTYPE_PS>() == true;
							const bool vHP =  desc.Contains<API::Rendering::PSO_SUBTYPE_HS>() == false ||  desc.Contains<API::Rendering::PSO_SUBTYPE_PS>() == true;
							if ((vHD && vCP && vVP && vGP && vHP) == false)
							{
								hRet = E_FAIL;
							}
						}
						if (SUCCEEDED(hRet))
						{
							m_type = desc.Contains<API::Rendering::PSO_SUBTYPE_CS>() == true ? PIPELINE_STATE_TYPE_COMPUTE : (desc.Contains<API::Rendering::PSO_SUBTYPE_PS>() == true ? PIPELINE_STATE_TYPE_GRAPHIC : PIPELINE_STATE_TYPE_NONE);
							m_useTess = m_type == PIPELINE_STATE_TYPE_GRAPHIC && desc.Contains<API::Rendering::PSO_SUBTYPE_DS>() == true;
							m_root->Finalize(root);
							hRet = CreatePSO(clm, desc, m_root, &nPSO);
							if (CloakCheckError(hRet, API::Global::Debug::Error::GRAPHIC_PSO_CREATION, false)) { RELEASE(nPSO); }
						}

						ID3D12PipelineState* oPSO = m_pso;
						m_pso = nPSO;
						RELEASE(oPSO);
						return SUCCEEDED(hRet);
					}
					bool CLOAK_CALL_THIS PipelineState::Reset(In const API::Rendering::SHADER_SET& shader, In size_t dataSize, In_reads_bytes(dataSize) const void* data)
					{
						bool r = false;
						if (CloakDebugCheckOK(dataSize >= Impl::Rendering::PS_SERIIALIZATION_HEADER_SIZE && data != nullptr, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true))
						{
							API::Rendering::PIPELINE_STATE_DESC pso;
							API::Rendering::RootDescription root;
							Impl::Rendering::Deserializer des(data);

							uint8_t cid = 0;
							bool suc = true;
							//Read Header:
							des.DeserializeData(&cid);
							suc = suc && cid == 'C';
							des.DeserializeData(&cid);
							suc = suc && cid == 'E';
							des.DeserializeData(&cid);
							suc = suc && cid == 'P';
							des.DeserializeData(&cid);
							suc = suc && cid == 'S';
							des.DeserializeData(&cid);
							suc = suc && cid == '\n';
							if (CloakCheckError(suc, API::Global::Debug::Error::SERIALIZE_ERROR, true)) { return false; }
							uint32_t version = 0;
							des.DeserializeData(&version);
							if (CloakCheckError(version == 1002, API::Global::Debug::Error::SERIALIZE_ERROR, true)) { return false; }
							uint32_t psoS = 0;
							des.DeserializeData(&psoS);
							uint32_t rootS = 0;
							des.DeserializeData(&rootS);
							if (CloakCheckError(psoS + rootS + Impl::Rendering::PS_SERIIALIZATION_HEADER_SIZE <= dataSize, API::Global::Debug::Error::SERIALIZE_ERROR, true)) { return false; }
							if (CloakCheckError(des.GetPos() == Impl::Rendering::PS_SERIIALIZATION_HEADER_SIZE, API::Global::Debug::Error::SERIALIZE_ERROR, true)) { return false; }

							//Read PSO:
							//TODO !!!
							/*
							des.DeserializeData(&pso.Type);
							switch (pso.Type)
							{
								case API::Rendering::PIPELINE_STATE_TYPE_COMPUTE:
								{
									pso.Compute.CS = shader.CS;
									break;
								}
								case API::Rendering::PIPELINE_STATE_TYPE_GRAPHIC:
								{
									pso.Graphic.VS = shader.VS;
									pso.Graphic.PS = shader.PS;
									pso.Graphic.DS = shader.DS;
									pso.Graphic.HS = shader.HS;
									pso.Graphic.GS = shader.GS;

									uint8_t boolMask = 0;
									std::string tempStr;
									//Stream Output:
									des.DeserializeData(&pso.Graphic.StreamOutput.NumEntries);
									des.DeserializeData(&pso.Graphic.StreamOutput.NumStrides);
									des.DeserializeData(&pso.Graphic.StreamOutput.RasterizedStreamIndex);
									API::Rendering::STREAM_OUTPUT_ENTRY_DESC* sen = NewArray(API::Rendering::STREAM_OUTPUT_ENTRY_DESC, pso.Graphic.StreamOutput.NumEntries);
									pso.Graphic.StreamOutput.Entries = sen;
									for (size_t a = 0; a < pso.Graphic.StreamOutput.NumEntries; a++)
									{
										des.DeserializeData(&sen[a].ComponentCount);
										des.DeserializeData(&sen[a].OutputSlot);
										des.DeserializeData(&sen[a].SemanticIndex);
										des.DeserializeData(&sen[a].SemanticName);
										des.DeserializeData(&sen[a].StartComponent);
										des.DeserializeData(&sen[a].Stream);
									}
									UINT* sst = NewArray(UINT, pso.Graphic.StreamOutput.NumStrides);
									pso.Graphic.StreamOutput.BufferStrides = sst;
									for (size_t a = 0; a < pso.Graphic.StreamOutput.NumStrides; a++)
									{
										des.DeserializeData(&sst[a]);
									}

									//Blend State
									des.DeserializeData(&boolMask);
									pso.Graphic.BlendState.IndependentBlend = (boolMask & 1) != 0;
									pso.Graphic.BlendState.AlphaToCoverage = (boolMask & 2) != 0;
									for (size_t a = 0; a < API::Rendering::MAX_RENDER_TARGET; a++)
									{
										des.DeserializeData(&boolMask);
										pso.Graphic.BlendState.RenderTarget[a].LogicOperationEnable = (boolMask & 1) != 0;
										pso.Graphic.BlendState.RenderTarget[a].BlendEnable = (boolMask & 2) != 0;
										des.DeserializeData(&pso.Graphic.BlendState.RenderTarget[a].SrcBlend);
										des.DeserializeData(&pso.Graphic.BlendState.RenderTarget[a].DstBlend);
										des.DeserializeData(&pso.Graphic.BlendState.RenderTarget[a].BlendOperation);
										des.DeserializeData(&pso.Graphic.BlendState.RenderTarget[a].SrcBlendAlpha);
										des.DeserializeData(&pso.Graphic.BlendState.RenderTarget[a].DstBlendAlpha);
										des.DeserializeData(&pso.Graphic.BlendState.RenderTarget[a].BlendOperationAlpha);
										des.DeserializeData(&pso.Graphic.BlendState.RenderTarget[a].LogicOperation);
										des.DeserializeData(&pso.Graphic.BlendState.RenderTarget[a].WriteMask);
									}

									//Sample Mask
									des.DeserializeData(&pso.Graphic.SampleMask);

									//Rasterizer State
									des.DeserializeData(&boolMask);
									pso.Graphic.RasterizerState.FrontCounterClockwise = (boolMask & 1) != 0;
									pso.Graphic.RasterizerState.DepthClip = (boolMask & 2) != 0;
									pso.Graphic.RasterizerState.Multisample = (boolMask & 4) != 0;
									pso.Graphic.RasterizerState.AntialiasedLine = (boolMask & 8) != 0;
									des.DeserializeData(&pso.Graphic.RasterizerState.FillMode);
									des.DeserializeData(&pso.Graphic.RasterizerState.CullMode);
									des.DeserializeData(&pso.Graphic.RasterizerState.DepthBias);
									des.DeserializeData(&pso.Graphic.RasterizerState.DepthBiasClamp);
									des.DeserializeData(&pso.Graphic.RasterizerState.SlopeScaledDepthBias);
									des.DeserializeData(&pso.Graphic.RasterizerState.ForcedSampleCount);
									des.DeserializeData(&pso.Graphic.RasterizerState.ConservativeRasterization);

									//Depth Stencil State
									des.DeserializeData(&boolMask);
									pso.Graphic.DepthStencilState.DepthEnable = (boolMask & 1) != 0;
									pso.Graphic.DepthStencilState.StencilEnable = (boolMask & 2) != 0;
									des.DeserializeData(&pso.Graphic.DepthStencilState.DepthWriteMask);
									des.DeserializeData(&pso.Graphic.DepthStencilState.DepthFunction);
									des.DeserializeData(&pso.Graphic.DepthStencilState.StencilReadMask);
									des.DeserializeData(&pso.Graphic.DepthStencilState.StencilWriteMask);
									des.DeserializeData(&pso.Graphic.DepthStencilState.FrontFace.StencilFailOperation);
									des.DeserializeData(&pso.Graphic.DepthStencilState.FrontFace.DepthFailOperation);
									des.DeserializeData(&pso.Graphic.DepthStencilState.FrontFace.PassOperation);
									des.DeserializeData(&pso.Graphic.DepthStencilState.FrontFace.StencilFunction);
									des.DeserializeData(&pso.Graphic.DepthStencilState.BackFace.StencilFailOperation);
									des.DeserializeData(&pso.Graphic.DepthStencilState.BackFace.DepthFailOperation);
									des.DeserializeData(&pso.Graphic.DepthStencilState.BackFace.PassOperation);
									des.DeserializeData(&pso.Graphic.DepthStencilState.BackFace.StencilFunction);

									//Input Layout
									des.DeserializeData(&pso.Graphic.InputLayout.Type);
									if (pso.Graphic.InputLayout.Type == API::Rendering::INPUT_LAYOUT_TYPE_CUSTOM)
									{
										des.DeserializeData(&pso.Graphic.InputLayout.Custom.NumElements);
										API::Rendering::INPUT_ELEMENT_DESC* ie = NewArray(API::Rendering::INPUT_ELEMENT_DESC, pso.Graphic.InputLayout.Custom.NumElements);
										pso.Graphic.InputLayout.Custom.Elements = ie;
										for (size_t a = 0; a < pso.Graphic.InputLayout.Custom.NumElements; a++)
										{
											des.DeserializeData(&ie[a].SemanticName);
											des.DeserializeData(&ie[a].SemanticIndex);
											des.DeserializeData(&ie[a].Format);
											des.DeserializeData(&ie[a].Offset);
											des.DeserializeData(&ie[a].InputSlot);
											des.DeserializeData(&ie[a].InputSlotClass);
											des.DeserializeData(&ie[a].InstanceDataStepRate);
										}
									}
									else
									{
										pso.Graphic.InputLayout.Custom.Elements = nullptr;
										pso.Graphic.InputLayout.Custom.NumElements = 0;
									}

									//Index Buffer Strip Cut Value
									des.DeserializeData(&pso.Graphic.IndexBufferStripCutValue);

									//Primitive Topology Type
									des.DeserializeData(&pso.Graphic.PrimitiveTopologyType);

									//Num Render Targets
									des.DeserializeData(&pso.Graphic.NumRenderTargets);

									//Formats
									for (size_t a = 0; a < pso.Graphic.NumRenderTargets; a++) { des.DeserializeData(&pso.Graphic.RTVFormat[a]); }
									des.DeserializeData(&pso.Graphic.DSVFormat);

									//Sample Desc
									des.DeserializeData(&pso.Graphic.SampleDesc.Count);
									des.DeserializeData(&pso.Graphic.SampleDesc.Quality);
									break;
								}
								default:
									break;
							}
							if (CloakCheckOK(des.GetPos() == Impl::Rendering::PS_SERIIALIZATION_HEADER_SIZE + psoS, API::Global::Debug::Error::SERIALIZE_ERROR, true))
							{
								//Read Root:
								API::Rendering::ROOT_SIGNATURE_FLAG rootFlag;
								des.DeserializeData(&rootFlag);
								uint32_t npc = 0;
								uint32_t nsc = 0;
								des.DeserializeData(&npc);
								des.DeserializeData(&nsc);
								root.Resize(npc, nsc);
								root.SetFlags(rootFlag);
								for (size_t a = 0; a < npc; a++)
								{
									API::Rendering::ROOT_PARAMETER_TYPE type;
									API::Rendering::SHADER_VISIBILITY visible;
									des.DeserializeData(&type);
									des.DeserializeData(&visible);
									switch (type)
									{
										case CloakEngine::API::Rendering::ROOT_PARAMETER_TYPE_TABLE:
										{
											UINT s = 0;
											des.DeserializeData(&s);
											API::Rendering::DESCRIPTOR_RANGE_ENTRY* entries = NewArray(API::Rendering::DESCRIPTOR_RANGE_ENTRY, s);
											for (size_t b = 0; b < s; b++)
											{
												des.DeserializeData(&entries[b].Type);
												des.DeserializeData(&entries[b].RegisterID);
												des.DeserializeData(&entries[b].Space);
												des.DeserializeData(&entries[b].Count);
												des.DeserializeData(&entries[b].Flags);
											}


											root[a].SetAsTable(s, entries, visible);
											DeleteArray(entries);
											break;
										}
										case CloakEngine::API::Rendering::ROOT_PARAMETER_TYPE_CONSTANTS:
										{
											UINT regID;
											UINT space;
											UINT numDWords;
											des.DeserializeData(&regID);
											des.DeserializeData(&space);
											des.DeserializeData(&numDWords);

											root[a].SetAsConstants(regID, numDWords, visible, space);
											break;
										}
										case CloakEngine::API::Rendering::ROOT_PARAMETER_TYPE_CBV:
										{
											UINT regID;
											UINT space;
											API::Rendering::ROOT_PARAMETER_FLAG flags;
											des.DeserializeData(&regID);
											des.DeserializeData(&space);
											des.DeserializeData(&flags);

											root[a].SetAsRootCBV(regID, visible, space, flags);
											break;
										}
										case CloakEngine::API::Rendering::ROOT_PARAMETER_TYPE_SRV:
										{
											UINT regID;
											UINT space;
											API::Rendering::ROOT_PARAMETER_FLAG flags;
											des.DeserializeData(&regID);
											des.DeserializeData(&space);
											des.DeserializeData(&flags);

											root[a].SetAsRootSRV(regID, visible, space, flags);
											break;
										}
										case CloakEngine::API::Rendering::ROOT_PARAMETER_TYPE_UAV:
										{
											UINT regID;
											UINT space;
											API::Rendering::ROOT_PARAMETER_FLAG flags;
											des.DeserializeData(&regID);
											des.DeserializeData(&space);
											des.DeserializeData(&flags);

											root[a].SetAsRootUAV(regID, visible, space, flags);
											break;
										}
										case CloakEngine::API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER:
										{
											API::Rendering::SAMPLER_DESC desc;
											UINT regID;
											UINT space;
											des.DeserializeData(&regID);
											des.DeserializeData(&space);
											des.DeserializeData(&desc.Filter);
											des.DeserializeData(&desc.AddressU);
											des.DeserializeData(&desc.AddressV);
											des.DeserializeData(&desc.AddressW);
											des.DeserializeData(&desc.MipLODBias);
											des.DeserializeData(&desc.MaxAnisotropy);
											des.DeserializeData(&desc.CompareFunction);
											des.DeserializeData(&desc.BorderColor[0]);
											des.DeserializeData(&desc.BorderColor[1]);
											des.DeserializeData(&desc.BorderColor[2]);
											des.DeserializeData(&desc.BorderColor[3]);
											des.DeserializeData(&desc.MinLOD);
											des.DeserializeData(&desc.MaxLOD);

											root[a].SetAsAutomaticSampler(regID, desc, visible, space);
											break;
										}
										default:
											break;
									}
								}
								for (size_t a = 0; a < nsc; a++)
								{
									API::Rendering::STATIC_SAMPLER_DESC ssampler;
									des.DeserializeData(&ssampler.Filter);
									des.DeserializeData(&ssampler.AddressU);
									des.DeserializeData(&ssampler.AddressV);
									des.DeserializeData(&ssampler.AddressW);
									des.DeserializeData(&ssampler.MipLODBias);
									des.DeserializeData(&ssampler.MaxAnisotropy);
									des.DeserializeData(&ssampler.CompareFunction);
									des.DeserializeData(&ssampler.BorderColor);
									des.DeserializeData(&ssampler.MinLOD);
									des.DeserializeData(&ssampler.MaxLOD);
									des.DeserializeData(&ssampler.RegisterID);
									des.DeserializeData(&ssampler.Space);
									des.DeserializeData(&ssampler.Visible);
									root.SetStaticSampler(a, ssampler);
								}
								if (CloakCheckOK(psoS + rootS + 13 == des.GetPos(), API::Global::Debug::Error::SERIALIZE_ERROR, true))
								{
									r = Reset(pso, root);
								}
							}
							DeleteArray(pso.Graphic.StreamOutput.Entries);
							DeleteArray(pso.Graphic.StreamOutput.BufferStrides);
							DeleteArray(pso.Graphic.InputLayout.Custom.Elements);
						}
						return r;
						*/
						}
						return false;
					}
					bool CLOAK_CALL_THIS PipelineState::Reset(In const API::Global::Graphic::Settings& set, In API::Files::IShader* shader)
					{
						ID3D12PipelineState* nPSO = nullptr;
						HRESULT hRet = E_FAIL;
						m_useTess = false;

						Impl::Files::IShader* rs = nullptr;
						if (shader != nullptr && SUCCEEDED(shader->QueryInterface(CE_QUERY_ARGS(&rs))))
						{
							hRet = S_OK;
							IManager* clm = Engine::Graphic::Core::GetCommandListManager();

							API::Rendering::PIPELINE_STATE_DESC desc;
							API::Rendering::RootDescription root;
							//TODO: When using a shader, we don't want to use RootDescription, but rather a binary pre-compiled root singature!
							//TODO
							/*
							hRet = rs->GetShaderData(&desc, &root, set, m_shaderModel, clm->GetHardwareSetting()) ? S_OK : E_FAIL;
							if (SUCCEEDED(hRet) && ((desc.Type == API::Rendering::PIPELINE_STATE_TYPE_GRAPHIC && (desc.Graphic.VS.Model[m_shaderModel].Length == 0 || desc.Graphic.PS.Model[m_shaderModel].Length == 0 || (desc.Graphic.DS.Model[m_shaderModel].Length == 0) != (desc.Graphic.HS.Model[m_shaderModel].Length == 0))) || (desc.Type == API::Rendering::PIPELINE_STATE_TYPE_COMPUTE && desc.Compute.CS.Model[m_shaderModel].Length == 0))) { hRet = E_FAIL; }
							if (SUCCEEDED(hRet))
							{
								m_useTess = desc.Type == API::Rendering::PIPELINE_STATE_TYPE_GRAPHIC && desc.Graphic.DS.Model[m_shaderModel].Length > 0;
								m_type = desc.Type;
								m_root->Finalize(root);
								hRet = CreatePSO(clm, desc, m_root, m_shaderModel, &nPSO);
								if (FAILED(hRet)) { RELEASE(nPSO); }
							}
							*/
						}
						SAVE_RELEASE(rs);
						ID3D12PipelineState* oPSO = m_pso;
						m_pso = nPSO;
						RELEASE(oPSO);
						return SUCCEEDED(hRet);
					}
					IRootSignature* CLOAK_CALL_THIS PipelineState::GetRoot() const { return m_root; }
					bool CLOAK_CALL_THIS PipelineState::UseTessellation() const { return m_useTess; }
					PIPELINE_STATE_TYPE CLOAK_CALL_THIS PipelineState::GetType() const { return m_type; }
					Ret_maybenull ID3D12PipelineState* CLOAK_CALL_THIS PipelineState::GetPSO() const { return m_pso; }
					Success(return == true) bool CLOAK_CALL_THIS PipelineState::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						if (riid == __uuidof(API::Rendering::IPipelineState)) { *ptr = (API::Rendering::IPipelineState*)this; return true; }
						else RENDERING_QUERY_IMPL(ptr, riid, PipelineState, PipelineState_v1, SavePtr);
					}
				}
			}
		}
	}
}
#endif