#pragma once
#ifndef CE_EDIT_WINDOW_FILEVIEW_H
#define CE_EDIT_WINDOW_FILEVIEW_H

#include "AFXCore.h"
#include "Project/Project.h"

namespace Editor {
	namespace Window {
		class FileView : public CDockablePane {
			public:
				CLOAK_CALL FileView();
				virtual CLOAK_CALL ~FileView();
				void CLOAK_CALL_THIS SetProject(In Project::Project* p);
				Project::FileTreeItem* CLOAK_CALL_THIS AddNewItem(In UINT contextMenu, In Project::FileTreeItem* parent, In const std::u16string& name, In int image, In int selectedImage);
			protected:
				class Toolbar : public CMFCToolBar {
					public:
						virtual void OnUpdateCmdUI(In CFrameWnd* pDst, In BOOL disable) override;
						virtual BOOL AllowShowOnList() const override;
				};
				class TreeView : public CTreeCtrl {
					public:
						CLOAK_CALL TreeView();
						virtual CLOAK_CALL ~TreeView();
					protected:
						virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* res) override;
						DECLARE_MESSAGE_MAP()
				};

				afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
				afx_msg void OnSize(UINT nType, int cx, int cy);
				afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
				afx_msg void OnPaint();
				afx_msg void OnSetFocus(CWnd* pOldWnd);

				void AdjustLayout();
				void CLOAK_CALL_THIS AddExistingItem(In Project::FileTreeItem* item, In_opt HTREEITEM parent = TVI_ROOT);

				void OnProjectRename();
				void OnProjectProperties();

				void OnImageAddNew();

				Toolbar m_toolbar;
				TreeView m_tree;
				Project::Project* m_project;
				Project::FileTreeItem* m_lastContext;

				DECLARE_MESSAGE_MAP()
		};
	}
}

#endif