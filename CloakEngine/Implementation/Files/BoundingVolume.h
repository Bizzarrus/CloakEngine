#pragma once
#ifndef CE_IMPL_FILES_BOUNDINGVOLUME_H
#define CE_IMPL_FILES_BOUNDINGVOLUME_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/BoundingVolume.h"
#include "CloakEngine/Helper/SyncSection.h"
#include "Implementation/Helper/SavePtr.h"

#pragma warning(push)
#pragma warning(disable: 4250)

namespace CloakEngine {
	namespace Impl {
		namespace Files {
			inline namespace BoundingVolume_v1 {
				class BoundingDisabled : public virtual API::Files::BoundingVolume_v1::IBoundingVolume, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL BoundingDisabled();
						virtual CLOAK_CALL ~BoundingDisabled();
						virtual bool CLOAK_CALL_THIS Check(In const API::Global::Math::OctantFrustum& frst, In size_t passID, In_opt bool checkNearPlane = true) override;
						virtual void CLOAK_CALL_THIS SetLocalToWorld(In const API::Global::Math::Matrix& localToWorld) override;
						virtual API::Files::BoundingDesc CLOAK_CALL_THIS GetDescription() const override;
						virtual void CLOAK_CALL_THIS Update() override;
						virtual float CLOAK_CALL_THIS GetRadius() const override;
						virtual API::Global::Math::AABB CLOAK_CALL_THIS ToAABB() const override;
						virtual API::Global::Math::AABB CLOAK_CALL_THIS ToAABB(In const API::Global::Math::Matrix& WorldToProjection) const override;
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;
				};
				class BoundingSphere : public virtual API::Files::BoundingVolume_v1::IBoundingVolume, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL BoundingSphere(In const API::Global::Math::Vector& Center, In float radius);
						virtual CLOAK_CALL ~BoundingSphere();
						virtual bool CLOAK_CALL_THIS Check(In const API::Global::Math::OctantFrustum& frst, In size_t passID, In_opt bool checkNearPlane = true) override;
						virtual void CLOAK_CALL_THIS SetLocalToWorld(In const API::Global::Math::Matrix& localToWorld) override;
						virtual API::Files::BoundingDesc CLOAK_CALL_THIS GetDescription() const override;
						virtual void CLOAK_CALL_THIS Update() override;
						virtual float CLOAK_CALL_THIS GetRadius() const override;
						virtual API::Global::Math::AABB CLOAK_CALL_THIS ToAABB() const override;
						virtual API::Global::Math::AABB CLOAK_CALL_THIS ToAABB(In const API::Global::Math::Matrix& WorldToProjection) const override;

						virtual void CLOAK_CALL_THIS Update(In const API::Global::Math::Vector& Center, In float radius);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						API::Helper::ISyncSection* m_sync;
						API::Global::Math::Vector m_center;
						float m_radius;
						float m_d_radius;

						API::Global::Math::Vector m_d_center;

						struct CULLING_HIT {
							size_t lastPlaneHit = 0;
						};

						CULLING_HIT* m_culling;
						size_t m_passCounts;
						size_t m_maxPass;
				};
				class BoundingBox : public virtual API::Files::BoundingVolume_v1::IBoundingVolume, public Impl::Helper::SavePtr_v1::SavePtr {
					public:
						CLOAK_CALL BoundingBox(In const API::Global::Math::Vector& Center, In const API::Global::Math::Vector scaledAxis[3]);
						virtual CLOAK_CALL ~BoundingBox();
						virtual bool CLOAK_CALL_THIS Check(In const API::Global::Math::OctantFrustum& frst, In size_t passID, In_opt bool checkNearPlane = true) override;
						virtual void CLOAK_CALL_THIS SetLocalToWorld(In const API::Global::Math::Matrix& localToWorld) override;
						virtual API::Files::BoundingDesc CLOAK_CALL_THIS GetDescription() const override;
						virtual void CLOAK_CALL_THIS Update() override;
						virtual float CLOAK_CALL_THIS GetRadius() const override;
						virtual API::Global::Math::AABB CLOAK_CALL_THIS ToAABB() const override;
						virtual API::Global::Math::AABB CLOAK_CALL_THIS ToAABB(In const API::Global::Math::Matrix& WorldToProjection) const override;

						virtual void CLOAK_CALL_THIS Update(In const API::Global::Math::Vector& Center, In const API::Global::Math::Vector scaledAxis[3]);
					protected:
						Success(return == true) virtual bool CLOAK_CALL_THIS iQueryInterface(In REFIID riid, Outptr void** ptr) override;

						API::Helper::ISyncSection* m_sync;
						API::Global::Math::Vector m_center;
						API::Global::Math::Vector m_axis[3];
						API::Global::Math::Vector m_points[8];
						API::Global::Math::Vector m_d_points[9];
						float m_radius;
						float m_d_radius;

						struct CULLING_HIT {
							size_t lastPlaneHit = 0;
							size_t lastPointHit[6] = { 0,0,0,0,0,0 };
						};

						CULLING_HIT* m_culling;
						size_t m_passCounts;
						size_t m_maxPass;
				};
			}
		}
	}
}

#pragma warning(pop)

#endif