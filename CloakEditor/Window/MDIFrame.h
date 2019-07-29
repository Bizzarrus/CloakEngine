#pragma once
#ifndef CE_EDIT_WINDOW_MDIFRAME_H
#define CE_EDIT_WINDOW_MDIFRAME_H

namespace Editor {
	namespace Window {
		class MDIFrame : public CMDIChildWndEx {
			DECLARE_DYNCREATE(MDIFrame);
			public:
				virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
			protected:
				DECLARE_MESSAGE_MAP()
		};
	}
}

#endif