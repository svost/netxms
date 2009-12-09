/* 
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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

#include "libnxdb.h"


//
// Static data
//

static void (*s_dbgPrintCb)(int, const TCHAR *, va_list) = NULL;


//
// Write log
//

void __DBWriteLog(WORD level, const TCHAR *format, ...)
{
	TCHAR buffer[4096];
	va_list args;
	
	va_start(args, format);
	_vsntprintf(buffer, 4096, format, args);
	va_end(args);
	nxlog_write(g_logMsgCode, level, "s", buffer);
}


//
// Debug output
//

void __DBDbgPrintf(int level, const TCHAR *format, ...)
{
	if (s_dbgPrintCb != NULL)
	{
		va_list args;

		va_start(args, format);
		s_dbgPrintCb(level, format, args);
		va_end(args);
	}
}


//
// Set debug print callback
//

void LIBNXDB_EXPORTABLE DBSetDebugPrintCallback(void (*cb)(int, const TCHAR *, va_list))
{
	s_dbgPrintCb = cb;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
