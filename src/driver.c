/*
 * Copyright (C) 2008-2023 by Erik Hofman.
 * Copyright (C) 2009-2023 by Adalin B.V.
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* strrchr */
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>	/* access */
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#include <base/logging.h>
#include <base/memory.h>

#include "driver.h"


#define SRC_ADD(p, l, m, s) { \
    size_t sl = strlen(s); \
    if (m && l) *p++ = '|'; \
    if (l > sl) { \
        memcpy(p, s, sl); \
        p += sl; l -= sl; m = 1; *p= 0; \
    } \
}

char *
getDeviceName(int argc, char **argv)
{
    static char devname[256];
    int len = 255;
    char *s;

    /* -d for device name */
    s = getCommandLineOption(argc, argv, "-d");
    if (!s) s = getCommandLineOption(argc, argv, "--device");
    if (s)
    {
        strlcpy((char *)&devname, s, len);
        len -= strlen(s);

        /* -r for a separate renderer */
        s = getCommandLineOption(argc, argv, "-r");
        if (!s) s = getCommandLineOption(argc, argv, "--renderer");
        if (s)
        {
            strncat((char *)&devname, " on ", len);
            len -= 4;

            strncat((char *)&devname, s, len);
        }
        s = (char *)&devname;
    }

    return s;
}

char *
getCaptureName(int argc, char **argv)
{
    static char devname[256];
    int len = 255;
    char *s;

   /* -c for a capture device */
    s = getCommandLineOption(argc, argv, "-c");
    if (!s) s = getCommandLineOption(argc, argv, "--capture");
    if (s)
    {
        strlcpy((char *)&devname, s, len);
        len -= strlen(s);
    }

    return s;
}

char *
getRenderer(int argc, char **argv)
{
    char *renderer = 0;

    /* -r for a separate renderer */
    renderer = getCommandLineOption(argc, argv, "-r");
    if (!renderer) renderer = getCommandLineOption(argc, argv, "--renderer");
    return renderer;
}

int
getNumEmitters(int argc, char **argv)
{
    int rv = 1;
    char *ret;

    /* -n for the number of emitters */
    ret = getCommandLineOption(argc, argv, "-n");
    if (!ret) ret = getCommandLineOption(argc, argv, "--num");
    if (ret) rv = atoi(ret);
    return rv;
}

float
getFrequency(int argc, char **argv)
{
    float rv = 0.0f;
    char *ret = getCommandLineOption(argc, argv, "-f");
    if (!ret) ret = getCommandLineOption(argc, argv, "--frequency");
    if (ret) rv = (float)atof(ret);
    return rv;
}

float
getPitch(int argc, char **argv)
{
    float rv = 1.0f;
    char *ret = getCommandLineOption(argc, argv, "-p");
    if (!ret) ret = getCommandLineOption(argc, argv, "--pitch");
    if (ret) rv = (float)atof(ret);
    return rv;
}

float
getPitchRange(int argc, char **argv)
{
    float rv = 0.0f;
    char *ret = getCommandLineOption(argc, argv, "-p");
    if (!ret) ret = getCommandLineOption(argc, argv, "--pitch");
    if (ret) {
       ret = strchr(ret, '-');
       if (ret) ret++;
    }
    if (ret) rv = (float)atof(ret);
    return rv;
}

float
getPitchTime(int argc, char **argv)
{
    float rv = 0.0f;
    char *ret = getCommandLineOption(argc, argv, "-p");
    if (!ret) ret = getCommandLineOption(argc, argv, "--pitch");
    if (ret) {
       ret = strchr(ret, ':');
       if (ret) ret++;
    }
    if (ret) rv = (float)atof(ret);
    return rv;
}

float
getGain(int argc, char **argv)
{
    float rv = 1.0f;
    char *ret = getCommandLineOption(argc, argv, "-g");
    if (!ret) ret = getCommandLineOption(argc, argv, "--gain");
    if (ret) rv = (float)atof(ret);
    return rv;
}

float
getEnvelopeStage(int argc, char **argv, int stage)
{
    float rv = -1e-6f;
    char *ret = getCommandLineOption(argc, argv, "-g");
    if (!ret) ret = getCommandLineOption(argc, argv, "--gain");
    if (ret) rv = (float)atof(ret);
    if (ret && stage > 0)
    {
	rv = -1e-6f;
        do {
            ret = strchr(ret, '-');
            if (ret) ret++;
        } while (ret && --stage);
        if (ret) rv = (float)atof(ret);
    }
    return rv;
}

float
getGainRange(int argc, char **argv)
{
    return getEnvelopeStage(argc, argv, 1);
}

float
getGainTime(int argc, char **argv)
{
    float rv = 1.0f;
    char *ret = getCommandLineOption(argc, argv, "-g");
    if (!ret) ret = getCommandLineOption(argc, argv, "--gain");
    if (ret) {
       ret = strchr(ret, ':');
       if (ret) ret++;
    }
    if (ret) rv = (float)atof(ret);
    return rv;
}

static float
handleTime(char *ret, float rv)
{
   char *ptr1 = strchr(ret, ':');
   char *ptr2 = ptr1 ? strchr(ptr1+1, ':') : NULL;

   if (ptr2)
   {
      rv = atof(ptr2+1);
      rv += 60.0f*atof(ptr1+1);
      rv += 60.0f*60.0f*atof(ret);
   }
   else if (ptr1)
   {
      rv = atof(ptr1+1);
      rv += 60.0f*atof(ret);
   }
   else { 
      rv = (float)atof(ret);
   }

   return rv;
}

float getTime(int argc, char **argv)
{
    float rv = 0.0f;
    char *ret = getCommandLineOption(argc, argv, "-t");
    if (!ret) ret = getCommandLineOption(argc, argv, "--time");
    if (ret) {
        rv = handleTime(ret, rv);
    }
    return rv;
}

float
getDuration(int argc, char **argv)
{
    float rv = 1.0f;
    char *ret = getCommandLineOption(argc, argv, "-t");
    if (!ret) ret = getCommandLineOption(argc, argv, "--time");
    if (ret) {
        rv = handleTime(ret, rv);
    }
    return rv;
}


int
printCopyright(int argc, char **argv)
{
    char *ret = getCommandLineOption(argc, argv, "-c");
    if (!ret) ret = getCommandLineOption(argc, argv, "--copyright");
    if (ret) {
        printf("%s\n", aaxGetString(AAX_COPYRIGHT_STRING));
    }
    return ret ? -1 : 0;
}

int
getMode(int argc, char **argv)
{
    int mode = AAX_MODE_WRITE_STEREO;
    char *ret = getCommandLineOption(argc, argv, "-m");
    if (!ret) ret = getCommandLineOption(argc, argv, "--mode");
    if (ret)
    {
        if (!strcasecmp(ret, "hrtf")) mode = AAX_MODE_WRITE_HRTF;
        else if (!strcasecmp(ret, "spatial")) mode = AAX_MODE_WRITE_SPATIAL;
        else if (!strcasecmp(ret, "surround")) mode = AAX_MODE_WRITE_SURROUND;
    }
    return mode;
}

char *
getInputFile(int argc, char **argv, const char *filename)
{
    char *fn = getCommandLineOption(argc, argv, "-i");

    if (!fn) fn = getCommandLineOption(argc, argv, "--input");
    if (!fn) fn = (char *)filename;
//  if (access(fn, F_OK|R_OK) < 0) fn = NULL;
    return fn;
}

char *
getInputFileExt(int argc, char **argv, const char *ext, const char *filename)
{
    int elen = strlen(ext);
    char *rv = 0;
    int i;

    for (i=0; i<argc; i++)
    {
        int alen;

        if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output") ||
            !strcmp(argv[i], "-d") || !strcmp(argv[i], "--device") ||
            !strcmp(argv[i], "-r") || !strcmp(argv[i], "--renderer"))
        {
           i++;
           continue;
        }

        alen = strlen(argv[i]);
        if (alen > elen && strcasecmp(argv[i]+alen-elen, ext) == 0)
        {
            rv = argv[i];
            break;
        }
    }
    return rv;
}

char *
getOutputFile(int argc, char **argv, const char *filename)
{
    char *fn = getCommandLineOption(argc, argv, "-o");

    if (!fn) fn = getCommandLineOption(argc, argv, "--output");
    if (!fn) fn = (char *)filename;
    return fn;
}

enum aaxFormat
getAudioFormat(int argc, char **argv, enum aaxFormat format)
{
   char *fn = getCommandLineOption(argc, argv, "-f");
   enum aaxFormat rv = 0;

   if (!fn) fn = getCommandLineOption(argc, argv, "--format");
   if (fn)
   {
      char *ptr = fn+strlen(fn)-strlen("_LE");

      if (!strcasecmp(ptr, "_LE"))
      {
         *ptr = 0;
         rv = AAX_FORMAT_LE;
      }
      else if (!strcasecmp(ptr, "_BE"))
      {
         *ptr = 0;
         rv = AAX_FORMAT_BE;
      }

      if (!strcasecmp(fn, "AAX_PCM8S")) {
         rv = AAX_PCM8S;
      } else if (!strcasecmp(fn, "AAX_PCM16S")) {
         rv |= AAX_PCM16S;
      } else if (!strcasecmp(fn, "AAX_PCM24S")) {
         rv |= AAX_PCM24S;
      } else if (!strcasecmp(fn, "AAX_PCM32S")) {
         rv |= AAX_PCM32S;
      } else if (!strcasecmp(fn, "AAX_FLOAT")) {
         rv |= AAX_FLOAT;
      } else if (!strcasecmp(fn, "AAX_DOUBLE")) {
         rv |= AAX_DOUBLE;
      } else if (!strcasecmp(fn, "AAX_MULAW")) {
         rv = AAX_MULAW;
      } else if (!strcasecmp(fn, "AAX_ALAW")) {
         rv = AAX_ALAW;
      } else if (!strcasecmp(fn, "AAX_IMA4_ADPCM")) {
         rv = AAX_IMA4_ADPCM; 
      } else if (!strcasecmp(fn, "AAX_PCM24S_PACKED")) {
         rv = AAX_PCM24S_PACKED;

      } else if (!strcasecmp(fn, "AAX_PCM8U")) {
         rv = AAX_PCM8U;
      } else if (!strcasecmp(fn, "AAX_PCM16U")) {
         rv |= AAX_PCM16U;
      } else if (!strcasecmp(fn, "AAX_PCM24U")) {
         rv |= AAX_PCM24U;
      } else if (!strcasecmp(fn, "AAX_PCM32U")) {
         rv |= AAX_PCM32U;
      } else if (!strcasecmp(fn, "AAX_AAXS16S") || !strcasecmp(fn, "AAX_AAXS24S")) {
         rv = AAX_AAXS16S;
      } else {
         rv = format;
      }
   }

   return rv;
}

void
testForError(void *p, char const *s)
{
    if (p == NULL)
    {
        int err = aaxGetErrorNo();
        printf("\nError: %s\n", s);
        if (err) {
            printf("%s\n\n", aaxGetErrorString(err));
        }
        exit(-1);
    }
}

void
testForState_int(int res, const char *func, int line)
{
    if (res != AAX_TRUE)
    {
        int err = aaxGetErrorNo();
        printf("%s:\t\t%i   at line: %i\n", func, res, line);
        printf("(%i) %s\n\n", err, aaxGetErrorString(err));
        exit(-1);
    }
}

char *
getCommandLineOption(int argc, char **argv, char const *option)
{
    int slen = strlen(option);
    char *rv = 0;
    int i;

    for (i=0; i<argc; i++)
    {
        if (strncmp(argv[i], option, slen) == 0)
        {
            int alen = strlen(argv[i]);

            if (alen > slen && *(argv[i]+slen) == '=')  rv = argv[i]+slen+1;
            else if (++i<argc) rv = argv[i];
            else rv = "";
        }
    }
    return rv;
}

#ifndef _WIN32
# include <termios.h>

void set_mode(int want_key)
{
    static struct termios old, new;
    if (!want_key) {
        tcsetattr(STDIN_FILENO, TCSANOW, &old);
        return;
    }

    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
}

int get_key()
{
    int c = 0;
    struct timeval tv;
    fd_set fs;
    tv.tv_usec = tv.tv_sec = 0;

    FD_ZERO(&fs);
    FD_SET(STDIN_FILENO, &fs);
    select(STDIN_FILENO + 1, &fs, 0, 0, &tv);

    if (FD_ISSET(STDIN_FILENO, &fs)) {
        c = getchar();
    }
    return c;
}

#else

# include <conio.h>

int get_key()
{
   if (kbhit()) {
      return getch();
   }
   return 0;
}

void set_mode(int want_key)
{
}
#endif

char *strDup(const char *s)
{
    unsigned int len = strlen(s)+1;
    char *p = malloc(len);
    if (p) memcpy(p, s,len);
    return p;
}

aaxBuffer
setFiltersEffects(int argc, char **argv, aaxConfig c, aaxConfig m, aaxFrame f, aaxEmitter e, const char *aaxs)
{
    static char fname[256];
    aaxBuffer buffer = NULL;
    int len = 255;
    char *s;

   /* -c for a capture device */
    s = getCommandLineOption(argc, argv, "-x");
    if (!s) s = getCommandLineOption(argc, argv, "--aaxs");
    if (s)
    {
        strlcpy((char *)&fname, s, len);
        len -= strlen(s);

        buffer = aaxBufferReadFromStream(c, fname);
    }

    if (!buffer && aaxs)
    {
        buffer = aaxBufferCreate(c, 1, 1, AAX_AAXS16S);
        if (buffer && !aaxBufferSetData(buffer, aaxs))
        {
            printf("Error: %s\n", aaxGetErrorString(aaxGetErrorNo()));
            aaxBufferDestroy(buffer);
            buffer = NULL;
        }
    }

    if (buffer)
    {
       if (m) aaxMixerAddBuffer(m, buffer);
       if (f) aaxAudioFrameAddBuffer(f, buffer);
       if (e) aaxEmitterAddBuffer(e, buffer);
    }

    return buffer;
}


float
_vec3dMagnitude(const aaxVec3d v)
{
   double val = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
   return sqrt(val);
}

float
_vec3fMagnitude(const aaxVec3f v)
{
   float val = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
   return sqrtf(val);
}

const char*
getFormatString(enum aaxFormat format)
{
   static const char* _format_s[AAX_FORMAT_MAX] = {
      "signed, 8-bits per sample",
      "signed, 16-bits per sample",
      "signed, 24-bits per sample, 32-bit encoded",
      "signed, 32-bits per sample",
      "32-bit floating point, range: -1.0 to 1.0",
      "64-bit floating point, range: -1.0 to 1.0",
      "mulaw, 16-bit with 2:1 compression",
      "alaw, 16-bit with 2:1 compression",
      "IMA4 ADPCM, 16-bit with 4:1 compression",
      "signed, 24-bits per sample, 24-bit encoded"
   };
   static const char* _format_us[] = {
      "unsigned, 8-bits per sample",
      "unsigned, 16-bits per sample",
      "unsigned, 24-bits per sample, 32-bit encoded",
      "unsigned, 32-bits per sample"
   };
   int pos = format & AAX_FORMAT_NATIVE;
   const char *rv = "";

   if (pos < AAX_FORMAT_MAX)
   {
      if (format & AAX_FORMAT_UNSIGNED && pos <= AAX_PCM32S) {
         rv = _format_us[pos];
      } else {
         rv = _format_s[pos];
      }
   }

   return rv;
}

char*
getSourceString(enum aaxSourceType type, char freqfilter, char delay)
{
    enum aaxSourceType ntype = type & AAX_NOISE_MASK;
    enum aaxSourceType stype = type & AAX_SOURCE_MASK;
    char rv[1024] = "none";
    int l = 1024;
    char *p = rv;
    char m = 0;

    if (type & AAX_INVERSE) {
        SRC_ADD(p, l, m, "inverse-");
    }

    m = 0;
    /* AAX_CONSTANT, AAX_IMPULSE and noises are different for
       frequency filters. */
    switch(stype)
    {
    case AAX_SAWTOOTH:
        SRC_ADD(p, l, m, "sawtooth");
        break;
    case AAX_SQUARE:
        SRC_ADD(p, l, m, "square");
        break;
    case AAX_TRIANGLE:
        SRC_ADD(p, l, m, "triangle");
        break;
    case AAX_SINE:
        SRC_ADD(p, l, m, "sine");
        break;
    case AAX_CYCLOID:
        SRC_ADD(p, l, m, "cycloid");
        break;
    case AAX_ENVELOPE_FOLLOW:
        SRC_ADD(p, l, m, "envelope");
        break;
    case AAX_TIMED_TRANSITION:
        SRC_ADD(p, l, m, "timed");
        break;
    case AAX_RANDOMNESS:
        SRC_ADD(p, l, m, "randomness");
        break;
    case AAX_RANDOM_SELECT:
        SRC_ADD(p, l, m, "random");
        break;
    case AAX_PURE_SAWTOOTH:
        SRC_ADD(p, l, m, "pure-sawtooth");
        break;
    case AAX_PURE_SQUARE:
        SRC_ADD(p, l, m, "pure-square");
        break;
    case AAX_PURE_TRIANGLE:
        SRC_ADD(p, l, m, "pure-triangle");
        break;
    case AAX_PURE_SINE:
        SRC_ADD(p, l, m, "pure-sine");
        break;
    case AAX_PURE_CYCLOID:
        SRC_ADD(p, l, m, "pure-cycloid");
        break;
    case AAX_WAVE_NONE:
    default:
        break;
    }

    if (delay)
    {
        int order = type & AAX_ORDER_MASK;
        if (order & AAX_EFFECT_1ST_ORDER) {
            SRC_ADD(p, l, m, "1st-order");
        } else if (order & AAX_EFFECT_2ND_ORDER) {
            SRC_ADD(p, l, m, "2nd-order");
        }

        if (order == AAX_1STAGE) {
            SRC_ADD(p, l, m, "1-stage");
        } else if (order == AAX_2STAGE) {
            SRC_ADD(p, l, m, "2-stage");
        } else if (order == AAX_3STAGE) {
            SRC_ADD(p, l, m, "3-stage");
        } else if (order == AAX_4STAGE) {
            SRC_ADD(p, l, m, "4-stage");
        } else if (order == AAX_5STAGE) {
            SRC_ADD(p, l, m, "5-stage");
        } else if (order == AAX_6STAGE) {
            SRC_ADD(p, l, m, "6-stage");
        } else if (order == AAX_7STAGE) {
            SRC_ADD(p, l, m, "7-stage");
        } else if (order == AAX_8STAGE) {
            SRC_ADD(p, l, m, "8-stage");
        }
    }

    if (freqfilter)
    {
        switch(type & AAX_ORDER_MASK)
        {
        case AAX_6DB_OCT:
            SRC_ADD(p, l, m, "6db");
            break;
        case AAX_12DB_OCT:
            SRC_ADD(p, l, m, "12db");
            break;
        case AAX_24DB_OCT:
            SRC_ADD(p, l, m, "24db");
            break;
        case AAX_36DB_OCT:
            SRC_ADD(p, l, m, "36db");
            break;
        case AAX_48DB_OCT:
            SRC_ADD(p, l, m, "48db");
            break;
        case AAX_RESONANCE_FACTOR:
            SRC_ADD(p, l, m, "Q");
            break;
        default:
            break;
        }

        if (type & AAX_BESSEL) {
            SRC_ADD(p, l, m, "bessel");
        }

        if (type & AAX_LFO_EXPONENTIAL) {
            SRC_ADD(p, l, m, "logarithmic");
        }
    }
    else
    {
        switch(stype)
        {
        case AAX_CONSTANT:
            SRC_ADD(p, l, m, "true");
            break;
        case AAX_IMPULSE:
            SRC_ADD(p, l, m, "impulse");
            break;
        default:
            break;
        }

        switch(ntype)
        {
        case AAX_WHITE_NOISE:
            SRC_ADD(p, l, m, "white-noise");
            break;
        case AAX_PINK_NOISE:
            SRC_ADD(p, l, m, "pink-noise");
            break;
        case AAX_BROWNIAN_NOISE:
            SRC_ADD(p, l, m, "brownian-noise");
            break;
        default:
            break;
        }

        if (type & AAX_LFO_EXPONENTIAL) {
            SRC_ADD(p, l, m, "exponential");
        }
    }

    if (!strcmp(rv, "none")) return NULL;
    return strdup(rv);
}


/*
 * Mix a newly generated waveform-, or noise-type with the existing sound data
 * of the buffer.
 *
 * The buffer parameter is the handle as returned by aaxBufferCreate.
 *
 * The rate parameter specifies;
 * - the frequency of the waveform-type (between 1 Hz and 22050 Hz).
 * - how static a noise-type will sound (0.0 for non-static and 1.0 for highly
 *   static).
 *
 * The type parameter is an enumerated type aaxWaveformType and specifies which
 * waveform-types to mix with the existing sound data.
 *
 * The ratio parameter specifies the mixing ratio of the new waveform and the
 * existing data. A value of 1.0 will overwrite existing data and a value of
 * 0.0 will preserve existing data. A value of 0.5 mixes the new waveform and
 * existing data at an equal level.
 *
 * The ptype parameter is an enumerated type aaxProcessingType and specifies
 * how the newly generated waveform-type should act upon the current data in
 * the buffer. Possible values;
 *  - AAX_OVERWRITE, AAX_ADD, AAX_MIX, AAX_RINGMODULATE, AAX_APPEND
 */
int
bufferProcessWaveform(aaxBuffer buffer, float rate, enum aaxSourceType stype,
                      float ratio, enum aaxProcessingType ptype)
{
    static char been_here = 0;
    static char *proc_type[] = {
     "none", "overwrite", "add", "mix", "modulate", "append"
    };
    static const char aax_fmt[] = "<?xml version=\"1.0\"?>\n\
<aeonwave>\n <sound frequency=\"%.0f\">\n </sound>\n</aeonwave>";
    static char aaxs[4097];
    int rv = AAX_FALSE;
    float freq;

    freq = aaxBufferGetSetup(buffer, AAX_BASE_FREQUENCY);
    if (!been_here || ptype == AAX_OVERWRITE || ratio == 1.0f)
    {
        freq = rate;
        snprintf(aaxs, 4096, aax_fmt, freq);
        been_here = 1;
    }

    if (ratio > 0.0f && ptype < AAX_PROCESSING_MAX)
    {
        char *ptr = strstr(aaxs, " </sound>");
        if (ptr)
        {
            char *processing = proc_type[ptype];
            char *waveform = getSourceString(stype, 0, 0);
            char *inverse = (stype & AAX_INVERSE) ? "inverse-" : "";
            int len = 4096 - (ptr-aaxs);

            if (stype >= AAX_1ST_WAVE && stype <= AAX_LAST_WAVE)
            {
                float pitch = rate/freq;
                snprintf(ptr, len, "  <waveform src=\"%s%s\" processing=\"%s\" ratio=\"%.3f\" pitch=\"%.3f\"/>\n </sound>\n</aeonwave>", inverse, waveform, processing, ratio, pitch);
            }
            else if (stype >= AAX_1ST_NOISE && stype <= AAX_LAST_NOISE)
            {
                snprintf(ptr, len, "  <waveform src=\"%s%s\" processing=\"%s\" ratio=\"%.3f\" staticity=\"%.3f\"/>\n </sound>\n</aeonwave>", inverse, waveform, processing, ratio, rate);

            }
            aaxBufferSetSetup(buffer, AAX_FORMAT, AAX_AAXS16S);
            rv = aaxBufferSetData(buffer, aaxs);
        }
    }
    else {
        rv = AAX_TRUE;
    }

    return rv;
}
