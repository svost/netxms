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
#include <zlib.h>

/**
 * Wrapper for inflateInit()
 */
ZStream ZInflateInit_STD(const void *input, size_t size)
{
   z_stream *stream = MemAllocStruct<z_stream>();
   stream->avail_in = static_cast<uInt>(size);
   stream->next_in = (z_const Bytef *)input;
   if (inflateInit(stream) != Z_OK)
   {
      MemFreeAndNull(stream);
   }
   return stream;
}

/**
 * Wrapper for inflate()
 */
ZResult ZInflate_STD(ZStream stream, ZFlushMode flush)
{
   int rc = inflate(static_cast<z_stream*>(stream),
            (flush == ZFlushMode::FINISH) ? Z_FINISH : ((flush == ZFlushMode::FLUSH) ? Z_SYNC_FLUSH : Z_NO_FLUSH));
   return (rc == Z_STREAM_END) ? ZResult::STREAM_END : ((rc == Z_OK) ? ZResult::OK : ZResult::ERROR);
}

/**
 * Wrapper for inflateEnd()
 */
void ZInflateEnd_STD(ZStream stream)
{
   inflateEnd(static_cast<z_stream*>(stream));
   MemFree(stream);
}

/**
 * Wrapper for deflateInit()
 */
ZStream ZDeflateInit_STD()
{
   z_stream *stream = MemAllocStruct<z_stream>();
   if (deflateInit(stream, 9) != Z_OK)
   {
      MemFreeAndNull(stream);
   }
   return stream;
}

/**
 * Wrapper for deflateBound()
 */
size_t ZDeflateBound_STD(ZStream stream, size_t size)
{
   return deflateBound(static_cast<z_stream*>(stream), static_cast<unsigned long>(size));
}

/**
 * Wrapper for deflate()
 */
ZResult ZDeflate_STD(ZStream stream, ZFlushMode flush)
{
   int rc = deflate(static_cast<z_stream*>(stream),
            (flush == ZFlushMode::FINISH) ? Z_FINISH : ((flush == ZFlushMode::FLUSH) ? Z_SYNC_FLUSH : Z_NO_FLUSH));
   return (rc == Z_STREAM_END) ? ZResult::STREAM_END : ((rc == Z_OK) ? ZResult::OK : ZResult::ERROR);
}

/**
 * Wrapper for deflateEnd()
 */
void ZDeflateEnd_STD(ZStream stream)
{
   deflateEnd(static_cast<z_stream*>(stream));
   MemFree(stream);
}

/**
 * Set stream input
 */
void ZSetInput_STD(ZStream stream, const void *data, size_t size)
{
   static_cast<z_stream*>(stream)->avail_in = static_cast<uInt>(size);
   static_cast<z_stream*>(stream)->next_in = (z_const Bytef*)data;
}

/**
 * Set stream output
 */
void ZSetOutput_STD(ZStream stream, void *buffer, size_t size)
{
   static_cast<z_stream*>(stream)->avail_out = static_cast<uInt>(size);
   static_cast<z_stream*>(stream)->next_out = static_cast<Bytef*>(buffer);
}

/**
 * Get available output space
 */
size_t ZGetAvailableOutput_STD(ZStream stream)
{
   return static_cast<z_stream*>(stream)->avail_out;
}
