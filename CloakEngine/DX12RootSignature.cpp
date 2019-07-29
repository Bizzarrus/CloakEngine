#include "stdafx.h"
#if CHECK_OS(WINDOWS,10)
#include "Implementation/Rendering/DX12/RootSignature.h"
#include "Implementation/Rendering/DX12/Casting.h"
#include "Implementation/Rendering/DX12/Defines.h"
#include "Implementation/Rendering/DX12/Lib.h"
#include "Implementation/Rendering/Manager.h"

#include "Engine/Hash.h"
#include "Engine/Graphic/Core.h"

#include "CloakEngine/Helper/SyncSection.h"
#include "CloakEngine/Global/Game.h"

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				namespace RootSignature_v1 {
					constexpr API::Rendering::ROOT_SIGNATURE_FLAG DENY_ALL_ACCESS = API::Rendering::ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | API::Rendering::ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | API::Rendering::ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | API::Rendering::ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS | API::Rendering::ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
					constexpr CLOAK_FORCEINLINE bool CLOAK_CALL CheckAccess(In API::Rendering::SHADER_VISIBILITY visible, In API::Rendering::SHADER_VISIBILITY test) { return (visible & test) == test; }

					constexpr CLOAK_FORCEINLINE void CLOAK_CALL EnableAccess(Inout API::Rendering::ROOT_SIGNATURE_FLAG* flags, In API::Rendering::SHADER_VISIBILITY visible)
					{
						if (CheckAccess(visible, API::Rendering::VISIBILITY_PIXEL)) { *flags &= ~API::Rendering::ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS; }
						if (CheckAccess(visible, API::Rendering::VISIBILITY_VERTEX)) { *flags &= ~API::Rendering::ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS; }
						if (CheckAccess(visible, API::Rendering::VISIBILITY_HULL)) { *flags &= ~API::Rendering::ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS; }
						if (CheckAccess(visible, API::Rendering::VISIBILITY_DOMAIN)) { *flags &= ~API::Rendering::ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS; }
						if (CheckAccess(visible, API::Rendering::VISIBILITY_GEOMETRY)) { *flags &= ~API::Rendering::ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS; }
					}
					constexpr CLOAK_FORCEINLINE D3D12_DESCRIPTOR_RANGE_FLAGS CLOAK_CALL ToRangeFlags(In API::Rendering::ROOT_PARAMETER_FLAG flags)
					{
						D3D12_DESCRIPTOR_RANGE_FLAGS r = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
						if ((flags & API::Rendering::ROOT_PARAMETER_FLAG_STATIC) != API::Rendering::ROOT_PARAMETER_FLAG_NONE) { r |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC; }
						if ((flags & API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION) != API::Rendering::ROOT_PARAMETER_FLAG_NONE) { r |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE; }
						if ((flags & API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE) != API::Rendering::ROOT_PARAMETER_FLAG_NONE) { r |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE; }
						if ((flags & API::Rendering::ROOT_PARAMETER_FLAG_DYNAMIC_DESCRIPTOR) != API::Rendering::ROOT_PARAMETER_FLAG_NONE) { r |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE; }
						return r;
					}
					constexpr CLOAK_FORCEINLINE D3D12_ROOT_DESCRIPTOR_FLAGS CLOAK_CALL ToSingleFlags(In API::Rendering::ROOT_PARAMETER_FLAG flags)
					{
						D3D12_ROOT_DESCRIPTOR_FLAGS r = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
						if ((flags & API::Rendering::ROOT_PARAMETER_FLAG_STATIC) != API::Rendering::ROOT_PARAMETER_FLAG_NONE) { r |= D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC; }
						if ((flags & API::Rendering::ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION) != API::Rendering::ROOT_PARAMETER_FLAG_NONE) { r |= D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE; }
						if ((flags & API::Rendering::ROOT_PARAMETER_FLAG_VOLATILE) != API::Rendering::ROOT_PARAMETER_FLAG_NONE) { r |= D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE; }
						return r;
					}

					CLOAK_CALL RootSignature::RootSignature()
					{
						m_sync = nullptr;
						m_signature = nullptr;
						m_entries = nullptr;
						m_samplers = nullptr;
						m_samplerHeaps = nullptr;
						m_staticSampler = nullptr;
						m_tableBitMap = 0;
						m_numParams = 0;
						m_numSamplers = 0;
						m_numEntries = 0;
						m_numStaticSampler = 0;
						m_rebuilding = false;

						CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					}
					CLOAK_CALL RootSignature::~RootSignature()
					{
						RELEASE(m_signature);
						SAVE_RELEASE(m_sync);
						for (size_t a = 0; a < m_numSamplers; a++) { m_samplers[a].~Sampler(); }
						for (size_t a = 0; a < m_numEntries; a++) { m_entries[a].~ROOT_ENTRY_CACHE(); }
						for (size_t a = 0; a < m_numStaticSampler; a++) { m_staticSampler[a].~D3D12_STATIC_SAMPLER_DESC(); }
						API::Global::Memory::MemoryHeap::Free(m_samplers);
						API::Global::Memory::MemoryHeap::Free(m_entries);
						API::Global::Memory::MemoryHeap::Free(m_samplerHeaps);
						API::Global::Memory::MemoryHeap::Free(m_staticSampler);
					}
					void CLOAK_CALL_THIS RootSignature::Finalize(In const API::Rendering::RootDescription& desc)
					{
						API::Helper::WriteLock lock(m_sync);
						RELEASE(m_signature);
						m_tableBitMap = 0;
						m_rebuilding = false;
						for (size_t a = 0; a < m_numSamplers; a++) { m_samplers[a].~Sampler(); }
						for (size_t a = 0; a < m_numEntries; a++) { m_entries[a].~ROOT_ENTRY_CACHE(); }
						for (size_t a = 0; a < m_numStaticSampler; a++) { m_staticSampler[a].~D3D12_STATIC_SAMPLER_DESC(); }
						API::Global::Memory::MemoryHeap::Free(m_samplers);
						API::Global::Memory::MemoryHeap::Free(m_entries);
						API::Global::Memory::MemoryHeap::Free(m_samplerHeaps);
						API::Global::Memory::MemoryHeap::Free(m_staticSampler);

						IManager* clm = Engine::Graphic::Core::GetCommandListManager();
						IDevice* dev = clm->GetDevice();
						API::Rendering::ROOT_SIGNATURE_FLAG flags = desc.GetFlags() | DENY_ALL_ACCESS;
						m_numParams = static_cast<size_t>(desc.ParameterCount());
						m_numNodes = static_cast<UINT>(clm->GetNodeCount());
						CLOAK_ASSUME(m_numParams <= MAX_ROOT_PARAMETERS);
						UINT tableStart = 0;
						UINT samplerStart = 0;
						for (size_t a = 0; a < m_numParams; a++)
						{
							const API::Rendering::ROOT_PARAMETER& p = desc[a].GetData();
							m_params[a].Type = p.Type;
							m_params[a].Visible = Casting::CastForward(p.Visible);
							EnableAccess(&flags, p.Visible);
							switch (p.Type)
							{
								case API::Rendering::ROOT_PARAMETER_TYPE_UNKNOWN:
								{
									CloakError(API::Global::Debug::Error::ILLEGAL_ARGUMENT, true, "Root parameter not set!");
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_CBV:
								{
									m_params[a].CBV.Flags = ToSingleFlags(p.CBV.Flags);
									m_params[a].CBV.RegisterID = p.CBV.RegisterID;
									m_params[a].CBV.Space = p.CBV.Space;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_SRV:
								{
									m_params[a].SRV.Flags = ToSingleFlags(p.SRV.Flags);
									m_params[a].SRV.RegisterID = p.SRV.RegisterID;
									m_params[a].SRV.Space = p.SRV.Space;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_UAV:
								{
									m_params[a].UAV.Flags = ToSingleFlags(p.UAV.Flags);
									m_params[a].UAV.RegisterID = p.UAV.RegisterID;
									m_params[a].UAV.Space = p.UAV.Space;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_CONSTANTS:
								{
									m_params[a].Constants.NumDWords = p.Constants.NumDWords;
									m_params[a].Constants.RegisterID = p.Constants.RegisterID;
									m_params[a].Constants.Space = p.Constants.Space;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER:
								{
									m_tableBitMap |= 1 << a;

									m_params[a].Sampler.RegisterID = p.Sampler.RegisterID;
									m_params[a].Sampler.Space = p.Sampler.Space;
									m_params[a].Sampler.Offset = samplerStart;
									samplerStart += static_cast<UINT>(m_numNodes);
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_TABLE:
								{
									m_tableBitMap |= 1 << a;

									m_params[a].Table.Start = tableStart;
									m_params[a].Table.EntryCount = p.Table.Size;
									m_params[a].Table.FullSize = 0;
									for (size_t b = 0; b < p.Table.Size; b++) { m_params[a].Table.FullSize += p.Table.Range[b].Count; }
									tableStart += p.Table.Size;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_TEXTURE_ATLAS:
								{
									m_tableBitMap |= 1 << a;

									m_params[a].TextureAtlas.CurrentSize = 1;
									m_params[a].TextureAtlas.RequestedSize = 1;
									m_params[a].TextureAtlas.Flags = ToRangeFlags(p.TextureAtlas.Flags | API::Rendering::ROOT_PARAMETER_FLAG_DYNAMIC_DESCRIPTOR);
									m_params[a].TextureAtlas.RegisterID = p.TextureAtlas.RegisterID;
									m_params[a].TextureAtlas.Space = p.TextureAtlas.Space;
									break;
								}
								default:
									break;
							}
						}

						m_numSamplers = samplerStart * m_numNodes;
						m_numEntries = tableStart;
						m_numStaticSampler = desc.StaticSamplerCount();
						m_samplers = reinterpret_cast<Sampler*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(Sampler) * m_numSamplers));
						m_entries = reinterpret_cast<ROOT_ENTRY_CACHE*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(ROOT_ENTRY_CACHE) * tableStart));
						m_staticSampler = reinterpret_cast<D3D12_STATIC_SAMPLER_DESC*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(D3D12_STATIC_SAMPLER_DESC) * m_numStaticSampler));

						const UINT numHandles = samplerStart * SAMPLER_MAX_MIP_CLAMP;
						API::Rendering::ResourceView* handles = NewArray(API::Rendering::ResourceView, numHandles * m_numNodes);
						IDescriptorHeap** heaps = NewArray(IDescriptorHeap*, m_numNodes);

						if (m_numSamplers > 0)
						{
							m_samplerHeaps = reinterpret_cast<size_t*>(API::Global::Memory::MemoryHeap::Allocate(sizeof(size_t) * m_numNodes));
							for (size_t a = 0; a < m_numNodes; a++)
							{
								const size_t so = a * samplerStart;
								heaps[a] = clm->GetQueueByNodeID(a)->GetDescriptorHeap(HEAP_SAMPLER);
								m_samplerHeaps[a] = heaps[a]->Allocate(&handles[numHandles * a], static_cast<UINT>(numHandles));
							}
						}
						else
						{
							m_samplerHeaps = nullptr;
						}

						samplerStart = 0;
						tableStart = 0;
						for (size_t a = 0; a < m_numParams; a++)
						{
							const API::Rendering::ROOT_PARAMETER& p = desc[a].GetData();
							switch (p.Type)
							{
								case API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER:
								{
									for (size_t b = 0; b < m_numNodes; b++)
									{
										::new(&m_samplers[m_params[a].Sampler.Offset + b])Sampler(heaps[b], &handles[(b * numHandles) + (samplerStart * SAMPLER_MAX_MIP_CLAMP)], p.Sampler.Desc, static_cast<UINT>(a), dev);
									}
									samplerStart++;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_TABLE:
								{
									for (size_t b = 0; b < p.Table.Size; b++)
									{
										const API::Rendering::DESCRIPTOR_RANGE_ENTRY& e = p.Table.Range[b];

										m_entries[tableStart].Type = Casting::CastForward(e.Type);
										m_entries[tableStart].Flags = ToRangeFlags(e.Flags);
										m_entries[tableStart].RegisterID = e.RegisterID;
										m_entries[tableStart].Size = e.Count;
										m_entries[tableStart].Space = e.Space;
										tableStart++;
									}
									break;
								}
							}
						}

						DeleteArray(heaps);
						DeleteArray(handles);

						for (size_t a = 0; a < m_numStaticSampler; a++)
						{
							const API::Rendering::STATIC_SAMPLER_DESC& s = desc.GetStaticSampler(a);
							m_staticSampler[a].BorderColor = Casting::CastForward(s.BorderColor);
							m_staticSampler[a].AddressU = Casting::CastForward(s.AddressU);
							m_staticSampler[a].AddressV = Casting::CastForward(s.AddressV);
							m_staticSampler[a].AddressW = Casting::CastForward(s.AddressW);
							m_staticSampler[a].ComparisonFunc = Casting::CastForward(s.CompareFunction);
							m_staticSampler[a].Filter = Casting::CastForward(s.Filter);
							m_staticSampler[a].MaxAnisotropy = s.MaxAnisotropy;
							m_staticSampler[a].MaxLOD = s.MaxLOD;
							m_staticSampler[a].MinLOD = s.MinLOD;
							m_staticSampler[a].MipLODBias = s.MipLODBias;
							m_staticSampler[a].RegisterSpace = s.Space;
							m_staticSampler[a].ShaderRegister = s.RegisterID;
							m_staticSampler[a].ShaderVisibility = Casting::CastForward(s.Visible);
							EnableAccess(&flags, s.Visible);
						}
						m_flags = Casting::CastForward(flags);


						const API::Rendering::Hardware hardware = clm->GetHardwareSetting();
						if (hardware.RootVersion == API::Rendering::ROOT_SIGNATURE_VERSION_1_1) { m_signature = iFinalizeV1(dev); }
						else { m_signature = iFinalizeV0(dev); }
						dev->Release();
					}
					UINT CLOAK_CALL_THIS RootSignature::GetTableSize(In DWORD rootIndex) const
					{
						API::Helper::ReadLock lock(m_sync);
						if (rootIndex >= m_numParams) { return 0; }
						switch (m_params[rootIndex].Type)
						{
							case API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER: return 1;
							case API::Rendering::ROOT_PARAMETER_TYPE_TABLE: return m_params[rootIndex].Table.FullSize;
							case API::Rendering::ROOT_PARAMETER_TYPE_TEXTURE_ATLAS: return m_params[rootIndex].TextureAtlas.CurrentSize;
							default: break;
						}
						return 0;
					}
					uint32_t CLOAK_CALL_THIS RootSignature::GetDescriptorBitMap() const
					{
						API::Helper::ReadLock lock(m_sync);
						return m_tableBitMap;
					}
					API::Rendering::ROOT_PARAMETER_TYPE CLOAK_CALL_THIS RootSignature::GetParameterType(In size_t id) const
					{
						API::Helper::ReadLock lock(m_sync);
						if (id < m_numParams) { return m_params[id].Type; }
						return API::Rendering::ROOT_PARAMETER_TYPE_UNKNOWN;
					}
					UINT CLOAK_CALL_THIS RootSignature::RequestTextureAtlasSize(In DWORD rootIndex, In UINT size)
					{
						API::Helper::ReadLock lock(m_sync);
						if (rootIndex >= m_numParams) { return 0; }
						if (CloakCheckOK(m_params[rootIndex].Type == API::Rendering::ROOT_PARAMETER_TYPE_TEXTURE_ATLAS, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
						{
							UINT ps = m_params[rootIndex].TextureAtlas.RequestedSize.load();
							const bool r = ps < size;
							while (ps < size && m_params[rootIndex].TextureAtlas.RequestedSize.compare_exchange_strong(ps, size) == false);
							if (r == true && m_rebuilding.exchange(true) == false)
							{
								API::Global::Task t = [this](In size_t threadID) {this->Rebuild(); };
								t.AddDependency(m_rebuild);
								t.Schedule();
								m_rebuild = t;
							}
							return m_params[rootIndex].TextureAtlas.CurrentSize;
						}
						return 0;
					}

					void CLOAK_CALL_THIS RootSignature::Rebuild()
					{
						if (m_rebuilding.exchange(false) == true)
						{
							API::Helper::WriteLock wlock(m_sync);
							//Check if rebuilding is required:
							bool reqReb = false;
							for (size_t a = 0; a < m_numParams; a++)
							{
								if (m_params[a].Type == API::Rendering::ROOT_PARAMETER_TYPE_TEXTURE_ATLAS) 
								{
									const UINT rqs = m_params[a].TextureAtlas.RequestedSize;
									if (m_params[a].TextureAtlas.CurrentSize != rqs) { m_params[a].TextureAtlas.CurrentSize = rqs; reqReb = true; }
								}
							}
							if (reqReb == true)
							{
								API::Helper::ReadLock lock(m_sync);
								wlock.unlock();
								IManager* clm = Engine::Graphic::Core::GetCommandListManager();
								IDevice* dev = clm->GetDevice();
								const API::Rendering::Hardware hardware = clm->GetHardwareSetting();
								ID3D12RootSignature* sig = nullptr;
								if (hardware.RootVersion == API::Rendering::ROOT_SIGNATURE_VERSION_1_1) { sig = iFinalizeV1(dev); }
								else { sig = iFinalizeV0(dev); }
								dev->Release();
								wlock.lock(m_sync);
								RELEASE(m_signature);
								m_signature = sig;
							}
						}
					}
					size_t CLOAK_CALL_THIS RootSignature::GetSamplerHeap(In size_t nodeID) const
					{
						API::Helper::ReadLock lock(m_sync);
						if (m_samplerHeaps == nullptr) { return API::Rendering::HANDLEINFO_INVALID_HEAP; }
						return m_samplerHeaps[nodeID];
					}
					void CLOAK_CALL_THIS RootSignature::SetSamplers(In UINT minMipLevel, In Impl::Rendering::IContext* context, In size_t nodeID) const
					{
						API::Helper::ReadLock lock(m_sync);
						for (size_t a = 0; a < m_numParams; a++)
						{
							if (m_params[a].Type == API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER)
							{
								Sampler* s = &m_samplers[m_params[a].Sampler.Offset + nodeID];
								context->SetDescriptorTable(s->GetRootIndex(), s->GetGPUAddress(minMipLevel));
							}
						}
					}
					ID3D12RootSignature* CLOAK_CALL_THIS RootSignature::GetSignature() const
					{
						API::Helper::ReadLock lock(m_sync);
						return m_signature;
					}

					Success(return == true) bool CLOAK_CALL_THIS RootSignature::iQueryInterface(In REFIID riid, Outptr void** ptr)
					{
						RENDERING_QUERY_IMPL(ptr, riid, RootSignature, RootSignature_v1, SavePtr);
					}
					ID3D12RootSignature* CLOAK_CALL_THIS RootSignature::iFinalizeV0(In IDevice* dev) const
					{
						ID3D12RootSignature* res = nullptr;
						size_t entrySize = 0;
						for (size_t a = 0; a < m_numParams; a++)
						{
							switch (m_params[a].Type)
							{
								case API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER: entrySize += 1; break;
								case API::Rendering::ROOT_PARAMETER_TYPE_TABLE: entrySize += m_params[a].Table.FullSize; break;
								case API::Rendering::ROOT_PARAMETER_TYPE_TEXTURE_ATLAS: entrySize += m_params[a].TextureAtlas.CurrentSize; break;
								default: break;
							}
						}
						D3D12_DESCRIPTOR_RANGE* entry = NewArray(D3D12_DESCRIPTOR_RANGE, entrySize);
						D3D12_ROOT_PARAMETER* params = NewArray(D3D12_ROOT_PARAMETER, m_numParams);

						entrySize = 0;
						for (size_t a = 0; a < m_numParams; a++)
						{
							const ROOT_PARAMETER_CACHE& p = m_params[a];
							params[a].ShaderVisibility = p.Visible;
							switch (m_params[a].Type)
							{
								case API::Rendering::ROOT_PARAMETER_TYPE_CBV:
								{
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
									params[a].Descriptor.RegisterSpace = p.CBV.Space;
									params[a].Descriptor.ShaderRegister = p.CBV.RegisterID;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_SRV:
								{
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
									params[a].Descriptor.RegisterSpace = p.SRV.Space;
									params[a].Descriptor.ShaderRegister = p.SRV.RegisterID;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_UAV:
								{
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
									params[a].Descriptor.RegisterSpace = p.UAV.Space;
									params[a].Descriptor.ShaderRegister = p.UAV.RegisterID;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_CONSTANTS:
								{
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
									params[a].Constants.Num32BitValues = p.Constants.NumDWords;
									params[a].Constants.RegisterSpace = p.Constants.Space;
									params[a].Constants.ShaderRegister = p.Constants.RegisterID;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER:
								{
									entry[entrySize + 0].BaseShaderRegister = p.Sampler.RegisterID;
									entry[entrySize + 0].RegisterSpace = p.Sampler.Space;
									entry[entrySize + 0].NumDescriptors = 1;
									entry[entrySize + 0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
									entry[entrySize + 0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
									params[a].DescriptorTable.NumDescriptorRanges = 1;
									params[a].DescriptorTable.pDescriptorRanges = &entry[entrySize];
									entrySize++;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_TABLE:
								{
									for (size_t b = 0; b < p.Table.EntryCount; b++)
									{
										const ROOT_ENTRY_CACHE& e = m_entries[p.Table.Start + b];

										entry[entrySize + b].BaseShaderRegister = e.RegisterID;
										entry[entrySize + b].RegisterSpace = e.Space;
										entry[entrySize + b].NumDescriptors = e.Size;
										entry[entrySize + b].RangeType = e.Type;
										entry[entrySize + b].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
									}
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
									params[a].DescriptorTable.NumDescriptorRanges = p.Table.EntryCount;
									params[a].DescriptorTable.pDescriptorRanges = &entry[entrySize];
									entrySize += p.Table.EntryCount;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_TEXTURE_ATLAS:
								{
									entry[entrySize + 0].BaseShaderRegister = p.TextureAtlas.RegisterID;
									entry[entrySize + 0].RegisterSpace = p.TextureAtlas.Space;
									entry[entrySize + 0].NumDescriptors = p.TextureAtlas.CurrentSize;
									entry[entrySize + 0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
									entry[entrySize + 0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
									params[a].DescriptorTable.NumDescriptorRanges = 1;
									params[a].DescriptorTable.pDescriptorRanges = &entry[entrySize];
									entrySize++;
									break;
								}
								break;
							}
						}

						D3D12_VERSIONED_ROOT_SIGNATURE_DESC rdesc;
						rdesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
						rdesc.Desc_1_0.NumParameters = static_cast<UINT>(m_numParams);
						rdesc.Desc_1_0.pParameters = params;
						rdesc.Desc_1_0.NumStaticSamplers = static_cast<UINT>(m_numStaticSampler);
						rdesc.Desc_1_0.pStaticSamplers = m_staticSampler;
						rdesc.Desc_1_0.Flags = m_flags;

						ID3DBlob* resBlob = nullptr;
						ID3DBlob* errBlob = nullptr;
						HRESULT hRet = E_FAIL;
						if (Lib::D3D12SerializeVersionedRootSignature != nullptr)
						{
							hRet = Lib::D3D12SerializeVersionedRootSignature(&rdesc, &resBlob, &errBlob);
						}
						else
						{
							hRet = Lib::D3D12SerializeRootSignature(&rdesc.Desc_1_0, D3D_ROOT_SIGNATURE_VERSION_1_0, &resBlob, &errBlob);
						}
						if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_ROOT_SERIALIZE, true))
						{
							hRet = dev->CreateRootSignature(resBlob->GetBufferPointer(), resBlob->GetBufferSize(), CE_QUERY_ARGS(&res));
							CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_NO_ROOT, true);
						}
						else if (errBlob != nullptr)
						{
							const std::string err(reinterpret_cast<char*>(errBlob->GetBufferPointer()), errBlob->GetBufferSize());
							CE::Global::Log::WriteToLog(err, CE::Global::Log::Type::Error);
						}

						if (resBlob != nullptr) { resBlob->Release(); }
						if (errBlob != nullptr) { errBlob->Release(); }
						DeleteArray(params);
						DeleteArray(entry);

						return res;
					}
					ID3D12RootSignature* CLOAK_CALL_THIS RootSignature::iFinalizeV1(In IDevice* dev) const
					{
						ID3D12RootSignature* res = nullptr;
						size_t entrySize = 0;
						for (size_t a = 0; a < m_numParams; a++)
						{
							switch (m_params[a].Type)
							{
								case API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER: entrySize += 1; break;
								case API::Rendering::ROOT_PARAMETER_TYPE_TABLE: entrySize += m_params[a].Table.FullSize; break;
								case API::Rendering::ROOT_PARAMETER_TYPE_TEXTURE_ATLAS: entrySize += m_params[a].TextureAtlas.CurrentSize; break;
								default: break;
							}
						}
						D3D12_DESCRIPTOR_RANGE1* entry = NewArray(D3D12_DESCRIPTOR_RANGE1, entrySize);
						D3D12_ROOT_PARAMETER1* params = NewArray(D3D12_ROOT_PARAMETER1, m_numParams);

						entrySize = 0;
						for (size_t a = 0; a < m_numParams; a++)
						{
							const ROOT_PARAMETER_CACHE& p = m_params[a];
							params[a].ShaderVisibility = p.Visible;
							switch (m_params[a].Type)
							{
								case API::Rendering::ROOT_PARAMETER_TYPE_CBV:
								{
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
									params[a].Descriptor.RegisterSpace = p.CBV.Space;
									params[a].Descriptor.ShaderRegister = p.CBV.RegisterID;
									params[a].Descriptor.Flags = p.CBV.Flags;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_SRV:
								{
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
									params[a].Descriptor.RegisterSpace = p.SRV.Space;
									params[a].Descriptor.ShaderRegister = p.SRV.RegisterID;
									params[a].Descriptor.Flags = p.SRV.Flags;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_UAV:
								{
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
									params[a].Descriptor.RegisterSpace = p.UAV.Space;
									params[a].Descriptor.ShaderRegister = p.UAV.RegisterID;
									params[a].Descriptor.Flags = p.UAV.Flags;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_CONSTANTS:
								{
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
									params[a].Constants.Num32BitValues = p.Constants.NumDWords;
									params[a].Constants.RegisterSpace = p.Constants.Space;
									params[a].Constants.ShaderRegister = p.Constants.RegisterID;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_SAMPLER:
								{
									entry[entrySize + 0].BaseShaderRegister = p.Sampler.RegisterID;
									entry[entrySize + 0].RegisterSpace = p.Sampler.Space;
									entry[entrySize + 0].NumDescriptors = 1;
									entry[entrySize + 0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
									entry[entrySize + 0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
									entry[entrySize + 0].Flags = ToRangeFlags(API::Rendering::ROOT_PARAMETER_FLAG_NONE);

									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
									params[a].DescriptorTable.NumDescriptorRanges = 1;
									params[a].DescriptorTable.pDescriptorRanges = &entry[entrySize];
									entrySize++;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_TABLE:
								{
									for (size_t b = 0; b < p.Table.EntryCount; b++)
									{
										const ROOT_ENTRY_CACHE& e = m_entries[p.Table.Start + b];

										entry[entrySize + b].BaseShaderRegister = e.RegisterID;
										entry[entrySize + b].RegisterSpace = e.Space;
										entry[entrySize + b].NumDescriptors = e.Size;
										entry[entrySize + b].RangeType = e.Type;
										entry[entrySize + b].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
										entry[entrySize + b].Flags = e.Flags;
									}
									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
									params[a].DescriptorTable.NumDescriptorRanges = p.Table.EntryCount;
									params[a].DescriptorTable.pDescriptorRanges = &entry[entrySize];
									entrySize += p.Table.EntryCount;
									break;
								}
								case API::Rendering::ROOT_PARAMETER_TYPE_TEXTURE_ATLAS:
								{
									entry[entrySize + 0].BaseShaderRegister = p.TextureAtlas.RegisterID;
									entry[entrySize + 0].RegisterSpace = p.TextureAtlas.Space;
									entry[entrySize + 0].NumDescriptors = p.TextureAtlas.CurrentSize;
									entry[entrySize + 0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
									entry[entrySize + 0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
									entry[entrySize + 0].Flags = p.TextureAtlas.Flags;

									params[a].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
									params[a].DescriptorTable.NumDescriptorRanges = 1;
									params[a].DescriptorTable.pDescriptorRanges = &entry[entrySize];
									entrySize++;
									break;
								}
								break;
							}
						}

						D3D12_VERSIONED_ROOT_SIGNATURE_DESC rdesc;
						rdesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
						rdesc.Desc_1_1.NumParameters = static_cast<UINT>(m_numParams);
						rdesc.Desc_1_1.pParameters = params;
						rdesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(m_numStaticSampler);
						rdesc.Desc_1_1.pStaticSamplers = m_staticSampler;
						rdesc.Desc_1_1.Flags = m_flags;

						ID3DBlob* resBlob = nullptr;
						ID3DBlob* errBlob = nullptr;
						HRESULT hRet = Lib::D3D12SerializeVersionedRootSignature(&rdesc, &resBlob, &errBlob);
						if (CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_ROOT_SERIALIZE, true))
						{
							hRet = dev->CreateRootSignature(resBlob->GetBufferPointer(), resBlob->GetBufferSize(), CE_QUERY_ARGS(&res));
							CloakCheckOK(hRet, API::Global::Debug::Error::GRAPHIC_NO_ROOT, true);
						}
						else if (errBlob != nullptr)
						{
							const std::string err(reinterpret_cast<char*>(errBlob->GetBufferPointer()), errBlob->GetBufferSize());
							CE::Global::Log::WriteToLog(err, CE::Global::Log::Type::Error);
						}

						if (resBlob != nullptr) { resBlob->Release(); }
						if (errBlob != nullptr) { errBlob->Release(); }
						DeleteArray(params);
						DeleteArray(entry);

						return res;
					}
				}
			}
		}
	}
}
#endif