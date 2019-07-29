#include "stdafx.h"
#include "Engine/BoundingVolume.h"
#include "Engine/ConvexHull.h"

#include <vector>
#include <utility>
#include <random>
#include <assert.h>

//Enable this to calculate antipodal edges using a very simple algorithm. Should only be used for debuging purposes
//#define SIMPLE_ANTIPODAL_ALGORITHM
//Enable this to calculate sidepodal edges using a very simple algorithm. Should only be used for debuging purposes
//#define SIMPLE_SIDEPODAL_ALGORITHM
//Enable this to validate that each proposed OOBB actually includes all points
//#define CHECK_BOX_VALIDITY

namespace CloakCompiler {
	namespace Engine {
		namespace BoundingVolume {
			constexpr double EPSILON_D = 1e-14;
			constexpr float EPSILON_F = 1e-4f;
			constexpr float EPSILON_ANGLE = 1e-3f;
			constexpr size_t DIMENSION = 3;
			namespace Sphere {
				//Using Fisher's "Smallest Enclosing Ball" algorithm to calculate smallest spherical bounding volume
				class Subspan {
					private:
						struct Point {
							CloakEngine::Global::Math::Vector pos;
							bool member;
						};

						float m_Q[DIMENSION][DIMENSION];
						float m_R[DIMENSION][DIMENSION];
						CloakEngine::Global::Math::Vector m_u;
						CloakEngine::Global::Math::Vector m_v;
						size_t m_rank;
						size_t m_members[DIMENSION + 1];
						uint8_t* const m_SHeap;
						Point* m_S;
						const size_t m_cap;
						size_t m_size;

						inline void CLOAK_CALL_THIS givens(In float a, In float b, Out float* c, Out float* s)
						{
							if (b == 0) { *c = 1; *s = 0; }
							else if (abs(b) > abs(a))
							{
								const float t = a / b;
								const float ns = 1 / sqrtf(1 + (t*t));
								*s = ns;
								*c = ns*t;
							}
							else
							{
								const float t = b / a;
								const float nc = 1 / sqrtf(1 + (t*t));
								*c = nc;
								*s = nc*t;
							}
						}
						inline void CLOAK_CALL_THIS Clear(In size_t i)
						{
							for (size_t a = i; a < m_rank; a++)
							{
								float c, s;
								givens(m_R[a][a], m_R[a][a + 1], &c, &s);
								m_R[a][a] = (c*m_R[a][a]) + (s*m_R[a][a + 1]);
								for (size_t b = a + 1; b < m_rank; b++)
								{
									const float r[] = { m_R[b][a],m_R[b][a + 1] };
									m_R[b][a] = (c*r[0]) + (s*r[1]);
									m_R[b][a + 1] = (c*r[1]) - (s*r[0]);
								}
								for (size_t b = 0; b < DIMENSION; b++)
								{
									const float q[] = { m_Q[a][b],m_Q[a + 1][b] };
									m_Q[a][b] = (c*q[0]) + (s*q[1]);
									m_Q[a + 1][b] = (c*q[1]) - (s*q[0]);
								}
							}
						}
					public:
						CLOAK_CALL Subspan(In const CloakEngine::Global::Math::Vector* points, In size_t size, In size_t index) : m_cap(size), m_SHeap(new uint8_t[(sizeof(Point)*size) + 15])
						{
							m_S = reinterpret_cast<Point*>((reinterpret_cast<uintptr_t>(m_SHeap) + 15) & ~static_cast<uintptr_t>(0xF));
							for (size_t a = 0; a < size; a++)
							{
								m_S[a].pos = points[a];
								m_S[a].member = a == index;
							}
							for (size_t a = 0; a < DIMENSION; a++)
							{
								for (size_t b = 0; b < DIMENSION; b++)
								{
									m_Q[a][b] = a == b ? 1.0f : 0.0f;
									m_R[a][b] = 0.0f;
								}
							}
							m_rank = 0;
							m_members[m_rank] = index;
						}
						CLOAK_CALL ~Subspan()
						{
							delete[] m_SHeap;
						}
						size_t CLOAK_CALL_THIS GetAny() const { return m_members[m_rank]; }
						bool CLOAK_CALL_THIS IsMember(In size_t s) const { return m_S[s].member; }
						void CLOAK_CALL_THIS AddPoint(In size_t i)
						{
							m_u = m_S[i].pos - m_S[m_members[m_rank]].pos;
							for (size_t a = 0; a < DIMENSION; a++)
							{
								const CloakEngine::Global::Math::Vector q(m_Q[a][0], m_Q[a][1], m_Q[a][2]);
								m_R[m_rank][a] = q.Dot(m_u);
							}
							for (size_t a = DIMENSION - 1; a > m_rank; a--)
							{
								float c, s;
								givens(m_R[m_rank][a - 1], m_R[m_rank][a], &c, &s);
								m_R[m_rank][a - 1] = (c*m_R[m_rank][a - 1]) + (s*m_R[m_rank][a]);
								for (size_t b = 0; b < DIMENSION; b++)
								{
									const float q[] = { m_Q[a - 1][b],m_Q[a][b] };
									m_Q[a - 1][b] = (c*q[0]) + (s*q[1]);
									m_Q[a][b] = (c*q[1]) - (s*q[0]);
								}
							}
							m_S[i].member = true;
							m_members[m_rank + 1] = m_members[m_rank];
							m_members[m_rank] = i;
							m_rank++;
						}
						void CLOAK_CALL_THIS RemovePoint(In size_t i)
						{
							m_S[m_members[i]].member = false;
							if (i == m_rank)
							{
								m_u = m_S[m_members[m_rank]].pos - m_S[m_members[m_rank - 1]].pos;
								m_rank--;
								CloakEngine::Global::Math::Point w(0, 0, 0);
								for (size_t a = 0; a < DIMENSION; a++)
								{
									const CloakEngine::Global::Math::Vector q(m_Q[a][0], m_Q[a][1], m_Q[a][2]);
									w.Data[a] = q.Dot(m_u);
								}
								for (size_t a = DIMENSION - 1; a > 0; a--)
								{
									float c, s;
									givens(w.Data[a - 1], w.Data[a], &c, &s);
									w.Data[a - 1] = (c*w.Data[a - 1]) + (s*w.Data[a]);
									m_R[a - 1][a] = -s*m_R[a - 1][a - 1];
									m_R[a - 1][a - 1] *= c;
									for (size_t b = a; b < m_rank; b++)
									{
										const float r[] = { m_R[b][a - 1],m_R[b][a] };
										m_R[b][a - 1] = (c*r[0]) + (s*r[1]);
										m_R[b][a] = (c*r[1]) - (s*r[0]);
									}
									for (size_t b = 0; b < DIMENSION; b++)
									{
										const float q[] = { m_Q[a - 1][b],m_Q[a][b] };
										m_Q[a - 1][b] = (c*q[0]) + (s*q[1]);
										m_Q[a][b] = (c*q[1]) - (s*q[0]);
									}
								}
								for (size_t a = 0; a < m_rank; a++)
								{
									m_R[a][0] += w.Data[0];
								}
								Clear(0);
							}
							else
							{
								for (size_t a = i + 1; a < m_rank; a++)
								{
									for (size_t b = 0; b < DIMENSION; b++) { m_R[a - 1][b] = m_R[a][b]; }
									m_members[a - 1] = m_members[a];
								}
								m_members[m_rank - 1] = m_members[m_rank];
								Clear(i);
							}
						}
						size_t CLOAK_CALL_THIS Size() const { return m_rank + 1; }
						float CLOAK_CALL_THIS VectorToSpan(In const CloakEngine::Global::Math::Vector& p, Out CloakEngine::Global::Math::Vector* w)
						{
							*w = m_S[m_members[m_rank]].pos - p;
							for (size_t a = 0; a < m_rank; a++)
							{
								const CloakEngine::Global::Math::Vector q(m_Q[a][0], m_Q[a][1], m_Q[a][2]);
								const float s = w->Dot(q);
								*w -= s*q;
							}
							return w->Dot(*w);
						}
						void CLOAK_CALL_THIS FindCoefficients(In const CloakEngine::Global::Math::Vector& p, Out float lam[DIMENSION + 1])
						{
							m_u = p - m_S[m_members[m_rank]].pos;
							CloakEngine::Global::Math::Point w(0, 0, 0);
							for (size_t a = 0; a < DIMENSION; a++)
							{
								const CloakEngine::Global::Math::Vector q(m_Q[a][0], m_Q[a][1], m_Q[a][2]);
								w.Data[a] = q.Dot(m_u);
							}
							//m_v=w;
							float ol = 1;
							for (size_t a = m_rank; a > 0; a--)
							{
								const size_t c = a - 1;
								for (size_t b = a; b < m_rank; b++)
								{
									w.Data[c] -= lam[b] * m_R[b][c];
								}
								lam[c] = w.Data[c] / m_R[c][c];
								ol -= lam[c];
							}
							lam[m_rank] = ol;
						}
				};
				bool CLOAK_CALL TryDrop(Inout Subspan* support, In const CloakEngine::Global::Math::Vector& Center)
				{
					float lambdas[DIMENSION + 1];
					support->FindCoefficients(Center, lambdas);
					size_t smallest = 0;
					float mini = 1;
					for (size_t a = 0; a < support->Size(); a++)
					{
						if (lambdas[a] < mini)
						{
							mini = lambdas[a];
							smallest = a;
						}
					}
					if (mini <= 0)
					{
						support->RemovePoint(smallest);
						return true;
					}
					return false;
				}
				float CLOAK_CALL FindStop(In const CloakEngine::Global::Math::Vector* points, In size_t size, In const CloakEngine::Global::Math::Vector& center, In const CloakEngine::Global::Math::Vector& centerAff, In float dAff, In float dAffSqt, In float radius, In float radiusSqt, Inout Subspan* support, Out int* stop)
				{
					float scale = 1;
					*stop = -1;
					for (size_t a = 0; a < size; a++)
					{
						if (!support->IsMember(a))
						{
							CloakEngine::Global::Math::Vector centerPoint = points[a] - center;
							const float dpp = centerAff.Dot(centerPoint);
							if (dAffSqt - dpp >= EPSILON_D*dAff*radius)
							{
								const float b = (radiusSqt - centerPoint.Dot(centerPoint)) / (2 * (dAffSqt - dpp));
								if (b > 0 && b < scale)
								{
									scale = b;
									*stop = static_cast<int>(a);
								}
							}
						}
					}
					return scale;
				}
			}
			namespace OOBB {
				constexpr size_t INVALID_EDGE = ~0;
				constexpr size_t LOOP_THRESHOLD = 20;
				constexpr float BOX_ERROR_TOLERANCE = 1e-3f;

				struct Extreme {
					size_t Index;
					float Distance;
				};
				struct ExtremePoints {
					Extreme Smallest;
					Extreme Largest;
				};

				inline ExtremePoints CLOAK_CALL FindExtremePoint(In const CE::Global::Math::Vector& direction, In_reads(size) const CloakEngine::Global::Math::Vector* points, In size_t size)
				{
					CLOAK_ASSUME(size > 0 && points != nullptr);
					ExtremePoints res;
					res.Largest.Index = res.Smallest.Index = 0;
					res.Largest.Distance = -std::numeric_limits<float>::infinity();
					res.Smallest.Distance = std::numeric_limits<float>::infinity();

					for (size_t a = 0; a < size; a++)
					{
						const float d = direction.Dot(points[a]);
						if (d < res.Smallest.Distance)
						{
							res.Smallest.Distance = d;
							res.Smallest.Index = a;
						}
						if (d > res.Largest.Distance)
						{
							res.Largest.Distance = d;
							res.Largest.Index = a;
						}
					}
					return res;
				}
				inline Extreme CLOAK_CALL FindExtremePoint(In const ConvexHull::Hull& hull, In const CE::List<CE::List<size_t>>& adjacencyData, In const CE::Global::Math::Vector& direction, Inout CE::List<uint64_t>* floodFillVisited, In uint64_t floodFillCounter, In size_t startingVertex)
				{
					float curD = direction.Dot(hull.GetVertex(startingVertex));
					CLOAK_ASSUME(curD == curD); //NaN check
					floodFillVisited->at(startingVertex) = floodFillCounter;

					size_t sb = hull.GetVertexCount();
					float sbd = curD - 1e-3f;
					for (size_t a = 0; a < adjacencyData[startingVertex].size();)
					{
						const size_t n = adjacencyData[startingVertex][a++];
						if (floodFillVisited->at(n) != floodFillCounter)
						{
							const float d = direction.Dot(hull.GetVertex(n));
							CLOAK_ASSUME(d == d); //NaN check
							if (d > curD)
							{
								startingVertex = n;
								curD = d;
								floodFillVisited->at(n) = floodFillCounter;
								sb = hull.GetVertexCount();
								sbd = curD - 1e-3f;
								a = 0;
							}
							else if(d > sbd)
							{
								sb = n;
								sbd = d;
							}
						}
					}
					if (sb < hull.GetVertexCount() && floodFillVisited->at(sb) != floodFillCounter)
					{
						const Extreme st = FindExtremePoint(hull, adjacencyData, direction, floodFillVisited, floodFillCounter, sb);
						if (st.Distance > curD) { return st; }
					}
					Extreme r;
					r.Distance = curD;
					r.Index = startingVertex;
					return r;
				}
				inline std::pair<bool, CE::Global::Math::Vector> CLOAK_CALL SolveAxB(In const CE::Global::Math::Vector& v0, In const CE::Global::Math::Vector& v1, In const CE::Global::Math::Vector& v2, In const CE::Global::Math::Vector& b)
				{
					const CE::Global::Math::Point av0 = static_cast<CE::Global::Math::Point>(v0.Abs());
					const CE::Global::Math::Point av1 = static_cast<CE::Global::Math::Point>(v1.Abs());
					CE::Global::Math::Point p0 = static_cast<CE::Global::Math::Point>(v0);
					CE::Global::Math::Point p1 = static_cast<CE::Global::Math::Point>(v1);
					CE::Global::Math::Point p2 = static_cast<CE::Global::Math::Point>(v2);
					CE::Global::Math::Point pb = static_cast<CE::Global::Math::Point>(b);

					//Find largest absolute value:
					if (av0.Y >= av0.X && av0.Y >= av0.Z)
					{
						std::swap(p0.X, p0.Y);
						std::swap(p1.X, p1.Y);
						std::swap(p2.X, p2.Y);
						std::swap(pb.X, pb.Y);
					}
					else if (av0.Z >= av0.X)
					{
						std::swap(p0.X, p0.Z);
						std::swap(p1.X, p1.Z);
						std::swap(p2.X, p2.Z);
						std::swap(pb.X, pb.Z);
					}

					//Solve:
					// v0 v1 v2 | vb
					//----------|---
					// a  b  c  | x
					// d  e  f  | y
					// g  h  i  | z
					//	where |a| >= |d| and |a| >= |g|

					if (CE::Global::Math::abs(p0.X) < EPSILON_F) { return std::make_pair(false, CE::Global::Math::Vector::Zero); }

					//Normalize rows to leading element
					float dn = 1 / p0.X;
					p1.X *= dn;
					p2.X *= dn;
					pb.X *= dn;

					// v0 v1 v2 | vb
					//----------|---
					// 1  b  c  | x
					// d  e  f  | y
					// g  h  i  | z

					//Solve first column
					p1.Y -= p0.Y * p1.X;
					p2.Y -= p0.Y * p2.X;
					pb.Y -= p0.Y * pb.X;

					p1.Z -= p0.Z * p1.X;
					p2.Z -= p0.Z * p2.X;
					pb.Z -= p0.Z * pb.X;

					// v0 v1 v2 | vb
					//----------|---
					// 1  b  c  | x
					// 0  e  f  | y
					// 0  h  i  | z

					//Second pivot:
					if (av1.Z >= av1.Y)
					{
						std::swap(p1.Y, p1.Z);
						std::swap(p2.Y, p2.Z);
						std::swap(pb.Y, pb.Z);
					}

					if (CE::Global::Math::abs(p1.Y) < EPSILON_F) { return std::make_pair(false, CE::Global::Math::Vector::Zero); }

					// v0 v1 v2 | vb
					//----------|---
					// 1  b  c  | x
					// 0  e  f  | y
					// 0  h  i  | z
					//	where |e| >= |h|

					//Normalize rows to second pivot:
					dn = 1 / p1.Y;
					p2.Y *= dn;
					pb.Y *= dn;

					// v0 v1 v2 | vb
					//----------|---
					// 1  b  c  | x
					// 0  1  f  | y
					// 0  h  i  | z

					//Solve second column:
					p2.Z -= p1.Z * p2.Y;
					pb.Z -= p1.Z * pb.Y;

					// v0 v1 v2 | vb
					//----------|---
					// 1  b  c  | x
					// 0  1  f  | y
					// 0  0  i  | z

					if (CE::Global::Math::abs(p2.Z) < EPSILON_F) { return std::make_pair(false, CE::Global::Math::Vector::Zero); }

					CE::Global::Math::Point r;
					r.Z = pb.Z / p2.Z;
					r.Y = pb.Y - (r.Z * p2.Y);
					r.X = pb.X - ((r.Z * p2.X) + (r.Y * p1.X));

					return std::make_pair(true, static_cast<CE::Global::Math::Vector>(r));
				}

				inline bool CLOAK_CALL IsAntipodalToEdge(In const ConvexHull::Hull& hull, In size_t vertex, In const CE::List<size_t>& neighbors, In const CE::Global::Math::Vector& na, In const CE::Global::Math::Vector& nb)
				{
					float mi = 0;
					float ma = 1;

					const CE::Global::Math::Vector v = hull.GetVertex(vertex);
					const CE::Global::Math::Vector nd = nb - na;
					for (size_t a = 0; a < neighbors.size(); a++)
					{
						const CE::Global::Math::Vector e = hull.GetVertex(neighbors[a]) - v;
						const float b = nd.Dot(e);
						const float c = nb.Dot(e);
						if (b > EPSILON_F) { ma = min(ma, c / b); }
						else if (b < -EPSILON_F) { mi = max(mi, c / b); }
						else if (c < -EPSILON_F) { return false; }
						//Interval is degenerate: (-1e-3f is a too strict)
						if (ma - mi < -5e-2f) { return false; }
					}
					return true;
				}
				inline bool CLOAK_CALL IsSidepodalToEdge(In const CE::Global::Math::Vector& n1a, In const CE::Global::Math::Vector& n1b, In const CE::Global::Math::Vector& n2a, In const CE::Global::Math::Vector& n2b)
				{
					const CE::Global::Math::Vector nd1 = n1a - n1b;
					const CE::Global::Math::Vector nd2 = n2a - n2b;
					const float a = n1b.Dot(n2b);
					const float b = nd1.Dot(n2b);
					const float c = nd2.Dot(n1b);
					const float d = nd1.Dot(nd2);

					const float ab = a + b;
					const float ac = a + c;
					const float abcd = ab + c + d;
					const float minV = min(min(a, ab), min(ac, abcd));
					const float maxV = max(max(a, ab), max(ac, abcd));
					return minV <= 0.0f && maxV >= 0.0f;
				}
				inline std::pair<bool, CE::Global::Math::Vector> CLOAK_CALL IsOpposingEdge(In const CE::Global::Math::Vector& n1a, In const CE::Global::Math::Vector& n1b, In const CE::Global::Math::Vector& n2a, In const CE::Global::Math::Vector& n2b)
				{
					const std::pair<bool, CE::Global::Math::Vector> sol = SolveAxB(n2b, n1a - n1b, n2a - n2b, -n1b);
					const CE::Global::Math::Point s = static_cast<CE::Global::Math::Point>(sol.second);
					if (sol.first == false || s.X <= 0 || s.Y < EPSILON_F || s.Y > 1 - EPSILON_F || s.Z < 0 || s.Z > s.X) { return std::make_pair(false, CE::Global::Math::Vector::Zero); }
					const float u = s.Z / s.X;
					if (u < EPSILON_F || u > 1 - EPSILON_F) { return std::make_pair(false, CE::Global::Math::Vector::Zero); }
					return std::make_pair(true, n1b + ((n1a - n1b)*s.Y));
				}
				inline size_t CLOAK_CALL ComputeBasis(In const CE::Global::Math::Vector& n1a, In const CE::Global::Math::Vector& n1b, In const CE::Global::Math::Vector& n2a, In const CE::Global::Math::Vector& n2b, In const CE::Global::Math::Vector& n3a, In const CE::Global::Math::Vector& n3b, Out CE::Global::Math::Vector (&n1)[2], Out CE::Global::Math::Vector(&n2)[2], Out CE::Global::Math::Vector(&n3)[2])
				{
					const CE::Global::Math::Vector a = n1b;
					const CE::Global::Math::Vector b = n1a - n1b;
					const CE::Global::Math::Vector c = n2b;
					const CE::Global::Math::Vector d = n2a - n2b;
					const CE::Global::Math::Vector e = n3b;
					const CE::Global::Math::Vector f = n3a - n3b;

					const float g = (a.Dot(c) * d.Dot(e)) - (a.Dot(d) * c.Dot(e));
					const float h = (a.Dot(c) * d.Dot(f)) - (a.Dot(d) * c.Dot(f));
					const float i = (b.Dot(c) * d.Dot(e)) - (b.Dot(d) * c.Dot(e));
					const float j = (b.Dot(c) * d.Dot(f)) - (b.Dot(d) * c.Dot(f));

					const float k =  (g * b.Dot(e)) - (i * a.Dot(e));
					const float l = ((h * b.Dot(e)) + (g * b.Dot(f))) - ((i * a.Dot(f)) + (j * a.Dot(e)));
					const float m =  (h * b.Dot(f)) - (j * a.Dot(f));

					const float s = (l * l) - (4 * m * k);
					if (CE::Global::Math::abs(m) < 1e-5f || CE::Global::Math::abs(s) < 1e-5f)
					{
						const float v = -k / l;
						const float t = -(g + (h*v)) / (i + (j*v));
						const float u = -(c.Dot(e) + (c.Dot(f) * v)) / (d.Dot(e) + (d.Dot(f) * v));
						if (v >= -EPSILON_F && t >= -EPSILON_F && u >= -EPSILON_F && v <= 1 + EPSILON_F && t <= 1 + EPSILON_F && u <= 1 + EPSILON_F)
						{
							n1[0] = (a + (b * t)).Normalize();
							n2[0] = (c + (d * u)).Normalize();
							n3[0] = (e + (f * v)).Normalize();
							if (CE::Global::Math::abs(n1[0].Dot(n2[0])) < EPSILON_ANGLE && CE::Global::Math::abs(n1[0].Dot(n3[0])) < EPSILON_ANGLE && CE::Global::Math::abs(n2[0].Dot(n3[0])) < EPSILON_ANGLE) { return 1; }
						}
						return 0;
					}
					if (s < 0) { return 0; }
					const float sgnL = l < 0 ? -1.0f : 1.0f;
					const float V0 = -(l + (sgnL * CE::Global::Math::sqrt(s))) / 2;
					const float V1 = V0 / m;
					const float V2 = k / V0;

					const float T1 = -(g + (h * V1)) / (i + (j * V1));
					const float T2 = -(g + (h * V2)) / (i + (j * V2));
					const float U1 = -(c.Dot(e) + (c.Dot(f) * V1)) / (d.Dot(e) + (d.Dot(f) * V1));
					const float U2 = -(c.Dot(e) + (c.Dot(f) * V2)) / (d.Dot(e) + (d.Dot(f) * V2));

					size_t r = 0;
					if (V1 >= -EPSILON_F && T1 >= -EPSILON_F && U1 >= -EPSILON_F && V1 <= 1 + EPSILON_F && T1 <= 1 + EPSILON_F && U1 <= 1 + EPSILON_F)
					{
						n1[r] = (a + (b*T1)).Normalize();
						n2[r] = (c + (d*U1)).Normalize();
						n3[r] = (e + (f*V1)).Normalize();

						if (CE::Global::Math::abs(n1[r].Dot(n2[r])) < EPSILON_ANGLE && CE::Global::Math::abs(n1[r].Dot(n3[r])) < EPSILON_ANGLE && CE::Global::Math::abs(n2[r].Dot(n3[r])) < EPSILON_ANGLE) { r++; }
					}
					if (V2 >= -EPSILON_F && T2 >= -EPSILON_F && U2 >= -EPSILON_F && V2 <= 1 + EPSILON_F && T2 <= 1 + EPSILON_F && U2 <= 1 + EPSILON_F)
					{
						n1[r] = (a + (b*T2)).Normalize();
						n2[r] = (c + (d*U2)).Normalize();
						n3[r] = (e + (f*V2)).Normalize();

						if (CE::Global::Math::abs(n1[r].Dot(n2[r])) < EPSILON_ANGLE && CE::Global::Math::abs(n1[r].Dot(n3[r])) < EPSILON_ANGLE && CE::Global::Math::abs(n2[r].Dot(n3[r])) < EPSILON_ANGLE) { r++; }
					}
					if (s < EPSILON_F && r == 2) { r = 1; }
					return r;
				}
				inline bool CLOAK_CALL IsBoxValid(In const BoundingOOBB& minOOBB, In_reads(size) const CE::Global::Math::Vector* data, In size_t size)
				{
#ifdef CHECK_BOX_VALIDITY
					//Check if all points are in box:
					float maxRelError = -1;
					float maxAbsError = -1;
					size_t axisErrAbs = 4;
					size_t axisErrRel = 4;
					CE::Global::Math::Vector pointErrAbs = CE::Global::Math::Vector::Zero;
					CE::Global::Math::Vector pointErrRel = CE::Global::Math::Vector::Zero;
					for (size_t a = 0; a < size; a++)
					{
						const CE::Global::Math::Vector d = data[a] - minOOBB.Center;
						for (size_t b = 0; b < 3; b++)
						{
							const float p = minOOBB.Axis[b].Dot(d);
							if (CE::Global::Math::abs(p) > minOOBB.HalfSize[b])
							{
								const float ae = CE::Global::Math::abs(p) - minOOBB.HalfSize[b];
								const float pe = ae / minOOBB.HalfSize[b];
								if (ae > maxAbsError) { maxAbsError = ae; axisErrAbs = b; pointErrAbs = d; }
								if (pe > maxRelError) { maxRelError = pe; axisErrRel = b; pointErrRel = d; }
							}
						}
					}
					const bool res = maxAbsError < BOX_ERROR_TOLERANCE;
					CLOAK_ASSUME(res);
					return res;
#else
					return true;
#endif
				}

#define NEXT_POINT(p) points[(p+1) % points.size()]
				inline BoundingOOBB CLOAK_CALL CalculateBounding2D(In_reads(size) const CE::Global::Math::Vector* data, In size_t size)
				{
					BoundingOOBB res;
					res.Enabled = false;
					CLOAK_ASSUME(size >= 3);

					CE::Global::Math::Vector cn = CE::Global::Math::Vector::Zero;
					for (size_t a = 2; a < size; a++)
					{
						CE::Global::Math::Vector n = (data[a] - data[a - 2]).Cross(data[a] - data[a - 1]);
						if (n.Length() > 0)
						{
							n = n.Normalize();
							if (cn.Dot(n) >= 0) { cn += n; }
							else { cn -= n; }
							cn = cn.Normalize();
						}
					}

					CE::Global::Math::Vector edgeA = CE::Global::Math::Vector::Zero;
					CE::Global::Math::Vector edgeB = CE::Global::Math::Vector::Zero;

					//Smallest volume jiggle:
					size_t notImproved = 0;
					float bestVolume = std::numeric_limits<float>::infinity();
					CE::Global::Math::Vector edgeChoice[2] = { cn,cn };
					CE::List<ConvexHull::Point2D> points;
					do {
						points.resize(size);
						const auto ep = FindExtremePoint(cn, data, size);
						const float edgeLen = CE::Global::Math::abs((data[ep.Largest.Index] - data[ep.Smallest.Index]).Dot(cn));
						const std::pair<CE::Global::Math::Vector, CE::Global::Math::Vector> basis = cn.GetOrthographicBasis();

						for (size_t a = 0; a < size; a++) { points[a] = ConvexHull::Point2D(basis.first.Dot(data[a]), basis.second.Dot(data[a]), a); }

						//Check if points are CCW-oriented:
						for (size_t a = 2; a <= points.size(); a++)
						{
							if (ConvexHull::IsCounterClockwiseOriented(points[a - 2], points[a - 1], points[a % points.size()]) == false)
							{
								//Recreate convex hull in-place:
								const size_t h = ConvexHull::CalculateConvexHull2DInPlace(points.data(), points.size());
								points.resize(h);
								break;
							}
						}

						//Handle too small point sets:
						if (points.size() == 1)
						{
							res.Center = data[points[0].Vertex];
							res.Enabled = true;
							res.Axis[0] = CE::Global::Math::Vector::Right;
							res.Axis[1] = CE::Global::Math::Vector::Top;
							res.Axis[2] = CE::Global::Math::Vector::Forward;
							res.HalfSize[0] = 0;
							res.HalfSize[1] = 0;
							res.HalfSize[2] = 0;
							return res;
						}
						else if (points.size() == 2)
						{
							const CE::Global::Math::Vector ps[2] = { data[points[0].Vertex], data[points[1].Vertex] };
							const CE::Global::Math::Vector d = ps[1] - ps[0];

							res.Center = (ps[0] + ps[1]) * 0.5f;
							res.Axis[0] = d.Normalize();

							std::pair<CE::Global::Math::Vector, CE::Global::Math::Vector> basis = res.Axis[0].GetOrthographicBasis();
							res.Axis[1] = basis.first;
							res.Axis[2] = basis.second;
							res.HalfSize[0] = d.Length() * 0.5f;
							res.HalfSize[1] = 0;
							res.HalfSize[2] = 0;
							return res;
						}
						//antipodal point pairs: e[0] and e[2] are pairs, as well as e[1] and e[3]
						size_t e[4] = { 0, 0, 0, 0 };

						//Compute initial AABB with counter-clockwise orientation:
						for (size_t a = 1; a < points.size(); a++)
						{
							if (points[a].X < points[e[0]].X) { e[0] = a; }
							else if (points[a].X > points[e[2]].X) { e[2] = a; }
							if (points[a].Y < points[e[1]].Y) { e[1] = a; }
							else if (points[a].Y > points[e[3]].Y) { e[3] = a; }
						}

						//direction of currently tested edge:
						ConvexHull::Point2D dir(0, -1);
						float minArea = std::numeric_limits<float>::infinity();
						
						//track directions of convex hull at each antipodal point:
						ConvexHull::Point2D d[4];
						d[0] = (NEXT_POINT(e[0]) - points[e[0]]).Normalize();
						d[1] = (NEXT_POINT(e[1]) - points[e[1]]).Normalize();
						d[2] = (NEXT_POINT(e[2]) - points[e[2]]).Normalize();
						d[3] = (NEXT_POINT(e[3]) - points[e[3]]).Normalize();

						float minR[2] = { std::numeric_limits<float>::infinity(),  std::numeric_limits<float>::infinity() };
						float maxR[2] = { -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity() };
						ConvexHull::Point2D dirR[2];

						//Rotate by 90 degrees to see each edge that might support bounding rectangle
						while (dir.Y <= 0)
						{
							const float cosA[] = {
								dir.Dot(d[0]),
								dir.PerpendicularDot(d[1]),
								-dir.Dot(d[2]),
								-dir.PerpendicularDot(d[3]),
							};

							size_t lc = 0;
							for (size_t a = 1; a < ARRAYSIZE(cosA); a++) { if (cosA[a] > cosA[lc]) { lc = a; } }
							dir = d[lc];
							e[lc] = (e[lc] + 1) % points.size();
							d[lc] = (NEXT_POINT(e[lc]) - points[e[lc]]).Normalize();
							//Rotate dir 'lc' times 90 degrees in clockwise direction:
							for (size_t a = 0; a < lc; a++) { std::swap(dir.X, dir.Y); dir.Y = -dir.Y; }

							const float minU = dir.PerpendicularDot(points[e[0]]);
							const float maxU = dir.PerpendicularDot(points[e[2]]);
							const float minV = dir.Dot(points[e[1]]);
							const float maxV = dir.Dot(points[e[3]]);

							const float area = CE::Global::Math::abs((maxU - minU) * (maxV - minV));
							if (area < minArea)
							{
								dirR[1] = dir;
								minArea = area;
								minR[0] = min(minU, maxU);
								maxR[0] = max(minU, maxU);
								minR[1] = min(minV, maxV);
								maxR[1] = max(minV, maxV);
							}
						}
						dirR[0].X = -dirR[1].Y;
						dirR[0].Y = dirR[1].X;
						const ConvexHull::Point2D center = 0.5f*((dirR[0] * (minR[0] + maxR[0])) + (dirR[1] * (minR[1] + maxR[1])));

						const ConvexHull::Point2D c10 = (maxR[1] - minR[1]) * dirR[1];
						const ConvexHull::Point2D c20 = (maxR[0] - minR[0]) * dirR[0];

						const CE::Global::Math::Vector edge0 = (c10.X * basis.first) + (c10.Y * basis.second);
						const CE::Global::Math::Vector edge1 = (c20.X * basis.first) + (c20.Y * basis.second);

						const float volume = edge0.Length() * edge1.Length() * edgeLen;
						if (volume < bestVolume)
						{
							bestVolume = volume;
							edgeChoice[0] = (dirR[0].X * basis.first) + (dirR[0].Y * basis.second);
							float l = edgeChoice[0].Length();
							CLOAK_ASSUME(l > 0);
							edgeChoice[0] = edgeChoice[0].Normalize();
							CLOAK_ASSUME(CE::Global::Math::abs(edgeChoice[0].Dot(cn)) < EPSILON_F);

							edgeChoice[1] = (dirR[1].X * basis.first) + (dirR[1].Y * basis.second);
							l = edgeChoice[1].Length();
							CLOAK_ASSUME(l > 0);
							edgeChoice[1] = edgeChoice[1].Normalize();
							CLOAK_ASSUME(CE::Global::Math::abs(edgeChoice[1].Dot(cn)) < EPSILON_F);
							CLOAK_ASSUME(CE::Global::Math::abs(edgeChoice[0].Dot(edgeChoice[1])) < EPSILON_F);

							edgeA = cn;
							edgeB = edgeChoice[0];
							cn = edgeChoice[0];
							notImproved = 0;
						}
						else
						{
							notImproved++;
							cn = edgeChoice[1];
						}
					} while (notImproved <= 1);

					//Fix orientation:
					const CE::Global::Math::Vector edgeC = edgeA.Cross(edgeB).Normalize();
					const ExtremePoints epA = FindExtremePoint(edgeA, data, size);
					const ExtremePoints epB = FindExtremePoint(edgeB, data, size);
					const ExtremePoints epC = FindExtremePoint(edgeC, data, size);

					const float rdA = (epA.Largest.Distance - epA.Smallest.Distance) * 0.5f;
					const float rdB = (epB.Largest.Distance - epB.Smallest.Distance) * 0.5f;
					const float rdC = (epC.Largest.Distance - epC.Smallest.Distance) * 0.5f;

					res.Enabled = true;
					res.Center = ((epA.Smallest.Distance + rdA) * edgeA) + ((epB.Smallest.Distance + rdB) * edgeB) + ((epC.Smallest.Distance + rdC) * edgeC);
					res.Axis[0] = edgeA;
					res.Axis[1] = edgeB;
					res.Axis[2] = edgeC;
					res.HalfSize[0] = rdA;
					res.HalfSize[1] = rdB;
					res.HalfSize[2] = rdC;
					return res;
				}

				inline BoundingOOBB CLOAK_CALL OptimalBounding(In const ConvexHull::Hull& hull, In_reads(size) const CE::Global::Math::Vector* data, In size_t size)
				{
					BoundingOOBB minOOBB;
					minOOBB.Enabled = false;
					float minVolume = std::numeric_limits<float>::infinity();

					if (hull.GetFaceCount() <= 1 || hull.GetVertexCount() <= 3) { return minOOBB; }
					CE::List<CE::List<size_t>> adjacencyData(hull.GetVertexCount(), CE::List<size_t>());
					//Calculate adjacency data:
					for (size_t a = 0; a < hull.GetFaceCount(); a++)
					{
						const ConvexHull::HullPolygon& face = hull.GetFace(a);
						size_t vl = face.Vertex[ARRAYSIZE(face.Vertex) - 1];
						for (size_t b = 0; b < ARRAYSIZE(face.Vertex); b++)
						{
							const size_t vn = face.Vertex[b];
							adjacencyData[vl].push_back(vn);
							vl = vn;
						}
					}
					//Precalculate face normals:
					CE::List<CE::Global::Math::Vector> faceNormals;
					faceNormals.reserve(hull.GetFaceCount());
					for (size_t a = 0; a < hull.GetFaceCount(); a++) 
					{ 
						faceNormals.push_back(hull.GetPlane(a).GetNormal()); 
					}
					
					//All edges in hull. Uses full edges (v0 -> v1 and v1 -> v0 are the same edge)
					CE::List<std::pair<size_t, size_t>> edges;
					//Includes both faces an edge corresponds to
					CE::List<std::pair<size_t, size_t>> edgeToFaces;
					edges.reserve(hull.GetVertexCount() * 2);
					edgeToFaces.reserve(hull.GetVertexCount() * 2);
					//get edge index of two vertices. Contains entries for v0 -> v1 and v1 -> v0
					CE::List<size_t> vertexToEdges(hull.GetVertexCount()*hull.GetVertexCount(), INVALID_EDGE);

#define VERTEX_TO_EDGE(v0, v1) vertexToEdges[((v0) * hull.GetVertexCount()) + (v1)]
#define IS_INTERNAL_EDGE(i) (faceNormals[edgeToFaces[(i)].first].Dot(faceNormals[edgeToFaces[(i)].second]) > 1.0f - 1e-3f)

					//Precompute edge data structures:
					for (size_t a = 0; a < hull.GetFaceCount(); a++)
					{
						const ConvexHull::HullPolygon& face = hull.GetFace(a);
						size_t vl = face.Vertex[ARRAYSIZE(face.Vertex) - 1];
						for (size_t b = 0; b < ARRAYSIZE(face.Vertex); b++)
						{
							const size_t vn = face.Vertex[b];
							std::pair<size_t, size_t> edge = std::make_pair(vl, vn);
							if (VERTEX_TO_EDGE(vl, vn) == INVALID_EDGE)
							{
								VERTEX_TO_EDGE(vl, vn) = edges.size();
								VERTEX_TO_EDGE(vn, vl) = edges.size();
								edges.push_back(edge);
								edgeToFaces.push_back(std::make_pair(a, hull.GetFaceCount())); //second value will be filled later
							}
							else { edgeToFaces[VERTEX_TO_EDGE(vl, vn)].second = a; }
							vl = vn;
						}
					}

					//Flood Fill search structure:
					CE::List<uint64_t> floodFillVisited(hull.GetVertexCount(), 0);
					uint64_t floodFillCounter = 1;

					CE::List<CE::List<size_t>> antipodalPointsOfEdge(edges.size());
					CE::List<size_t> faceOrder;
					CE::List<size_t> edgeOrder;

					//Random number generation:
					std::random_device rd;
					std::default_random_engine re(rd());

					{
						CE::List<std::pair<size_t, size_t>> stack;
						CE::List<bool> visitedEdges(edges.size(), false);
						CE::List<bool> visitedFaces(hull.GetFaceCount(), false);
						stack.push_back(std::make_pair(0, adjacencyData[0][0]));
						do {
							std::pair<size_t, size_t> e = stack.back();
							stack.pop_back();
							const size_t edge = VERTEX_TO_EDGE(e.first, e.second);
							if (visitedEdges[edge] == true) { continue; }
							visitedEdges[edge] = true;
							if (visitedFaces[edgeToFaces[edge].first] == false)
							{
								visitedFaces[edgeToFaces[edge].first] = true;
								faceOrder.push_back(edgeToFaces[edge].first);
							}
							if (visitedFaces[edgeToFaces[edge].second] == false)
							{
								visitedFaces[edgeToFaces[edge].second] = true;
								faceOrder.push_back(edgeToFaces[edge].second);
							}
							if (IS_INTERNAL_EDGE(edge) == false) { edgeOrder.push_back(edge); }
							const size_t os = stack.size();
							for (size_t a = 0; a < adjacencyData[e.second].size(); a++)
							{
								const size_t vn = adjacencyData[e.second][a];
								const size_t ne = VERTEX_TO_EDGE(e.second, vn);
								if (visitedEdges[ne] == true) { continue; }
								stack.push_back(std::make_pair(e.second, vn));
							}
							//Continue with a random new edge:
							const size_t ns = stack.size() - os;
							if (ns > 0)
							{
								std::uniform_int_distribution<size_t> id(0, ns - 1);
								const size_t r = id(re);
								std::swap(stack.back(), stack[os + r]);
							}
						} while (stack.empty() == false);
					}

					CE::Stack<size_t> stack;
					//Exploiting spatial locality during searches:
					size_t searchStart = 0;
#ifdef SIMPLE_ANTIPODAL_ALGORITHM
					//Calculate antipodal points in an rather simple way:
					for (size_t a = 0; a < edges.size(); a++)
					{
						const CE::Global::Math::Vector& na = faceNormals[edgeToFaces[a].first];
						const CE::Global::Math::Vector& nb = faceNormals[edgeToFaces[a].second];
						for (size_t b = 0; b < hull.GetVertexCount(); b++)
						{
							if (IsAntipodalToEdge(hull, b, adjacencyData[b], na, nb)) { antipodalPointsOfEdge[a].push_back(b); }
						}
					}
#else
					//Calculate antipodal points the smart way:
					for (size_t a = 0; a < edgeOrder.size(); a++)
					{
						const size_t b = edgeOrder[a];
						const CE::Global::Math::Vector& na = faceNormals[edgeToFaces[b].first];
						const CE::Global::Math::Vector& nb = faceNormals[edgeToFaces[b].second];

						floodFillCounter++;
						searchStart = FindExtremePoint(hull, adjacencyData, -na, &floodFillVisited, floodFillCounter, searchStart).Index;
						floodFillCounter++;
						stack.push(searchStart);
						floodFillVisited[searchStart] = floodFillCounter;
						do {
							const size_t v = stack.top();
							stack.pop();
							const CE::List<size_t>& neighbors = adjacencyData[v];
							if (IsAntipodalToEdge(hull, v, neighbors, na, nb))
							{
								if (edges[b].first == v || edges[b].second == v)
								{
									//Input set is planar
									return CalculateBounding2D(data, size);
								}
								antipodalPointsOfEdge[b].push_back(v);
								for (size_t c = 0; c < neighbors.size(); c++)
								{
									if (floodFillVisited[neighbors[c]] != floodFillCounter)
									{
										stack.push(neighbors[c]);
										floodFillVisited[neighbors[c]] = floodFillCounter;
									}
								}
							}
						} while (stack.empty() == false);

						// The above search may fail due to numerical imprecision (happens very rarely). In that case, fall back to simple linear search.
						if (antipodalPointsOfEdge[b].empty())
						{
							for (size_t c = 0; c < hull.GetVertexCount(); c++)
							{
								if (IsAntipodalToEdge(hull, c, adjacencyData[c], na, nb)) { antipodalPointsOfEdge[b].push_back(c); }
							}
						}
					}
#endif
					//For each edge, store sidepodal edges
					CE::List<CE::List<size_t>> sidepodalEdges(edges.size());
					CE::List<bool> sidepodalVertices(edges.size()*hull.GetVertexCount(), false);

#define SIDEPODAL_VERTEX(e1, e2) sidepodalVertices[((e1) * hull.GetVertexCount()) + (e2)]

#ifdef SIMPLE_SIDEPODAL_ALGORITHM
					//Calculate sidepodal edges using a very simple algorithm
					for (size_t a = 0; a < edges.size(); a++)
					{
						const CE::Global::Math::Vector& na = faceNormals[edgeToFaces[a].first];
						const CE::Global::Math::Vector& nb = faceNormals[edgeToFaces[a].second];
						//An edge can be it's own sidepodal, so also include self-check!
						for (size_t b = a; b < edges.size(); b++)
						{
							if (IsSidepodalToEdge(na, nb, faceNormals[edgeToFaces[b].first], faceNormals[edgeToFaces[b].second]))
							{
								sidepodalEdges[a].push_back(b);
								SIDEPODAL_VERTEX(a, edges[b].first) = true;
								SIDEPODAL_VERTEX(a, edges[b].second) = true;
								if (a != b)
								{
									sidepodalEdges[b].push_back(a);
									SIDEPODAL_VERTEX(b, edges[a].first) = true;
									SIDEPODAL_VERTEX(b, edges[a].second) = true;
								}
							}
						}
					}
#else
					//Compute sidepodal edges using a graph search:
					for (size_t a = 0; a < edgeOrder.size(); a++)
					{
						const size_t b = edgeOrder[a];
						const CE::Global::Math::Vector& na = faceNormals[edgeToFaces[b].first];
						const CE::Global::Math::Vector& nb = faceNormals[edgeToFaces[b].second];
						const CE::Global::Math::Vector deadDir = (na + nb)*0.5f;
						const std::pair<CE::Global::Math::Vector, CE::Global::Math::Vector> basis = deadDir.GetOrthographicBasis();

						const CE::Global::Math::Vector dir = na.GetOrthographicBasis().first;
						floodFillCounter++;
						searchStart = FindExtremePoint(hull, adjacencyData, dir, &floodFillVisited, floodFillCounter, searchStart).Index;
						floodFillCounter++;
						stack.push(searchStart);
						do {
							const size_t v = stack.top();
							stack.pop();
							if (floodFillVisited[v] == floodFillCounter) { continue; }
							floodFillVisited[v] = floodFillCounter;
							const CE::List<size_t>& neighbors = adjacencyData[v];
							for (size_t c = 0; c < neighbors.size(); c++)
							{
								const size_t va = neighbors[c];
								if (floodFillVisited[va] == floodFillCounter) { continue; }
								const size_t edge = VERTEX_TO_EDGE(v, va);
								if (IsSidepodalToEdge(na, nb, faceNormals[edgeToFaces[edge].first], faceNormals[edgeToFaces[edge].second]))
								{
									if (b <= edge)
									{
										if (IS_INTERNAL_EDGE(edge) == false) { sidepodalEdges[b].push_back(edge); }
										SIDEPODAL_VERTEX(b, edges[edge].first) = true;
										SIDEPODAL_VERTEX(b, edges[edge].second) = true;
										if (b != edge)
										{
											if (IS_INTERNAL_EDGE(edge) == false) { sidepodalEdges[edge].push_back(b); }
											SIDEPODAL_VERTEX(edge, edges[b].first) = true;
											SIDEPODAL_VERTEX(edge, edges[b].second) = true;
										}
									}
									stack.push(va);
								}
							}
						} while (stack.empty() == false);
					}
#endif
					//Explot spatial locality:
					size_t searchHints[7] = { 0,0,0,0,0,0,0 };
					CE::Queue<size_t> queue;
					CE::List<size_t> antipodalEdges;
					CE::List<CE::Global::Math::Vector> antipodalNormals;

					//Test all configurations where all three edges are on adjacent faces:
					for (size_t a = 0; a < edgeOrder.size(); a++)
					{
						const size_t edgeA = edgeOrder[a];
						const CE::Global::Math::Vector& n1a = faceNormals[edgeToFaces[edgeA].first];
						const CE::Global::Math::Vector& n1b = faceNormals[edgeToFaces[edgeA].second];
						const CE::Global::Math::Vector deadDir1 = (n1a + n1b)*0.5f;

						const CE::List<size_t>& sidepodal = sidepodalEdges[edgeA];
						for (size_t c = 0; c < sidepodal.size(); c++)
						{
							const size_t edgeB = sidepodal[c];
							if (edgeB < edgeA) { continue; }
							const CE::Global::Math::Vector& n2a = faceNormals[edgeToFaces[edgeB].first];
							const CE::Global::Math::Vector& n2b = faceNormals[edgeToFaces[edgeB].second];
							const CE::Global::Math::Vector deadDir2 = (n2a + n2b)*0.5f;

							CE::Global::Math::Vector searchDir = deadDir1.Cross(deadDir2);
							float len = searchDir.Length();
							if (len == 0)
							{
								searchDir = n1a.Cross(n2a);
								len = searchDir.Length();
								if (len == 0) { searchDir = n1a.GetOrthographicBasis().first; }
							}
							searchDir = searchDir.Normalize();

							floodFillCounter++;
							searchHints[0] = FindExtremePoint(hull, adjacencyData, searchDir, &floodFillVisited, floodFillCounter, searchHints[0]).Index;
							floodFillCounter++;
							searchHints[1] = FindExtremePoint(hull, adjacencyData, -searchDir, &floodFillVisited, floodFillCounter, searchHints[1]).Index;
							size_t secondSearch = hull.GetVertexCount();
							if (SIDEPODAL_VERTEX(edgeB, searchHints[0])) { stack.push(searchHints[0]); }
							else { queue.push(searchHints[0]); }
							if (SIDEPODAL_VERTEX(edgeB, searchHints[1])) { stack.push(searchHints[1]); }
							else { secondSearch = searchHints[1]; }
							//Walk to a good vertex that is sidepodal to both edges ('edge' and 'b')
							floodFillCounter++;
							while (queue.empty() == false)
							{
								const size_t v = queue.front();
								queue.pop();
								if (floodFillVisited[v] == floodFillCounter) { continue; }
								floodFillVisited[v] = floodFillCounter;
								const CE::List<size_t>& neighbors = adjacencyData[v];
								for (size_t d = 0; d < neighbors.size(); d++)
								{
									const size_t va = neighbors[d];
									if (floodFillVisited[va] != floodFillCounter && SIDEPODAL_VERTEX(edgeA, va))
									{
										if (SIDEPODAL_VERTEX(edgeB, va))
										{
											queue.clear();
											if (secondSearch < hull.GetVertexCount())
											{
												queue.push(secondSearch);
												secondSearch = hull.GetVertexCount();
												floodFillVisited[va] = floodFillCounter;
											}
											stack.push(va);
											break;
										}
										else { queue.push(va); }
									}
								}
							}

							floodFillCounter++;
							while (stack.empty() == false)
							{
								const size_t v = stack.top();
								stack.pop();
								if (floodFillVisited[v] == floodFillCounter) { continue; }
								floodFillVisited[v] = floodFillCounter;
								const CE::List<size_t>& neighbors = adjacencyData[v];
								for (size_t d = 0; d < neighbors.size(); d++)
								{
									const size_t va = neighbors[d];
									const size_t edgeC = VERTEX_TO_EDGE(v, va);
									if (IS_INTERNAL_EDGE(edgeC)) { continue; }
									if (SIDEPODAL_VERTEX(edgeA, va) && SIDEPODAL_VERTEX(edgeB, va))
									{
										if (floodFillVisited[va] != floodFillCounter) { stack.push(va); }
										if (edgeB < edgeC)
										{
											const CE::Global::Math::Vector& n3a = faceNormals[edgeToFaces[edgeC].first];
											const CE::Global::Math::Vector& n3b = faceNormals[edgeToFaces[edgeC].second];
											CE::Global::Math::Vector n1[2];
											CE::Global::Math::Vector n2[2];
											CE::Global::Math::Vector n3[2];
											const size_t nSol = ComputeBasis(n1a, n1b, n2a, n2b, n3a, n3b, n1, n2, n3);
											for (size_t s = 0; s < nSol; s++)
											{
												//Compute the most extreme points in each direction:
												float maxN1 = n1[s].Dot(hull.GetVertex(edges[edgeA].first));
												float maxN2 = n2[s].Dot(hull.GetVertex(edges[edgeB].first));
												float maxN3 = n3[s].Dot(hull.GetVertex(edges[edgeC].first));
												float minN1 = std::numeric_limits<float>::infinity();
												float minN2 = std::numeric_limits<float>::infinity();
												float minN3 = std::numeric_limits<float>::infinity();

												for (size_t x = 0; x < antipodalPointsOfEdge[edgeA].size(); x++)
												{
													const float y = n1[s].Dot(hull.GetVertex(antipodalPointsOfEdge[edgeA][x]));
													minN1 = min(minN1, y);
												}
												for (size_t x = 0; x < antipodalPointsOfEdge[edgeB].size(); x++)
												{
													const float y = n2[s].Dot(hull.GetVertex(antipodalPointsOfEdge[edgeB][x]));
													minN2 = min(minN2, y);
												}
												for (size_t x = 0; x < antipodalPointsOfEdge[edgeC].size(); x++)
												{
													const float y = n3[s].Dot(hull.GetVertex(antipodalPointsOfEdge[edgeC][x]));
													minN3 = min(minN3, y);
												}

												//Sanity checks:
												if (minN1 > maxN1 || minN2 > maxN2 || minN3 > maxN3) { continue; }

												//Check if that OOBB is better then the current:
												const float volume = (maxN1 - minN1) * (maxN2 - minN2) * (maxN3 - minN3);
												CLOAK_ASSUME(volume >= 0);
												if (volume < minVolume)
												{
													BoundingOOBB nOOBB;

													nOOBB.Axis[0] = n1[s];
													nOOBB.Axis[1] = n2[s];
													nOOBB.Axis[2] = n3[s];
													nOOBB.HalfSize[0] = (maxN1 - minN1) * 0.5f;
													nOOBB.HalfSize[1] = (maxN2 - minN2) * 0.5f;
													nOOBB.HalfSize[2] = (maxN3 - minN3) * 0.5f;
													nOOBB.Center = ((maxN1 + minN1) * 0.5f * n1[s]) + ((maxN2 + minN2) * 0.5f * n2[s]) + ((maxN3 + minN3) * 0.5f * n3[s]);
													nOOBB.Enabled = true;
													if (IsBoxValid(nOOBB, data, size))
													{
														minVolume = volume;
														minOOBB = nOOBB;
													}
												}
											}
										}
									}
								}
							}
						}
					}

					//Test all configurations where OOBB is flush with convex hull endges:
					for (size_t a = 0; a < edgeOrder.size(); a++)
					{
						const size_t edgeA = edgeOrder[a];
						const CE::Global::Math::Vector& n1a = faceNormals[edgeToFaces[edgeA].first];
						const CE::Global::Math::Vector& n1b = faceNormals[edgeToFaces[edgeA].second];

						antipodalEdges.clear();
						antipodalNormals.clear();

						const CE::List<size_t>& antipodals = antipodalPointsOfEdge[edgeA];
						for (size_t b = 0; b < antipodals.size(); b++)
						{
							const size_t v = antipodals[b];
							const CE::List<size_t>& neighbors = adjacencyData[v];
							for (size_t c = 0; c < neighbors.size(); c++)
							{
								const size_t va = neighbors[c];
								if (va < v) { continue; } //We search unordered edges, so we will encounter both edge v->va and va->v, but we only need one of them
								
								const size_t edgeB = VERTEX_TO_EDGE(v, va);
								if (edgeA > edgeB || IS_INTERNAL_EDGE(edgeB)) { continue; }

								const CE::Global::Math::Vector& n2a = faceNormals[edgeToFaces[edgeB].first];
								const CE::Global::Math::Vector& n2b = faceNormals[edgeToFaces[edgeB].second];
								const std::pair<bool, CE::Global::Math::Vector> N = IsOpposingEdge(n1a, n1b, n2a, n2b);
								if (N.first == true)
								{
									antipodalEdges.push_back(edgeB);
									antipodalNormals.push_back(N.second.Normalize());
								}
							}
						}
						const CE::List<size_t>& sidepodal = sidepodalEdges[edgeA];
						for (size_t b = 0; b < sidepodal.size(); b++)
						{
							const size_t edgeB = sidepodal[b];
							for (size_t c = 0; c < antipodalEdges.size(); c++)
							{
								const size_t edgeC = antipodalEdges[c];
								const CE::Global::Math::Vector& n1 = antipodalNormals[c];

								float minN1 = n1.Dot(hull.GetVertex(edges[edgeC].first));
								float maxN1 = n1.Dot(hull.GetVertex(edges[edgeA].first));

								//Test all compatible edges:
								const CE::Global::Math::Vector& n2a = faceNormals[edgeToFaces[edgeB].first];
								const CE::Global::Math::Vector& n2b = faceNormals[edgeToFaces[edgeB].second];
								float nu = n1.Dot(n2b);
								float de = n1.Dot(n2b - n2a);
								if (de < 0) { nu = -nu; de = -de; }

								if (de < EPSILON_F)
								{
									nu = CE::Global::Math::abs(nu) < EPSILON_F ? 0.0f : -1.0f;
									de = 1.0f;
								}

								if (nu >= de * -EPSILON_F && nu <= de * (1.0f + EPSILON_F))
								{
									const float v = nu / de;
									const CE::Global::Math::Vector n3 = (n2b + ((n2a - n2b) * v)).Normalize();
									const CE::Global::Math::Vector n2 = n3.Cross(n1).Normalize();

									floodFillCounter++;
									auto hf = FindExtremePoint(hull, adjacencyData, n2, &floodFillVisited, floodFillCounter, c == 0 ? searchHints[0] : searchHints[4]);
									if (c == 0) { searchHints[0] = hf.Index; }
									else { searchHints[4] = hf.Index; }

									const float maxN2 = hf.Distance;

									floodFillCounter++;
									hf = FindExtremePoint(hull, adjacencyData, -n2, &floodFillVisited, floodFillCounter, c == 0 ? searchHints[1] : searchHints[5]);
									if (c == 0) { searchHints[1] = hf.Index; }
									else { searchHints[5] = hf.Index; }

									const float minN2 = -hf.Distance;

									const float maxN3 = n3.Dot(hull.GetVertex(edges[edgeB].first));
									float minN3 = std::numeric_limits<float>::infinity();
									const CE::List<size_t>& antipodalsB = antipodalPointsOfEdge[edgeB];
									if (antipodalsB.size() < LOOP_THRESHOLD) // Small number of vertices, just do a tight loop
									{
										for (size_t d = 0; d < antipodalsB.size(); d++) 
										{
											const float nm = n3.Dot(hull.GetVertex(antipodalsB[d]));
											minN3 = min(minN3, nm);
										}
									}
									else // Large number of vertices, do graph search
									{
										floodFillCounter++;
										hf = FindExtremePoint(hull, adjacencyData, -n3, &floodFillVisited, floodFillCounter, c == 0 ? searchHints[2] : searchHints[6]);
										if (c == 0) { searchHints[2] = hf.Index; }
										else { searchHints[6] = hf.Index; }

										minN3 = -hf.Distance;
									}

									//Sanity checks:
									if (minN1 > maxN1 || minN2 > maxN2 || minN3 > maxN3) { continue; }

									//Check if that OOBB is better then the current:
									const float volume = (maxN1 - minN1) * (maxN2 - minN2) * (maxN3 - minN3);
									CLOAK_ASSUME(volume >= 0);
									if (volume < minVolume)
									{
										BoundingOOBB nOOBB;
										nOOBB.Axis[0] = n1;
										nOOBB.Axis[1] = n2;
										nOOBB.Axis[2] = n3;
										nOOBB.HalfSize[0] = (maxN1 - minN1) * 0.5f;
										nOOBB.HalfSize[1] = (maxN2 - minN2) * 0.5f;
										nOOBB.HalfSize[2] = (maxN3 - minN3) * 0.5f;
										nOOBB.Center = ((maxN1 + minN1) * 0.5f * n1) + ((maxN2 + minN2) * 0.5f * n2) + ((maxN3 + minN3) * 0.5f * n3);
										nOOBB.Enabled = true;
										if (IsBoxValid(nOOBB, data, size))
										{
											minVolume = volume;
											minOOBB = nOOBB;
										}
									}
								}
							}
						}
					}

					//Test all configurations where OOBB touches two edges of the same face:
					for (size_t a = 0; a < faceOrder.size(); a++)
					{
						const size_t b = faceOrder[a];
						const CE::Global::Math::Vector& n1 = faceNormals[b];
						const ConvexHull::HullPolygon& face = hull.GetFace(b);

						//Selecting two edges on the face. Try to find a pair with mostly mutually exclusive sidepodal sets to
						//speed up following searches
						size_t edgeA = edges.size();
						size_t vl = face.Vertex[ARRAYSIZE(face.Vertex) - 1];
						for (size_t c = 0; c < ARRAYSIZE(face.Vertex); c++)
						{
							const size_t vn = face.Vertex[c];
							const size_t e = VERTEX_TO_EDGE(vl, vn);
							if (IS_INTERNAL_EDGE(e) == false) { edgeA = e; break; }
							vl = vn;
						}
						if (edgeA == edges.size()) { continue; } //Only has internal edges
						
						const CE::List<size_t>& antipodals = antipodalPointsOfEdge[edgeA];
						const CE::List<size_t>& sidepodals = sidepodalEdges[edgeA];

						const float maxN1 = n1.Dot(hull.GetVertex(edges[edgeA].first));

						float minN1 = std::numeric_limits<float>::infinity();
						if (antipodals.size() < LOOP_THRESHOLD)
						{
							for (size_t c = 0; c < antipodals.size(); c++)
							{
								const float d = n1.Dot(hull.GetVertex(antipodals[c]));
								minN1 = min(minN1, d);
							}
						}
						else
						{
							floodFillCounter++;
							auto fe = FindExtremePoint(hull, adjacencyData, -n1, &floodFillVisited, floodFillCounter, searchHints[3]);
							searchHints[3] = fe.Index;
							minN1 = -fe.Distance;
						}

						//Sanity check:
						if (minN1 > maxN1) { continue; }

						for (size_t c = 0; c < sidepodals.size(); c++)
						{
							const size_t edgeB = sidepodals[c];
							const CE::Global::Math::Vector& n2a = faceNormals[edgeToFaces[edgeB].first];
							const CE::Global::Math::Vector& n2b = faceNormals[edgeToFaces[edgeB].second];

							const float nu = n1.Dot(n2b);
							const float de = n1.Dot(n2b - n2a);
							float v = 0.0f;
							if (CE::Global::Math::abs(de) >= EPSILON_F) { v = nu / de; }
							else if (CE::Global::Math::abs(nu) < EPSILON_F) { v = 0.0f; }
							else { v = -1.0f; }

							if (v >= -EPSILON_F && v <= 1.0f + EPSILON_F)
							{
								const CE::Global::Math::Vector n3 = (n2b + ((n2a - n2b)*v)).Normalize();
								const CE::Global::Math::Vector n2 = n3.Cross(n1).Normalize();

								floodFillCounter++;
								auto ef = FindExtremePoint(hull, adjacencyData, n2, &floodFillVisited, floodFillCounter, searchHints[0]);
								searchHints[0] = ef.Index;
								const float maxN2 = ef.Distance;

								floodFillCounter++;
								ef = FindExtremePoint(hull, adjacencyData, -n2, &floodFillVisited, floodFillCounter, searchHints[1]);
								searchHints[1] = ef.Index;
								const float minN2 = -ef.Distance;

								const float maxN3 = n3.Dot(hull.GetVertex(edges[edgeB].first));

								const CE::List<size_t>& antipodalsB = antipodalPointsOfEdge[edgeB];
								float minN3 = std::numeric_limits<float>::infinity();
								if (antipodalsB.size() < LOOP_THRESHOLD)
								{
									for (size_t d = 0; d < antipodalsB.size(); d++)
									{
										const float e = n3.Dot(hull.GetVertex(antipodalsB[d]));
										minN3 = min(minN3, e);
									}
								}
								else
								{
									floodFillCounter++;
									ef = FindExtremePoint(hull, adjacencyData, -n3, &floodFillVisited, floodFillCounter, searchHints[2]);
									searchHints[2] = ef.Index;
									minN3 = -ef.Distance;
								}

								//Sanity checks:
								if (minN2 > maxN2 || minN3 > maxN3) { continue; }

								//Check if that OOBB is better then the current:
								const float volume = (maxN1 - minN1) * (maxN2 - minN2) * (maxN3 - minN3);
								CLOAK_ASSUME(volume >= 0);
								if (volume < minVolume)
								{
									BoundingOOBB nOOBB;
									nOOBB.Axis[0] = n1;
									nOOBB.Axis[1] = n2;
									nOOBB.Axis[2] = n3;
									nOOBB.HalfSize[0] = (maxN1 - minN1) * 0.5f;
									nOOBB.HalfSize[1] = (maxN2 - minN2) * 0.5f;
									nOOBB.HalfSize[2] = (maxN3 - minN3) * 0.5f;
									nOOBB.Center = ((maxN1 + minN1) * 0.5f * n1) + ((maxN2 + minN2) * 0.5f * n2) + ((maxN3 + minN3) * 0.5f * n3);
									nOOBB.Enabled = true;
									if (IsBoxValid(nOOBB, data, size))
									{
										minVolume = volume;
										minOOBB = nOOBB;
									}
								}
							}
						}
					}

					CLOAK_ASSUME(minVolume < std::numeric_limits<float>::infinity()); //Check that we actualy got a valid box

					//Fix cross-product orientation:
					if (minOOBB.Axis[0].Cross(minOOBB.Axis[1]).Dot(minOOBB.Axis[2]) < 0) { minOOBB.Axis[2] = -minOOBB.Axis[2]; }

					//Slightly increase box size to accommodate for error tolerance:
					for (size_t a = 0; a < 3; a++) { minOOBB.HalfSize[a] = CE::Global::Math::abs(minOOBB.HalfSize[a]) + BOX_ERROR_TOLERANCE; }

					return minOOBB;
				}
			}
			//Calculates minimal bounding sphere using the algorithm by Kaspar Fischer, Bernd Gärtner and Martin Kutz
			BoundingSphere CLOAK_CALL CalculateBoundingSphere(In_reads(size) const CloakEngine::Global::Math::Vector* points, In size_t size)
			{
				CloakEngine::Global::Math::Vector Center = points[0];
				size_t furth = 0;
				float radius;
				float radiusSqt = 0;
				for (size_t a = 0; a < size; a++)
				{
					CloakEngine::Global::Math::Vector dv = Center - points[a];
					float d = dv.Dot(dv);
					if (d > radiusSqt)
					{
						radiusSqt = d;
						furth = a;
					}
				}
				radius = sqrtf(radiusSqt);
				Sphere::Subspan support(points, size, furth);
				CloakEngine::Global::Math::Vector CenterAff;
				float distAff;
				float distAffSqt;
				while (true)
				{
					bool walk = true;
					do {
						distAffSqt = support.VectorToSpan(Center, &CenterAff);
						distAff = sqrtf(distAffSqt);
						walk = distAff <= EPSILON_D*radius;
						if (walk && !Sphere::TryDrop(&support, Center))
						{
							BoundingSphere res;
							res.Center = Center;
							res.Radius = radius;
							return res;
						}
					} while (walk);
					int stop;
					float scale = Sphere::FindStop(points, size, Center, CenterAff, distAff, distAffSqt, radius, radiusSqt, &support, &stop);
					if (stop >= 0 && support.Size() <= DIMENSION)
					{
						Center += scale*CenterAff;
						const CloakEngine::Global::Math::Vector& sp = points[support.GetAny()];
						CloakEngine::Global::Math::Vector dv = Center - sp;
						radiusSqt = dv.Dot(dv);
						radius = sqrtf(radiusSqt);
						support.AddPoint(stop);
					}
					else
					{
						Center += CenterAff;
						const CloakEngine::Global::Math::Vector& sp = points[support.GetAny()];
						CloakEngine::Global::Math::Vector dv = Center - sp;
						radiusSqt = dv.Dot(dv);
						radius = sqrtf(radiusSqt);
						if (!Sphere::TryDrop(&support, Center))
						{
							BoundingSphere res;
							res.Center = Center;
							res.Radius = radius;
							return res;
						}
					}
				}
			}



			//Calculates minimal object oriented bounding box using the algorithm by Jukka Jyänki
			//	See https://github.com/juj/MathGeoLib/blob/master/src/Geometry/OBB.cpp
			BoundingOOBB CLOAK_CALL CalculateBoundingOOBB(In_reads(size) const CloakEngine::Global::Math::Vector* points, In size_t size)
			{
				ConvexHull::Hull hull;
				ConvexHull::CalculateSingleConvexHull(points, size, &hull);
				BoundingOOBB res;
				if (hull.GetFaceCount() <= 1 || hull.GetVertexCount() <= 3 || hull.IsFlat())
				{
					switch (size)
					{
						case 1:
						{
							res.Center = points[0];
							res.Enabled = true;
							res.Axis[0] = CE::Global::Math::Vector::Right;
							res.Axis[1] = CE::Global::Math::Vector::Top;
							res.Axis[2] = CE::Global::Math::Vector::Forward;
							res.HalfSize[0] = 0;
							res.HalfSize[1] = 0;
							res.HalfSize[2] = 0;
							return res;
						}
						case 2:
						{
							const CE::Global::Math::Vector d = points[1] - points[0];

							res.Center = (points[0] + points[1]) * 0.5f;
							res.Axis[0] = d.Normalize();
							
							std::pair<CE::Global::Math::Vector, CE::Global::Math::Vector> basis = res.Axis[0].GetOrthographicBasis();
							res.Axis[1] = basis.first;
							res.Axis[2] = basis.second;
							res.HalfSize[0] = d.Length() * 0.5f;
							res.HalfSize[1] = 0;
							res.HalfSize[2] = 0;
							return res;
						}
						default:
						{
							return OOBB::CalculateBounding2D(points, size);
						}
					}
				}
				return OOBB::OptimalBounding(hull, points, size);
			}
		}
	}
}