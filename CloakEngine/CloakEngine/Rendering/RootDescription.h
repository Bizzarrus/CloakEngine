#pragma once
#ifndef CE_API_RENDERING_ROOTDESCRIPTION_H
#define CE_API_RENDERING_ROOTDESCRIPTION_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Rendering/Defines.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Rendering {
			inline namespace RootDescription_v1 {
				class CLOAKENGINE_API RootParameter{
					public:
						CLOAK_CALL RootParameter();
						CLOAK_CALL RootParameter(In const RootParameter& o);
						CLOAK_CALL ~RootParameter();

						RootParameter& CLOAK_CALL_THIS operator=(In const RootParameter& o);

						void CLOAK_CALL_THIS Clear();
						void CLOAK_CALL_THIS SetAsConstants(In UINT registerID, In UINT numDWords, In SHADER_VISIBILITY visible, In_opt UINT space = 0);
						void CLOAK_CALL_THIS SetAsConstants(In UINT registerID, In UINT numDWords, In_opt UINT space = 0);

						void CLOAK_CALL_THIS SetAsRootCBV(In UINT registerID, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsRootCBV(In UINT registerID, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsRootCBV(In UINT registerID, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsRootCBV(In UINT registerID, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);

						void CLOAK_CALL_THIS SetAsRootSRV(In UINT registerID, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsRootSRV(In UINT registerID, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsRootSRV(In UINT registerID, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsRootSRV(In UINT registerID, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);

						void CLOAK_CALL_THIS SetAsRootUAV(In UINT registerID, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_VOLATILE);
						void CLOAK_CALL_THIS SetAsRootUAV(In UINT registerID, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_VOLATILE);
						void CLOAK_CALL_THIS SetAsRootUAV(In UINT registerID, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_VOLATILE);
						void CLOAK_CALL_THIS SetAsRootUAV(In UINT registerID, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_VOLATILE);

						void CLOAK_CALL_THIS SetAsCBV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsCBV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsCBV(In UINT registerID, In UINT count, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsCBV(In UINT registerID, In UINT count, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);

						void CLOAK_CALL_THIS SetAsSRV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsSRV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsSRV(In UINT registerID, In UINT count, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
						void CLOAK_CALL_THIS SetAsSRV(In UINT registerID, In UINT count, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_STATIC_DURING_EXECUTION);
																		
						void CLOAK_CALL_THIS SetAsUAV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_VOLATILE);
						void CLOAK_CALL_THIS SetAsUAV(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_VOLATILE);
						void CLOAK_CALL_THIS SetAsUAV(In UINT registerID, In UINT count, In UINT space, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_VOLATILE);
						void CLOAK_CALL_THIS SetAsUAV(In UINT registerID, In UINT count, In_opt ROOT_PARAMETER_FLAG flags = ROOT_PARAMETER_FLAG_VOLATILE);

						void CLOAK_CALL_THIS SetAsSampler(In UINT registerID, In UINT count, In SHADER_VISIBILITY visible, In_opt UINT space = 0);
						void CLOAK_CALL_THIS SetAsSampler(In UINT registerID, In UINT count, In_opt UINT space = 0);
						void CLOAK_CALL_THIS SetAsAutomaticSampler(In UINT registerID, In const SAMPLER_DESC& desc, In SHADER_VISIBILITY visible, In_opt UINT space = 0);
						void CLOAK_CALL_THIS SetAsAutomaticSampler(In UINT registerID, In const SAMPLER_DESC& desc, In_opt UINT space = 0);
						void CLOAK_CALL_THIS SetAsTable(In UINT tableSize, In_reads(tableSize) const DESCRIPTOR_RANGE_ENTRY* table, In_opt SHADER_VISIBILITY visible = VISIBILITY_ALL);

						const ROOT_PARAMETER& CLOAK_CALL_THIS GetData() const;
					private:
						ROOT_PARAMETER m_data;
						bool m_set;
				};
				class CLOAKENGINE_API RootDescription {
					public:
						static constexpr size_t MAX_ROOT_PARAMETER_COUNT = 16;

						CLOAK_CALL RootDescription();
						CLOAK_CALL RootDescription(In size_t numParameters, In size_t numStaticSamplers);
						CLOAK_CALL RootDescription(In const RootDescription& o);
						CLOAK_CALL ~RootDescription();

						RootDescription& CLOAK_CALL_THIS operator=(In const RootDescription& o);
						RootParameter& CLOAK_CALL_THIS operator[](In size_t pos);
						const RootParameter& CLOAK_CALL_THIS operator[](In size_t pos) const;

						RootParameter& CLOAK_CALL_THIS At(In size_t pos);
						const RootParameter& CLOAK_CALL_THIS At(In size_t pos) const;
						void CLOAK_CALL_THIS SetStaticSampler(In size_t pos, In UINT registerID, In const SAMPLER_DESC& sampler, In SHADER_VISIBILITY visible, In_opt size_t space = 0);
						void CLOAK_CALL_THIS SetStaticSampler(In size_t pos, In UINT registerID, In const SAMPLER_DESC& sampler, In_opt size_t space = 0);
						void CLOAK_CALL_THIS SetStaticSampler(In size_t pos, In const STATIC_SAMPLER_DESC& sampler);
						const STATIC_SAMPLER_DESC& CLOAK_CALL_THIS GetStaticSampler(In size_t pos) const;
						void CLOAK_CALL_THIS SetFlags(In ROOT_SIGNATURE_FLAG flags);
						ROOT_SIGNATURE_FLAG CLOAK_CALL_THIS GetFlags() const;

						size_t CLOAK_CALL_THIS ParameterCount() const;
						size_t CLOAK_CALL_THIS StaticSamplerCount() const;
						void CLOAK_CALL_THIS Resize(In size_t numParameters, In size_t numStaticSamplers);
						void CLOAK_CALL_THIS ClearParameters();
					private:
						RootParameter m_params[MAX_ROOT_PARAMETER_COUNT];
						STATIC_SAMPLER_DESC* m_samplers;
						size_t m_paramCount;
						size_t m_samplerCount;
						ROOT_SIGNATURE_FLAG m_flags;
				};
			}
		}
	}
}

#endif