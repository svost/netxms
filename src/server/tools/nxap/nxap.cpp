/*
** nxap - command line tool used to manage agent policies
** Copyright (C) 2010-2015 Victor Kirhenshtein
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
** File: nxap.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxsrvapi.h>

#ifndef _WIN32
#include <netdb.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxap)

/**
 * Static fields
 */
static int s_action = -1;
static uuid s_guid;

/**
 * Get policy list
 */
static int GetPolicyInventory(AgentConnection *conn)
{
	AgentPolicyInfo *ap;
	UINT32 rcc = conn->getPolicyInventory(&ap);
	if (rcc == ERR_SUCCESS)
	{
		_tprintf(_T("GUID                                 Type ServerInfo                                                       ServerId         Version\n")
		         _T("-----------------------------------------------------------------------------------------------------------------------------------\n"));
		for(int i = 0; i < ap->size(); i++)
		{
		   TCHAR buffer[64];
			_tprintf(_T("%-16s %-16s %-64s ") UINT64X_FMT(_T("016")) _T(" %-3d\n"), ap->getGuid(i).toString(buffer), ap->getType(i), ap->getServerInfo(i), ap->getServerId(i), ap->getVersion(i));
		}
		delete ap;
	}
	else
	{
      _tprintf(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
	}
	return (rcc == ERR_SUCCESS) ? 0 : 1;
}


/*
 * Uninstall policy
 */
static int UninstallPolicy(AgentConnection *conn, const uuid& guid)
{
	UINT32 rcc = conn->uninstallPolicy(guid);
	if (rcc == ERR_SUCCESS)
	{
		_tprintf(_T("Policy successfully uninstalled from agent\n"));
	}
	else
	{
      _tprintf(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
	}
	return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Parse nxap specific parameters
 */
static bool ParseAdditionalOptionCb(const char ch, const char *optarg)
{
   switch(ch)
   {
      case 'l':   // List policies on agent
         s_action = 0;
         break;
      case 'u':   // Uninstall policy from agent
         s_action = 1;
#ifdef UNICODE
         WCHAR wguid[256];
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, wguid, 256);
         wguid[255] = 0;
         s_guid = uuid::parse(wguid);
#else
         s_guid = uuid::parse(optarg);
#endif
         break;
      default:
         break;
   }
   return true;
}

/**
 * Validate argument count
 */
static bool IsArgMissingCb(int currentCount)
{
   // Check parameter correctness
   if (s_action == -1)
   {
      _tprintf(_T("ERROR: You must specify either -l or -u option\n"));
      return FALSE;
   }

   return currentCount < 1;
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, char *argv[], RSA *pServerKey)
{
   int exitCode;
   if (s_action == 0)
   {
      exitCode = GetPolicyInventory(conn);
   }
   else
   {
      exitCode = UninstallPolicy(conn, s_guid);
   }
   return exitCode;
}

/**
 * Startup
 */
int main(int argc, char *argv[])
{
   ServerCmdToolParameters parameters;
   parameters.argc = argc;
   parameters.argv = argv;
   parameters.mainHelpText = _T("Usage: nxap [<options>] -l <host>\n")
                           _T("   or: nxap [<options>] -u <guid> <host>\n")
                           _T("Tool specific options are:\n")
                           _T("   -l           : List policies.\n")
                           _T("   -u <guid>    : Uninstall policy.\n")
                           _T("\n");
   parameters.additionalOptions = "lu:";
   parameters.executeCommandCb = &ExecuteCommandCb;
   parameters.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   parameters.isArgMissingCb = &IsArgMissingCb;

   return RunServerCmdTool(&parameters);
}
