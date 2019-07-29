// CloakEditor.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//
#include "stdafx.h"
#include "Resource.h"
#include "CloakEditor.h"
#include "CloakEngine/CloakEngine.h"
#include "CloakCallback/CEFactory.h"
#include "Window/MainFrame.h"
#include "Window/MDIFrame.h"
#include "Document/MeshDoc.h"
#include "Document/ImageDoc.h"
#include "UndoManager.h"

#include <atomic>

#define DEL_REPL(a,n) delete a; a=n

#define MDI_TEMPLATE_PATH(Name) Editor::Document::Name
#define MDI_TEMPLATE(Name) new CMultiDocTemplate(MDI_TEMPLATE_PATH(Name)::TypeID, RUNTIME_CLASS(MDI_TEMPLATE_PATH(Name)::Name##Document), RUNTIME_CLASS(Editor::Window::MDIFrame), RUNTIME_CLASS(MDI_TEMPLATE_PATH(Name)::Name##View))

class CloakEditorApp : public CWinAppEx {
	public:
		CLOAK_CALL CloakEditorApp()
		{
			SetAppID(_T("CloakEditor.Version." C_EDIT_VERSION));
		}
		virtual BOOL InitInstance() override
		{
			Editor::UndoManager::OnStart();
			//Init main frame
			INITCOMMONCONTROLSEX InitCtrls;
			InitCtrls.dwSize = sizeof(InitCtrls);
			InitCtrls.dwICC = ICC_WIN95_CLASSES;
			InitCommonControlsEx(&InitCtrls);
			CWinAppEx::InitInstance();
			if (!AfxOleInit())
			{
				//AfxMessageBox(IDP_OLE_INIT_FAILED);
				return FALSE;
			}
			AfxEnableControlContainer();
			EnableTaskbarInteraction();
			SetRegistryKey(_T("CloakEditor"));
			InitContextMenuManager();
			InitKeyboardManager();
			InitTooltipManager();
			CMFCToolTipInfo ttParams;
			ttParams.m_bVislManagerTheme = TRUE;
			GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL, RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

			//TODO: Register MultiDocTemplate, example:
			m_docMesh = MDI_TEMPLATE(Mesh);
			m_docImage = MDI_TEMPLATE(Image);
			AddDocTemplate(m_docMesh);
			AddDocTemplate(m_docImage);

			m_frame = new Editor::Window::MainFrame();
			if (m_frame == nullptr || !m_frame->LoadFrame(IDR_MAINFRAME))
			{
				delete m_frame;
				m_frame = nullptr;
				return FALSE;
			}
			m_pMainWnd = m_frame;
			m_frame->SetProject(m_proj);

			CCommandLineInfo cmdInfo;
			ParseCommandLine(cmdInfo);
			//TODO: May parse commands
			if (!ProcessShellCommand(cmdInfo)) { return FALSE; }

#ifdef _DEBUG
			SetProject(new Editor::Project::Project(u"Test-Project"));
#endif

			//Start Engine
			CloakEngine::Global::GameInfo ginfo;
#ifdef _DEBUG
			ginfo.debugMode = true;
#else
			ginfo.debugMode = false;
#endif
			ginfo.useConsole = false;
			ginfo.useSteam = false;
			ginfo.useWindow = true;
			CloakEngine::Global::Game::StartEngineAsync(new Editor::CEFactory(), ginfo, m_frame->GetEngineWindowHandle());

			//Show main frame
			m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
			m_pMainWnd->UpdateWindow();
			return TRUE;
		}
		virtual int ExitInstance() override
		{
			if (m_exiting.exchange(true) == false)
			{
				CloakEngine::Global::Game::Stop();
			}
			CloakEngine::Global::Game::WaitForStop();
			Editor::UndoManager::OnStop();
			AfxOleTerm(FALSE);
			DEL_REPL(m_proj, nullptr);
			return CWinAppEx::ExitInstance();
		}

		void CLOAK_CALL_THIS SetProject(In Editor::Project::Project* p)
		{
			if (p != m_proj)
			{
				if (m_frame != nullptr) { m_frame->SetProject(p); }
				DEL_REPL(m_proj, p);
			}
		}
		Editor::Window::MainFrame* CLOAK_CALL_THIS GetWindow() { return m_frame; }

		void AFX_MSG_CALL OnFileNew()
		{
			
			//CWinAppEx::OnFileNew();
			if (m_showStartup.exchange(false))
			{
				//TODO
			}
			else
			{
				//TODO
				CloakDebugLog("OnFileNew");
				CDocument* doc = m_docMesh->OpenDocumentFile(NULL);
				m_frame->OnCreateDocument();
			}
		}
		void AFX_MSG_CALL OnFileOpen()
		{
			//TODO
			CloakDebugLog("OnFileOpen");
		}
		void AFX_MSG_CALL OnFileSave()
		{
			//TODO
			CloakDebugLog("OnFileSave");
		}
		void AFX_MSG_CALL OnFileSaveAs()
		{
			//TODO
			CloakDebugLog("OnFileSaveAs");
		}

		void AFX_MSG_CALL OnEditUndo()
		{
			Editor::UndoManager::Undo();
		}
		void AFX_MSG_CALL OnEditUndoUpdate(CCmdUI* ui)
		{
			if (Editor::UndoManager::CanUndo()) { ui->Enable(TRUE); }
			else { ui->Enable(FALSE); }
		}
		void AFX_MSG_CALL OnEditRedo()
		{
			Editor::UndoManager::Redo();
		}
		void AFX_MSG_CALL OnEditRedoUpdate(CCmdUI* ui)
		{
			if (Editor::UndoManager::CanRedo()) { ui->Enable(TRUE); }
			else { ui->Enable(FALSE); }
		}

		void CLOAK_CALL OpenDocMesh(In const std::string& file)
		{
			OpenDocument(m_docMesh, file);
		}
		void CLOAK_CALL OpenDocImage(In const std::string& file)
		{
			OpenDocument(m_docImage, file);
		}

		void CLOAK_CALL Close()
		{
			if (m_exiting.exchange(true) == false)
			{
				if (m_pMainWnd != nullptr) { m_pMainWnd->PostMessageW(WM_CLOSE); }
			}
		}
		DECLARE_MESSAGE_MAP()
	private:
		std::atomic<bool> m_exiting = false;
		std::atomic<bool> m_showStartup = true;
		Editor::Window::MainFrame* m_frame = nullptr;
		Editor::Project::Project* m_proj = nullptr;
		CMultiDocTemplate* m_docMesh = nullptr;
		CMultiDocTemplate* m_docImage = nullptr;

		void CLOAK_CALL_THIS OpenDocument(In CMultiDocTemplate* doc, In const std::string& file)
		{
			if (file.length() == 0) { doc->OpenDocumentFile(NULL); }
			else
			{
				std::tstring tf = CloakEngine::Helper::StringConvert::ConvertToT(file);
				doc->OpenDocumentFile(tf.c_str());
			}
			m_frame->OnCreateDocument();
		}
};

BEGIN_MESSAGE_MAP(CloakEditorApp, CWinAppEx)
	//ON_COMMAND(ID_APP_ABOUT, &CloakEditorApp::OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, &CloakEditorApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CloakEditorApp::OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, &CloakEditorApp::OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, &CloakEditorApp::OnFileSaveAs)
	ON_COMMAND(ID_EDIT_UNDO,&CloakEditorApp::OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO,&CloakEditorApp::OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO,&CloakEditorApp::OnEditUndoUpdate)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CloakEditorApp::OnEditRedoUpdate)
END_MESSAGE_MAP()

CloakEditorApp g_app;

namespace Editor {
	void CLOAK_CALL Stop()
	{
		g_app.Close();
	}
	CWinAppEx* CLOAK_CALL GetApp() { return &g_app; }
	void CLOAK_CALL SetProject(In Project::Project* p) { g_app.SetProject(p); }
	void CLOAK_CALL SetStatus(In const std::string& str, In int pos)
	{
		std::tstring s = CloakEngine::Helper::StringConvert::ConvertToT(str);
		//g_app.GetWindow()->GetStatusBar()
	}
	namespace Console {
		void CLOAK_CALL WriteToLog(In const std::u16string& str) { g_app.GetWindow()->GetOutput()->PostString(Window::OutputWindow::Console, str); }
		void CLOAK_CALL WriteToLog(In const std::string& str) { WriteToLog(CloakEngine::Helper::StringConvert::ConvertToU16(str)); }
	}
	namespace Document {
		void CLOAK_CALL OpenMesh(In_opt std::string file) { g_app.OpenDocMesh(file); }
		void CLOAK_CALL OpenImage(In_opt std::string file) {}
	}
}