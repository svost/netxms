// ObjectBrowser.cpp : implementation file
//

#include "stdafx.h"
#include "nxcon.h"
#include "ObjectBrowser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Compare two items in object tree hash for qsort()
//

static int CompareTreeHashItems(const void *p1, const void *p2)
{
   return ((OBJ_TREE_HASH *)p1)->dwObjectId < ((OBJ_TREE_HASH *)p2)->dwObjectId ? -1 :
            (((OBJ_TREE_HASH *)p1)->dwObjectId > ((OBJ_TREE_HASH *)p2)->dwObjectId ? 1 : 0);
}


/////////////////////////////////////////////////////////////////////////////
// CObjectBrowser

IMPLEMENT_DYNCREATE(CObjectBrowser, CMDIChildWnd)

CObjectBrowser::CObjectBrowser()
{
   m_dwFlags = 0;
   m_pCurrentObject = NULL;
   m_pImageList = NULL;
   m_dwTreeHashSize = 0;
   m_pTreeHash = NULL;
}

CObjectBrowser::CObjectBrowser(TCHAR *pszParams)
{
   m_dwFlags = 0;
   m_pCurrentObject = NULL;
   m_pImageList = NULL;
   m_dwTreeHashSize = 0;
   m_pTreeHash = NULL;
}

CObjectBrowser::~CObjectBrowser()
{
   delete m_pImageList;
   safe_free(m_pTreeHash);
}


BEGIN_MESSAGE_MAP(CObjectBrowser, CMDIChildWnd)
	//{{AFX_MSG_MAP(CObjectBrowser)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_OBJECT_PROPERTIES, OnObjectProperties)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_PROPERTIES, OnUpdateObjectProperties)
	ON_COMMAND(ID_OBJECT_AGENTCFG, OnObjectAgentcfg)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_AGENTCFG, OnUpdateObjectAgentcfg)
	ON_COMMAND(ID_OBJECT_APPLY, OnObjectApply)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_APPLY, OnUpdateObjectApply)
	ON_COMMAND(ID_OBJECT_BIND, OnObjectBind)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_BIND, OnUpdateObjectBind)
	ON_COMMAND(ID_OBJECT_CHANGEIPADDRESS, OnObjectChangeipaddress)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_CHANGEIPADDRESS, OnUpdateObjectChangeipaddress)
	ON_COMMAND(ID_OBJECT_COMMENTS, OnObjectComments)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_COMMENTS, OnUpdateObjectComments)
	ON_COMMAND(ID_OBJECT_CREATE_CONDITION, OnObjectCreateCondition)
	ON_COMMAND(ID_OBJECT_CREATE_CONTAINER, OnObjectCreateContainer)
	ON_COMMAND(ID_OBJECT_CREATE_NODE, OnObjectCreateNode)
	ON_COMMAND(ID_OBJECT_CREATE_SERVICE, OnObjectCreateService)
	ON_COMMAND(ID_OBJECT_CREATE_TEMPLATE, OnObjectCreateTemplate)
	ON_COMMAND(ID_OBJECT_CREATE_TEMPLATEGROUP, OnObjectCreateTemplategroup)
	ON_COMMAND(ID_OBJECT_CREATE_VPNCONNECTOR, OnObjectCreateVpnconnector)
	ON_COMMAND(ID_OBJECT_DATACOLLECTION, OnObjectDatacollection)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_DATACOLLECTION, OnUpdateObjectDatacollection)
	ON_COMMAND(ID_OBJECT_DELETE, OnObjectDelete)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_DELETE, OnUpdateObjectDelete)
	ON_COMMAND(ID_OBJECT_LASTDCIVALUES, OnObjectLastdcivalues)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_LASTDCIVALUES, OnUpdateObjectLastdcivalues)
	ON_COMMAND(ID_OBJECT_MANAGE, OnObjectManage)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_MANAGE, OnUpdateObjectManage)
	ON_COMMAND(ID_OBJECT_MOVE, OnObjectMove)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_MOVE, OnUpdateObjectMove)
	ON_COMMAND(ID_OBJECT_POLL_CONFIGURATION, OnObjectPollConfiguration)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_POLL_CONFIGURATION, OnUpdateObjectPollConfiguration)
	ON_COMMAND(ID_OBJECT_POLL_STATUS, OnObjectPollStatus)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_POLL_STATUS, OnUpdateObjectPollStatus)
	ON_COMMAND(ID_OBJECT_UNBIND, OnObjectUnbind)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_UNBIND, OnUpdateObjectUnbind)
	ON_COMMAND(ID_OBJECT_UNMANAGE, OnObjectUnmanage)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_UNMANAGE, OnUpdateObjectUnmanage)
	ON_COMMAND(ID_OBJECT_CREATE_CLUSTER, OnObjectCreateCluster)
	//}}AFX_MSG_MAP
   ON_NOTIFY(TVN_SELCHANGED, AFX_IDW_PANE_FIRST, OnTreeViewSelChange)
   ON_NOTIFY(TVN_GETDISPINFO, AFX_IDW_PANE_FIRST, OnTreeViewGetDispInfo)
   ON_NOTIFY(TVN_ITEMEXPANDING, AFX_IDW_PANE_FIRST, OnTreeViewItemExpanding)
   ON_MESSAGE(NXCM_OBJECT_CHANGE, OnObjectChange)
   ON_COMMAND_RANGE(OBJTOOL_MENU_FIRST_ID, OBJTOOL_MENU_LAST_ID, OnObjectTool)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectBrowser message handlers

BOOL CObjectBrowser::PreCreateWindow(CREATESTRUCT& cs) 
{
   if (cs.lpszClass == NULL)
      cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, NULL, NULL, 
                                         AfxGetApp()->LoadIcon(IDI_TREE));
	return CMDIChildWnd::PreCreateWindow(cs);
}


//
// WM_CREATE message handler
//

int CObjectBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
   RECT rect;
   int i;

	if (CMDIChildWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

   GetClientRect(&rect);
	
   // Create splitter
   m_wndSplitter.CreateStatic(this, 1, 2, WS_CHILD | WS_VISIBLE, IDC_SPLITTER);
	
   // Create tree view control
   m_wndTreeCtrl.Create(WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
                        rect, &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 0));

   // Create image list
   m_pImageList = new CImageList;
   m_pImageList->Create(g_pObjectSmallImageList);
   m_nLastObjectImage = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_UP));
   m_pImageList->Add(theApp.LoadIcon(IDI_SORT_DOWN));
   m_nStatusImageBase = m_pImageList->GetImageCount();
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_WARNING));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_MINOR));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_MAJOR));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_CRITICAL));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_UNKNOWN));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_UNMANAGED));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_DISABLED));
   m_pImageList->Add(theApp.LoadIcon(IDI_OVL_STATUS_TESTING));
   for(i = STATUS_WARNING; i <= STATUS_TESTING; i++)
      m_pImageList->SetOverlayImage(m_nStatusImageBase + i - 1, i);
   m_wndTreeCtrl.SetImageList(m_pImageList, TVSIL_NORMAL);

   // Create object view in right pane
   m_wndObjectView.Create(NULL, _T("Object View"), WS_CHILD | WS_VISIBLE, rect,
                          &m_wndSplitter, m_wndSplitter.IdFromRowCol(0, 1));

   // Create panes in splitter
   m_wndSplitter.SetupView(0, 0, CSize(250, 100));
   m_wndSplitter.SetupView(0, 1, CSize(400, 100));
   m_wndSplitter.InitComplete();
   m_wndSplitter.SetWindowPos(NULL, 0, 0, rect.right, rect.bottom, SWP_NOZORDER);

   theApp.OnViewCreate(VIEW_OBJECTS, this);
   PostMessage(WM_COMMAND, ID_VIEW_REFRESH, 0);
	return 0;
}


//
// WM_DESTROY message handler
//

void CObjectBrowser::OnDestroy() 
{
   theApp.OnViewDestroy(VIEW_OBJECTS, this);
	CMDIChildWnd::OnDestroy();
}


//
// WM_SIZE message handler
//

void CObjectBrowser::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
   m_wndSplitter.SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOZORDER);
}


//
// WM_SETFOCUS message handler
//

void CObjectBrowser::OnSetFocus(CWnd* pOldWnd) 
{
	CMDIChildWnd::OnSetFocus(pOldWnd);
   m_wndTreeCtrl.SetFocus();
}


//
// WM_COMMAND::ID_VIEW_REFRESH
//

void CObjectBrowser::OnViewRefresh() 
{
   NXC_OBJECT **ppRootObjects;
   NXC_OBJECT_INDEX *pIndex;
   DWORD i, j, dwNumObjects, dwNumRootObj;
   
   // Select root objects
   NXCLockObjectIndex(g_hSession);
   pIndex = (NXC_OBJECT_INDEX *)NXCGetObjectIndex(g_hSession, &dwNumObjects);
   ppRootObjects = (NXC_OBJECT **)malloc(sizeof(NXC_OBJECT *) * dwNumObjects);
   for(i = 0, dwNumRootObj = 0; i < dwNumObjects; i++)
      if (!pIndex[i].pObject->bIsDeleted)
      {
         // Check if some of the parents are accessible
         for(j = 0; j < pIndex[i].pObject->dwNumParents; j++)
            if (NXCFindObjectByIdNoLock(g_hSession, pIndex[i].pObject->pdwParentList[j]) != NULL)
               break;
         if (j == pIndex[i].pObject->dwNumParents)
         {
            // No accessible parents or no parents at all
            ppRootObjects[dwNumRootObj++] = pIndex[i].pObject;
         }
      }
   NXCUnlockObjectIndex(g_hSession);

   // Populate objects' tree
   m_wndTreeCtrl.DeleteAllItems();
   m_dwTreeHashSize = 0;

   for(i = 0; i < dwNumRootObj; i++)
      AddObjectToTree(ppRootObjects[i], TVI_ROOT);
   safe_free(ppRootObjects);
}


//
// Add new object to tree
//

void CObjectBrowser::AddObjectToTree(NXC_OBJECT *pObject, HTREEITEM hParent)
{
   HTREEITEM hItem;
   TVITEM tvi;
   TCHAR szBuffer[512];
   int nImage;

   // Add object record with class-dependent text
   CreateTreeItemText(pObject, szBuffer);
   nImage = GetObjectImageIndex(pObject);
   hItem = m_wndTreeCtrl.InsertItem(szBuffer, nImage, nImage, hParent);
   m_wndTreeCtrl.SetItemData(hItem, (LPARAM)pObject);
   m_wndTreeCtrl.SetItemState(hItem, INDEXTOOVERLAYMASK(pObject->iStatus), TVIS_OVERLAYMASK);

   // Add to hash
   AddObjectEntryToHash(pObject, hItem);

   // Don't add childs immediatelly to
   // prevent adding millions of items if node has thousands of interfaces in
   // thousands subnets. Childs will be added only if user expands node.
   tvi.mask = TVIF_CHILDREN;
   tvi.hItem = hItem;
   tvi.cChildren = I_CHILDRENCALLBACK;
   m_wndTreeCtrl.SetItem(&tvi);
}


//
// Create class-depemdent text for tree item
//

void CObjectBrowser::CreateTreeItemText(NXC_OBJECT *pObject, TCHAR *pszBuffer)
{
   TCHAR szIpBuffer[32];

   switch(pObject->iClass)
   {
      case OBJECT_SUBNET:
         _stprintf(pszBuffer, _T("%s [Status: %s]"), pObject->szName, g_szStatusText[pObject->iStatus]);
         break;
      case OBJECT_INTERFACE:
         if (pObject->dwIpAddr != 0)
            _stprintf(pszBuffer, _T("%s [IP: %s/%d Status: %s]"), pObject->szName, 
                    IpToStr(pObject->dwIpAddr, szIpBuffer), 
                    BitsInMask(pObject->iface.dwIpNetMask), g_szStatusText[pObject->iStatus]);
         else
            _stprintf(pszBuffer, _T("%s [Status: %s]"), pObject->szName, g_szStatusText[pObject->iStatus]);
         break;
      default:
         _tcscpy(pszBuffer, pObject->szName);
         break;
   }
}


//
// WM_NOTIFY::TVN_SELCHANGED message handler
//

void CObjectBrowser::OnTreeViewSelChange(LPNMTREEVIEW lpnmt, LRESULT *pResult)
{
   m_pCurrentObject = (NXC_OBJECT *)lpnmt->itemNew.lParam;
   m_wndObjectView.SetCurrentObject(m_pCurrentObject);
   *pResult = 0;
}


//
// Handler for TVN_GETDISPINFO notification from tree view control
//

void CObjectBrowser::OnTreeViewGetDispInfo(LPNMTVDISPINFO lpdi, LRESULT *pResult)
{
   NXC_OBJECT *pObject;

   if (lpdi->item.mask == TVIF_CHILDREN)
   {
      pObject = (NXC_OBJECT *)lpdi->item.lParam;
      if (pObject != NULL)
      {
         lpdi->item.cChildren = (pObject->dwNumChilds > 0) ? 1 : 0;
      }
      else
      {
         lpdi->item.cChildren = 0;
      }
   }
   *pResult = 0;
}


//
// Handler for TVN_ITEMEXPANDING notification from tree view control
//

void CObjectBrowser::OnTreeViewItemExpanding(LPNMTREEVIEW lpnmt, LRESULT *pResult)
{
   if ((lpnmt->action == TVE_EXPAND) ||
       (lpnmt->action == TVE_EXPANDPARTIAL) ||
       (lpnmt->action == TVE_TOGGLE))
   {
      NXC_OBJECT *pObject, *pChildObject;
      DWORD i;

      pObject = (NXC_OBJECT *)lpnmt->itemNew.lParam;
      if ((pObject != NULL) && ((m_wndTreeCtrl.GetItemState(lpnmt->itemNew.hItem, TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE) == 0))
      {
         for(i = 0; i < pObject->dwNumChilds; i++)
         {
            pChildObject = NXCFindObjectById(g_hSession, pObject->pdwChildList[i]);
            if (pChildObject != NULL)
               AddObjectToTree(pChildObject, lpnmt->itemNew.hItem);
         }
         SortTreeItems(lpnmt->itemNew.hItem);
      }
   }
   *pResult = 0;
}


//
// Add new object entry to hash
//

void CObjectBrowser::AddObjectEntryToHash(NXC_OBJECT *pObject, HTREEITEM hItem)
{
   DWORD dwIndex, dwEntry;

   dwIndex = FindObjectInTree(pObject->dwId);
   if (dwIndex == INVALID_INDEX)
   {
      m_pTreeHash = (OBJ_TREE_HASH *)realloc(m_pTreeHash, sizeof(OBJ_TREE_HASH) * (m_dwTreeHashSize + 1));
      m_pTreeHash[m_dwTreeHashSize].dwObjectId = pObject->dwId;
      m_pTreeHash[m_dwTreeHashSize].dwNumEntries = 1;
      m_pTreeHash[m_dwTreeHashSize].phTreeItemList = (HTREEITEM *)malloc(sizeof(HTREEITEM));
      m_pTreeHash[m_dwTreeHashSize].phTreeItemList[0] = hItem;
      m_dwTreeHashSize++;
      if ((m_dwTreeHashSize > 1) && 
          (m_pTreeHash[m_dwTreeHashSize - 1].dwObjectId < m_pTreeHash[m_dwTreeHashSize - 2].dwObjectId))
      {
         qsort(m_pTreeHash, m_dwTreeHashSize, sizeof(OBJ_TREE_HASH), CompareTreeHashItems);
      }
   }
   else
   {
      // Object already presented in hash array, just add new HTREEITEM
      dwEntry = m_pTreeHash[dwIndex].dwNumEntries;
      m_pTreeHash[dwIndex].dwNumEntries++;
      m_pTreeHash[dwIndex].phTreeItemList = (HTREEITEM *)realloc(m_pTreeHash[dwIndex].phTreeItemList, sizeof(HTREEITEM)  * m_pTreeHash[dwIndex].dwNumEntries);
      m_pTreeHash[dwIndex].phTreeItemList[dwEntry] = hItem;
   }
}


//
// Adjust object index after new object insertion into tree
//

DWORD CObjectBrowser::AdjustIndex(DWORD dwIndex, DWORD dwObjectId)
{
   if (m_pTreeHash[dwIndex].dwObjectId == dwObjectId)
      return dwIndex;
   if ((dwIndex > 0) && (m_pTreeHash[dwIndex - 1].dwObjectId == dwObjectId))
      return dwIndex - 1;
   else
      return dwIndex + 1;
}


//
// Comparision function for tree items sorting
//

static int CALLBACK CompareTreeItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   TCHAR szName1[MAX_OBJECT_NAME], szName2[MAX_OBJECT_NAME];

   NXCGetComparableObjectNameEx((NXC_OBJECT *)lParam1, szName1);
   NXCGetComparableObjectNameEx((NXC_OBJECT *)lParam2, szName2);
   return _tcsicmp(szName1, szName2);
}


//
// Sort childs of given tree item
//

void CObjectBrowser::SortTreeItems(HTREEITEM hItem)
{
   TVSORTCB tvs;

   tvs.hParent = hItem;
   tvs.lpfnCompare = CompareTreeItems;
   m_wndTreeCtrl.SortChildrenCB(&tvs);
}


//
// Perform binary search on tree hash
// Returns INVALID_INDEX if key not found or position of appropriate network object
// We assume that pHash == NULL will not be passed
//

static DWORD SearchTreeHash(OBJ_TREE_HASH *pBase, DWORD dwHashSize, DWORD dwKey)
{
   DWORD dwFirst, dwLast, dwMid;

   dwFirst = 0;
   dwLast = dwHashSize - 1;

   if ((dwKey < pBase[0].dwObjectId) || (dwKey > pBase[dwLast].dwObjectId))
      return INVALID_INDEX;

   while(dwFirst < dwLast)
   {
      dwMid = (dwFirst + dwLast) / 2;
      if (dwKey == pBase[dwMid].dwObjectId)
         return dwMid;
      if (dwKey < pBase[dwMid].dwObjectId)
         dwLast = dwMid - 1;
      else
         dwFirst = dwMid + 1;
   }

   if (dwKey == pBase[dwLast].dwObjectId)
      return dwLast;

   return INVALID_INDEX;
}


//
// Find object's tree item for give object's id
//

DWORD CObjectBrowser::FindObjectInTree(DWORD dwObjectId)
{
   // First, check for built-in objects
   // If they presented, they will be located in first 10 positions
   if (dwObjectId < 10)
   {
      DWORD i, dwLimit;

      dwLimit = min(m_dwTreeHashSize, dwObjectId);
      for(i = 0; i < dwLimit; i++)
         if (m_pTreeHash[i].dwObjectId == dwObjectId)
         {
            return i;
         }
      return INVALID_INDEX;
   }

   // Second, check two common cases - object is last added, i.e. it
   // will be located in last cell, and object is newly created -
   // then it's id will be greater than last cell value
   if (m_dwTreeHashSize > 0)
   {
      if (m_pTreeHash[m_dwTreeHashSize - 1].dwObjectId == dwObjectId)
         return m_dwTreeHashSize - 1;
      if (m_pTreeHash[m_dwTreeHashSize - 1].dwObjectId < dwObjectId)
         return INVALID_INDEX;
   }

   // Do binary search on a hash array as last resort
   return (m_pTreeHash != NULL) ? SearchTreeHash(m_pTreeHash, m_dwTreeHashSize, dwObjectId) : INVALID_INDEX;
}


//
// WM_OBJECT_CHANGE message handler
// wParam contains object's ID, and lParam pointer to corresponding NXC_OBJECT structure
//

void CObjectBrowser::OnObjectChange(WPARAM wParam, LPARAM lParam)
{
   UpdateObjectTree(wParam, (NXC_OBJECT *)lParam);
}


//
// Update objects' tree when we receive WM_OBJECT_CHANGE message
//

void CObjectBrowser::UpdateObjectTree(DWORD dwObjectId, NXC_OBJECT *pObject)
{
   DWORD i, j, dwIndex;

   // Find object in tree
   dwIndex = FindObjectInTree(dwObjectId);

   if (pObject->bIsDeleted)
   {
      if (dwIndex != INVALID_INDEX)
      {
         HTREEITEM *phItemList;
         DWORD dwNumItems;

         dwNumItems = m_pTreeHash[dwIndex].dwNumEntries;
         phItemList = (HTREEITEM *)nx_memdup(m_pTreeHash[dwIndex].phTreeItemList, sizeof(HTREEITEM) * dwNumItems);
         // Delete all tree items
         for(i = 0; i < dwNumItems; i++)
            DeleteObjectTreeItem(phItemList[i]);
         free(phItemList);
      }
   }
   else
   {
      HTREEITEM hItem;
      NXC_OBJECT *pParent;

      if (dwIndex != INVALID_INDEX)
      {
         TCHAR szBuffer[256];
         DWORD j, *pdwParentList;

         // Create a copy of object's parent list
         pdwParentList = (DWORD *)nx_memdup(pObject->pdwParentList, 
                                            sizeof(DWORD) * pObject->dwNumParents);

         CreateTreeItemText(pObject, szBuffer);
         for(i = 0; i < m_pTreeHash[dwIndex].dwNumEntries; i++)
         {
            // Check if this item's parent still in object's parents list
            hItem = m_wndTreeCtrl.GetParentItem(m_pTreeHash[dwIndex].phTreeItemList[i]);
            if (hItem != NULL)
            {
               pParent = (NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem);
               for(j = 0; j < pObject->dwNumParents; j++)
                  if (pObject->pdwParentList[j] == pParent->dwId)
                  {
                     pdwParentList[j] = 0;   // Mark this parent as presented
                     break;
                  }
               if (j == pObject->dwNumParents)  // Not a parent anymore
               {
                  // Delete this tree item
                  BOOL bStop = (m_pTreeHash[dwIndex].dwNumEntries == 1);
                  DeleteObjectTreeItem(m_pTreeHash[dwIndex].phTreeItemList[i]);
                  if (bStop)
                     break;
                  i--;
               }
               else  // Current tree item is still valid
               {
                  m_wndTreeCtrl.SetItemText(m_pTreeHash[dwIndex].phTreeItemList[i], szBuffer);
                  m_wndTreeCtrl.SetItemState(m_pTreeHash[dwIndex].phTreeItemList[i],
                                    INDEXTOOVERLAYMASK(pObject->iStatus), TVIS_OVERLAYMASK);
                  SortTreeItems(hItem);
               }
            }
            else  // Current tree item has no parent
            {
               m_wndTreeCtrl.SetItemText(m_pTreeHash[dwIndex].phTreeItemList[i], szBuffer);
               m_wndTreeCtrl.SetItemState(m_pTreeHash[dwIndex].phTreeItemList[i],
                                       INDEXTOOVERLAYMASK(pObject->iStatus), TVIS_OVERLAYMASK);
            }
         }

         // Now walk through all object's parents which hasn't corresponding
         // items in tree view
         for(i = 0; i < pObject->dwNumParents; i++)
            if (pdwParentList[i] != 0)
            {
               dwIndex = FindObjectInTree(pdwParentList[i]);
               if (dwIndex != INVALID_INDEX)
               {
                  // Walk through all occurences of current parent object
                  for(j = 0; j < m_pTreeHash[dwIndex].dwNumEntries; j++)
                  {
                     if (m_wndTreeCtrl.GetItemState(m_pTreeHash[dwIndex].phTreeItemList[j], TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
                     {
                        AddObjectToTree(pObject, m_pTreeHash[dwIndex].phTreeItemList[j]);
                        dwIndex = AdjustIndex(dwIndex, pObject->pdwParentList[i]);
                        SortTreeItems(m_pTreeHash[dwIndex].phTreeItemList[j]);
                     }
                  }
               }
            }

         // Destroy copy of object's parents list
         safe_free(pdwParentList);
      }
      else
      {
         // New object, link to all parents
         for(i = 0; i < pObject->dwNumParents; i++)
         {
            dwIndex = FindObjectInTree(pObject->pdwParentList[i]);
            if (dwIndex != INVALID_INDEX)
            {
               // Walk through all occurences of current parent object
               for(j = 0; j < m_pTreeHash[dwIndex].dwNumEntries; j++)
               {
                  if (m_wndTreeCtrl.GetItemState(m_pTreeHash[dwIndex].phTreeItemList[j], TVIS_EXPANDEDONCE) & TVIS_EXPANDEDONCE)
                  {
                     AddObjectToTree(pObject, m_pTreeHash[dwIndex].phTreeItemList[j]);
                     dwIndex = AdjustIndex(dwIndex, pObject->pdwParentList[i]);
                     SortTreeItems(m_pTreeHash[dwIndex].phTreeItemList[j]);
                  }
               }
            }
         }
      }

      if (m_dwFlags & FOLLOW_OBJECT_UPDATES)
      {
         dwIndex = FindObjectInTree(dwObjectId);
         if (dwIndex != INVALID_INDEX)    // Shouldn't happen
            m_wndTreeCtrl.Select(m_pTreeHash[dwIndex].phTreeItemList[0], TVGN_CARET);
      }
      else
      {
         // Check if current item has been changed
         hItem = m_wndTreeCtrl.GetSelectedItem();
         if (hItem != NULL)
         {
            if (((NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem))->dwId == dwObjectId)
               m_wndObjectView.Refresh();
         }
      }
   }
}


//
// Delete tree item and appropriate record in hash
//

void CObjectBrowser::DeleteObjectTreeItem(HTREEITEM hRootItem)
{
   HTREEITEM hItem;
   DWORD i, dwIndex;
   NXC_OBJECT *pObject;

   // Delete all child items
   hItem = m_wndTreeCtrl.GetNextItem(hRootItem, TVGN_CHILD);
   while(hItem != NULL)
   {
      DeleteObjectTreeItem(hItem);
      hItem = m_wndTreeCtrl.GetNextItem(hRootItem, TVGN_CHILD);
   }

   // Find hash record for current item
   pObject = (NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hRootItem);
   dwIndex = FindObjectInTree(pObject->dwId);
   if (dwIndex != INVALID_INDEX)
   {
      for(i = 0; i < m_pTreeHash[dwIndex].dwNumEntries; i++)
         if (m_pTreeHash[dwIndex].phTreeItemList[i] == hRootItem)
         {
            if (m_pTreeHash[dwIndex].dwNumEntries == 1)
            {
               // Last entry, delete entire record in tree hash list
               free(m_pTreeHash[dwIndex].phTreeItemList);
               m_dwTreeHashSize--;
               if (dwIndex < m_dwTreeHashSize)
                  memmove(&m_pTreeHash[dwIndex], &m_pTreeHash[dwIndex + 1], 
                          sizeof(OBJ_TREE_HASH) * (m_dwTreeHashSize - dwIndex));
            }
            else
            {
               m_pTreeHash[dwIndex].dwNumEntries--;
               memmove(&m_pTreeHash[dwIndex].phTreeItemList[i],
                       &m_pTreeHash[dwIndex].phTreeItemList[i + 1],
                       sizeof(HTREEITEM) * (m_pTreeHash[dwIndex].dwNumEntries - i));
            }
            break;
         }
   }

   // Delete item from tree control
   m_wndTreeCtrl.DeleteItem(hRootItem);
}


//
// WM_CONTEXTMENU message handler
//

void CObjectBrowser::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CMenu *pMenu, *pToolsMenu;
   CPoint pt;
   DWORD dwTemp;
   HTREEITEM hItem;
   UINT uFlags;
   CWnd *pWndPane;
   BOOL bMenuInserted = FALSE;

   pt = point;
   pWnd->ScreenToClient(&pt);
   pWndPane = pWnd->ChildWindowFromPoint(pt, CWP_SKIPINVISIBLE);
   if (pWndPane->GetDlgCtrlID() != AFX_IDW_PANE_FIRST)
      return;

   pt = point;
   pWndPane->ScreenToClient(&pt);

   hItem = m_wndTreeCtrl.HitTest(pt, &uFlags);
   if ((hItem != NULL) && (uFlags & TVHT_ONITEM))
   {
      m_wndTreeCtrl.Select(hItem, TVGN_CARET);

      pMenu = theApp.GetContextMenu(1);
      dwTemp = 0;
      pToolsMenu = CreateToolsSubmenu(m_pCurrentObject, _T(""), &dwTemp);
      if (pToolsMenu->GetMenuItemCount() > 0)
      {
         pMenu->InsertMenu(14, MF_BYPOSITION | MF_STRING | MF_POPUP,
                           (UINT)pToolsMenu->GetSafeHmenu(), _T("&Tools"));
         pToolsMenu->Detach();
         bMenuInserted = TRUE;
      }
      delete pToolsMenu;
      pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this, NULL);
      if (bMenuInserted)
      {
         pMenu->DeleteMenu(14, MF_BYPOSITION);
      }
   }
}


//
// Get ID of currently selected object
// Will return 0 if there are no selection
//

DWORD CObjectBrowser::GetSelectedObject(void)
{
   HTREEITEM hItem;

   hItem = m_wndTreeCtrl.GetSelectedItem();
   return (hItem != NULL) ? ((NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem))->dwId : 0;
}


//
// Returns TRUE if currently selected object is node
//

BOOL CObjectBrowser::CurrObjectIsNode(BOOL bIncludeTemplates)
{
   if (m_pCurrentObject == NULL)
   {
      return FALSE;
   }
   else
   {
      return bIncludeTemplates ? 
         ((m_pCurrentObject->iClass == OBJECT_NODE) || 
          (m_pCurrentObject->iClass == OBJECT_TEMPLATE)) :
         (m_pCurrentObject->iClass == OBJECT_NODE);
   }
}


//
// Returns TRUE if currently selected object is interface
//

BOOL CObjectBrowser::CurrObjectIsInterface(void)
{
   if (m_pCurrentObject == NULL)
   {
      return FALSE;
   }
   else
   {
      return m_pCurrentObject->iClass == OBJECT_INTERFACE;
   }
}


//
// Handler for "Properties" menu
//

void CObjectBrowser::OnObjectProperties(void) 
{
   DWORD dwId;

   dwId = GetSelectedObject();
   if (dwId != 0)
      theApp.ObjectProperties(dwId);
}

void CObjectBrowser::OnUpdateObjectProperties(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}


//
// Handler for "Edit agent config" menu
//

void CObjectBrowser::OnObjectAgentcfg(void) 
{
   if (m_pCurrentObject != NULL)
      theApp.EditAgentConfig(m_pCurrentObject);
}

void CObjectBrowser::OnUpdateObjectAgentcfg(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(FALSE));
}


//
// Handler for "Apply" menu
//

void CObjectBrowser::OnObjectApply() 
{
   if (m_pCurrentObject != NULL)
      theApp.ApplyTemplate(m_pCurrentObject);
}

void CObjectBrowser::OnUpdateObjectApply(CCmdUI* pCmdUI) 
{
   if (m_pCurrentObject == NULL)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      pCmdUI->Enable(m_pCurrentObject->iClass == OBJECT_TEMPLATE);
   }
}


//
// Handler for "Bind" menu
//

void CObjectBrowser::OnObjectBind() 
{
   if (m_pCurrentObject != NULL)
      theApp.BindObject(m_pCurrentObject);
}

void CObjectBrowser::OnUpdateObjectBind(CCmdUI* pCmdUI) 
{
   if (m_pCurrentObject == NULL)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      pCmdUI->Enable((m_pCurrentObject->iClass == OBJECT_CONTAINER) ||
                     (m_pCurrentObject->iClass == OBJECT_SERVICEROOT));
   }
}


//
// Handler for "Change IP address" menu
//

void CObjectBrowser::OnObjectChangeipaddress() 
{
   if (m_pCurrentObject != NULL)
      theApp.ChangeNodeAddress(m_pCurrentObject->dwId);
}

void CObjectBrowser::OnUpdateObjectChangeipaddress(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(FALSE));
}


//
// Handler for "Comments" menu
//

void CObjectBrowser::OnObjectComments() 
{
   if (m_pCurrentObject != NULL)
      theApp.ShowObjectComments(m_pCurrentObject);
}

void CObjectBrowser::OnUpdateObjectComments(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}


//
// Handler for "Create->Condition" menu
//

void CObjectBrowser::OnObjectCreateCondition() 
{
   theApp.CreateCondition((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// Handler for "Create->Container" menu
//

void CObjectBrowser::OnObjectCreateContainer() 
{
   theApp.CreateContainer((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// Handler for "Create->Node" menu
//

void CObjectBrowser::OnObjectCreateNode() 
{
   theApp.CreateNode((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// Handler for "Create->Cluster" menu
//

void CObjectBrowser::OnObjectCreateCluster() 
{
   theApp.CreateCluster((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// Handler for "Create->Service" menu
//

void CObjectBrowser::OnObjectCreateService() 
{
   theApp.CreateNetworkService((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// Handler for "Create->Template" menu
//

void CObjectBrowser::OnObjectCreateTemplate() 
{
   theApp.CreateTemplate((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// Handler for "Create->Template group" menu
//

void CObjectBrowser::OnObjectCreateTemplategroup() 
{
   theApp.CreateTemplateGroup((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// Handler for "Create->VPN connector" menu
//

void CObjectBrowser::OnObjectCreateVpnconnector() 
{
   theApp.CreateVPNConnector((m_pCurrentObject != NULL) ? m_pCurrentObject->dwId : 0);
}


//
// Handler for "Data collection" menu
//

void CObjectBrowser::OnObjectDatacollection() 
{
   if (m_pCurrentObject != NULL)
      theApp.StartObjectDCEditor(m_pCurrentObject);
}

void CObjectBrowser::OnUpdateObjectDatacollection(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(TRUE));
}


//
// Handler for "Delete" menu
//

void CObjectBrowser::OnObjectDelete() 
{
   BOOL bOK;

   if (m_pCurrentObject != NULL)
   {
      if (g_dwOptions & UI_OPT_CONFIRM_OBJ_DELETE)
      {
         TCHAR szBuffer[256];

         _sntprintf(szBuffer, 256, _T("Do you really want to delete object %s?"), m_pCurrentObject->szName);
         bOK = (MessageBox(szBuffer, _T("Confirmation"), MB_YESNO | MB_ICONQUESTION) == IDYES);
      }
      else
      {
         bOK = TRUE;
      }
      if (bOK)
         theApp.DeleteNetXMSObject(m_pCurrentObject);
   }
}

void CObjectBrowser::OnUpdateObjectDelete(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}


//
// Handler for "Last DCI values" menu
//

void CObjectBrowser::OnObjectLastdcivalues() 
{
   if (m_pCurrentObject != NULL)
      theApp.ShowLastValues(m_pCurrentObject);
}

void CObjectBrowser::OnUpdateObjectLastdcivalues(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(FALSE));
}


//
// Handler for "Manage" menu
//

void CObjectBrowser::OnObjectManage() 
{
   if (m_pCurrentObject != NULL)
      theApp.SetObjectMgmtStatus(m_pCurrentObject, TRUE);
}

void CObjectBrowser::OnUpdateObjectManage(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}


//
// Handler for "Move" menu
//

void CObjectBrowser::OnObjectMove() 
{
   HTREEITEM hItem;

   if (m_pCurrentObject != NULL)
   {
      hItem = m_wndTreeCtrl.GetParentItem(m_wndTreeCtrl.GetSelectedItem());
      if (hItem != NULL)
      {
         theApp.MoveObject(m_pCurrentObject->dwId, 
                           ((NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem))->dwId);
      }
   }
}

void CObjectBrowser::OnUpdateObjectMove(CCmdUI* pCmdUI) 
{
   BOOL bEnable = FALSE;

   if (m_pCurrentObject != NULL)
   {
      HTREEITEM hItem;

      hItem = m_wndTreeCtrl.GetParentItem(m_wndTreeCtrl.GetSelectedItem());
      if (hItem != NULL)
      {
         NXC_OBJECT *pObject;

         pObject = (NXC_OBJECT *)m_wndTreeCtrl.GetItemData(hItem);
         if (pObject != NULL)
         {
            if ((pObject->iClass == OBJECT_CONTAINER) ||
                (pObject->iClass == OBJECT_SERVICEROOT))
               bEnable = TRUE;
         }
      }
   }
   pCmdUI->Enable(bEnable);
}


//
// Handler for "Poll->Configuration" menu
//

void CObjectBrowser::OnObjectPollConfiguration() 
{
   if (m_pCurrentObject != NULL)
      theApp.PollNode(m_pCurrentObject->dwId, POLL_CONFIGURATION);
}

void CObjectBrowser::OnUpdateObjectPollConfiguration(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(FALSE));
}


//
// Handler for "Poll->Status" menu
//

void CObjectBrowser::OnObjectPollStatus() 
{
   if (m_pCurrentObject != NULL)
      theApp.PollNode(m_pCurrentObject->dwId, POLL_STATUS);
}

void CObjectBrowser::OnUpdateObjectPollStatus(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(CurrObjectIsNode(FALSE));
}


//
// Handler for "Unbind" menu
//

void CObjectBrowser::OnObjectUnbind() 
{
   if (m_pCurrentObject != NULL)
      theApp.UnbindObject(m_pCurrentObject);
}

void CObjectBrowser::OnUpdateObjectUnbind(CCmdUI* pCmdUI) 
{
   if (m_pCurrentObject == NULL)
   {
      pCmdUI->Enable(FALSE);
   }
   else
   {
      pCmdUI->Enable((m_pCurrentObject->iClass == OBJECT_CONTAINER) ||
                     (m_pCurrentObject->iClass == OBJECT_SERVICEROOT) ||
                     (m_pCurrentObject->iClass == OBJECT_TEMPLATE));
   }
}


//
// Handler for "Unmanage" menu
//

void CObjectBrowser::OnObjectUnmanage() 
{
   if (m_pCurrentObject != NULL)
      theApp.SetObjectMgmtStatus(m_pCurrentObject, FALSE);
}

void CObjectBrowser::OnUpdateObjectUnmanage(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable(m_pCurrentObject != NULL);
}


//
// Overrided OnCmdMsg which will route all commands
// through active tab in CObjectView first
//

BOOL CObjectBrowser::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
   if (::IsWindow(m_wndObjectView.m_hWnd))
      if (m_wndObjectView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
         return TRUE;
	
	return CMDIChildWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


//
// Process alarm updates
//

void CObjectBrowser::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   m_wndObjectView.OnAlarmUpdate(dwCode, pAlarm);
}


//
// Handler for object tools
//

void CObjectBrowser::OnObjectTool(UINT nID)
{
   if (m_pCurrentObject != NULL)
      theApp.ExecuteObjectTool(m_pCurrentObject, nID - OBJTOOL_MENU_FIRST_ID);
}
