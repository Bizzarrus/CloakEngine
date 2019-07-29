#pragma once
#ifndef CE_IMPL_RENDERING_DX12_PIPELINESTATE_H
#define CE_IMPL_RENDERING_DX12_PIPELINESTATE_H

#include "Implementation/Rendering/PipelineState.h"
#include "Implementation/Rendering/Manager.h"
#include "Implementation/Helper/SavePtr.h"

#include <d3d12.h>

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			namespace DX12 {
				inline namespace PipelineState_v1 {
					class CLOAK_UUID("{F4626DC2-647A-47C6-9A84-DA3861B65C06}") PipelineState : public virtual IPipelineState, public Impl::Helper::SavePtr_v1::SavePtr {
						public:
							Ret_notnull static PipelineState* CLOAK_CALL Create(In const IManager* manager);

							virtual CLOAK_CALL ~PipelineState();
							bool CLOAK_CALL_THIS Reset(In const API::Rendering::PIPELINE_STATE_DESC& desc, In const API::Rendering::RootDescription& root) override;
							bool CLOAK_CALL_THIS Reset(In const API::Rendering::SHADER_SET& shader, In size_t dataSize, In_reads_bytes(dataSize) const void* data) override;
							bool CLOAK_CALL_THIS Reset(In const API::Global::Graphic::Settings& set, In API::Files::IShader* shader) override;
							IRootSignature* CLOAK_CALL_THIS GetRoot() const override;
							bool CLOAK_CALL_THIS UseTessellation() const override;
							PIPELINE_STATE_TYPE CLOAK_CALL_THIS GetType() const override;

							Ret_maybenull ID3D12PipelineState* CLOAK_CALL_THIS GetPSO() const;
						private:
							Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
							CLOAK_CALL PipelineState(In const IManager* manager);

							bool m_useTess;
							IRootSignature* m_root;
							ID3D12PipelineState* m_pso;
							PIPELINE_STATE_TYPE m_type;
					};
				}
			}
		}
	}
}

#pragma warning(pop)

#endif