#include "stdafx.h"
#include "Implementation\Rendering\PipelineState.h"
#include "Implementation\Files\Shader.h"

#include "CloakEngine\Helper\SyncSection.h"
#include "CloakEngine\Global\Game.h"

#include "Engine\Graphic\Core.h"
#include "Engine\Graphic\PSO.h"

#include "Includes\DefaultShader.h"

#include <vector>

#define COPY_SHADER_VERSION DX_5_1
#define COPY_SHADER_HELPER(name,shad) copyBytes(&shaderData.shad, Shader::DefaultShader::COPY_SHADER_VERSION::name::shad::data, Shader::DefaultShader::COPY_SHADER_VERSION::name::shad::size)
#define COPY_SHADER(name) COPY_SHADER_HELPER(name,VS);COPY_SHADER_HELPER(name,HS);COPY_SHADER_HELPER(name,DS);COPY_SHADER_HELPER(name,GS);COPY_SHADER_HELPER(name,PS);COPY_SHADER_HELPER(name,CS)
#define COPY_SHADER_AA(name) if(useMSAA){if(MSAACount==2){COPY_SHADER(name##MSAAx2);}else if(MSAACount==4){COPY_SHADER(name##MSAAx4);}else if(MSAACount==8){COPY_SHADER(name##MSAAx8);}}else if (set.antiAliasing == API::Global::Graphic::AntiAliasing::FXAA){COPY_SHADER(name##FXAA);}else{COPY_SHADER(name##NoAA);}

namespace CloakEngine {
	namespace Impl {
		namespace Rendering {
			API::Helper::ISyncSection* g_sync = nullptr;
			std::vector<ID3D12PipelineState*> g_pso;

			void CLOAK_CALL copyBytes(_Inout_ Impl::Files::ShaderData* dst, _In_reads_(srcSize) const BYTE* src, _In_ const size_t srcSize)
			{
				if (dst != nullptr)
				{
					dst->size = srcSize;
					if (srcSize > 0)
					{
						dst->data = new BYTE[srcSize];
						for (size_t a = 0; a < srcSize; a++) { dst->data[a] = src[a]; }
					}
					else
					{
						dst->data = nullptr;
					}
				}
			}

			_Ret_notnull_ PipelineState* CLOAK_CALL PipelineState::Create(_In_ ShaderTyp typ)
			{
				API::Helper::Lock lock(g_sync);
				g_pso.push_back(nullptr);
				return new PipelineState(typ, g_pso.size() - 1);
			}
			void CLOAK_CALL PipelineState::Initialize()
			{
				API::Global::Game::CreateInterface(IID_PPV_ARGS(&g_sync));
			}
			void CLOAK_CALL PipelineState::ShutDown()
			{
				SAVE_RELEASE(g_sync);
				for (size_t a = 0; a < g_pso.size(); a++)
				{
					ID3D12PipelineState* b = g_pso[a];
					if (b != nullptr) { b->Release(); }
				}
				g_pso.clear();
			}

			CLOAK_CALL PipelineState::PipelineState(_In_ ShaderTyp typ, _In_ size_t pos) : m_typ(typ), m_pos(pos)
			{
				m_pso = g_pso[pos];
			}
			CLOAK_CALL PipelineState::~PipelineState()
			{

			}

#define CE_COPY_SHADER_SING(Name) case ShaderTyp::Name: COPY_SHADER(Name); break
#define CE_COPY_SHADER_ALIS(Name) case ShaderTyp::Name: COPY_SHADER_AA(Name); break

			HRESULT CLOAK_CALL_THIS PipelineState::Reset(_In_ const API::Global::Graphic::Settings& set)
			{
				HRESULT hRet = S_OK;
				bool useMSAA = false;
				unsigned int MSAACount = 0;
				if (set.antiAliasing == API::Global::Graphic::AntiAliasing::MSAAx2)
				{
					useMSAA = true;
					MSAACount = 2;
				}
				else if (set.antiAliasing == API::Global::Graphic::AntiAliasing::MSAAx4)
				{
					useMSAA = true;
					MSAACount = 4;
				}
				else if (set.antiAliasing == API::Global::Graphic::AntiAliasing::MSAAx8)
				{
					useMSAA = true;
					MSAACount = 8;
				}
				Impl::Files::SingleShader shaderData;
				if (!Engine::Graphic::Core::GetShader(m_typ, ShaderVersion::DX_5_1, set.antiAliasing, &shaderData))
				{
					switch (m_typ)
					{
						CE_PER_SHADER(CE_COPY_SHADER_SING,CE_COPY_SHADER_ALIS,CE_SEP_SIMECOLON)
						default:
							hRet = E_FAIL;
							break;
					}
				}
				ID3D12PipelineState* nPSO = nullptr;
				if (SUCCEEDED(hRet))
				{
					hRet = Engine::Graphic::PSO::CreatePSO(nullptr, m_typ, set, shaderData, useMSAA, MSAACount, &nPSO, &m_root);
					delete[] shaderData.VS.data; shaderData.VS.data = nullptr;
					delete[] shaderData.HS.data; shaderData.HS.data = nullptr;
					delete[] shaderData.DS.data; shaderData.DS.data = nullptr;
					delete[] shaderData.GS.data; shaderData.GS.data = nullptr;
					delete[] shaderData.PS.data; shaderData.PS.data = nullptr;
					delete[] shaderData.CS.data; shaderData.CS.data = nullptr;
					if (FAILED(hRet)) { RELEASE(nPSO); }
				}
				ID3D12PipelineState* oPSO = m_pso;
				m_pso = nPSO;
				API::Helper::Lock lock(g_sync);
				g_pso[m_pos] = m_pso;
				lock.unlock();
				RELEASE(oPSO);
				return hRet;
			}
			const RootSignature& CLOAK_CALL_THIS PipelineState::GetRoot() { return m_root; }
			_Ret_maybenull_ ID3D12PipelineState* CLOAK_CALL_THIS PipelineState::GetPSO() { return m_pso; }
		}
	}
}