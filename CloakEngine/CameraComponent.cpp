#include "stdafx.h"
#include "Implementation/Components/Camera.h"

#pragma warning(push)
#pragma warning(disable: 4436)

namespace CloakEngine {
	namespace Impl {
		namespace Components {
			namespace Camera_v1 {
				const API::Global::Math::Vector FORWARD = API::Global::Math::Vector(0, 0, 1);
				const API::Global::Math::Vector UP = API::Global::Math::Vector(0, 1, 0);
				const API::Global::Math::Vector POSITION = API::Global::Math::Vector(0, 0, 0);

				CLOAK_CALL Camera::Camera(In API::World::IEntity* parent, In type_id typeID) : ICamera(parent, typeID), Component(parent, typeID)
				{
					m_streamAdr = nullptr;
					m_fov = 0;
					m_ownFov = false;
					m_settings.BloomBoost = 0.38f;
					m_settings.BloomThreshold = 0.1f;
					m_settings.Far = 120.0f;
					m_settings.Mid = 120.0f;
					m_settings.LoadRange = 184.0f;
					m_settings.Near = 0.1f;
					m_type = API::Components::Camera_v1::ProjectionType::Perspective;
					m_node = nullptr;
					m_targetType = API::Components::Camera_v1::RenderTargetType::None;
					m_target = nullptr;
					m_targetSize.Left = 0;
					m_targetSize.Right = 1;
					m_targetSize.Top = 0;
					m_targetSize.Bottom = 1;
					m_updatedClip = true;
				}
				CLOAK_CALL Camera::Camera(In API::World::IEntity* parent, In API::World::Component* src) : Camera(parent, src->TypeID)
				{
					Camera* c = dynamic_cast<Camera*>(src);
					if (c != nullptr)
					{
						m_node = c->m_node;
						m_fov = c->m_fov;
						m_ownFov = c->m_ownFov;
						m_settings = c->m_settings;
						m_type = c->m_type;
					}
				}
				CLOAK_CALL Camera::~Camera()
				{
					SAVE_RELEASE(m_target);
				}

				void CLOAK_CALL_THIS Camera::SetViewDistance(In float maxNearDist, In_opt float nearDist, In_opt float farOffset, In_opt float loadOffset)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_settings.Near = nearDist;
					m_settings.Mid = max(nearDist, maxNearDist);
					m_settings.Far = m_settings.Mid + farOffset;
					m_settings.LoadRange = m_settings.Far + loadOffset;
					Engine::WorldStreamer::UpdateStreamer(m_streamAdr, m_settings.LoadRange, m_settings.Mid, m_settings.Far);
					m_updatedClip = true;
				}
				void CLOAK_CALL_THIS Camera::SetBloom(In float threshold, In float boost)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_settings.BloomThreshold = threshold;
					m_settings.BloomBoost = boost;
				}
				void CLOAK_CALL_THIS Camera::SetFOV(In float fov)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_ownFov = true;
					m_fov = fov;
					m_updatedClip = true;
				}
				void CLOAK_CALL_THIS Camera::ResetFOV()
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_ownFov = false;
					m_updatedClip = true;
				}
				void CLOAK_CALL_THIS Camera::SetProjection(In API::Components::Camera_v1::ProjectionType type)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					m_type = type;
					m_updatedClip = true;
				}
				void CLOAK_CALL_THIS Camera::SetRenderTarget(In API::Components::Camera_v1::RenderTargetType type, In_opt API::Rendering::IColorBuffer* target)
				{
					SetRenderTarget(type, target, 0, 1, 0, 1);
				}
				void CLOAK_CALL_THIS Camera::SetRenderTarget(In API::Components::Camera_v1::RenderTargetType type, In float left, In float right, In float top, In float bottom, In_opt API::Rendering::IColorBuffer* target)
				{
					SetRenderTarget(type, target, left, right, top, bottom);
				}
				void CLOAK_CALL_THIS Camera::SetRenderTarget(In API::Components::Camera_v1::RenderTargetType type, In API::Rendering::IColorBuffer* target, In float left, In float right, In float top, In float bottom)
				{
					if (CloakDebugCheckOK((type == API::Components::Camera_v1::RenderTargetType::Texture) == (target != nullptr), API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						API::Helper::WriteLock lock = m_parent->LockWrite();
						if (type != API::Components::Camera_v1::RenderTargetType::None)
						{
							SWAP(m_target, target);
							if (m_targetType == API::Components::Camera_v1::RenderTargetType::None)
							{
								m_streamAdr = Engine::WorldStreamer::AddStreamer(m_node, m_settings.LoadRange, m_settings.Mid, m_settings.Far);
							}
						}
						else if (m_targetType != API::Components::Camera_v1::RenderTargetType::None)
						{
							SAVE_RELEASE(m_target);
							Engine::WorldStreamer::RemoveStreamer(m_streamAdr);
							m_streamAdr = nullptr;
						}
						m_targetSize.Left = min(left, right);
						m_targetSize.Right = max(left, right);
						m_targetSize.Top = min(top, bottom);
						m_targetSize.Bottom = max(top, bottom);
						m_targetType = type;
						m_updatedClip = true;
					}
				}

				void CLOAK_CALL_THIS Camera::Update(In API::Global::Time time, In Impl::Components::Position* pos, In unsigned int W, In unsigned int H, In float fov, In bool updated, Out API::Rendering::CameraData* data)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					size_t worldID = 0;
					Impl::Files::WorldNode* node = nullptr;
					const API::Global::Math::Matrix LTN = pos->GetLocalToNode(time, &node);
					const API::Global::Math::Matrix Rotation = LTN.RemoveTranslation();
					if (node != nullptr)
					{
						API::Files::IScene* scene = node->GetParentScene();
						API::Global::Math::Matrix NTW = node->GetNodeToWorld(&worldID);
						m_pos = LTN.GetTranslation() * NTW;
						m_WTV = API::Global::Math::Matrix::LookTo(m_pos, FORWARD * Rotation, UP * Rotation);
						if (m_targetType == API::Components::RenderTargetType::Texture)
						{
							W = m_target->GetWidth();
							H = m_target->GetHeight();
						}
						W = static_cast<unsigned int>(W * (m_targetSize.Right - m_targetSize.Left));
						H = static_cast<unsigned int>(H * (m_targetSize.Bottom - m_targetSize.Top));
						if (m_updatedClip == true || updated == true)
						{
							switch (m_type)
							{
								case CloakEngine::API::Components::Camera_v1::ProjectionType::Perspective:
									m_VTP = API::Global::Math::Matrix::PerspectiveProjection(m_ownFov ? m_fov : fov, W, H, m_settings.Near, m_settings.Far);
									break;
								case CloakEngine::API::Components::Camera_v1::ProjectionType::Orthographic:
									m_VTP = API::Global::Math::Matrix::OrthographicProjection(W, H, m_settings.Near, m_settings.Far);
									break;
								default:
									CLOAK_ASSUME(false);
									break;
							}
							data->UpdatedClip = true;
							m_updatedClip = false;
						}
						else { data->UpdatedClip = false; }
						data->Bloom.Boost = m_settings.BloomBoost;
						data->Bloom.Threshold = m_settings.BloomThreshold;
						data->WorldToView = m_WTV;
						data->ViewToWorld = API::Global::Math::Matrix::Inverse(m_WTV);
						data->ViewToClip = m_VTP;
						data->ClipToView = API::Global::Math::Matrix::Inverse(m_VTP);
						data->WorldToClip = m_WTV * m_VTP;
						data->ClipToWorld = data->ClipToView * data->ViewToWorld;
						data->ViewPosition = m_pos;
						data->FarPlane = m_settings.Far;
						data->NearPlane = m_settings.Near;
						data->ViewDistance = m_settings.Mid;
						data->AmbientColor = scene->GetAmbientLight();
						data->WorldID = worldID;
						data->Skybox = nullptr;
						if (scene->GetSkybox(&data->SkyIntensity, CE_QUERY_ARGS(&data->Skybox)) == false) { data->Skybox = nullptr; }
					}
				}

				const API::Global::Math::Matrix& CLOAK_CALL_THIS Camera::GetWorldToView() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_WTV;
				}
				const API::Global::Math::Matrix& CLOAK_CALL_THIS Camera::GetViewToProjection() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_VTP;
				}
				const CamSettings& CLOAK_CALL_THIS Camera::GetSettings() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_settings;
				}
				API::Rendering::RenderTargetData CLOAK_CALL_THIS Camera::GetRenderTarget(In API::Rendering::IColorBuffer* screen) const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					API::Rendering::RenderTargetData res;
					res.UV.X = m_targetSize.Left;
					res.UV.Y = m_targetSize.Top;
					res.UV.W = m_targetSize.Right - m_targetSize.Left;
					res.UV.H = m_targetSize.Bottom - m_targetSize.Top;

					res.Target = nullptr;
					switch (m_targetType)
					{
						case CloakEngine::API::Components::Camera_v1::RenderTargetType::None:
							break;
						case CloakEngine::API::Components::Camera_v1::RenderTargetType::Screen:
							res.Target = screen;
							break;
						case CloakEngine::API::Components::Camera_v1::RenderTargetType::Texture:
							res.Target = m_target;
							break;
						default:
							CLOAK_ASSUME(false);
							break;
					}
					if (res.Target != nullptr) 
					{ 
						res.Target->AddRef(); 
						const UINT W = res.Target->GetWidth();
						const UINT H = res.Target->GetHeight();
						res.Pixel.X = static_cast<UINT>(W * res.UV.X);
						res.Pixel.Y = static_cast<UINT>(H * res.UV.Y);
						res.Pixel.W = static_cast<UINT>(W * res.UV.W);
						res.Pixel.H = static_cast<UINT>(H * res.UV.H);
						res.BaseSize.W = W;
						res.BaseSize.H = H;
					}
					return res;
				}
				const API::Global::Math::Vector& CLOAK_CALL_THIS Camera::GetViewPosition() const
				{
					API::Helper::ReadLock lock = m_parent->LockRead();
					return m_pos;
				}

				void CLOAK_CALL_THIS Camera::OnNodeChange(In API::Files::IScene* scene, In API::Files::IWorldNode* node)
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					Impl::Files::WorldNode* wn = nullptr;
					HRESULT hRet = node == nullptr ? S_OK : node->QueryInterface(CE_QUERY_ARGS(&wn));
					if (CloakDebugCheckOK(hRet, API::Global::Debug::Error::ILLEGAL_ARGUMENT, false))
					{
						m_node = wn;
						Engine::WorldStreamer::UpdateStreamer(m_streamAdr, wn);
						SAVE_RELEASE(wn);
					}
				}
				void CLOAK_CALL_THIS Camera::OnEnable()
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					if (m_targetType != API::Components::Camera_v1::RenderTargetType::None)
					{
						m_streamAdr = Engine::WorldStreamer::AddStreamer(m_node, m_settings.LoadRange, m_settings.Mid, m_settings.Far);
					}
				}
				void CLOAK_CALL_THIS Camera::OnDisable()
				{
					API::Helper::WriteLock lock = m_parent->LockWrite();
					Engine::WorldStreamer::RemoveStreamer(m_streamAdr);
					m_streamAdr = nullptr;
				}
			}
		}
	}
}

#pragma warning(pop)