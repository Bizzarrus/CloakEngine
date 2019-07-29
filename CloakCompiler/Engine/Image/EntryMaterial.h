#pragma once
#ifndef CC_E_IMAGE_ENTRYMATERIAL_H
#define CC_E_IMAGE_ENTRYMATERIAL_H

#include "Engine/Image/Entry.h"

namespace CloakCompiler {
	namespace Engine {
		namespace Image {
			inline namespace v1008 {
				inline namespace EntryMaterial_v1 {
					struct FillPlan {
						size_t img = 0;
						CloakEngine::Helper::Color::RGBA Constant;
					};
					class EntryMaterial : public IEntry {
					private:
						API::Image::v1008::Heap m_heap;
						uint8_t m_qualityOffset;
						API::Image::v1008::MaterialType m_type;
						bool m_err;
						bool m_useTemp;
						MipTexture m_textures[4];
						TextureMeta m_metas[11];
						TextureMeta m_comMetas[4];
						API::Image::v1008::NORMAL_FLAGS m_normFlags;
						FillPlan m_fillPlan[11];
						bool m_useDisplacement;
						size_t m_layerCount;
						//layer[0] is header, all other layers are mip maps, where m_layers[1] is the biggest and m_layers[m_layerCount-1] is the smallest
						LayerBuffer* m_layers;
					public:
						CLOAK_CALL EntryMaterial(In const API::Image::v1008::IMAGE_INFO_MATERIAL& img, In const ResponseCaller& response, In size_t imgNum, In uint8_t qualityOffset, In API::Image::v1008::Heap heap);
						virtual CLOAK_CALL ~EntryMaterial();
						bool CLOAK_CALL_THIS GotError() const override;
						void CLOAK_CALL_THIS CheckTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum) override;
						void CLOAK_CALL_THIS WriteTemp(In const API::EncodeDesc& encode, In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum) override;
						void CLOAK_CALL_THIS Encode(In const BitInfoList& bitInfos, In const ResponseCaller& response, In size_t imgNum) override;
						void CLOAK_CALL_THIS WriteHeader(In const CE::RefPointer<CloakEngine::Files::IWriter>& write) override;
						size_t CLOAK_CALL_THIS GetMaxLayerLength() override;
						void CLOAK_CALL_THIS WriteLayer(In const CE::RefPointer<CloakEngine::Files::IWriter>& write, In size_t layer) override;
						void CLOAK_CALL_THIS WriteLibFiles(In const CE::RefPointer<CloakEngine::Files::IWriter>& file, In const API::Image::LibInfo& lib, In const BitInfo& bits, In size_t entryID, In const Engine::Lib::CMakeFile& cmake, In const CE::Global::Task& finishTask) override;
					};
				}
			}
		}
	}
}

#endif