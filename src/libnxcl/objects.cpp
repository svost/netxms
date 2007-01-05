/* 
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: objects.cpp
**
**/

#include "libnxcl.h"


//
// Destroy object
//

void DestroyObject(NXC_OBJECT *pObject)
{
   DebugPrintf(_T("DestroyObject(id:%d, name:\"%s\")"), pObject->dwId, pObject->szName);
   switch(pObject->iClass)
   {
      case OBJECT_NETWORKSERVICE:
         safe_free(pObject->netsrv.pszRequest);
         safe_free(pObject->netsrv.pszResponse);
         break;
      case OBJECT_ZONE:
         safe_free(pObject->zone.pdwAddrList);
         break;
      case OBJECT_VPNCONNECTOR:
         safe_free(pObject->vpnc.pLocalNetList);
         safe_free(pObject->vpnc.pRemoteNetList);
         break;
      case OBJECT_CONDITION:
         safe_free(pObject->cond.pszScript);
         safe_free(pObject->cond.pDCIList);
         break;
      case OBJECT_CLUSTER:
         safe_free(pObject->cluster.pSyncNetList);
         break;
   }
   safe_free(pObject->pdwChildList);
   safe_free(pObject->pdwParentList);
   safe_free(pObject->pAccessList);
   safe_free(pObject->pszComments);
   free(pObject);
}


//
// Perform binary search on index
// Returns INVALID_INDEX if key not found or position of appropriate network object
// We assume that pIndex == NULL will not be passed
//

static DWORD SearchIndex(INDEX *pIndex, DWORD dwIndexSize, DWORD dwKey)
{
   DWORD dwFirst, dwLast, dwMid;

   if (dwIndexSize == 0)
      return INVALID_INDEX;

   dwFirst = 0;
   dwLast = dwIndexSize - 1;

   if ((dwKey < pIndex[0].dwKey) || (dwKey > pIndex[dwLast].dwKey))
      return INVALID_INDEX;

   while(dwFirst < dwLast)
   {
      dwMid = (dwFirst + dwLast) / 2;
      if (dwKey == pIndex[dwMid].dwKey)
         return dwMid;
      if (dwKey < pIndex[dwMid].dwKey)
         dwLast = dwMid - 1;
      else
         dwFirst = dwMid + 1;
   }

   if (dwKey == pIndex[dwLast].dwKey)
      return dwLast;

   return INVALID_INDEX;
}


//
// Index comparision callback for qsort()
//

static int IndexCompare(const void *pArg1, const void *pArg2)
{
   return ((INDEX *)pArg1)->dwKey < ((INDEX *)pArg2)->dwKey ? -1 :
            (((INDEX *)pArg1)->dwKey > ((INDEX *)pArg2)->dwKey ? 1 : 0);
}


//
// Add object to list
//

void NXCL_Session::AddObject(NXC_OBJECT *pObject, BOOL bSortIndex)
{
   DebugPrintf(_T("AddObject(id:%d, name:\"%s\")"), pObject->dwId, pObject->szName);
   LockObjectIndex();
   m_pIndexById = (INDEX *)realloc(m_pIndexById, sizeof(INDEX) * (m_dwNumObjects + 1));
   m_pIndexById[m_dwNumObjects].dwKey = pObject->dwId;
   m_pIndexById[m_dwNumObjects].pObject = pObject;
   m_dwNumObjects++;
   if (bSortIndex)
      qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
   UnlockObjectIndex();
}


//
// Replace object's data in list
//

static void ReplaceObject(NXC_OBJECT *pObject, NXC_OBJECT *pNewObject)
{
   DebugPrintf(_T("ReplaceObject(id:%d, name:\"%s\")"), pObject->dwId, pObject->szName);
   switch(pObject->iClass)
   {
      case OBJECT_NETWORKSERVICE:
         safe_free(pObject->netsrv.pszRequest);
         safe_free(pObject->netsrv.pszResponse);
         break;
      case OBJECT_ZONE:
         safe_free(pObject->zone.pdwAddrList);
         break;
      case OBJECT_VPNCONNECTOR:
         safe_free(pObject->vpnc.pLocalNetList);
         safe_free(pObject->vpnc.pRemoteNetList);
         break;
      case OBJECT_CONDITION:
         safe_free(pObject->cond.pszScript);
         safe_free(pObject->cond.pDCIList);
         break;
      case OBJECT_CLUSTER:
         safe_free(pObject->cluster.pSyncNetList);
         break;
   }
   safe_free(pObject->pdwChildList);
   safe_free(pObject->pdwParentList);
   safe_free(pObject->pAccessList);
   safe_free(pObject->pszComments);
   memcpy(pObject, pNewObject, sizeof(NXC_OBJECT));
   free(pNewObject);
}


//
// Create new object from message
//

static NXC_OBJECT *NewObjectFromMsg(CSCPMessage *pMsg)
{
   NXC_OBJECT *pObject;
   DWORD i, dwId1, dwId2;

   // Allocate memory for new object structure
   pObject = (NXC_OBJECT *)malloc(sizeof(NXC_OBJECT));
   memset(pObject, 0, sizeof(NXC_OBJECT));

   // Common attributes
   pObject->dwId = pMsg->GetVariableLong(VID_OBJECT_ID);
   pObject->iClass = pMsg->GetVariableShort(VID_OBJECT_CLASS);
   pMsg->GetVariableStr(VID_OBJECT_NAME, pObject->szName, MAX_OBJECT_NAME);
   pObject->iStatus = pMsg->GetVariableShort(VID_OBJECT_STATUS);
   pObject->dwIpAddr = pMsg->GetVariableLong(VID_IP_ADDRESS);
   pObject->bIsDeleted = pMsg->GetVariableShort(VID_IS_DELETED);
   pObject->dwImage = pMsg->GetVariableLong(VID_IMAGE_ID);
   pObject->iStatusCalcAlg = (int)pMsg->GetVariableShort(VID_STATUS_CALCULATION_ALG);
   pObject->iStatusPropAlg = (int)pMsg->GetVariableShort(VID_STATUS_PROPAGATION_ALG);
   pObject->iFixedStatus = (int)pMsg->GetVariableShort(VID_FIXED_STATUS);
   pObject->iStatusShift = pMsg->GetVariableShortAsInt32(VID_STATUS_SHIFT);
   pObject->iStatusTrans[0] = (int)pMsg->GetVariableShort(VID_STATUS_TRANSLATION_1);
   pObject->iStatusTrans[1] = (int)pMsg->GetVariableShort(VID_STATUS_TRANSLATION_2);
   pObject->iStatusTrans[2] = (int)pMsg->GetVariableShort(VID_STATUS_TRANSLATION_3);
   pObject->iStatusTrans[3] = (int)pMsg->GetVariableShort(VID_STATUS_TRANSLATION_4);
   pObject->iStatusSingleTh = (int)pMsg->GetVariableShort(VID_STATUS_SINGLE_THRESHOLD);
   pObject->iStatusThresholds[0] = (int)pMsg->GetVariableShort(VID_STATUS_THRESHOLD_1);
   pObject->iStatusThresholds[1] = (int)pMsg->GetVariableShort(VID_STATUS_THRESHOLD_2);
   pObject->iStatusThresholds[2] = (int)pMsg->GetVariableShort(VID_STATUS_THRESHOLD_3);
   pObject->iStatusThresholds[3] = (int)pMsg->GetVariableShort(VID_STATUS_THRESHOLD_4);
   pObject->pszComments = pMsg->GetVariableStr(VID_COMMENTS);

   // Parents
   pObject->dwNumParents = pMsg->GetVariableLong(VID_PARENT_CNT);
   pObject->pdwParentList = (DWORD *)malloc(sizeof(DWORD) * pObject->dwNumParents);
   for(i = 0, dwId1 = VID_PARENT_ID_BASE; i < pObject->dwNumParents; i++, dwId1++)
      pObject->pdwParentList[i] = pMsg->GetVariableLong(dwId1);

   // Childs
   pObject->dwNumChilds = pMsg->GetVariableLong(VID_CHILD_CNT);
   pObject->pdwChildList = (DWORD *)malloc(sizeof(DWORD) * pObject->dwNumChilds);
   for(i = 0, dwId1 = VID_CHILD_ID_BASE; i < pObject->dwNumChilds; i++, dwId1++)
      pObject->pdwChildList[i] = pMsg->GetVariableLong(dwId1);

   // Access control
   pObject->bInheritRights = pMsg->GetVariableShort(VID_INHERIT_RIGHTS);
   pObject->dwAclSize = pMsg->GetVariableLong(VID_ACL_SIZE);
   pObject->pAccessList = (NXC_ACL_ENTRY *)malloc(sizeof(NXC_ACL_ENTRY) * pObject->dwAclSize);
   for(i = 0, dwId1 = VID_ACL_USER_BASE, dwId2 = VID_ACL_RIGHTS_BASE; 
       i < pObject->dwAclSize; i++, dwId1++, dwId2++)
   {
      pObject->pAccessList[i].dwUserId = pMsg->GetVariableLong(dwId1);
      pObject->pAccessList[i].dwAccessRights = pMsg->GetVariableLong(dwId2);
   }

   // Class-specific attributes
   switch(pObject->iClass)
   {
      case OBJECT_INTERFACE:
         pObject->iface.dwIpNetMask = pMsg->GetVariableLong(VID_IP_NETMASK);
         pObject->iface.dwIfIndex = pMsg->GetVariableLong(VID_IF_INDEX);
         pObject->iface.dwIfType = pMsg->GetVariableLong(VID_IF_TYPE);
         pMsg->GetVariableBinary(VID_MAC_ADDR, pObject->iface.bMacAddr, MAC_ADDR_LENGTH);
         break;
      case OBJECT_NODE:
         pObject->node.dwFlags = pMsg->GetVariableLong(VID_FLAGS);
         pObject->node.dwNodeType = pMsg->GetVariableLong(VID_NODE_TYPE);
         pObject->node.dwPollerNode = pMsg->GetVariableLong(VID_POLLER_NODE_ID);
         pObject->node.dwProxyNode = pMsg->GetVariableLong(VID_PROXY_NODE);
         pObject->node.dwZoneGUID = pMsg->GetVariableLong(VID_ZONE_GUID);
         pObject->node.wAgentPort = pMsg->GetVariableShort(VID_AGENT_PORT);
         pObject->node.wAuthMethod = pMsg->GetVariableShort(VID_AUTH_METHOD);
         pMsg->GetVariableStr(VID_SHARED_SECRET, pObject->node.szSharedSecret, MAX_SECRET_LENGTH);
         pMsg->GetVariableStr(VID_COMMUNITY_STRING, pObject->node.szCommunityString, MAX_COMMUNITY_LENGTH);
         pMsg->GetVariableStr(VID_SNMP_OID, pObject->node.szObjectId, MAX_OID_LENGTH);
         pObject->node.wSNMPVersion = pMsg->GetVariableShort(VID_SNMP_VERSION);
         pMsg->GetVariableStr(VID_AGENT_VERSION, pObject->node.szAgentVersion, MAX_AGENT_VERSION_LEN);
         pMsg->GetVariableStr(VID_PLATFORM_NAME, pObject->node.szPlatformName, MAX_PLATFORM_NAME_LEN);
         break;
      case OBJECT_SUBNET:
         pObject->subnet.dwIpNetMask = pMsg->GetVariableLong(VID_IP_NETMASK);
         pObject->subnet.dwZoneGUID = pMsg->GetVariableLong(VID_ZONE_GUID);
         break;
      case OBJECT_CONTAINER:
         pObject->container.dwCategory = pMsg->GetVariableLong(VID_CATEGORY);
         break;
      case OBJECT_TEMPLATE:
         pObject->dct.dwVersion = pMsg->GetVariableLong(VID_TEMPLATE_VERSION);
         break;
      case OBJECT_NETWORKSERVICE:
         pObject->netsrv.iServiceType = (int)pMsg->GetVariableShort(VID_SERVICE_TYPE);
         pObject->netsrv.wProto = pMsg->GetVariableShort(VID_IP_PROTO);
         pObject->netsrv.wPort = pMsg->GetVariableShort(VID_IP_PORT);
         pObject->netsrv.dwPollerNode = pMsg->GetVariableLong(VID_POLLER_NODE_ID);
         pObject->netsrv.pszRequest = pMsg->GetVariableStr(VID_SERVICE_REQUEST);
         pObject->netsrv.pszResponse = pMsg->GetVariableStr(VID_SERVICE_RESPONSE);
         break;
      case OBJECT_ZONE:
         pObject->zone.dwZoneGUID = pMsg->GetVariableLong(VID_ZONE_GUID);
         pObject->zone.wZoneType = pMsg->GetVariableShort(VID_ZONE_TYPE);
         pObject->zone.dwControllerIpAddr = pMsg->GetVariableLong(VID_CONTROLLER_IP_ADDR);
         pObject->zone.dwAddrListSize = pMsg->GetVariableLong(VID_ADDR_LIST_SIZE);
         pObject->zone.pdwAddrList = (DWORD *)malloc(sizeof(DWORD) * pObject->zone.dwAddrListSize);
         pMsg->GetVariableInt32Array(VID_IP_ADDR_LIST, pObject->zone.dwAddrListSize, pObject->zone.pdwAddrList);
         break;
      case OBJECT_VPNCONNECTOR:
         pObject->vpnc.dwPeerGateway = pMsg->GetVariableLong(VID_PEER_GATEWAY);
         pObject->vpnc.dwNumLocalNets = pMsg->GetVariableLong(VID_NUM_LOCAL_NETS);
         pObject->vpnc.pLocalNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * pObject->vpnc.dwNumLocalNets);
         for(i = 0, dwId1 = VID_VPN_NETWORK_BASE; i < pObject->vpnc.dwNumLocalNets; i++)
         {
            pObject->vpnc.pLocalNetList[i].dwAddr = pMsg->GetVariableLong(dwId1++);
            pObject->vpnc.pLocalNetList[i].dwMask = pMsg->GetVariableLong(dwId1++);
         }
         pObject->vpnc.dwNumRemoteNets = pMsg->GetVariableLong(VID_NUM_REMOTE_NETS);
         pObject->vpnc.pRemoteNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * pObject->vpnc.dwNumRemoteNets);
         for(i = 0; i < pObject->vpnc.dwNumRemoteNets; i++)
         {
            pObject->vpnc.pRemoteNetList[i].dwAddr = pMsg->GetVariableLong(dwId1++);
            pObject->vpnc.pRemoteNetList[i].dwMask = pMsg->GetVariableLong(dwId1++);
         }
         break;
      case OBJECT_CONDITION:
         pObject->cond.dwActivationEvent = pMsg->GetVariableLong(VID_ACTIVATION_EVENT);
         pObject->cond.dwDeactivationEvent = pMsg->GetVariableLong(VID_DEACTIVATION_EVENT);
         pObject->cond.dwSourceObject = pMsg->GetVariableLong(VID_SOURCE_OBJECT);
         pObject->cond.pszScript = pMsg->GetVariableStr(VID_SCRIPT);
         pObject->cond.wActiveStatus = pMsg->GetVariableShort(VID_ACTIVE_STATUS);
         pObject->cond.wInactiveStatus = pMsg->GetVariableShort(VID_INACTIVE_STATUS);
         pObject->cond.dwNumDCI = pMsg->GetVariableLong(VID_NUM_ITEMS);
         pObject->cond.pDCIList = (INPUT_DCI *)malloc(sizeof(INPUT_DCI) * pObject->cond.dwNumDCI);
         for(i = 0, dwId1 = VID_DCI_LIST_BASE; i < pObject->cond.dwNumDCI; i++)
         {
            pObject->cond.pDCIList[i].dwId = pMsg->GetVariableLong(dwId1++);
            pObject->cond.pDCIList[i].dwNodeId = pMsg->GetVariableLong(dwId1++);
            pObject->cond.pDCIList[i].nFunction = pMsg->GetVariableShort(dwId1++);
            pObject->cond.pDCIList[i].nPolls = pMsg->GetVariableShort(dwId1++);
            dwId1 += 6;
         }
         break;
      case OBJECT_CLUSTER:
         pObject->cluster.dwClusterType = pMsg->GetVariableLong(VID_CLUSTER_TYPE);
         pObject->cluster.dwNumSyncNets = pMsg->GetVariableLong(VID_NUM_SYNC_SUBNETS);
         pObject->cluster.pSyncNetList = (IP_NETWORK *)malloc(sizeof(IP_NETWORK) * pObject->cluster.dwNumSyncNets);
			pMsg->GetVariableInt32Array(VID_SYNC_SUBNETS, pObject->cluster.dwNumSyncNets * 2, (DWORD *)pObject->cluster.pSyncNetList);
         break;
      default:
         break;
   }

   return pObject;
}


//
// Process object information received from server
//

void NXCL_Session::ProcessObjectUpdate(CSCPMessage *pMsg)
{
   NXC_OBJECT *pObject, *pNewObject;
	TCHAR *pTmp;

   switch(pMsg->GetCode())
   {
      case CMD_OBJECT_LIST_END:
         if (!(m_dwFlags & NXC_SF_HAS_OBJECT_CACHE))
         {
            LockObjectIndex();
            qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
            UnlockObjectIndex();
         }
         CompleteSync(SYNC_OBJECTS, RCC_SUCCESS);
         break;
      case CMD_OBJECT:
			pTmp = pMsg->GetVariableStr(VID_OBJECT_NAME);
         DebugPrintf(_T("RECV_OBJECT: ID=%d Name=\"%s\" Class=%d"), pMsg->GetVariableLong(VID_OBJECT_ID),
                     pTmp, pMsg->GetVariableShort(VID_OBJECT_CLASS));
			free(pTmp);
      
         // Create new object from message and add it to list
         pNewObject = NewObjectFromMsg(pMsg);
         if (m_dwFlags & NXC_SF_HAS_OBJECT_CACHE)
         {
            // We already have some objects loaded from cache file
            pObject = FindObjectById(pNewObject->dwId, TRUE);
            if (pObject == NULL)
            {
               AddObject(pNewObject, TRUE);
            }
            else
            {
               ReplaceObject(pObject, pNewObject);
            }
         }
         else
         {
            // No cache file, all objects are new
            AddObject(pNewObject, FALSE);
         }
         break;
      case CMD_OBJECT_UPDATE:
         pNewObject = NewObjectFromMsg(pMsg);
         pObject = FindObjectById(pNewObject->dwId, TRUE);
         if (pObject == NULL)
         {
            AddObject(pNewObject, TRUE);
            pObject = pNewObject;
         }
         else
         {
            ReplaceObject(pObject, pNewObject);
         }
         CallEventHandler(NXC_EVENT_OBJECT_CHANGED, pObject->dwId, pObject);
         break;
      default:
         break;
   }
}


//
// Synchronize objects with the server
// This function is NOT REENTRANT
//

DWORD NXCL_Session::SyncObjects(TCHAR *pszCacheFile, BOOL bSyncComments)
{
   CSCPMessage msg;
   DWORD dwRetCode, dwRqId;

   dwRqId = CreateRqId();
   PrepareForSync(SYNC_OBJECTS);

   DestroyAllObjects();

   m_dwFlags &= ~NXC_SF_HAS_OBJECT_CACHE;
   if (pszCacheFile != NULL)
      LoadObjectsFromCache(pszCacheFile);

   msg.SetCode(CMD_GET_OBJECTS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_TIMESTAMP, m_dwTimeStamp);
   msg.SetVariable(VID_SYNC_COMMENTS, (WORD)bSyncComments);
   SendMsg(&msg);

   dwRetCode = WaitForRCC(dwRqId);

   // If request was successful, wait for object list end or for disconnection
   if (dwRetCode == RCC_SUCCESS)
      dwRetCode = WaitForSync(SYNC_OBJECTS, INFINITE);
   else
      UnlockSyncOp(SYNC_OBJECTS);

   return dwRetCode;
}


//
// Wrappers for NXCL_Session::SyncObjects()
//

DWORD LIBNXCL_EXPORTABLE NXCSyncObjects(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->SyncObjects(NULL, FALSE);
}

DWORD LIBNXCL_EXPORTABLE NXCSyncObjectsEx(NXC_SESSION hSession, TCHAR *pszCacheFile,
                                          BOOL bSyncComments)
{
   return ((NXCL_Session *)hSession)->SyncObjects(pszCacheFile, bSyncComments);
}


//
// Find object by ID
//

NXC_OBJECT *NXCL_Session::FindObjectById(DWORD dwId, BOOL bLock)
{
   DWORD dwPos;
   NXC_OBJECT *pObject;

   if (bLock)
      LockObjectIndex();

   dwPos = SearchIndex(m_pIndexById, m_dwNumObjects, dwId);
   pObject = (dwPos == INVALID_INDEX) ? NULL : m_pIndexById[dwPos].pObject;

   if (bLock)
      UnlockObjectIndex();

   return pObject;
}

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectById(NXC_SESSION hSession, DWORD dwId)
{
   return ((NXCL_Session *)hSession)->FindObjectById(dwId, TRUE);
}

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByIdNoLock(NXC_SESSION hSession, DWORD dwId)
{
   return ((NXCL_Session *)hSession)->FindObjectById(dwId, FALSE);
}


//
// Find object by name
//

NXC_OBJECT *NXCL_Session::FindObjectByName(TCHAR *pszName)
{
   NXC_OBJECT *pObject = NULL;
   DWORD i;

   if (pszName != NULL)
      if (*pszName != 0)
      {
         LockObjectIndex();

         for(i = 0; i < m_dwNumObjects; i++)
            if (MatchString(pszName, m_pIndexById[i].pObject->szName, FALSE))
            {
               pObject = m_pIndexById[i].pObject;
               break;
            }

         UnlockObjectIndex();
      }
   return pObject;
}

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCFindObjectByName(NXC_SESSION hSession, TCHAR *pszName)
{
   return ((NXCL_Session *)hSession)->FindObjectByName(pszName);
}


//
// Enumerate all objects
//

void NXCL_Session::EnumerateObjects(BOOL (* pHandler)(NXC_OBJECT *))
{
   DWORD i;

   LockObjectIndex();
   for(i = 0; i < m_dwNumObjects; i++)
      if (!pHandler(m_pIndexById[i].pObject))
         break;
   UnlockObjectIndex();
}

void LIBNXCL_EXPORTABLE NXCEnumerateObjects(NXC_SESSION hSession, BOOL (* pHandler)(NXC_OBJECT *))
{
   ((NXCL_Session *)hSession)->EnumerateObjects(pHandler);
}


//
// Get root object
//

NXC_OBJECT *NXCL_Session::GetRootObject(DWORD dwId, DWORD dwIndex)
{
   if (m_dwNumObjects > dwIndex)
      if (m_pIndexById[dwIndex].dwKey == dwId)
         return m_pIndexById[dwIndex].pObject;
   return NULL;
}


//
// Get topology root ("Entire Network") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTopologyRootObject(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->GetRootObject(1, 0);
}


//
// Get service tree root ("All Services") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetServiceRootObject(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->GetRootObject(2, 1);
}


//
// Get template tree root ("Templates") object
//

NXC_OBJECT LIBNXCL_EXPORTABLE *NXCGetTemplateRootObject(NXC_SESSION hSession)
{
   return ((NXCL_Session *)hSession)->GetRootObject(3, 2);
}


//
// Get pointer to first object on objects' list and entire number of objects
//

void *NXCL_Session::GetObjectIndex(DWORD *pdwNumObjects)
{
   if (pdwNumObjects != NULL)
      *pdwNumObjects = m_dwNumObjects;
   return m_pIndexById;
}

void LIBNXCL_EXPORTABLE *NXCGetObjectIndex(NXC_SESSION hSession, DWORD *pdwNumObjects)
{
   return ((NXCL_Session *)hSession)->GetObjectIndex(pdwNumObjects);
}


//
// Lock object index
//

void LIBNXCL_EXPORTABLE NXCLockObjectIndex(NXC_SESSION hSession)
{
   ((NXCL_Session *)hSession)->LockObjectIndex();
}


//
// Unlock object index
//

void LIBNXCL_EXPORTABLE NXCUnlockObjectIndex(NXC_SESSION hSession)
{
   ((NXCL_Session *)hSession)->UnlockObjectIndex();
}


//
// Modify object
//

DWORD LIBNXCL_EXPORTABLE NXCModifyObject(NXC_SESSION hSession, NXC_OBJECT_UPDATE *pUpdate)
{
   CSCPMessage msg;
   DWORD dwRqId, i, dwId1, dwId2;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_MODIFY_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, pUpdate->dwObjectId);
   if (pUpdate->dwFlags & OBJ_UPDATE_NAME)
      msg.SetVariable(VID_OBJECT_NAME, pUpdate->pszName);
   if (pUpdate->dwFlags & OBJ_UPDATE_AGENT_PORT)
      msg.SetVariable(VID_AGENT_PORT, (WORD)pUpdate->iAgentPort);
   if (pUpdate->dwFlags & OBJ_UPDATE_AGENT_AUTH)
      msg.SetVariable(VID_AUTH_METHOD, (WORD)pUpdate->iAuthType);
   if (pUpdate->dwFlags & OBJ_UPDATE_AGENT_SECRET)
      msg.SetVariable(VID_SHARED_SECRET, pUpdate->pszSecret);
   if (pUpdate->dwFlags & OBJ_UPDATE_SNMP_COMMUNITY)
      msg.SetVariable(VID_COMMUNITY_STRING, pUpdate->pszCommunity);
   if (pUpdate->dwFlags & OBJ_UPDATE_IMAGE)
      msg.SetVariable(VID_IMAGE_ID, pUpdate->dwImage);
   if (pUpdate->dwFlags & OBJ_UPDATE_SNMP_VERSION)
      msg.SetVariable(VID_SNMP_VERSION, pUpdate->wSNMPVersion);
   if (pUpdate->dwFlags & OBJ_UPDATE_CHECK_REQUEST)
      msg.SetVariable(VID_SERVICE_REQUEST, pUpdate->pszRequest);
   if (pUpdate->dwFlags & OBJ_UPDATE_CHECK_RESPONSE)
      msg.SetVariable(VID_SERVICE_RESPONSE, pUpdate->pszResponse);
   if (pUpdate->dwFlags & OBJ_UPDATE_IP_PROTO)
      msg.SetVariable(VID_IP_PROTO, pUpdate->wProto);
   if (pUpdate->dwFlags & OBJ_UPDATE_IP_PORT)
      msg.SetVariable(VID_IP_PORT, pUpdate->wPort);
   if (pUpdate->dwFlags & OBJ_UPDATE_SERVICE_TYPE)
      msg.SetVariable(VID_SERVICE_TYPE, (WORD)pUpdate->iServiceType);
   if (pUpdate->dwFlags & OBJ_UPDATE_POLLER_NODE)
      msg.SetVariable(VID_POLLER_NODE_ID, pUpdate->dwPollerNode);
   if (pUpdate->dwFlags & OBJ_UPDATE_PROXY_NODE)
      msg.SetVariable(VID_PROXY_NODE, pUpdate->dwProxyNode);
   if (pUpdate->dwFlags & OBJ_UPDATE_IP_ADDR)
      msg.SetVariable(VID_IP_ADDRESS, pUpdate->dwIpAddr);
   if (pUpdate->dwFlags & OBJ_UPDATE_PEER_GATEWAY)
      msg.SetVariable(VID_PEER_GATEWAY, pUpdate->dwPeerGateway);
   if (pUpdate->dwFlags & OBJ_UPDATE_NODE_FLAGS)
      msg.SetVariable(VID_FLAGS, pUpdate->dwNodeFlags);
   if (pUpdate->dwFlags & OBJ_UPDATE_ACT_EVENT)
      msg.SetVariable(VID_ACTIVATION_EVENT, pUpdate->dwActivationEvent);
   if (pUpdate->dwFlags & OBJ_UPDATE_DEACT_EVENT)
      msg.SetVariable(VID_DEACTIVATION_EVENT, pUpdate->dwDeactivationEvent);
   if (pUpdate->dwFlags & OBJ_UPDATE_SOURCE_OBJECT)
      msg.SetVariable(VID_SOURCE_OBJECT, pUpdate->dwSourceObject);
   if (pUpdate->dwFlags & OBJ_UPDATE_ACTIVE_STATUS)
      msg.SetVariable(VID_ACTIVE_STATUS, (WORD)pUpdate->nActiveStatus);
   if (pUpdate->dwFlags & OBJ_UPDATE_INACTIVE_STATUS)
      msg.SetVariable(VID_INACTIVE_STATUS, (WORD)pUpdate->nInactiveStatus);
   if (pUpdate->dwFlags & OBJ_UPDATE_SCRIPT)
      msg.SetVariable(VID_SCRIPT, pUpdate->pszScript);
   if (pUpdate->dwFlags & OBJ_UPDATE_CLUSTER_TYPE)
      msg.SetVariable(VID_CLUSTER_TYPE, pUpdate->dwClusterType);
   if (pUpdate->dwFlags & OBJ_UPDATE_STATUS_ALG)
   {
      msg.SetVariable(VID_STATUS_CALCULATION_ALG, (WORD)pUpdate->iStatusCalcAlg);
      msg.SetVariable(VID_STATUS_PROPAGATION_ALG, (WORD)pUpdate->iStatusPropAlg);
      msg.SetVariable(VID_FIXED_STATUS, (WORD)pUpdate->iFixedStatus);
      msg.SetVariable(VID_STATUS_SHIFT, (WORD)pUpdate->iStatusShift);
      msg.SetVariable(VID_STATUS_TRANSLATION_1, (WORD)pUpdate->iStatusTrans[0]);
      msg.SetVariable(VID_STATUS_TRANSLATION_2, (WORD)pUpdate->iStatusTrans[1]);
      msg.SetVariable(VID_STATUS_TRANSLATION_3, (WORD)pUpdate->iStatusTrans[2]);
      msg.SetVariable(VID_STATUS_TRANSLATION_4, (WORD)pUpdate->iStatusTrans[3]);
      msg.SetVariable(VID_STATUS_SINGLE_THRESHOLD, (WORD)pUpdate->iStatusSingleTh);
      msg.SetVariable(VID_STATUS_THRESHOLD_1, (WORD)pUpdate->iStatusThresholds[0]);
      msg.SetVariable(VID_STATUS_THRESHOLD_2, (WORD)pUpdate->iStatusThresholds[1]);
      msg.SetVariable(VID_STATUS_THRESHOLD_3, (WORD)pUpdate->iStatusThresholds[2]);
      msg.SetVariable(VID_STATUS_THRESHOLD_4, (WORD)pUpdate->iStatusThresholds[3]);
   }
   if (pUpdate->dwFlags & OBJ_UPDATE_NETWORK_LIST)
   {
      msg.SetVariable(VID_NUM_LOCAL_NETS, pUpdate->dwNumLocalNets);
      msg.SetVariable(VID_NUM_REMOTE_NETS, pUpdate->dwNumRemoteNets);
      for(i = 0, dwId1 = VID_VPN_NETWORK_BASE; i < pUpdate->dwNumLocalNets; i++)
      {
         msg.SetVariable(dwId1++, pUpdate->pLocalNetList[i].dwAddr);
         msg.SetVariable(dwId1++, pUpdate->pLocalNetList[i].dwMask);
      }
      for(i = 0; i < pUpdate->dwNumRemoteNets; i++)
      {
         msg.SetVariable(dwId1++, pUpdate->pRemoteNetList[i].dwAddr);
         msg.SetVariable(dwId1++, pUpdate->pRemoteNetList[i].dwMask);
      }
   }
   if (pUpdate->dwFlags & OBJ_UPDATE_ACL)
   {
      msg.SetVariable(VID_ACL_SIZE, pUpdate->dwAclSize);
      msg.SetVariable(VID_INHERIT_RIGHTS, (WORD)pUpdate->bInheritRights);
      for(i = 0, dwId1 = VID_ACL_USER_BASE, dwId2 = VID_ACL_RIGHTS_BASE;
          i < pUpdate->dwAclSize; i++, dwId1++, dwId2++)
      {
         msg.SetVariable(dwId1, pUpdate->pAccessList[i].dwUserId);
         msg.SetVariable(dwId2, pUpdate->pAccessList[i].dwAccessRights);
      }
   }
   if (pUpdate->dwFlags & OBJ_UPDATE_DCI_LIST)
   {
      msg.SetVariable(VID_NUM_ITEMS, pUpdate->dwNumDCI);
      for(i = 0, dwId1 = VID_DCI_LIST_BASE; i < pUpdate->dwNumDCI; i++)
      {
         msg.SetVariable(dwId1++, pUpdate->pDCIList[i].dwId);
         msg.SetVariable(dwId1++, pUpdate->pDCIList[i].dwNodeId);
         msg.SetVariable(dwId1++, (WORD)pUpdate->pDCIList[i].nFunction);
         msg.SetVariable(dwId1++, (WORD)pUpdate->pDCIList[i].nPolls);
         dwId1 += 6;
      }
   }
   if (pUpdate->dwFlags & OBJ_UPDATE_SYNC_NETS)
   {
      msg.SetVariable(VID_NUM_SYNC_SUBNETS, pUpdate->dwNumSyncNets);
		msg.SetVariableToInt32Array(VID_SYNC_SUBNETS, pUpdate->dwNumSyncNets * 2, (DWORD *)pUpdate->pSyncNetList);
   }

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Set object's mamagement status
//

DWORD LIBNXCL_EXPORTABLE NXCSetObjectMgmtStatus(NXC_SESSION hSession, DWORD dwObjectId, 
                                                BOOL bIsManaged)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_SET_OBJECT_MGMT_STATUS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_MGMT_STATUS, (WORD)bIsManaged);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Create new object
//

DWORD LIBNXCL_EXPORTABLE NXCCreateObject(NXC_SESSION hSession, 
                                         NXC_OBJECT_CREATE_INFO *pCreateInfo, 
                                         DWORD *pdwObjectId)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_CREATE_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PARENT_ID, pCreateInfo->dwParentId);
   msg.SetVariable(VID_OBJECT_CLASS, (WORD)pCreateInfo->iClass);
   msg.SetVariable(VID_OBJECT_NAME, pCreateInfo->pszName);
	if (pCreateInfo->pszComments != NULL)
	   msg.SetVariable(VID_COMMENTS, pCreateInfo->pszComments);
   switch(pCreateInfo->iClass)
   {
      case OBJECT_NODE:
         msg.SetVariable(VID_IP_ADDRESS, pCreateInfo->cs.node.dwIpAddr);
         msg.SetVariable(VID_IP_NETMASK, pCreateInfo->cs.node.dwNetMask);
         msg.SetVariable(VID_CREATION_FLAGS, pCreateInfo->cs.node.dwCreationFlags);
         msg.SetVariable(VID_PROXY_NODE, pCreateInfo->cs.node.dwProxyNode);
         break;
      case OBJECT_CONTAINER:
         msg.SetVariable(VID_CATEGORY, pCreateInfo->cs.container.dwCategory);
         break;
      case OBJECT_NETWORKSERVICE:
         msg.SetVariable(VID_SERVICE_TYPE, (WORD)pCreateInfo->cs.netsrv.iServiceType);
         msg.SetVariable(VID_IP_PROTO, pCreateInfo->cs.netsrv.wProto);
         msg.SetVariable(VID_IP_PORT, pCreateInfo->cs.netsrv.wPort);
         msg.SetVariable(VID_SERVICE_REQUEST, pCreateInfo->cs.netsrv.pszRequest);
         msg.SetVariable(VID_SERVICE_RESPONSE, pCreateInfo->cs.netsrv.pszResponse);
         break;
      default:
         break;
   }

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response. Creating node object can include polling,
   // which can take a minute or even more in worst cases
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 300000);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwObjectId = pResponse->GetVariableLong(VID_OBJECT_ID);
      }
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}


//
// Bind/unbind objects
//

static DWORD ChangeObjectBinding(NXCL_Session *pSession, DWORD dwParentObject,
                                 DWORD dwChildObject, BOOL bBind, BOOL bRemoveDCI)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = pSession->CreateRqId();

   // Build request message
   msg.SetCode(bBind ? CMD_BIND_OBJECT : CMD_UNBIND_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_PARENT_ID, dwParentObject);
   msg.SetVariable(VID_CHILD_ID, dwChildObject);
   if (!bBind)
      msg.SetVariable(VID_REMOVE_DCI, (WORD)bRemoveDCI);

   // Send request
   pSession->SendMsg(&msg);

   // Wait for reply
   return pSession->WaitForRCC(dwRqId);
}


//
// Bind object
//

DWORD LIBNXCL_EXPORTABLE NXCBindObject(NXC_SESSION hSession, DWORD dwParentObject, 
                                       DWORD dwChildObject)
{
   return ChangeObjectBinding((NXCL_Session *)hSession, dwParentObject,
                              dwChildObject, TRUE, FALSE);
}


//
// Unbind object
//

DWORD LIBNXCL_EXPORTABLE NXCUnbindObject(NXC_SESSION hSession, DWORD dwParentObject,
                                         DWORD dwChildObject)
{
   return ChangeObjectBinding((NXCL_Session *)hSession, dwParentObject, 
                              dwChildObject, FALSE, FALSE);
}


//
// Remove template from node
//

DWORD LIBNXCL_EXPORTABLE NXCRemoveTemplate(NXC_SESSION hSession, DWORD dwTemplateId,
                                           DWORD dwNodeId, BOOL bRemoveDCI)
{
   return ChangeObjectBinding((NXCL_Session *)hSession, dwTemplateId, 
                              dwNodeId, FALSE, bRemoveDCI);
}


//
// Delete object
//

DWORD LIBNXCL_EXPORTABLE NXCDeleteObject(NXC_SESSION hSession, DWORD dwObject)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_DELETE_OBJECT);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObject);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for reply
   // For node objects, deletion may take long time if object
   // is currently polled
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId, 300000);
}


//
// Load container categories
//

DWORD LIBNXCL_EXPORTABLE NXCLoadCCList(NXC_SESSION hSession, NXC_CC_LIST **ppList)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwRetCode = RCC_SUCCESS, dwNumCats = 0, dwCatId = 0;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_CONTAINER_CAT_LIST);
   msg.SetId(dwRqId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   *ppList = (NXC_CC_LIST *)malloc(sizeof(NXC_CC_LIST));
   (*ppList)->dwNumElements = 0;
   (*ppList)->pElements = NULL;

   do
   {
      pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_CONTAINER_CAT_DATA, dwRqId);
      if (pResponse != NULL)
      {
         dwCatId = pResponse->GetVariableLong(VID_CATEGORY_ID);
         if (dwCatId != 0)  // 0 is end of list indicator
         {
            (*ppList)->pElements = (NXC_CONTAINER_CATEGORY *)realloc((*ppList)->pElements, 
               sizeof(NXC_CONTAINER_CATEGORY) * ((*ppList)->dwNumElements + 1));
            (*ppList)->pElements[(*ppList)->dwNumElements].dwId = dwCatId;
            (*ppList)->pElements[(*ppList)->dwNumElements].dwImageId =
               pResponse->GetVariableLong(VID_IMAGE_ID);
            pResponse->GetVariableStr(VID_CATEGORY_NAME, 
               (*ppList)->pElements[(*ppList)->dwNumElements].szName, MAX_OBJECT_NAME);
            (*ppList)->pElements[(*ppList)->dwNumElements].pszDescription =
               pResponse->GetVariableStr(VID_DESCRIPTION);
            (*ppList)->dwNumElements++;
         }
         delete pResponse;
      }
      else
      {
         dwRetCode = RCC_TIMEOUT;
         dwCatId = 0;
      }
   }
   while(dwCatId != 0);

   // Destroy results on failure
   if (dwRetCode != RCC_SUCCESS)
   {
      safe_free((*ppList)->pElements);
      free(*ppList);
      *ppList = NULL;
   }

   return dwRetCode;
}


//
// Destroy list of container categories
//

void LIBNXCL_EXPORTABLE NXCDestroyCCList(NXC_CC_LIST *pList)
{
   DWORD i;

   if (pList == NULL)
      return;

   for(i = 0; i < pList->dwNumElements; i++)
      safe_free(pList->pElements[i].pszDescription);
   safe_free(pList->pElements);
   free(pList);
}


//
// Perform a forced node poll
//

DWORD LIBNXCL_EXPORTABLE NXCPollNode(NXC_SESSION hSession, DWORD dwObjectId, int iPollType, 
                                     void (* pfCallback)(TCHAR *, void *), void *pArg)
{
   DWORD dwRetCode, dwRqId;
   CSCPMessage msg, *pResponse;
   TCHAR *pszMsg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_POLL_NODE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_POLL_TYPE, (WORD)iPollType);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   do
   {
      // Polls can take a long time, so we wait up to 120 seconds for each message
      pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_POLLING_INFO, dwRqId, 120000);
      if (pResponse != NULL)
      {
         dwRetCode = pResponse->GetVariableLong(VID_RCC);
         if ((dwRetCode == RCC_OPERATION_IN_PROGRESS) && (pfCallback != NULL))
         {
            pszMsg = pResponse->GetVariableStr(VID_POLLER_MESSAGE);
            pfCallback(pszMsg, pArg);
            free(pszMsg);
         }
         delete pResponse;
      }
      else
      {
         dwRetCode = RCC_TIMEOUT;
      }
   }
   while(dwRetCode == RCC_OPERATION_IN_PROGRESS);

   return dwRetCode;
}


//
// Wake up node by sending magic packet
//

DWORD LIBNXCL_EXPORTABLE NXCWakeUpNode(NXC_SESSION hSession, DWORD dwObjectId)
{
   DWORD dwRqId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_WAKEUP_NODE);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Retrieve list of supported agent parameters
//

DWORD LIBNXCL_EXPORTABLE NXCGetSupportedParameters(NXC_SESSION hSession, DWORD dwNodeId,
                                                   DWORD *pdwNumParams, 
                                                   NXC_AGENT_PARAM **ppParamList)
{
   CSCPMessage msg, *pResponse;
   DWORD i, dwId, dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   *pdwNumParams = 0;
   *ppParamList = NULL;

   // Build request message
   msg.SetCode(CMD_GET_PARAMETER_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *pdwNumParams = pResponse->GetVariableLong(VID_NUM_PARAMETERS);
         *ppParamList = (NXC_AGENT_PARAM *)malloc(sizeof(NXC_AGENT_PARAM) * *pdwNumParams);
         for(i = 0, dwId = VID_PARAM_LIST_BASE; i < *pdwNumParams; i++)
         {
            pResponse->GetVariableStr(dwId++, (*ppParamList)[i].szName, MAX_PARAM_NAME);
            pResponse->GetVariableStr(dwId++, (*ppParamList)[i].szDescription, MAX_DB_STRING);
            (*ppParamList)[i].iDataType = (int)pResponse->GetVariableShort(dwId++);
         }
      }
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}


//
// Check if object has default name formed from IP address
//

static BOOL ObjectHasDefaultName(NXC_OBJECT *pObject)
{
   if (pObject->iClass == OBJECT_SUBNET)
   {
      TCHAR szBuffer[64], szIpAddr[32];
      _stprintf(szBuffer, _T("%s/%d"), IpToStr(pObject->dwIpAddr, szIpAddr),
                BitsInMask(pObject->subnet.dwIpNetMask));
      return !_tcscmp(szBuffer, pObject->szName);
   }
   else
   {
      return ((pObject->dwIpAddr != 0) &&
              (ntohl(_t_inet_addr(pObject->szName)) == pObject->dwIpAddr));
   }
}


//
// Get object name suitable for comparision
//

void LIBNXCL_EXPORTABLE NXCGetComparableObjectName(NXC_SESSION hSession, DWORD dwObjectId, TCHAR *pszName)
{
   NXCGetComparableObjectNameEx(((NXCL_Session *)hSession)->FindObjectById(dwObjectId, TRUE), pszName);
}


//
// Get object name suitable for comparision
//

void LIBNXCL_EXPORTABLE NXCGetComparableObjectNameEx(NXC_OBJECT *pObject, TCHAR *pszName)
{
   if (pObject != NULL)
   {
      // If object has an IP address as name, we sort as numbers
      // otherwise in alphabetical order
      if (ObjectHasDefaultName(pObject))
      {
         _sntprintf(pszName, MAX_OBJECT_NAME, _T("\x01%03d%03d%03d%03d"),
                    pObject->dwIpAddr >> 24, (pObject->dwIpAddr >> 16) & 255,
                    (pObject->dwIpAddr >> 8) & 255, pObject->dwIpAddr & 255);
      }
      else
      {
         _tcscpy(pszName, pObject->szName);
      }
   }
   else
   {
      *pszName = 0;
   }
}


//
// Save object's cache to file
//

DWORD LIBNXCL_EXPORTABLE NXCSaveObjectCache(NXC_SESSION hSession, TCHAR *pszFile)
{
   FILE *hFile;
   OBJECT_CACHE_HEADER hdr;
   DWORD i, dwResult, dwNumObjects, dwSize;
   INDEX *pList;

   hFile = _tfopen(pszFile, _T("wb"));
   if (hFile != NULL)
   {
      ((NXCL_Session *)hSession)->LockObjectIndex();
      pList = (INDEX *)((NXCL_Session *)hSession)->GetObjectIndex(&dwNumObjects);

      // Write cache file header
      hdr.dwMagic = OBJECT_CACHE_MAGIC;
      hdr.dwStructSize = sizeof(NXC_OBJECT);
      hdr.dwTimeStamp = ((NXCL_Session *)hSession)->GetTimeStamp();
      hdr.dwNumObjects = dwNumObjects;
      memcpy(hdr.bsServerId, ((NXCL_Session *)hSession)->m_bsServerId, 8);
      fwrite(&hdr, 1, sizeof(OBJECT_CACHE_HEADER), hFile);

      // Write all objects
      for(i = 0; i < dwNumObjects; i++)
      {
         fwrite(pList[i].pObject, 1, sizeof(NXC_OBJECT), hFile);
         fwrite(pList[i].pObject->pdwChildList, 1, 
                sizeof(DWORD) * pList[i].pObject->dwNumChilds, hFile);
         fwrite(pList[i].pObject->pdwParentList, 1, 
                sizeof(DWORD) * pList[i].pObject->dwNumParents, hFile);
         fwrite(pList[i].pObject->pAccessList, 1, 
                sizeof(NXC_ACL_ENTRY) * pList[i].pObject->dwAclSize, hFile);
         
			dwSize = _tcslen(pList[i].pObject->pszComments) * sizeof(TCHAR);
         fwrite(&dwSize, 1, sizeof(DWORD), hFile);
         fwrite(pList[i].pObject->pszComments, 1, dwSize, hFile);

         switch(pList[i].pObject->iClass)
         {
            case OBJECT_NETWORKSERVICE:
               dwSize = _tcslen(pList[i].pObject->netsrv.pszRequest) * sizeof(TCHAR);
               fwrite(&dwSize, 1, sizeof(DWORD), hFile);
               fwrite(pList[i].pObject->netsrv.pszRequest, 1, dwSize, hFile);

               dwSize = _tcslen(pList[i].pObject->netsrv.pszResponse) * sizeof(TCHAR);
               fwrite(&dwSize, 1, sizeof(DWORD), hFile);
               fwrite(pList[i].pObject->netsrv.pszResponse, 1, dwSize, hFile);
               break;
            case OBJECT_ZONE:
               if (pList[i].pObject->zone.dwAddrListSize > 0)
                  fwrite(pList[i].pObject->zone.pdwAddrList, sizeof(DWORD),
                         pList[i].pObject->zone.dwAddrListSize, hFile);
               break;
            case OBJECT_VPNCONNECTOR:
               fwrite(pList[i].pObject->vpnc.pLocalNetList, 1, 
                      pList[i].pObject->vpnc.dwNumLocalNets * sizeof(IP_NETWORK), hFile);
               fwrite(pList[i].pObject->vpnc.pRemoteNetList, 1, 
                      pList[i].pObject->vpnc.dwNumRemoteNets * sizeof(IP_NETWORK), hFile);
               break;
            case OBJECT_CONDITION:
               dwSize = _tcslen(pList[i].pObject->cond.pszScript) * sizeof(TCHAR);
               fwrite(&dwSize, 1, sizeof(DWORD), hFile);
               fwrite(pList[i].pObject->cond.pszScript, 1, dwSize, hFile);

               fwrite(pList[i].pObject->cond.pDCIList, 1, 
                      pList[i].pObject->cond.dwNumDCI * sizeof(INPUT_DCI), hFile);
               break;
            default:
               break;
         }
      }

      ((NXCL_Session *)hSession)->UnlockObjectIndex();
      fclose(hFile);
      dwResult = RCC_SUCCESS;
   }
   else
   {
      dwResult = RCC_IO_ERROR;
   }

   return dwResult;
}


//
// Load objects from cache file
//

void NXCL_Session::LoadObjectsFromCache(TCHAR *pszFile)
{
   FILE *hFile;
   OBJECT_CACHE_HEADER hdr;
   NXC_OBJECT object;
   DWORD i, dwSize;

   hFile = _tfopen(pszFile, _T("rb"));
   if (hFile != NULL)
   {
      // Read header
      DebugPrintf(_T("Checking cache file %s"), pszFile);
      if (fread(&hdr, 1, sizeof(OBJECT_CACHE_HEADER), hFile) == sizeof(OBJECT_CACHE_HEADER))
      {
         if ((hdr.dwMagic == OBJECT_CACHE_MAGIC) &&
             (hdr.dwStructSize == sizeof(NXC_OBJECT)) &&
             (!memcmp(hdr.bsServerId, m_bsServerId, 8)))
         {
            DebugPrintf(_T("Cache file OK, loading objects"));
            m_dwTimeStamp = hdr.dwTimeStamp;
            for(i = 0; i < hdr.dwNumObjects; i++)
            {
               if (fread(&object, 1, sizeof(NXC_OBJECT), hFile) == sizeof(NXC_OBJECT))
               {
                  object.pdwChildList = (DWORD *)malloc(sizeof(DWORD) * object.dwNumChilds);
                  fread(object.pdwChildList, 1, sizeof(DWORD) * object.dwNumChilds, hFile);
                  
                  object.pdwParentList = (DWORD *)malloc(sizeof(DWORD) * object.dwNumParents);
                  fread(object.pdwParentList, 1, sizeof(DWORD) * object.dwNumParents, hFile);
                  
                  object.pAccessList = (NXC_ACL_ENTRY *)malloc(sizeof(NXC_ACL_ENTRY) * object.dwAclSize);
                  fread(object.pAccessList, 1, sizeof(NXC_ACL_ENTRY) * object.dwAclSize, hFile);

                  fread(&dwSize, 1, sizeof(DWORD), hFile);
                  object.pszComments = (TCHAR *)malloc(dwSize + sizeof(TCHAR));
                  fread(object.pszComments, 1, dwSize, hFile);
                  object.pszComments[dwSize / sizeof(TCHAR)] = 0;

                  switch(object.iClass)
                  {
                     case OBJECT_NETWORKSERVICE:
                        fread(&dwSize, 1, sizeof(DWORD), hFile);
                        object.netsrv.pszRequest = (TCHAR *)malloc(dwSize + sizeof(TCHAR));
                        fread(object.netsrv.pszRequest, 1, dwSize, hFile);
                        object.netsrv.pszRequest[dwSize / sizeof(TCHAR)] = 0;

                        fread(&dwSize, 1, sizeof(DWORD), hFile);
                        object.netsrv.pszResponse = (TCHAR *)malloc(dwSize + sizeof(TCHAR));
                        fread(object.netsrv.pszResponse, 1, dwSize, hFile);
                        object.netsrv.pszResponse[dwSize / sizeof(TCHAR)] = 0;
                        break;
                     case OBJECT_ZONE:
                        if (object.zone.dwAddrListSize > 0)
                        {
                           object.zone.pdwAddrList = (DWORD *)malloc(object.zone.dwAddrListSize * sizeof(DWORD));
                           fread(object.zone.pdwAddrList, sizeof(DWORD), object.zone.dwAddrListSize, hFile);
                        }
                        else
                        {
                           object.zone.pdwAddrList = NULL;
                        }
                        break;
                     case OBJECT_VPNCONNECTOR:
                        dwSize = object.vpnc.dwNumLocalNets * sizeof(IP_NETWORK);
                        object.vpnc.pLocalNetList = (IP_NETWORK *)malloc(dwSize);
                        fread(object.vpnc.pLocalNetList, 1, dwSize, hFile);

                        dwSize = object.vpnc.dwNumRemoteNets * sizeof(IP_NETWORK);
                        object.vpnc.pRemoteNetList = (IP_NETWORK *)malloc(dwSize);
                        fread(object.vpnc.pRemoteNetList, 1, dwSize, hFile);
                        break;
                     case OBJECT_CONDITION:
                        fread(&dwSize, 1, sizeof(DWORD), hFile);
                        object.cond.pszScript = (TCHAR *)malloc(dwSize + sizeof(TCHAR));
                        fread(object.cond.pszScript, 1, dwSize, hFile);
                        object.cond.pszScript[dwSize / sizeof(TCHAR)] = 0;

                        dwSize = object.cond.dwNumDCI * sizeof(INPUT_DCI);
                        object.cond.pDCIList = (INPUT_DCI *)malloc(dwSize);
                        fread(object.cond.pDCIList, 1, dwSize, hFile);
                        break;
                     default:
                        break;
                  }

                  AddObject((NXC_OBJECT *)nx_memdup(&object, sizeof(NXC_OBJECT)), FALSE);
               }
            }
            LockObjectIndex();
            qsort(m_pIndexById, m_dwNumObjects, sizeof(INDEX), IndexCompare);
            UnlockObjectIndex();
            m_dwFlags |= NXC_SF_HAS_OBJECT_CACHE;
         }
      }

      fclose(hFile);
   }
}


//
// Change node's IP address
//

DWORD LIBNXCL_EXPORTABLE NXCChangeNodeIP(NXC_SESSION hSession, DWORD dwNodeId, DWORD dwIpAddr)
{
   DWORD dwRqId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_CHANGE_IP_ADDR);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   msg.SetVariable(VID_IP_ADDRESS, dwIpAddr);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId, 300000);
}


//
// Read agent's configuration file
//

DWORD LIBNXCL_EXPORTABLE NXCGetAgentConfig(NXC_SESSION hSession, DWORD dwNodeId,
                                           TCHAR **ppszConfig)
{
   CSCPMessage msg, *pResponse;
   DWORD dwRqId, dwRetCode;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   *ppszConfig = NULL;

   // Build request message
   msg.SetCode(CMD_GET_AGENT_CONFIG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   // Wait for response
   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 60000);
   if (pResponse != NULL)
   {
      dwRetCode = pResponse->GetVariableLong(VID_RCC);
      if (dwRetCode == RCC_SUCCESS)
      {
         *ppszConfig = pResponse->GetVariableStr(VID_CONFIG_FILE);
      }
      delete pResponse;
   }
   else
   {
      dwRetCode = RCC_TIMEOUT;
   }

   return dwRetCode;
}


//
// Update agent's configuration file
//

DWORD LIBNXCL_EXPORTABLE NXCUpdateAgentConfig(NXC_SESSION hSession, DWORD dwNodeId,
                                              TCHAR *pszConfig, BOOL bApply)
{
   CSCPMessage msg;
   DWORD dwRqId;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   // Build request message
   msg.SetCode(CMD_UPDATE_AGENT_CONFIG);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwNodeId);
   msg.SetVariable(VID_CONFIG_FILE, pszConfig);
   msg.SetVariable(VID_APPLY_FLAG, (WORD)bApply);

   // Send request
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId, 60000);
}


//
// Execute action on agent
//

DWORD LIBNXCL_EXPORTABLE NXCExecuteAction(NXC_SESSION hSession, DWORD dwObjectId,
                                          TCHAR *pszAction)
{
   DWORD dwRqId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_EXECUTE_ACTION);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_ACTION_NAME, pszAction);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Get object's comments
//

DWORD LIBNXCL_EXPORTABLE NXCGetObjectComments(NXC_SESSION hSession,
                                              DWORD dwObjectId, TCHAR **ppszText)
{
   DWORD dwRqId, dwResult;
   CSCPMessage msg, *pResponse;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_OBJECT_COMMENTS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
         *ppszText = pResponse->GetVariableStr(VID_COMMENT);
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}


//
// Update object's comments
//

DWORD LIBNXCL_EXPORTABLE NXCUpdateObjectComments(NXC_SESSION hSession,
                                                 DWORD dwObjectId, TCHAR *pszText)
{
   DWORD dwRqId;
   CSCPMessage msg;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_UPDATE_OBJECT_COMMENTS);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   msg.SetVariable(VID_COMMENT, pszText);
   ((NXCL_Session *)hSession)->SendMsg(&msg);
   return ((NXCL_Session *)hSession)->WaitForRCC(dwRqId);
}


//
// Actual check for parent-child relation
//

static BOOL IsParent(NXC_SESSION hSession, DWORD dwParent, DWORD dwChild)
{
   DWORD i;
   NXC_OBJECT *pObject;
   BOOL bRet = FALSE;

   pObject = ((NXCL_Session *)hSession)->FindObjectById(dwChild, FALSE);
   if (pObject != NULL)
   {
      for(i = 0; i < pObject->dwNumParents; i++)
      {
         if (pObject->pdwParentList[i] == dwParent)
         {
            bRet = TRUE;
            break;
         }
      }

      if (!bRet)
      {
         for(i = 0; i < pObject->dwNumParents; i++)
         {
            if (IsParent(hSession, dwParent, pObject->pdwParentList[i]))
            {
               bRet = TRUE;
               break;
            }
         }
      }
   }
   return bRet;
}


//
// Check if first object is a parent of second object
//

BOOL LIBNXCL_EXPORTABLE NXCIsParent(NXC_SESSION hSession, DWORD dwParent, DWORD dwChild)
{
   BOOL bRet;

   ((NXCL_Session *)hSession)->LockObjectIndex();
   bRet = IsParent(hSession, dwParent, dwChild);
   ((NXCL_Session *)hSession)->UnlockObjectIndex();
   return bRet;
}


//
// Get list of events used by template's or node's DCIs
//

DWORD LIBNXCL_EXPORTABLE NXCGetDCIEventsList(NXC_SESSION hSession, DWORD dwObjectId,
                                             DWORD **ppdwList, DWORD *pdwListSize)
{
   DWORD dwRqId, dwResult;
   CSCPMessage msg, *pResponse;

   *ppdwList = NULL;
   *pdwListSize = 0;

   dwRqId = ((NXCL_Session *)hSession)->CreateRqId();

   msg.SetCode(CMD_GET_DCI_EVENTS_LIST);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_OBJECT_ID, dwObjectId);
   ((NXCL_Session *)hSession)->SendMsg(&msg);

   pResponse = ((NXCL_Session *)hSession)->WaitForMessage(CMD_REQUEST_COMPLETED, dwRqId);
   if (pResponse != NULL)
   {
      dwResult = pResponse->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
         *pdwListSize = pResponse->GetVariableLong(VID_NUM_EVENTS);
         if (*pdwListSize > 0)
         {
            *ppdwList = (DWORD *)malloc(sizeof(DWORD) * (*pdwListSize));
            pResponse->GetVariableInt32Array(VID_EVENT_LIST, *pdwListSize, *ppdwList);
         }
      }
      delete pResponse;
   }
   else
   {
      dwResult = RCC_TIMEOUT;
   }
   return dwResult;
}
