// SyslogBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "SyslogBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSyslogBrowser

IMPLEMENT_DYNCREATE(CSyslogBrowser, CMDIChildWnd)

CSyslogBrowser::CSyslogBrowser()
{
   m_pImageList = NULL;
   m_bIsBusy = FALSE;
}

CSyslogBrowser::~CSyslogBrowser()
{
   delete m_pImageList;
}


BEGIN_MESSAGE_MAP(CSyslogBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CSyslogBrowser)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	//}}AFX_MSG_MAP
   ON_COMMAND(ID_REQUEST_COMPLETED, OnRequestCompleted)
   ON_MESSAGE(NXCM_GET_SAVE_INFO, OnGetSaveInfo)
   ON_MESSAGE(NXCM_SYSLOG_RECORD, OnSyslogRecord)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSyslogBrowser message handlers

BOOL CSyslogBrowser::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, 
                                         NULL, 
                                         GetSysColorBrush(COLOR_WINDOW), 
                                         AfxGetApp()->LoadIcon(IDI_LOG));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CSyslogBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   DWORD dwResult;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   // Create list view control
   GetClientRect(&rect);
   m_wndListCtrl.Create(WS_CHILD | WS_VISIBLE | LVS_REPORT, rect, this, ID_LIST_VIEW);
   m_wndListCtrl.SetExtendedStyle(LVS_EX_TRACKSELECT | LVS_EX_UNDERLINEHOT | 
                                  LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
   m_wndListCtrl.SetHoverTime(0x7FFFFFFF);

   // Create image list
   m_pImageList = CreateEventImageList();
   m_wndListCtrl.SetImageList(m_pImageList, LVSIL_SMALL);

   // Setup columns
   m_wndListCtrl.InsertColumn(0, _T("Time"), LVCFMT_LEFT, 135);
   m_wndListCtrl.InsertColumn(1, _T("Severity"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(2, _T("Facility"), LVCFMT_LEFT, 80);
   m_wndListCtrl.InsertColumn(3, _T("Hostname"), LVCFMT_LEFT, 100);
   m_wndListCtrl.InsertColumn(4, _T("Tag"), LVCFMT_LEFT, 90);
   m_wndListCtrl.InsertColumn(5, _T("Message"), LVCFMT_LEFT, 500);
	
   // Create wait view
   m_wndWaitView.SetText(_T("Loading syslog..."));
   m_wndWaitView.Create(NULL, NULL, WS_CHILD, rect, this, ID_WAIT_VIEW);

   theApp.OnViewCreate(VIEW_SYSLOG, this);
   dwResult = DoRequestArg2(NXCSubscribe, g_hSession,
                            (void *)NXC_CHANNEL_SYSLOG,
                            _T("Subscribing to SYSLOG channel..."));
   if (dwResult != RCC_SUCCESS)
      theApp.ErrorBox(dwResult, _T("Cannot subscribe to SYSLOG channel: %s"));

   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CSyslogBrowser::OnDestroy() 
{
   DoRequestArg2(NXCUnsubscribe, g_hSession, (void *)NXC_CHANNEL_SYSLOG,
                 _T("Unsubscribing from SYSLOG channel..."));
   theApp.OnViewDestroy(VIEW_SYSLOG, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CSyslogBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
   m_wndListCtrl.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
   m_wndWaitView.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CSyslogBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
	
   if (m_bIsBusy)
      m_wndWaitView.SetFocus();
   else
      m_wndListCtrl.SetFocus();
}


//
// Message loading thread
//

static THREAD_RESULT THREAD_CALL LoadMessages(void *pArg)
{
   NXCSyncSyslog(g_hSession, g_dwMaxLogRecords);
   PostMessage((HWND)pArg, WM_COMMAND, ID_REQUEST_COMPLETED, 0);
	return THREAD_OK;
}


//
// WM_COMMAND::ID_VIEW_REFRESH message handler
//

void CSyslogBrowser::OnViewRefresh() 
{
   if (!m_bIsBusy)
   {
      m_bIsBusy = TRUE;
      m_wndWaitView.ShowWindow(SW_SHOW);
      m_wndListCtrl.ShowWindow(SW_HIDE);
      m_wndWaitView.Start();
      m_wndListCtrl.DeleteAllItems();
      ThreadCreate(LoadMessages, 0, m_hWnd);
   }
}


//
// WM_REQUEST_COMPLETED message handler
//

void CSyslogBrowser::OnRequestCompleted(void)
{
   if (m_bIsBusy)
   {
      m_bIsBusy = FALSE;
      m_wndListCtrl.ShowWindow(SW_SHOW);
      m_wndWaitView.ShowWindow(SW_HIDE);
      m_wndWaitView.Stop();
   }
}


//
// Get save info for desktop saving
//

LRESULT CSyslogBrowser::OnGetSaveInfo(WPARAM wParam, LPARAM lParam)
{
	WINDOW_SAVE_INFO *pInfo = (WINDOW_SAVE_INFO *)lParam;
   pInfo->iWndClass = WNDC_SYSLOG_BROWSER;
   GetWindowPlacement(&pInfo->placement);
   pInfo->szParameters[0] = 0;
   return 1;
}


//
// WM_SYSLOG_RECORD message handler
//

LRESULT CSyslogBrowser::OnSyslogRecord(WPARAM wParam, LPARAM lParam)
{
   AddRecord((NXC_SYSLOG_RECORD *)lParam, wParam == RECORD_ORDER_NORMAL);
   safe_free(((NXC_SYSLOG_RECORD *)lParam)->pszText);
   free((NXC_SYSLOG_RECORD *)lParam);
	return 0;
}


//
// Add new record to list
//

void CSyslogBrowser::AddRecord(NXC_SYSLOG_RECORD *pRec, BOOL bAppend)
{
   int iIdx;
   TCHAR szBuffer[64];
   static int nImage[8] = { 4, 4, 3, 3, 2, 1, 0, 0 };

   FormatTimeStamp(pRec->dwTimeStamp, szBuffer, TS_LONG_DATE_TIME);
   iIdx = m_wndListCtrl.InsertItem(bAppend ? 0x7FFFFFFF : 0, szBuffer, nImage[pRec->wSeverity]);
   if (iIdx != -1)
   {
      m_wndListCtrl.SetItemText(iIdx, 1, g_szSyslogSeverity[pRec->wSeverity]);
      if (pRec->wFacility <= 23)
      {
         m_wndListCtrl.SetItemText(iIdx, 2, g_szSyslogFacility[pRec->wFacility]);
      }
      else
      {
         _sntprintf_s(szBuffer, 64, _TRUNCATE, _T("<%d>"), pRec->wFacility);
         m_wndListCtrl.SetItemText(iIdx, 2, szBuffer);
      }
      m_wndListCtrl.SetItemText(iIdx, 3, pRec->szHost);
      m_wndListCtrl.SetItemText(iIdx, 4, pRec->szTag);
      m_wndListCtrl.SetItemText(iIdx, 5, pRec->pszText);

      if (bAppend)
         m_wndListCtrl.EnsureVisible(iIdx, FALSE);
   }
}