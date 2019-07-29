#pragma once
#ifndef CE_EDIT_WINDOW_ENGINEWINDOW_H
#define CE_EDIT_WINDOW_ENGINEWINDOW_H

#include "AFXCore.h"

namespace Editor {
	namespace Window {
		class EngineWindow;
		class EngineView : public CView {
			friend class EngineWindow;
			DECLARE_DYNCREATE(EngineView);
			public:
				virtual void OnDraw(CDC* pDC) override;
			protected:
				void CLOAK_CALL_THIS Activate(In EngineWindow* wnd);
				void CLOAK_CALL_THIS Deactivate();
				afx_msg void OnSize(UINT nType, int cx, int cy);
				
				DECLARE_MESSAGE_MAP()

				EngineWindow* m_eWnd = nullptr;
		};
		class EngineWindow : public CWnd {
			friend class EngineView;
			public:
				void CLOAK_CALL_THIS SetParent(In CWnd* par);
				void CLOAK_CALL_THIS SetDocument(In EngineView* view);
			protected:
				EngineView* m_view = nullptr;
				CWnd* m_par = nullptr;
		};
	}
}

#endif