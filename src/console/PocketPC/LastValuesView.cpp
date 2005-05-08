// LastValuesView.cpp : implementation file
//

#include "stdafx.h"
#include "nxpc.h"
#include "LastValuesView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLastValuesView

CLastValuesView::CLastValuesView()
{
   m_dwNodeId = 0;
}

CLastValuesView::~CLastValuesView()
{
}


BEGIN_MESSAGE_MAP(CLastValuesView, CDynamicView)
	//{{AFX_MSG_MAP(CLastValuesView)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// Get view's fingerprint
// Should be overriden in child classes.
//

QWORD CLastValuesView::GetFingerprint()
{
   return CREATE_VIEW_FINGERPRINT(VIEW_CLASS_LAST_VALUES, 0, m_dwNodeId);
}


//
// Initialize view. Called after view object creation, but
// before WM_CREATE. Data is a pointer to object.
//

void CLastValuesView::InitView(void *pData)
{
   m_dwNodeId = ((NXC_OBJECT *)pData)->dwId;
}


/////////////////////////////////////////////////////////////////////////////
// CLastValuesView message handlers


//
// WM_CREATE message handler
//

int CLastValuesView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;

	if (CDynamicView::OnCreate(lpCreateStruct) == -1)
		return -1;

   // Create list control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_CTRL);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);
   m_wndListCtrl.InsertColumn(0, L"ID", LVCFMT_LEFT, 30);
   m_wndListCtrl.InsertColumn(1, L"Description", LVCFMT_LEFT, 130);
   m_wndListCtrl.InsertColumn(2, L"Value", LVCFMT_LEFT, 70);
   m_wndListCtrl.InsertColumn(3, L"Timestamp", LVCFMT_LEFT, 124);
	
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH);
	
	return 0;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CLastValuesView::OnViewRefresh() 
{
   DWORD i, dwResult, dwNumItems;
   NXC_DCI_VALUE *pItemList;
   int iItem;
   TCHAR szBuffer[256];
   struct tm *ptm;

   m_wndListCtrl.DeleteAllItems();
   dwResult = DoRequestArg4(NXCGetLastValues, g_hSession, (void *)m_dwNodeId,
                            &dwNumItems, &pItemList, _T("Loading last DCI values..."));
   if (dwResult == RCC_SUCCESS)
   {
      for(i = 0; i < dwNumItems; i++)
      {
         _sntprintf(szBuffer, 256, _T("%ld"), pItemList[i].dwId);
         iItem = m_wndListCtrl.InsertItem(0x7FFFFFFF, szBuffer);
         if (iItem != -1)
         {
            m_wndListCtrl.SetItemData(iItem, pItemList[i].dwId);
            m_wndListCtrl.SetItemText(iItem, 1, pItemList[i].szDescription);
            m_wndListCtrl.SetItemText(iItem, 2, pItemList[i].szValue);

            // Create timestamp
            ptm = WCE_FCTN(localtime)((time_t *)&pItemList[i].dwTimestamp);
            _stprintf(szBuffer, _T("%02d-%02d-%04d %02d:%02d:%02d"), ptm->tm_mday,
                      ptm->tm_mon, ptm->tm_year, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
            m_wndListCtrl.SetItemText(iItem, 3, szBuffer);
         }
      }
   }
   else
   {
      theApp.ErrorBox(dwResult, _T("Error loading last DCI values: %s"));
   }
}


//
// WM_SIZE message handler
//

void CLastValuesView::OnSize(UINT nType, int cx, int cy) 
{
	CDynamicView::OnSize(nType, cx, cy);
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}

