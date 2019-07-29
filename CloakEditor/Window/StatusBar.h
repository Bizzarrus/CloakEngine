#pragma once
#ifndef CE_EDIT_WINDOW_STATUSBAR_H
#define CE_EDIT_WINDOW_STATUSBAR_H

#include "AFXCore.h"

#define SBCB_ID_Pos IDS_INDICATOR_POS
#define SBCB_ID_Line IDS_INDICATOR_LINE
#define SBCB_ID_Changed IDS_INDICATOR_CHANGED
#define SBCB_ID_Progress IDS_INDICATOR_PROGRESS
#define SBCB_ID_Space IDS_INDICATOR_SPACE
#define SBCB_NAME(Name) OnStatusBarCallback##Name##Update
#define SBCB_DECL(Name,par,bar) afx_msg void SBCB_NAME(Name)(CCmdUI* ui)
#define SBCB_IMPL(Name,par,bar) afx_msg void par::SBCB_NAME(Name)(CCmdUI* ui){bar.OnUpdate##Name(ui);}
#define SBCB_OUCU(Name,par,bar) ON_UPDATE_COMMAND_UI(SBCB_ID_##Name,SBCB_NAME(Name))
#define SBCB(func,par,bar,ln) func(Pos,par,bar) ln func(Line,par,bar) ln func(Changed,par,bar) ln func(Progress,par,bar) ln func(Space,par,bar) ln

#define ON_STATUSBAR_UPDATE() SBCB(SBCB_OUCU,,,)
#define DECLARE_STATUSBAR_CALLBACK() SBCB(SBCB_DECL,,,;)
#define IMPLEMENT_STATUSBAR_CALLBACK(bar,par) SBCB(SBCB_IMPL,par,bar,)

namespace Editor {
	namespace Window {
		class StatusBar : public CMFCStatusBar {
			public:
				CLOAK_CALL StatusBar();
				void CLOAK_CALL_THIS SetIndicators();
				void CLOAK_CALL_THIS OnUpdatePos(In CCmdUI* ui);
				void CLOAK_CALL_THIS OnUpdateLine(In CCmdUI* ui);
				void CLOAK_CALL_THIS OnUpdateChanged(In CCmdUI* ui);
				void CLOAK_CALL_THIS OnUpdateProgress(In CCmdUI* ui);
				void CLOAK_CALL_THIS OnUpdateSpace(In CCmdUI* ui);
				void CLOAK_CALL_THIS OnUpdateMessage();

				void CLOAK_CALL_THIS SetLine(In size_t ln);
				void CLOAK_CALL_THIS SetPos(In size_t pos);
				void CLOAK_CALL_THIS SetChanged(In size_t c);
				void CLOAK_CALL_THIS SetMessage(In std::tstring msg);
				void CLOAK_CALL_THIS ShowProgressBar(In_opt unsigned int max = 100);
				void CLOAK_CALL_THIS HideProgressBar();
				void CLOAK_CALL_THIS SetProgress(In unsigned int p);
				void CLOAK_CALL_THIS HideLinePos();
			protected:
				bool m_lpV;
				size_t m_ln;
				size_t m_pos;
				size_t m_changed;
				std::tstring m_msg;
				COLORREF m_progressColor;
		};
	}
}

#endif