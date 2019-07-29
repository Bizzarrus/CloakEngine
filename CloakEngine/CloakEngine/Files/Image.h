#pragma once
#ifndef CE_API_FILES_IMAGE_H
#define CE_API_FILES_IMAGE_H

#include "CloakEngine/Defines.h"
#include "CloakEngine/Files/FileHandler.h"
#include "CloakEngine/Rendering/ColorBuffer.h"
#include "CloakEngine/Rendering/StructuredBuffer.h"
#include "CloakEngine/Files/Animation.h"

namespace CloakEngine {
	CLOAKENGINE_API_NAMESPACE namespace API {
		namespace Files {
			inline namespace Image_v2 {
				enum class MaterialType { 
					Unknown = 0x0, 
					Opaque = 0x1, 
					AlphaMapped = 0x2, 
					Transparent = 0x4,
				};
				DEFINE_ENUM_FLAG_OPERATORS(MaterialType);
				CLOAK_INTERFACE_ID("{C1A62A1B-9D97-4E67-ABF3-D4934A3A3342}") ITexture : public virtual Files::FileHandler_v1::IFileHandler, public virtual Files::FileHandler_v1::ILODFile{
					public:
						virtual size_t CLOAK_CALL_THIS GetBufferCount() const = 0;
						virtual Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer(In_opt size_t buffer = 0) const = 0;
						virtual bool CLOAK_CALL_THIS IsTransparentAt(In float X, In float Y, In_opt size_t buffer = 0) const = 0;
						virtual bool CLOAK_CALL_THIS IsBorderedImage(In_opt size_t buffer = 0) const = 0;
				};
				CLOAK_INTERFACE_ID("{91A29FAE-2876-4FCD-B61D-F9267A9957F8}") ITextureArray : public virtual Files::FileHandler_v1::IFileHandler, public virtual Files::FileHandler_v1::ILODFile{
					public:
						virtual Rendering::IColorBuffer* CLOAK_CALL_THIS GetBuffer() const = 0;
						virtual size_t CLOAK_CALL_THIS GetSize() const = 0;
				};
				CLOAK_INTERFACE_ID("{DC1CF14F-DE15-4C6A-BCD7-344D4ECB2991}") ICubeMap : public virtual ITextureArray {
					public:
				};
				CLOAK_INTERFACE_ID("{98B18DBE-2B74-45C5-8CF7-FE7FF589559B}") ISkybox : public virtual ICubeMap{
					public:
						virtual Rendering::IColorBuffer* CLOAK_CALL GetLUT() const = 0;
						virtual Rendering::IColorBuffer* CLOAK_CALL GetRadianceMap() const = 0;
						virtual Rendering::IConstantBuffer* CLOAK_CALL GetIrradiance() const = 0;
				};
				CLOAK_INTERFACE_ID("{B33DC435-9896-478E-88AC-E93085B5864D}") ITextureAnmiation : public virtual ITexture, public virtual Animation_v1::IAnimation{
					public:
				};
				CLOAK_INTERFACE_ID("{16547BB5-7EF5-4359-B060-958390753999}") IMaterial : public virtual Files::FileHandler_v1::IFileHandler, public virtual Files::FileHandler_v1::ILODFile{
					public:
						virtual MaterialType CLOAK_CALL_THIS GetType() const = 0;
						virtual bool CLOAK_CALL_THIS SupportTesslation() const = 0;
						virtual Rendering::IColorBuffer* CLOAK_CALL_THIS GetAlbedo() const = 0;
						virtual Rendering::IColorBuffer* CLOAK_CALL_THIS GetNormalMap() const = 0;
						virtual Rendering::IColorBuffer* CLOAK_CALL_THIS GetSpecularMap() const = 0;
						virtual Rendering::IColorBuffer* CLOAK_CALL_THIS GetDisplacementMap() const = 0;
						virtual Rendering::ResourceView CLOAK_CALL_THIS GetSRVMaterial() const = 0;
						virtual Rendering::ResourceView CLOAK_CALL_THIS GetSRVDisplacement() const = 0;
				};
				CLOAK_INTERFACE_ID("{3E7B9777-6FE1-40A7-A35C-9805209CAD41}") IImage : public virtual Files::FileHandler_v1::IFileHandler {
					public:
						virtual API::RefPointer<ITexture> CLOAK_CALL_THIS GetTexture(In size_t id, In_opt size_t buffer = 0) = 0;
						virtual API::RefPointer<ITextureAnmiation> CLOAK_CALL_THIS GetAnimation(In size_t id) = 0;
						virtual API::RefPointer<ITextureArray> CLOAK_CALL_THIS GetTextureArray(In size_t id) = 0;
						virtual API::RefPointer<ICubeMap> CLOAK_CALL_THIS GetCubeMap(In size_t id) = 0;
						virtual API::RefPointer<IMaterial> CLOAK_CALL_THIS GetMaterial(In size_t id) = 0;
						virtual API::RefPointer<ISkybox> CLOAK_CALL_THIS GetSkybox(In size_t id) = 0;
				};
			}
		}
	}
}

#endif