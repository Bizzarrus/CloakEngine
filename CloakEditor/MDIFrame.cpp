#include "stdafx.h"
#include "Window/MDIFrame.h"

namespace Editor {
	namespace Window {
		IMPLEMENT_DYNCREATE(MDIFrame, CMDIChildWndEx);
		BEGIN_MESSAGE_MAP(MDIFrame, CMDIChildWndEx)
		END_MESSAGE_MAP()

		BOOL MDIFrame::PreCreateWindow(CREATESTRUCT& cs)
		{
			if (!CMDIChildWndEx::PreCreateWindow(cs)) { return FALSE; }
			return TRUE;
		}
	}
}