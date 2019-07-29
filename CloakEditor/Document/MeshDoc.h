#pragma once
#ifndef CE_EDIT_DOCUMENT_MESHDOC_H
#define CE_EDIT_DOCUMENT_MESHDOC_H

#include "resource.h"
#include "Window/EngineWindow.h"

namespace Editor {
	namespace Document {
		namespace Mesh {
			constexpr UINT TypeID = IDR_DOC_MESH;
			class MeshView : public Window::EngineView {
				DECLARE_DYNCREATE(MeshView);
				public:

				protected:
					DECLARE_MESSAGE_MAP()
			};
			class MeshDocument : public CDocument {
				DECLARE_DYNCREATE(MeshDocument);
				public:

				protected:
					DECLARE_MESSAGE_MAP()
			};
		}
	}
}

#endif