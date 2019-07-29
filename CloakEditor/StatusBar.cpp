#include "stdafx.h"
#include "resource.h"
#include "Window/StatusBar.h"

#define INDICATORS(func)	func(ID_SEPARATOR)\
							func(IDS_INDICATOR_PROGRESS)\
							func(IDS_INDICATOR_SPACE)\
							func(IDS_INDICATOR_POS)\
							func(IDS_INDICATOR_LINE)\
							func(IDS_INDICATOR_CHANGED)\
							func(ID_INDICATOR_CAPS)\
							func(ID_INDICATOR_NUM)\
							func(ID_INDICATOR_SCRL)

#define INDICATOR_DEF(name) name,
#define INDICATOR_POS(name) POS_##name,
#define INIT_PANE(name,wid) SetPaneWidth(POS_##name,wid); SetPaneStyle(POS_##name,g_StyleDefault)

namespace Editor {
	namespace Window {
		constexpr UINT g_StyleDefault = SBPS_NOBORDERS;
		constexpr UINT g_StyleBordered = g_StyleDefault & ~(SBPS_NOBORDERS);

		constexpr COLORREF g_progressColor[] = {
			CLR_DEFAULT,
			RGB(71, 184, 222)
		};

		constexpr UINT g_indicators[] = {
			INDICATORS(INDICATOR_DEF)
		};
		enum indicatorPos {
			INDICATORS(INDICATOR_POS)
			POS_IDS_INDICATOR_DEFAULT=0,
		};

		CLOAK_CALL StatusBar::StatusBar()
		{
			m_pos = 0;
			m_ln = 0;
			m_changed = 0;
			m_lpV = false;
			m_msg = _T("Ready");
			m_progressColor = g_progressColor[0];
		}
		void CLOAK_CALL_THIS StatusBar::SetIndicators()
		{
			CMFCStatusBar::SetIndicators(g_indicators, ARRAYSIZE(g_indicators));
			INIT_PANE(IDS_INDICATOR_PROGRESS, 200);
			INIT_PANE(IDS_INDICATOR_SPACE, 600);
			INIT_PANE(IDS_INDICATOR_POS, 100);
			INIT_PANE(IDS_INDICATOR_LINE, 100);
			INIT_PANE(IDS_INDICATOR_CHANGED, 150);

			ShowProgressBar();
			SetProgress(50);
		}
		void CLOAK_CALL_THIS StatusBar::OnUpdateProgress(In CCmdUI* ui)
		{
			ui->Enable(TRUE);
			ui->SetText(_T(""));
		}
		void CLOAK_CALL_THIS StatusBar::OnUpdateSpace(In CCmdUI* ui)
		{
			ui->SetText(_T(""));
			ui->Enable(FALSE);
		}
		void CLOAK_CALL_THIS StatusBar::OnUpdatePos(In CCmdUI* ui)
		{
			ui->Enable(TRUE);
			if (m_lpV)
			{
				std::tstring s = CloakEngine::Helper::StringConvert::ConvertToT("Pos: " + std::to_string(m_pos));
				ui->SetText(s.c_str());
			}
			else
			{
				ui->SetText(_T(""));
			}
		}
		void CLOAK_CALL_THIS StatusBar::OnUpdateLine(In CCmdUI* ui)
		{
			ui->Enable(TRUE);
			if (m_lpV)
			{	
				std::tstring s = CloakEngine::Helper::StringConvert::ConvertToT("Ln: " + std::to_string(m_ln));
				ui->SetText(s.c_str());
			}
			else
			{
				ui->SetText(_T(""));
			}
		}
		void CLOAK_CALL_THIS StatusBar::OnUpdateChanged(In CCmdUI* ui)
		{
			ui->Enable(TRUE);
			std::tstring s = CloakEngine::Helper::StringConvert::ConvertToT("Changed: " + std::to_string(m_changed));
			ui->SetText(s.c_str());
		}
		void CLOAK_CALL_THIS StatusBar::OnUpdateMessage()
		{
			SetPaneText(POS_IDS_INDICATOR_DEFAULT, m_msg.c_str());
		}

		void CLOAK_CALL_THIS StatusBar::ShowProgressBar(In_opt unsigned int max)
		{
			EnablePaneProgressBar(POS_IDS_INDICATOR_PROGRESS, static_cast<long>(max), 0, m_progressColor, m_progressColor);
			SetPaneStyle(POS_IDS_INDICATOR_PROGRESS, g_StyleBordered);
			SetPaneProgress(POS_IDS_INDICATOR_PROGRESS, 0);
		}
		void CLOAK_CALL_THIS StatusBar::HideProgressBar()
		{
			SetPaneStyle(POS_IDS_INDICATOR_PROGRESS, g_StyleDefault);
			EnablePaneProgressBar(POS_IDS_INDICATOR_PROGRESS, -1);
		}
		void CLOAK_CALL_THIS StatusBar::SetProgress(In unsigned int p)
		{
			SetPaneProgress(POS_IDS_INDICATOR_PROGRESS, p);
		}
		void CLOAK_CALL_THIS StatusBar::SetLine(In size_t ln)
		{
			m_ln = ln;
			m_lpV = true;
		}
		void CLOAK_CALL_THIS StatusBar::SetPos(In size_t pos)
		{
			m_pos = pos;
			m_lpV = true;
		}
		void CLOAK_CALL_THIS StatusBar::SetChanged(In size_t c)
		{
			m_changed = c;
		}
		void CLOAK_CALL_THIS StatusBar::SetMessage(In std::tstring msg)
		{
			m_msg = msg;
		}
		void CLOAK_CALL_THIS StatusBar::HideLinePos()
		{
			m_lpV = false;
		}
	}
}