/*
 * Copyright (C) 2008-2018 by Erik Hofman.
 * Copyright (C) 2009-2018 by Adalin B.V.
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
# include "config.h"
#endif

#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

#include <stdio.h>
#include <string.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#if _WIN32
# include <Windows.h>
#endif
#ifndef NDEBUG
# ifdef HAVE_RMALLOC_H
#  include <rmalloc.h>
# endif
#endif

#include <xml.h>
#include <aax/aax.h>
#include "wavfile.h"
#include "driver.h"

static int maximumWidth = 80;

#if _WIN32
static int
terminalWidth()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int ret, rv = maximumWidth;
    ret = GetConsoleScreenBufferInfo(GetStdHandle( STD_OUTPUT_HANDLE ),&csbi);
    if (ret) rv = csbi.dwSize.X;
    return rv;
}
#elif HAVE_SYS_IOCTL_H
static int
terminalWidth()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}
#else
#pragma warning Implement terminal width
static int
terminalWidth() {
    return maximumWidth;
}
#endif

int main(int argc, char **argv)
{
    unsigned int i, x, y, z, max;
    aaxConfig cfg;
    const char *s;
    char *devname;
    int mode;

    if (printCopyright(argc, argv) || playAudioTune(argc, argv)) {
        return 0;
    }

    maximumWidth = terminalWidth()-1;
    printf("aaxinfo version %i.%i.%i\n", AAX_UTILS_MAJOR_VERSION,
                                         AAX_UTILS_MINOR_VERSION,
                                         AAX_UTILS_MICRO_VERSION);
    printf("Run %s -copyright to read the copyright information.\n", argv[0]);

    for (mode = AAX_MODE_READ; mode <= AAX_MODE_WRITE_STEREO; mode++)
    {
        char *desc[2] = { "capture", "playback"};

        printf("\nDevices that support %s:\n", desc[mode]);

        max = aaxDriverGetCount(mode);
        for (x=0; x<max; x++)
        {
            cfg = aaxDriverGetByPos(x, mode);
            if (cfg) {
                unsigned max_device;
                const char *d;

                d = aaxDriverGetSetup(cfg, AAX_DRIVER_STRING);
                max_device = aaxDriverGetDeviceCount(cfg, mode);
                if (max_device)
                {
                    for (y=0; y<max_device; y++)
                    {
                        unsigned int max_interface;
                        const char *r;

                        r = aaxDriverGetDeviceNameByPos(cfg, y, mode);

                        max_interface= aaxDriverGetInterfaceCount(cfg, r, mode);
                        if (max_interface)
                        {
                            for (z=0; z<max_interface; z++)
                            {
                                const char *ifs;

                                ifs = aaxDriverGetInterfaceNameByPos(cfg, r, z,
                                                                     mode);
                                printf(" '%s on %s: %s'\n", d, r, ifs);
                            }
                        }
                        else {
                           printf(" '%s on %s'\n", d, r);
                        }
                    }
                }
                else {
                    printf(" '%s'\n", d);
                }
                aaxDriverClose(cfg);
                aaxDriverDestroy(cfg);
            } else {
                printf("\t%i. not found\n", x);
            }
        }
    }

    mode = AAX_MODE_WRITE_STEREO;
    devname = getDeviceName(argc, argv);
    cfg = aaxDriverGetByName(devname, mode);
    if (cfg)
    {
        printf("\n");
        cfg = aaxDriverOpen(cfg);
        if (cfg)
        {
            char filename[256];
            int res, min, max;
            void *xid;

            s = aaxDriverGetSetup(cfg, AAX_SHARED_DATA_DIR);
            printf("Shared data directory: %s\n" , s);

            snprintf(filename, 255, "%s/gmmidi.xml", s);
            xid = xmlOpen(filename);
            if (!xid)
            {
                snprintf(filename, 255, "%s/ultrasynth/gmmidi.xml", s);
                xid = xmlOpen(filename);
            }
            if (xid)
            {
                void *xnid = xmlNodeGet(xid, "aeonwave/midi");
                if (xnid)
                {
                    char *patches = xmlAttributeGetString(xnid, "name");
                    char *version = xmlAttributeGetString(xnid, "version");
                    printf("Patch set: %s instrument set version %s\n",
                            patches, version);
                    xmlFree(version);
                    xmlFree(patches);
                }
                xmlFree(xid);
            }
            printf("\n");

            res = aaxMixerSetState(cfg, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            s = aaxDriverGetSetup(cfg, AAX_DRIVER_STRING);
            printf("Driver   : %s\n", s);

            s = aaxDriverGetSetup(cfg, AAX_RENDERER_STRING);
            printf("Renderer : %s\n", s);

            x = aaxGetMajorVersion();
            y = aaxGetMinorVersion();
            s = (char *)aaxGetVersionString(cfg);
            printf("Version  : %s (%i.%i)\n", s, x, y);

            s = aaxDriverGetSetup(cfg, AAX_VENDOR_STRING);
            printf("Vendor   : %s\n", s);

            x = aaxMixerGetSetup(cfg, AAX_TIMER_MODE);
            printf ("Mixer timed mode support:   %s\n", x ? "yes" : "no");

            x = aaxMixerGetSetup(cfg, AAX_SHARED_MODE);
            printf ("Mixer shared mode support:  %s\n", x ? "yes" : "no");

            x = aaxMixerGetSetup(cfg, AAX_BATCHED_MODE);
            printf ("Mixer batched mode support: %s\n", x ? "yes" : "no");

            x = aaxMixerGetSetup(cfg, AAX_SEEKABLE_SUPPORT);
            printf ("Mixer seekable support: %s\n", x ? "yes" : "no");

            x = aaxMixerGetSetup(cfg, AAX_FORMAT);
            printf("Mixer format: %2i-bit per sample\n",aaxGetBitsPerSample(x));

            min = aaxMixerGetSetup(cfg, AAX_TRACKS_MIN);
            max = aaxMixerGetSetup(cfg, AAX_TRACKS_MAX);
            printf("Mixer supported track range: %i - %i tracks\n", min, max);

            min = aaxMixerGetSetup(cfg, AAX_FREQUENCY_MIN);
            max = aaxMixerGetSetup(cfg, AAX_FREQUENCY_MAX);
            printf("Mixer frequency range: %4.1fkHz - %4.1fkHz\n",
                    min/1000.0f, max/1000.0f);

            x = aaxMixerGetSetup(cfg, AAX_FREQUENCY);
            printf("Mixer frequency: %i Hz\n", x);

            x = aaxMixerGetSetup(cfg, AAX_REFRESHRATE);
            printf("Mixer refresh rate: %u Hz\n", x);

            x = aaxMixerGetSetup(cfg, AAX_UPDATERATE);
            if (x) {
                printf("Mixer update rate:  %u Hz\n", x);
            }

            x = aaxMixerGetSetup(cfg, AAX_LATENCY);
            if (x) {
               printf("Mixer latency: %7.2f ms\n", (float)x*1e-3f);
            }

            x = aaxMixerGetSetup(cfg, AAX_MONO_EMITTERS);
            y = aaxMixerGetSetup(cfg, AAX_STEREO_EMITTERS);
            printf("Available mono emitters:   ");
            if (x == UINT_MAX) printf("infinite\n");
            else printf("%3i\n", x);
            printf("Available stereo emitters: ");
            if (y == UINT_MAX/2) printf("infinite\n");
            else printf("%3i\n", y);
            x = aaxMixerGetSetup(cfg, AAX_AUDIO_FRAMES);
            printf("Available audio-frames: ");
            if (x == UINT_MAX) printf("   infinite\n");
            else printf("%6i\n", x);

            printf("\nSupported Filters:\n ");
            for (i=1; i<aaxMaxFilter(); i++)
            {
                const char *s = aaxFilterGetNameByType(cfg, i);
                static int len = 1;
                if (aaxIsFilterSupported(cfg, s))
                {
                    len += strlen(s)+1;   /* one for leading space */
                    if (len >= maximumWidth)
                    {
                        printf("\n ");
                        len = strlen(s)+1;
                    }
                    printf(" %s", s);
                }
            }

            printf("\n\nSupported Effects:\n ");
            for (i=1; i<aaxMaxEffect(); i++)
            {
                const char *s = aaxEffectGetNameByType(cfg, i);
                static int len = 1;
                if (aaxIsEffectSupported(cfg, s))
                {
                    len += strlen(s)+1;   /* one for leading space */
                    if (len >= maximumWidth)
                    {
                        printf("\n ");
                        len = strlen(s)+1;
                    }
                    printf(" %s", s);
                }
            }
            printf("\n\n");

            aaxDriverClose(cfg);
            aaxDriverDestroy(cfg);
        }
        else
        {
           enum aaxErrorType err = aaxGetErrorNo();
           printf("Error opening the default device: ");
           printf("%s\n\n", aaxGetErrorString(err));
        }
    }
    else {
        printf("\nDefault driver not available.\n\n");
    }

    return 0;
}
