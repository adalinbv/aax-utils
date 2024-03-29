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

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"

#define FILE_PATH		SRC_PATH"/stereo.wav"

int main(int argc, char **argv)
{
    char *devname, *infile;
    aaxConfig config;
    int num, res, rv = 0;

    num = getNumEmitters(argc, argv);
    if (num>256) num = 256;

    devname = getDeviceName(argc, argv);
    infile = getInputFile(argc, argv, FILE_PATH);

    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config && (rv >= 0))
    {
        aaxBuffer buffer = bufferFromFile(config, infile);
        if (buffer)
        {
            aaxEmitter emitter[256];
            float pitch, dt = 0.0f;
            aaxEffect effect;
            int q, state;

            /** mixer */
            res = aaxMixerSetSetup(config, AAX_REFRESH_RATE, 50);
            testForState(res, "aaxMixerSetSetup");

            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            res = aaxMixerSetState(config, AAX_PLAYING);
            testForState(res, "aaxMixerStart");

            /** emitter */
            pitch = getPitch(argc, argv);
            for (q=0; q<num; q++)
            {
                emitter[q] = aaxEmitterCreate();
                testForError(emitter[q], "Unable to create a new emitter");

                /* pitch */
                effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
                testForError(effect, "Unable to create the pitch effect");

                res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR, pitch);
                testForState(res, "aaxEffectSetParam");

                res = aaxEmitterSetEffect(emitter[q], effect);
                testForState(res, "aaxEmitterSetPitch");
                aaxEffectDestroy(effect);

                res = aaxEmitterAddBuffer(emitter[q], buffer);
                testForState(res, "aaxEmitterAddBuffer");

                res = aaxMixerRegisterEmitter(config, emitter[q]);
                testForState(res, "aaxMixerRegisterEmitter");

                /** schedule the emitter for playback */
                res = aaxEmitterSetState(emitter[q], AAX_PLAYING);
                testForState(res, "aaxEmitterStart");
            }

            q = 0;
            do
            {
                msecSleep(50);
                dt += 0.05f;

                if (++q > 10)
                {
                    unsigned long offs, offs_bytes;
                    float off_s;
                    q = 0;

                    off_s = aaxEmitterGetOffsetSec(emitter[q]);
                    offs = aaxEmitterGetOffset(emitter[q], AAX_SAMPLES);
                    offs_bytes = aaxEmitterGetOffset(emitter[q], AAX_BYTES);
                    printf("playing time: %5.2f, buffer position: %5.2f "
                           "(%li samples/ %li bytes)\n", dt, off_s,
                           offs, offs_bytes);
                }
                state = aaxEmitterGetState(emitter[0]);
            }
            while (state == AAX_PLAYING);

            for (q=0; q<num; q++)
            {
                res = aaxMixerDeregisterEmitter(config, emitter[q]);
                res = aaxEmitterDestroy(emitter[q]);
            }
            res = aaxMixerSetState(config, AAX_STOPPED);
            res = aaxBufferDestroy(buffer);
        }
    }

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    return rv;
}
