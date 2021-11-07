/*
 * Copyright (C) 2014-2021 by Erik Hofman
 * Copyright (C) 2014-2021 by Adalin B.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY ADALIN B.V. ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL ADALIN B.V. OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUTOF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Adalin B.V.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>

#include <aax/aax.h>

#include "playlist.h"

char*
getURLFromPlaylist(aaxConfig config, const char *playlist)
{
   static const int buf_offs = strlen("AeonWave on Audio Files: ");
   static char buf[1025] = "AeonWave on Audio Files: ";
   const char *ext = strrchr(playlist, '.');
   char *ptr = strchr(playlist, ':');
   char *rv = playlist;

   if (ptr)
   {
      if (*(++ptr) == ' ') ++ptr;
      playlist = ptr;
   }

   if (ext && (!strcasecmp(ext, ".m3u") || !strcasecmp(ext, ".m3u8") ||
               !strcasecmp(ext, ".pls")))
   {
      aaxBuffer buffer = aaxBufferReadFromStream(config, playlist);
      if (buffer)
      {
         void **data = aaxBufferGetData(buffer);
         if (data)
         {
            struct entry_t entries[MAX_ENTRIES];
            int no_entries;

            if (!strcasecmp(ext, ".m3u")) {
               no_entries = readM3U(data[0], AAX_FALSE, entries);
            } else if (!strcasecmp(ext, ".m3u8")) {
               no_entries = readM3U(data[0], AAX_TRUE, entries);
            } else { // ".pls"
               no_entries = readPLS(data[0], entries);
            }

            // randomly select one from the list
            if (no_entries)
            {
               int i, len;

               srand(time(NULL));
               i = rand() % no_entries;
               len = entries[i].len;
               if (len > 1024-buf_offs) {
                  len = 1024-buf_offs;
               }
               memcpy(buf+buf_offs, entries[i].url, len);
               buf[1024] = '\0';
               rv = buf;
            }

            aaxFree(data);
         }
      }
   }
   return rv;
}

int
readM3U(const char *data, int utf8, struct entry_t entries[MAX_ENTRIES])
{
   int rv = 0;
#if 0
   if (utf8) {
        tstream.setCodec("UTF-8");
    }
    while(!tstream.atEnd())
    {
        QString line = tstream.readLine();
        if (line.at(0) != '#')
        {
            infile = line;
            if (setFileOrPlaylist(list) == false) {
                list.append(line);
            }
        }
    }
#endif
   return rv;
}

int
readPLS(const char *pls, struct entry_t entries[MAX_ENTRIES])
{
   int rv = 0;

   if (pls)
   {
      size_t len = strlen(pls);
      int no_urls = 0;

      while (len > sizeof("FileXX=") && no_urls < MAX_ENTRIES)
      {
         const char *next = strchr(pls+1, '\n');
         if (!next) next = pls+strlen(pls);
         else next++;

         if (!strncasecmp(pls, "File", strlen("File")))
         {
            char *start = strchr(pls, '=');
            if (start)
            {
               entries[no_urls].url = ++start;
               entries[no_urls++].len = next - start - 1;
            }
         }

         len -= next-pls;
         pls = next;
      }
      rv = no_urls;
   }

   return rv;
}


