#pragma once
#ifndef CE_EDIT_AFXCORE_H
#define CE_EDIT_AFXCORE_H

#include <afx.h>
#include <afxwin.h>         // MFC-Kern- und -Standardkomponenten
#include <afxext.h>         // MFC-Erweiterungen

#include <afxdisp.h>        // MFC-Automatisierungsklassen

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC-Unterstützung für allgemeine Steuerelemente von Internet Explorer 4
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC-Unterstützung für allgemeine Windows-Steuerelemente
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC-Unterstützung für Menübänder und Steuerleisten

#include "CloakEngine/CloakEngine.h"

#endif