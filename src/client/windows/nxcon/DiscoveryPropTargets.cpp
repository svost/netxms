// DiscoveryPropTargets.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "DiscoveryPropTargets.h"
#include "AddrEntryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropTargets property page

IMPLEMENT_DYNCREATE(CDiscoveryPropTargets, CPropertyPage)

CDiscoveryPropTargets::CDiscoveryPropTargets() : CPropertyPage(CDiscoveryPropTargets::IDD)
{
	//{{AFX_DATA_INIT(CDiscoveryPropTargets)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   
   m_pAddrList = NULL;
   m_dwAddrCount = 0;
}

CDiscoveryPropTargets::~CDiscoveryPropTargets()
{
}

void CDiscoveryPropTargets::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiscoveryPropTargets)
	DDX_Control(pDX, IDC_LIST_TARGETS, m_wndListCtrl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiscoveryPropTargets, CPropertyPage)
	//{{AFX_MSG_MAP(CDiscoveryPropTargets)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiscoveryPropTargets message handlers

BOOL CDiscoveryPropTargets::OnInitDialog() 
{
   RECT rect;
   DWORD i;

	CPropertyPage::OnInitDialog();

   m_wndListCtrl.GetClientRect(&rect);
   m_wndListCtrl.InsertColumn(0, _T("Address"), LVCFMT_LEFT,
                              rect.right - GetSystemMetrics(SM_CXVSCROLL));
	m_wndListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

   for(i = 0; i < m_dwAddrCount; i++)
   {
      AddRecordToList(i, &m_pAddrList[i]);
   }
	
	return TRUE;
}


//
// WM_DESTROY message handler
//

void CDiscoveryPropTargets::OnDestroy() 
{
   int i, nCount;

   nCount = m_wndListCtrl.GetItemCount();
   for(i = 0; i < nCount; i++)
      safe_free((void *)m_wndListCtrl.GetItemData(i));
	CPropertyPage::OnDestroy();
}


//
// Add address record to list
//

void CDiscoveryPropTargets::AddRecordToList(int nItem, NXC_ADDR_ENTRY *pAddr)
{
   TCHAR szBuffer[256], szAddr1[16], szAddr2[16];

   _sntprintf_s(szBuffer, 256, _TRUNCATE, _T("%s%s%s"), IpToStr(pAddr->dwAddr1, szAddr1),
                pAddr->dwType == 0 ? _T("/") : _T(" - "),
                IpToStr(pAddr->dwAddr2, szAddr2));
   m_wndListCtrl.InsertItem(nItem, szBuffer);
   m_wndListCtrl.SetItemData(nItem, (LPARAM)nx_memdup(pAddr, sizeof(NXC_ADDR_ENTRY)));
}


//
// "Add" button handler
//

void CDiscoveryPropTargets::OnButtonAdd() 
{
   CAddrEntryDlg dlg;
   NXC_ADDR_ENTRY addr;

   dlg.m_nType = 0;
   dlg.m_dwAddr1 = 0;
   dlg.m_dwAddr2 = 0;
   if (dlg.DoModal() == IDOK)
   {
      addr.dwType = dlg.m_nType;
      addr.dwAddr1 = dlg.m_dwAddr1;
      addr.dwAddr2 = dlg.m_dwAddr2;
      AddRecordToList(m_wndListCtrl.GetItemCount(), &addr);
   }
}


//
// "OK" button handler
//

void CDiscoveryPropTargets::OnOK() 
{
   DWORD i;

   m_dwAddrCount = m_wndListCtrl.GetItemCount();
   m_pAddrList = (NXC_ADDR_ENTRY *)realloc(m_pAddrList, sizeof(NXC_ADDR_ENTRY) * m_dwAddrCount);
   for(i = 0; i < m_dwAddrCount; i++)
      memcpy(&m_pAddrList[i], (void *)m_wndListCtrl.GetItemData(i), sizeof(NXC_ADDR_ENTRY));
	CPropertyPage::OnOK();
}


//
// "Delete" button handler
//

void CDiscoveryPropTargets::OnButtonDelete() 
{
   int nItem;

   nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   while(nItem != -1)
   {
      safe_free((void *)m_wndListCtrl.GetItemData(nItem));
      m_wndListCtrl.DeleteItem(nItem);
      nItem = m_wndListCtrl.GetNextItem(-1, LVIS_SELECTED);
   }
}