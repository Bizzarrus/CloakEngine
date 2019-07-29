#include "stdafx.h"
#include "Document/ImageDoc.h"

namespace Editor {
	namespace Document {
		namespace Image {
			IMPLEMENT_DYNCREATE(ImageView, Window::EngineView);
			BEGIN_MESSAGE_MAP(ImageView, Window::EngineView)
			END_MESSAGE_MAP()

			IMPLEMENT_DYNCREATE(ImageDocument, CDocument);
			BEGIN_MESSAGE_MAP(ImageDocument, CDocument)
			END_MESSAGE_MAP()


		}
	}
}