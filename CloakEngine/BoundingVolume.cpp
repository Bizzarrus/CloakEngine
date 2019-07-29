#include "stdafx.h"
#include "Implementation/Files/BoundingVolume.h"

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			namespace BoundingVolume_v1 {
				CLOAK_CALL BoundingDisabled::BoundingDisabled() {}
				CLOAK_CALL BoundingDisabled::~BoundingDisabled() {}
				bool CLOAK_CALL_THIS BoundingDisabled::Check(In const API::Global::Math::OctantFrustum& frst, In size_t passID, In_opt bool checkNearPlane) { return true; }
				void CLOAK_CALL_THIS BoundingDisabled::SetLocalToWorld(In const API::Global::Math::Matrix& localToWorld) {}
				API::Files::BoundingDesc CLOAK_CALL_THIS BoundingDisabled::GetDescription() const
				{
					API::Files::BoundingDesc r;
					r.Type = API::Files::BoundingType::NONE;
					return r;
				}
				void CLOAK_CALL_THIS BoundingDisabled::Update() {}
				float CLOAK_CALL_THIS BoundingDisabled::GetRadius() const { return 0; }
				API::Global::Math::AABB CLOAK_CALL_THIS BoundingDisabled::ToAABB() const { return API::Global::Math::AABB(); }
				API::Global::Math::AABB CLOAK_CALL_THIS BoundingDisabled::ToAABB(In const API::Global::Math::Matrix& WorldToProjection) const { return API::Global::Math::AABB(); }
				Success(return == true) bool CLOAK_CALL_THIS BoundingDisabled::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::BoundingVolume_v1::IBoundingVolume)) { *ptr = (API::Files::BoundingVolume_v1::IBoundingVolume*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL BoundingSphere::BoundingSphere(In const API::Global::Math::Vector& Center, In float radius) : m_center(Center), m_radius(radius > 0 ? radius : 0)
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					m_culling = nullptr;
					m_maxPass = 0;
					m_passCounts = 0;
					m_d_center = m_center;
					m_d_radius = m_radius;
				}
				CLOAK_CALL BoundingSphere::~BoundingSphere()
				{
					SAVE_RELEASE(m_sync);
					DeleteArray(m_culling);
				}
				bool CLOAK_CALL_THIS BoundingSphere::Check(In const API::Global::Math::OctantFrustum& frst, In size_t passID, In_opt bool checkNearPlane)
				{
					API::Helper::Lock lock(m_sync);
					m_maxPass = max(passID + 1, m_maxPass);
					size_t lph = 0;
					if (passID < m_passCounts) { lph = m_culling[passID].lastPlaneHit; }
					for (size_t a = 0; a < 6; a++)
					{
						const size_t i = (lph + a) % 6;
						if ((i != 4 || checkNearPlane) && frst.Base[i].SignedDistance(m_d_center) > m_d_radius)
						{
							if (passID < m_passCounts) { m_culling[passID].lastPlaneHit = i; }
							return false;
						}
					}
					return true;
				}
				void CLOAK_CALL_THIS BoundingSphere::SetLocalToWorld(In const API::Global::Math::Matrix& localToWorld)
				{
					API::Helper::Lock lock(m_sync);
					m_d_center = m_center * localToWorld;
					API::Global::Math::Matrix ltw = localToWorld.RemoveTranslation();
					API::Global::Math::Vector v1 = API::Global::Math::Vector::Forward * ltw;
					API::Global::Math::Vector v2 = API::Global::Math::Vector::Top * ltw;
					API::Global::Math::Vector v3 = API::Global::Math::Vector::Left * ltw;
					const float l1 = v1.Length();
					const float l2 = v2.Length();
					const float l3 = v3.Length();
					const float l4 = max(l1, l2);
					m_d_radius = m_radius * max(l3, l4);
				}
				API::Files::BoundingDesc CLOAK_CALL_THIS BoundingSphere::GetDescription() const
				{
					API::Helper::Lock lock(m_sync);
					API::Files::BoundingDesc desc;
					desc.Type = API::Files::BoundingType::SPHERE;
					desc.Sphere.Center = m_center;
					desc.Sphere.Radius = m_radius;
					return desc;
				}
				void CLOAK_CALL_THIS BoundingSphere::Update()
				{
					API::Helper::Lock lock(m_sync);
					if (m_maxPass != m_passCounts)
					{
						CULLING_HIT* l = m_culling;
						m_culling = NewArray(CULLING_HIT, m_maxPass);
						for (size_t a = 0; a < m_passCounts; a++) { m_culling[a] = l[a]; }
						m_passCounts = m_maxPass;
						DeleteArray(l);
					}
				}
				float CLOAK_CALL_THIS BoundingSphere::GetRadius() const 
				{
					API::Helper::Lock lock(m_sync);
					return m_d_radius;
				}
				API::Global::Math::AABB CLOAK_CALL_THIS BoundingSphere::ToAABB() const
				{
					API::Helper::Lock lock(m_sync);
					API::Global::Math::Point c = static_cast<API::Global::Math::Point>(m_d_center);
					return API::Global::Math::AABB(c.X - m_d_radius, c.X + m_d_radius, c.Y - m_d_radius, c.Y + m_d_radius, c.Z - m_d_radius, c.Z + m_d_radius);
				}
				API::Global::Math::AABB CLOAK_CALL_THIS BoundingSphere::ToAABB(In const API::Global::Math::Matrix& WorldToProjection) const
				{
					API::Helper::Lock lock(m_sync);
					float d = m_d_center.Dot(m_d_center);
					float a = std::sqrt(d - (m_d_radius*m_d_radius));

					API::Global::Math::Point c = static_cast<API::Global::Math::Point>(m_d_center);
					API::Global::Math::Vector R = (m_d_radius / a)*API::Global::Math::Vector(-c.Z, 0, c.X);
					API::Global::Math::Vector U = API::Global::Math::Vector(0, m_d_radius, 0);
					API::Global::Math::Vector F = R.Cross(U) / m_d_radius;

					API::Global::Math::Matrix ProjNoTrans = WorldToProjection.RemoveTranslation();
					API::Global::Math::Vector pR = R * ProjNoTrans;
					API::Global::Math::Vector pU = U * ProjNoTrans;
					API::Global::Math::Vector pF = F * ProjNoTrans;
					API::Global::Math::Vector pC = m_d_center * WorldToProjection;

					API::Global::Math::Vector vN = pC + pU;
					API::Global::Math::Vector vE = pC + pR;
					API::Global::Math::Vector vF = pC + pF;
					API::Global::Math::Vector vS = pC - pU;
					API::Global::Math::Vector vW = pC - pR;
					API::Global::Math::Vector vD = pC - pF;

					API::Global::Math::Vector mi = API::Global::Math::Vector::Min(API::Global::Math::Vector::Min(API::Global::Math::Vector::Min(vN, vE), API::Global::Math::Vector::Min(vF, vS)), API::Global::Math::Vector::Min(vW, vD));
					API::Global::Math::Vector ma = API::Global::Math::Vector::Max(API::Global::Math::Vector::Max(API::Global::Math::Vector::Max(vN, vE), API::Global::Math::Vector::Max(vF, vS)), API::Global::Math::Vector::Max(vW, vD));

					API::Global::Math::Point mip = static_cast<API::Global::Math::Point>(mi);
					API::Global::Math::Point map = static_cast<API::Global::Math::Point>(ma);

					return API::Global::Math::AABB(mip.X, map.X, mip.Y, map.Y, mip.Z, map.Z);
				}
				void CLOAK_CALL_THIS BoundingSphere::Update(In const API::Global::Math::Vector& Center, In float radius)
				{
					API::Helper::Lock lock(m_sync);
					m_center = Center;
					m_radius = radius > 0 ? radius : 0;
				}

				Success(return == true) bool CLOAK_CALL_THIS BoundingSphere::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::BoundingVolume_v1::IBoundingVolume)) { *ptr = (API::Files::BoundingVolume_v1::IBoundingVolume*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}

				CLOAK_CALL BoundingBox::BoundingBox(In const API::Global::Math::Vector& Center, In const API::Global::Math::Vector scaledAxis[3])
				{
					CREATE_INTERFACE(CE_QUERY_ARGS(&m_sync));
					m_culling = nullptr;
					m_maxPass = 0;
					m_passCounts = 0;
					m_d_radius = 0;
					Update(Center, scaledAxis);
				}
				CLOAK_CALL BoundingBox::~BoundingBox()
				{
					SAVE_RELEASE(m_sync);
					DeleteArray(m_culling);
				}
				bool CLOAK_CALL_THIS BoundingBox::Check(In const API::Global::Math::OctantFrustum& frst, In size_t passID, In_opt bool checkNearPlane)
				{
					API::Helper::Lock lock(m_sync);
					m_maxPass = max(passID + 1, m_maxPass);
					size_t lPlaneH = 0;
					size_t lPointH[6] = { 0,0,0,0,0,0 };
					if (passID < m_passCounts)
					{
						lPlaneH = m_culling[passID].lastPlaneHit;
						for (size_t a = 0; a < 6; a++) { lPointH[a] = m_culling[passID].lastPointHit[a]; }
					}
					if (m_d_radius < frst.CenterDistance)
					{
						const size_t o = frst.LocateOctant(m_d_points[8]);
						for (size_t a = 0; a < 3; a++)
						{
							const size_t i = (lPlaneH + a) % 3;
							const size_t ni = API::Global::Math::OctantFrustum::OCTANT_PLANES[o][i];
							if (ni != 4 || checkNearPlane)
							{
								for (size_t b = 0; b < 8; b++)
								{
									const size_t j = (lPointH[ni] + b) % 8;
									if (frst.Base[ni].SignedDistance(m_d_points[j]) <= 0)
									{
										if (passID < m_passCounts) { m_culling[passID].lastPointHit[ni] = j; }
										goto box_octant_found_hit;
									}
								}
								if (passID < m_passCounts) { m_culling[passID].lastPlaneHit = i; }
								return false;

							box_octant_found_hit:;

							}
						}
					}
					else
					{
						for (size_t a = 0; a < 6; a++)
						{
							const size_t i = (lPlaneH + a) % 6;
							if (i != 4 || checkNearPlane)
							{
								for (size_t b = 0; b < 8; b++)
								{
									const size_t j = (lPointH[i] + b) % 8;
									if (frst.Base[i].SignedDistance(m_d_points[j]) <= 0)
									{
										if (passID < m_passCounts) { m_culling[passID].lastPointHit[i] = j; }
										goto box_found_hit;
									}
								}
								if (passID < m_passCounts) { m_culling[passID].lastPlaneHit = i; }
								return false;

							box_found_hit:;

							}
						}
					}
					return true;
				}
				void CLOAK_CALL_THIS BoundingBox::SetLocalToWorld(In const API::Global::Math::Matrix& localToWorld)
				{
					API::Helper::Lock lock(m_sync);
					for (size_t a = 0; a < 8; a++) { m_d_points[a] = m_points[a] * localToWorld; }
					m_d_points[8] = m_center * localToWorld;
					float rs = 0;
					for (size_t a = 0; a < 8; a++)
					{
						const API::Global::Math::Vector v = m_d_points[a << 1] - m_d_points[8];
						const float l = v.Length();
						rs = max(rs, l);
					}
					m_d_radius = m_radius * rs;
				}
				API::Files::BoundingDesc CLOAK_CALL_THIS BoundingBox::GetDescription() const
				{
					API::Helper::Lock lock(m_sync);
					API::Files::BoundingDesc desc;
					desc.Type = API::Files::BoundingType::BOX;
					desc.Box.Center = m_center;
					desc.Box.ScaledAxis[0] = m_axis[0];
					desc.Box.ScaledAxis[1] = m_axis[1];
					desc.Box.ScaledAxis[2] = m_axis[2];
					return desc;
				}
				void CLOAK_CALL_THIS BoundingBox::Update()
				{
					API::Helper::Lock lock(m_sync);
					if (m_maxPass != m_passCounts)
					{
						CULLING_HIT* l = m_culling;
						m_culling = NewArray(CULLING_HIT, m_maxPass);
						for (size_t a = 0; a < m_passCounts; a++) { m_culling[a] = l[a]; }
						m_passCounts = m_maxPass;
						DeleteArray(l);
					}
				}
				float CLOAK_CALL_THIS BoundingBox::GetRadius() const 
				{
					API::Helper::Lock lock(m_sync);
					return m_d_radius;
				}
				API::Global::Math::AABB CLOAK_CALL_THIS BoundingBox::ToAABB() const
				{
					API::Helper::Lock lock(m_sync);
					return API::Global::Math::AABB(8, m_d_points);
				}
				API::Global::Math::AABB CLOAK_CALL_THIS BoundingBox::ToAABB(In const API::Global::Math::Matrix& WorldToProjection) const
				{
					API::Helper::Lock lock(m_sync);
					API::Global::Math::Vector p[8];
					for (size_t a = 0; a < 8; a++) { p[a] = m_d_points[a] * WorldToProjection; }
					return API::Global::Math::AABB(8, p);
				}

				void CLOAK_CALL_THIS BoundingBox::Update(In const API::Global::Math::Vector& Center, In const API::Global::Math::Vector scaledAxis[3])
				{
					API::Helper::Lock lock(m_sync);
					m_center = Center;
					m_axis[0] = scaledAxis[0];
					m_axis[1] = scaledAxis[1];
					m_axis[2] = scaledAxis[2];
					API::Global::Math::Vector v[4];
					v[0] = (m_axis[0] + m_axis[1]) + m_axis[2];
					v[1] = (m_axis[0] + m_axis[1]) - m_axis[2];
					v[2] = (m_axis[0] - m_axis[1]) + m_axis[2];
					v[3] = (m_axis[0] - m_axis[1]) - m_axis[2];
					m_radius = 0;
					for (size_t a = 0; a < 4; a++)
					{
						m_points[(a << 1) + 0] = m_center + v[a];
						m_points[(a << 1) + 1] = m_center - v[a];
						const float l = v[a].Length();
						m_radius = max(m_radius, l);
					}
					for (size_t a = 0; a < 8; a++) { m_d_points[a] = m_points[a]; }
					m_d_points[8] = m_center;
				}

				Success(return == true) bool CLOAK_CALL_THIS BoundingBox::iQueryInterface(In REFIID riid, Outptr void** ptr)
				{
					if (riid == __uuidof(API::Files::BoundingVolume_v1::IBoundingVolume)) { *ptr = (API::Files::BoundingVolume_v1::IBoundingVolume*)this; return true; }
					return SavePtr::iQueryInterface(riid, ptr);
				}
			}
		}
	}
	namespace API {
		namespace Files {
			namespace BoundingVolume_v1 {
				CLOAKENGINE_API IBoundingVolume* CLOAK_CALL CreateBoundingVolume(In const BoundingDesc& desc)
				{
					switch (desc.Type)
					{
						case API::Files::BoundingVolume_v1::BoundingType::NONE:
							return new Impl::Files::BoundingVolume_v1::BoundingDisabled();
						case API::Files::BoundingVolume_v1::BoundingType::BOX:
							return new Impl::Files::BoundingVolume_v1::BoundingBox(desc.Box.Center, desc.Box.ScaledAxis);
						case API::Files::BoundingVolume_v1::BoundingType::SPHERE:
							return new Impl::Files::BoundingVolume_v1::BoundingSphere(desc.Sphere.Center, desc.Sphere.Radius);
						default:
							break;
					}
					return nullptr;
				}
				CLOAKENGINE_API IBoundingVolume* CLOAK_CALL UpdateBoundingVolume(In IBoundingVolume* volume, In const BoundingDesc& desc)
				{
					if (volume == nullptr) { return CreateBoundingVolume(desc); }
					BoundingDesc o = volume->GetDescription();
					if (o.Type != desc.Type) { volume->Release(); return CreateBoundingVolume(desc); }
					switch (desc.Type)
					{
						case API::Files::BoundingVolume_v1::BoundingType::NONE:
							break;
						case API::Files::BoundingVolume_v1::BoundingType::BOX:
						{
							Impl::Files::BoundingVolume_v1::BoundingBox* box = dynamic_cast<Impl::Files::BoundingVolume_v1::BoundingBox*>(volume);
							CLOAK_ASSUME(box != nullptr);
							box->Update(desc.Box.Center, desc.Box.ScaledAxis);
							break;
						}
						case API::Files::BoundingVolume_v1::BoundingType::SPHERE:
						{
							Impl::Files::BoundingVolume_v1::BoundingSphere* sphere = dynamic_cast<Impl::Files::BoundingVolume_v1::BoundingSphere*>(volume);
							CLOAK_ASSUME(sphere != nullptr);
							sphere->Update(desc.Sphere.Center, desc.Sphere.Radius);
							break;
						}
						default:
							break;
					}
					return volume;
				}
			}
		}
	}
}