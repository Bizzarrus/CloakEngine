#pragma once
#ifndef CE_IMPL_RENDERING_DX12_ROOTSIGNATURE_H
#define CE_IMPL_RENDERING_DX12_ROOTSIGNATURE_H

#include "Implementation/Rendering/RootSignature.h"
#include "Implementation\Rendering\Context.h"
#include "Implementation/Helper/SavePtr.h"
#include "Implementation/Rendering/DX12/Device.h"
#include "Implementation/Rendering/DX12/Sampler.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Task.h"

#include <d3d12.h>
#include <atomic>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace RootSignature_v1 {
					struct ROOT_PARAMETER_CACHE {
						API::Rendering::ROOT_PARAMETER_TYPE Type;
						D3D12_SHADER_VISIBILITY Visible;
						union {
							struct {
								UINT Start;
								UINT EntryCount;
								UINT FullSize;
							} Table;
							struct {
								UINT RegisterID;
								UINT Space;
								UINT NumDWords;
							} Constants;
							struct {
								UINT RegisterID;
								UINT Space;
								D3D12_ROOT_DESCRIPTOR_FLAGS Flags;
							} CBV;
							struct {
								UINT RegisterID;
								UINT Space;
								D3D12_ROOT_DESCRIPTOR_FLAGS Flags;
							} SRV;
							struct {
								UINT RegisterID;
								UINT Space;
								D3D12_ROOT_DESCRIPTOR_FLAGS Flags;
							} UAV;
							struct {
								UINT RegisterID;
								UINT Space;
								UINT Offset;
							} Sampler;
							struct {
								UINT RegisterID;
								UINT Space;
								D3D12_DESCRIPTOR_RANGE_FLAGS Flags;
								UINT CurrentSize;
								std::atomic<UINT> RequestedSize;
							} TextureAtlas;
						};
					};
					struct ROOT_ENTRY_CACHE {
						D3D12_DESCRIPTOR_RANGE_TYPE Type;
						UINT Size;
						UINT RegisterID;
						UINT Space;
						D3D12_DESCRIPTOR_RANGE_FLAGS Flags;
					};

					class CLOAK_UUID("{6D0D1650-247B-4EDE-8794-C660C0E7DE05}") RootSignature : public virtual IRootSignature, public Impl::Helper::SavePtr_v1::SavePtr {
						friend class Device;
						public:
							CLOAK_CALL RootSignature();
							CLOAK_CALL ~RootSignature();
							void CLOAK_CALL_THIS Finalize(In const API::Rendering::RootDescription& desc) override;
							UINT CLOAK_CALL_THIS GetTableSize(In DWORD rootIndex) const override;
							uint32_t CLOAK_CALL_THIS GetDescriptorBitMap() const override;
							API::Rendering::ROOT_PARAMETER_TYPE CLOAK_CALL_THIS GetParameterType(In size_t id) const override;
							UINT CLOAK_CALL_THIS RequestTextureAtlasSize(In DWORD rootIndex, In UINT size) override;

							//Rebuild should only be called by internal rebuilding task
							void CLOAK_CALL_THIS Rebuild();
							size_t CLOAK_CALL_THIS GetSamplerHeap(In size_t nodeID) const;
							void CLOAK_CALL_THIS SetSamplers(In UINT minMipLevel, In Impl::Rendering::IContext* context, In size_t nodeID) const;
							ID3D12RootSignature* CLOAK_CALL_THIS GetSignature() const;
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							ID3D12RootSignature* CLOAK_CALL_THIS iFinalizeV0(In IDevice* dev) const;
							ID3D12RootSignature* CLOAK_CALL_THIS iFinalizeV1(In IDevice* dev) const;

							API::Helper::ISyncSection* m_sync;
							ID3D12RootSignature* m_signature;
							uint32_t m_tableBitMap;
							API::Global::Task m_rebuild;
							size_t m_numParams;
							size_t m_numEntries;
							size_t m_numSamplers;
							size_t m_numNodes;
							size_t m_numStaticSampler;
							D3D12_ROOT_SIGNATURE_FLAGS m_flags;
							ROOT_PARAMETER_CACHE m_params[MAX_ROOT_PARAMETERS];
							ROOT_ENTRY_CACHE* m_entries;
							D3D12_STATIC_SAMPLER_DESC* m_staticSampler;
							Sampler* m_samplers;
							size_t* m_samplerHeaps;
							std::atomic<bool> m_rebuilding;
					};
					/*
					class CLOAK_UUID("{6D0D1650-247B-4EDE-8794-C660C0E7DE05}") RootSignature : public virtual IRootSignature, public Impl::Helper::SavePtr_v1::SavePtr {
						friend class Device;
						public:
							CLOAK_CALL RootSignature();
							CLOAK_CALL ~RootSignature();
							void CLOAK_CALL_THIS Finalize(In const API::Rendering::RootDescription& desc) override;
							UINT CLOAK_CALL_THIS GetTableSize(In DWORD rootIndex) const override;
							uint32_t CLOAK_CALL_THIS GetDescriptorBitMap() const override;
							API::Rendering::ROOT_PARAMETER_TYPE CLOAK_CALL_THIS GetParameterType(In size_t id) const override;
							UINT CLOAK_CALL_THIS RequestTextureAtlasSize(In DWORD rootIndex, In UINT size) override;

							void CLOAK_CALL_THIS Rebuild();
							size_t CLOAK_CALL_THIS GetSamplerHeap(In size_t nodeID) const;
							void CLOAK_CALL_THIS SetSamplers(In UINT minMipLevel, In Impl::Rendering::IContext* context, In size_t nodeID) const;
							ID3D12RootSignature* CLOAK_CALL_THIS GetSignature() const;
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							void CLOAK_CALL_THIS iFinalizeV0(In const API::Rendering::RootDescription& desc);
							void CLOAK_CALL_THIS iFinalizeV1(In const API::Rendering::RootDescription& desc);
							void CLOAK_CALL_THIS iCreateRoot(In size_t numBytes, In_reads(numBytes) const void* data, In_reads(m_numDynSamplers) const DYN_SAMPLER_INFO* sampler);
							
							bool m_finalized;
							ID3D12RootSignature* m_signature;
							UINT m_numParams;
							UINT m_numDynSamplers;
							UINT m_numNodes;
							uint32_t m_TableBitMap;
							uint32_t m_TableSize[MAX_ROOT_PARAMETERS];
							API::Rendering::ROOT_PARAMETER_TYPE m_paramTypes[MAX_ROOT_PARAMETERS];
							size_t* m_samplerHeaps;
							Sampler** m_dynSamplers;
					};
					*/
				}
			}
		}
	}
}

#pragma warning(pop)

#endif