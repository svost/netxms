/*
** nxget - command line tool used to retrieve parameters from NetXMS agent
** Copyright (C) 2004-2018 Victor Kirhenshtein
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
** File: nxget.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxsrvapi.h>
#include <curl/curl.h>

#ifndef _WIN32
#include <netdb.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxserviceget)

#define MAX_LINE_SIZE      4096
#define MAX_CRED_LEN       128

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != NULL)
      _tprintf(_T("<%s> "), tag);
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
}

/**
 * Callback for printing results
 */
static EnumerationCallbackResult PrintResults(const TCHAR *key, const void *value, void *data)
{
   WriteToTerminalEx(_T("%s = %s\n"), key, value);
   return _CONTINUE;
}

/**
 * Get service parameter
 */
static int GetServiceParameter(AgentConnection *pConn, const TCHAR *url, UINT32 retentionTime, const TCHAR *login, const TCHAR *password, long authType, StringList *headers, StringList *parameters, bool verifyCert)
{
   TCHAR szBuffer[1024];
   StringMap results;

   UINT32 dwError = pConn->getServiceParameter(url, retentionTime, login, password, authType, headers, parameters, verifyCert, &results);
   if (dwError == ERR_SUCCESS)
   {
      results.forEach(PrintResults, NULL);
   }
   else
   {
      WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }
   fflush(stdout);
   return (dwError == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   char *eptr;
   BOOL start = TRUE, useProxy = FALSE;
   int i, ch, iPos, iExitCode = 3, iInterval = 0;
   int authMethod = AUTH_NONE, proxyAuth = AUTH_NONE;
#ifdef _WITH_ENCRYPTION
   int iEncryptionPolicy = ENCRYPTION_PREFERRED;
#else
   int iEncryptionPolicy = ENCRYPTION_DISABLED;
#endif
   WORD agentPort = AGENT_LISTEN_PORT, proxyPort = AGENT_LISTEN_PORT;
   UINT32 dwTimeout = 5000, dwConnTimeout = 30000, dwError;
   TCHAR szSecret[MAX_SECRET_LENGTH] = _T("");
   TCHAR keyFile[MAX_PATH];
   char szProxy[MAX_OBJECT_NAME] = "";
	TCHAR szProxySecret[MAX_SECRET_LENGTH] = _T("");
   RSA *pServerKey = NULL;
   UINT32 retentionTime = 60;
   TCHAR login[MAX_CRED_LEN] = _T("");
   TCHAR password[MAX_CRED_LEN] = _T("");
   long authType = CURLAUTH_ANY;
   StringList headers;
   bool verifyCert = true;

   InitNetXMSProcess(true);
#ifdef _WIN32
	SetExceptionHandler(SEHDefaultConsoleHandler, NULL, NULL, _T("nxserviceget"), FALSE, FALSE);
#endif
	nxlog_set_debug_writer(DebugWriter);

   GetNetXMSDirectory(nxDirData, keyFile);
   _tcscat(keyFile, DFILE_KEYS);

   // Parse command line
   opterr = 1;
   while((ch = getopt(argc, argv, "a:A:cD:e:hH:i:K:LO:p:P:r:R:s:t:vw:W:X:Z:")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            _tprintf(_T("Usage: nxget [<options>] <host> <URL> <parameter> [<parameter> ...]\n")
                     _T("Valid options are:\n")
                     _T("   -a auth      : Authentication method. Valid methods are \"none\",\n")
                     _T("                  \"plain\", \"md5\" and \"sha1\". Default is \"none\".\n")
                     _T("   -A auth      : Authentication method for proxy agent.\n")
                     _T("   -c           : Do not verify service certificate.\n")
                     _T("   -D level     : Set debug level (default is 0).\n")
#ifdef _WITH_ENCRYPTION
                     _T("   -e policy    : Set encryption policy. Possible values are:\n")
                     _T("                    0 = Encryption disabled;\n")
                     _T("                    1 = Encrypt connection only if agent requires encryption;\n")
                     _T("                    2 = Encrypt connection if agent supports encryption;\n")
                     _T("                    3 = Force encrypted connection;\n")
                     _T("                  Default value is 1.\n")
#endif
                     _T("   -h           : Display help and exit.\n")
                     _T("   -H header    : Service header.\n")
                     _T("   -i seconds   : Get specified parameter(s) continuously with given interval.\n")
#ifdef _WITH_ENCRYPTION
                     _T("   -K file      : Specify server's key file\n")
                     _T("                  (default is %s).\n")
#endif
                     _T("   -L login     : Service login name.\n")
                     _T("   -O port      : Proxy agent's port number. Default is %d.\n")
                     _T("   -p port      : Agent's port number. Default is %d.\n")
                     _T("   -P passwod   : Service passwod.\n")
                     _T("   -r seconds   : Casched data retention time.\n")
                     _T("   -s secret    : Shared secret for authentication.\n")
                     _T("   -t auth      : Service auth type. Valid methods are \"basic\", \"digest_IE\",\n")
                     _T("                  \"digest\", \"bearer\", \"any\", \"anysafe\". Default is \"any\".\n")
                     _T("   -v           : Display version and exit.\n")
                     _T("   -w seconds   : Set command timeout (default is 5 seconds).\n")
                     _T("   -W seconds   : Set connection timeout (default is 30 seconds).\n")
                     _T("   -X addr      : Use proxy agent at given address.\n")
                     _T("   -Z secret    : Shared secret for proxy agent authentication.\n")
                     _T("\n"),
#ifdef _WITH_ENCRYPTION
                     keyFile,
#endif
                     agentPort, agentPort);
            start = FALSE;
            break;
         case 'a':   // Auth method
         case 'A':
            if (!strcmp(optarg, "none"))
               i = AUTH_NONE;
            else if (!strcmp(optarg, "plain"))
               i = AUTH_PLAINTEXT;
            else if (!strcmp(optarg, "md5"))
               i = AUTH_MD5_HASH;
            else if (!strcmp(optarg, "sha1"))
               i = AUTH_SHA1_HASH;
            else
            {
               printf("Invalid authentication method \"%s\"\n", optarg);
               start = FALSE;
            }
            if (ch == 'a')
               authMethod = i;
            else
               proxyAuth = i;
            break;
         case 'c':   // debug level
            verifyCert = false;
            break;
         case 'D':   // debug level
            nxlog_set_debug_level((int)strtol(optarg, NULL, 0));
            break;
         case 'i':   // Interval
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i <= 0))
            {
               printf("Invalid interval \"%s\"\n", optarg);
               start = FALSE;
            }
            else
            {
               iInterval = i;
            }
            break;
         case 'H':   // Password
            TCHAR header[512];
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(header, optarg, 512);
#else
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, header, 512);
#endif
            password[MAX_SECRET_LENGTH - 1] = 0;
#else
            strlcpy(header, optarg, 512);
#endif
            headers.addPreallocated(header);
            break;
         case 'L':   // Login
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(login, optarg, MAX_SECRET_LENGTH);
#else
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, login, MAX_SECRET_LENGTH);
#endif
            login[MAX_SECRET_LENGTH - 1] = 0;
#else
            strlcpy(login, optarg, MAX_SECRET_LENGTH);
#endif
            break;
         case 'p':   // Agent's port number
         case 'O':   // Proxy agent's port number
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               printf("Invalid port number \"%s\"\n", optarg);
               start = FALSE;
            }
            else
            {
               if (ch == 'p')
                  agentPort = (WORD)i;
               else if (ch == 'O')
                  proxyPort = (WORD)i;
            }
            break;
         case 'P':   // Password
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(password, optarg, MAX_SECRET_LENGTH);
#else
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, password, MAX_SECRET_LENGTH);
#endif
            password[MAX_SECRET_LENGTH - 1] = 0;
#else
            strlcpy(password, optarg, MAX_SECRET_LENGTH);
#endif
            break;
         case 'r':   // Retention time
            retentionTime = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 0) || (i > 65535))
            {
               printf("Invalid retention time \"%s\"\n", optarg);
               start = FALSE;
            }
            break;
         case 's':   // Shared secret
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(szSecret, optarg, MAX_SECRET_LENGTH);
#else
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szSecret, MAX_SECRET_LENGTH);
#endif
				szSecret[MAX_SECRET_LENGTH - 1] = 0;
#else
            strlcpy(szSecret, optarg, MAX_SECRET_LENGTH);
#endif
            break;
         case 't':
            if (!strcmp(optarg, "basic"))
               authType = CURLAUTH_BASIC;
            else if (!strcmp(optarg, "digest"))
               authType = CURLAUTH_DIGEST;
            else if (!strcmp(optarg, "digest_IE"))
               authType = CURLAUTH_DIGEST_IE;
            //else if (!strcmp(optarg, "bearer"))
            //   authType = CURLAUTH_BEARER;
            else if (!strcmp(optarg, "any"))
               authType = CURLAUTH_ANY;
            else if (!strcmp(optarg, "anysafe"))
               authType = CURLAUTH_ANYSAFE;
            else
            {
               printf("Invalid authentication method \"%s\"\n", optarg);
               start = FALSE;
            }
            break;
         case 'v':   // Print version and exit
            _tprintf(_T("NetXMS GET command-line utility Version ") NETXMS_VERSION_STRING _T("\n"));
            start = FALSE;
            break;
         case 'w':   // Command timeout
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               _tprintf(_T("Invalid timeout \"%hs\"\n"), optarg);
               start = FALSE;
            }
            else
            {
               dwTimeout = (UINT32)i * 1000;  // Convert to milliseconds
            }
            break;
         case 'W':   // Connection timeout
            i = strtol(optarg, &eptr, 0);
            if ((*eptr != 0) || (i < 1) || (i > 120))
            {
               printf("Invalid timeout \"%s\"\n", optarg);
               start = FALSE;
            }
            else
            {
               dwConnTimeout = (UINT32)i * 1000;  // Convert to milliseconds
            }
            break;
#ifdef _WITH_ENCRYPTION
         case 'e':
            iEncryptionPolicy = atoi(optarg);
            if ((iEncryptionPolicy < 0) ||
                (iEncryptionPolicy > 3))
            {
               printf("Invalid encryption policy %d\n", iEncryptionPolicy);
               start = FALSE;
            }
            break;
         case 'K':
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(keyFile, optarg, MAX_PATH);
#else
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, keyFile, MAX_PATH);
#endif
				keyFile[MAX_PATH - 1] = 0;
#else
            strlcpy(keyFile, optarg, MAX_PATH);
#endif
            break;
#else
         case 'e':
         case 'K':
            printf("ERROR: This tool was compiled without encryption support\n");
            start = FALSE;
            break;
#endif
         case 'X':   // Use proxy
            strncpy(szProxy, optarg, MAX_OBJECT_NAME);
				szProxy[MAX_OBJECT_NAME - 1] = 0;
            useProxy = TRUE;
            break;
         case 'Z':   // Shared secret for proxy agent
#ifdef UNICODE
#if HAVE_MBSTOWCS
            mbstowcs(szProxySecret, optarg, MAX_SECRET_LENGTH);
#else
	         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, szProxySecret, MAX_SECRET_LENGTH);
#endif
				szProxySecret[MAX_SECRET_LENGTH - 1] = 0;
#else
            strlcpy(szProxySecret, optarg, MAX_SECRET_LENGTH);
#endif
            break;
         case '?':
            start = FALSE;
            break;
         default:
            break;
      }
   }

   // Check parameter correctness
   if (start)
   {
      if (argc - optind < 3)
      {
         printf("Required argument(s) missing.\nUse nxget -h to get complete command line syntax.\n");
         start = FALSE;
      }
      else if ((authMethod != AUTH_NONE) && (szSecret[0] == 0))
      {
         printf("Shared secret not specified or empty\n");
         start = FALSE;
      }

      // Load server key if requested
#ifdef _WITH_ENCRYPTION
      if ((iEncryptionPolicy != ENCRYPTION_DISABLED) && start)
      {
         if (InitCryptoLib(0xFFFF))
         {
            pServerKey = LoadRSAKeys(keyFile);
            if (pServerKey == NULL)
            {
               pServerKey = RSAGenerateKey(2048);
               if (pServerKey == NULL)
               {
                  _tprintf(_T("Cannot load server RSA key from \"%s\" or generate new key\n"), keyFile);
                  if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
                     start = FALSE;
               }
            }
         }
         else
         {
            printf("Error initializing cryptography module\n");
            if (iEncryptionPolicy == ENCRYPTION_REQUIRED)
               start = FALSE;
         }
      }
#endif

      // If everything is ok, start communications
      if (start)
      {
         // Initialize WinSock
#ifdef _WIN32
         WSADATA wsaData;
         WSAStartup(2, &wsaData);
#endif
         InetAddress addr = InetAddress::resolveHostName(argv[optind]);
         InetAddress proxyAddr = useProxy ? InetAddress::resolveHostName(szProxy) : InetAddress();
         if (!addr.isValid())
         {
            fprintf(stderr, "Invalid host name or address \"%s\"\n", argv[optind]);
         }
         else if (useProxy && !proxyAddr.isValid())
         {
            fprintf(stderr, "Invalid host name or address \"%s\"\n", szProxy);
         }
         else
         {
            DecryptPassword(_T("netxms"), szSecret, szSecret, MAX_SECRET_LENGTH);
            AgentConnection *conn = new AgentConnection(addr, agentPort, authMethod, szSecret);

            conn->setConnectionTimeout(dwConnTimeout);
            conn->setCommandTimeout(dwTimeout);
            conn->setEncryptionPolicy(iEncryptionPolicy);
            if (useProxy)
               conn->setProxy(proxyAddr, proxyPort, proxyAuth, szProxySecret);
            if (conn->connect(pServerKey, &dwError))
            {
               do
               {
                  TCHAR *url;
                  StringList parameters;
#ifdef UNICODE
                  url = WideStringFromMBStringSysLocale(argv[iPos++]);
#else
                  url = MemCopyStr(argv[iPos++]);
#endif
                  iExitCode = GetServiceParameter(conn, url, retentionTime, (login[0] == 0) ? NULL: login, (password[0] == 0) ? NULL: password, authType, &headers, &parameters, verifyCert);
                  ThreadSleep(iInterval);

                  MemFree(url);
               }
               while(iInterval > 0);
            }
            else
            {
               WriteToTerminalEx(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
               iExitCode = 2;
            }
            conn->decRefCount();
         }
      }
   }

   return iExitCode;
}
