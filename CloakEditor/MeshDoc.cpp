#include "stdafx.h"
#include "Document/MeshDoc.h"

namespace Editor {
	namespace Document {
		namespace Mesh {
			IMPLEMENT_DYNCREATE(MeshView, Window::EngineView);
			BEGIN_MESSAGE_MAP(MeshView, Window::EngineView)
			END_MESSAGE_MAP()

			IMPLEMENT_DYNCREATE(MeshDocument, CDocument);
			BEGIN_MESSAGE_MAP(MeshDocument, CDocument)
			END_MESSAGE_MAP()


		}
	}
}