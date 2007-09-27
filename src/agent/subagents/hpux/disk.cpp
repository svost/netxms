/* $Id: disk.cpp,v 1.2 2007-09-27 09:20:41 alk Exp $ */

/* 
** NetXMS subagent for HP-UX
** Copyright (C) 2006 Alex Kirhenshtein
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
*/

#include <nms_common.h>
#include <nms_agent.h>

#include <sys/statvfs.h>

#include "disk.h"

LONG H_DiskInfo(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;
	struct statvfs s;
	char szArg[512] = {0};

	NxGetParameterArg(pszParam, 1, szArg, sizeof(szArg));

	if (szArg[0] != 0 && statvfs(szArg, &s) == 0)
	{
		nRet = SYSINFO_RC_SUCCESS;
		
		QWORD usedBlocks = (QWORD)(s.f_blocks - s.f_bfree);
		QWORD totalBlocks = (QWORD)s.f_blocks;
		QWORD blockSize = (QWORD)s.f_frsize;
		QWORD freeBlocks = (QWORD)s.f_bfree;
		QWORD availableBlocks = (QWORD)s.f_bavail;
		
		switch((long)pArg)
		{
			case DISK_TOTAL:
				ret_uint64(pValue, totalBlocks * blockSize);
				break;
			case DISK_USED:
				ret_uint64(pValue, usedBlocks * blockSize);
				break;
			case DISK_FREE:
				ret_uint64(pValue, freeBlocks * blockSize);
				break;
			case DISK_AVAIL:
				ret_uint64(pValue, availableBlocks * blockSize);
				break;
			case DISK_USED_PERC:
				ret_double(pValue, (usedBlocks * 100) / totalBlocks);
				break;
			case DISK_AVAIL_PERC:
				ret_double(pValue, (availableBlocks * 100) / totalBlocks);
				break;
			case DISK_FREE_PERC:
				ret_double(pValue, (freeBlocks * 100) / totalBlocks);
				break;
			default:
				nRet = SYSINFO_RC_ERROR;
				break;
		}
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1  2006/10/04 14:59:13  alk
initial version of HPUX subagent


*/
