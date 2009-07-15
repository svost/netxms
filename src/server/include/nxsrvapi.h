/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: nxsrvapi.h
**
**/

#ifndef _nxsrvapi_h_
#define _nxsrvapi_h_

#ifdef _WIN32
#ifdef LIBNXSRV_EXPORTS
#define LIBNXSRV_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXSRV_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXSRV_EXPORTABLE
#endif

#ifndef LIBNXCL_NO_DECLARATIONS
#define LIBNXCL_NO_DECLARATIONS 1
#endif
#include <nxclapi.h>
#include <nxcpapi.h>
#include <nms_agent.h>
#include "../libnxsrv/messages.h"
#include <dbdrv.h>
#include <nxsnmp.h>
#include <netxms_isc.h>


//
// Default files
//

#ifdef _WIN32

# define DEFAULT_CONFIG_FILE   _T("C:\\netxmsd.conf")

# define DEFAULT_SHELL         "cmd.exe"
# define DEFAULT_LOG_FILE      "C:\\NetXMS.log"
# define DEFAULT_DATA_DIR      "C:\\NetXMS\\var"
# define DEFAULT_DUMP_DIR      "C:\\"

# define DDIR_MIBS             "\\mibs"
# define DDIR_PACKAGES         "\\packages"
# define DDIR_BACKGROUNDS      "\\backgrounds"
# define DDIR_SHARED_FILES     "\\shared"
# define DFILE_KEYS            "\\server_key"
# define DFILE_COMPILED_MIB    "\\mibs\\netxms.mib"

#else    /* _WIN32 */

# define DEFAULT_CONFIG_FILE   _T("{search}")

# define DEFAULT_SHELL         "/bin/sh"

# ifndef DATADIR
#  define DATADIR              "/var/netxms"
# endif

# define DEFAULT_LOG_FILE      DATADIR"/log/netxmsd.log"
# define DEFAULT_DATA_DIR      DATADIR
# define DEFAULT_DUMP_DIR      "/"

# define DDIR_MIBS             "/mibs"
# define DDIR_PACKAGES         "/packages"
# define DDIR_BACKGROUNDS      "/backgrounds"
# define DDIR_SHARED_FILES     "/shared"
# define DFILE_KEYS            "/.server_key"
# define DFILE_COMPILED_MIB    "/mibs/netxms.mib"

#endif   /* _WIN32 */


//
// Application flags
//

#define AF_DAEMON                         0x00000001
#define AF_USE_SYSLOG                     0x00000002
#define AF_ENABLE_NETWORK_DISCOVERY       0x00000004
#define AF_ACTIVE_NETWORK_DISCOVERY       0x00000008
#define AF_LOG_SQL_ERRORS                 0x00000010
#define AF_DELETE_EMPTY_SUBNETS           0x00000020
#define AF_ENABLE_SNMP_TRAPD              0x00000040
#define AF_ENABLE_ZONING                  0x00000080
#define AF_SYNC_NODE_NAMES_WITH_DNS       0x00000100
#define AF_CHECK_TRUSTED_NODES            0x00000200
#define AF_RESOLVE_NODE_NAMES             0x00100000
#define AF_CATCH_EXCEPTIONS               0x00200000
#define AF_INTERNAL_CA                    0x00400000
#define AF_DB_LOCKED                      0x01000000
#define AF_ENABLE_MULTIPLE_DB_CONN        0x02000000
#define AF_DB_CONNECTION_LOST             0x04000000
#define AF_NO_NETWORK_CONNECTIVITY        0x08000000
#define AF_EVENT_STORM_DETECTED           0x08000000
#define AF_SERVER_INITIALIZED             0x40000000
#define AF_SHUTDOWN                       0x80000000

#define IsStandalone()                    (!(g_dwFlags & AF_DAEMON))
#define ShutdownInProgress()              (g_dwFlags & AF_SHUTDOWN)


//
// Encryption usage policies
//

#define ENCRYPTION_DISABLED   0
#define ENCRYPTION_ALLOWED    1
#define ENCRYPTION_PREFERRED  2
#define ENCRYPTION_REQUIRED   3


//
// DB-related constants
//

#define MAX_DB_LOGIN          64
#define MAX_DB_PASSWORD       64
#define MAX_DB_NAME           256


//
// DB events
//

#define DBEVENT_CONNECTION_LOST        0
#define DBEVENT_CONNECTION_RESTORED    1
#define DBEVENT_QUERY_FAILED           2


//
// Database syntax codes
//

#define DB_SYNTAX_MYSQL    0
#define DB_SYNTAX_PGSQL    1
#define DB_SYNTAX_MSSQL    2
#define DB_SYNTAX_ORACLE   3
#define DB_SYNTAX_SQLITE   4
#define DB_SYNTAX_UNKNOWN	-1


//
// Database connection structure
//

struct db_handle_t
{
   DB_CONNECTION hConn;
   MUTEX mutexTransLock;      // Transaction lock
   int nTransactionLevel;
   TCHAR *pszServer;
   TCHAR *pszLogin;
   TCHAR *pszPassword;
   TCHAR *pszDBName;
};
typedef db_handle_t * DB_HANDLE;


//
// Win32 service and syslog constants
//

#ifdef _WIN32

#define CORE_SERVICE_NAME     _T("NetXMSCore")
#define CORE_EVENT_SOURCE     _T("NetXMSCore")
#define NETXMSD_SYSLOG_NAME   CORE_EVENT_SOURCE

#else

#define NETXMSD_SYSLOG_NAME   _T("netxmsd")

#endif   /* _WIN32 */


//
// Single ARP cache entry
//

typedef struct
{
   DWORD dwIndex;       // Interface index
   DWORD dwIpAddr;
   BYTE bMacAddr[MAC_ADDR_LENGTH];
} ARP_ENTRY;


//
// ARP cache structure used by discovery functions and AgentConnection class
//

typedef struct
{
   DWORD dwNumEntries;
   ARP_ENTRY *pEntries;
} ARP_CACHE;


//
// Interface information structure used by discovery functions and AgentConnection class
//

typedef struct
{
   TCHAR szName[MAX_DB_STRING];
   DWORD dwIndex;
   DWORD dwType;
   DWORD dwIpAddr;
   DWORD dwIpNetMask;
   BYTE bMacAddr[MAC_ADDR_LENGTH];
   int iNumSecondary;      // Number of secondary IP's on this interface
} INTERFACE_INFO;


//
// Interface list used by discovery functions and AgentConnection class
//

typedef struct
{
   int iNumEntries;              // Number of entries in pInterfaces
   int iEnumPos;                 // Used by index enumeration handler
   void *pArg;                   // Can be used by custom enumeration handlers
   INTERFACE_INFO *pInterfaces;  // Interface entries
} INTERFACE_LIST;


//
// Route information
//

typedef struct
{
   DWORD dwDestAddr;
   DWORD dwDestMask;
   DWORD dwNextHop;
   DWORD dwIfIndex;
   DWORD dwRouteType;
} ROUTE;


//
// Routing table
//

typedef struct
{
   int iNumEntries;     // Number of entries
   ROUTE *pRoutes;      // Route list
} ROUTING_TABLE;


//
// Agent connection
//

class LIBNXSRV_EXPORTABLE AgentConnection
{
private:
   DWORD m_dwAddr;
   int m_nProtocolVersion;
   int m_iAuthMethod;
   TCHAR m_szSecret[MAX_SECRET_LENGTH];
   time_t m_tLastCommandTime;
   SOCKET m_hSocket;
   DWORD m_dwNumDataLines;
   DWORD m_dwRequestId;
   DWORD m_dwCommandTimeout;
   DWORD m_dwRecvTimeout;
   TCHAR **m_ppDataLines;
   MsgWaitQueue *m_pMsgWaitQueue;
   BOOL m_bIsConnected;
   MUTEX m_mutexDataLock;
   THREAD m_hReceiverThread;
   CSCP_ENCRYPTION_CONTEXT *m_pCtx;
   int m_iEncryptionPolicy;
   BOOL m_bUseProxy;
   DWORD m_dwProxyAddr;
   WORD m_wPort;
   WORD m_wProxyPort;
   int m_iProxyAuth;
   TCHAR m_szProxySecret[MAX_SECRET_LENGTH];
	int m_hCurrFile;
	TCHAR m_currentFileName[MAX_PATH];
	DWORD m_dwDownloadRequestId;
	CONDITION m_condFileDownload;
	BOOL m_fileDownloadSucceeded;
	void (*m_downloadProgressCallback)(size_t, void *);
	void *m_downloadProgressCallbackArg;
	bool m_deleteFileOnDownloadFailure;

   void ReceiverThread(void);
   static THREAD_RESULT THREAD_CALL ReceiverThreadStarter(void *);

protected:
   void DestroyResultData(void);
   BOOL SendMessage(CSCPMessage *pMsg);
   CSCPMessage *WaitForMessage(WORD wCode, DWORD dwId, DWORD dwTimeOut) { return m_pMsgWaitQueue->WaitForMessage(wCode, dwId, dwTimeOut); }
   DWORD WaitForRCC(DWORD dwRqId, DWORD dwTimeOut);
   DWORD SetupEncryption(RSA *pServerKey);
   DWORD Authenticate(BOOL bProxyData);
   DWORD SetupProxyConnection(void);
   DWORD GetIpAddr(void) { return ntohl(m_dwAddr); }
	DWORD PrepareFileDownload(const TCHAR *fileName, DWORD rqId, bool append, void (*downloadProgressCallback)(size_t, void *), void *cbArg);

	DWORD generateRequestId() { return m_dwRequestId++; }

   virtual void PrintMsg(const TCHAR *pszFormat, ...);
   virtual void OnTrap(CSCPMessage *pMsg);
	virtual void OnFileDownload(BOOL success);

   void Lock(void) { MutexLock(m_mutexDataLock, INFINITE); }
   void Unlock(void) { MutexUnlock(m_mutexDataLock); }

public:
   AgentConnection();
   AgentConnection(DWORD dwAddr, WORD wPort = AGENT_LISTEN_PORT,
                   int iAuthMethod = AUTH_NONE, const TCHAR *pszSecret = NULL);
   virtual ~AgentConnection();

   BOOL Connect(RSA *pServerKey = NULL, BOOL bVerbose = FALSE, DWORD *pdwError = NULL);
   void Disconnect(void);
   BOOL IsConnected(void) { return m_bIsConnected; }
	int getProtocolVersion(void) { return m_nProtocolVersion; }

	SOCKET getSocket() { return m_hSocket; }

   ARP_CACHE *GetArpCache(void);
   INTERFACE_LIST *GetInterfaceList(void);
   ROUTING_TABLE *GetRoutingTable(void);
   DWORD GetParameter(const TCHAR *pszParam, DWORD dwBufSize, TCHAR *pszBuffer);
   DWORD GetList(const TCHAR *pszParam);
   DWORD nop(void);
   DWORD ExecAction(const TCHAR *pszAction, int argc, TCHAR **argv);
   DWORD UploadFile(const TCHAR *pszFile);
   DWORD StartUpgrade(const TCHAR *pszPkgName);
   DWORD CheckNetworkService(DWORD *pdwStatus, DWORD dwIpAddr, int iServiceType, WORD wPort = 0, 
                             WORD wProto = 0, const TCHAR *pszRequest = NULL, const TCHAR *pszResponse = NULL);
   DWORD GetSupportedParameters(DWORD *pdwNumParams, NXC_AGENT_PARAM **ppParamList);
   DWORD GetConfigFile(TCHAR **ppszConfig, DWORD *pdwSize);
   DWORD updateConfigFile(const TCHAR *pszConfig);
   DWORD enableTraps(void);
	CSCPMessage *customRequest(CSCPMessage *pRequest, const TCHAR *recvFile = NULL, bool appendFile = false,
	                           void (*downloadProgressCallback)(size_t, void *) = NULL, void *cbArg = NULL);

   DWORD GetNumDataLines(void) { return m_dwNumDataLines; }
   const TCHAR *GetDataLine(DWORD dwIndex) { return dwIndex < m_dwNumDataLines ? m_ppDataLines[dwIndex] : _T("(error)"); }

   void setCommandTimeout(DWORD dwTimeout) { m_dwCommandTimeout = max(dwTimeout, 500); }
	DWORD getCommandTimeout() { return m_dwCommandTimeout; }
   void SetRecvTimeout(DWORD dwTimeout) { m_dwRecvTimeout = max(dwTimeout, 10000); }
   void SetEncryptionPolicy(int iPolicy) { m_iEncryptionPolicy = iPolicy; }
   void SetProxy(DWORD dwAddr, WORD wPort = AGENT_LISTEN_PORT,
                 int iAuthMethod = AUTH_NONE, const TCHAR *pszSecret = NULL);
   void SetPort(WORD wPort) { m_wPort = wPort; }
   void SetAuthData(int nMethod, const char *pszSecret) { m_iAuthMethod = nMethod; nx_strncpy(m_szSecret, pszSecret, MAX_SECRET_LENGTH); }
	void setDeleteFileOnDownloadFailure(bool flag) { m_deleteFileOnDownloadFailure = flag; }
};


//
// Proxy SNMP transport
//

class LIBNXSRV_EXPORTABLE SNMP_ProxyTransport : public SNMP_Transport
{
protected:
	AgentConnection *m_pAgentConnection;
	CSCPMessage *m_pResponse;
	DWORD m_dwIpAddr;
	WORD m_wPort;

public:
	SNMP_ProxyTransport(AgentConnection *pConn, DWORD dwIpAddr, WORD wPort);
	virtual ~SNMP_ProxyTransport();

   virtual int readMessage(SNMP_PDU **ppData, DWORD dwTimeout = INFINITE,
                           struct sockaddr *pSender = NULL, socklen_t *piAddrSize = NULL);
   virtual int sendMessage(SNMP_PDU *pdu);
};


//
// ISC flags
//

#define ISCF_IS_CONNECTED        ((DWORD)0x00000001)
#define ISCF_REQUIRE_ENCRYPTION  ((DWORD)0x00000002)


//
// Inter-server connection (ISC)
//

class LIBNXSRV_EXPORTABLE ISC
{
private:
	DWORD m_flags;
   DWORD m_addr;
	WORD m_port;
   SOCKET m_socket;
   int m_protocolVersion;
	DWORD m_requestId;
	DWORD m_recvTimeout;
   MsgWaitQueue *m_msgWaitQueue;
   MUTEX m_mutexDataLock;
   THREAD m_hReceiverThread;
   CSCP_ENCRYPTION_CONTEXT *m_ctx;
	DWORD m_commandTimeout;

   void ReceiverThread(void);
   static THREAD_RESULT THREAD_CALL ReceiverThreadStarter(void *);

protected:
   void DestroyResultData(void);
   DWORD SetupEncryption(RSA *pServerKey);
	DWORD ConnectToService(DWORD service);

   void Lock(void) { MutexLock(m_mutexDataLock, INFINITE); }
   void Unlock(void) { MutexUnlock(m_mutexDataLock); }

   virtual void PrintMsg(const TCHAR *format, ...);

public:
   ISC();
   ISC(DWORD addr, WORD port = NETXMS_ISC_PORT);
   virtual ~ISC();

   DWORD Connect(DWORD service, RSA *serverKey = NULL, BOOL requireEncryption = FALSE);
	void Disconnect();

   BOOL SendMessage(CSCPMessage *msg);
   CSCPMessage *WaitForMessage(WORD code, DWORD id, DWORD timeOut) { return m_msgWaitQueue->WaitForMessage(code, id, timeOut); }
   DWORD WaitForRCC(DWORD rqId, DWORD timeOut);

   DWORD Nop(void);
};


//
// Functions
//

void LIBNXSRV_EXPORTABLE DestroyArpCache(ARP_CACHE *pArpCache);
void LIBNXSRV_EXPORTABLE DestroyInterfaceList(INTERFACE_LIST *pIfList);
void LIBNXSRV_EXPORTABLE DestroyRoutingTable(ROUTING_TABLE *pRT);
void LIBNXSRV_EXPORTABLE SortRoutingTable(ROUTING_TABLE *pRT);
const TCHAR LIBNXSRV_EXPORTABLE *AgentErrorCodeToText(int iError);

void LIBNXSRV_EXPORTABLE WriteLogOther(WORD wType, const TCHAR *format, ...);
void LIBNXSRV_EXPORTABLE DbgPrintf(int level, const TCHAR *format, ...);

BOOL LIBNXSRV_EXPORTABLE DBInit(BOOL bWriteLog, BOOL bLogErrors, BOOL bDumpSQL,
                                void (* fpEventHandler)(DWORD, const TCHAR *, const TCHAR *));
DB_HANDLE LIBNXSRV_EXPORTABLE DBConnect(void);
DB_HANDLE LIBNXSRV_EXPORTABLE DBConnectEx(const TCHAR *pszServer, const TCHAR *pszDBName,
                                          const TCHAR *pszLogin, const TCHAR *pszPassword);
void LIBNXSRV_EXPORTABLE DBDisconnect(DB_HANDLE hConn);
BOOL LIBNXSRV_EXPORTABLE DBQuery(DB_HANDLE hConn, const TCHAR *szQuery);
BOOL LIBNXSRV_EXPORTABLE DBQueryEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);
DB_RESULT LIBNXSRV_EXPORTABLE DBSelect(DB_HANDLE hConn, const TCHAR *szQuery);
DB_RESULT LIBNXSRV_EXPORTABLE DBSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);
DB_ASYNC_RESULT LIBNXSRV_EXPORTABLE DBAsyncSelect(DB_HANDLE hConn, const TCHAR *szQuery);
DB_ASYNC_RESULT LIBNXSRV_EXPORTABLE DBAsyncSelectEx(DB_HANDLE hConn, const TCHAR *szQuery, TCHAR *errorText);
BOOL LIBNXSRV_EXPORTABLE DBFetch(DB_ASYNC_RESULT hResult);
int LIBNXSRV_EXPORTABLE DBGetColumnCount(DB_RESULT hResult);
BOOL LIBNXSRV_EXPORTABLE DBGetColumnName(DB_RESULT hResult, int column, TCHAR *buffer, int bufSize);
int LIBNXSRV_EXPORTABLE DBGetColumnCountAsync(DB_ASYNC_RESULT hResult);
BOOL LIBNXSRV_EXPORTABLE DBGetColumnNameAsync(DB_ASYNC_RESULT hResult, int column, TCHAR *buffer, int bufSize);
TCHAR LIBNXSRV_EXPORTABLE *DBGetField(DB_RESULT hResult, int iRow, int iColumn,
                                      TCHAR *pszBuffer, int nBufLen);
LONG LIBNXSRV_EXPORTABLE DBGetFieldLong(DB_RESULT hResult, int iRow, int iColumn);
DWORD LIBNXSRV_EXPORTABLE DBGetFieldULong(DB_RESULT hResult, int iRow, int iColumn);
INT64 LIBNXSRV_EXPORTABLE DBGetFieldInt64(DB_RESULT hResult, int iRow, int iColumn);
QWORD LIBNXSRV_EXPORTABLE DBGetFieldUInt64(DB_RESULT hResult, int iRow, int iColumn);
double LIBNXSRV_EXPORTABLE DBGetFieldDouble(DB_RESULT hResult, int iRow, int iColumn);
DWORD LIBNXSRV_EXPORTABLE DBGetFieldIPAddr(DB_RESULT hResult, int iRow, int iColumn);
BOOL LIBNXSRV_EXPORTABLE DBGetFieldByteArray(DB_RESULT hResult, int iRow, int iColumn,
                                             int *pnArray, int nSize, int nDefault);
BOOL LIBNXSRV_EXPORTABLE DBGetFieldGUID(DB_RESULT hResult, int iRow,
                                        int iColumn, uuid_t guid);
TCHAR LIBNXSRV_EXPORTABLE *DBGetFieldAsync(DB_ASYNC_RESULT hResult, int iColumn, TCHAR *pBuffer, int iBufSize);
LONG LIBNXSRV_EXPORTABLE DBGetFieldAsyncLong(DB_RESULT hResult, int iColumn);
DWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncULong(DB_ASYNC_RESULT hResult, int iColumn);
INT64 LIBNXSRV_EXPORTABLE DBGetFieldAsyncInt64(DB_RESULT hResult, int iColumn);
QWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncUInt64(DB_ASYNC_RESULT hResult, int iColumn);
double LIBNXSRV_EXPORTABLE DBGetFieldAsyncDouble(DB_RESULT hResult, int iColumn);
DWORD LIBNXSRV_EXPORTABLE DBGetFieldAsyncIPAddr(DB_RESULT hResult, int iColumn);
int LIBNXSRV_EXPORTABLE DBGetNumRows(DB_RESULT hResult);
void LIBNXSRV_EXPORTABLE DBFreeResult(DB_RESULT hResult);
void LIBNXSRV_EXPORTABLE DBFreeAsyncResult(DB_HANDLE hConn, DB_ASYNC_RESULT hResult);
BOOL LIBNXSRV_EXPORTABLE DBBegin(DB_HANDLE hConn);
BOOL LIBNXSRV_EXPORTABLE DBCommit(DB_HANDLE hConn);
BOOL LIBNXSRV_EXPORTABLE DBRollback(DB_HANDLE hConn);
void LIBNXSRV_EXPORTABLE DBUnloadDriver(void);

int LIBNXSRV_EXPORTABLE DBGetSchemaVersion(DB_HANDLE conn);
int LIBNXSRV_EXPORTABLE DBGetSyntax(DB_HANDLE conn);

TCHAR LIBNXSRV_EXPORTABLE *EncodeSQLString(const TCHAR *pszIn);
void LIBNXSRV_EXPORTABLE DecodeSQLString(TCHAR *pszStr);

void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy);

const TCHAR LIBNXSRV_EXPORTABLE *ISCErrorCodeToText(DWORD code);


//
// Variables
//

extern DWORD LIBNXSRV_EXPORTABLE g_dwFlags;
extern int LIBNXSRV_EXPORTABLE g_nDebugLevel;
extern TCHAR LIBNXSRV_EXPORTABLE g_szDbDriver[];
extern TCHAR LIBNXSRV_EXPORTABLE g_szDbDrvParams[];
extern TCHAR LIBNXSRV_EXPORTABLE g_szDbServer[];
extern TCHAR LIBNXSRV_EXPORTABLE g_szDbLogin[];
extern TCHAR LIBNXSRV_EXPORTABLE g_szDbPassword[];
extern TCHAR LIBNXSRV_EXPORTABLE g_szDbName[];

#endif   /* _nxsrvapi_h_ */
