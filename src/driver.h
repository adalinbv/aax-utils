/*
 * Copyright (C) 2008-2022 by Erik Hofman.
 * Copyright (C) 2009-2022 by Adalin B.V.
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

#ifndef __DRIVER_H_
#define __DRIVER_H_

#if defined(__cplusplus)
extern "C" {
#endif

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <aax/aax.h>

#ifdef NDEBUG
# include <malloc.h>
#else
# include <base/logging.h>
#endif
#include <base/types.h>
#include <base/timer.h>

#define TAB_KEY		0x09
#define SPACE_KEY	0x20


#define TRY(a) do { \
    if (!(a)) printf("Error at line %i: %s\n", __LINE__, aax.strerror()); \
} while(0)

struct mmap_t {
    int fd;
    size_t len;
    char*start;
#ifdef WIN32
    struct {
        HANDLE m;
        void *p;
    } un;
#else
    int un;                             /* referenced but not used */
#endif
};

#ifdef WIN32
#define setenv(a,b,c)	SetEnvironmentVariable((a), (b))
#define unsetenv(a)	SetEnvironmentVariable((a), NULL)

typedef struct
{
    HANDLE m;
    void *p;
} SIMPLE_UNMMAP;

void * simple_mmap(int fd, size_t length, SIMPLE_UNMMAP *un);
void simple_unmmap(void *addr, size_t len, SIMPLE_UNMMAP *un);
#else
# define simple_mmap(a, b, c)   mmap(0, (b), PROT_READ, MAP_PRIVATE, (a), 0L)
# define simple_unmmap(a, b, c) munmap((a), (b))
#endif

void set_mode(int want_key);
int get_key();

char* getDeviceName(int, char**);
char* getCaptureName(int, char**);
char* getCommandLineOption(int, char**, char const *);
char* getInputFile(int, char**, const char*);
char* getInputFileExt(int, char**, const char*, const char*);
char* getOutputFile(int, char**, const char*);
enum aaxFormat getAudioFormat(int, char**, enum aaxFormat);
const char* getFormatString(enum aaxFormat format);
int getNumEmitters(int, char**);
float getFrequency(int, char**);
float getPitch(int, char**);
float getPitchRange(int, char**);
float getPitchTime(int, char**);
float getGain(int, char**);
float getGainRange(int, char**);
float getGainTime(int, char**);
float getTime(int, char**);
float getDuration(int, char**);
int getMode(int, char**);
char* getRenderer(int, char**);
aaxBuffer setFiltersEffects(int, char**, aaxConfig, aaxConfig, aaxFrame, aaxEmitter, const char*);
int printCopyright(int, char**);
char* strDup(const char*);

#define testForState(a,b)	testForState_int((a),(b),__LINE__)

void testForError(void *, char const *);
void testForState_int(int, const char*, int);
void testForALCError(void *);
void testForALError();

/* geometry */
float _vec3dMagnitude(const aaxVec3d v);
float _vec3fMagnitude(const aaxVec3f v);


#if defined(__cplusplus)
}  /* extern "C" */
#endif

#endif

