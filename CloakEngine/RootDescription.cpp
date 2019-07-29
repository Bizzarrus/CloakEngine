#include "stdafx.h"
#include "CloakEngine/Rendering/RootDescription.h"
#include "CloakEngine/Global/Memory.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			namespace RootDescription_v1 {
				CLOAK_CALL RootParameter::RootParameter()
				{
					m_set = false;
				}
				CLOAK_CALL RootParameter::RootParameter(In const RootParameter& o)
				{
					m_set = o.m_set;
					memcpy(&m_data, &o.m_data, sizeof(m_data));
					if (m_set == true && m_data.Type == ROOT_PARAMETER_TYPE_TABLE)
					{
						DESCRIPTOR_RANGE_ENTRY* r = NewArray(DESCRIPTOR_RANGE_ENTRY, m_data.Table.Size);
						for (size_t a = 0; a < m_data.Table.Size; a++) { r[a] = o.m_data.Table.Range[a]; }
						m_data.Table.Range = r;
					}
				}
				CLOAK_CALL RootParameter::~RootParameter()
				{
					if (m_set == true && m_data.Type == ROOT_PARAMETER_TYPE_TABLE)
					{
						DeleteArray(m_data.Table.Range);
					}
				}

				RootParameter& CLOAK_CALL_THIS RootParameter::operator=(In const RootParameter& o)
				{
					if (&o != this)
					{
						if (m_set == true && m_data.Type == ROOT_PARAMETER_TYPE_TABLE)
						{
							DeleteArray(m_data.Table.Range);
						}
						m_set = o.m_set;
						memcpy(&m_data, &o.m_data, sizeof(m_data));
						if (m_set == true && m_data.Type == ROOT_PARAMETER_TYPE_TABLE)
						{
							DESCRIPTOR_RANGE_ENTRY* r = NewArray(DESCRIPTOR_RANGE_ENTRY, m_data.Table.Size);
							for (size_t a = 0; a < m_data.Table.Size; a++) { r[a] = o.m_data.Table.Range[a]; }
							m_data.Table.Range = r;
						}
					}
					return *this;
				}

				void CLOAK_CALL_THIS RootParameter::Clear()
				{
					if (m_set == true)
					{
						if (m_data.Type == ROOT_PARAMETER_TYPE_TABLE)
						{
							DeleteArray(m_data.Table.Range);
						}
						m_set = false;
					}
				}

				void CLOAK_CALL_THIS RootParameter::SetAsConstants(In UINT registerID, In UINT numDWords, In SHADER_VISIBILITY visible, In_opt UINT space)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_CONSTANTS;
						m_data.Visible = visible;
						m_data.Constants.NumDWords = numDWords;
						m_data.Constants.RegisterID = registerID;
						m_data.Constants.Space = space;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsConstants(In UINT registerID, In UINT numDWords, In_opt UINT space) { SetAsConstants(registerID, numDWords, VISIBILITY_ALL, space); }

				void CLOAK_CALL_THIS RootParameter::SetAsRootCBV(In UINT registerID, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_CBV;
						m_data.Visible = visible;
						m_data.CBV.RegisterID = registerID;
						m_data.CBV.Space = space;
						m_data.CBV.Flags = flags;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsRootCBV(In UINT registerID, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootCBV(registerID, visible, 0, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsRootCBV(In UINT registerID, In UINT space, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootCBV(registerID, VISIBILITY_ALL, space, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsRootCBV(In UINT registerID, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootCBV(registerID, VISIBILITY_ALL, 0, flags); }

				void CLOAK_CALL_THIS RootParameter::SetAsRootSRV(In UINT registerID, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_SRV;
						m_data.Visible = visible;
						m_data.SRV.RegisterID = registerID;
						m_data.SRV.Space = space;
						m_data.SRV.Flags = flags;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsRootSRV(In UINT registerID, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootSRV(registerID, visible, 0, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsRootSRV(In UINT registerID, In UINT space, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootSRV(registerID, VISIBILITY_ALL, space, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsRootSRV(In UINT registerID, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootSRV(registerID, VISIBILITY_ALL, 0, flags); }

				void CLOAK_CALL_THIS RootParameter::SetAsRootUAV(In UINT registerID, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_UAV;
						m_data.Visible = visible;
						m_data.UAV.RegisterID = registerID;
						m_data.UAV.Space = space;
						m_data.UAV.Flags = flags;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsRootUAV(In UINT registerID, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootUAV(registerID, visible, 0, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsRootUAV(In UINT registerID, In UINT space, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootUAV(registerID, VISIBILITY_ALL, space, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsRootUAV(In UINT registerID, In_opt ROOT_PARAMETER_FLAG flags) { SetAsRootUAV(registerID, VISIBILITY_ALL, 0, flags); }

				void CLOAK_CALL_THIS RootParameter::SetAsCBV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						DESCRIPTOR_RANGE_ENTRY* r = NewArray(DESCRIPTOR_RANGE_ENTRY, 1);
						r->Count = count;
						r->RegisterID = registerID;
						r->Space = space;
						r->Type = RANGE_TYPE_CBV;
						r->Flags = flags;

						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_TABLE;
						m_data.Visible = visible;
						m_data.Table.Size = 1;
						m_data.Table.Range = r;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsCBV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags) { SetAsCBV(registerID, count, visible, 0, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsCBV(In UINT registerID, In UINT count, In UINT space, In_opt ROOT_PARAMETER_FLAG flags) { SetAsCBV(registerID, count, VISIBILITY_ALL, space, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsCBV(In UINT registerID, In UINT count, In_opt ROOT_PARAMETER_FLAG flags) { SetAsCBV(registerID, count, VISIBILITY_ALL, 0, flags); }

				void CLOAK_CALL_THIS RootParameter::SetAsSRV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						DESCRIPTOR_RANGE_ENTRY* r = NewArray(DESCRIPTOR_RANGE_ENTRY, 1);
						r->Count = count;
						r->RegisterID = registerID;
						r->Space = space;
						r->Type = RANGE_TYPE_SRV;
						r->Flags = flags;

						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_TABLE;
						m_data.Visible = visible;
						m_data.Table.Size = 1;
						m_data.Table.Range = r;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsSRV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags) { SetAsSRV(registerID, count, visible, 0, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsSRV(In UINT registerID, In UINT count, In UINT space, In_opt ROOT_PARAMETER_FLAG flags) { SetAsSRV(registerID, count, VISIBILITY_ALL, space, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsSRV(In UINT registerID, In UINT count, In_opt ROOT_PARAMETER_FLAG flags) { SetAsSRV(registerID, count, VISIBILITY_ALL, 0, flags); }

				void CLOAK_CALL_THIS RootParameter::SetAsUAV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						DESCRIPTOR_RANGE_ENTRY* r = NewArray(DESCRIPTOR_RANGE_ENTRY, 1);
						r->Count = count;
						r->RegisterID = registerID;
						r->Space = space;
						r->Type = RANGE_TYPE_UAV;
						r->Flags = flags;

						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_TABLE;
						m_data.Visible = visible;
						m_data.Table.Size = 1;
						m_data.Table.Range = r;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsUAV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags) { SetAsUAV(registerID, count, visible, 0, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsUAV(In UINT registerID, In UINT count, In UINT space, In_opt ROOT_PARAMETER_FLAG flags) { SetAsUAV(registerID, count, VISIBILITY_ALL, space, flags); }
				void CLOAK_CALL_THIS RootParameter::SetAsUAV(In UINT registerID, In UINT count, In_opt ROOT_PARAMETER_FLAG flags) { SetAsUAV(registerID, count, VISIBILITY_ALL, 0, flags); }

				void CLOAK_CALL_THIS RootParameter::SetAsSampler(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In_opt UINT space)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						DESCRIPTOR_RANGE_ENTRY* r = NewArray(DESCRIPTOR_RANGE_ENTRY, 1);
						r->Count = count;
						r->RegisterID = registerID;
						r->Space = space;
						r->Type = RANGE_TYPE_SAMPLER;

						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_TABLE;
						m_data.Visible = visible;
						m_data.Table.Size = 1;
						m_data.Table.Range = r;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsSampler(In UINT registerID, In UINT count, In_opt UINT space)
				{
					SetAsSampler(registerID, count, VISIBILITY_ALL, space);
				}
				void CLOAK_CALL_THIS RootParameter::SetAsAutomaticSampler(In UINT registerID, In const SAMPLER_DESC& desc, In SHADER_VISIBILITY visible, In_opt UINT space)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_SAMPLER;
						m_data.Visible = visible;
						m_data.Sampler.Desc = desc;
						m_data.Sampler.RegisterID = registerID;
						m_data.Sampler.Space = space;
					}
				}
				void CLOAK_CALL_THIS RootParameter::SetAsAutomaticSampler(In UINT registerID, In const SAMPLER_DESC& desc, In_opt UINT space)
				{
					SetAsAutomaticSampler(registerID, desc, VISIBILITY_ALL, space);
				}
				void CLOAK_CALL_THIS RootParameter::SetAsTable(In UINT tableSize, In_reads(tableSize) const DESCRIPTOR_RANGE_ENTRY* table, In_opt SHADER_VISIBILITY visible)
				{
					if (CloakCheckOK(m_set == false, API::Global::Debug::Error::ILLEGAL_FUNC_CALL, true))
					{
						DESCRIPTOR_RANGE_ENTRY* r = NewArray(DESCRIPTOR_RANGE_ENTRY, tableSize);
						for (UINT a = 0; a < tableSize; a++) { r[a] = table[a]; }

						m_set = true;
						m_data.Type = ROOT_PARAMETER_TYPE_TABLE;
						m_data.Visible = visible;
						m_data.Table.Size = tableSize;
						m_data.Table.Range = r;
					}
				}

				const ROOT_PARAMETER& CLOAK_CALL_THIS RootParameter::GetData() const
				{
					CloakCheckOK(m_set == true, API::Global::Debug::Error::GRAPHIC_PARAMETER_NOT_SET, true);
					return m_data;
				}

				CLOAK_CALL RootDescription::RootDescription() : RootDescription(0, 0) {}
				CLOAK_CALL RootDescription::RootDescription(In size_t numParameters, In size_t numStaticSamplers)
				{
					if (CloakDebugCheckError(numParameters <= MAX_ROOT_PARAMETER_COUNT, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true, "Only up to 16 root parameters are allowed!")) { numParameters = MAX_ROOT_PARAMETER_COUNT; }
					m_flags = ROOT_SIGNATURE_FLAG_NONE;
					m_paramCount = numParameters;
					m_samplerCount = numStaticSamplers;
					m_samplers = NewArray(STATIC_SAMPLER_DESC, numStaticSamplers);
				}
				CLOAK_CALL RootDescription::RootDescription(In const RootDescription& o)
				{
					m_paramCount = o.m_paramCount;
					m_flags = o.m_flags;
					m_samplerCount = o.m_samplerCount;
					m_samplers = NewArray(STATIC_SAMPLER_DESC, m_samplerCount);
					for (size_t a = 0; a < m_paramCount; a++) { m_params[a] = o.m_params[a]; }
					for (size_t a = 0; a < m_samplerCount; a++) { m_samplers[a] = o.m_samplers[a]; }
				}
				CLOAK_CALL RootDescription::~RootDescription()
				{
					DeleteArray(m_samplers);
				}

				RootDescription& CLOAK_CALL_THIS RootDescription::operator=(In const RootDescription& o)
				{
					if (&o != this)
					{
						Resize(o.m_paramCount, o.m_samplerCount);
						for (size_t a = 0; a < m_paramCount; a++) { m_params[a] = o.m_params[a]; }
						for (size_t a = 0; a < m_samplerCount; a++) { m_samplers[a] = o.m_samplers[a]; }
						m_flags = o.m_flags;
					}
					return *this;
				}
				RootParameter& CLOAK_CALL_THIS RootDescription::operator[](In size_t pos)
				{
					CloakDebugCheckOK(pos < m_paramCount, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true);
					return m_params[pos];
				}
				const RootParameter& CLOAK_CALL_THIS RootDescription::operator[](In size_t pos) const
				{
					CloakDebugCheckOK(pos < m_paramCount, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true);
					return m_params[pos];
				}

				RootParameter& CLOAK_CALL_THIS RootDescription::At(In size_t pos)
				{
					CloakDebugCheckOK(pos < m_paramCount, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true);
					return m_params[pos];
				}
				const RootParameter& CLOAK_CALL_THIS RootDescription::At(In size_t pos) const
				{
					CloakDebugCheckOK(pos < m_paramCount, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true);
					return m_params[pos];
				}
				void CLOAK_CALL_THIS RootDescription::SetStaticSampler(In size_t pos, In UINT registerID, In const SAMPLER_DESC& sampler, In SHADER_VISIBILITY visible, In_opt size_t space)
				{
					CloakDebugCheckOK(pos < m_samplerCount, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true);
					const bool bcbw = sampler.BorderColor[0] == sampler.BorderColor[1] && sampler.BorderColor[0] == sampler.BorderColor[2];
					const bool bcts = sampler.BorderColor[0] == sampler.BorderColor[3];
					const bool bct = bcts && bcbw && sampler.BorderColor[0] == 0.0f;
					const bool bcw = bcts && bcbw && sampler.BorderColor[0] == 1.0f;
					const bool bcb = bcbw && sampler.BorderColor[0] == 0.0f && sampler.BorderColor[3] == 1.0f;

					if (CloakDebugCheckOK(bct || bcw || bcb, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true, "Border color of static samplers must be opauqe black, opaque white or transparent black"))
					{
						m_samplers[pos].AddressU = sampler.AddressU;
						m_samplers[pos].AddressV = sampler.AddressV;
						m_samplers[pos].AddressW = sampler.AddressW;
						m_samplers[pos].CompareFunction = sampler.CompareFunction;
						m_samplers[pos].Filter = sampler.Filter;
						m_samplers[pos].MaxAnisotropy = sampler.MaxAnisotropy;
						m_samplers[pos].MaxLOD = sampler.MaxLOD;
						m_samplers[pos].MinLOD = sampler.MinLOD;
						m_samplers[pos].MipLODBias = sampler.MipLODBias;
						m_samplers[pos].RegisterID = registerID;
						m_samplers[pos].Space = static_cast<UINT>(space);
						m_samplers[pos].Visible = visible;
						m_samplers[pos].BorderColor = bct ? STATIC_BORDER_COLOR_TRANSPARENT_BLACK : (bcw ? STATIC_BORDER_COLOR_OPAQUE_WHITE : STATIC_BORDER_COLOR_OPAQUE_BLACK);
					}
				}
				void CLOAK_CALL_THIS RootDescription::SetStaticSampler(In size_t pos, In UINT registerID, In const SAMPLER_DESC& sampler, In_opt size_t space)
				{
					SetStaticSampler(pos, registerID, sampler, VISIBILITY_ALL, space);
				}
				void CLOAK_CALL_THIS RootDescription::SetStaticSampler(In size_t pos, In const STATIC_SAMPLER_DESC& sampler)
				{
					CloakDebugCheckOK(pos < m_samplerCount, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true);
					m_samplers[pos] = sampler;
				}
				const STATIC_SAMPLER_DESC& CLOAK_CALL_THIS RootDescription::GetStaticSampler(In size_t pos) const
				{
					CloakDebugCheckOK(pos < m_samplerCount, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true);
					return m_samplers[pos];
				}
				void CLOAK_CALL_THIS RootDescription::SetFlags(In ROOT_SIGNATURE_FLAG flags) { m_flags = flags; }
				ROOT_SIGNATURE_FLAG CLOAK_CALL_THIS RootDescription::GetFlags() const { return m_flags; }

				size_t CLOAK_CALL_THIS RootDescription::ParameterCount() const
				{
					return m_paramCount;
				}
				size_t CLOAK_CALL_THIS RootDescription::StaticSamplerCount() const
				{
					return m_samplerCount;
				}
				void CLOAK_CALL_THIS RootDescription::Resize(In size_t numParameters, In size_t numStaticSamplers)
				{
					if (CloakDebugCheckError(numParameters <= MAX_ROOT_PARAMETER_COUNT, API::Global::Debug::Error::ILLEGAL_ARGUMENT, true, "Only up to 16 root parameters are allowed!")) { numParameters = MAX_ROOT_PARAMETER_COUNT; }
					if (m_paramCount != numParameters)
					{
						m_paramCount = numParameters;
					}
					if (m_samplerCount != numStaticSamplers)
					{
						DeleteArray(m_samplers);
						m_samplerCount = numStaticSamplers;
						m_samplers = NewArray(STATIC_SAMPLER_DESC, m_samplerCount);
					}
				}
				void CLOAK_CALL_THIS RootDescription::ClearParameters()
				{
					for (size_t a = 0; a < m_paramCount; a++) { m_params[a].Clear(); }
				}
			}
		}
	}
}