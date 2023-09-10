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
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>	// getenv
#include <errno.h>	// EINTR
#include <math.h>

#include <aax/aax.h>

#include "base/random.h"
#include "base/geometry.h"
#include "driver.h"

#define	SAMPLE_FREQ		48000
#define FILTER_FREQUENCY	1000

void
_aax_allpass_compute(float fc, float fs, float *a)
{        
    float c = tanf(GMATH_PI*fc/fs);
    *a = (c - 1.0f)/(c + 1.0f);
}

void // d[k] = a1*s[k] + s[k-1] - a1*d[k-1]
_batch_iir_allpass_float(float* d, const float* s, size_t i, float a1)
{
    float smp;

    smp = 0.0f;
    do
    {
        *d = a1*(*s) + smp;
        smp = *s++ - a1*(*d++); // s[k-1] - a1*d[k-1]
    }
    while (--i);
}

void
_batch_phaser_float(float* d, const float* s, size_t i, float a1)
{
   _batch_iir_allpass_float(d, s, i, a1);
   do {
      *d++ += *s++;
   }
   while (--i);
}

void
_aax_ema_compute(float fc, float fs, float *a)
{           
    float n = *a;

    float c = cosf(GMATH_2PI*fc/fs);
    *a = c - 1.0f + sqrtf((0.5f*c*c - 2.0f*c + 1.5f)*n);
}

void // d[k] = (1-a1)*d[k] + a1*s[k-1]
_batch_ema_iir_float(float* d, const float* s, size_t i, float a1)
{
    float smp = 0.0f;
    do
    {
//      smp = (1.0f-a1)*smp + a1*(*s++);
//      smp = smp + a1*(-smp + *s++);
        smp += a1*(*s++ - smp);
        *d++ = smp;
    }
    while (--i);
}

int main()
{
    char *tmp, devname[128], filename[64];
    aaxConfig config;
    int res = 0;

    tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = "/tmp";

    snprintf(filename, 64, "%s/whitenoise.wav", tmp);
    snprintf(devname, 128, "AeonWave on Audio Files: %s", filename);

    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config)
    {
        float src[4*SAMPLE_FREQ], dst[4*SAMPLE_FREQ];
        int i, no_samples = 4*SAMPLE_FREQ;
        aaxEmitter emitter;
        aaxBuffer buffer;
        float dt, a1;

        // fill with white-noise
        _aax_srandom();
        for (i=0; i<no_samples; ++i) {
           src[i] = _aax_random(); dst[i] = 0.0f;
        }

        // EMA frequency filtering
        a1 = 1.0f;
        _aax_ema_compute(FILTER_FREQUENCY, SAMPLE_FREQ, &a1);
        _batch_phaser_float(dst, src, no_samples, a1);

        /** buffer */
        buffer = aaxBufferCreate(config, no_samples, 1, AAX_FLOAT);
        testForError(buffer, "Unable to generate buffer\n");

        res = aaxBufferSetSetup(buffer, AAX_FREQUENCY, SAMPLE_FREQ);
        testForState(res, "aaxBufferSetFrequency");

        res = aaxBufferSetData(buffer, dst);
        testForState(res, "aaxBufferSetData");

        /** mixer */
        res = aaxMixerSetState(config, AAX_INITIALIZED);
        testForState(res, "aaxMixerInit");

        res = aaxMixerSetState(config, AAX_PLAYING);
        testForState(res, "aaxMixerStart");

        /** emitter */
        emitter = aaxEmitterCreate();
        testForError(emitter, "Unable to create a new emitter\n");

        res = aaxEmitterAddBuffer(emitter, buffer);
        testForState(res, "aaxEmitterAddBuffer");

        res = aaxEmitterSetMode(emitter, AAX_LOOPING, AAX_TRUE);
        testForState(res, "aaxEmitterSetMode");

        res = aaxMixerRegisterEmitter(config, emitter);
        testForState(res, "aaxMixerRegisterEmitter");

        printf("writing white noise to: %s\n", filename);
        res = aaxEmitterSetState(emitter, AAX_PLAYING);
        testForState(res, "aaxEmitterStart");

        dt = 0.0f;
        do
        {
            dt += 0.05f;
            msecSleep(50);
            aaxEmitterGetState(emitter);
        }
        while (dt < 1.0f);;

        res = aaxEmitterSetState(emitter, AAX_PROCESSED);
        testForState(res, "aaxEmitterStop");

        res = aaxEmitterRemoveBuffer(emitter);
        testForState(res, "aaxEmitterRemoveBuffer");

        res = aaxBufferDestroy(buffer);
        testForState(res, "aaxBufferDestroy");

        res = aaxMixerDeregisterEmitter(config, emitter);
        res = aaxMixerSetState(config, AAX_STOPPED);
        res = aaxEmitterDestroy(emitter);
    }

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    return res;
}
