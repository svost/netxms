/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: session.cpp
**
**/

#include "nxagentd.h"


//
// Externals
//

void UnregisterSession(DWORD dwIndex);


//
// Constants
//

#define RAW_MSG_SIZE    262144


//
// Client communication read thread
//

THREAD_RESULT THREAD_CALL CommSession::ReadThreadStarter(void *pArg)
{
   ((CommSession *)pArg)->ReadThread();

   // When CommSession::ReadThread exits, all other session
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterSession(((CommSession *)pArg)->GetIndex());
   delete (CommSession *)pArg;
   return THREAD_OK;
}


//
// Client communication write thread
//

THREAD_RESULT THREAD_CALL CommSession::WriteThreadStarter(void *pArg)
{
   ((CommSession *)pArg)->WriteThread();
   return THREAD_OK;
}


//
// Received message processing thread
//

THREAD_RESULT THREAD_CALL CommSession::ProcessingThreadStarter(void *pArg)
{
   ((CommSession *)pArg)->ProcessingThread();
   return THREAD_OK;
}


//
// Client session class constructor
//

CommSession::CommSession(SOCKET hSocket, DWORD dwHostAddr,
                         BOOL bInstallationServer, BOOL bControlServer)
{
   m_pSendQueue = new Queue;
   m_pMessageQueue = new Queue;
   m_hSocket = hSocket;
   m_dwIndex = INVALID_INDEX;
   m_pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   m_hWriteThread = INVALID_THREAD_HANDLE;
   m_hProcessingThread = INVALID_THREAD_HANDLE;
   m_dwHostAddr = dwHostAddr;
   m_bIsAuthenticated = (g_dwFlags & AF_REQUIRE_AUTH) ? FALSE : TRUE;
   m_bInstallationServer = bInstallationServer;
   m_bControlServer = bControlServer;
   m_hCurrFile = -1;
   m_pCtx = NULL;
   m_ts = time(NULL);
}


//
// Destructor
//

CommSession::~CommSession()
{
   shutdown(m_hSocket, SHUT_RDWR);
   closesocket(m_hSocket);
   delete m_pSendQueue;
   delete m_pMessageQueue;
   safe_free(m_pMsgBuffer);
   if (m_hCurrFile != -1)
      close(m_hCurrFile);
   DestroyEncryptionContext(m_pCtx);
}


//
// Start all threads
//

void CommSession::Run(void)
{
   m_hWriteThread = ThreadCreateEx(WriteThreadStarter, 0, this);
   m_hProcessingThread = ThreadCreateEx(ProcessingThreadStarter, 0, this);
   ThreadCreate(ReadThreadStarter, 0, this);
}


//
// Disconnect session
//

void CommSession::Disconnect(void)
{
   shutdown(m_hSocket, SHUT_RDWR);
}


//
// Reading thread
//

void CommSession::ReadThread(void)
{
   CSCP_MESSAGE *pRawMsg;
   CSCPMessage *pMsg;
   BYTE *pDecryptionBuffer = NULL;
   int iErr;
   char szBuffer[256];
   WORD wFlags;

   // Initialize raw message receiving function
   RecvCSCPMessage(0, NULL, m_pMsgBuffer, 0, NULL, NULL);

   pRawMsg = (CSCP_MESSAGE *)malloc(RAW_MSG_SIZE);
#ifdef _WITH_ENCRYPTION
   pDecryptionBuffer = (BYTE *)malloc(RAW_MSG_SIZE);
#endif
   while(1)
   {
      if ((iErr = RecvCSCPMessage(m_hSocket, pRawMsg, m_pMsgBuffer, RAW_MSG_SIZE,
                                  &m_pCtx, pDecryptionBuffer)) <= 0)
         break;

      // Check if message is too large
      if (iErr == 1)
         continue;

      // Check for decryption failure
      if (iErr == 2)
      {
         WriteLog(MSG_DECRYPTION_FAILURE, EVENTLOG_WARNING_TYPE, NULL);
         continue;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         DebugPrintf("Actual message size doesn't match wSize value (%d,%d)", iErr, ntohl(pRawMsg->dwSize));
         continue;   // Bad packet, wait for next
      }

      // Update activity timestamp
      m_ts = time(NULL);

      wFlags = ntohs(pRawMsg->wFlags);
      if (wFlags & MF_BINARY)
      {
         // Convert message header to host format
         pRawMsg->dwId = ntohl(pRawMsg->dwId);
         pRawMsg->wCode = ntohs(pRawMsg->wCode);
         pRawMsg->dwNumVars = ntohl(pRawMsg->dwNumVars);
         DebugPrintf("Received raw message %s", CSCPMessageCodeName(pRawMsg->wCode, szBuffer));

         if (pRawMsg->wCode == CMD_FILE_DATA)
         {
            if ((m_hCurrFile != -1) && (m_dwFileRqId == pRawMsg->dwId))
            {
               if (write(m_hCurrFile, pRawMsg->df, pRawMsg->dwNumVars) == (int)pRawMsg->dwNumVars)
               {
                  if (wFlags & MF_EOF)
                  {
                     CSCPMessage msg;

                     close(m_hCurrFile);
                     m_hCurrFile = -1;
                  
                     msg.SetCode(CMD_REQUEST_COMPLETED);
                     msg.SetId(pRawMsg->dwId);
                     msg.SetVariable(VID_RCC, ERR_SUCCESS);
                     SendMessage(&msg);
                  }
               }
               else
               {
                  // I/O error
                  CSCPMessage msg;

                  close(m_hCurrFile);
                  m_hCurrFile = -1;
               
                  msg.SetCode(CMD_REQUEST_COMPLETED);
                  msg.SetId(pRawMsg->dwId);
                  msg.SetVariable(VID_RCC, ERR_IO_FAILURE);
                  SendMessage(&msg);
               }
            }
         }
      }
      else
      {
         // Create message object from raw message
         pMsg = new CSCPMessage(pRawMsg);
         if (pMsg->GetCode() == CMD_REQUEST_SESSION_KEY)
         {
            DebugPrintf("Received message %s", CSCPMessageCodeName(pMsg->GetCode(), szBuffer));
            if (m_pCtx == NULL)
            {
               CSCPMessage *pResponce;

               SetupEncryptionContext(pMsg, &m_pCtx, &pResponce, NULL);
               SendMessage(pResponce);
               delete pResponce;
            }
            delete pMsg;
         }
         else
         {
            m_pMessageQueue->Put(pMsg);
         }
      }
   }
   if (iErr < 0)
      WriteLog(MSG_SESSION_BROKEN, EVENTLOG_WARNING_TYPE, "e", WSAGetLastError());
   free(pRawMsg);
#ifdef _WITH_ENCRYPTION
   free(pDecryptionBuffer);
#endif

   // Notify other threads to exit
   m_pSendQueue->Put(INVALID_POINTER_VALUE);
   m_pMessageQueue->Put(INVALID_POINTER_VALUE);

   // Wait for other threads to finish
   ThreadJoin(m_hWriteThread);
   ThreadJoin(m_hProcessingThread);

   DebugPrintf("Session with %s closed", IpToStr(m_dwHostAddr, szBuffer));
}


//
// Writing thread
//

void CommSession::WriteThread(void)
{
   CSCP_MESSAGE *pMsg;
   char szBuffer[128];

   while(1)
   {
      pMsg = (CSCP_MESSAGE *)m_pSendQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      DebugPrintf("Sending message %s", CSCPMessageCodeName(ntohs(pMsg->wCode), szBuffer));
      if (m_pCtx != NULL)
      {
         CSCP_ENCRYPTED_MESSAGE *pEnMsg;

         pEnMsg = CSCPEncryptMessage(m_pCtx, pMsg);
         free(pMsg);
         if (pEnMsg != NULL)
         {
            if (SendEx(m_hSocket, (const char *)pEnMsg, ntohl(pEnMsg->dwSize), 0) <= 0)
            {
               free(pEnMsg);
               break;
            }
            free(pEnMsg);
         }
      }
      else
      {
         if (SendEx(m_hSocket, (const char *)pMsg, ntohl(pMsg->dwSize), 0) <= 0)
         {
            free(pMsg);
            break;
         }
         free(pMsg);
      }
   }
}


//
// Message processing thread
//

void CommSession::ProcessingThread(void)
{
   CSCPMessage *pMsg;
   char szBuffer[128];
   CSCPMessage msg;
   DWORD dwCommand;

   while(1)
   {
      pMsg = (CSCPMessage *)m_pMessageQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      dwCommand = pMsg->GetCode();
      DebugPrintf("Received message %s", CSCPMessageCodeName((WORD)dwCommand, szBuffer));

      // Prepare responce message
      msg.SetCode(CMD_REQUEST_COMPLETED);
      msg.SetId(pMsg->GetId());

      // Check if authentication required
      if ((!m_bIsAuthenticated) && (dwCommand != CMD_AUTHENTICATE))
      {
         msg.SetVariable(VID_RCC, ERR_AUTH_REQUIRED);
      }
      else if ((g_dwFlags & AF_REQUIRE_ENCRYPTION) && (m_pCtx == NULL))
      {
         msg.SetVariable(VID_RCC, ERR_ENCRYPTION_REQUIRED);
      }
      else
      {
         switch(dwCommand)
         {
            case CMD_AUTHENTICATE:
               Authenticate(pMsg, &msg);
               break;
            case CMD_GET_PARAMETER:
               GetParameter(pMsg, &msg);
               break;
            case CMD_GET_LIST:
               GetList(pMsg, &msg);
               break;
            case CMD_KEEPALIVE:
               msg.SetVariable(VID_RCC, ERR_SUCCESS);
               break;
            case CMD_ACTION:
               Action(pMsg, &msg);
               break;
            case CMD_TRANSFER_FILE:
               RecvFile(pMsg, &msg);
               break;
            case CMD_UPGRADE_AGENT:
               msg.SetVariable(VID_RCC, Upgrade(pMsg));
               break;
            case CMD_GET_PARAMETER_LIST:
               msg.SetVariable(VID_RCC, ERR_SUCCESS);
               GetParameterList(&msg);
               break;
            case CMD_GET_AGENT_CONFIG:
               GetConfig(&msg);
               break;
            case CMD_UPDATE_AGENT_CONFIG:
               msg.SetVariable(VID_RCC, ERR_NOT_IMPLEMENTED);
               break;
            default:
               // Attempt to process unknown command by subagents
               if (!ProcessCmdBySubAgent(dwCommand, pMsg, &msg))
                  msg.SetVariable(VID_RCC, ERR_UNKNOWN_COMMAND);
               break;
         }
      }
      delete pMsg;

      // Send responce
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }
}


//
// Authenticate peer
//

void CommSession::Authenticate(CSCPMessage *pRequest, CSCPMessage *pMsg)
{
   if (m_bIsAuthenticated)
   {
      // Already authenticated
      pMsg->SetVariable(VID_RCC, (g_dwFlags & AF_REQUIRE_AUTH) ? ERR_ALREADY_AUTHENTICATED : ERR_AUTH_NOT_REQUIRED);
   }
   else
   {
      char szSecret[MAX_SECRET_LENGTH];
      BYTE hash[32];
      WORD wAuthMethod;

      wAuthMethod = pRequest->GetVariableShort(VID_AUTH_METHOD);
      switch(wAuthMethod)
      {
         case AUTH_PLAINTEXT:
            pRequest->GetVariableStr(VID_SHARED_SECRET, szSecret, MAX_SECRET_LENGTH);
            if (!strcmp(szSecret, g_szSharedSecret))
            {
               m_bIsAuthenticated = TRUE;
               pMsg->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               WriteLog(MSG_AUTH_FAILED, EVENTLOG_WARNING_TYPE, "as", m_dwHostAddr, "PLAIN");
               pMsg->SetVariable(VID_RCC, ERR_AUTH_FAILED);
            }
            break;
         case AUTH_MD5_HASH:
            pRequest->GetVariableBinary(VID_SHARED_SECRET, (BYTE *)szSecret, MD5_DIGEST_SIZE);
            CalculateMD5Hash((BYTE *)g_szSharedSecret, strlen(g_szSharedSecret), hash);
            if (!memcmp(szSecret, hash, MD5_DIGEST_SIZE))
            {
               m_bIsAuthenticated = TRUE;
               pMsg->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               WriteLog(MSG_AUTH_FAILED, EVENTLOG_WARNING_TYPE, "as", m_dwHostAddr, "MD5");
               pMsg->SetVariable(VID_RCC, ERR_AUTH_FAILED);
            }
            break;
         case AUTH_SHA1_HASH:
            pRequest->GetVariableBinary(VID_SHARED_SECRET, (BYTE *)szSecret, SHA1_DIGEST_SIZE);
            CalculateSHA1Hash((BYTE *)g_szSharedSecret, strlen(g_szSharedSecret), hash);
            if (!memcmp(szSecret, hash, SHA1_DIGEST_SIZE))
            {
               m_bIsAuthenticated = TRUE;
               pMsg->SetVariable(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               WriteLog(MSG_AUTH_FAILED, EVENTLOG_WARNING_TYPE, "as", m_dwHostAddr, "SHA1");
               pMsg->SetVariable(VID_RCC, ERR_AUTH_FAILED);
            }
            break;
         default:
            pMsg->SetVariable(VID_RCC, ERR_NOT_IMPLEMENTED);
            break;
      }
   }
}


//
// Get parameter's value
//

void CommSession::GetParameter(CSCPMessage *pRequest, CSCPMessage *pMsg)
{
   char szParameter[MAX_PARAM_NAME], szValue[MAX_RESULT_LENGTH];
   DWORD dwErrorCode;

   pRequest->GetVariableStr(VID_PARAMETER, szParameter, MAX_PARAM_NAME);
   dwErrorCode = GetParameterValue(szParameter, szValue);
   pMsg->SetVariable(VID_RCC, dwErrorCode);
   if (dwErrorCode == ERR_SUCCESS)
      pMsg->SetVariable(VID_VALUE, szValue);
}


//
// Get list of values
//

void CommSession::GetList(CSCPMessage *pRequest, CSCPMessage *pMsg)
{
   char szParameter[MAX_PARAM_NAME];
   DWORD i, dwErrorCode;
   NETXMS_VALUES_LIST value;

   pRequest->GetVariableStr(VID_PARAMETER, szParameter, MAX_PARAM_NAME);
   value.dwNumStrings = 0;
   value.ppStringList = NULL;

   dwErrorCode = GetEnumValue(szParameter, &value);
   pMsg->SetVariable(VID_RCC, dwErrorCode);
   if (dwErrorCode == ERR_SUCCESS)
   {
      pMsg->SetVariable(VID_NUM_STRINGS, value.dwNumStrings);
      for(i = 0; i < value.dwNumStrings; i++)
         pMsg->SetVariable(VID_ENUM_VALUE_BASE + i, value.ppStringList[i]);
   }

   for(i = 0; i < value.dwNumStrings; i++)
      safe_free(value.ppStringList[i]);
   safe_free(value.ppStringList);
}


//
// Perform action on request
//

void CommSession::Action(CSCPMessage *pRequest, CSCPMessage *pMsg)
{
   char szAction[MAX_PARAM_NAME];
   NETXMS_VALUES_LIST args;
   DWORD i, dwRetCode;

   if ((g_dwFlags & AF_ENABLE_ACTIONS) && m_bControlServer)
   {
      // Get action name and arguments
      pRequest->GetVariableStr(VID_ACTION_NAME, szAction, MAX_PARAM_NAME);
      args.dwNumStrings = pRequest->GetVariableLong(VID_NUM_ARGS);
      args.ppStringList = (char **)malloc(sizeof(char *) * args.dwNumStrings);
      for(i = 0; i < args.dwNumStrings; i++)
         args.ppStringList[i] = pRequest->GetVariableStr(VID_ACTION_ARG_BASE + i);

      // Execute action
      dwRetCode = ExecAction(szAction, &args);
      pMsg->SetVariable(VID_RCC, dwRetCode);

      // Cleanup
      for(i = 0; i < args.dwNumStrings; i++)
         safe_free(args.ppStringList[i]);
      safe_free(args.ppStringList);
   }
   else
   {
      pMsg->SetVariable(VID_RCC, ERR_ACCESS_DENIED);
   }
}


//
// Prepare for receiving file
//

void CommSession::RecvFile(CSCPMessage *pRequest, CSCPMessage *pMsg)
{
   TCHAR szFileName[MAX_PATH], szFullPath[MAX_PATH];

   if (m_bInstallationServer)
   {
      szFileName[0] = 0;
      pRequest->GetVariableStr(VID_FILE_NAME, szFileName, MAX_PATH);
      DebugPrintf("Preparing for receiving file \"%s\"", szFileName);
      BuildFullPath(szFileName, szFullPath);

      // Check if for some reason we have already opened file
      if (m_hCurrFile != -1)
      {
         pMsg->SetVariable(VID_RCC, ERR_RESOURCE_BUSY);
      }
      else
      {
         DebugPrintf("Writing to local file \"%s\"", szFullPath);
         m_hCurrFile = _topen(szFullPath, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0600);
         if (m_hCurrFile == -1)
         {
            DebugPrintf("Error opening file for writing: %s", strerror(errno));
            pMsg->SetVariable(VID_RCC, ERR_IO_FAILURE);
         }
         else
         {
            m_dwFileRqId = pRequest->GetId();
            pMsg->SetVariable(VID_RCC, ERR_SUCCESS);
         }
      }
   }
   else
   {
      pMsg->SetVariable(VID_RCC, ERR_ACCESS_DENIED);
   }
}


//
// Upgrade agent from package in the file store
//

DWORD CommSession::Upgrade(CSCPMessage *pRequest)
{
   if (m_bInstallationServer)
   {
      TCHAR szPkgName[MAX_PATH], szFullPath[MAX_PATH];

      szPkgName[0] = 0;
      pRequest->GetVariableStr(VID_FILE_NAME, szPkgName, MAX_PATH);
      BuildFullPath(szPkgName, szFullPath);
      return UpgradeAgent(szFullPath);
   }
   else
   {
      return ERR_ACCESS_DENIED;
   }
}


//
// Get agent's configuration file
//

void CommSession::GetConfig(CSCPMessage *pMsg)
{
   if (m_bInstallationServer)
   {
      pMsg->SetVariable(VID_RCC, 
         pMsg->SetVariableFromFile(VID_CONFIG_FILE, g_szConfigFile) ? ERR_SUCCESS : ERR_IO_FAILURE);
   }
   else
   {
      pMsg->SetVariable(VID_RCC, ERR_ACCESS_DENIED);
   }
}
