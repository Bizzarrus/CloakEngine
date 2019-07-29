#include "stdafx.h"
#include "CloakCompiler/Converter.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Include/tiny_obj_loader.h"

namespace CloakCompiler {
	namespace API {
		namespace Converter {
			struct AVD {
				bool reqNormRecalc = false;
			};
			void CLOAK_CALL InitMaterialInfo(Inout Image::BuildInfo* info, In const std::u16string& file, In unsigned int blockSize, In Image::ColorSpace space)
			{
				info->Meta.Type = Image::BuildType::NORMAL;
				info->Normal.Path = file;
				info->Meta.Space = space;
				info->Meta.BlockSize = blockSize;
			}
			void CLOAK_CALL InitMaterialInfo(Inout Image::OptionalInfo* info, In const std::u16string& file, In unsigned int blockSize, In Image::ColorSpace space)
			{
				info->Use = file.length() > 0;
				InitMaterialInfo(static_cast<Image::BuildInfo*>(info), file, blockSize, space);
			}
			CLOAKCOMPILER_API bool CLOAK_CALL ConvertOBJFile(In const std::u16string& filePath, In Image::SupportedQuality quality, In unsigned int blockSize, Out Mesh::Desc* mesh, Out std::vector<Image::ImageInfo>* materials, Out std::string* error, In_opt std::u16string matPath)
			{
				if (materials == nullptr || error == nullptr) 
				{
					if (error != nullptr) { *error = "Invalid Parameter!"; }
					return false; 
				}
				*error = "";
				std::vector<tinyobj::shape_t> shapes;
				std::vector<tinyobj::material_t> mats;
				tinyobj::attrib_t attributes;
				std::string file = CloakEngine::Helper::StringConvert::ConvertToU8(filePath);
				std::string matP = CloakEngine::Helper::StringConvert::ConvertToU8(matPath) + "/";
				const char* matPC = nullptr;
				if (matP.length() > 0) { matPC = matP.c_str(); }
				bool res = tinyobj::LoadObj(&attributes, &shapes, &mats, error, file.c_str(), matPC);
				CloakDebugLog("Converter reading " + std::to_string(mats.size()) + " materials!");
				if (res == false) 
				{ 
					//error is filled by tinyobj
					return false; 
				}

				//Reading Mesh data:
				if (mesh != nullptr)
				{
					for (size_t a = 0; a < shapes.size(); a++)
					{
						const tinyobj::shape_t& shape = shapes[a];
						std::vector<AVD> addVertData;
						for (size_t b = 0, c = 0; b < shape.mesh.num_face_vertices.size(); b++)
						{
							const unsigned char num_vertices = shape.mesh.num_face_vertices[b];
							if (num_vertices < 3)
							{
								*error += "Faces with less than 3 vertices are not allowed!";
								return false;
							}
							std::vector<size_t> vertexIDs;
							vertexIDs.reserve(num_vertices);
							for (size_t d = 0; d < num_vertices; d++)
							{
								tinyobj::index_t index = shape.mesh.indices[c + d];
								Mesh::Vertex v;
								AVD avd;
								const size_t vid = index.vertex_index < 0 ? 0 : (3 * index.vertex_index);
								const size_t nid = index.normal_index < 0 ? 0 : (3 * index.normal_index);
								const size_t tid = index.texcoord_index < 0 ? 0 : (2 * index.texcoord_index);
								if (attributes.vertices.size() >= vid + 3)
								{
									v.Position = CloakEngine::Global::Math::Vector(attributes.vertices[vid + 0], attributes.vertices[vid + 1], attributes.vertices[vid + 2]);
								}
								else
								{
									v.Position = CloakEngine::Global::Math::Vector(0, 0, 0);
								}
								if (attributes.normals.size() >= nid + 3)
								{
									v.Normal = CloakEngine::Global::Math::Vector(attributes.normals[nid + 0], attributes.normals[nid + 1], attributes.normals[nid + 2]);
								}
								else
								{
									v.Normal = CloakEngine::Global::Math::Vector(0, 0, 0);
									avd.reqNormRecalc = true;
								}
								if (attributes.texcoords.size() >= tid + 2)
								{
									v.TexCoord.U = attributes.texcoords[tid + 0];
									v.TexCoord.V = attributes.texcoords[tid + 1];
								}
								else
								{
									v.TexCoord.U = 0;
									v.TexCoord.V = 0;
								}
								for (size_t e = 0; e < 4; e++) { v.Bones[e].BoneID = 0; v.Bones[e].Weight = 0; }
								vertexIDs.push_back(mesh->Vertices.size());
								mesh->Vertices.push_back(v);
								addVertData.push_back(avd);
							}
							for (size_t d = 0; d + 2 < vertexIDs.size(); d++)
							{
								Mesh::Polygon p;
								p.Point[0] = static_cast<uint32_t>(vertexIDs[0]);
								p.Point[1] = static_cast<uint32_t>(vertexIDs[d + 1]);
								p.Point[2] = static_cast<uint32_t>(vertexIDs[d + 2]);
								p.Material = static_cast<uint32_t>(shape.mesh.material_ids[b] < 0 ? 0 : shape.mesh.material_ids[b]);
								p.AutoGenerateNormal = addVertData[p.Point[0]].reqNormRecalc || addVertData[p.Point[1]].reqNormRecalc || addVertData[p.Point[2]].reqNormRecalc;
								//Check orientation (counter-clockwise)
								if (p.AutoGenerateNormal == false)
								{
									const API::Mesh::Vertex& v0 = mesh->Vertices[p.Point[0]];
									const API::Mesh::Vertex& v1 = mesh->Vertices[p.Point[1]];
									const API::Mesh::Vertex& v2 = mesh->Vertices[p.Point[2]];
									CloakEngine::Global::Math::Vector vec[2];
									vec[0] = static_cast<CloakEngine::Global::Math::Vector>(v1.Position) - v0.Position;
									vec[1] = static_cast<CloakEngine::Global::Math::Vector>(v2.Position) - v0.Position;
									CloakEngine::Global::Math::Vector norm = vec[0].Cross(vec[1]);
									if (norm.Dot(v0.Normal) < 0 && norm.Dot(v1.Normal) < 0 && norm.Dot(v2.Normal) < 0)
									{
										const uint32_t t = p.Point[2];
										p.Point[2] = p.Point[1];
										p.Point[1] = t;
									}
								}
								mesh->Polygons.push_back(p);
							}
							c += num_vertices;
							assert(mesh->Vertices.size() == c);
						}
					}
				}
				//Reading Material Data:
				for (size_t a = 0; a < mats.size(); a++)
				{
					const tinyobj::material_t mat = mats[a];
					Image::ImageInfo info;
					info.Heap = Image::Heap::WORLD;
					info.SupportedQuality = quality;
					info.Type = Image::ImageInfoType::MATERIAL;
					if (mat.dissolve > 1 - 1e-3f)
					{
						if (mat.alpha_texname.length() > 0)
						{
							info.Material.Type = Image::MaterialType::AlphaMapped;
							auto& material = info.Material.AlphaMapped;

							InitMaterialInfo(&material.Textures.Albedo, CloakEngine::Helper::StringConvert::ConvertToU16(mat.ambient_texname), blockSize, Image::ColorSpace::RGBA);
							InitMaterialInfo(&material.Textures.NormalMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.normal_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.Metallic, CloakEngine::Helper::StringConvert::ConvertToU16(mat.metallic_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.AmbientOcclusion, u"", blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.RoughnessMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.specular_highlight_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.SpecularMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.specular_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.IndexOfRefractionMap, u"", blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.ExtinctionCoefficientMap, u"", blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.DisplacementMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.displacement_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.AlphaMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.alpha_texname), blockSize, Image::ColorSpace::RGBA);
							
							material.Textures.PorosityMap.Use = false;

							material.Constants.Albedo = CloakEngine::Helper::Color::RGBX(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
							CloakDebugLog("Read constant IOR = " + std::to_string(mat.ior));
							for (size_t a = 0; a < ARRAYSIZE(material.Constants.Fresnel); a++)
							{
								material.Constants.Fresnel[a].RefractionIndex = mat.ior;
								material.Constants.Fresnel[a].ExtinctionCoefficient = 0;
							}
							material.Constants.Roughness = mat.shininess;
							material.Constants.Metallic = mat.metallic;
							material.Constants.Porosity = 0.35f;
						}
						else
						{
							info.Material.Type = Image::MaterialType::Opaque;
							auto& material = info.Material.Opaque;
							InitMaterialInfo(&material.Textures.Albedo, CloakEngine::Helper::StringConvert::ConvertToU16(mat.ambient_texname), blockSize, Image::ColorSpace::RGBA);
							InitMaterialInfo(&material.Textures.NormalMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.normal_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.Metallic, CloakEngine::Helper::StringConvert::ConvertToU16(mat.metallic_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.AmbientOcclusion, u"", blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.RoughnessMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.specular_highlight_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.SpecularMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.specular_texname), blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.IndexOfRefractionMap, u"", blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.ExtinctionCoefficientMap, u"", blockSize, Image::ColorSpace::Material);
							InitMaterialInfo(&material.Textures.DisplacementMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.displacement_texname), blockSize, Image::ColorSpace::Material);

							material.Textures.PorosityMap.Use = false;

							material.Constants.Albedo = CloakEngine::Helper::Color::RGBX(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
							CloakDebugLog("Read constant IOR = " + std::to_string(mat.ior));
							for (size_t a = 0; a < ARRAYSIZE(material.Constants.Fresnel); a++)
							{
								material.Constants.Fresnel[a].RefractionIndex = mat.ior;
								material.Constants.Fresnel[a].ExtinctionCoefficient = 0;
							}
							material.Constants.Roughness = mat.shininess;
							material.Constants.Metallic = mat.metallic;
							material.Constants.Porosity = 0.35f;
						}
					}
					else
					{
						info.Material.Type = Image::MaterialType::Transparent;
						auto& material = info.Material.Transparent;
						InitMaterialInfo(&material.Textures.Albedo, CloakEngine::Helper::StringConvert::ConvertToU16(mat.ambient_texname), blockSize, Image::ColorSpace::RGBA);
						InitMaterialInfo(&material.Textures.NormalMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.normal_texname), blockSize, Image::ColorSpace::Material);
						InitMaterialInfo(&material.Textures.Metallic, CloakEngine::Helper::StringConvert::ConvertToU16(mat.metallic_texname), blockSize, Image::ColorSpace::Material);
						InitMaterialInfo(&material.Textures.AmbientOcclusion, u"", blockSize, Image::ColorSpace::Material);
						InitMaterialInfo(&material.Textures.RoughnessMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.specular_highlight_texname), blockSize, Image::ColorSpace::Material);
						InitMaterialInfo(&material.Textures.SpecularMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.specular_texname), blockSize, Image::ColorSpace::Material);
						InitMaterialInfo(&material.Textures.IndexOfRefractionMap, u"", blockSize, Image::ColorSpace::Material);
						InitMaterialInfo(&material.Textures.ExtinctionCoefficientMap, u"", blockSize, Image::ColorSpace::Material);
						InitMaterialInfo(&material.Textures.DisplacementMap, CloakEngine::Helper::StringConvert::ConvertToU16(mat.displacement_texname), blockSize, Image::ColorSpace::Material);

						material.Textures.PorosityMap.Use = false;

						material.Constants.Albedo = CloakEngine::Helper::Color::RGBA(mat.ambient[0], mat.ambient[1], mat.ambient[2], mat.dissolve);
						CloakDebugLog("Read constant IOR = " + std::to_string(mat.ior));
						for (size_t a = 0; a < ARRAYSIZE(material.Constants.Fresnel); a++)
						{
							material.Constants.Fresnel[a].RefractionIndex = mat.ior;
							material.Constants.Fresnel[a].ExtinctionCoefficient = 0;
						}
						material.Constants.Roughness = mat.shininess;
						material.Constants.Metallic = mat.metallic;
						material.Constants.Porosity = 0.35f;
					}
					materials->push_back(info);
				}
				return true;
			}
		}
	}
}