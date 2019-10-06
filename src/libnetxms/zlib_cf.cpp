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
** File: zlib_cf.cpp
**
**/

#include "libnetxms.h"
#include <zlib_api.h>
#include "../zlibcf/zlib.h"

#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#if HAVE_ZLIB_CF

/**
 * Symbols from library
 */
int (*fp_inflateInit)(z_streamp, const char *, int);
int (*fp_inflate)(z_streamp, int);
int (*fp_inflateEnd)(z_streamp);
int (*fp_deflateInit)(z_streamp, int, const char *, int);
int (*fp_deflate)(z_streamp, int);
uLong (*fp_deflateBound)(z_streamp, uLong);
int (*fp_deflateEnd)(z_streamp);

/**
 * Load Cloudflare ZLib version
 */
bool LoadZLibCF()
{
   char path[MAX_PATH];
#ifdef UNICODE
   WCHAR wpath[MAX_PATH];
   GetNetXMSDirectory(nxDirLib, wpath);
   WideCharToMultiByte(CP_UTF8, 0, wpath, -1, path, MAX_PATH, NULL, NULL);
#else
   GetNetXMSDirectory(nxDirLib, path);
#endif
   //strlcat(path, "/libnxzlibcf.so", MAX_PATH);
   strlcpy(path, "/opt/netxms/lib/libnxzlibcf.so", MAX_PATH);
_tprintf(_T("loading %hs\n"), path);
   void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
   if (handle == NULL)
   {
      _tprintf(_T("FAIL: %hs\n"), dlerror());
      return false;
   }
_tprintf(_T("loaded %hs\n"), path);

   fp_inflateInit = reinterpret_cast<int (*)(z_streamp, const char *, int)>(dlsym(handle, "inflateInit_"));
   fp_inflate = reinterpret_cast<int (*)(z_streamp, int)>(dlsym(handle, "inflate"));
   fp_inflateEnd = reinterpret_cast<int (*)(z_streamp)>(dlsym(handle, "inflateEnd"));
   fp_deflateInit = reinterpret_cast<int (*)(z_streamp, int, const char *, int)>(dlsym(handle, "deflateInit_"));
   fp_deflate = reinterpret_cast<int (*)(z_streamp, int)>(dlsym(handle, "deflate"));
   fp_deflateBound = reinterpret_cast<uLong (*)(z_streamp, uLong)>(dlsym(handle, "deflateBound"));
   fp_deflateEnd = reinterpret_cast<int (*)(z_streamp)>(dlsym(handle, "deflateEnd"));

   if ((fp_inflateInit == NULL) || (fp_inflate == NULL) || (fp_inflateEnd == NULL) ||
       (fp_deflateInit == NULL) || (fp_deflate == NULL) || (fp_deflateBound == NULL) ||
       (fp_deflateEnd == NULL))
   {
      dlclose(handle);
      return false;
   }

   return true;
}

/**
 * Wrapper for inflateInit()
 */
ZStream ZInflateInit_CF(const void *input, size_t size)
{
   z_stream *stream = MemAllocStruct<z_stream>();
   stream->avail_in = static_cast<uInt>(size);
   stream->next_in = (z_const Bytef *)input;
   if (fp_inflateInit(stream, ZLIB_VERSION, static_cast<int>(sizeof(z_stream))) != Z_OK)
   {
      MemFreeAndNull(stream);
   }
   return stream;
}

/**
 * Wrapper for inflate()
 */
ZResult ZInflate_CF(ZStream stream, ZFlushMode flush)
{
   int rc = fp_inflate(static_cast<z_stream*>(stream),
            (flush == ZFlushMode::FINISH) ? Z_FINISH : ((flush == ZFlushMode::FLUSH) ? Z_SYNC_FLUSH : Z_NO_FLUSH));
   return (rc == Z_STREAM_END) ? ZResult::STREAM_END : ((rc == Z_OK) ? ZResult::OK : ZResult::ERROR);
}

/**
 * Wrapper for inflateEnd()
 */
void ZInflateEnd_CF(ZStream stream)
{
   fp_inflateEnd(static_cast<z_stream*>(stream));
   MemFree(stream);
}

/**
 * Wrapper for deflateInit()
 */
ZStream ZDeflateInit_CF()
{
   z_stream *stream = MemAllocStruct<z_stream>();
   if (fp_deflateInit(stream, 9, ZLIB_VERSION, static_cast<int>(sizeof(z_stream))) != Z_OK)
   {
      MemFreeAndNull(stream);
   }
   return stream;
}

/**
 * Wrapper for deflateBound()
 */
size_t ZDeflateBound_CF(ZStream stream, size_t size)
{
   return fp_deflateBound(static_cast<z_stream*>(stream), static_cast<unsigned long>(size));
}

/**
 * Wrapper for deflate()
 */
ZResult ZDeflate_CF(ZStream stream, ZFlushMode flush)
{
   int rc = fp_deflate(static_cast<z_stream*>(stream),
            (flush == ZFlushMode::FINISH) ? Z_FINISH : ((flush == ZFlushMode::FLUSH) ? Z_SYNC_FLUSH : Z_NO_FLUSH));
   return (rc == Z_STREAM_END) ? ZResult::STREAM_END : ((rc == Z_OK) ? ZResult::OK : ZResult::ERROR);
}

/**
 * Wrapper for deflateEnd()
 */
void ZDeflateEnd_CF(ZStream stream)
{
   fp_deflateEnd(static_cast<z_stream*>(stream));
   MemFree(stream);
}

/**
 * Set stream input
 */
void ZSetInput_CF(ZStream stream, const void *data, size_t size)
{
   static_cast<z_stream*>(stream)->avail_in = static_cast<uInt>(size);
   static_cast<z_stream*>(stream)->next_in = (z_const Bytef*)data;
}

/**
 * Set stream output
 */
void ZSetOutput_CF(ZStream stream, void *buffer, size_t size)
{
   static_cast<z_stream*>(stream)->avail_out = static_cast<uInt>(size);
   static_cast<z_stream*>(stream)->next_out = static_cast<Bytef*>(buffer);
}

/**
 * Get available output space
 */
size_t ZGetAvailableOutput_CF(ZStream stream)
{
   return static_cast<z_stream*>(stream)->avail_out;
}

#endif
