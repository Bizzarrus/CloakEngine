#include "stdafx.h"
#include "Engine/ConvexHull.h"

#include <set>
#include <vector>
#include <unordered_map>
#include <utility>
#include <assert.h>
#include <random>

namespace CloakCompiler {
	namespace Engine {
		namespace ConvexHull {
			constexpr float EPSILON_F = 1e-4f;
			constexpr float EPSILON_FS = 1e-6f;
			const CloakEngine::Global::Math::Vector HULL_DIRECTIONS[] = {
				{ -1,-1,-1, 1 },
				{  1, 0, 0, 1 },
				{  0, 1, 0, 1 },
				{  0, 0, 1, 1 },

				{  1, 1, 0, 1 },
				{  1, 0, 1, 1 },
				{  0, 1, 1, 1 },
				{  1,-1, 0, 1 },
				{  1, 0,-1, 1 },
				{  0, 1,-1, 1 },
				{  1, 1, 1, 1 },
				{ -1, 1, 1, 1 },
				{  1,-1, 1, 1 },
				{  1, 1,-1, 1 },
			};


			class Edge
			{
				private:
					std::pair<size_t, size_t> m_pair;
				public:
					CLOAK_CALL Edge(In size_t a, In size_t b) : m_pair(std::make_pair(a, b))
					{

					}
					size_t CLOAK_CALL_THIS first() const { return m_pair.first; }
					size_t CLOAK_CALL_THIS second() const { return m_pair.second; }
					Edge& CLOAK_CALL_THIS operator=(In const Edge& b)
					{
						m_pair = std::make_pair(b.m_pair.first, b.m_pair.second);
						return *this;
					}
					bool CLOAK_CALL_THIS operator==(In const Edge& b) const
					{
						return m_pair.first == b.m_pair.first && m_pair.second == b.m_pair.second;
					}
			};
			struct EdgeToFace {
				bool Enabled = true;
				size_t Face = 0;

				CLOAK_CALL EdgeToFace() {}
				CLOAK_CALL EdgeToFace(In size_t a) { Enabled = true; Face = a; }
			};
			struct EdgeHash {
				size_t CLOAK_CALL operator()(In const Edge& p) const {
					return (p.first() << (sizeof(size_t) << 2)) ^ p.second();
				}
			};

			//Set of unique points
			class PointSet {
				private:
#ifdef _DEBUG
					CE::List<CE::Global::Math::Vector> m_points;
#else
					const CE::Global::Math::Vector* m_points;
#endif
					CE::List<size_t> m_set;
				public:
#ifdef _DEBUG
					CLOAK_CALL PointSet(In const CloakEngine::Global::Math::Vector* points, In size_t size) : m_points(&points[0],&points[size])
#else
					CLOAK_CALL PointSet(In const CloakEngine::Global::Math::Vector* points, In size_t size) : m_points(points)
#endif
					{
						m_set.reserve(size);
						for (size_t a = 0; a < size; a++)
						{
							for (size_t b = 0; b < m_set.size(); b++)
							{
								if (points[a] == points[m_set[b]]) { goto point_in_set; }
							}
							m_set.push_back(a);
						point_in_set:
							continue;
						}
					}
#ifdef _DEBUG
					CLOAK_CALL PointSet(In const PointSet& p) : m_points(p.m_points), m_set(p.m_set)
#else
					CLOAK_CALL PointSet(In const PointSet& p) : m_points(p.m_points), m_set(p.m_set)
#endif
					{

					}
					CLOAK_CALL ~PointSet()
					{
						
					}
					void CLOAK_CALL_THIS swap(In size_t a, In size_t b)
					{
						std::swap(m_set[a], m_set[b]);
					}
					size_t CLOAK_CALL_THIS size() const { return m_set.size(); }
					const CloakEngine::Global::Math::Vector& CLOAK_CALL_THIS at(In size_t p) const
					{
						return m_points[m_set[p]];
					}
					const CloakEngine::Global::Math::Vector& CLOAK_CALL_THIS operator[](In size_t p) const
					{
						return at(p);
					}
			};
			class Face {
				private:
					const PointSet* const m_set;
					CloakEngine::Global::Math::Plane m_plane;
					size_t m_points[3];
					bool m_enabled;
				public:
					CLOAK_CALL Face(In const Face& f) : m_set(f.m_set)
					{
						operator=(f);
					}
					CLOAK_CALL Face(In const PointSet& set, In size_t p1, In size_t p2, In size_t p3) : m_set(&set)
					{
						m_points[0] = p1;
						m_points[1] = p2;
						m_points[2] = p3;
						m_plane = CloakEngine::Global::Math::Plane(set[p1], set[p2], set[p3]);
						m_enabled = true;
					}
					bool CLOAK_CALL_THIS UsesSameSet(In const Face& f) const { return m_set == f.m_set; }
					Face& CLOAK_CALL_THIS operator=(In const Face& f)
					{
						for (size_t a = 0; a < 3; a++) { m_points[a] = f.m_points[a]; }
						m_plane = f.m_plane;
						m_enabled = f.m_enabled;
						return *this;
					}
					const CloakEngine::Global::Math::Vector& CLOAK_CALL_THIS GetPoint(In size_t a) const
					{
						return m_set->at(m_points[a]);
					}
					size_t CLOAK_CALL_THIS GetPointIndex(In size_t a) const { return m_points[a]; }
					const CloakEngine::Global::Math::Vector& CLOAK_CALL_THIS GetNormal() const
					{
						return m_plane.GetNormal();
					}
					void CLOAK_CALL_THIS SetPoints(In size_t p1, In size_t p2, In size_t p3)
					{
						m_points[0] = p1;
						m_points[1] = p2;
						m_points[2] = p3;
						m_plane = CloakEngine::Global::Math::Plane(m_set->at(p1), m_set->at(p2), m_set->at(p3));
					}
					void CLOAK_CALL_THIS CheckWinding(In const CloakEngine::Global::Math::Vector& target)
					{
						if (m_plane.SignedDistance(target) > 0)
						{
							SetPoints(m_points[2], m_points[1], m_points[0]);
						}
					}
					float CLOAK_CALL_THIS SignedDistance(In const CloakEngine::Global::Math::Vector& point) const
					{
						return m_plane.SignedDistance(point);
					}
					bool CLOAK_CALL_THIS IsEnabled() const { return m_enabled; }
					void CLOAK_CALL_THIS Disable() { m_enabled = false; }
			};

			inline void CLOAK_CALL BuildHullFromFaces(In const CloakEngine::List<Face>& faces, Out Hull* hull)
			{
				CE::List<CE::Global::Math::Vector> points;
				CE::HashMap<size_t, size_t> pToP;
				for (size_t a = 0; a < faces.size(); a++)
				{
					CLOAK_ASSUME(a == 0 || faces[a - 1].UsesSameSet(faces[a]));
					if (faces[a].IsEnabled())
					{
						for (size_t b = 0; b < 3; b++)
						{
							if (pToP.find(faces[a].GetPointIndex(b)) == pToP.end())
							{
								points.push_back(faces[a].GetPoint(b));
								pToP[faces[a].GetPointIndex(b)] = points.size() - 1;
							}
						}
					}
				}

				HullPolygon* polygons = NewArray(HullPolygon,faces.size());
				size_t polyCount = 0;
				for (size_t a = 0; a < faces.size(); a++)
				{
					if (faces[a].IsEnabled())
					{
						for (size_t b = 0; b < 3; b++)
						{
							polygons[polyCount].Vertex[b] = pToP[faces[a].GetPointIndex(b)];
						}
						polyCount++;
					}
				}
				hull->Recreate(points, polygons, polyCount);
				DeleteArray(polygons);
			}
			inline float CLOAK_CALL PerfDot(In const Point2D& o, In const Point2D& a, In const Point2D& b)
			{
				const std::pair<double, double> od = std::make_pair(static_cast<double>(o.X), static_cast<double>(o.Y));
				const std::pair<double, double> ad = std::make_pair(static_cast<double>(a.X), static_cast<double>(a.Y));
				const std::pair<double, double> bd = std::make_pair(static_cast<double>(b.X), static_cast<double>(b.Y));
				return static_cast<float>(((ad.first - od.first) * (bd.second - od.second)) - ((ad.second - od.second) * (bd.first - od.first)));
			}
			inline float CLOAK_CALL DistSq(In const Point2D& a, In const Point2D& b)
			{
				const float dx = a.X - b.X;
				const float dy = a.Y - b.Y;
				return (dx*dx) + (dy*dy);
			}
			
			bool CLOAK_CALL IsClockwiseOriented(In const Point2D& a, In const Point2D& b, In const Point2D& c)
			{
				return PerfDot(a, b, c) <= 0.0f;
			}
			bool CLOAK_CALL IsCounterClockwiseOriented(In const Point2D& a, In const Point2D& b, In const Point2D& c)
			{
				return PerfDot(a, b, c) >= 0.0f;
			}

			void CLOAK_CALL CalculateConvexHull2D(In const CloakEngine::Global::Math::Vector* points, In size_t size, Out Hull* hull)
			{
				//Common face normal:
				CE::Global::Math::Vector cn = CE::Global::Math::Vector::Zero;
				for (size_t a = 2; a < size; a++)
				{
					CE::Global::Math::Vector n = (points[a] - points[a - 2]).Cross(points[a] - points[a - 1]);
					if (n.Length() > 0)
					{
						n = n.Normalize();
						if (cn.Dot(n) >= 0) { cn += n; }
						else { cn -= n; }
						cn = cn.Normalize();
					}
				}

				//Transform all 3D points to 2D points in that basis:
				const std::pair<CE::Global::Math::Vector, CE::Global::Math::Vector> basis = cn.GetOrthographicBasis();
				CE::List<Point2D> pts2D(size);
				for (size_t a = 0; a < size; a++) { pts2D[a] = Point2D(basis.first.Dot(points[a]), basis.second.Dot(points[a]), a); }

				const size_t h = CalculateConvexHull2DInPlace(pts2D.data(), pts2D.size());

				//Build hull:
				CE::List<HullPolygon> polygons;
				CE::List<CE::Global::Math::Vector> p(h + 1);
				for (size_t a = 0; a < h + 1; a++) { p[a] = points[pts2D[a].Vertex]; }

				size_t i[2];
				HullPolygon cp;
				cp.Vertex[0] = 0;
				cp.Vertex[1] = 1;
				i[0] = 2;
				i[1] = h;
				do {
					cp.Vertex[2] = i[0]++;
					polygons.push_back(cp);
					std::swap(cp.Vertex[1], cp.Vertex[2]);
					polygons.push_back(cp);

					if (i[0] != i[1])
					{
						cp.Vertex[2] = i[1]--;
						polygons.push_back(cp);
						std::swap(cp.Vertex[0], cp.Vertex[2]);
						polygons.push_back(cp);
					}
				} while (i[0] != i[1]);
				hull->Recreate(p, polygons.data(), polygons.size());
			}
			size_t CLOAK_CALL CalculateConvexHull2DInPlace(In Point2D* points, In size_t size)
			{
				// See https://github.com/juj/MathGeoLib/blob/master/src/Math/float2.inl

				CE::List<Point2D> hullPts(2 * size);

				//Sort and filter points:
				std::sort(&points[0], &points[size], [](In const Point2D& a, In const Point2D& b) {return a.X < b.X || (a.X == b.X && a.Y < b.Y); });

				hullPts[0] = points[0];
				hullPts[1] = points[1];
				size_t k = 2;

				//Construct upper hull part:
				for (size_t a = 2; a < size; a++)
				{
					while (k >= 2 && IsCounterClockwiseOriented(hullPts[k - 2], hullPts[k - 1], points[a]) == false) { k--; }
					hullPts[k++] = points[a];
				}

				//Construct lower hull part:
				const size_t start = k;
				for (size_t a = size - 2; a > 0; a--)
				{
					while (k > start && IsCounterClockwiseOriented(hullPts[k - 2], hullPts[k - 1], points[a]) == false) { k--; }
					hullPts[k++] = points[a];
				}
				//The first point is allways part of the convex hull, but we don't want it twice in the array. Since we still need
				//to filter with the last (=first) point, we do this outside of the above loop:
				while (k > start && IsCounterClockwiseOriented(hullPts[k - 2], hullPts[k - 1], points[0]) == false) { k--; }

				//Remove duplicate entries:
				size_t h = 0;
				points[0] = hullPts[0];
				for (size_t a = 1; a < k; a++)
				{
					if (DistSq(hullPts[a], points[h]) > EPSILON_FS)
					{
						points[++h] = hullPts[a];
					}
				}
				while (DistSq(points[h], points[0]) < EPSILON_FS && h > 0) { h--; }

				//The first 'h+1' points in pts2D are now the vertices of the convex hull in counter-clockwise order
				return h + 1;
			}
			//See https://github.com/juj/MathGeoLib/blob/master/src/Geometry/Polyhedron.cpp
			void CLOAK_CALL CalculateSingleConvexHull(In const CloakEngine::Global::Math::Vector* points, In size_t size, Out Hull* hull)
			{
				CLOAK_ASSUME(size >= 3);
				hull->Recreate();
				PointSet set(points, size);
				CLOAK_ASSUME(set.size() >= 3);
				CE::List<Face> faces;
				CE::FlatSet<size_t> extremes;
				CE::HashMap<Edge, EdgeToFace, EdgeHash> edgesToFaces;
				//Find most extremest points in all given directions:
				for (size_t a = 0; a < ARRAYSIZE(HULL_DIRECTIONS); a++)
				{
					size_t i = 0;
					float md = -std::numeric_limits<float>::infinity();
					for (size_t b = 0; b < set.size(); b++)
					{
						const float d = HULL_DIRECTIONS[a].Dot(set[b]);
						if (d >= md)
						{
							md = d;
							i = b;
						}
					}
					extremes.insert(i);
				}
				if (extremes.size() < 3) { return; } //some points are NaN or duplicates

				//Degenerate case (volume in given directions is zero):
				if (extremes.size() == 3)
				{
					size_t v[3];
					size_t vi = 0;
					for (auto i : extremes) { v[vi++] = i; }
					CLOAK_ASSUME(vi == 3);
					CE::Global::Math::Plane p(set[v[0]], set[v[1]], set[v[2]]);
					for (size_t a = 0; a < set.size(); a++)
					{
						if (CE::Global::Math::abs(p.SignedDistance(set[a])) > EPSILON_F)
						{ 
							extremes.insert(a); 
							if (extremes.size() >= 4) { break; }
						}
					}

					//Input is truely planar:
					if (extremes.size() == 3)
					{
						faces.push_back(Face(set, v[0], v[1], v[2]));
						faces.push_back(Face(set, v[2], v[1], v[0]));
						BuildHullFromFaces(faces, hull);
						return;
					}
				}
				//Swap the four extrem points to the start of the set:
				size_t vi = 0;
				for (auto i : extremes) 
				{ 
					set.swap(vi++, i); 
				}

				//As we may got more then 4 extremes, choose the one that build the largest volume as start points:
				float maxVolume = 0;
				size_t bestStart[4];
				for (size_t a = 0; a + 3 < vi;a++)
				{
					const CE::Global::Math::Vector pa = set[a];
					for (size_t b = a + 1; b + 2 < vi; b++)
					{
						const CE::Global::Math::Vector pb = set[b];
						for (size_t c = b + 1; c + 1 < vi; c++)
						{
							const CE::Global::Math::Vector pc = set[c];
							for (size_t d = c + 1; d < vi; d++)
							{
								const CE::Global::Math::Vector pd = set[d];
								const float volume = CE::Global::Math::abs((pa - pd).Dot((pb - pd).Cross(pc - pd)));
								if (volume > maxVolume)
								{
									maxVolume = volume;
									bestStart[0] = a;
									bestStart[1] = b;
									bestStart[2] = c;
									bestStart[3] = d;
								}
							}
						}
					}
				}
				if (maxVolume < 1e-6f)// Set is planar:
				{
					CalculateConvexHull2D(points, size, hull);
					return;
				}
				//Swap the four choosen start points to the start of the set:
				for (size_t a = 0; a < ARRAYSIZE(bestStart); a++) { set.swap(a, bestStart[a]); }
				//Create tetrahedon (simplex):
				faces.push_back(Face(set, 0, 1, 2));
				faces.push_back(Face(set, 3, 1, 0));
				faces.push_back(Face(set, 0, 2, 3));
				faces.push_back(Face(set, 3, 2, 1));
				for (size_t a = 0; a < faces.size(); a++) { faces[a].CheckWinding(set[3 - a]); }
				//Calculate Half-Edges' face relation:
				for (size_t a = 0; a < faces.size(); a++)
				{
					const Face& f = faces[a];
					size_t vl = f.GetPointIndex(2);
					for (size_t b = 0; b < 3; b++)
					{
						const size_t vn = f.GetPointIndex(b);
						edgesToFaces[Edge(vl, vn)] = a;
						vl = vn;
					}
				}
				//For each face, maintain a list of conflicting vertices. A vertex conflicts with a face if that vertex is on the positive side of that face,
				//so that this face cannot be part of the final convex hull
				CE::List<CE::List<size_t>> faceConflicts(faces.size());
				//For each vertex, maintain a list of conflicting faces. This is the inverse of the fanceConflicts lists
				CE::List<CE::Set<size_t>> vertexConflicts(set.size());

				//Assign all remaining vertices to the conflict lists:
				for (size_t a = 0; a < faces.size(); a++)
				{
					for (size_t b = 0; b < set.size(); b++)
					{
						if (faces[a].SignedDistance(set[b]) > EPSILON_F)
						{
							faceConflicts[a].push_back(b);
							vertexConflicts[b].insert(a);
						}
					}
				}

				CE::List<size_t> stack; //Stack of conflicting faces. We are dequeuing in random order, so can't be an actual stack data structure
				for (size_t a = 0; a < faceConflicts.size(); a++) 
				{ 
					if (faceConflicts[a].empty() == false) { stack.push_back(a); } 
				}

				CE::Set<size_t> conflicting; //Set of conflicting vertices
				CE::List<Edge> boundary; //Boundary edges of "hole" that's created by removing conflicting faces
				CE::Stack<size_t> visitFaceStack;
				CE::List<bool> hullVertices(4, true);
				CE::List<uint64_t> floodFillVisited(faces.size(), 0);
				uint64_t floodFillCounter = 1;

				std::random_device rd;
				std::default_random_engine re(rd());

				while (stack.empty() == false)
				{
					//Choose random plane to prevent worst case scenarios:
					std::uniform_int_distribution<size_t> dist(0, stack.size() - 1);
					size_t rfid = dist(re);
					const size_t f = stack[rfid];
					stack[rfid] = stack.back();
					stack.pop_back();

					auto& fcon = faceConflicts.at(f); //List of all vertices that conflicts with face 'f'
					if (fcon.empty()) { continue; }

					const Face& face = faces[f];
					//Find most extreme conflicting vertex of that face:
					float ed = -std::numeric_limits<float>::infinity();
					size_t eci = fcon.size(); // Index in fcon list
					size_t ei = set.size(); // index in point set
					for (size_t a = 0; a < fcon.size(); a++)
					{
						const size_t vt = fcon[a];
						if (vt < hullVertices.size() && hullVertices[vt] == true) { continue; } //Vertex is already on hull
						const float d = face.SignedDistance(set[vt]);
						if (d > EPSILON_F)
						{
							ed = d;
							eci = a;
							ei = vt;
						}
					}
					//No point found, so nothing is in conflict:
					if (eci == fcon.size()) { fcon.clear(); continue; }
					CLOAK_ASSUME(ei < set.size());
					CLOAK_ASSUME(eci < fcon.size());
					//Remove most extreme conflicting vertex, as that vertex will become part of the hull:
					fcon[eci] = fcon.back();
					fcon.pop_back();
					if (ed <= EPSILON_F) { continue; } //Point is on face. Shouldn't ever happen

					auto& vcon = vertexConflicts[ei]; //List of all faces that conflicts with vertex 'ei'
					floodFillVisited[f] = floodFillCounter;
					visitFaceStack.push(f);
					for (auto a : vcon) 
					{ 
						if (a != f)
						{
							visitFaceStack.push(a);
							floodFillVisited[a] = floodFillCounter;
						}
					}

					//Resolve all conflicting faces:
					while (visitFaceStack.empty() == false)
					{
						const size_t fi = visitFaceStack.top();
						visitFaceStack.pop();
						//Move list of conflicting vertices of that face into conflicting set:
						conflicting.insert(faceConflicts[fi].begin(), faceConflicts[fi].end());
						faceConflicts[fi].clear();

						Face& pf = faces[fi];
						if (pf.IsEnabled() == false) { continue; }
						size_t vl = pf.GetPointIndex(2);
						for (size_t a = 0; a < 3; a++)
						{
							const size_t vn = pf.GetPointIndex(a);
							const EdgeToFace etf = edgesToFaces[Edge(vn, vl)];
							if (etf.Enabled == false || faces[etf.Face].IsEnabled() == false || floodFillVisited[etf.Face] == floodFillCounter)
							{
								//Face has already been processed in some way
								vl = vn;
								continue;
							}
							if (vcon.find(etf.Face) != vcon.end() || faces[etf.Face].SignedDistance(set[ei]) > EPSILON_F)
							{
								if (floodFillVisited[etf.Face] != floodFillCounter)
								{
									visitFaceStack.push(etf.Face);
									floodFillVisited[etf.Face] = floodFillCounter;
								}
							}
							else
							{
								boundary.push_back(Edge(vl, vn));
							}
							edgesToFaces[Edge(vl, vn)].Enabled = false; //Mark face as deleted
							vl = vn;
						}
						pf.Disable();
					}
					CLOAK_ASSUME(boundary.size() >= 3);
					floodFillCounter++;
					
					//Clean up removed faces:
					for (auto a : conflicting)
					{
						if (a != ei)
						{
							for (auto b : vcon)
							{
								auto i = vertexConflicts[a].find(b);
								if (i != vertexConflicts[a].end()) { vertexConflicts[a].erase(i); }
							}
						}
					}
					vcon.clear();

					//Fix boundary order:
					for (size_t a = 0; a < boundary.size(); a++)
					{
						for (size_t b = a + 1; b < boundary.size(); b++)
						{
							if (boundary[a].second() == boundary[b].first()) { std::swap(boundary[a + 1], boundary[b]); break; }
						}
					}

#ifdef _DEBUG
					//Check boundary:
					{
						Edge el = boundary.back();
						for (size_t a = 0; a < boundary.size(); a++)
						{
							if (el.second() != boundary[a].first())
							{
								CLOAK_ASSUME(false);
								return;
							}
							el = boundary[a];
						}
					}
#endif
					//Add new faces to hull:
					const size_t ofc = faces.size();
					for (size_t a = 0; a < boundary.size(); a++)
					{
						const Edge& e = boundary[a];
						faces.push_back(Face(set, e.first(), e.second(), ei));
						edgesToFaces[e] = faces.size() - 1;
						edgesToFaces[Edge(e.second(), ei)] = faces.size() - 1;
						edgesToFaces[Edge(ei, e.first())] = faces.size() - 1;
					}
					boundary.clear();

					//Flag new vertex (with index 'ei') as part of the hull:
					if (hullVertices.size() <= ei)
					{
						//Add missing entries:
						hullVertices.insert(hullVertices.end(), ei + 1 - hullVertices.size(), false);
					}
					hullVertices[ei] = true;

					//Redistribute conflicting points to new faces:
					faceConflicts.insert(faceConflicts.end(), faces.size() - ofc, CE::List<size_t>());
					floodFillVisited.insert(floodFillVisited.end(), faces.size() - ofc, 0);
					for (auto a : conflicting)
					{
						for (size_t b = ofc; b < faces.size(); b++)
						{
							if (faces[b].SignedDistance(set[a]) > EPSILON_F)
							{
								faceConflicts[b].push_back(a);
								vertexConflicts[a].insert(b);
							}
						}
					}
					conflicting.clear();

					//Add new faces with conflicts to working stack:
					for (size_t b = ofc; b < faces.size(); b++)
					{
						if (faceConflicts[b].empty() == false) { stack.push_back(b); }
					}
				}

				//Finish hull:
				BuildHullFromFaces(faces, hull);

#ifdef _DEBUG
				//Check if hull is truely convex:
				for (size_t a = 0; a < size; a++)
				{
					for (size_t b = 0; b < hull->GetFaceCount(); b++)
					{
						CLOAK_ASSUME(hull->GetPlane(b).SignedDistance(points[a]) <= EPSILON_F);
					}
				}
#endif
			}

			CLOAK_CALL Hull::Hull()
			{
				m_flat = false;
			}
			CLOAK_CALL Hull::~Hull()
			{

			}
			void CLOAK_CALL_THIS Hull::Recreate(In const CloakEngine::List<CloakEngine::Global::Math::Vector>& points, In_reads(faceCount) const HullPolygon* faces, In size_t faceCount)
			{
				m_flat = false;
				m_vertices = points;
				m_faces.resize(faceCount);
				for (size_t a = 0; a < faceCount; a++) { m_faces[a] = faces[a]; }

				if (m_faces.size() > 0)
				{
					m_flat = true;
					const CloakEngine::Global::Math::Vector n = GetPlane(0).GetNormal();
					for (size_t a = 1; a < m_faces.size() && m_flat == true; a++)
					{
						const CloakEngine::Global::Math::Vector m = GetPlane(a).GetNormal();
						const float d = n.Dot(m);
						if (abs(d) < 1 - EPSILON_F) { m_flat = false; }
					}
				}
			}
			void CLOAK_CALL_THIS Hull::Recreate()
			{
				m_faces.clear();
				m_vertices.clear();
				m_flat = false;
			}
			size_t CLOAK_CALL_THIS Hull::GetFaceCount() const
			{
				return m_faces.size();
			}
			size_t CLOAK_CALL_THIS Hull::GetVertexCount() const
			{
				return m_vertices.size();
			}
			size_t CLOAK_CALL_THIS Hull::GetVertexID(In size_t polygon, In In_range(0, 2) size_t vertex) const
			{
				CLOAK_ASSUME(polygon < m_faces.size());
				CLOAK_ASSUME(vertex < 3);
				return m_faces[polygon].Vertex[vertex];
			}
			const CloakEngine::Global::Math::Vector& CLOAK_CALL_THIS Hull::GetVertex(In size_t v) const
			{
				CLOAK_ASSUME(v < m_vertices.size());
				return m_vertices[v];
			}
			const CloakEngine::Global::Math::Vector& CLOAK_CALL_THIS Hull::GetVertex(In size_t polygon, In In_range(0, 2) size_t vertex) const
			{
				return GetVertex(GetVertexID(polygon, vertex));
			}
			const CloakEngine::Global::Math::Vector* CLOAK_CALL_THIS Hull::GetVertexData() const { return m_vertices.data(); }
			const HullPolygon& CLOAK_CALL_THIS Hull::GetFace(In size_t polygon) const
			{
				CLOAK_ASSUME(polygon < m_faces.size());
				return m_faces[polygon];
			}
			CloakEngine::Global::Math::Plane CLOAK_CALL_THIS Hull::GetPlane(In size_t polygon) const
			{
				return CloakEngine::Global::Math::Plane(GetVertex(polygon, 0), GetVertex(polygon, 1), GetVertex(polygon, 2));
			}
			bool CLOAK_CALL_THIS Hull::IsFlat() const
			{
				return m_flat;
			}
		}
	}
}