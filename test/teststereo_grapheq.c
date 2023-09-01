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
#include <math.h>

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"

#define FILE_PATH		SRC_PATH"/stereo.wav"

float _lin2db(float v) { return 20.0f*log10f(_MAX(v, 1e-9f)); }
float band[AAX_MAX_BANDS] = {
   4.0f, 2.0f, 1.0f, 0.5f, 0.7f, 0.7f, 1.0f, 0.7f
};

int main(int argc, char **argv)
{
    char *devname, *infile;
    aaxBuffer buffer = 0;
    aaxConfig config;
    int res;

    devname = getDeviceName(argc, argv);
    infile = getInputFile(argc, argv, FILE_PATH);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config)
    {
        buffer = bufferFromFile(config, infile);
        if (buffer)
        {
            aaxEmitter emitter;
            aaxFilter filter;
            aaxEffect effect;
            float dt = 0.0f;
            int i, q, state;
            float pitch;

            printf("\nPlayback stereo with 8-band graphic equalizer enabled.");
            /** emitter */
            emitter = aaxEmitterCreate();
            testForError(emitter, "Unable to create a new emitter");

            /* pitch */
            pitch = getPitch(argc, argv);
            effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
            testForError(effect, "Unable to create the pitch effect");

            res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR, pitch);
            testForState(res, "aaxEffectSetParam");

            res = aaxEmitterSetEffect(emitter, effect);
            testForState(res, "aaxEmitterSetPitch");
            aaxEffectDestroy(effect);

            res = aaxEmitterAddBuffer(emitter, buffer);
            testForState(res, "aaxEmitterAddBuffer");

            /** mixer */
            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            res = aaxMixerRegisterEmitter(config, emitter);
            testForState(res, "aaxMixerRegisterEmitter");

            res = aaxMixerSetState(config, AAX_PLAYING);
            testForState(res, "aaxMixerStart");

            /* equalizer */
            printf("\n|  44Hz | 100Hz | 220Hz | 500Hz |"
                   " 1.2kHz| 2.7kHz| 6.3kHz| 15kHz |\n|");
            for (i=0; i<AAX_MAX_BANDS; ++i) {
                printf("% 6.1f |", band[i]);
            }
            printf("\n\n");

            filter = aaxFilterCreate(config, AAX_GRAPHIC_EQUALIZER);
            testForError(filter, "aaxFilterCreate");

            res = aaxFilterSetSlot(filter, 0, AAX_LINEAR,
                                   band[0], band[1], band[2], band[3]);
            testForState(res, "aaxFilterSetSlot/0");

            res = aaxFilterSetSlot(filter, 1, AAX_LINEAR,
                                   band[4], band[5], band[6], band[7]);
            testForState(res, "aaxFilterSetSlot/1");

            res = aaxFilterSetState(filter, AAX_TRUE);
            testForState(res, "aaxFilterSetState");

            res = aaxMixerSetFilter(config, filter);
            testForState(res, "aaxMixerSetFilter");

            res = aaxFilterDestroy(filter);
            testForState(res, "aaxFilterDestroy");

            /** schedule the emitter for playback */
            res = aaxEmitterSetState(emitter, AAX_PLAYING);
            testForState(res, "aaxEmitterStart");

            q = 0;
            do
            {
                msecSleep(5);
                dt += 0.005f;
#if 0
                if (++q > 10)
                {
                    unsigned long offs, offs_bytes;
                    float off_s;
                    q = 0;

                    off_s = aaxEmitterGetOffsetSec(emitter);
                    offs = aaxEmitterGetOffset(emitter, AAX_SAMPLES);
                    offs_bytes = aaxEmitterGetOffset(emitter, AAX_BYTES);
                    printf("playing time: %5.2f, buffer position: %5.2f "
                           "(%li samples/ %li bytes)\n", dt, off_s,
                           offs, offs_bytes);
                }
#endif
                state = aaxEmitterGetState(emitter);

                printf("|");
                for (i=0; i<AAX_MAX_BANDS; ++i)
                {
                   int b = (i+1)<<8;
                   int a = aaxMixerGetSetup(config, AAX_AVERAGE_VALUE+b);
                   float g = AAX_TO_FLOAT(a);
                   printf("% 6.1f |", _lin2db(g));
               }
               printf("\r");
            }
            while (state == AAX_PLAYING);
            printf("\n");

            filter = aaxMixerGetFilter(config, AAX_GRAPHIC_EQUALIZER);
            aaxFilterSetState(filter, AAX_FALSE);
            aaxMixerSetFilter(config, filter);
            aaxFilterDestroy(filter);

            res = aaxMixerDeregisterEmitter(config, emitter);
            res = aaxMixerSetState(config, AAX_STOPPED);
            res = aaxEmitterDestroy(emitter);
            res = aaxBufferDestroy(buffer);
        }
    }

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    return 0;
}
