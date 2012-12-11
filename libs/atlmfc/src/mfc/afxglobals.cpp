// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "comdef.h"
#include "afxglobals.h"

/////////////////////////////////////////////////////////////////////////////
// Cached system metrics, etc
AFX_GLOBAL_DATA afxGlobalData;

// Initialization code
AFX_GLOBAL_DATA::AFX_GLOBAL_DATA()
{
	// Detect the kind of OS:
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	::GetVersionEx(&osvi);

	bIsRemoteSession = GetSystemMetrics(SM_REMOTESESSION);

	bIsWindowsVista = (osvi.dwMajorVersion >= 6);
	bIsWindows7 = (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion >= 1) || (osvi.dwMajorVersion > 6) ;
	bDisableAero = FALSE;

	// Cached system values(updated in CWnd::OnSysColorChange)
	hbrBtnShadow = NULL;
	hbrBtnHilite = NULL;
	hbrWindow = NULL;

	{
		m_pfDrawThemeBackground = NULL;
		m_pfDrawThemeTextEx = NULL;

		m_pfBufferedPaintInit = NULL;
		m_pfBufferedPaintUnInit = NULL;

		m_pfBeginBufferedPaint = NULL;
		m_pfEndBufferedPaint = NULL;
	}

	{
		m_pfDwmExtendFrameIntoClientArea = NULL;
		m_pfDwmDefWindowProc = NULL;
		m_pfDwmIsCompositionEnabled = NULL;
	}

	m_hcurStretch = NULL;
	m_hcurStretchVert = NULL;
	m_hcurHand = NULL;
	m_hcurSizeAll = NULL;
	m_hiconTool = NULL;
	m_hiconLink = NULL;
	m_hiconColors = NULL;
	m_hcurMoveTab = NULL;
	m_hcurNoMoveTab = NULL;

	m_bUseSystemFont = FALSE;
	m_bInSettingChange = FALSE;

	OnSettingChange();

	m_bIsRTL = FALSE;

	m_nDragFrameThicknessFloat = 4;  // pixels
	m_nDragFrameThicknessDock = 3;   // pixels

	m_nAutoHideToolBarSpacing = 14; // pixels
	m_nAutoHideToolBarMargin  = 4;  // pixels

	m_nCoveredMainWndClientAreaPercent = 50; // percents

	m_nMaxToolTipWidth = -1;
	m_bIsBlackHighContrast = FALSE;
	m_bIsWhiteHighContrast = FALSE;

	m_bUseBuiltIn32BitIcons = TRUE;

	m_bComInitialized = FALSE;

	m_pTaskbarList = NULL;
	m_pTaskbarList3 = NULL;
	m_bTaskBarInterfacesAvailable = TRUE;

	EnableAccessibilitySupport();
}

AFX_GLOBAL_DATA::~AFX_GLOBAL_DATA()
{
	CleanUp();
}

static BOOL CALLBACK InfoEnumProc( HMONITOR hMonitor, HDC /*hdcMonitor*/, LPRECT /*lprcMonitor*/, LPARAM dwData)
{
	CRect* pRect = (CRect*) dwData;

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);

	if (GetMonitorInfo(hMonitor, &mi))
	{
		CRect rectMon = mi.rcWork;

		pRect->left = min(pRect->left, rectMon.left);
		pRect->right = max(pRect->right, rectMon.right);
		pRect->top = min(pRect->top, rectMon.top);
		pRect->bottom = max(pRect->bottom, rectMon.bottom);
	}

	return TRUE;
}

void AFX_GLOBAL_DATA::OnSettingChange()
{
	m_bInSettingChange = TRUE;

	m_sizeSmallIcon.cx = ::GetSystemMetrics(SM_CXSMICON);
	m_sizeSmallIcon.cy = ::GetSystemMetrics(SM_CYSMICON);

	m_rectVirtual.SetRectEmpty();

	if (!EnumDisplayMonitors(NULL, NULL, InfoEnumProc, (LPARAM) &m_rectVirtual))
	{
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &m_rectVirtual, 0);
	}

	// Get system menu animation type:
	m_bMenuAnimation = FALSE;
	m_bMenuFadeEffect = FALSE;

	if (!bIsRemoteSession)
	{
		::SystemParametersInfo(SPI_GETMENUANIMATION, 0, &m_bMenuAnimation, 0);

		if (m_bMenuAnimation)
		{
			::SystemParametersInfo(SPI_GETMENUFADE, 0, &m_bMenuFadeEffect, 0);
		}
	}

	m_nShellAutohideBars = 0;
	m_bRefreshAutohideBars = TRUE;

	::SystemParametersInfo(SPI_GETMENUUNDERLINES, 0, &m_bSysUnderlineKeyboardShortcuts, 0);
	m_bUnderlineKeyboardShortcuts = m_bSysUnderlineKeyboardShortcuts;

	m_bInSettingChange = FALSE;
}

BOOL AFX_GLOBAL_DATA::SetMenuFont(LPLOGFONT lpLogFont, BOOL bHorz)
{
	ENSURE(lpLogFont != NULL);

	if (bHorz)
	{
		// Create regular font:
		fontRegular.DeleteObject();
		if (!fontRegular.CreateFontIndirect(lpLogFont))
		{
			ASSERT(FALSE);
			return FALSE;
		}

		// Create underline font:
		lpLogFont->lfUnderline = TRUE;
		fontUnderline.DeleteObject();
		fontUnderline.CreateFontIndirect(lpLogFont);
		lpLogFont->lfUnderline = FALSE;

		// Create bold font(used in the default menu items):
		long lSavedWeight = lpLogFont->lfWeight;
		lpLogFont->lfWeight = 700;

		fontBold.DeleteObject();
		BOOL bResult = fontBold.CreateFontIndirect(lpLogFont);

		lpLogFont->lfWeight = lSavedWeight; // Restore weight

		if (!bResult)
		{
			ASSERT(FALSE);
			return FALSE;
		}
	}
	else // Vertical font
	{
		fontVert.DeleteObject();
		if (!fontVert.CreateFontIndirect(lpLogFont))
		{
			ASSERT(FALSE);
			return FALSE;
		}
	}

	UpdateTextMetrics();
	return TRUE;
}

void AFX_GLOBAL_DATA::UpdateTextMetrics()
{
	CWindowDC dc(NULL);

	CFont* pOldFont = dc.SelectObject(&fontRegular);
	ENSURE(pOldFont != NULL);

	TEXTMETRIC tm;
	dc.GetTextMetrics(&tm);

	int nExtra = tm.tmHeight < 15 ? 2 : 5;

	m_nTextHeightHorz = tm.tmHeight + nExtra;
	m_nTextWidthHorz = tm.tmMaxCharWidth + nExtra;

	dc.SelectObject(&fontVert);
	dc.GetTextMetrics(&tm);

	nExtra = tm.tmHeight < 15 ? 2 : 5;

	m_nTextHeightVert = tm.tmHeight + nExtra;
	m_nTextWidthVert = tm.tmMaxCharWidth + nExtra;

	dc.SelectObject(pOldFont);
}

HBITMAP AFX_GLOBAL_DATA::CreateDitherBitmap(HDC hDC)
{
	struct  // BITMAPINFO with 16 colors
	{
		BITMAPINFOHEADER bmiHeader;
		RGBQUAD      bmiColors[16];
	}
	bmi;
	memset(&bmi, 0, sizeof(bmi));

	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 8;
	bmi.bmiHeader.biHeight = 8;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	COLORREF clr = afxGlobalData.clrBtnFace;

	bmi.bmiColors[0].rgbBlue = GetBValue(clr);
	bmi.bmiColors[0].rgbGreen = GetGValue(clr);
	bmi.bmiColors[0].rgbRed = GetRValue(clr);

	clr = ::GetSysColor(COLOR_BTNHIGHLIGHT);
	bmi.bmiColors[1].rgbBlue = GetBValue(clr);
	bmi.bmiColors[1].rgbGreen = GetGValue(clr);
	bmi.bmiColors[1].rgbRed = GetRValue(clr);

	// initialize the brushes
	long patGray[8];
	for (int i = 0; i < 8; i++)
		patGray[i] = (i & 1) ? 0xAAAA5555L : 0x5555AAAAL;

	HBITMAP hbm = CreateDIBitmap(hDC, &bmi.bmiHeader, CBM_INIT, (LPBYTE)patGray, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS);
	return hbm;
}

#if (WINVER >= 0x0601)
ITaskbarList* AFX_GLOBAL_DATA::GetITaskbarList()
{
	HRESULT hr = S_OK;

	if (!bIsWindows7 || !m_bTaskBarInterfacesAvailable)
	{
		return NULL;
	}

	if (m_pTaskbarList != NULL)
	{
		return m_pTaskbarList;
	}

	if (!m_bComInitialized)
	{
		hr = CoInitialize(NULL);
		if (SUCCEEDED(hr))
		{
			m_bComInitialized = TRUE;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pTaskbarList));
	}

	ASSERT(SUCCEEDED(hr));
	return m_pTaskbarList;
}

ITaskbarList3* AFX_GLOBAL_DATA::GetITaskbarList3()
{
	HRESULT hr = S_OK;

	if (!bIsWindows7 || !m_bTaskBarInterfacesAvailable)
	{
		return NULL;
	}

	if (m_pTaskbarList3 != NULL)
	{
		return m_pTaskbarList3;
	}

	if (!m_bComInitialized)
	{
		hr = CoInitialize(NULL);
		if (SUCCEEDED(hr))
		{
			m_bComInitialized = TRUE;
		}
	}

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pTaskbarList3));
	}

	ASSERT(SUCCEEDED(hr));
	return m_pTaskbarList3;
}

void AFX_GLOBAL_DATA::ReleaseTaskBarRefs()
{
	m_bTaskBarInterfacesAvailable = FALSE;

	if (m_pTaskbarList != NULL)
	{
		RELEASE(m_pTaskbarList);
		m_pTaskbarList = NULL;
	}

	if (m_pTaskbarList3 != NULL)
	{
		RELEASE(m_pTaskbarList3);
		m_pTaskbarList3 = NULL;
	}

	if (m_bComInitialized)
	{
		CoUninitialize();
		m_bComInitialized = FALSE;
	}
}
#endif

void AFX_GLOBAL_DATA::CleanUp()
{
	if (brLight.GetSafeHandle())
	{
		brLight.DeleteObject();
	}

	// cleanup fonts:
	fontRegular.DeleteObject();
	fontBold.DeleteObject();
	fontUnderline.DeleteObject();
	fontVert.DeleteObject();
	fontVertCaption.DeleteObject();
	fontTooltip.DeleteObject();

	ReleaseTaskBarRefs();

	m_bEnableAccessibility = FALSE;
}

void ControlBarCleanUp()
{
	afxGlobalData.CleanUp();
}

BOOL AFX_GLOBAL_DATA::DrawParentBackground(CWnd* pWnd, CDC* pDC, LPRECT rectClip)
{
	ASSERT_VALID(pDC);
	ASSERT_VALID(pWnd);

	BOOL bRes = FALSE;

	CRgn rgn;
	if (rectClip != NULL)
	{
		rgn.CreateRectRgnIndirect(rectClip);
		pDC->SelectClipRgn(&rgn);
	}

	CWnd* pParent = pWnd->GetParent();
	ASSERT_VALID(pParent);

	// In Windows XP, we need to call DrawThemeParentBackground function to implement
	// transparent controls
	if (m_pfDrawThemeBackground != NULL)
	{
		bRes = (*m_pfDrawThemeBackground)(pWnd->GetSafeHwnd(), pDC->GetSafeHdc(), rectClip) == S_OK;
	}

	if (!bRes)
	{
		CPoint pt(0, 0);
		pWnd->MapWindowPoints(pParent, &pt, 1);
		pt = pDC->OffsetWindowOrg(pt.x, pt.y);

		bRes = (BOOL) pParent->SendMessage(WM_ERASEBKGND, (WPARAM)pDC->m_hDC);

		pDC->SetWindowOrg(pt.x, pt.y);
	}

	pDC->SelectClipRgn(NULL);

	return bRes;
}

CFrameWnd* AFXGetParentFrame(const CWnd* pWnd)
{
	if (pWnd->GetSafeHwnd() == NULL)
	{
		return NULL;
	}
	ASSERT_VALID(pWnd);

	const CWnd* pParentWnd = pWnd;

	while (pParentWnd != NULL)
	{
		{
			pParentWnd = pParentWnd->GetParent();
		}

		if (pParentWnd == NULL)
		{
			return NULL;
		}

		if (pParentWnd->IsFrameWnd())
		{
			return(CFrameWnd*)pParentWnd;
		}
	}

	return NULL;
}

COLORREF AFX_GLOBAL_DATA::GetColor(int nColor)
{
	switch(nColor)
	{
		case COLOR_BTNFACE:             return clrBtnFace;
		case COLOR_BTNSHADOW:           return clrBtnShadow;
		case COLOR_3DDKSHADOW:          return clrBtnDkShadow;
		case COLOR_3DLIGHT:             return clrBtnLight;
		case COLOR_BTNHIGHLIGHT:        return clrBtnHilite;
		case COLOR_BTNTEXT:             return clrBtnText;
		case COLOR_GRAYTEXT:            return clrGrayedText;
		case COLOR_WINDOWFRAME:         return clrWindowFrame;

		case COLOR_HIGHLIGHT:           return clrHilite;
		case COLOR_HIGHLIGHTTEXT:       return clrTextHilite;

		case COLOR_WINDOW:              return clrWindow;
		case COLOR_WINDOWTEXT:          return clrWindowText;

		case COLOR_CAPTIONTEXT:         return clrCaptionText;
		case COLOR_MENUTEXT:            return clrMenuText;

		case COLOR_ACTIVECAPTION:       return clrActiveCaption;
		case COLOR_INACTIVECAPTION:     return clrInactiveCaption;

		case COLOR_ACTIVEBORDER:        return clrActiveBorder;
		case COLOR_INACTIVEBORDER:      return clrInactiveBorder;

		case COLOR_INACTIVECAPTIONTEXT: return clrInactiveCaptionText;
	}

	return ::GetSysColor(nColor);
}

BOOL AFX_GLOBAL_DATA::SetLayeredAttrib(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags)
{
	return(::SetLayeredWindowAttributes(hwnd, crKey, bAlpha, dwFlags));
}

void AFX_GLOBAL_DATA::EnableAccessibilitySupport(BOOL bEnable/* = TRUE*/)
{
	m_bEnableAccessibility = bEnable;
}

CString AFX_GLOBAL_DATA::RegisterWindowClass(LPCTSTR lpszClassNamePrefix)
{
	ENSURE(lpszClassNamePrefix != NULL);

	// Register a new window class:
	HINSTANCE hInst = AfxGetInstanceHandle();
	UINT uiClassStyle = CS_DBLCLKS;
	HCURSOR hCursor = ::LoadCursor(NULL, IDC_ARROW);
	HBRUSH hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

	CString strClassName;
	strClassName.Format(_T("%s:%x:%x:%x:%x"), lpszClassNamePrefix, (UINT_PTR)hInst, uiClassStyle, (UINT_PTR)hCursor, (UINT_PTR)hbrBackground);

	// See if the class already exists:
	WNDCLASS wndcls;
	if (::GetClassInfo(hInst, strClassName, &wndcls))
	{
		// Already registered, assert everything is good:
		ASSERT(wndcls.style == uiClassStyle);
	}
	else
	{
		// Otherwise we need to register a new class:
		wndcls.style = uiClassStyle;
		wndcls.lpfnWndProc = ::DefWindowProc;
		wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
		wndcls.hInstance = hInst;
		wndcls.hIcon = NULL;
		wndcls.hCursor = hCursor;
		wndcls.hbrBackground = hbrBackground;
		wndcls.lpszMenuName = NULL;
		wndcls.lpszClassName = strClassName;

		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
		}
	}

	return strClassName;
}

BOOL AFX_GLOBAL_DATA::ExcludeTag(CString& strBuffer, LPCTSTR lpszTag, CString& strTag, BOOL bIsCharsList /* = FALSE*/)
{
	const int iBufLen = strBuffer.GetLength();

	CString strTagStart = _T("<");
	strTagStart += lpszTag;
	strTagStart += _T(">");

	const int iTagStartLen = strTagStart.GetLength();

	int iStart = -1;

	int iIndexStart = strBuffer.Find(strTagStart);
	if (iIndexStart < 0)
	{
		return FALSE;
	}

	iStart = iIndexStart + iTagStartLen;

	CString strTagEnd = _T("</");
	strTagEnd += lpszTag;
	strTagEnd += _T('>');

	const int iTagEndLen = strTagEnd.GetLength();

	int iIndexEnd =  -1;
	int nBalanse = 1;
	for (int i = iStart; i < iBufLen - iTagEndLen + 1; i ++)
	{
		if (strBuffer [i] != '<')
		{
			continue;
		}

		if (i < iBufLen - iTagStartLen && _tcsncmp(strBuffer.Mid(i), strTagStart, iTagStartLen) == 0)
		{
			i += iTagStartLen - 1;
			nBalanse ++;
			continue;
		}

		if (_tcsncmp(strBuffer.Mid(i), strTagEnd, iTagEndLen) == 0)
		{
			nBalanse --;
			if (nBalanse == 0)
			{
				iIndexEnd = i;
				break;
			}

			i += iTagEndLen - 1;
		}
	}

	if (iIndexEnd == -1 || iStart > iIndexEnd)
	{
		return FALSE;
	}

	strTag = strBuffer.Mid(iStart, iIndexEnd - iStart);
	strTag.TrimLeft();
	strTag.TrimRight();

	strBuffer.Delete(iIndexStart, iIndexEnd + iTagEndLen - iIndexStart);

	if (bIsCharsList)
	{
		if (strTag.GetLength() > 1 && strTag [0] == _T('\"'))
		{
			strTag = strTag.Mid(1, strTag.GetLength() - 2);
		}

		strTag.Replace(_T("\\t"), _T("\t"));
		strTag.Replace(_T("\\n"), _T("\n"));
		strTag.Replace(_T("\\r"), _T("\r"));
		strTag.Replace(_T("\\b"), _T("\b"));
		strTag.Replace(_T("LT"), _T("<"));
		strTag.Replace(_T("GT"), _T(">"));
		strTag.Replace(_T("AMP"), _T("&"));
	}

	return TRUE;
}

BOOL AFX_GLOBAL_DATA::DwmExtendFrameIntoClientArea(HWND hWnd, AFX_MARGINS* pMargins)
{
	if (m_pfDwmExtendFrameIntoClientArea == NULL)
	{
		return FALSE;
	}

	HRESULT hres = (*m_pfDwmExtendFrameIntoClientArea)(hWnd, pMargins);
	return hres == S_OK;
}

LRESULT AFX_GLOBAL_DATA::DwmDefWindowProc(HWND hWnd, UINT message, WPARAM wp, LPARAM lp)
{
	if (m_pfDwmDefWindowProc == NULL)
	{
		return(LRESULT)-1;
	}

	LRESULT lres = 0;
	(*m_pfDwmDefWindowProc)(hWnd, message, wp, lp, &lres);

	return lres;
}

BOOL AFX_GLOBAL_DATA::DwmIsCompositionEnabled()
{
	if (m_pfDwmIsCompositionEnabled == NULL || bDisableAero)
	{
		return FALSE;
	}

	BOOL bEnabled = FALSE;

	(*m_pfDwmIsCompositionEnabled)(&bEnabled);
	return bEnabled;
}

BOOL AFX_GLOBAL_DATA::DrawTextOnGlass(HTHEME hTheme, CDC* pDC, int iPartId, int iStateId, CString strText, CRect rect, DWORD dwFlags, int nGlowSize, COLORREF clrText)
{
	//---- bits used in dwFlags of DTTOPTS ----
#define AFX_DTT_TEXTCOLOR    (1UL << 0)      // crText has been specified
#define AFX_DTT_BORDERCOLOR  (1UL << 1)      // crBorder has been specified
#define AFX_DTT_SHADOWCOLOR  (1UL << 2)      // crShadow has been specified
#define AFX_DTT_SHADOWTYPE   (1UL << 3)      // iTextShadowType has been specified
#define AFX_DTT_SHADOWOFFSET (1UL << 4)      // ptShadowOffset has been specified
#define AFX_DTT_BORDERSIZE   (1UL << 5)      // nBorderSize has been specified
#define AFX_DTT_FONTPROP     (1UL << 6)      // iFontPropId has been specified
#define AFX_DTT_COLORPROP    (1UL << 7)      // iColorPropId has been specified
#define AFX_DTT_STATEID      (1UL << 8)      // IStateId has been specified
#define AFX_DTT_CALCRECT     (1UL << 9)      // Use pRect as and in/out parameter
#define AFX_DTT_APPLYOVERLAY (1UL << 10)     // fApplyOverlay has been specified
#define AFX_DTT_GLOWSIZE     (1UL << 11)     // iGlowSize has been specified
#define AFX_DTT_CALLBACK     (1UL << 12)     // pfnDrawTextCallback has been specified
#define AFX_DTT_COMPOSITED   (1UL << 13)     // Draws text with antialiased alpha(needs a DIB section)

	if (hTheme == NULL || m_pfDrawThemeTextEx == NULL || !DwmIsCompositionEnabled())
	{
		pDC->DrawText(strText, rect, dwFlags);
		return FALSE;
	}

	CComBSTR bstmp = (LPCTSTR)strText;

	wchar_t* wbuf = new wchar_t[bstmp.Length() + 1];
	wcscpy_s(wbuf, bstmp.Length() + 1, bstmp);

	AFX_DTTOPTS dto;
	memset(&dto, 0, sizeof(AFX_DTTOPTS));
	dto.dwSize = sizeof(AFX_DTTOPTS);
	dto.dwFlags = AFX_DTT_COMPOSITED;

	if (nGlowSize > 0)
	{
		dto.dwFlags |= AFX_DTT_GLOWSIZE;
		dto.iGlowSize = nGlowSize;
	}

	if (clrText != (COLORREF)-1)
	{
		dto.dwFlags |= AFX_DTT_TEXTCOLOR;
		dto.crText = clrText;
	}

	(*m_pfDrawThemeTextEx)(hTheme, pDC->GetSafeHdc(), iPartId, iStateId, wbuf, -1, dwFlags, rect, &dto);

	delete [] wbuf;

	return TRUE;
}

HCURSOR AFX_GLOBAL_DATA::GetHandCursor()
{
	if (m_hcurHand == NULL)
	{
		m_hcurHand = ::LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_HAND));
	}

	return m_hcurHand;
}

BOOL AFX_GLOBAL_DATA::Resume()
{
	{
		m_pfDrawThemeBackground = NULL;
		m_pfDrawThemeTextEx = NULL;
		m_pfBeginBufferedPaint = NULL;
		m_pfEndBufferedPaint = NULL;
	}

	if (m_bEnableAccessibility)
	{
		EnableAccessibilitySupport();
	}

	return TRUE;
}

BOOL AFX_GLOBAL_DATA::GetNonClientMetrics (NONCLIENTMETRICS& info)
{
	struct AFX_OLDNONCLIENTMETRICS
	{
		UINT    cbSize;
		int     iBorderWidth;
		int     iScrollWidth;
		int     iScrollHeight;
		int     iCaptionWidth;
		int     iCaptionHeight;
		LOGFONT lfCaptionFont;
		int     iSmCaptionWidth;
		int     iSmCaptionHeight;
		LOGFONT lfSmCaptionFont;
		int     iMenuWidth;
		int     iMenuHeight;
		LOGFONT lfMenuFont;
		LOGFONT lfStatusFont;
		LOGFONT lfMessageFont;
	};

	info.cbSize = 0;

	return 0;
}


BOOL AFXAPI AfxIsExtendedFrameClass(CWnd* pWnd)
{
	ENSURE( pWnd );
	return FALSE;
}


BOOL AFXAPI AfxIsMFCToolBar(CWnd* pWnd)
{
	ENSURE( pWnd );
	return FALSE;
}

HRESULT AFX_GLOBAL_DATA::ShellCreateItemFromParsingName(PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv)
{
	static HMODULE hShellDll = AfxCtxLoadLibrary(_T("Shell32.dll"));
	ENSURE(hShellDll != NULL);

	typedef	HRESULT (__stdcall *PFNSHCREATEITEMFROMPARSINGNAME)(
		PCWSTR,
		IBindCtx*,
		REFIID,
		void**
		);

	PFNSHCREATEITEMFROMPARSINGNAME pSHCreateItemFromParsingName =
		(PFNSHCREATEITEMFROMPARSINGNAME)GetProcAddress(hShellDll, "SHCreateItemFromParsingName");
	if (pSHCreateItemFromParsingName == NULL)
	{
		return E_FAIL;
	}

	return (*pSHCreateItemFromParsingName)(pszPath, pbc, riid, ppv);
}
