/*
 * Copyright (C) 2014-2018 by Erik Hofman.
 * Copyright (C) 2014-2018 by Adalin B.V.
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

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"

#ifndef O_BINARY
# define O_BINARY       0
#endif

#define MAX_LOOPS		6
static int _mask_t[MAX_LOOPS] = {
    0,
    AAX_FORMAT_UNSIGNED,
    AAX_FORMAT_LE,
    AAX_FORMAT_LE_UNSIGNED,
    AAX_FORMAT_BE,
    AAX_FORMAT_BE_UNSIGNED
};

static const char* _format_s[AAX_FORMAT_MAX] = {
    "AAX_PCM8S",
    "AAX_PCM16S",
    "AAX_PCM24S",
    "AAX_PCM32S",
    "AAX_FLOAT",
    "AAX_DOUBLE",
    "AAX_MULAW",
    "AAX_ALAW",
    "AAX_IMA4_ADPCM",
    "AAX_PCM24S_PACKED",
};

static const char* _format_us[] = {
    "AAX_PCM8U",
    "AAX_PCM16U",
    "AAX_PCM24U",
    "AAX_PCM32U"
};

static const char* _mask_s[MAX_LOOPS][2] = {
    { "Native format", "" },
    { "Unsigned format", "" },
    { "Little endian, signed", "_LE" },
    { "Little endian, unsigned", "_LE" },
    { "Big endian, signed", "_BE" },
    { "Big endian, unsigned", "_BE" }
};

void
help()
{
    printf("aaxcvt version %i.%i.%i\n\n", AAX_UTILS_MAJOR_VERSION,
                                           AAX_UTILS_MINOR_VERSION,
                                           AAX_UTILS_MICRO_VERSION);
    printf("Usage: aaxcvt [options]\n");
    printf("Converts an input WAV audio file to an output file in the"
           "\nspecified output format.\n");

    printf("\nOptions:\n");
    printf("  -i, --input <file>\t\tconvert audio from this WAV file\n");
    printf("  -o, --output <file>\t\twrite the audio to this file\n");
    printf("  -r, --raw\t\t\tdo not write the WAV file header if specified\n");
    printf("  -p, --playfs\t\t\tspecifies the playback sample rate in Hz\n");
    printf("  -f, --format <format>\t\tspecifies the output format\n");
    printf("  -l, --list\t\t\tshow a list of all supported formats\n");
    printf("  -h, --help\t\t\tprint this message and exit\n");

    printf("\nNote that WAV files are little endian only and AeonWave "
           "automatically\ncompensates for that.\n");

    printf("\n");
    exit(-1);
}

void
list()
{
    int q;
    for (q=0; q<MAX_LOOPS; q++)
    {
        int fmt = 0;

        printf("%s\n", _mask_s[q][0]);
        do
        {
            int pos = fmt & AAX_FORMAT_NATIVE;
            int nfmt = fmt | _mask_t[q];
            char name[80];
 
            if (q)
            {
                if (pos == 0) continue;
                if (pos > AAX_PCM32S) break;
            }

            if (nfmt & AAX_FORMAT_UNSIGNED) {
                snprintf(name, 79, "%s%s:", _format_us[pos], _mask_s[q][1]);
            } else {
                snprintf(name, 79, "%s%s:", _format_s[pos], _mask_s[q][1]);
            }
            printf("  %-32s%s\n", name, getFormatString(nfmt));
        }
        while (++fmt < AAX_FORMAT_MAX);
    }
    exit(-1);
}
//
int main(int argc, char **argv)
{
    enum aaxFormat format;
    char *infile, *outfile;
    int raw, rv = 0;

    if (getCommandLineOption(argc, argv, "-l") ||
        getCommandLineOption(argc, argv, "--list"))
    {
       list();
    }

    if ((argc < 7) || (argc > 9))  {
        help();
    }

    raw = getCommandLineOption(argc, argv, "-r") ? 1 : 0;

    infile = getInputFile(argc, argv, NULL);
    if (!infile)
    {
        printf("Input file not specified or unable to access.\n");
        help();
        exit(-2);
    }
    outfile = getOutputFile(argc, argv, NULL);
    if (!outfile) {
       help();
    }

    format = getAudioFormat(argc, argv, AAX_FORMAT_NONE);
    if (format == AAX_AAXS16S) raw = AAX_TRUE;

    if (format != AAX_FORMAT_NONE)
    {
        char *rfs = getCommandLineOption(argc, argv, "-p");
        aaxConfig config;
        aaxBuffer buffer;

        config=aaxDriverOpenByName("AeonWave Loopback", AAX_MODE_WRITE_STEREO);
        if (rfs) {
            aaxMixerSetSetup(config, AAX_FREQUENCY, atoi(rfs));
         }

        buffer = bufferFromFile(config, infile);
        if (buffer)
        {
            aaxBufferSetSetup(buffer, AAX_FORMAT, format);
            if (!raw) {
               aaxBufferWriteToFile(buffer, outfile, AAX_OVERWRITE);
            }
            else
            {
               int fd, flags = O_WRONLY|O_CREAT|O_TRUNC;

               if (format != AAX_AAXS16S) flags += O_BINARY;
               fd = open(outfile, flags, 0644);
               if (fd >= 0)
               {
                  int size = aaxBufferGetSetup(buffer, AAX_TRACKSIZE);
                  void **data = aaxBufferGetData(buffer);
                  if (data)
                  {
                     int res = write(fd, *data, size);
                     if (res != size) {
                        printf("Written %i bytes of the required: %i\n", res, size);
                     }
                  }
                  else {
                     printf("Error: %s\n", aaxGetErrorString(aaxGetErrorNo()));
                  }
                  close(fd);
               }
               else {
                  printf("Unable to open file for writing: %s\n", outfile);
               }
            }
            aaxBufferDestroy(buffer);
        }
        aaxDriverDestroy(config);
    }
    else {
        printf("Unsupported audio format.\n");
        rv = -2;
    }

    return rv;
}

