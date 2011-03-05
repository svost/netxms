/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 NetXMS Team
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
** File: main.cpp
**
**/

#include "nxcore.h"
#include <netxmsdb.h>

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H && HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif

#ifdef _WIN32
#include <errno.h>
#include <psapi.h>
#define open	_open
#define write	_write
#define close	_close
#else
#include <signal.h>
#include <sys/wait.h>
#endif


//
// Messages generated by mc.pl (for UNIX version only)
//

#ifndef _WIN32
extern unsigned int g_dwNumMessages;
extern const TCHAR *g_szMessages[];
#endif


//
// Shutdown reasons
//

#define SHUTDOWN_DEFAULT	0
#define SHUTDOWN_FROM_CONSOLE	1
#define SHUTDOWN_BY_SIGNAL	2


//
// Externals
//

extern Queue g_statusPollQueue;
extern Queue g_configPollQueue;
extern Queue g_topologyPollQueue;
extern Queue g_routePollQueue;
extern Queue g_discoveryPollQueue;
extern Queue g_nodePollerQueue;
extern Queue g_conditionPollerQueue;
extern Queue *g_pItemQueue;

void InitCertificates(void);
void InitUsers();
void CleanupUsers();


//
// Thread functions
//

THREAD_RESULT THREAD_CALL HouseKeeper(void *pArg);
THREAD_RESULT THREAD_CALL Syncer(void *pArg);
THREAD_RESULT THREAD_CALL NodePoller(void *pArg);
THREAD_RESULT THREAD_CALL PollManager(void *pArg);
THREAD_RESULT THREAD_CALL EventProcessor(void *pArg);
THREAD_RESULT THREAD_CALL WatchdogThread(void *pArg);
THREAD_RESULT THREAD_CALL ClientListener(void *pArg);
THREAD_RESULT THREAD_CALL ISCListener(void *pArg);
THREAD_RESULT THREAD_CALL LocalAdminListener(void *pArg);
THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *pArg);
THREAD_RESULT THREAD_CALL SyslogDaemon(void *pArg);
THREAD_RESULT THREAD_CALL BeaconPoller(void *pArg);
THREAD_RESULT THREAD_CALL JobManagerThread(void *arg);


//
// Global variables
//

TCHAR NXCORE_EXPORTABLE g_szConfigFile[MAX_PATH] = DEFAULT_CONFIG_FILE;
TCHAR NXCORE_EXPORTABLE g_szLogFile[MAX_PATH] = DEFAULT_LOG_FILE;
TCHAR NXCORE_EXPORTABLE g_szDumpDir[MAX_PATH] = DEFAULT_DUMP_DIR;
TCHAR g_szCodePage[256] = ICONV_DEFAULT_CODEPAGE;
TCHAR NXCORE_EXPORTABLE g_szListenAddress[MAX_PATH] = _T("0.0.0.0");
#ifndef _WIN32
char NXCORE_EXPORTABLE g_szPIDFile[MAX_PATH] = _T("/var/run/netxmsd.pid");
#endif
DB_HANDLE g_hCoreDB = 0;
DWORD g_dwDiscoveryPollingInterval;
DWORD g_dwStatusPollingInterval;
DWORD g_dwConfigurationPollingInterval;
DWORD g_dwRoutingTableUpdateInterval;
DWORD g_dwTopologyPollingInterval;
DWORD g_dwConditionPollingInterval;
DWORD g_dwPingSize;
DWORD g_dwAuditFlags;
TCHAR g_szDataDir[MAX_PATH] = _T("");
int g_nDBSyntax = DB_SYNTAX_UNKNOWN;
DWORD NXCORE_EXPORTABLE g_processAffinityMask = DEFAULT_AFFINITY_MASK;
QWORD g_qwServerId;
RSA *g_pServerKey = NULL;
time_t g_tServerStartTime = 0;
DWORD g_dwLockTimeout = 60000;   // Default timeout for acquiring mutex
DWORD g_dwSNMPTimeout = 2000;		// Default timeout for SNMP requests
DWORD g_dwAgentCommandTimeout = 2000;  // Default timeout for requests to agent
DWORD g_dwThresholdRepeatInterval = 0;	// Disabled by default
int g_nRequiredPolls = 1;
DB_DRIVER g_dbDriver = NULL;


//
// Static data
//

static CONDITION m_condShutdown = INVALID_CONDITION_HANDLE;
static THREAD m_thPollManager = INVALID_THREAD_HANDLE;
static THREAD m_thHouseKeeper = INVALID_THREAD_HANDLE;
static THREAD m_thSyncer = INVALID_THREAD_HANDLE;
static THREAD m_thSyslogDaemon = INVALID_THREAD_HANDLE;
static int m_nShutdownReason = SHUTDOWN_DEFAULT;

#ifndef _WIN32
static pthread_t m_signalHandlerThread;
#endif


//
// Sleep for specified number of seconds or until system shutdown arrives
// Function will return TRUE if shutdown event occurs
//

BOOL NXCORE_EXPORTABLE SleepAndCheckForShutdown(int iSeconds)
{
	return ConditionWait(m_condShutdown, iSeconds * 1000);
}


//
// Disconnect from database (exportable function for startup module)
//

void NXCORE_EXPORTABLE ShutdownDB()
{
	if (g_hCoreDB != NULL)
		DBDisconnect(g_hCoreDB);
	DBUnloadDriver(g_dbDriver);
}


//
// Check data directory for existence
//

static BOOL CheckDataDir()
{
	TCHAR szBuffer[MAX_PATH];

	if (_tchdir(g_szDataDir) == -1)
	{
		nxlog_write(MSG_INVALID_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", g_szDataDir);
		return FALSE;
	}

#ifdef _WIN32
#define MKDIR(name) _tmkdir(name)
#else
#define MKDIR(name) _tmkdir(name, 0700)
#endif

	// Create directory for mib files if it doesn't exist
	_tcscpy(szBuffer, g_szDataDir);
	_tcscat(szBuffer, DDIR_MIBS);
	if (MKDIR(szBuffer) == -1)
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}

	// Create directory for package files if it doesn't exist
	_tcscpy(szBuffer, g_szDataDir);
	_tcscat(szBuffer, DDIR_PACKAGES);
	if (MKDIR(szBuffer) == -1)
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}

	// Create directory for map background images if it doesn't exist
	_tcscpy(szBuffer, g_szDataDir);
	_tcscat(szBuffer, DDIR_BACKGROUNDS);
	if (MKDIR(szBuffer) == -1)
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}

	// Create directory for image library is if does't exists
	_tcscpy(szBuffer, g_szDataDir);
	_tcscat(szBuffer, DDIR_IMAGES);
	if (MKDIR(szBuffer) == -1)
	{
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}
	}

	// Create directory for file store if does't exists
	_tcscpy(szBuffer, g_szDataDir);
	_tcscat(szBuffer, DDIR_FILES);
	if (MKDIR(szBuffer) == -1)
	{
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}
	}

#undef MKDIR

	return TRUE;
}


//
// Load global configuration parameters
//

static void LoadGlobalConfig()
{
	g_dwDiscoveryPollingInterval = ConfigReadInt(_T("DiscoveryPollingInterval"), 900);
	g_dwStatusPollingInterval = ConfigReadInt(_T("StatusPollingInterval"), 60);
	g_dwConfigurationPollingInterval = ConfigReadInt(_T("ConfigurationPollingInterval"), 3600);
	g_dwRoutingTableUpdateInterval = ConfigReadInt(_T("RoutingTableUpdateInterval"), 300);
	g_dwTopologyPollingInterval = ConfigReadInt(_T("TopologyPollingInterval"), 1800);
	g_dwConditionPollingInterval = ConfigReadInt(_T("ConditionPollingInterval"), 60);
	if (ConfigReadInt(_T("DeleteEmptySubnets"), 1))
		g_dwFlags |= AF_DELETE_EMPTY_SUBNETS;
	if (ConfigReadInt(_T("EnableSNMPTraps"), 1))
		g_dwFlags |= AF_ENABLE_SNMP_TRAPD;
	if (ConfigReadInt(_T("EnableZoning"), 0))
		g_dwFlags |= AF_ENABLE_ZONING;
	if (ConfigReadInt(_T("EnableMultipleDBConnections"), 1))
	{
		// SQLite has troubles with multiple connections to the same database
		// from different threads, and it does not speed up database access
		// anyway, so we will not enable multiple connections for SQLite
		if (g_nDBSyntax != DB_SYNTAX_SQLITE)
		{
			g_dwFlags |= AF_ENABLE_MULTIPLE_DB_CONN;
		}
		else
		{
			DbgPrintf(1, _T("Configuration parameter EnableMultipleDBConnections ignored because database engine is SQLite"));
		}
	}
	if (ConfigReadInt(_T("RunNetworkDiscovery"), 0))
		g_dwFlags |= AF_ENABLE_NETWORK_DISCOVERY;
	if (ConfigReadInt(_T("ActiveNetworkDiscovery"), 0))
		g_dwFlags |= AF_ACTIVE_NETWORK_DISCOVERY;
	if (ConfigReadInt(_T("ResolveNodeNames"), 1))
		g_dwFlags |= AF_RESOLVE_NODE_NAMES;
	if (ConfigReadInt(_T("SyncNodeNamesWithDNS"), 0))
		g_dwFlags |= AF_SYNC_NODE_NAMES_WITH_DNS;
	if (ConfigReadInt(_T("InternalCA"), 0))
		g_dwFlags |= AF_INTERNAL_CA;
	if (ConfigReadInt(_T("CheckTrustedNodes"), 1))
		g_dwFlags |= AF_CHECK_TRUSTED_NODES;
	
	if (g_szDataDir[0] == 0)
	{
		ConfigReadStr(_T("DataDirectory"), g_szDataDir, MAX_PATH, DEFAULT_DATA_DIR);
		DbgPrintf(1, _T("Data directory set to %s from server configuration variable"), g_szDataDir);
	}
	else
	{
		DbgPrintf(1, _T("Using data directory %s"), g_szDataDir);
	}

	g_dwPingSize = ConfigReadInt(_T("IcmpPingSize"), 46);
	g_dwLockTimeout = ConfigReadInt(_T("LockTimeout"), 60000);
	g_dwSNMPTimeout = ConfigReadInt(_T("SNMPRequestTimeout"), 2000);
	g_dwAgentCommandTimeout = ConfigReadInt(_T("AgentCommandTimeout"), 2000);
	g_dwThresholdRepeatInterval = ConfigReadInt(_T("ThresholdRepeatInterval"), 0);
	g_nRequiredPolls = ConfigReadInt(_T("PollCountForStatusChange"), 1);
}


//
// Initialize cryptografic functions
//

static BOOL InitCryptografy()
{
#ifdef _WITH_ENCRYPTION
	TCHAR szKeyFile[MAX_PATH];
	BOOL bResult = FALSE;
	int fd, iPolicy;
	DWORD dwLen;
	BYTE *pBufPos, *pKeyBuffer, hash[SHA1_DIGEST_SIZE];

	if (!InitCryptoLib(ConfigReadULong(_T("AllowedCiphers"), 15)))
		return FALSE;

	_tcscpy(szKeyFile, g_szDataDir);
	_tcscat(szKeyFile, DFILE_KEYS);
	fd = _topen(szKeyFile, O_RDONLY | O_BINARY);
	g_pServerKey = LoadRSAKeys(szKeyFile);
	if (g_pServerKey == NULL)
	{
		DbgPrintf(1, _T("Generating RSA key pair..."));
		g_pServerKey = RSA_generate_key(NETXMS_RSA_KEYLEN, 17, NULL, 0);
		if (g_pServerKey != NULL)
		{
			fd = _topen(szKeyFile, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0600);
			if (fd != -1)
			{
				dwLen = i2d_RSAPublicKey(g_pServerKey, NULL);
				dwLen += i2d_RSAPrivateKey(g_pServerKey, NULL);
				pKeyBuffer = (BYTE *)malloc(dwLen);

				pBufPos = pKeyBuffer;
				i2d_RSAPublicKey(g_pServerKey, &pBufPos);
				i2d_RSAPrivateKey(g_pServerKey, &pBufPos);
				write(fd, &dwLen, sizeof(DWORD));
				write(fd, pKeyBuffer, dwLen);

				CalculateSHA1Hash(pKeyBuffer, dwLen, hash);
				write(fd, hash, SHA1_DIGEST_SIZE);

				close(fd);
				free(pKeyBuffer);
				bResult = TRUE;
			}
		}
	}
	else
	{
		bResult = TRUE;
	}

	iPolicy = ConfigReadInt(_T("DefaultEncryptionPolicy"), 1);
	if ((iPolicy < 0) || (iPolicy > 3))
		iPolicy = 1;
	SetAgentDEP(iPolicy);

	return bResult;
#else
	return TRUE;
#endif
}


//
// Check if process with given PID exists and is a NetXMS server process
//

static BOOL IsNetxmsdProcess(DWORD dwPID)
{
#ifdef _WIN32
	HANDLE hProcess;
	TCHAR szExtModule[MAX_PATH], szIntModule[MAX_PATH];
	BOOL bRet = FALSE;

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPID);
	if (hProcess != NULL)
	{
		if ((GetModuleBaseName(hProcess, NULL, szExtModule, MAX_PATH) > 0) &&
				(GetModuleBaseName(GetCurrentProcess(), NULL, szIntModule, MAX_PATH) > 0))
		{
			bRet = !_tcsicmp(szExtModule, szIntModule);
		}
		else
		{
			// Cannot read process name, for safety assume that it's a server process
			bRet = TRUE;
		}
		CloseHandle(hProcess);
	}
	return bRet;
#else
	return (kill((pid_t)dwPID, 0) != -1);
#endif
}


//
// Database event handler
//

static void DBEventHandler(DWORD dwEvent, const WCHAR *pszArg1, const WCHAR *pszArg2, void *userArg)
{
	if (!(g_dwFlags & AF_SERVER_INITIALIZED))
		return;     // Don't try to do anything if server is not ready yet

	switch(dwEvent)
	{
		case DBEVENT_CONNECTION_LOST:
			PostEvent(EVENT_DB_CONNECTION_LOST, g_dwMgmtNode, NULL);
			g_dwFlags |= AF_DB_CONNECTION_LOST;
			NotifyClientSessions(NX_NOTIFY_DBCONN_STATUS, FALSE);
			break;
		case DBEVENT_CONNECTION_RESTORED:
			PostEvent(EVENT_DB_CONNECTION_RESTORED, g_dwMgmtNode, NULL);
			g_dwFlags &= ~AF_DB_CONNECTION_LOST;
			NotifyClientSessions(NX_NOTIFY_DBCONN_STATUS, TRUE);
			break;
		case DBEVENT_QUERY_FAILED:
			PostEvent(EVENT_DB_QUERY_FAILED, g_dwMgmtNode, "ss", pszArg1, pszArg2);
			break;
		default:
			break;
	}
}


//
// Server initialization
//

BOOL NXCORE_EXPORTABLE Initialize()
{
	int i, iDBVersion;
	DWORD dwAddr;
	TCHAR szInfo[256];

	g_tServerStartTime = time(NULL);
	srand((unsigned int)g_tServerStartTime);
   nxlog_open((g_dwFlags & AF_USE_SYSLOG) ? NETXMSD_SYSLOG_NAME : g_szLogFile,
	           ((g_dwFlags & AF_USE_SYSLOG) ? NXLOG_USE_SYSLOG : 0) |
				  ((g_dwFlags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT),
              _T("LIBNXSRV.DLL"),
#ifdef _WIN32
				  0, NULL);
#else
				  g_dwNumMessages, g_szMessages);
#endif

	// Set code page
#ifndef _WIN32
	if (SetDefaultCodepage(g_szCodePage))
	{
		DbgPrintf(1, _T("Code page set to %s"), g_szCodePage);
	}
	else
	{
		nxlog_write(MSG_CODEPAGE_ERROR, EVENTLOG_WARNING_TYPE, "s", g_szCodePage);
	}
#endif

	// Set process affinity mask
	if (g_processAffinityMask != DEFAULT_AFFINITY_MASK)
	{
#ifdef _WIN32
		if (SetProcessAffinityMask(GetCurrentProcess(), g_processAffinityMask))
			DbgPrintf(1, _T("Process affinity mask set to 0x%08X"), g_processAffinityMask);
#else
		nxlog_write(MSG_SET_PROCESS_AFFINITY_NOT_SUPPORTED, EVENTLOG_WARNING_TYPE, NULL);
#endif
	}

#ifdef _WIN32
	WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wrc != 0)
	{
		nxlog_write(MSG_WSASTARTUP_FAILED, EVENTLOG_ERROR_TYPE, "e", wrc);
		return FALSE;
	}
#endif

	InitLocalNetInfo();

	// Create queue for delayed SQL queries
	g_pLazyRequestQueue = new Queue(256, 64);

	// Initialize database driver and connect to database
	DBSetDebugPrintCallback(DbgPrintf2);
	if (!DBInit(MSG_OTHER, (g_dwFlags & AF_LOG_SQL_ERRORS) ? MSG_SQL_ERROR : 0))
		return FALSE;
	g_dbDriver = DBLoadDriver(g_szDbDriver, g_szDbDrvParams, (g_nDebugLevel >= 9), DBEventHandler, NULL);
	if (g_dbDriver == NULL)
		return FALSE;

	// Connect to database
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	for(i = 0; ; i++)
	{
		g_hCoreDB = DBConnect(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, errorText);
		if ((g_hCoreDB != NULL) || (i == 5))
			break;
		ThreadSleep(5);
	}
	if (g_hCoreDB == NULL)
	{
		nxlog_write(MSG_DB_CONNFAIL, EVENTLOG_ERROR_TYPE, "s", errorText);
		return FALSE;
	}
	DbgPrintf(1, _T("Successfully connected to database %s@%s"), g_szDbName, g_szDbServer);

	// Check database version
	iDBVersion = DBGetSchemaVersion(g_hCoreDB);
	if (iDBVersion != DB_FORMAT_VERSION)
	{
		nxlog_write(MSG_WRONG_DB_VERSION, EVENTLOG_ERROR_TYPE, "dd", iDBVersion, DB_FORMAT_VERSION);
		return FALSE;
	}

	int baseSize = ConfigReadInt(_T("ConnectionPoolBaseSize"), 5);
	int maxSize = ConfigReadInt(_T("ConnectionPoolMaxSize"), 20);
	int cooldownTime = ConfigReadInt(_T("ConnectionPoolCooldownTime"), 300);
	DBConnectionPoolStartup(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, baseSize, maxSize, cooldownTime, g_hCoreDB);

	// Read database syntax
	g_nDBSyntax = DBGetSyntax(g_hCoreDB);

	// Read server ID
	ConfigReadStr(_T("ServerID"), szInfo, 256, _T(""));
	StrStrip(szInfo);
	if (szInfo[0] != 0)
	{
		StrToBin(szInfo, (BYTE *)&g_qwServerId, sizeof(QWORD));
	}
	else
	{
		// Generate new ID
		g_qwServerId = (((QWORD)time(NULL)) << 32) | rand();
		BinToStr((BYTE *)&g_qwServerId, sizeof(QWORD), szInfo);
		ConfigWriteStr(_T("ServerID"), szInfo, TRUE);
	}

	// Initialize locks
retry_db_lock:
	if (!InitLocks(&dwAddr, szInfo))
	{
		if (dwAddr == UNLOCKED)    // Some SQL problems
		{
			nxlog_write(MSG_INIT_LOCKS_FAILED, EVENTLOG_ERROR_TYPE, NULL);
		}
		else     // Database already locked by another server instance
		{
			// Check for lock from crashed/terminated local process
			if (dwAddr == GetLocalIpAddr())
			{
				DWORD dwPID;

				dwPID = ConfigReadULong(_T("DBLockPID"), 0);
				if (!IsNetxmsdProcess(dwPID) || (dwPID == GetCurrentProcessId()))
				{
					UnlockDB();
					nxlog_write(MSG_DB_LOCK_REMOVED, EVENTLOG_INFORMATION_TYPE, NULL);
					goto retry_db_lock;
				}
			}
			nxlog_write(MSG_DB_LOCKED, EVENTLOG_ERROR_TYPE, "as", dwAddr, szInfo);
		}
		return FALSE;
	}
	g_dwFlags |= AF_DB_LOCKED;

	// Load global configuration parameters
	LoadGlobalConfig();
	DbgPrintf(1, _T("Global configuration loaded"));

	// Check data directory
	if (!CheckDataDir())
		return FALSE;

	// Initialize cryptografy
	if (!InitCryptografy())
	{
		nxlog_write(MSG_INIT_CRYPTO_FAILED, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
	}

	// Initialize certificate store and CA
	InitCertificates();

	// Initialize SNMP stuff
	SnmpInit();

	// Create synchronization stuff
	m_condShutdown = ConditionCreate(TRUE);

	// Setup unique identifiers table
	if (!InitIdTable())
		return FALSE;
	DbgPrintf(2, _T("ID table created"));

	// Load and compile scripts
	LoadScripts();

	// Initialize mailer and SMS sender
	InitMailer();
	InitSMSSender();

	// Load users from database
	InitUsers();
	if (!LoadUsers())
	{
		nxlog_write(MSG_ERROR_LOADING_USERS, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
	}
	DbgPrintf(2, _T("User accounts loaded"));

	// Initialize audit
	InitAuditLog();

	// Initialize objects infrastructure and load objects from database
	ObjectsInit();
	if (!LoadObjects())
		return FALSE;
	LoadMaps();
	DbgPrintf(1, _T("Objects loaded and initialized"));
	
	// Initialize situations
	if (!SituationsInit())
		return FALSE;
	DbgPrintf(1, _T("Situations loaded and initialized"));

	// Initialize and load event actions
	if (!InitActions())
	{
		nxlog_write(MSG_ACTION_INIT_ERROR, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
	}

	// Initialize event handling subsystem
	if (!InitEventSubsystem())
		return FALSE;

	// Initialize alarms
	if (!g_alarmMgr.Init())
		return FALSE;

	// Initialize data collection subsystem
	if (!InitDataCollector())
		return FALSE;

	InitLogAccess();

	// Initialize watchdog
	WatchdogInit();

	// Check if management node object presented in database
	CheckForMgmtNode();
	if (g_dwMgmtNode == 0)
	{
		nxlog_write(MSG_CANNOT_FIND_SELF, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
	}

	// Start threads
	ThreadCreate(WatchdogThread, 0, NULL);
	ThreadCreate(NodePoller, 0, NULL);
	ThreadCreate(JobManagerThread, 0, NULL);
	m_thSyncer = ThreadCreateEx(Syncer, 0, NULL);
	m_thHouseKeeper = ThreadCreateEx(HouseKeeper, 0, NULL);
	m_thPollManager = ThreadCreateEx(PollManager, 0, NULL);

	// Start event processor
	ThreadCreate(EventProcessor, 0, NULL);

	// Start SNMP trapper
	InitTraps();
	if (ConfigReadInt(_T("EnableSNMPTraps"), 1))
		ThreadCreate(SNMPTrapReceiver, 0, NULL);

	// Start built-in syslog daemon
	if (ConfigReadInt(_T("EnableSyslogDaemon"), 0))
		m_thSyslogDaemon = ThreadCreateEx(SyslogDaemon, 0, NULL);

	// Start database _T("lazy") write thread
	StartDBWriter();

	// Start local administartive interface listener if required
	if (ConfigReadInt(_T("EnableAdminInterface"), 1))
		ThreadCreate(LocalAdminListener, 0, NULL);

	// Load modules
	LoadNetXMSModules();

	// Start beacon host poller
	ThreadCreate(BeaconPoller, 0, NULL);

	// Start inter-server communication listener
	if (ConfigReadInt(_T("EnableISCListener"), 0))
		ThreadCreate(ISCListener, 0, NULL);

	// Allow clients to connect
	ThreadCreate(ClientListener, 0, NULL);

	g_dwFlags |= AF_SERVER_INITIALIZED;
	DbgPrintf(1, _T("Server initialization completed"));
	return TRUE;
}


//
// Server shutdown
//

void NXCORE_EXPORTABLE Shutdown()
{
	DWORD i, dwNumThreads;

	// Notify clients
	NotifyClientSessions(NX_NOTIFY_SHUTDOWN, 0);

	nxlog_write(MSG_SERVER_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL);
	g_dwFlags |= AF_SHUTDOWN;     // Set shutdown flag
	ConditionSet(m_condShutdown);

#ifndef _WIN32
	if (IsStandalone() && (m_nShutdownReason != SHUTDOWN_BY_SIGNAL))
	{
		pthread_kill(m_signalHandlerThread, SIGUSR1);   // Terminate signal handler
	}
#endif

	// Stop event processor(s)
	g_pEventQueue->Clear();
	dwNumThreads = ConfigReadInt(_T("NumberOfEventProcessors"), 1);
	for(i = 0; i < dwNumThreads; i++)
		g_pEventQueue->Put(INVALID_POINTER_VALUE);

	ShutdownMailer();
	ShutdownSMSSender();

	ThreadSleep(1);     // Give other threads a chance to terminate in a safe way
	DbgPrintf(2, _T("All threads was notified, continue with shutdown"));

	// Wait for critical threads
	ThreadJoin(m_thHouseKeeper);
	ThreadJoin(m_thPollManager);
	ThreadJoin(m_thSyncer);
	ThreadJoin(m_thSyslogDaemon);

	SaveObjects(g_hCoreDB);
	DbgPrintf(2, _T("All objects saved to database"));
	SaveUsers(g_hCoreDB);
	DbgPrintf(2, _T("All users saved to database"));
	StopDBWriter();
	DbgPrintf(1, _T("Database writer stopped"));

	CleanupUsers();

	// Remove database lock
	UnlockDB();

	// Disconnect from database and unload driver
	if (g_hCoreDB != NULL)
		DBDisconnect(g_hCoreDB);

	DBConnectionPoolShutdown();

	DBUnloadDriver(g_dbDriver);
	DbgPrintf(1, _T("Database driver unloaded"));

	CleanupActions();
	ShutdownEventSubsystem();
	DbgPrintf(1, _T("Event processing stopped"));

	// Delete all objects
	//for(i = 0; i < g_dwIdIndexSize; i++)
	//   delete g_pIndexById[i].pObject;

	delete g_pScriptLibrary;

	nxlog_close();

	// Remove PID file
#ifndef _WIN32
	remove(g_szPIDFile);
#endif

	// Terminate process
#ifdef _WIN32
	if (!(g_dwFlags & AF_DAEMON))
		ExitProcess(0);
#else
	exit(0);
#endif
}


//
// Fast server shutdown - normally called only by Windows service on system shutdown
//

void NXCORE_EXPORTABLE FastShutdown(void)
{
	g_dwFlags |= AF_SHUTDOWN;     // Set shutdown flag
	ConditionSet(m_condShutdown);

	SaveObjects(g_hCoreDB);
	DbgPrintf(2, _T("All objects saved to database"));
	SaveUsers(g_hCoreDB);
	DbgPrintf(2, _T("All users saved to database"));

	// Remove database lock first, because we have a chance to loose DB connection
	UnlockDB();

	// Stop database writers
	StopDBWriter();
	DbgPrintf(1, _T("Database writer stopped"));

	nxlog_close();
}


//
// Compare given string to command template with abbreviation possibility
//

static BOOL IsCommand(const TCHAR *pszTemplate, TCHAR *pszString, int iMinChars)
{
	int i;

	// Convert given string to uppercase
	_tcsupr(pszString);

	for(i = 0; pszString[i] != 0; i++)
		if (pszString[i] != pszTemplate[i])
			return FALSE;
	if (i < iMinChars)
		return FALSE;
	return TRUE;
}


//
// Dump index
//

static void DumpIndex(CONSOLE_CTX pCtx, RWLOCK hLock, INDEX *pIndex, DWORD dwSize, BOOL bIndexByIp)
{
	DWORD i;
	TCHAR szIpAddr[16];

	if (!RWLockReadLock(hLock, 5000))
	{
		ConsolePrintf(pCtx, _T("ERROR: unable to obtain index lock in 5 seconds\n"));
		return;
	}

	for(i = 0; i < dwSize; i++)
	{
		if (bIndexByIp)
		{
			ConsolePrintf(pCtx, _T("%08X [%-15s] %p %s\n"), pIndex[i].dwKey,
					IpToStr(pIndex[i].dwKey, szIpAddr),
					pIndex[i].pObject, ((NetObj *)pIndex[i].pObject)->Name());
		}
		else
		{
			ConsolePrintf(pCtx, _T("%08X %p %s\n"), pIndex[i].dwKey, pIndex[i].pObject,
					((NetObj *)pIndex[i].pObject)->Name());
		}
	}

	RWLockUnlock(hLock);
}


//
// Process command entered from command line in standalone mode
// Return TRUE if command was _T("down")
//

int ProcessConsoleCommand(const TCHAR *pszCmdLine, CONSOLE_CTX pCtx)
{
	const TCHAR *pArg;
	TCHAR szBuffer[256];
	int nExitCode = CMD_EXIT_CONTINUE;

	// Get command
	pArg = ExtractWord(pszCmdLine, szBuffer);

	if (IsCommand(_T("DEBUG"), szBuffer, 2))
	{
		// Get argument
		pArg = ExtractWord(pArg, szBuffer);

		if (IsCommand(_T("ON"), szBuffer, 2))
		{
			g_nDebugLevel = 8;
			ConsolePrintf(pCtx, _T("Debug mode turned on\n"));
		}
		else if (IsCommand(_T("OFF"), szBuffer, 2))
		{
			g_nDebugLevel = 0;
			ConsolePrintf(pCtx, _T("Debug mode turned off\n"));
		}
		else
		{
			if (szBuffer[0] == 0)
				ConsolePrintf(pCtx, _T("ERROR: Missing argument\n\n"));
			else
				ConsolePrintf(pCtx, _T("ERROR: Invalid DEBUG argument\n\n"));
		}
	}
	else if (IsCommand(_T("DOWN"), szBuffer, 4))
	{
		ConsolePrintf(pCtx, _T("Proceeding with server shutdown...\n"));
		nExitCode = CMD_EXIT_SHUTDOWN;
	}
	else if (IsCommand(_T("DUMP"), szBuffer, 4))
	{
		DumpProcess(pCtx);
	}
	else if (IsCommand(_T("RAISE"), szBuffer, 5))
	{
		// Get argument
		pArg = ExtractWord(pArg, szBuffer);

		if (IsCommand(_T("ACCESS"), szBuffer, 6))
		{
			ConsolePrintf(pCtx, _T("Raising exception...\n"));
			char *p = NULL;
			*p = 0;
		}
		else if (IsCommand(_T("BREAKPOINT"), szBuffer, 5))
		{
#ifdef _WIN32
			ConsolePrintf(pCtx, _T("Raising exception...\n"));
			RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
#else
			ConsolePrintf(pCtx, _T("ERROR: Not supported on current platform\n"));
#endif
		}
		else
		{
			ConsolePrintf(pCtx, _T("Invalid exception name; possible names are:\nACCESS BREAKPOINT\n"));
		}
	}
	else if (IsCommand(_T("EXIT"), szBuffer, 4))
	{
		if (pCtx->hSocket != -1)
		{
			ConsolePrintf(pCtx, _T("Closing session...\n"));
			nExitCode = CMD_EXIT_CLOSE_SESSION;
		}
		else
		{
			ConsolePrintf(pCtx, _T("Cannot exit from local server console\n"));
		}
	}
	else if (IsCommand(_T("SHOW"), szBuffer, 2))
	{
		// Get argument
		pArg = ExtractWord(pArg, szBuffer);

		if (IsCommand(_T("FLAGS"), szBuffer, 1))
		{
			ConsolePrintf(pCtx, _T("Flags: 0x%08X\n"), g_dwFlags);
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DAEMON));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_USE_SYSLOG));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_NETWORK_DISCOVERY));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ACTIVE_NETWORK_DISCOVERY));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_LOG_SQL_ERRORS));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DELETE_EMPTY_SUBNETS));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_SNMP_TRAPD));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_ZONING));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SYNC_NODE_NAMES_WITH_DNS));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_CHECK_TRUSTED_NODES));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_WRITE_FULL_DUMP));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_RESOLVE_NODE_NAMES));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_CATCH_EXCEPTIONS));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_INTERNAL_CA));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DB_LOCKED));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_ENABLE_MULTIPLE_DB_CONN));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_DB_CONNECTION_LOST));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_NO_NETWORK_CONNECTIVITY));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_EVENT_STORM_DETECTED));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SERVER_INITIALIZED));
			ConsolePrintf(pCtx, SHOW_FLAG_VALUE(AF_SHUTDOWN));
			ConsolePrintf(pCtx, _T("\n"));
		}
		else if (IsCommand(_T("INDEX"), szBuffer, 1))
		{
			// Get argument
			pArg = ExtractWord(pArg, szBuffer);

			if (IsCommand(_T("CONDITION"), szBuffer, 1))
			{
				DumpIndex(pCtx, g_rwlockConditionIndex, g_pConditionIndex, g_dwConditionIndexSize, FALSE);
			}
			else if (IsCommand(_T("ID"), szBuffer, 2))
			{
				DumpIndex(pCtx, g_rwlockIdIndex, g_pIndexById, g_dwIdIndexSize, FALSE);
			}
			else if (IsCommand(_T("INTERFACE"), szBuffer, 2))
			{
				DumpIndex(pCtx, g_rwlockInterfaceIndex, g_pInterfaceIndexByAddr,
						g_dwInterfaceAddrIndexSize, TRUE);
			}
			else if (IsCommand(_T("NODE"), szBuffer, 1))
			{
				DumpIndex(pCtx, g_rwlockNodeIndex, g_pNodeIndexByAddr,
						g_dwNodeAddrIndexSize, TRUE);
			}
			else if (IsCommand(_T("SUBNET"), szBuffer, 1))
			{
				DumpIndex(pCtx, g_rwlockSubnetIndex, g_pSubnetIndexByAddr,
						g_dwSubnetAddrIndexSize, TRUE);
			}
			else
			{
				if (szBuffer[0] == 0)
					ConsolePrintf(pCtx, _T("ERROR: Missing index name\n")
							_T("Valid names are: CONDITION, ID, INTERFACE, NODE, SUBNET\n\n"));
				else
					ConsolePrintf(pCtx, _T("ERROR: Invalid index name\n\n"));
			}
		}
		else if (IsCommand(_T("MEMORY"), szBuffer, 2))
		{
#ifdef NETXMS_MEMORY_DEBUG
			PrintMemoryBlocks();
#else
			ConsolePrintf(pCtx, _T("ERROR: Server was compiled without memory debugger\n\n"));
#endif
		}
		else if (IsCommand(_T("MUTEX"), szBuffer, 2))
		{
			ConsolePrintf(pCtx, _T("Mutex status:\n"));
			DbgTestRWLock(g_rwlockIdIndex, _T("g_hMutexIdIndex"), pCtx);
			DbgTestRWLock(g_rwlockNodeIndex, _T("g_hMutexNodeIndex"), pCtx);
			DbgTestRWLock(g_rwlockSubnetIndex, _T("g_hMutexSubnetIndex"), pCtx);
			DbgTestRWLock(g_rwlockInterfaceIndex, _T("g_hMutexInterfaceIndex"), pCtx);
			ConsolePrintf(pCtx, _T("\n"));
		}
		else if (IsCommand(_T("OBJECTS"), szBuffer, 1))
		{
			DumpObjects(pCtx);
		}
		else if (IsCommand(_T("POLLERS"), szBuffer, 1))
		{
			ShowPollerState(pCtx);
		}
		else if (IsCommand(_T("QUEUES"), szBuffer, 1))
		{
			ShowQueueStats(pCtx, &g_conditionPollerQueue, _T("Condition poller"));
			ShowQueueStats(pCtx, &g_configPollQueue, _T("Configuration poller"));
			ShowQueueStats(pCtx, &g_topologyPollQueue, _T("Topology poller"));
			ShowQueueStats(pCtx, g_pItemQueue, _T("Data collector"));
			ShowQueueStats(pCtx, g_pLazyRequestQueue, _T("Database writer"));
			ShowQueueStats(pCtx, g_pEventQueue, _T("Event processor"));
			ShowQueueStats(pCtx, &g_discoveryPollQueue, _T("Network discovery poller"));
			ShowQueueStats(pCtx, &g_nodePollerQueue, _T("Node poller"));
			ShowQueueStats(pCtx, &g_routePollQueue, _T("Routing table poller"));
			ShowQueueStats(pCtx, &g_statusPollQueue, _T("Status poller"));
			ConsolePrintf(pCtx, _T("\n"));
		}
		else if (IsCommand(_T("ROUTING-TABLE"), szBuffer, 1))
		{
			DWORD dwNode;
			NetObj *pObject;

			pArg = ExtractWord(pArg, szBuffer);
			dwNode = _tcstoul(szBuffer, NULL, 0);
			if (dwNode != 0)
			{
				pObject = FindObjectById(dwNode);
				if (pObject != NULL)
				{
					if (pObject->Type() == OBJECT_NODE)
					{
						ROUTING_TABLE *pRT;
						TCHAR szIpAddr[16];
						int i;

						ConsolePrintf(pCtx, _T("Routing table for node %s:\n\n"), pObject->Name());
						pRT = ((Node *)pObject)->getCachedRoutingTable();
						if (pRT != NULL)
						{
							for(i = 0; i < pRT->iNumEntries; i++)
							{
								_sntprintf(szBuffer, 256, _T("%s/%d"), IpToStr(pRT->pRoutes[i].dwDestAddr, szIpAddr),
										     BitsInMask(pRT->pRoutes[i].dwDestMask));
								ConsolePrintf(pCtx, _T("%-18s %-15s %-6d %d\n"), szBuffer,
										        IpToStr(pRT->pRoutes[i].dwNextHop, szIpAddr),
										        pRT->pRoutes[i].dwIfIndex, pRT->pRoutes[i].dwRouteType);
							}
							ConsolePrintf(pCtx, _T("\n"));
						}
						else
						{
							ConsolePrintf(pCtx, _T("Node doesn't have cached routing table\n\n"));
						}
					}
					else
					{
						ConsolePrintf(pCtx, _T("ERROR: Object is not a node\n\n"));
					}
				}
				else
				{
					ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode);
				}
			}
			else
			{
				ConsolePrintf(pCtx, _T("ERROR: Invalid or missing node ID\n\n"));
			}
		}
		else if (IsCommand(_T("SESSIONS"), szBuffer, 2))
		{
			DumpSessions(pCtx);
		}
		else if (IsCommand(_T("STATS"), szBuffer, 2))
		{
			ShowServerStats(pCtx);
		}
		else if (IsCommand(_T("USERS"), szBuffer, 1))
		{
			DumpUsers(pCtx);
		}
		else if (IsCommand(_T("WATCHDOG"), szBuffer, 1))
		{
			WatchdogPrintStatus(pCtx);
			ConsolePrintf(pCtx, _T("\n"));
		}
		else
		{
			if (szBuffer[0] == 0)
				ConsolePrintf(pCtx, _T("ERROR: Missing subcommand\n\n"));
			else
				ConsolePrintf(pCtx, _T("ERROR: Invalid SHOW subcommand\n\n"));
		}
	}
	else if (IsCommand(_T("TRACE"), szBuffer, 1))
	{
		DWORD dwNode1, dwNode2;
		NetObj *pObject1, *pObject2;
		NETWORK_PATH_TRACE *pTrace;
		TCHAR szNextHop[16];
		int i;

		// Get arguments
		pArg = ExtractWord(pArg, szBuffer);
		dwNode1 = _tcstoul(szBuffer, NULL, 0);

		pArg = ExtractWord(pArg, szBuffer);
		dwNode2 = _tcstoul(szBuffer, NULL, 0);

		if ((dwNode1 != 0) && (dwNode2 != 0))
		{
			pObject1 = FindObjectById(dwNode1);
			if (pObject1 == NULL)
			{
				ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode1);
			}
			else
			{
				pObject2 = FindObjectById(dwNode2);
				if (pObject2 == NULL)
				{
					ConsolePrintf(pCtx, _T("ERROR: Object with ID %d does not exist\n\n"), dwNode2);
				}
				else
				{
					if ((pObject1->Type() == OBJECT_NODE) &&
							(pObject2->Type() == OBJECT_NODE))
					{
						pTrace = TraceRoute((Node *)pObject1, (Node *)pObject2);
						if (pTrace != NULL)
						{
							ConsolePrintf(pCtx, _T("Trace from %s to %s (%d hops):\n"),
									pObject1->Name(), pObject2->Name(), pTrace->iNumHops);
							for(i = 0; i < pTrace->iNumHops; i++)
								ConsolePrintf(pCtx, _T("[%d] %s %s %s %d\n"),
										pTrace->pHopList[i].pObject->Id(),
										pTrace->pHopList[i].pObject->Name(),
										IpToStr(pTrace->pHopList[i].dwNextHop, szNextHop),
										pTrace->pHopList[i].bIsVPN ? _T("VPN Connector ID:") : _T("Interface Index: "),
										pTrace->pHopList[i].dwIfIndex);
							DestroyTraceData(pTrace);
							ConsolePrintf(pCtx, _T("\n"));
						}
						else
						{
							ConsolePrintf(pCtx, _T("ERROR: Call to TraceRoute() failed\n\n"));
						}
					}
					else
					{
						ConsolePrintf(pCtx, _T("ERROR: Object is not a node\n\n"));
					}
				}
			}
		}
		else
		{
			ConsolePrintf(pCtx, _T("ERROR: Invalid or missing node id(s)\n\n"));
		}
	}
	else if (IsCommand(_T("HELP"), szBuffer, 2) || IsCommand(_T("?"), szBuffer, 1))
	{
		ConsolePrintf(pCtx, _T("Valid commands are:\n")
				_T("   debug [on|off]            - Turn debug mode on or off\n")
				_T("   down                      - Down NetXMS server\n")
				_T("   exit                      - Exit from remote session\n")
				_T("   help                      - Display this help\n")
				_T("   raise <exception>         - Raise exception\n")
				_T("   show flags                - Show internal server flags\n")
				_T("   show index <index>        - Show internal index\n")
				_T("   show mutex                - Display mutex status\n")
				_T("   show objects              - Dump network objects to screen\n")
				_T("   show pollers              - Show poller threads state information\n")
				_T("   show queues               - Show internal queues statistics\n")
				_T("   show routing-table <node> - Show cached routing table for node\n")
				_T("   show sessions             - Show active client sessions\n")
				_T("   show stats                - Show server statistics\n")
				_T("   show users                - Show users\n")
				_T("   show watchdog             - Display watchdog information\n")
				_T("   trace <node1> <node2>     - Show network path trace between two nodes\n")
				_T("\nAlmost all commands can be abbreviated to 2 or 3 characters\n")
				_T("\n"));
	}
	else
	{
		ConsolePrintf(pCtx, _T("UNKNOWN COMMAND\n\n"));
	}

	return nExitCode;
}


//
// Signal handler for UNIX platforms
//

#ifndef _WIN32

void SignalHandlerStub(int nSignal)
{
	// should be unused, but JIC...
	if (nSignal == SIGCHLD)
	{
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
	}
}

THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *pArg)
{
	sigset_t signals;
	int nSignal;
	BOOL bCallShutdown = FALSE;

	m_signalHandlerThread = pthread_self();

	// default for SIGCHLD: ignore
	signal(SIGCHLD, &SignalHandlerStub);

	sigemptyset(&signals);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGPIPE);
	sigaddset(&signals, SIGSEGV);
	sigaddset(&signals, SIGCHLD);
	sigaddset(&signals, SIGHUP);
	sigaddset(&signals, SIGUSR1);
	sigaddset(&signals, SIGUSR2);

	sigprocmask(SIG_BLOCK, &signals, NULL);

	while(1)
	{
		if (sigwait(&signals, &nSignal) == 0)
		{
			switch(nSignal)
			{
				case SIGTERM:
				case SIGINT:
					m_nShutdownReason = SHUTDOWN_BY_SIGNAL;
					if (IsStandalone())
						bCallShutdown = TRUE;
					ConditionSet(m_condShutdown);
					goto stop_handler;
				case SIGSEGV:
					abort();
					break;
				case SIGCHLD:
					while (waitpid(-1, NULL, WNOHANG) > 0)
						;
					break;
				case SIGUSR1:
					if (g_dwFlags & AF_SHUTDOWN)
						goto stop_handler;
					break;
				default:
					break;
			}
		}
		else
		{
			ThreadSleepMs(100);
		}
	}

stop_handler:
	sigprocmask(SIG_UNBLOCK, &signals, NULL);
	if (bCallShutdown)
		Shutdown();
	return THREAD_OK;
}

#endif


//
// Common main()
//

THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *pArg)
{
	nxlog_write(MSG_SERVER_STARTED, EVENTLOG_INFORMATION_TYPE, NULL);

	if (IsStandalone())
	{
		char *ptr, szCommand[256];
		struct __console_ctx ctx;
#ifdef UNICODE
		WCHAR wcCommand[256];
#endif

		ctx.hSocket = -1;
		ctx.pMsg = NULL;
		ctx.session = NULL;
		_tprintf(_T("\nNetXMS Server V") NETXMS_VERSION_STRING _T(" Ready\n")
				   _T("Enter \"help\" for command list or \"down\" for server shutdown\n")
				   _T("System Console\n\n"));

#if USE_READLINE
		// Initialize readline library if we use it
		rl_bind_key('\t', RL_INSERT_CAST rl_insert);
#endif

		while(1)
		{
#if USE_READLINE
			ptr = readline("netxmsd: ");
#else
			printf("netxmsd: ");
			fflush(stdout);
			if (fgets(szCommand, 255, stdin) == NULL)
				break;   // Error reading stdin
			ptr = strchr(szCommand, '\n');
			if (ptr != NULL)
				*ptr = 0;
			ptr = szCommand;
#endif

			if (ptr != NULL)
			{
#ifdef UNICODE
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ptr, -1, wcCommand, 256);
				wcCommand[255] = 0;
				StrStrip(wcCommand);
				if (wcCommand[0] != 0)
				{
					if (ProcessConsoleCommand(wcCommand, &ctx) == CMD_EXIT_SHUTDOWN)
#else
				StrStrip(ptr);
				if (*ptr != 0)
				{
					if (ProcessConsoleCommand(ptr, &ctx) == CMD_EXIT_SHUTDOWN)
#endif
						break;
#if USE_READLINE
					add_history(ptr);
#endif
				}
#if USE_READLINE
				free(ptr);
#endif
			}
			else
			{
				printf("\n");
			}
		}

#if USE_READLINE
		free(ptr);
#endif
		m_nShutdownReason = SHUTDOWN_FROM_CONSOLE;
		Shutdown();
	}
	else
	{
		ConditionWait(m_condShutdown, INFINITE);
		// On Win32, Shutdown() will be called by service control handler
#ifndef _WIN32
		Shutdown();
#endif
	}
	return THREAD_OK;
}


//
// Initiate server shutdown
//

void InitiateShutdown()
{
#ifdef _WIN32
	Shutdown();
#else
	if (IsStandalone())
	{
		Shutdown();
	}
	else
	{
		pthread_kill(m_signalHandlerThread, SIGTERM);
	}
#endif
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
