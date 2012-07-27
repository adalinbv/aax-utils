/*
 * Copyright (C) 2008-2012 by Erik Hofman.
 * Copyright (C) 2009-2012 by Adalin B.V.
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

#include <aax/defines.h>

#include <base/geometry.h>
#include <base/types.h>
#include "driver.h"
#include "wavfile.h"

#define SAMPLE_FREQUENCY	22050

aaxVec3f EmitterPos = { 0.0f, 0.0f, -1.0f };
aaxVec3f EmitterDir = { 0.0f, 0.0f,  1.0f };

aaxVec3f SensorPos = { 0.0f, 0.0f,  0.0f };
aaxVec3f SensorAt  = { 0.0f, 0.0f, -1.0f };
aaxVec3f SensorUp  = { 0.0f, 1.0f,  0.0f };

int main(int argc, char **argv)
{
    aaxConfig config;
    int res, rv = 0;
    char *devname;

    devname = getDeviceName(argc, argv);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (!aaxIsValid(config, AAX_CONFIG_HD))
    {
        printf("Warning:\n");
        printf("  %s requires a registered version of AeonWave\n", argv[0]);
        printf("  Please visit http://www.adalin.com/buy_aeonwaveHD.html to ");
        printf("obtain\n  a product-key.\n\n");
        rv = -1;

        goto finish;
    }

    if (config)
    {
        unsigned int no_samples;
        aaxEmitter emitter;
        aaxBuffer buffer;
        aaxFilter filter;
        aaxMtx4f mtx;
        float pitch;

        no_samples = (unsigned int)(1.1f*SAMPLE_FREQUENCY);
        buffer = aaxBufferCreate(config, no_samples, 1, AAX_PCM16S);
        testForError(buffer, "Unable to generate buffer\n");

        res = aaxBufferSetFrequency(buffer, SAMPLE_FREQUENCY);
        testForState(res, "aaxBufferSetFrequency");

        res = aaxBufferSetWaveform(buffer, 1250.0f, AAX_TRIANGLE_WAVE);
        res = aaxBufferMixWaveform(buffer, 1500.0f, AAX_SINE_WAVE, 0.6f);
        res=aaxBufferRingmodulateWaveform(buffer, 500.0f, AAX_SINE_WAVE, 0.6f);
        /* res = aaxBufferAddWaveform(buffer, 0.0f, AAX_PINK_NOISE, 0.08f); */
        testForState(res, "aaxBufferProcessWaveform");

        /** emitter */
        emitter = aaxEmitterCreate();
        testForError(emitter, "Unable to create a new emitter\n");

        res = aaxEmitterAddBuffer(emitter, buffer);
        testForState(res, "aaxEmitterAddBuffer");

        pitch = getPitch(argc, argv);
        res = aaxEmitterSetPitch(emitter, pitch);
        testForState(res, "aaxEmitterSetPitch");

        printf("Locate the emitter 5 meters in front of the sensor\n");
        res = aaxMatrixSetDirection(mtx, EmitterPos, EmitterDir);
        testForState(res, "aaxMatrixSetDirection");

        res = aaxEmitterSetMatrix(emitter, mtx);
        testForState(res, "aaxEmitterSetMatrix");

        res = aaxEmitterSetMode(emitter, AAX_POSITION, AAX_ABSOLUTE);
        testForState(res, "aaxEmitterSetMode");

        res = aaxEmitterSetMode(emitter, AAX_LOOPING, AAX_TRUE);
        testForState(res, "aaxEmitterSetLooping");

        /* tremolo filter for emitter */
        filter = aaxFilterCreate(config, AAX_TREMOLO_FILTER);
        testForError(filter, "aaxFilterCreate");

        filter = aaxFilterSetSlot(filter, 0, AAX_LINEAR,
                                          0.0f, 2.8f, 0.4f, 0.0f);
        testForError(filter, "aaxFilterSetSlot");

        filter = aaxFilterSetState(filter, AAX_SINE_WAVE);
        testForError(filter, "aaxFilterSetState");

        res = aaxEmitterSetFilter(emitter, filter);
        testForState(res, "aaxEmitterSetFilter");

        res = aaxFilterDestroy(filter);
        testForState(res, "aaxFilterDestroy");

        /** mixer */
        res = aaxMixerInit(config);
        testForState(res, "aaxMixerInit");

        res = aaxMixerRegisterEmitter(config, emitter);
        testForState(res, "aaxMixerRegisterEmitter");

        res = aaxMixerSetState(config, AAX_PLAYING);
        testForState(res, "aaxMixerStart");

        res = aaxMatrixSetOrientation(mtx, SensorPos, SensorAt, SensorUp);
        testForState(res, "aaxMatrixSetOrientation");

        /* tremolo filter for mixer */
        filter = aaxFilterCreate(config, AAX_TREMOLO_FILTER);
        testForError(filter, "aaxFilterCreate");

        filter = aaxFilterSetSlot(filter, 0, AAX_LINEAR,
                                          0.0f, 0.9f, 0.5f, 0.0f);
        testForError(filter, "aaxFilterSetSlot");

        filter = aaxFilterSetState(filter, AAX_TRIANGLE_WAVE);
        testForError(filter, "aaxFilterSetState");

        res = aaxMixerSetFilter(config, filter);
        testForState(res, "aaxMixerSetFilter");

        res = aaxFilterDestroy(filter);
        testForState(res, "aaxFilterDestroy");

        /** sensor */
        res = aaxMatrixInverse(mtx);
        res = aaxSensorSetMatrix(config, mtx);
        testForState(res, "aaxSensorSetMatrix");

        /** schedule the emitter for playback */
        res = aaxEmitterSetState(emitter, AAX_PLAYING);
        testForState(res, "aaxEmitterStart");

        do
        {
            static int i = 0;

            msecSleep(2000);	/* 2 seconds */

            switch(i)
            {
            case 0:
                printf("rotate the sensor 90 degrees counter-clockwise\n");
                printf("the emitter is now 5 meters to the left\n");
                SensorAt[0] =  1.0f;
                SensorAt[1] =  0.0f;
                SensorAt[2] =  0.0f;
                res=aaxMatrixSetOrientation(mtx, SensorPos, SensorAt, SensorUp);
                break;
            case 1:
                printf("translate the sensor 5 meters backwards\n");
                printf("the emitter is now 5 meters the left and 5 "
                       "meters in front\n");
                res=aaxMatrixSetOrientation(mtx, SensorPos, SensorAt, SensorUp);
                res = aaxMatrixTranslate(mtx, 0.0f, 0.0f, -5.0f);
                break;
            case 2:
                printf("Rotate the sensor 90 degrees counter clockwise\n");
                res = aaxMatrixSetOrientation(mtx, SensorPos,
                                                   SensorAt, SensorUp);
                res = aaxMatrixTranslate(mtx, 0.0f, 0.0f, -5.0f);
                res = aaxMatrixRotate(mtx, -90.0f*GMATH_DEG_TO_RAD,
                                           0.0f, 1.0f, 0.0f);
                break;
            default:
                break;
            }
            res = aaxMatrixInverse(mtx);
            res = aaxSensorSetMatrix(config, mtx);

            if (++i == 4) break;
        }
        while (1);

        res = aaxEmitterStop(emitter);
        res = aaxEmitterRemoveBuffer(emitter);
        testForState(res, "aaxEmitterRemoveBuffer");

        res = aaxBufferDestroy(buffer);
        testForState(res, "aaxBufferDestroy");

        res = aaxMixerDeregisterEmitter(config, emitter);
        res = aaxMixerSetState(config, AAX_STOPPED);
        res = aaxEmitterDestroy(emitter);
    }

finish:
    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    return rv;
}
