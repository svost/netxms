/*
** NetXMS - Network Management System
** NXCP API
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: zlib_api.h
**
**/

#ifndef _zlib_api_h_
#define _zlib_api_h_

typedef void *ZStream;

enum class ZFlushMode
{
   NO_FLUSH,
   FLUSH,
   FINISH
};

enum class ZResult
{
   OK,
   STREAM_END,
   ERROR
};

void LIBNETXMS_EXPORTABLE InitZLibWrapper();

ZStream LIBNETXMS_EXPORTABLE ZInflateInit(const void *input, size_t size);
ZResult LIBNETXMS_EXPORTABLE ZInflate(ZStream stream, ZFlushMode flush);
void LIBNETXMS_EXPORTABLE ZInflateEnd(ZStream stream);

ZStream LIBNETXMS_EXPORTABLE ZDeflateInit();
size_t LIBNETXMS_EXPORTABLE ZDeflateBound(ZStream stream, size_t size);
ZResult LIBNETXMS_EXPORTABLE ZDeflate(ZStream stream, ZFlushMode flush);
void LIBNETXMS_EXPORTABLE ZDeflateEnd(ZStream stream);

void LIBNETXMS_EXPORTABLE ZSetInput(ZStream stream, const void *data, size_t size);
void LIBNETXMS_EXPORTABLE ZSetOutput(ZStream stream, void *buffer, size_t size);
size_t LIBNETXMS_EXPORTABLE ZGetAvailableOutput(ZStream stream);

#endif
