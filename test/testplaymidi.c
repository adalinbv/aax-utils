/*
 * Copyright (C) 2018-2020 by Erik Hofman.
 * Copyright (C) 2018-2020 by Adalin B.V.
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
#include <assert.h>
#ifdef HAVE_RMALLOC_H
# include <rmalloc.h>
#else
# include <string.h>
# if HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>

#include <aax/aax.h>
#include <aax/midi.h>

#include "driver.h"


#define MAX_TRACKS		16
#define IFILE_PATH		SRC_PATH"/beethoven_opus10_3.mid"

struct track_data_t
{
    char *ptr;
    char *start;
    size_t len;

};

struct midi_data_t
{
    uint16_t PPQN;
    uint32_t uSPP;
    uint16_t format;

    unsigned no_tracks;
    struct track_data_t track[MAX_TRACKS];
};

struct midi_file_t
{
    struct mmap_t mm;
    struct midi_data_t midi;
};

uint8_t
getInt8(char **start, const char *end)
{
    char *ptr = *start;
    uint8_t rv = 0;
    if (ptr+sizeof(uint8_t) < end)
    {
        rv = *ptr++;
        *start = ptr;
    }
    return rv;
}

uint16_t
getInt16(char **start, const char *end)
{
    char *ptr = *start;
    uint16_t rv = 0;
    if (ptr+sizeof(uint16_t) < end)
    {
        rv = (uint16_t)(*ptr++) << 8;
        rv |= (uint16_t)(*ptr++);
        *start = ptr;
    }
    return rv;
}

uint32_t
getInt32(char **start, const char *end)
{
    char *ptr = *start;
    uint32_t rv = 0;
    if (ptr+sizeof(uint32_t) < end)
    {
        rv = (uint32_t)(*ptr++) << 24;
        rv |= (uint32_t)(*ptr++) << 16;
        rv |= (uint32_t)(*ptr++) << 8;
        rv |= (uint32_t)(*ptr++);
        *start = ptr;
    }
    return rv;
}

struct midi_file_t*
midiFileOpen(const char *filename)
{
    struct midi_file_t *file = malloc(sizeof(struct midi_file_t));
    if (file)
    {
        file->mm.fd = open(filename, O_RDONLY);
        if (file->mm.fd >= 0)
        {
            struct stat statbuf;

            fstat(file->mm.fd, &statbuf);
            file->mm.len = statbuf.st_size;
            
            file->mm.start = simple_mmap(file->mm.fd, file->mm.len, &file->mm.un);
            if (file->mm.start != (void*)-1)
            {
            }
        }
        else
        {
            free(file);
            file = NULL;
        }
    }

    return file;
}

void
midiFileClose(struct midi_file_t *file)
{
    simple_unmmap(file->mm.start, file->mm.len, &(file->mm).un);
    close(file->mm.fd);
    file->mm.fd = -1;
}

int
midiFileInitialize(struct midi_file_t *file)
{
    int rv = AAX_FALSE;
    if (file)
    {
        file->midi.no_tracks = 0;
        if (file->mm.len)
        {
            char *ptr = file->mm.start;
            char *end = file->mm.start+file->mm.len;
            uint32_t size, header = getInt32(&ptr, end);
            uint16_t track_no = 0;

            if (header != 0x4d546864) goto abort; // "MThd"

            size = getInt32(&ptr, end);
            if (size != 6) goto abort;

            file->midi.uSPP = 500000/24;

            file->midi.format = getInt16(&ptr, end);
            if (file->midi.format != 0 && file->midi.format != 1) goto abort;

            file->midi.no_tracks = getInt16(&ptr, end);
            if (file->midi.format == 0 && file->midi.no_tracks != 1) goto abort;

            file->midi.PPQN = getInt16(&ptr, end);
            if (file->midi.PPQN & 0x8000) // SMPTE
            {
                uint8_t fps = (file->midi.PPQN >> 8) & 0xff;
                uint8_t resolution = file->midi.PPQN & 0xff;
                if (fps == 232) fps = 24;
                else if (fps == 231) fps = 25;
                else if (fps == 227) fps = 29;
                else if (fps == 226) fps = 30;
                else fps = 0;
                file->midi.PPQN = fps*resolution;
            }

            while ((end-ptr) < sizeof(header))
            {
                header = getInt32(&ptr, end);
                if (header != 0x4d54726b) break;

                size = getInt32(&ptr, end);
                if (size <= sizeof(uint32_t) && size <= (end-ptr))
                {
                    file->midi.track[track_no].start = ptr;
                    file->midi.track[track_no].len = size;
                    file->midi.track[track_no].ptr = ptr;
                    track_no++;
                }
            }
            file->midi.no_tracks = track_no;
            rv = AAX_TRUE;
        }
    }

abort:
    return rv;
}

int
midiTrackProcess(struct track_data_t *track, uint64_t time_offs_parts, uint32_t *elapsed_parts, uint32_t *next)
{
    int rv = track->ptr < (track->start + track->len);

}


int
midiFileProcess(struct midi_file_t *file, uint64_t time_parts, uint32_t *next)
{
    uint32_t elapsed_parts = *next;
    uint32_t nval = *next;
    uint32_t wait_parts;
    char rv = AAX_FALSE;
    size_t t;

    nval = UINT_MAX;
    for (t=0; t<file->midi.no_tracks; ++t)
    {
        wait_parts = nval;

        rv |= midiTrackProcess(&file->midi.track[t], time_parts, &elapsed_parts,
                               &wait_parts);

        if (nval > wait_parts) {
            nval = wait_parts;
        }
    }

    if (nval == UINT_MAX) {
        nval = 100;
    }
    *next = nval;

    return rv;
}
    

int
main(int argc, char **argv)
{
    char *filename, *devname;
    struct midi_file_t *file;
    int64_t sleep_us, dt_us;
    uint64_t time_parts = 0;
    uint32_t wait_parts;
    struct timeval now;
    aaxConfig config;
    aaxMidi midi;
    int res;

    devname = getDeviceName(argc, argv);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    res = aaxMixerSetSetup(config, AAX_REFRESH_RATE, 90.0f);
    testForState(res, "aaxMixerSetSetup");

    res = aaxMixerSetState(config, AAX_INITIALIZED);
    testForState(res, "aaxMixerInit");

    midi = aaxMidiCreate(config);
    testForError(midi, "aaxMidiCreate");

    filename = getInputFile(argc, argv, IFILE_PATH);
    file = midiFileOpen(filename);
    testForError(file, "midiFileOpen");

    res = midiFileInitialize(file);
    testForState(res, "midiFileInitialize");

    res = aaxMixerSetState(config, AAX_PLAYING);
    testForState(res, "aaxMixerStart");

    res = aaxMidiSetState(midi, AAX_INITIALIZED);
    testForState(res, "aaxMidiSetState");

    res = aaxMidiSetState(midi, AAX_PLAYING);
    testForState(res, "aaxMidiSetState");

    wait_parts = 1000;
    set_mode(1);

    gettimeofday(&now, NULL);

    dt_us = -(now.tv_sec * 1000000 + now.tv_usec);
    do
    {
        if (!midiFileProcess(file, time_parts, &wait_parts)) break;

        if (wait_parts > 0)
        {
            uint32_t wait_us;

            gettimeofday(&now, NULL);

            dt_us += now.tv_sec * 1000000 + now.tv_usec;

            wait_us = wait_parts * file->midi.uSPP;
            sleep_us = wait_us - dt_us;

            if (sleep_us > 0) {
                msecSleep(sleep_us*1000);
            }

            gettimeofday(&now, NULL);

            dt_us = -(now.tv_sec * 1000000 + now.tv_usec);
        }
        time_parts += wait_parts;
    }
    while(!get_key());
    set_mode(0);

    res = aaxMidiSetState(midi, AAX_STOPPED);
    res = aaxMidiDestroy(midi);

    res = aaxMixerSetState(config, AAX_STOPPED);

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    midiFileClose(file);
    free(file);

    return 0;
}

