#include "stdafx.h"
#include "resource.h"
#include "Window/MainFrame.h"

#include <vector>

namespace Editor {
	namespace Window {
		struct IterateMenuInfo {
			CMenu* menu;
			int pos;
			IterateMenuInfo(In CMenu* m) { menu = m; pos = 0; }
		};

		BEGIN_MESSAGE_MAP(MainFrame, CMDIFrameWndEx)
			ON_WM_CREATE()
			ON_WM_SETTINGCHANGE()
			ON_COMMAND(ID_VIEW_TOOLBAR_CUSTOM, &MainFrame::OnCustomizeToolbar)
			//ON_COMMAND(ID_WINDOW_MANAGER, &CMainFrame::OnWindowManager)
			//ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
			ON_TOGGLE_FRAME(Toolbar, ID_VIEW_TOOLBAR_VISIBLE, MainFrame)
			ON_TOGGLE_FRAME(Statusbar, ID_VIEW_STATUS_BAR, MainFrame)
			ON_TOGGLE_FRAME(Output, ID_VIEW_OUTPUT, MainFrame)
			ON_TOGGLE_FRAME(FileView, ID_VIEW_FILEVIEW, MainFrame)
			ON_MESSAGE(WM_SETMESSAGESTRING, OnSetMessageString)
			ON_REGISTERED_MESSAGE(AFX_WM_CHANGING_ACTIVE_TAB, OnChangingDocumentTab)
			ON_STATUSBAR_UPDATE()
			ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &MainFrame::OnToolbarCreateNew)
		END_MESSAGE_MAP()

		CLOAK_CALL MainFrame::MainFrame()
		{
			//Create(nullptr, _T("CloakEditor"));
		}
		CLOAK_CALL MainFrame::~MainFrame()
		{

		}
		BOOL MainFrame::PreCreateWindow(CREATESTRUCT& cs)
		{
			if (!CMDIFrameWndEx::PreCreateWindow(cs)) { return FALSE; }
			cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE | WS_SYSMENU;
			return TRUE;
		}
		BOOL MainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
		{
			if (!CMDIFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext)) { return FALSE; }
			return TRUE;
		}
		void CLOAK_CALL_THIS MainFrame::SetProject(In Project::Project* proj)
		{
			m_fileView.SetProject(proj);
		}
		void CLOAK_CALL_THIS MainFrame::OnCreateDocument()
		{
			UpdateDocument();
		}
		FileView* CLOAK_CALL_THIS MainFrame::GetFileView() { return &m_fileView; }
		Output* CLOAK_CALL_THIS MainFrame::GetOutput() { return &m_output; }
		StatusBar* CLOAK_CALL_THIS MainFrame::GetStatusBar() { return &m_statusBar; }
		HWND CLOAK_CALL_THIS MainFrame::GetEngineWindowHandle() { return m_engine.GetSafeHwnd(); }

		void CLOAK_CALL_THIS MainFrame::UpdateDocument()
		{
			CWnd* main = AfxGetMainWnd();
			CFrameWnd* frame = dynamic_cast<CMDIFrameWnd*>(main)->MDIGetActive();
			if (frame != nullptr)
			{
				CView* active = frame->GetActiveView();
				if (active != nullptr)
				{
					Editor::Window::EngineView* view = dynamic_cast<Editor::Window::EngineView*>(active);
					m_engine.SetDocument(view);
				}
			}
		}

		afx_msg LPARAM MainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM lp)
		{
			return CMDIFrameWndEx::OnToolbarCreateNew(wp, lp);
		}
		afx_msg void MainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
		{

		}
		afx_msg void MainFrame::OnCustomizeToolbar()
		{

		}
		afx_msg LRESULT MainFrame::OnSetMessageString(WPARAM wp, LPARAM lp)
		{
			if (wp == AFX_IDS_IDLEMESSAGE) 
			{
				m_statusBar.OnUpdateMessage();
				return 0;
			}
			return CMDIFrameWndEx::OnSetMessageString(wp, lp);
		}
		afx_msg LRESULT MainFrame::OnChangingDocumentTab(WPARAM wParam, LPARAM lParam)
		{
			UpdateDocument();
			return 0;
		}
		afx_msg int MainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
		{
			if (CMDIFrameWndEx::OnCreate(lpCreateStruct) == -1) { return -1; }
			SetWindowText(_T("CloakEditor"));

			CMDITabInfo tifo;
			tifo.m_style = CMFCTabCtrl::STYLE_3D_ONENOTE;
			tifo.m_bActiveTabCloseButton = TRUE;
			tifo.m_bTabIcons = FALSE;
			tifo.m_bAutoColor = TRUE;
			tifo.m_bDocumentMenu = TRUE;
			EnableMDITabbedGroups(TRUE, tifo);

			if (!m_menuBar.Create(this)) { return -1; }
			m_menuBar.SetPaneStyle(m_menuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

			CMFCPopupMenu::SetForceMenuFocus(FALSE);

			BOOL rb = m_toolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
			rb = rb || m_toolBar.LoadToolBar(IDRT_MAINFRAME);
			if (!rb) { return -1; }

			CString tbName;
			rb = tbName.LoadString(IDS_TB_MAINFRAME_DEFAULT);
			ASSERT(rb);
			m_toolBar.SetWindowText(tbName);

			CString tbCustom;
			rb = tbCustom.LoadString(IDS_TB_MAINFRAME_CUSTOM);
			ASSERT(rb);
			m_toolBar.EnableCustomizeButton(TRUE, ID_VIEW_TOOLBAR_CUSTOM, tbCustom);

			if (!m_statusBar.Create(this)) { return -1; }
			m_statusBar.SetIndicators();

			EnablePaneMenu(TRUE, ID_VIEW_TOOLBAR_CUSTOM, tbCustom, ID_VIEW_TOOLBAR);

			CMFCToolBar::EnableQuickCustomization();

			//m_menuBar.EnableDocking(CBRS_ALIGN_ANY);
			//m_toolBar.EnableDocking(CBRS_ALIGN_ANY);
			EnableDocking(CBRS_ALIGN_ANY);
			DockPane(&m_menuBar);
			DockPane(&m_toolBar);

			CDockingManager::SetDockingMode(DT_SMART);
			EnableAutoHidePanes(CBRS_ALIGN_ANY);

			//Initialize docking windows
			rb = tbName.LoadString(IDS_FILEVIEW_NAME);
			ASSERT(rb);
			if (!m_fileView.Create(tbName, this, CRect(0, 0, 200, 200), TRUE, ID_FILEVIEW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
			{
				return -1;
			}
			m_fileView.EnableDocking(CBRS_ALIGN_ANY);
			DockPane(&m_fileView);

			rb = tbName.LoadString(IDS_OUTPUT_NAME);
			ASSERT(rb);
			if (!m_output.Create(tbName, this, CRect(0, 0, 100, 100), TRUE, ID_OUTPUT_WINDOW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
			{
				return -1;
			}
			m_output.EnableDocking(CBRS_ALIGN_ANY);
			DockPane(&m_output);

			if (!m_engine.Create(_T("STATIC"), NULL, WS_CHILD | WS_VISIBLE, CRect(50, 50, 500, 500), this, ID_EngineWindow)) { return -1; }
			m_engine.SetRedraw(FALSE);
			m_engine.SetParent(this);

			//Init basic comands
			CList<UINT, UINT> cmds;
			cmds.AddTail(ID_FILE_NEW);
			cmds.AddTail(ID_FILE_OPEN);
			cmds.AddTail(ID_FILE_SAVE);
			cmds.AddTail(ID_FILE_SAVE_AS);
			cmds.AddTail(ID_FILE_NEW_IMAGE);
			cmds.AddTail(ID_APP_EXIT);
			cmds.AddTail(ID_EDIT_UNDO);
			cmds.AddTail(ID_EDIT_REDO);
			cmds.AddTail(ID_EDIT_PASTE);
			cmds.AddTail(ID_EDIT_CUT);
			cmds.AddTail(ID_EDIT_COPY);
			cmds.AddTail(ID_EDIT_FIND);
			cmds.AddTail(ID_VIEW_TOOLBAR_VISIBLE);
			cmds.AddTail(ID_VIEW_TOOLBAR_CUSTOM);
			cmds.AddTail(ID_VIEW_STATUS_BAR);
			cmds.AddTail(ID_VIEW_OUTPUT);
			cmds.AddTail(ID_VIEW_FILEVIEW);
			CMFCToolBar::SetBasicCommands(cmds);

			ModifyStyle(0, FWS_PREFIXTITLE);
			return 0;
		}

		IMPLEMENT_STATUSBAR_CALLBACK(m_statusBar, MainFrame);
		IMPLEMENT_TOGGLE_FRAME(Toolbar, m_toolBar, MainFrame);
		IMPLEMENT_TOGGLE_FRAME(Statusbar, m_statusBar, MainFrame);
		IMPLEMENT_TOGGLE_FRAME(Output, m_output, MainFrame);
		IMPLEMENT_TOGGLE_FRAME(FileView, m_fileView, MainFrame);
		IMPLEMENT_DYNAMIC(MainFrame, CMDIFrameWndEx);
	}
}