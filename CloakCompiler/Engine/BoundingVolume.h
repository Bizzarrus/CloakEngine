#pragma once
#ifndef CC_E_BOUNDINGVOLUME_H
#define CC_E_BOUNDINGVOLUME_H

#include "CloakCompiler/Defines.h"
#include "CloakEngine/CloakEngine.h"

namespace CloakCompiler {
	namespace Engine {
		namespace BoundingVolume {
			struct Bounding {
				bool Enabled = true;
			};
			struct BoundingSphere : public Bounding{
				CloakEngine::Global::Math::Vector Center;
				float Radius;
			};
			struct BoundingOOBB : public Bounding {
				CloakEngine::Global::Math::Vector Center;
				CloakEngine::Global::Math::Vector Axis[3];
				float HalfSize[3];
				//Each vertex of the OOBB is calculated by Center +/- Axis[0]*HalfSize[0] +/- Axis[1]*HalfSize[1] +/- Axis[2]*HalfSize[2]
			};
			BoundingSphere CLOAK_CALL CalculateBoundingSphere(In_reads(size) const CloakEngine::Global::Math::Vector* points, In size_t size);
			BoundingOOBB CLOAK_CALL CalculateBoundingOOBB(In_reads(size) const CloakEngine::Global::Math::Vector* points, In size_t size);
		}
	}
}

#endif