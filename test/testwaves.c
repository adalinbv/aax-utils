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

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"

#define	SAMPLE_FREQ		22050
#define SAMPLE_FORMAT		AAX_PCM16S
#define FILE_PATH		SRC_PATH"/stereo.wav"
#define MAX_WAVES		(AAX_MAX_WAVE+AAX_MAX_NOISE)

static struct {
    char* name;
    float rate;
    enum aaxSourceType type;
} buf_info[MAX_WAVES] =
{
  { "sawtooth",         440.0f, AAX_SAWTOOTH       },
  { "square wave",      440.0f, AAX_SQUARE         },
  { "triangle wave",    440.0f, AAX_TRIANGLE       },
  { "sine wave",        440.0f, AAX_SINE           },
  { "cycloid",          440.0f, AAX_CYCLOID        },
  { "impulse",          440.0f, AAX_IMPULSE        },
  { "white noise",        0.0f, AAX_WHITE_NOISE    },
  { "pink noise",         0.0f, AAX_PINK_NOISE     },
  { "brownian noise",     0.0f, AAX_BROWNIAN_NOISE },
  { "static white noise", 1.0f, AAX_WHITE_NOISE    }
};

aaxVec3d EmitterPos = { 0.0,   0.0,  -3.0  };
aaxVec3f EmitterDir = { 0.0f,  0.0f,  1.0f };

int main(int argc, char **argv)
{
    aaxConfig config;
    int res, rv = 0;
    char *devname;

    devname = getDeviceName(argc, argv);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config && (rv >= 0))
    {
        char fname[1024], *tmp, *outfile;
        aaxConfig file = NULL;
        aaxBuffer buffer[MAX_WAVES];
        aaxEmitter emitter;
        aaxFilter filter;
        int state, buf, i;
        float pitch;
        aaxMtx4d mtx64;

        tmp = getenv("TEMP");
        if (!tmp) tmp = getenv("TMP");
        if (!tmp) tmp = "/tmp";

        pitch = getPitch(argc, argv);

        for (i=0; i<MAX_WAVES; i++)
        {
            float rate = buf_info[i].rate;
            int type = buf_info[i].type;
            unsigned int no_samples;

            no_samples = (unsigned int)(1.1f*SAMPLE_FREQ);
            buffer[i] = aaxBufferCreate(config, no_samples, 1, SAMPLE_FORMAT);
            testForError(buffer, "Unable to generate buffer\n");

            res = aaxBufferSetSetup(buffer[i], AAX_FREQUENCY, SAMPLE_FREQ);
            testForState(res, "aaxBufferSetFrequency");

            res = bufferProcessWaveform(buffer[i], pitch*rate, type,
                                        1.0f, AAX_ADD);
            testForState(res, "bufferProcessWaveform");

#if 0
            snprintf(fname, 1024, "%s/%s.wav", tmp, buf_info[i].name);
            printf("saving the audio buffer to %s\n", fname);
            aaxBufferWriteToFile(buffer[i], fname, AAX_OVERWRITE);
#endif
        }

        /** emitter */
        emitter = aaxEmitterCreate();
        testForError(emitter, "Unable to create a new emitter\n");

        res = aaxEmitterSetMode(emitter, AAX_POSITION, AAX_ABSOLUTE);
        testForState(res, "aaxEmitterSetMode");

        res = aaxMatrix64SetDirection(mtx64, EmitterPos, EmitterDir);
        testForState(res, "aaxMatrix64SetDirection");

        res = aaxEmitterSetMatrix64(emitter, mtx64);
        testForState(res, "aaxSensorSetMatrix64");

        /** mixer */
        res = aaxMixerSetState(config, AAX_INITIALIZED);
        testForState(res, "aaxMixerInit");

        filter = aaxFilterCreate(config, AAX_DISTANCE_FILTER);
        testForError(filter, "Unable to create the distance filter");

        res = aaxFilterSetState(filter, AAX_EXPONENTIAL_DISTANCE_DELAY);
        testForState(res, "aaxFilterSetState");

        res = aaxScenerySetFilter(config, filter);
        testForState(res, "aaxScenerySetDistanceModel");
        aaxFilterDestroy(filter);

        res = aaxMixerRegisterEmitter(config, emitter);
        testForState(res, "aaxMixerRegisterEmitter");

        res = aaxMixerSetState(config, AAX_PLAYING);
        testForState(res, "aaxMixerStart");

        /* frequency filter */
#if 0
        filter = aaxFilterCreate(config, AAX_FREQUENCY_FILTER);
        testForError(filter, "aaxFilterCreate");

        res = aaxFilterSetSlot(filter, 0, AAX_LINEAR, 500.0f,
                                             0.33f, 3.33f, 1.0);
        testForState(res, "aaxFilterSetSlot");

        res = aaxFilterSetState(filter, AAX_24DB_OCT);
        testForState(res, "aaxFilterSetState");

        res = aaxEmitterSetFilter(emitter, filter);
        testForState(res, "aaxEmitterSetFilter");

        res = aaxFilterDestroy(filter);
#endif

        outfile = getOutputFile(argc, argv, NULL);
        if (outfile)
        {
            snprintf(fname, 256, "AeonWave on Audio Files: %s", outfile);
            file = aaxDriverOpenByName(fname, AAX_MODE_WRITE_STEREO);

            res = aaxMixerRegisterSensor(config, file);
            testForState(res, "aaxMixerRegisterSensor file out");

            res = aaxMixerSetState(file, AAX_INITIALIZED);
            testForState(res, "aaxMixerSetInitialize");

            res = aaxMixerSetState(file, AAX_PLAYING);
            testForState(res, "aaxSensorCaptureStart");
        }

        for (buf=0; buf<MAX_WAVES; buf++)
        {
            float dt = 0.0f;

            res = aaxEmitterAddBuffer(emitter, buffer[buf]);
            testForState(res, "aaxEmitterAddBuffer");

            res = aaxEmitterSetMode(emitter, AAX_LOOPING, AAX_TRUE);
            testForState(res, "aaxEmitterSetMode");

            /** schedule the emitter for playback */
            res = aaxEmitterSetState(emitter, AAX_PLAYING);
            testForState(res, "aaxEmitterStart");

            printf("playing buffer #%i: %s\n", buf, buf_info[buf].name);
            do
            {
                dt += 0.05f;
                msecSleep(50);
                state = aaxEmitterGetState(emitter);
            }
            while (dt < 1.0f); // state == AAX_PLAYING);

            res = aaxEmitterSetState(emitter, AAX_STOPPED);
            testForState(res, "aaxEmitterStop");

            do
            {
                msecSleep(50);
                state = aaxEmitterGetState(emitter);
            }
            while (state != AAX_PROCESSED);

            res = aaxEmitterRemoveBuffer(emitter);
            testForState(res, "aaxEmitterRemoveBuffer");

            res = aaxBufferDestroy(buffer[buf]);
            testForState(res, "aaxBufferDestroy");
        }

        if (file)
        {
            res = aaxMixerSetState(file, AAX_STOPPED);
            testForState(res, "aaxMixerSetState");

            res = aaxMixerDeregisterSensor(config, file);
            testForState(res, "aaxMixerRegisterSensor file out");

            res = aaxDriverClose(file);
            testForState(res, "aaxDriverClose");

            res = aaxDriverDestroy(file);
            testForState(res, "aaxDriverDestroy");
        }

        res = aaxMixerDeregisterEmitter(config, emitter);
        res = aaxMixerSetState(config, AAX_STOPPED);
        res = aaxEmitterDestroy(emitter);
    }

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    return rv;
}
