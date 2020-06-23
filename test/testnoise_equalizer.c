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
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>	// getenv
#include <errno.h>	// EINTR

#include <aax/aax.h>

#include "base/timer.h"
#include "driver.h"

#define NUM_FILTERS		3
#define	SAMPLE_FREQ		48000

#define LF_FILTER_FREQUENCY	 100.0f
#define LF_GAIN 		0.2f		//
#define LF_Q			1.2f

#define MF_FILTER_FREQUENCY	1000.0f
#define MF_GAIN1		1.0f		//
#define MF_Q			1.5f

#define HF_FILTER_FREQUENCY	8000.0f
#define MF_GAIN2		0.1f		//
#define HF_GAIN			0.0f		//
#define HF_Q			1.5f


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
        int no_samples; // state
        aaxEmitter emitter;
        aaxBuffer buffer;
        aaxFilter filter;
        float dt;

        no_samples = (unsigned int)(4*SAMPLE_FREQ);
        buffer = aaxBufferCreate(config, no_samples, 1, AAX_PCM16S);
        testForError(buffer, "Unable to generate buffer\n");

        res = aaxBufferSetSetup(buffer, AAX_FREQUENCY, SAMPLE_FREQ);
        testForState(res, "aaxBufferSetFrequency");

        res = aaxBufferProcessWaveform(buffer, 0.0f, AAX_WHITE_NOISE, 1.0f,
                                       AAX_OVERWRITE);
        testForState(res, "aaxBufferProcessWaveform");

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

        printf("%2.1f | %5.0f Hz | %2.1f | %5.0f Hz | %2.1f | %5.0f Hz | %2.1f\n",
                LF_GAIN, LF_FILTER_FREQUENCY, MF_GAIN1, MF_FILTER_FREQUENCY,
                MF_GAIN2, HF_FILTER_FREQUENCY, HF_GAIN);

        /* equalizer */
        filter = aaxFilterCreate(config, AAX_EQUALIZER);
        testForError(filter, "aaxFilterCreate");

        res = aaxFilterSetSlot(filter, 0, AAX_LINEAR, LF_FILTER_FREQUENCY,
                                             LF_GAIN, MF_GAIN1, LF_Q);
        testForState(res, "aaxFilterSetSlot 0");

#if (NUM_FILTERS > 1)
        res = aaxFilterSetSlot(filter, 1, AAX_LINEAR, MF_FILTER_FREQUENCY,
                                             MF_GAIN1, MF_GAIN2, MF_Q);
        testForState(res, "aaxFilterSetSlot 1");
#endif
#if (NUM_FILTERS > 2)
        res = aaxFilterSetSlot(filter, 2, AAX_LINEAR, HF_FILTER_FREQUENCY,
                                             MF_GAIN2 , HF_GAIN, HF_Q);
        testForState(res, "aaxFilterSetSlot 2");
#endif
        res = aaxFilterSetState(filter, AAX_TRUE);
        testForState(res, "aaxFilterSetState");

        res = aaxMixerSetFilter(config, filter);
        testForState(res, "aaxMixerSetFilter");

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
//          state = aaxEmitterGetState(emitter);
        }
        while (dt < 1.0f); // state == AAX_PLAYING);

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
