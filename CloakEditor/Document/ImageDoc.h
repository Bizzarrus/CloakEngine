#pragma once
#ifndef CE_EDIT_DOCUMENT_IMAGEDOC_H
#define CE_EDIT_DOCUMENT_IMAGEDOC_H

#include "resource.h"
#include "Window/EngineWindow.h"

namespace Editor {
	namespace Document {
		namespace Image {
			constexpr UINT TypeID = IDR_DOC_IMAGE;
			class ImageView : public Window::EngineView {
				DECLARE_DYNCREATE(ImageView);
				public:

				protected:
					DECLARE_MESSAGE_MAP()
			};
			class ImageDocument : public CDocument {
				DECLARE_DYNCREATE(ImageDocument);
				public:

				protected:
					DECLARE_MESSAGE_MAP()
			};
		}
	}
}

#endif