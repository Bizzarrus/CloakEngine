#pragma once
#ifndef CE_EDIT_WINDOW_MAINFRAME_H
#define CE_EDIT_WINDOW_MAINFRAME_H

#include "AFXCore.h"
#include "FileView.h"
#include "Output.h"
#include "StatusBar.h"
#include "EngineWindow.h"
#include "Project/Project.h"

#define DECLARE_TOGGLE_FRAME(Name) afx_msg void On##Name##Update(CCmdUI* ui); afx_msg void On##Name##Toggle();
#define IMPLEMENT_TOGGLE_FRAME(Name,Frame,Parent) afx_msg void Parent::On##Name##Update(CCmdUI* ui){ui->SetCheck(Frame.IsVisible());} afx_msg void Parent::On##Name##Toggle(){Frame.ShowWindow(Frame.IsVisible() ? SW_HIDE : SW_SHOW); RecalcLayout();}
#define ON_TOGGLE_FRAME(Name,ID,Parent) ON_UPDATE_COMMAND_UI(ID, &Parent::On##Name##Update) ON_COMMAND(ID, &Parent::On##Name##Toggle)

namespace Editor {
	namespace Window {
		class MainFrame : public CMDIFrameWndEx {
			DECLARE_DYNAMIC(MainFrame);
			public:
				CLOAK_CALL MainFrame();
				virtual CLOAK_CALL ~MainFrame();
				virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
				virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL) override;
				void CLOAK_CALL_THIS SetProject(In Project::Project* proj);
				void CLOAK_CALL_THIS OnCreateDocument();
				FileView* CLOAK_CALL_THIS GetFileView();
				Output* CLOAK_CALL_THIS GetOutput();
				StatusBar* CLOAK_CALL_THIS GetStatusBar();
				HWND CLOAK_CALL_THIS GetEngineWindowHandle();
			protected:
				void CLOAK_CALL_THIS UpdateDocument();

				afx_msg LPARAM OnToolbarCreateNew(WPARAM wp, LPARAM lp);
				afx_msg LRESULT OnSetMessageString(WPARAM wp, LPARAM lp);
				afx_msg LRESULT OnChangingDocumentTab(WPARAM wParam, LPARAM lParam);
				afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
				afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
				afx_msg void OnCustomizeToolbar();
				DECLARE_TOGGLE_FRAME(Toolbar)
				DECLARE_TOGGLE_FRAME(Statusbar)
				DECLARE_TOGGLE_FRAME(FileView)
				DECLARE_TOGGLE_FRAME(Output)
				DECLARE_STATUSBAR_CALLBACK();
				DECLARE_MESSAGE_MAP()

				CMFCMenuBar m_menuBar;
				CMFCToolBar m_toolBar;
				StatusBar m_statusBar;
				FileView m_fileView;
				Output m_output;
				EngineWindow m_engine;
		};
	}
}

#endif