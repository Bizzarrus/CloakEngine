#pragma once
#ifndef CE_EDIT_WINDOW_OUTPUT_H
#define CE_EDIT_WINDOW_OUTPUT_H

#include "AFXCore.h"

#include <string>

#define DECLARE_OUTPUT_WINDOW(Name) Editor::Window::OutputList Name;

namespace Editor {
	namespace Window {
		enum class OutputWindow {
			Build,
			Console
		};
		class OutputList : public CListBox {
			public:
				CLOAK_CALL OutputList();
				void CLOAK_CALL_THIS PostString(In const std::u16string& msg);
				void CLOAK_CALL_THIS Clear();
			protected:
				afx_msg void OnContextMenu(CWnd* wnd, CPoint point);
				afx_msg void OnEditCopy();
				afx_msg void OnEditClear();
				afx_msg void OnViewOutput();

				DECLARE_MESSAGE_MAP()
		};
		class Output : public CDockablePane {
			public:
				CLOAK_CALL Output();
				void CLOAK_CALL_THIS PostString(In OutputWindow window, In const std::u16string& msg);
				void CLOAK_CALL_THIS ClearWindow(In OutputWindow window);
			protected:
				afx_msg int OnCreate(LPCREATESTRUCT create);
				afx_msg void OnSize(UINT type, int x, int y);

				CMFCTabCtrl m_tabs;
				DECLARE_OUTPUT_WINDOW(m_build)
				DECLARE_OUTPUT_WINDOW(m_console)

				DECLARE_MESSAGE_MAP()
		};
	}
}

#endif