#include "stdafx.h"
#include "Window/FileView.h"
#include "resource.h"
#include "CloakEditor.h"

#define MODIFY_TOOLBAR_STYLE(toolbar) toolbar.SetPaneStyle((m_toolbar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY) & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT))

namespace Editor {
	namespace Window {
		CLOAK_CALL FileView::FileView() 
		{
			m_lastContext = nullptr;
		}
		CLOAK_CALL FileView::~FileView() {}
		void CLOAK_CALL_THIS FileView::SetProject(In Project::Project* p)
		{
			m_project = p;
			m_tree.DeleteAllItems();
			if (m_project != nullptr)
			{
				Project::FileTree* tree = m_project->GetFileTree();
				AddExistingItem(tree->GetRoot());
			}
		}
		Project::FileTreeItem* CLOAK_CALL_THIS FileView::AddNewItem(In UINT contextMenu, In Project::FileTreeItem* parent, In const std::u16string& name, In int image, In int selectedImage)
		{
			std::tstring n = CloakEngine::Helper::StringConvert::ConvertToT(name);
			HTREEITEM i = m_tree.InsertItem(n.c_str(), image, selectedImage, parent->GetHandle());
			Project::FileTreeItem* item = parent->AddItem(0, i, name, image, selectedImage);
			m_tree.SetItemData(i, reinterpret_cast<DWORD_PTR>(item));
			return item;
		}

		afx_msg int FileView::OnCreate(LPCREATESTRUCT create)
		{
			if (CDockablePane::OnCreate(create) == -1) { return -1; }
			CRect eptyRct;
			eptyRct.SetRectEmpty();
			const DWORD flags = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;
			if (!m_tree.Create(flags, eptyRct, this, 4)) { return -1; }
			m_toolbar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDRT_FILEVIEW);
			m_toolbar.LoadToolBar(IDRT_FILEVIEW, 0, 0, TRUE);
			m_toolbar.CleanUpLockedImages();
			m_toolbar.LoadBitmap(IDB_FILEVIEW, 0, 0, TRUE);
			MODIFY_TOOLBAR_STYLE(m_toolbar);
			m_toolbar.SetOwner(this);
			m_toolbar.SetRouteCommandsViaFrame(FALSE);
			AdjustLayout();
			return 0;
		}
		afx_msg void FileView::OnSize(UINT type, int cx, int cy)
		{
			CDockablePane::OnSize(type, cx, cy);
			AdjustLayout();
		}
		afx_msg void FileView::OnContextMenu(CWnd* wnd, CPoint point)
		{
			CTreeCtrl* tree = (CTreeCtrl*)&m_tree;
			if (wnd != tree) { CDockablePane::OnContextMenu(wnd, point); }
			else
			{
				Project::FileTreeItem* ftitem = nullptr;
				if (point.x != -1 || point.y != -1)
				{
					CPoint pt = point;
					tree->ScreenToClient(&pt);
					UINT flags = 0;
					HTREEITEM item = tree->HitTest(pt, &flags);
					if (item != NULL)
					{
						tree->SelectItem(item);
						ftitem = reinterpret_cast<Project::FileTreeItem*>(tree->GetItemData(item));
					}
				}
				tree->SetFocus();
				m_lastContext = ftitem;
				if (ftitem != nullptr) 
				{
					GetApp()->GetContextMenuManager()->ShowPopupMenu(ftitem->GetContextMenu(), point.x, point.y, this, TRUE);
				}
			}
		}
		afx_msg void FileView::OnPaint()
		{
			CPaintDC dc(this);
			CRect rct;
			m_tree.GetWindowRect(rct);
			ScreenToClient(rct);
			rct.InflateRect(1, 1);
			dc.Draw3dRect(rct, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DSHADOW));
		}
		afx_msg void FileView::OnSetFocus(CWnd* oldWnd)
		{
			CDockablePane::OnSetFocus(oldWnd);
			m_tree.SetFocus();
		}

		void FileView::AdjustLayout()
		{
			if (GetSafeHwnd() == nullptr) { return; }
			CRect rct;
			GetClientRect(rct);
			const int lyt = m_toolbar.CalcFixedLayout(FALSE, TRUE).cy;
			m_toolbar.SetWindowPos(nullptr, rct.left, rct.top, rct.Width(), lyt, SWP_NOACTIVATE | SWP_NOZORDER);
			m_tree.SetWindowPos(nullptr, rct.left + 1, rct.top + lyt + 1, rct.Width() - 2, rct.Height() - lyt - 2, SWP_NOACTIVATE | SWP_NOZORDER);
		}
		void CLOAK_CALL_THIS FileView::AddExistingItem(In Project::FileTreeItem* item, In HTREEITEM parent)
		{
			if (item != nullptr)
			{
				std::tstring name = CloakEngine::Helper::StringConvert::ConvertToT(item->GetName());
				HTREEITEM i = m_tree.InsertItem(name.c_str(), item->GetImage(), item->GetSelectedImage(), parent);
				item->SetHandle(i);
				m_tree.SetItemData(i, reinterpret_cast<DWORD_PTR>(item));
				for (size_t a = 0; a < item->GetChildCount(); a++) { AddExistingItem(item->GetChild(a), i); }
			}
		}
		void FileView::OnProjectRename()
		{
			//TODO
		}
		void FileView::OnProjectProperties()
		{
			//TODO
		}

		void FileView::OnImageAddNew()
		{
			if (m_project != nullptr)
			{
				Project::FileTreeItem* rt = m_project->GetFileTree()->GetImagesRoot();
				std::u16string tmpName = u"Image" + CloakEngine::Helper::StringConvert::ConvertToU16(std::to_string(rt->GetChildCount() + 1));
				AddNewItem(0/*TODO*/, rt, tmpName, 0/*TODO*/, 0/*TODO*/);
			}
		}

		BEGIN_MESSAGE_MAP(FileView, CDockablePane)
			ON_WM_CREATE()
			ON_WM_SIZE()
			ON_WM_CONTEXTMENU()
			ON_WM_PAINT()
			ON_WM_SETFOCUS()
			//Project context menu
			ON_COMMAND(ID_PROJECT_RENAME,OnProjectRename)
			ON_COMMAND(ID_PROJECT_PROPERTIES,OnProjectProperties)
			//Image-Main context menu
			ON_COMMAND(ID_IMAGE_ADDNEW,OnImageAddNew)
		END_MESSAGE_MAP()

		CLOAK_CALL FileView::TreeView::TreeView() {}
		CLOAK_CALL FileView::TreeView::~TreeView() {}
		BOOL FileView::TreeView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* res)
		{
			BOOL r = CTreeCtrl::OnNotify(wParam, lParam, res);
			NMHDR* hdr = (NMHDR*)lParam;
			if (hdr != nullptr && hdr->code == TTN_SHOW && GetToolTips() != nullptr)
			{
				GetToolTips()->SetWindowPos(&wndTop, -1, -1, -1, -1, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
			}
			return r;
		}

		BEGIN_MESSAGE_MAP(FileView::TreeView, CTreeCtrl)
		END_MESSAGE_MAP()

		void FileView::Toolbar::OnUpdateCmdUI(In CFrameWnd* pDst, In BOOL disable) { CMFCToolBar::OnUpdateCmdUI((CFrameWnd*)GetOwner(), disable); }
		BOOL FileView::Toolbar::AllowShowOnList() const { return FALSE; }
	}
}