/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: zlib_std.cpp
**
**/

#include "libnetxms.h"
#include <zlib_api.h>

/**
 * ZLib standard implementation
 */
ZStream ZInflateInit_STD(const void *, size_t);
ZResult ZInflate_STD(ZStream, ZFlushMode);
void ZInflateEnd_STD(ZStream);
ZStream ZDeflateInit_STD();
size_t ZDeflateBound_STD(ZStream, size_t);
ZResult ZDeflate_STD(ZStream, ZFlushMode);
void ZDeflateEnd_STD(ZStream);
void ZSetInput_STD(ZStream, const void *, size_t);
void ZSetOutput_STD(ZStream, void *, size_t);
size_t ZGetAvailableOutput_STD(ZStream);

/**
 * ZLib Cloudflare implementation
 */
#ifdef HAVE_ZLIB_CF
bool LoadZLibCF();
ZStream ZInflateInit_CF(const void *, size_t);
ZResult ZInflate_CF(ZStream, ZFlushMode);
void ZInflateEnd_CF(ZStream);
ZStream ZDeflateInit_CF();
size_t ZDeflateBound_CF(ZStream, size_t);
ZResult ZDeflate_CF(ZStream, ZFlushMode);
void ZDeflateEnd_CF(ZStream);
void ZSetInput_CF(ZStream, const void *, size_t);
void ZSetOutput_CF(ZStream, void *, size_t);
size_t ZGetAvailableOutput_CF(ZStream);
#endif

/**
 * Pointers to selected implementation
 */
static ZStream (*fp_ZInflateInit)(const void *, size_t) = ZInflateInit_STD;
static ZResult (*fp_ZInflate)(ZStream, ZFlushMode) = ZInflate_STD;
static void (*fp_ZInflateEnd)(ZStream) = ZInflateEnd_STD;
static ZStream (*fp_ZDeflateInit)() = ZDeflateInit_STD;
static size_t (*fp_ZDeflateBound)(ZStream, size_t) = ZDeflateBound_STD;
static ZResult (*fp_ZDeflate)(ZStream, ZFlushMode) = ZDeflate_STD;
static void (*fp_ZDeflateEnd)(ZStream) = ZDeflateEnd_STD;
static void (*fp_ZSetInput)(ZStream, const void *, size_t) = ZSetInput_STD;
static void (*fp_ZSetOutput)(ZStream, void *, size_t) = ZSetOutput_STD;
static size_t (*fp_ZGetAvailableOutput)(ZStream) = ZGetAvailableOutput_STD;

/**
 * Initialize ZLib wrapper
 */
void LIBNETXMS_EXPORTABLE InitZLibWrapper()
{
#ifdef HAVE_ZLIB_CF
   if (LoadZLibCF())
   {
_tprintf(_T("zlibcf loaded\n"));
      fp_ZInflateInit = ZInflateInit_CF;
      fp_ZInflate = ZInflate_CF;
      fp_ZInflateEnd = ZInflateEnd_CF;
      fp_ZDeflateInit = ZDeflateInit_CF;
      fp_ZDeflateBound = ZDeflateBound_CF;
      fp_ZDeflate = ZDeflate_CF;
      fp_ZDeflateEnd = ZDeflateEnd_CF;
      fp_ZSetInput = ZSetInput_CF;
      fp_ZSetOutput = ZSetOutput_CF;
   }
   else _tprintf(_T("load error\n"));
#endif
}

/**
 * Wrapper for inflateInit()
 */
ZStream LIBNETXMS_EXPORTABLE ZInflateInit(const void *input, size_t size)
{
   return fp_ZInflateInit(input, size);
}

/**
 * Wrapper for inflate()
 */
ZResult LIBNETXMS_EXPORTABLE ZInflate(ZStream stream, ZFlushMode flush)
{
   return fp_ZInflate(stream, flush);
}

/**
 * Wrapper for inflateEnd()
 */
void LIBNETXMS_EXPORTABLE ZInflateEnd(ZStream stream)
{
   return fp_ZInflateEnd(stream);
}

/**
 * Wrapper for deflateInit()
 */
ZStream LIBNETXMS_EXPORTABLE ZDeflateInit()
{
   return fp_ZDeflateInit();
}

/**
 * Wrapper for deflateBound()
 */
size_t LIBNETXMS_EXPORTABLE ZDeflateBound(ZStream stream, size_t size)
{
   return fp_ZDeflateBound(stream, size);
}

/**
 * Wrapper for deflate()
 */
ZResult LIBNETXMS_EXPORTABLE ZDeflate(ZStream stream, ZFlushMode flush)
{
   return fp_ZDeflate(stream, flush);
}

/**
 * Wrapper for deflateEnd()
 */
void LIBNETXMS_EXPORTABLE ZDeflateEnd(ZStream stream)
{
   fp_ZDeflateEnd(stream);
}

/**
 * Set stream input
 */
void LIBNETXMS_EXPORTABLE ZSetInput(ZStream stream, const void *data, size_t size)
{
   fp_ZSetInput(stream, data, size);
}

/**
 * Set stream output
 */
void LIBNETXMS_EXPORTABLE ZSetOutput(ZStream stream, void *buffer, size_t size)
{
   fp_ZSetOutput(stream, buffer, size);
}

/**
 * Get available output space
 */
size_t LIBNETXMS_EXPORTABLE ZGetAvailableOutput(ZStream stream)
{
   return fp_ZGetAvailableOutput(stream);
}
