#include "stdafx.h"
#include "Window/EngineWindow.h"

#include "CloakEditor.h"

namespace Editor {
	namespace Window {
		IMPLEMENT_DYNCREATE(EngineView, CView);
		BEGIN_MESSAGE_MAP(EngineView, CView)
			ON_WM_SIZE()
		END_MESSAGE_MAP()

		void EngineView::OnDraw(CDC* pDC) {}

		void CLOAK_CALL_THIS EngineView::Activate(In EngineWindow* wnd)
		{
			m_eWnd = wnd;
		}
		void CLOAK_CALL_THIS EngineView::Deactivate()
		{
			m_eWnd = nullptr;
		}
		afx_msg void EngineView::OnSize(UINT nType, int cx, int cy)
		{
			CView::OnSize(nType, cx, cy);
			if (m_eWnd != nullptr) { m_eWnd->SetDocument(this); }
		}

		void CLOAK_CALL_THIS EngineWindow::SetParent(In CWnd* par)
		{
			m_par = par;
			if (m_view != nullptr) { SetDocument(m_view); }
		}
		void CLOAK_CALL_THIS EngineWindow::SetDocument(In EngineView* view)
		{
			if (m_view != nullptr && m_view != view) { m_view->Deactivate(); }
			m_view = view;
			if (m_par == nullptr) { return; }
			if (m_view != nullptr)
			{
				RECT rct;
				m_view->GetWindowRect(&rct);
				m_par->ScreenToClient(&rct);
				MoveWindow(&rct, FALSE);
				m_view->Activate(this);
				CloakEngine::Global::Graphic::ShowWindow();
			}
			else
			{
				CloakEngine::Global::Graphic::HideWindow();
			}
		}
	}
}