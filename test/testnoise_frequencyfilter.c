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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>	// getenv
#include <errno.h>	// EINTR


#include <aax/aax.h>

#include "base/timer.h"
#include "driver.h"

#define	SAMPLE_FREQ		48000
#define FILTER_FREQUENCY	1000

// Order: AAX_6DB_OCT, AAX_12DB_OCT, AAX_24DB_OCT, AAX_36DB_OCT, AAX_48DB_OCT
// Type:  AAX_BUTTERWORTH, AAX_BESSEL
#define Q			1.0f
#define LF_GAIN			1.0f
#define HF_GAIN			0.0f
#define FILTER_ORDER		AAX_48DB_OCT
#define FILTER_TYPE		AAX_BUTTERWORTH
#define FILTER_STATE		(FILTER_TYPE|FILTER_ORDER)

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
        int no_samples;
        aaxEmitter emitter;
        aaxBuffer buffer;
        aaxFilter filter;
        float dt;

        if (aaxIsFilterSupported(config, "AAX_frequency_filter") == 0) {
           printf("AAX_frequency_filter not supported\n");
        }
        if (aaxIsFilterSupported(config, "AAX_frequency_filter_1.0") == 0) {
           printf("AAX_frequency_filter version 1.0 not supported\n");
        }
        if (aaxIsFilterSupported(config, "AAX_frequency_filter_1.1") == 0) {
           printf("AAX_frequency_filter version 1.1 not supported\n");
        }

        no_samples = (unsigned int)(4*SAMPLE_FREQ);
        buffer = aaxBufferCreate(config, no_samples, 1, AAX_PCM16S);
        testForError(buffer, "Unable to generate buffer\n");

        res = aaxBufferSetSetup(buffer, AAX_FREQUENCY, SAMPLE_FREQ);
        testForState(res, "aaxBufferSetFrequency");

        res = bufferProcessWaveform(buffer, 0.0f, AAX_WHITE_NOISE,
                                    1.0f, AAX_ADD);
        testForState(res, "bufferProcessWaveform");

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

        /* frequency filter */
        filter = aaxFilterCreate(config, AAX_FREQUENCY_FILTER);
        testForError(filter, "aaxFilterCreate");

        res = aaxFilterSetSlot(filter, 0, AAX_LINEAR, FILTER_FREQUENCY,
                                             LF_GAIN, HF_GAIN, Q);
        testForState(res, "aaxFilterSetSlot");

        res = aaxFilterSetState(filter, FILTER_STATE);
        testForState(res, "aaxFilterSetState");

        res = aaxEmitterSetFilter(emitter, filter);
        testForState(res, "aaxEmitterSetFilter");

        res = aaxFilterDestroy(filter);

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
