#include "stdafx.h"
#include "resource.h"
#include "Window/Output.h"

#include "CloakEditor.h"

#define ADD_TAB(ID,tabs,wnd) {CString name; BOOL rb=name.LoadString(ID); ASSERT(rb); tabs.AddTab(&wnd,name);}
#define INIT_OUTPUT_WINDOW(Name,rct,tabs,img) if(!Name.Create(LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,rct,&tabs,__COUNTER__+1)){return -1;} Name.SetFont(&afxGlobalData.fontRegular); ADD_TAB(img,tabs,Name)

namespace Editor {
	namespace Window {
		CLOAK_CALL Output::Output()
		{

		}
		void CLOAK_CALL_THIS Output::PostString(In OutputWindow window, In const std::u16string& msg)
		{
			OutputList* wnd = nullptr;
			switch (window)
			{
				case Editor::Window::OutputWindow::Build:
					wnd = &m_build;
					break;
				case Editor::Window::OutputWindow::Console:
					wnd = &m_console;
					break;
				default:
					break;
			}
			if (wnd != nullptr) { wnd->PostString(msg); }
		}
		void CLOAK_CALL_THIS Output::ClearWindow(In OutputWindow window)
		{
			OutputList* wnd = nullptr;
			switch (window)
			{
				case Editor::Window::OutputWindow::Build:
					wnd = &m_build;
					break;
				case Editor::Window::OutputWindow::Console:
					wnd = &m_console;
					break;
				default:
					break;
			}
			if (wnd != nullptr) { wnd->Clear(); }
		}

		afx_msg int Output::OnCreate(LPCREATESTRUCT create)
		{
			if (CDockablePane::OnCreate(create) == -1) { return -1; }
			CRect eptyRct;
			eptyRct.SetRectEmpty();
			if (!m_tabs.Create(CMFCTabCtrl::STYLE_3D, eptyRct, this, __COUNTER__ + 1)) { return -1; }
			INIT_OUTPUT_WINDOW(m_build, eptyRct, m_tabs, IDS_OUTPUT_BUILD);
			INIT_OUTPUT_WINDOW(m_console, eptyRct, m_tabs, IDS_OUTPUT_CONSOLE);
			return 0;
		}
		afx_msg void Output::OnSize(UINT type, int x, int y)
		{
			CDockablePane::OnSize(type, x, y);
			m_tabs.SetWindowPos(nullptr, -1, -1, x, y, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
		}

		BEGIN_MESSAGE_MAP(Output, CDockablePane)
			ON_WM_CREATE()
			ON_WM_SIZE()
		END_MESSAGE_MAP()


		CLOAK_CALL OutputList::OutputList()
		{

		}
		void CLOAK_CALL_THIS OutputList::PostString(In const std::u16string& msg)
		{
			std::tstring m = CloakEngine::Helper::StringConvert::ConvertToT(msg);
			AddString(m.c_str());
		}
		void CLOAK_CALL_THIS OutputList::Clear()
		{
			ResetContent();
		}

		afx_msg void OutputList::OnContextMenu(CWnd* wnd, CPoint point)
		{
			CWnd* main = AfxGetMainWnd();
			if (main->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
			{
				CMenu menu;
				menu.LoadMenu(IDR_OUTPUT);
				CMenu* sub = menu.GetSubMenu(0);
				CMFCPopupMenu* popup = new CMFCPopupMenu();
				if (!popup->Create(this, point.x, point.y, (HMENU)sub->m_hMenu, FALSE, TRUE))
				{
					return;
				}
				((CMDIFrameWndEx*)main)->OnShowPopupMenu(popup);
				UpdateDialogControls(this, FALSE);
			}
			SetFocus();
		}
		afx_msg void OutputList::OnEditCopy()
		{
			CloakDebugLog("OnCopy");
			for (int a = 0; a < 4; a++)
			{
				Editor::SetStatus("Status "+std::to_string(a), a);
			}
		}
		afx_msg void OutputList::OnEditClear()
		{
			Clear();
		}
		afx_msg void OutputList::OnViewOutput()
		{
			CDockablePane* par = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
			CMDIFrameWndEx* main = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());
			if (main != nullptr && par != nullptr)
			{
				main->SetFocus();
				main->ShowPane(par, FALSE, FALSE, FALSE);
				main->RecalcLayout();
			}
		}

		BEGIN_MESSAGE_MAP(OutputList, CListBox)
			ON_WM_CONTEXTMENU()
			ON_COMMAND(ID_OUTPUT_COPY, OnEditCopy)
			ON_COMMAND(ID_OUTPUT_DELETE, OnEditClear)
			ON_COMMAND(ID_OUTPUT_WINDOW, OnViewOutput)
			ON_WM_WINDOWPOSCHANGING()
		END_MESSAGE_MAP()
	}
}