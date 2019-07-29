#pragma once
#ifndef CC_E_CONVEXHULL_H
#define CC_E_CONVEXHULL_H

#include "CloakCompiler/Defines.h"
#include "CloakEngine/CloakEngine.h"

namespace CloakCompiler {
	namespace Engine {
		namespace ConvexHull {
			struct HullPolygon {
				size_t Vertex[3];
			};
			class Hull {
				public:
					CLOAK_CALL Hull();
					CLOAK_CALL ~Hull();
					void CLOAK_CALL_THIS Recreate(In const CloakEngine::List<CloakEngine::Global::Math::Vector>& points, In_reads(faceCount) const HullPolygon* faces, In size_t faceCount);
					void CLOAK_CALL_THIS Recreate();
					size_t CLOAK_CALL_THIS GetFaceCount() const;
					size_t CLOAK_CALL_THIS GetVertexCount() const;
					size_t CLOAK_CALL_THIS GetVertexID(In size_t polygon, In In_range(0, 2) size_t vertex) const;
					const CloakEngine::Global::Math::Vector& CLOAK_CALL_THIS GetVertex(In size_t v) const;
					const CloakEngine::Global::Math::Vector& CLOAK_CALL_THIS GetVertex(In size_t polygon, In In_range(0, 2) size_t vertex) const;
					const CloakEngine::Global::Math::Vector* CLOAK_CALL_THIS GetVertexData() const;
					const HullPolygon& CLOAK_CALL_THIS GetFace(In size_t polygon) const;
					CloakEngine::Global::Math::Plane CLOAK_CALL_THIS GetPlane(In size_t polygon) const;
					bool CLOAK_CALL_THIS IsFlat() const;
				private:
					CE::List<CE::Global::Math::Vector> m_vertices;
					CE::List<HullPolygon> m_faces;
					bool m_flat;
			};
			struct Point2D {
				union {
					struct {
						float X;
						float Y;
					};
					float Data[2];
				};
				size_t Vertex;

				CLOAK_CALL Point2D() : X(0), Y(0), Vertex(~0) {}
				CLOAK_CALL Point2D(In float x, In float y) : X(x), Y(y), Vertex(~0) {}
				CLOAK_CALL Point2D(In float x, In float y, In size_t v) : X(x), Y(y), Vertex(v) {}

				Point2D& CLOAK_CALL_THIS operator-=(In const Point2D& o) 
				{
					X -= o.X;
					Y -= o.Y;
					return *this;
				}
				Point2D& CLOAK_CALL_THIS operator+=(In const Point2D& o)
				{
					X += o.X;
					Y += o.Y;
					return *this;
				}
				Point2D& CLOAK_CALL_THIS operator*=(In float x)
				{
					X *= x;
					Y *= x;
					return *this;
				}
				Point2D& CLOAK_CALL_THIS operator/=(In float x) { return operator*=(1 / x); }
				Point2D CLOAK_CALL_THIS operator+(In const Point2D& o) const { auto r = Point2D(*this) += o; r.Vertex = ~0; return r; }
				Point2D CLOAK_CALL_THIS operator-(In const Point2D& o) const { auto r = Point2D(*this) -= o; r.Vertex = ~0; return r; }
				Point2D CLOAK_CALL_THIS operator*(In float x) const { auto r = Point2D(*this) *= x; r.Vertex = ~0; return r; }
				Point2D CLOAK_CALL_THIS operator/(In float x) const { auto r = Point2D(*this) /= x; r.Vertex = ~0; return r; }
				friend Point2D CLOAK_CALL operator*(In float x, In const Point2D& o) { return o * x; }

				float CLOAK_CALL_THIS Length() const { return CE::Global::Math::sqrt((X*X) + (Y*Y)); }
				float CLOAK_CALL_THIS Dot(In const Point2D& o) const { return (X*o.X) + (Y*o.Y); }
				float CLOAK_CALL_THIS PerpendicularDot(In const Point2D& o) const { return (X*o.Y) - (Y*o.X); }
				Point2D CLOAK_CALL_THIS Normalize() const { auto r = Point2D(*this) /= Length(); r.Vertex = ~0; return r; }
			};

			bool CLOAK_CALL IsClockwiseOriented(In const Point2D& a, In const Point2D& b, In const Point2D& c);
			bool CLOAK_CALL IsCounterClockwiseOriented(In const Point2D& a, In const Point2D& b, In const Point2D& c);

			size_t CLOAK_CALL CalculateConvexHull2DInPlace(In Point2D* points, In size_t size);
			void CLOAK_CALL CalculateConvexHull2D(In const CloakEngine::Global::Math::Vector* points, In size_t size, Out Hull* hull);
			void CLOAK_CALL CalculateSingleConvexHull(In const CloakEngine::Global::Math::Vector* points, In size_t size, Out Hull* hull);
		}
	}
}

#endif