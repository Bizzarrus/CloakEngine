#include "stdafx.h"
#include "CloakCompiler/Mesh.h"
#include "CloakEngine/Files/ExtendedBuffers.h"

#include "Engine/TempHandler.h"
#include "Engine/BoundingVolume.h"

#include <assert.h>
#include <sstream>

//#define DEBUG_ENFORCE_STRIPS
//#define DEBUG_ENFORCE_NO_STRIPS
#define ALLOW_STRIP_LIST_MIX

#define STRIP_FUNC(name) size_t name(In size_t triID, In size_t swapEdge, In const CE::List<HalfEdge>& edges, In const CE::List<Triangle>& faces, In uint64_t floodFillVisited, In bool firstCall)
#define PRINT_STRIPS

namespace CloakCompiler {
	namespace API {
		namespace Mesh {
			typedef std::function<void(const RespondeInfo&)> RespondeFunc;
			constexpr float g_floatDelta = 0.0001f;
			constexpr size_t STRIP_CUT_VALUE = 0xFFFFFFFF;
			constexpr size_t STRIP_CUT_VALUE_16 = 0xFFFF;
			constexpr size_t MAX_IB16_SIZE = 1 << 16;
			//Minimum required number of single triangles to not use them as strip
			//	Since draw calls have a cost, we prefere to have a bit longer index buffers than additional 
			//	draw calls with a few triangles. This value is just a guess, so it might change in future
			constexpr size_t MIN_TRI_COUNT = 128; 
			enum class IndexBufferType {
				IB16,
				IB16ShortCut,
				IB32,
			};
			struct FinalVertex {
				CloakEngine::Global::Math::Vector Position;
				CloakEngine::Global::Math::Vector Normal;
				CloakEngine::Global::Math::Vector Binormal;
				CloakEngine::Global::Math::Vector Tangent;
				size_t MaterialID;
				TexCoord TexCoord;
				Bone Bones[4];
				bool CLOAK_CALL CompareForIndex(In const FinalVertex& b) const
				{
					bool r = Position == b.Position;
					r = r && Normal == b.Normal;
					r = r && Binormal == b.Binormal;
					r = r && Tangent == b.Tangent;
					r = r && abs(TexCoord.U - b.TexCoord.U) < g_floatDelta;
					r = r && abs(TexCoord.V - b.TexCoord.V) < g_floatDelta;
					for (size_t a = 0; a < 4 && r; a++)
					{
						r = r && Bones[a].BoneID == b.Bones[a].BoneID;
						r = r && abs(Bones[a].Weight - b.Bones[a].Weight) < g_floatDelta;
					}
					return r;
				}
			};
			struct MaterialRange {
				size_t MaterialID;
				size_t ListCount;
				size_t StripCount;
			};
			constexpr size_t VertexSize = sizeof(FinalVertex) - sizeof(size_t);

			constexpr CloakEngine::Files::FileType g_TempFileType{ "MeshTemp","CEMT",1000 };

			namespace Strips {
				//Number of indices the triangle-list index buffer should at least include to justify the additional draw call:
				constexpr size_t MIN_TRIANGLE_LIST_SIZE = 128;
				//Percentage of indices (relative to the triangle-strip index buffer) the triangle-list index buffer should at least include to justify the additional draw call:
				constexpr float MIN_TRIANGLE_LIST_PERCENTAGE = 0.1f;

				struct HalfEdge {
					size_t Vertex;
					size_t OppositeEdge;
				};
				struct Triangle {
					size_t Edge;
					size_t AdjacentCount;
					size_t MaterialID;
					uint64_t FloodFillVisited;
				};
				CLOAK_FORCEINLINE constexpr size_t TriangleByEdge(In size_t edgeID)
				{
					return edgeID / 3;
				}
				CLOAK_FORCEINLINE constexpr size_t FirstEdgeOfTrinalge(In size_t edgeID)
				{
					return edgeID * 3;
				}
				CLOAK_FORCEINLINE constexpr size_t NextEdge(In size_t edgeID)
				{
					const size_t tID = TriangleByEdge(edgeID);
					return (3 * tID) + ((edgeID + 1) % 3);
				}
				CLOAK_FORCEINLINE constexpr size_t PrevEdge(In size_t edgeID)
				{
					const size_t tID = TriangleByEdge(edgeID);
					return (3 * tID) + ((edgeID + 2) % 3);
				}
				inline size_t CLOAK_CALL AdjacentTriangleCount(In size_t triangleID, In const CE::List<HalfEdge>& edges, In const CE::List<Triangle>& faces, In uint64_t floodFillVisited)
				{
					if (triangleID >= edges.size() / 3) { return 0; }
					size_t res = 0;
					for (size_t a = 0; a < 3; a++)
					{
						const size_t i = (3 * triangleID) + a;
						const size_t j = edges[i].OppositeEdge;
						if (j < edges.size() && faces[TriangleByEdge(j)].MaterialID == faces[TriangleByEdge(j)].MaterialID && faces[TriangleByEdge(j)].FloodFillVisited != floodFillVisited) { res++; }
					}
					return res;
				}
				inline void CLOAK_CALL UpdateVisited(In size_t triangleID, In const CE::List<HalfEdge>& edges, Inout CE::List<Triangle>* faces, In uint64_t floodFillVisited)
				{
					faces->at(triangleID).FloodFillVisited = floodFillVisited;
					for (size_t a = 0, b = FirstEdgeOfTrinalge(triangleID); a < 3; a++, b = NextEdge(b))
					{
						const size_t e = edges[b].OppositeEdge;
						if (e != edges.size())
						{
							const size_t t = TriangleByEdge(e);
							faces->at(t).AdjacentCount = AdjacentTriangleCount(t, edges, *faces, floodFillVisited);
						}
					}
				}
				typedef STRIP_FUNC((*StripFunc));

				STRIP_FUNC(LNLS)
				{
					size_t bstEdge = edges.size();
					size_t adjNum = static_cast<size_t>(-1);
					for (size_t a = FirstEdgeOfTrinalge(triID), b = 0; b < 3; b++, a = NextEdge(a))
					{
						const size_t opEdge = edges[a].OppositeEdge;
						if (opEdge < edges.size() && faces[TriangleByEdge(opEdge)].FloodFillVisited != floodFillVisited)
						{
							const size_t adjTri = TriangleByEdge(opEdge);
							const size_t adj = AdjacentTriangleCount(adjTri, edges, faces, floodFillVisited);
							//Check if edge require swap:
							const bool swap = !firstCall && a == swapEdge;

							if (adj < adjNum || (adj == adjNum && swap == false))
							{
								adjNum = adj;
								bstEdge = a;
							}
						}
					}
					return bstEdge;
				}
				STRIP_FUNC(LNLN)
				{
					size_t bstEdge = edges.size();
					size_t adjNum = static_cast<size_t>(-1);
					size_t adjNumSec = static_cast<size_t>(-1);
					for (size_t a = FirstEdgeOfTrinalge(triID), b = 0; b < 3; b++, a = NextEdge(a))
					{
						const size_t opEdge = edges[a].OppositeEdge;
						if (opEdge < edges.size() && faces[TriangleByEdge(opEdge)].FloodFillVisited != floodFillVisited)
						{
							const size_t adjTri = TriangleByEdge(opEdge);
							const size_t adj = AdjacentTriangleCount(adjTri, edges, faces, floodFillVisited);
							const bool swap = !firstCall && a == swapEdge;

							size_t adjSec = static_cast<size_t>(-1);
							if (adj <= adjNum)
							{
								//Look one step ahead:
								for (size_t c = adjTri * 3, d = 0; d < 3; d++, c = NextEdge(c))
								{
									const size_t opSec = edges[c].OppositeEdge;
									if (opSec < edges.size() && opSec != a && faces[TriangleByEdge(opSec)].FloodFillVisited != floodFillVisited)
									{
										const size_t adjTriSec = TriangleByEdge(opSec);
										const size_t sadj = AdjacentTriangleCount(adjTriSec, edges, faces, floodFillVisited);
										adjSec = min(adjSec, sadj);
									}
								}
							}

							if (adj < adjNum || (adj == adjNum && adjSec < adjNumSec) || (adj == adjNum && adjSec == adjNumSec && swap == false))
							{
								adjNum = adj;
								adjNumSec = adjSec;
								bstEdge = a;
							}
						}
					}
					return bstEdge;
				}

				constexpr StripFunc Functions[] = { LNLS, LNLN };


				inline bool CLOAK_CALL CalculateIndexBufferStrips(In const CE::List<size_t>& indexBaseBuffer, Inout CE::List<size_t>* indexBuffer, Inout CloakEngine::List<MaterialRange>* materialRanges)
				{
					bool res = false;
					size_t firstIndexPos = 0;
#ifndef DEBUG_ENFORCE_NO_STRIPS
#ifdef DEBUG_ENFORCE_STRIPS
					indexBuffer->clear();
#endif
					//Create half-edge structure:
					CE::List<Triangle> faces(indexBaseBuffer.size() / 3);
					CE::List<HalfEdge> edges(faces.size() * 3);
					//This array allows to find an edge by two entries of an index buffer:
					CE::FlatMap<std::pair<size_t, size_t>, size_t> vertexToEdge;
					//To test whether we included an face already in our new index buffer, we use a flood fill counter
					uint64_t floodFillVisited = 1;
#define VERTEX_TO_EDGE(v0, v1) vertexToEdge[std::make_pair((v0),(v1))]
#define VERTEX_EDGE_EXIST(v0, v1) (vertexToEdge.find(std::make_pair((v0), (v1))) != vertexToEdge.end())

					//Calculate edges:
					for (size_t a = 0; a < indexBaseBuffer.size(); a += 3)
					{
						size_t vli = a + 2;
						for (size_t b = 0; b < 3; b++)
						{
							const size_t vni = a + b;
							const size_t vl = indexBaseBuffer[vli];
							const size_t vn = indexBaseBuffer[vni];
							edges[vni].Vertex = vl;
							edges[vni].OppositeEdge = edges.size();
							VERTEX_TO_EDGE(vl, vn) = vni;
							vli = vni;
						}
					}
					//Initialize face values:
					for (size_t a = 0; a < faces.size(); a++) 
					{ 
						faces[a].Edge = a * 3; 
						faces[a].FloodFillVisited = 0;
						faces[a].AdjacentCount = 0;
						faces[a].MaterialID = ~0;
					}
					//Initialize material IDs:
					size_t firstEdge = 0;
					for (size_t a = 0; a < materialRanges->size(); a++)
					{
						CLOAK_ASSUME(materialRanges->at(a).StripCount == 0);
						const size_t lastEdge = min(firstEdge + materialRanges->at(a).ListCount, edges.size());
						CLOAK_ASSUME(lastEdge % 3 == 0);
						for (size_t c = firstEdge; c < lastEdge; c += 3)
						{
							const size_t t = TriangleByEdge(c);
							faces[t].MaterialID = materialRanges->at(a).MaterialID;
						}
						firstEdge = lastEdge;
					}
					//Calculate opposite edges:
					for (size_t a = 0; a < indexBaseBuffer.size(); a += 3)
					{
						size_t vli = a + 2;
						for (size_t b = 0; b < 3; b++)
						{
							const size_t vni = a + b;
							const size_t vl = indexBaseBuffer[vli];
							const size_t vn = indexBaseBuffer[vni];
							const size_t eM = VERTEX_TO_EDGE(vl, vn);
							if (VERTEX_EDGE_EXIST(vn, vl))
							{
								const size_t eO = VERTEX_TO_EDGE(vn, vl);
								const size_t tM = TriangleByEdge(eM);
								const size_t tO = TriangleByEdge(eO);
								if (faces[tM].MaterialID == faces[tO].MaterialID)
								{
									edges[eM].OppositeEdge = eO;
									vli = vni;
									continue;
								}
							}
							edges[eM].OppositeEdge = edges.size();
							vli = vni;
						}
					}

					CE::List<size_t> newIBStrip;
					CE::List<size_t> newIBList;
					firstEdge = 0;
					for (size_t a = 0; a < materialRanges->size(); a++)
					{
						CLOAK_ASSUME(materialRanges->at(a).StripCount == 0);
						if (materialRanges->at(a).ListCount < 3) { continue; }
						const size_t lastEdge = min(firstEdge + materialRanges->at(a).ListCount, edges.size());
						CLOAK_ASSUME(lastEdge % 3 == 0);
						//Calculate triangle strips:
						for (size_t b = 0; b < ARRAYSIZE(Functions); b++)
						{
							//Main Algorithm:
							do {
								//Find best face (with lowest adjacent count) to start with:
								size_t face = faces.size();
								size_t bstAdjC = ~0;
								for (size_t c = firstEdge; c < lastEdge; c += 3)
								{
									const size_t t = TriangleByEdge(c);
									faces[t].AdjacentCount = AdjacentTriangleCount(t, edges, faces, floodFillVisited);
									if (faces[t].FloodFillVisited != floodFillVisited && faces[t].AdjacentCount < bstAdjC)
									{
										bstAdjC = faces[t].AdjacentCount;
										face = t;
									}
								}
								if (face == faces.size()) { break; } // No more faces
								UpdateVisited(face, edges, &faces, floodFillVisited);

								size_t edge = Functions[b](face, edges.size(), edges, faces, floodFillVisited, true);
								if (edge == edges.size())
								{
									//Single triangle, all adjacent triangles were already used:
									edge = FirstEdgeOfTrinalge(face);
#ifdef ALLOW_STRIP_LIST_MIX
									newIBList.push_back(edges[edge].Vertex);
									edge = NextEdge(edge);
									newIBList.push_back(edges[edge].Vertex);
									edge = NextEdge(edge);
									newIBList.push_back(edges[edge].Vertex);
#else
									newIBStrip.push_back(edges[edge].Vertex);
									edge = NextEdge(edge);
									newIBStrip.push_back(edges[edge].Vertex);
									edge = NextEdge(edge);
									newIBStrip.push_back(edges[edge].Vertex);
									newIBStrip.push_back(STRIP_CUT_VALUE);
#endif
								}
								else
								{
									const size_t startEdge = edge;
									bool extendSecondDir = false;

									//Insert first triangle:
									newIBStrip.push_back(edges[PrevEdge(edge)].Vertex);
									newIBStrip.push_back(edges[edge].Vertex);

									size_t curEdge = edges[edge].OppositeEdge;
									CLOAK_ASSUME(curEdge < edges.size());
									newIBStrip.push_back(edges[curEdge].Vertex);
									//Walk along strip:
									do {

									insert_ccw_triangle:
										size_t swapEdge = NextEdge(curEdge);
										face = TriangleByEdge(curEdge);
										UpdateVisited(face, edges, &faces, floodFillVisited);

										edge = Functions[b](face, swapEdge, edges, faces, floodFillVisited, false);
										if (edge == edges.size())
										{
											//Strip ended (insert strip cut twice to enforce even number of indices)
											newIBStrip.push_back(edges[PrevEdge(curEdge)].Vertex);
											newIBStrip.push_back(STRIP_CUT_VALUE);
											newIBStrip.push_back(STRIP_CUT_VALUE);
											break;
										}
										else if (edge == swapEdge)
										{
											//Swap
											newIBStrip.push_back(edges[edge].Vertex);
											curEdge = edges[edge].OppositeEdge;
											CLOAK_ASSUME(curEdge < edges.size());
											newIBStrip.push_back(edges[curEdge].Vertex);
											goto insert_ccw_triangle;
										}
										else
										{
											newIBStrip.push_back(edges[edge].Vertex);
											curEdge = edges[edge].OppositeEdge;
											CLOAK_ASSUME(curEdge < edges.size());
										}

									insert_cw_triangle:
										swapEdge = PrevEdge(curEdge);
										face = TriangleByEdge(curEdge);
										UpdateVisited(face, edges, &faces, floodFillVisited);

										edge = Functions[b](face, swapEdge, edges, faces, floodFillVisited, false);
										if (edge == edges.size())
										{
											//Strip ended
											newIBStrip.push_back(edges[PrevEdge(curEdge)].Vertex);
											newIBStrip.push_back(STRIP_CUT_VALUE);
											break;
										}
										else if (edge == swapEdge)
										{
											//Swap
											newIBStrip.push_back(edges[curEdge].Vertex);
											newIBStrip.push_back(edges[edge].Vertex);
											curEdge = edges[edge].OppositeEdge;
											CLOAK_ASSUME(curEdge < edges.size());
											goto insert_cw_triangle;
										}
										else
										{
											curEdge = edges[edge].OppositeEdge;
											CLOAK_ASSUME(curEdge < edges.size());
											newIBStrip.push_back(edges[curEdge].Vertex);
										}
									} while (true);

									//Try to extend in second direction:
									if (extendSecondDir == false)
									{
										extendSecondDir = true;
										size_t swapEdge = NextEdge(startEdge);
										face = TriangleByEdge(startEdge);

										edge = Functions[b](face, swapEdge, edges, faces, floodFillVisited, false);

										if (edge < edges.size())
										{
											//Remove last strip cuts from strip:
											while (newIBStrip.empty() == false && newIBStrip.back() == STRIP_CUT_VALUE) { newIBStrip.pop_back(); }
											//Reverse strip:
											//	To reverse the strip, we need an even amount of indices in the strip. Otherwise, we would also reverse face directions
											if (newIBStrip.size() % 2 != 0) { newIBStrip.push_back(newIBStrip.back()); }
											for (size_t a = 0; a < newIBStrip.size() >> 1; a++) { std::swap(newIBStrip[a], newIBStrip[newIBStrip.size() - (a + 1)]); }

											if (edge == swapEdge)
											{
												newIBStrip.pop_back();
												newIBStrip.push_back(edges[edge].Vertex);
												curEdge = edges[edge].OppositeEdge;
												CLOAK_ASSUME(curEdge < edges.size());
												newIBStrip.push_back(edges[curEdge].Vertex);
												goto insert_ccw_triangle;
											}
											else
											{
												curEdge = edges[edge].OppositeEdge;
												CLOAK_ASSUME(curEdge < edges.size());
												goto insert_cw_triangle;
											}
										}
									}
								}
							} while (true);

							//If the number of single triangles in the list does not justify a second draw call, we will merge it into the strips:
							if (newIBStrip.size() > 0 && (newIBList.size() < MIN_TRIANGLE_LIST_SIZE || newIBList.size() < static_cast<size_t>(newIBStrip.size() * MIN_TRIANGLE_LIST_PERCENTAGE)))
							{
								for (size_t a = 0; a < newIBList.size(); a += 3)
								{
									newIBStrip.push_back(newIBList[a + 0]);
									newIBStrip.push_back(newIBList[a + 1]);
									newIBStrip.push_back(newIBList[a + 2]);
									newIBStrip.push_back(STRIP_CUT_VALUE);
								}
								newIBList.clear();
							}
							//Remove last strip cuts from strip:
							while (newIBStrip.empty() == false && newIBStrip.back() == STRIP_CUT_VALUE) { newIBStrip.pop_back(); }

							//Compare and copy new index buffer:
#ifndef DEBUG_ENFORCE_STRIPS
							if (newIBStrip.size() + newIBList.size() < materialRanges->at(a).ListCount + materialRanges->at(a).StripCount)
#else
							if (b == 0 || newIBStrip.size() + newIBList.size() < materialRanges->at(a).ListCount + materialRanges->at(a).StripCount)
#endif
							{
								res = true;
								materialRanges->at(a).ListCount = newIBList.size();
								materialRanges->at(a).StripCount = newIBStrip.size();
								CLOAK_ASSUME(firstIndexPos + newIBList.size() + newIBStrip.size() <= indexBuffer->size());
#ifndef DEBUG_ENFORCE_STRIPS
								size_t p = firstIndexPos;
								for (size_t b = 0; b < newIBStrip.size(); b++, p++) { indexBuffer->at(p) = newIBStrip[b]; }
								for (size_t b = 0; b < newIBList.size(); b++, p++) { indexBuffer->at(p) = newIBList[b]; }
#else
								indexBuffer->resize(firstIndexPos);
								for (size_t b = 0; b < newIBStrip.size(); b++) { indexBuffer->push_back(newIBStrip[b]); }
								for (size_t b = 0; b < newIBList.size(); b++) { indexBuffer->push_back(newIBList[b]); }
#endif
							}
							newIBStrip.clear();
							newIBList.clear();
							floodFillVisited++;
						}
						firstIndexPos += materialRanges->at(a).ListCount + materialRanges->at(a).StripCount;
						firstEdge = lastEdge;
					}
					indexBuffer->resize(firstIndexPos);
#undef VERTEX_TO_EDGE
#endif
#ifdef PRINT_STRIPS
					CE::Global::Log::WriteToLog("Final index buffer (" + std::to_string(indexBuffer->size()) + " Indices):");
					firstIndexPos = 0;
					for (size_t a = 0; a < materialRanges->size(); a++)
					{
						CE::Global::Log::WriteToLog("\tMaterial " + std::to_string(a));
						for (size_t b = 0, s = 0; b < materialRanges->at(a).StripCount; s++)
						{
							std::stringstream r;
							r << "\t\tStrip " << s << ": ";
							if (indexBuffer->at(firstIndexPos + b) != STRIP_CUT_VALUE)
							{
								r << indexBuffer->at(firstIndexPos + b++);
								while (b < materialRanges->at(a).StripCount && indexBuffer->at(firstIndexPos + b) != STRIP_CUT_VALUE) { r << " | " << indexBuffer->at(firstIndexPos + b++); }
							}
							while (b < materialRanges->at(a).StripCount && indexBuffer->at(firstIndexPos + b) == STRIP_CUT_VALUE) { r << " | CUT"; b++; }
							CE::Global::Log::WriteToLog(r.str());
						}
						if (materialRanges->at(a).ListCount > 0)
						{
							std::stringstream r;
							r << "\t\tList: " << indexBuffer->at(firstIndexPos + materialRanges->at(a).StripCount);
							for (size_t b = 1; b < materialRanges->at(a).ListCount; b++) { r << " | " << indexBuffer->at(firstIndexPos + materialRanges->at(a).StripCount + b); }
							CE::Global::Log::WriteToLog(r.str());
						}
						firstIndexPos += materialRanges->at(a).ListCount + materialRanges->at(a).StripCount;
					}
#endif
					return res;
				}
			}


			inline void CLOAK_CALL SendResponse(In RespondeFunc func, In RespondeCode code, In size_t Polygon, In_opt size_t Vertex = 0, In_opt std::string msg = "")
			{
				RespondeInfo info;
				info.Code = code;
				info.Polygon = Polygon;
				info.Vertex = Vertex;
				info.Msg = msg;
				func(info);
			}
			inline void CLOAK_CALL SendResponse(In RespondeFunc func, In RespondeCode code, In_opt std::string msg = "")
			{
				SendResponse(func, code, 0, 0, msg);
			}
			inline void CLOAK_CALL CopyVertex(In const Vertex& a, In const CloakEngine::Global::Math::Vector& normal, In const CloakEngine::Global::Math::Vector& binormal, In const CloakEngine::Global::Math::Vector& tangent, In uint32_t material, Out FinalVertex* b)
			{
				Bone tb[4];
				//Remove bones with same ID
				for (size_t c = 0; c < 4; c++)
				{
					if (a.Bones[c].Weight > 0)
					{
						bool f = false;
						for (size_t d = 0; d < c && f == false; d++)
						{
							if (tb[d].Weight > 0 && tb[d].BoneID == a.Bones[c].BoneID)
							{
								f = true;
								tb[d].Weight += a.Bones[c].Weight;
							}
						}
						if (f == false)
						{
							tb[c].BoneID = a.Bones[c].BoneID;
							tb[c].Weight = a.Bones[c].Weight;
						}
						else
						{
							tb[c].BoneID = 0;
							tb[c].Weight = 0;
						}
					}
					else
					{
						tb[c].BoneID = 0;
						tb[c].Weight = 0;
					}
				}
				float wNorm = 0;
				//Normalize bone weights
				for (size_t c = 0; c < 4; c++)
				{
					if (tb[c].Weight > 0) { wNorm += tb[c].Weight; }
				}
				//Set final vertex
				b->Position = a.Position;
				b->TexCoord = a.TexCoord;
				b->Normal = normal;
				b->Binormal = binormal;
				b->Tangent = tangent;
				b->MaterialID = material;
				for (size_t c = 0; c < 4; c++) 
				{
					if (tb[c].Weight > 0)
					{
						b->Bones[c].BoneID = tb[c].BoneID;
						b->Bones[c].Weight = tb[c].Weight / wNorm;
					}
					else
					{
						b->Bones[c].BoneID = 0;
						b->Bones[c].Weight = 0;
					}
				}
			}
			inline void CLOAK_CALL CalculatePolygonVertices(In const Vertex& a, In const Vertex& b, In const Vertex& c, In bool calcNorms, In size_t material, Out FinalVertex res[3])
			{
				TexCoord tex[2];
				tex[0].U = b.TexCoord.U - a.TexCoord.U;
				tex[0].V = b.TexCoord.V - a.TexCoord.V;
				tex[1].U = c.TexCoord.U - a.TexCoord.U;
				tex[1].V = c.TexCoord.V - a.TexCoord.V;
				CloakEngine::Global::Math::Vector vec[2];
				vec[0] = static_cast<const CloakEngine::Global::Math::Vector>(b.Position) - a.Position;
				vec[1] = static_cast<const CloakEngine::Global::Math::Vector>(c.Position) - a.Position;

				float det = (tex[0].U*tex[1].V) - (tex[1].U*tex[0].V);
				if (fabsf(det) < 1e-4f)
				{
					tex[0].U = 1;
					tex[0].V = 0;
					tex[1].U = 1;
					tex[1].V = -1;
					det = -1;
				}
				const float den = 1.0f / det;
				CloakEngine::Global::Math::Vector tangent = (den * ((tex[1].V * vec[0]) - (tex[0].V * vec[1]))).Normalize();
				CloakEngine::Global::Math::Vector binormal = (den * ((tex[0].U * vec[1]) - (tex[1].U * vec[0]))).Normalize();

				if (calcNorms)
				{
					CloakEngine::Global::Math::Vector normal = tangent.Cross(binormal).Normalize();

					tangent -= binormal * (binormal.Dot(tangent));
					if (normal.Cross(tangent).Dot(binormal) < 0) { tangent = -tangent; }
					tangent = tangent.Normalize();

					CopyVertex(a, normal, binormal, tangent, static_cast<uint32_t>(material), &res[0]);
					CopyVertex(b, normal, binormal, tangent, static_cast<uint32_t>(material), &res[1]);
					CopyVertex(c, normal, binormal, tangent, static_cast<uint32_t>(material), &res[2]);
				}
				else
				{
					const CloakEngine::Global::Math::Vector an = static_cast<CloakEngine::Global::Math::Vector>(a.Normal).Normalize();
					const CloakEngine::Global::Math::Vector bn = static_cast<CloakEngine::Global::Math::Vector>(b.Normal).Normalize();
					const CloakEngine::Global::Math::Vector cn = static_cast<CloakEngine::Global::Math::Vector>(c.Normal).Normalize();

					const CloakEngine::Global::Math::Vector at = (tangent - (an.Dot(tangent)*an)).Normalize();
					const CloakEngine::Global::Math::Vector bt = (tangent - (bn.Dot(tangent)*bn)).Normalize();
					const CloakEngine::Global::Math::Vector ct = (tangent - (cn.Dot(tangent)*cn)).Normalize();

					const CloakEngine::Global::Math::Vector ab = (binormal - ((an.Dot(binormal)*an) + (at.Dot(binormal)*at))).Normalize();
					const CloakEngine::Global::Math::Vector bb = (binormal - ((bn.Dot(binormal)*bn) + (bt.Dot(binormal)*bt))).Normalize();
					const CloakEngine::Global::Math::Vector cb = (binormal - ((cn.Dot(binormal)*cn) + (ct.Dot(binormal)*ct))).Normalize();

					CopyVertex(a, a.Normal, ab, at, static_cast<uint32_t>(material), &res[0]);
					CopyVertex(b, b.Normal, bb, bt, static_cast<uint32_t>(material), &res[1]);
					CopyVertex(c, c.Normal, cb, ct, static_cast<uint32_t>(material), &res[2]);
				}
			}
			inline IndexBufferType CLOAK_CALL CalculateIndexBuffer(In const FinalVertex* vb, In size_t vbs, Out CE::List<size_t>* ib, Out CE::List<bool>* usedVB, Out CloakEngine::List<MaterialRange>* materialRanges)
			{
				usedVB->resize(vbs);
				ib->clear();
				ib->reserve(vbs);
				materialRanges->clear();
				//Create Index Reference Buffer:
				CE::List<size_t> irb(vbs);
				for (size_t a = 0; a < vbs; a++)
				{
					const FinalVertex& v = vb[a];
					for (size_t b = 0; b < a; b++)
					{
						const FinalVertex& i = vb[irb[b]];
						if (v.CompareForIndex(i))
						{
							usedVB->at(a) = false;
							irb[a] = irb[b];
							goto irb_found_index;
						}
					}
					usedVB->at(a) = true;
					irb[a] = a;
				irb_found_index:
					continue;
				}
				//Create rebased index buffer:
				CE::List<size_t> ibb(vbs);
				ibb[0] = 0;
				for (size_t a = 1; a < vbs; a++)
				{
					if (usedVB->at(a) == true) { ibb[a] = ibb[a - 1] + 1; }
					else { ibb[a] = ibb[a - 1]; }
				}
				for (size_t a = 0; a < vbs; a++)
				{
					size_t p = a;
					while (p != irb[p])
					{
						CLOAK_ASSUME(usedVB->at(p) == false);
						p = irb[p];
					}
					ibb[a] = ibb[p];
				}
				
#ifdef PRINT_STRIPS
				CE::Global::Log::WriteToLog("Simple Index Buffer ("+std::to_string(ibb.size())+" Indices):");
#endif
				size_t ibStart = 0;
				for (size_t a = 0; a < vbs; a++)
				{
					ib->push_back(ibb[a]);
#ifdef PRINT_STRIPS
					CE::Global::Log::WriteToLog("\tIB[" + std::to_string(a) + "] = " + std::to_string(ibb[a]));
#endif
					if (a > 0 && vb[a].MaterialID != vb[a - 1].MaterialID)
					{
						MaterialRange mr;
						mr.ListCount = a - ibStart;
						mr.StripCount = 0;
						mr.MaterialID = vb[a - 1].MaterialID;
						materialRanges->push_back(mr);
						ibStart = a;
					}
				}
				//Add last material range:
				MaterialRange mr;
				mr.ListCount = vbs - ibStart;
				mr.StripCount = 0;
				mr.MaterialID = vb[vbs - 1].MaterialID;
				materialRanges->push_back(mr);

				//Calculate triangle strips:
				IndexBufferType res;
				if (Strips::CalculateIndexBufferStrips(ibb, ib, materialRanges) == false)
				{
					res = ib->size() < MAX_IB16_SIZE ? IndexBufferType::IB16 : IndexBufferType::IB32;
				}
				else
				{
					res = ib->size() < MAX_IB16_SIZE ? IndexBufferType::IB16 : IndexBufferType::IB32;
					if (res == IndexBufferType::IB16)
					{
						for (size_t a = 0; a < ib->size(); a++)
						{
							if (ib->at(a) == STRIP_CUT_VALUE) { res = IndexBufferType::IB16ShortCut; }
						}
					}
				}
				return res;
			}

			inline bool CLOAK_CALL CheckFloat(In CloakEngine::Files::IReader* r, In const float& f)
			{
				const float i = static_cast<float>(r->ReadDouble(32));
				return abs(i - f) < g_floatDelta;
			}
			inline bool CLOAK_CALL CheckVector(In CloakEngine::Files::IReader* r, In const CloakEngine::Global::Math::Vector& v)
			{
				CloakEngine::Global::Math::Point p(v);
				return CheckFloat(r, p.X) && CheckFloat(r, p.Y) && CheckFloat(r, p.Z);
			}
			inline void CLOAK_CALL WriteVector(In CloakEngine::Files::IWriter* w, In const CloakEngine::Global::Math::Vector& v)
			{
				CloakEngine::Global::Math::Point p(v);
				w->WriteDouble(32, p.X);
				w->WriteDouble(32, p.Y);
				w->WriteDouble(32, p.Z);
			}
			inline bool CLOAK_CALL CheckTemp(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In RespondeFunc func)
			{
				bool suc = false;
				if ((encode.flags & EncodeFlags::NO_TEMP_READ) == EncodeFlags::NONE)
				{
					CloakEngine::Files::IReader* read = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&read));
					if (read != nullptr)
					{
						const std::u16string tmpPath = encode.tempPath;

						suc = read->SetTarget(tmpPath, g_TempFileType, false, true) == g_TempFileType.Version;
						if (suc) { SendResponse(func, RespondeCode::CHECK_TMP); }
						if (suc) { suc = !Engine::TempHandler::CheckGameID(read, encode.targetGameID); }
						if (suc) { suc = static_cast<BoundingVolume>(read->ReadBits(8)) == desc.Bounding; }
						if (suc) { suc = read->ReadDynamic() == desc.Vertices.size(); }
						if (suc) { suc = read->ReadDynamic() == desc.Polygons.size(); }
						if (suc)
						{
							for (size_t a = 0; a < desc.Polygons.size() && suc; a++)
							{
								const Polygon& p = desc.Polygons[a];
								suc = suc && ((read->ReadBits(1) == 1) == p.AutoGenerateNormal);
								suc = suc && (read->ReadBits(32) == p.Material);
								suc = suc && (read->ReadBits(32) == p.Point[0]);
								suc = suc && (read->ReadBits(32) == p.Point[1]);
								suc = suc && (read->ReadBits(32) == p.Point[2]);
							}
						}
						if (suc)
						{
							for (size_t a = 0; a < desc.Vertices.size() && suc; a++)
							{
								const Vertex& v = desc.Vertices[a];
								suc = suc && CheckVector(read, v.Position);
								suc = suc && CheckVector(read, v.Normal);
								suc = suc && CheckFloat(read, v.TexCoord.U);
								suc = suc && CheckFloat(read, v.TexCoord.V);
								for (size_t b = 0; b < 4 && suc; b++)
								{
									suc = suc && (read->ReadBits(32) == v.Bones[b].BoneID);
									suc = suc && CheckFloat(read, v.Bones[b].Weight);
								}
							}
						}
						if (suc)
						{
							uint32_t bys = static_cast<uint32_t>(read->ReadBits(32));
							uint8_t bis = static_cast<uint8_t>(read->ReadBits(3));
							for (uint32_t a = 0; a < bys; a++) { output->WriteBits(8, read->ReadBits(8)); }
							if (bis > 0) { output->WriteBits(bis, read->ReadBits(bis)); }
						}
					}
					SAVE_RELEASE(read);
				}
				return !suc;
			}
			inline void CLOAK_CALL WriteTemp(In CloakEngine::Files::IVirtualWriteBuffer* data, In uint32_t bys, In uint8_t bis, In const EncodeDesc& encode, In const Desc& desc, In RespondeFunc response)
			{
				if ((encode.flags & EncodeFlags::NO_TEMP_WRITE) == EncodeFlags::NONE)
				{
					SendResponse(response, RespondeCode::WRITE_TMP);
					CloakEngine::Files::IWriter* write = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&write));
					write->SetTarget(encode.tempPath, g_TempFileType, CloakEngine::Files::CompressType::NONE);
					Engine::TempHandler::WriteGameID(write, encode.targetGameID);
					write->WriteBits(8, static_cast<uint8_t>(desc.Bounding));
					write->WriteDynamic(desc.Vertices.size());
					write->WriteDynamic(desc.Polygons.size());
					for (size_t a = 0; a < desc.Polygons.size(); a++)
					{
						const Polygon& p = desc.Polygons[a];
						write->WriteBits(1, p.AutoGenerateNormal ? 1 : 0);
						write->WriteBits(32, p.Material);
						write->WriteBits(32, p.Point[0]);
						write->WriteBits(32, p.Point[1]);
						write->WriteBits(32, p.Point[2]);
					}
					for (size_t a = 0; a < desc.Vertices.size(); a++)
					{
						const Vertex& v = desc.Vertices[a];
						WriteVector(write, v.Position);
						WriteVector(write, v.Normal);
						write->WriteDouble(32, v.TexCoord.U);
						write->WriteDouble(32, v.TexCoord.V);
						for (size_t b = 0; b < 4; b++)
						{
							write->WriteBits(32, v.Bones[b].BoneID);
							write->WriteDouble(32, v.Bones[b].Weight);
						}
					}
					write->WriteBits(32, bys);
					write->WriteBits(3, bis);
					write->WriteBuffer(data, bys, bis);
					SAVE_RELEASE(write);
				}
			}
			inline void CLOAK_CALL WriteSingleVertex(In CloakEngine::Files::IWriter* write, In const FinalVertex& v, In bool saveBones)
			{
				WriteVector(write, v.Position);
				WriteVector(write, v.Normal);
				WriteVector(write, v.Binormal);
				WriteVector(write, v.Tangent);
				write->WriteDouble(32, v.TexCoord.U);
				write->WriteDouble(32, v.TexCoord.V);
				if (saveBones)
				{
					size_t bc = 0;
					for (size_t a = 0; a < 4; a++)
					{
						if (v.Bones[a].Weight > 0)
						{
							bc++;
						}
					}
					write->WriteBits(2, bc);
					for (size_t a = 0; a < 4; a++)
					{
						if (v.Bones[a].Weight > 0)
						{
							write->WriteBits(32, v.Bones[a].BoneID);
							write->WriteDouble(32, v.Bones[a].Weight);
						}
					}
				}
			}
#ifdef _DEBUG
			inline void CLOAK_CALL __DBG_PrintVertex(In const FinalVertex& v)
			{
				CloakEngine::Global::Math::Point h(v.Position);
				CloakDebugLog("\tPosition = [" + std::to_string(h.X) + "|" + std::to_string(h.Y) + "|" + std::to_string(h.Z) + "]");
				h = static_cast<CloakEngine::Global::Math::Point>(v.Normal);
				CloakDebugLog("\tNormal = [" + std::to_string(h.X) + "|" + std::to_string(h.Y) + "|" + std::to_string(h.Z) + "]");
				h = static_cast<CloakEngine::Global::Math::Point>(v.Binormal);
				CloakDebugLog("\tBinormal = [" + std::to_string(h.X) + "|" + std::to_string(h.Y) + "|" + std::to_string(h.Z) + "]");
				h = static_cast<CloakEngine::Global::Math::Point>(v.Tangent);
				CloakDebugLog("\tTangent = [" + std::to_string(h.X) + "|" + std::to_string(h.Y) + "|" + std::to_string(h.Z) + "]");
				CloakDebugLog("\tTexCoord = [" + std::to_string(v.TexCoord.U) + "|" + std::to_string(v.TexCoord.V) + "]");
			}
#define PrintVertex(v) __DBG_PrintVertex(v)
#else
#define PrintVertex(v)
#endif
			inline void CLOAK_CALL WriteIndexVertexBuffer(In CloakEngine::Files::IWriter* write, In size_t size, In size_t boneCount, In_reads(size) const FinalVertex* vertexBuffer, In_reads(size) const CE::List<bool>& used)
			{
#ifdef _DEBUG
				size_t vertI = 0;
#endif
				for (size_t a = 0; a < size; a++)
				{
					if (used[a] == true)
					{
#ifdef _DEBUG
						CloakDebugLog("Write vertex " + std::to_string(a) + " at index " + std::to_string(vertI));
						PrintVertex(vertexBuffer[a]);
						vertI++;
#endif

						WriteSingleVertex(write, vertexBuffer[a], boneCount > 0);
					}
				}
			}
			inline void CLOAK_CALL WriteBoneUsage(In CloakEngine::Files::IWriter* write, In size_t begin, In size_t end, In size_t boneCount, In_reads(end) const FinalVertex* vertexBuffer)
			{
				if (boneCount > 0)
				{
					bool* usage = new bool[boneCount];
					for (size_t a = 0; a < boneCount; a++) { usage[a] = false; }
					for (size_t a = begin; a < end; a++)
					{
						const FinalVertex& v = vertexBuffer[a];
						for (size_t b = 0; b < 4; b++)
						{
							if (v.Bones[b].Weight > 0)
							{
								usage[v.Bones[b].BoneID] = true;
							}
						}
					}
					for (size_t a = 0; a < boneCount; a++) { write->WriteBits(1, usage[a] ? 1 : 0); }
					delete[] usage;
				}
			}
			inline void CLOAK_CALL WriteIndexBuffer(In CloakEngine::Files::IWriter* write, In size_t ibsl, In bool shortStripCut, In size_t boneCount, In_reads(size) const FinalVertex* vertexBuffer, In_reads(size) const CE::List<size_t>& indexBuffer, In const CloakEngine::List<MaterialRange>& matRanges)
			{
				for (size_t a = 0, b = 0; a < indexBuffer.size() && b < matRanges.size(); b++)
				{
					const MaterialRange& mr = matRanges[b];
					const size_t cS = min(mr.StripCount, indexBuffer.size() - a);
					const size_t cL = min(mr.ListCount, indexBuffer.size() - (a + cS));
					CloakDebugLog("Write material " + std::to_string(mr.MaterialID) + " (" + std::to_string(cS) + " triangle strip vertices, " + std::to_string(cL) + " triangle list vertices)");
					write->WriteBits(32, cS);
					write->WriteBits(32, cL);
					write->WriteBits(32, mr.MaterialID);
					WriteBoneUsage(write, a, a + cS + cL, boneCount, vertexBuffer);
					for (size_t c = 0; c < cS; a++, c++)
					{
						CLOAK_ASSUME(a < indexBuffer.size());
						CloakDebugLog("Write Index[" + std::to_string(a) + "]: " + (indexBuffer[a] == STRIP_CUT_VALUE ? "Strip Cut" : std::to_string(indexBuffer[a])));
						if (shortStripCut == true && indexBuffer[a] == STRIP_CUT_VALUE) { write->WriteBits(ibsl, STRIP_CUT_VALUE_16); }
						else { write->WriteBits(ibsl, indexBuffer[a]); }
					}
					for (size_t c = 0; c < cL; a++, c++)
					{
						CLOAK_ASSUME(a < indexBuffer.size());
						CloakDebugLog("Write Index[" + std::to_string(a) + "]: " + (indexBuffer[a] == STRIP_CUT_VALUE ? "Strip Cut" : std::to_string(indexBuffer[a])));
						write->WriteBits(ibsl, indexBuffer[a]);
					}
				}
			}
			inline void CLOAK_CALL WriteRawVertexBuffer(In CloakEngine::Files::IWriter* write, In size_t size, In size_t boneCount, In_reads(size) const FinalVertex* vertexBuffer)
			{
				size_t s = 0;
				size_t lMat = 0;
				for (size_t a = 0; a < size; a++, s++)
				{
					if (a == 0) { lMat = vertexBuffer[a].MaterialID; }
					else if (lMat != vertexBuffer[a].MaterialID)
					{
						CloakDebugLog("Write material " + std::to_string(lMat) + " (" + std::to_string(s) + " vertices)");

						write->WriteBits(32, s);
						write->WriteBits(32, lMat);
						WriteBoneUsage(write, a - s, a, boneCount, vertexBuffer);
						for (size_t b = a - s; b < a; b++)
						{
							CloakDebugLog("Write vertex " + std::to_string(b));
							PrintVertex(vertexBuffer[b]);

							const FinalVertex& v = vertexBuffer[b];
							WriteSingleVertex(write, v, boneCount > 0);
						}
						lMat = vertexBuffer[a].MaterialID;
						s = 0;
					}
				}
				CloakDebugLog("Write material " + std::to_string(lMat) + " (" + std::to_string(s) + " vertices)");

				write->WriteBits(32, s);
				write->WriteBits(32, lMat);
				WriteBoneUsage(write, size - s, size, boneCount, vertexBuffer);
				for (size_t b = size - s; b < size; b++)
				{
					CloakDebugLog("Write vertex " + std::to_string(b));
					PrintVertex(vertexBuffer[b]);

					const FinalVertex& v = vertexBuffer[b];
					WriteSingleVertex(write, v, boneCount > 0);
				}
			}


			CLOAKCOMPILER_API Vector::Vector()
			{
				X = Y = Z = W = 0;
			}
			CLOAKCOMPILER_API Vector::Vector(In const CloakEngine::Global::Math::Vector& v)
			{
				CloakEngine::Global::Math::Point p(v);
				X = p.X;
				Y = p.Y;
				Z = p.Z;
				W = p.W;
			}
			CLOAKCOMPILER_API Vector& Vector::operator=(In const Vector& p)
			{
				X = p.X;
				Y = p.Y;
				Z = p.Z;
				W = p.W;
				return *this;
			}
			CLOAKCOMPILER_API Vector& Vector::operator=(In const CloakEngine::Global::Math::Vector& v)
			{
				CloakEngine::Global::Math::Point p(v);
				X = p.X;
				Y = p.Y;
				Z = p.Z;
				W = p.W;
				return *this;
			}
			CLOAKCOMPILER_API Vector::operator CloakEngine::Global::Math::Vector()
			{
				return CloakEngine::Global::Math::Vector(X, Y, Z, W);
			}
			CLOAKCOMPILER_API Vector::operator const CloakEngine::Global::Math::Vector() const
			{
				return CloakEngine::Global::Math::Vector(X, Y, Z, W);
			}
			CLOAKCOMPILER_API void CLOAK_CALL EncodeToFile(In CloakEngine::Files::IWriter* output, In const EncodeDesc& encode, In const Desc& desc, In std::function<void(const RespondeInfo&)> response)
			{
				bool suc = true;
				if (CheckTemp(output, encode, desc, response))
				{
					CloakEngine::Files::IVirtualWriteBuffer* wrBuf = CloakEngine::Files::CreateVirtualWriteBuffer();
					CloakEngine::Files::IWriter* write = nullptr;
					CREATE_INTERFACE(CE_QUERY_ARGS(&write));
					write->SetTarget(wrBuf);

					const size_t vbs = desc.Polygons.size() * 3;
					FinalVertex* vb = NewArray(FinalVertex, vbs);

					size_t matCount = 0;
					const size_t polS = desc.Polygons.size();
					//Check polygon-vertex aviability
					for (size_t a = 0; a < desc.Polygons.size() && suc == true; a++)
					{
						const Polygon& p = desc.Polygons[a];
						for (size_t b = 0; b < 3 && suc == true; b++)
						{
							if (p.Point[b] >= desc.Vertices.size()) 
							{
								SendResponse(response, RespondeCode::ERROR_VERTEXREF, a, p.Point[b], "Polygon " + std::to_string(a) + " refers to vertex " + std::to_string(p.Point[b]) + " but there are only " + std::to_string(desc.Vertices.size()) + " vertices!");
								suc = false;
							}
						}
					}
					if (suc == true)
					{
						//Write required minimum of bones in animation skeleton
						SendResponse(response, RespondeCode::CALC_BONES);
						size_t maxBone = 0;
						for (size_t a = 0; a < desc.Vertices.size(); a++)
						{
							const Vertex& v = desc.Vertices[a];
							for (size_t b = 0; b < 4; b++)
							{
								if (v.Bones[b].Weight > 0)
								{
									maxBone = max(maxBone, v.Bones[b].BoneID);
								}
							}
						}
						const size_t boneCount = maxBone > 0 ? maxBone + 1 : 0;
						write->WriteBits(32, boneCount);

						//Sort polygons by material
						SendResponse(response, RespondeCode::CALC_SORT);
						size_t* const sortHeap = NewArray(size_t, polS * 2);
						size_t* sorted[2] = { &sortHeap[0],&sortHeap[polS] };
						size_t count[16];
						for (size_t a = 0; a < polS; a++) { sorted[0][a] = a; }
						for (uint32_t a = 0xf, b = 0; b < 8; a <<= 4, b++)
						{
							for (size_t c = 0; c < 16; c++) { count[c] = 0; }
							for (size_t c = 0; c < polS; c++) { count[(desc.Polygons[sorted[0][c]].Material & a) >> (4 * b)]++; }
							for (size_t c = 1; c < 16; c++) { count[c] += count[c - 1]; }
							for (size_t c = polS; c > 0; c--)
							{
								const size_t p = (desc.Polygons[sorted[0][c - 1]].Material & a) >> (4 * b);
								sorted[1][count[p] - 1] = sorted[0][c - 1];
								count[p]--;
							}
							size_t* t = sorted[0];
							sorted[0] = sorted[1];
							sorted[1] = t;
						}

						//Remap material ids to zero-based array indices
						SendResponse(response, RespondeCode::CALC_MATERIALS);
						uint32_t lastMat;
						for (size_t a = 0, b = 0; a < polS; a++)
						{
							const Polygon& p = desc.Polygons[sorted[0][a]];
							if (a == 0) { lastMat = p.Material; }
							else if (lastMat != p.Material)
							{
								lastMat = p.Material;
								b++;
								matCount = max(matCount, b + 1);
							}
						}
						matCount = max(1, matCount);

						//Calculate binormals/tangents & copy vertex data
						SendResponse(response, RespondeCode::CALC_BINORMALS);
						for (size_t a = 0; a < polS; a++)
						{
							const Polygon& p = desc.Polygons[sorted[0][a]];
							CalculatePolygonVertices(desc.Vertices[p.Point[0]], desc.Vertices[p.Point[1]], desc.Vertices[p.Point[2]], p.AutoGenerateNormal, p.Material, &vb[3 * a]);
						}
						if (suc == true)
						{
							if (boneCount == 0)
							{
								//Calculate bounding volume for full mesh
								if (desc.Bounding != BoundingVolume::None) { SendResponse(response, RespondeCode::CALC_BOUNDING); }
								const size_t posSize = desc.Vertices.size();
								uint8_t* poslHeap = new uint8_t[(sizeof(CloakEngine::Global::Math::Vector)*posSize) + 15];
								CloakEngine::Global::Math::Vector* posl = reinterpret_cast<CloakEngine::Global::Math::Vector*>((reinterpret_cast<uintptr_t>(poslHeap) + 15) & ~static_cast<uintptr_t>(0xF));
								for (size_t a = 0; a < posSize; a++) { posl[a] = desc.Vertices[a].Position; }
								switch (desc.Bounding)
								{
									case BoundingVolume::None:
									{
										write->WriteBits(2, 0);
										break;
									}
									case BoundingVolume::OOBB:
									{
										Engine::BoundingVolume::BoundingOOBB bound = Engine::BoundingVolume::CalculateBoundingOOBB(posl, posSize);
										if (bound.Enabled)
										{
											SendResponse(response, RespondeCode::WRITE_BOUNDING);
											write->WriteBits(2, 1);
											WriteVector(write, bound.Center);
											for (size_t a = 0; a < 3; a++) { WriteVector(write, bound.Axis[a]); }
											for (size_t a = 0; a < 3; a++) { write->WriteDouble(32, bound.HalfSize[a]); }


											CloakEngine::Global::Math::Point cen(bound.Center);
											CloakDebugLog("Bounding Box center: " + std::to_string(cen.X) + " | " + std::to_string(cen.Y) + " | " + std::to_string(cen.Z));
											for (size_t a = 0; a < 3; a++)
											{
												CloakEngine::Global::Math::Point p(bound.Axis[a]);
												CloakDebugLog("Bounding Box axis[" + std::to_string(a) + "]: " + std::to_string(p.X) + " | " + std::to_string(p.Y) + " | " + std::to_string(p.Z));
											}
											for (size_t a = 0; a < 3; a++)
											{
												CloakDebugLog("Bounding Box axis[" + std::to_string(a) + "] scale: " + std::to_string(bound.HalfSize[a]));
											}

											break;
										}
										else
										{
											SendResponse(response, RespondeCode::ERROR_BOUNDING, "Failed to calculate OOBB, switch to bounding-Volume: sphere");
										}
										//Fall through
									}
									case BoundingVolume::Sphere:
									{
										Engine::BoundingVolume::BoundingSphere bound = Engine::BoundingVolume::CalculateBoundingSphere(posl, posSize);
										if (bound.Enabled)
										{
											SendResponse(response, RespondeCode::WRITE_BOUNDING);
											write->WriteBits(2, 2);
											WriteVector(write, bound.Center);
											write->WriteDouble(32, bound.Radius);

											CloakEngine::Global::Math::Point cen(bound.Center);
											CloakDebugLog("Bounding Sphere center: " + std::to_string(cen.X) + " | " + std::to_string(cen.Y) + " | " + std::to_string(cen.Z));
											CloakDebugLog("Bounding Sphere radius: " + std::to_string(bound.Radius));

											break;
										}
										else
										{
											SendResponse(response, RespondeCode::ERROR_BOUNDING, "Failed to calculate bounding sphere!");
											suc = false;
										}
										break;
									}
									default:
										suc = false;
										SendResponse(response, RespondeCode::ERROR_BOUNDING, "Unknown bounding volume type");
										break;
								}
								delete[] poslHeap;
							}
							else
							{
								std::vector<std::vector<size_t>> boneToVertex(boneCount);
								for (size_t a = 0; a < vbs; a++)
								{
									const FinalVertex& v = vb[a];
									for (size_t b = 0; b < 4; b++)
									{
										if (v.Bones[b].Weight > 0)
										{
											boneToVertex[v.Bones[b].BoneID].push_back(a);
										}
									}
								}
								for (size_t a = 0; a < boneCount; a++)
								{
									if (boneToVertex[a].size() == 0) { continue; }
									//TODO: per-Bone bounding volume
									//TODO: Find solution to interpolated vertices (vertices between bones -> weight of one bone < 1)
									SendResponse(response, RespondeCode::ERROR_BOUNDING, "Bounding volume calculation for animated objects is not yet implemented!");
									suc = false;
								}
							}
						}
						if (suc == true)
						{
							bool useIndexBuffer = false;
							if (desc.UseIndexBuffer)
							{
								//Calculate index buffer
								SendResponse(response, RespondeCode::CALC_INDICES);
								CE::List<bool> vbUsed(vbs);
								CE::List<size_t> ib;
								CloakEngine::List<MaterialRange> matRanges;
								const IndexBufferType stripCut = CalculateIndexBuffer(vb, vbs, &ib, &vbUsed, &matRanges);
								size_t finVbs = 0;
								for (size_t a = 0; a < vbs; a++)
								{
									if (vbUsed[a] == true) { finVbs++; }
								}
								const size_t ibsL = stripCut == IndexBufferType::IB32 ? 4 : 2;
								CloakDebugLog("IB Comparison: IB = " + std::to_string((finVbs*VertexSize) + (ib.size()*ibsL)) + " (" + std::to_string(finVbs) + " Vertices) Raw = " + std::to_string(vbs*VertexSize) + " (" + std::to_string(vbs) + " Vertices)");
								//Test whether we use an index buffer at all:
								if ((vbs - finVbs)*VertexSize > (ib.size()*ibsL))
								{
									useIndexBuffer = true;
									//Write all vertices + sorted per material indices
									SendResponse(response, RespondeCode::WRITE_VERTICES);
									write->WriteBits(16, matRanges.size() - 1);
									write->WriteBits(32, finVbs);
									write->WriteBits(1, 1); //Use index buffer

									write->WriteBits(1, ibsL == 2 ? 0 : 1); //16 or 32 bit index buffer size
									if (ibsL == 2) { write->WriteBits(1, stripCut == IndexBufferType::IB16ShortCut ? 1 : 0); } //short strip cuts?
									write->WriteBits(32, ib.size());

									CloakDebugLog("Write Index + Vertex Buffer");
									WriteIndexVertexBuffer(write, vbs, boneCount, vb, vbUsed);
									WriteIndexBuffer(write, ibsL << 3, stripCut == IndexBufferType::IB16ShortCut, boneCount, vb, ib, matRanges);
								}
							}

							if (useIndexBuffer == false)
							{
								//Write vertices, sorted and seperated by materials
								SendResponse(response, RespondeCode::WRITE_VERTICES);
								write->WriteBits(16, matCount - 1);
								write->WriteBits(32, vbs);
								write->WriteBits(1, 0);

								CloakDebugLog("Write raw vertex buffer");
								WriteRawVertexBuffer(write, vbs, boneCount, vb);
							}
						}
						DeleteArray(sortHeap);
					}
					DeleteArray(vb);
					const uint32_t bys = static_cast<uint32_t>(write->GetPosition());
					const uint8_t bis = static_cast<uint8_t>(write->GetBitPosition());
					write->Save();
					if (suc)
					{
						output->WriteBuffer(wrBuf, bys, bis);
						WriteTemp(wrBuf, bys, bis, encode, desc, response);
					}
					SAVE_RELEASE(write);
					SAVE_RELEASE(wrBuf);
				}
				SendResponse(response, suc ? RespondeCode::FINISH_SUCCESS : RespondeCode::FINISH_ERROR);
			}
		}
	}
}