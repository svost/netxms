#if !defined(AFX_MAPMANAGER_H__595B2A63_F589_4DB1_AAC2_D4BB895DEC0C__INCLUDED_)
#define AFX_MAPMANAGER_H__595B2A63_F589_4DB1_AAC2_D4BB895DEC0C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapManager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapManager frame

class CMapManager : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CMapManager)
protected:
	CMapManager();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapManager)
	//}}AFX_VIRTUAL

// Implementation
protected:
	CListCtrl m_wndListCtrl;
	virtual ~CMapManager();

	// Generated message map functions
	//{{AFX_MSG(CMapManager)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPMANAGER_H__595B2A63_F589_4DB1_AAC2_D4BB895DEC0C__INCLUDED_)
