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
#include <math.h>

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"

#define FILE_PATH		SRC_PATH"/loop.wav"
#define LOOP_START_SEC		0.5750f
#define LOOP_END_SEC		1.977475f

int main(int argc, char **argv)
{
    char *devname, *infile;
    aaxConfig config;
    int res;

    devname = getDeviceName(argc, argv);
    infile = getInputFile(argc, argv, FILE_PATH);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config)
    {
        aaxBuffer buffer = bufferFromFile(config, infile);
        if (buffer)
        {
            aaxEmitter emitter;
            int freq, loop_start, loop_end;
            int q, state, num;
            char stopped = 0;
            float dt = 0.0f;

            freq = aaxBufferGetSetup(buffer, AAX_SAMPLE_RATE);
            loop_start = rintf(freq*LOOP_START_SEC);
            loop_end = rintf(freq*LOOP_END_SEC);

            res = aaxBufferSetSetup(buffer, AAX_SAMPLED_RELEASE, AAX_TRUE);
            res = aaxBufferSetSetup(buffer, AAX_LOOP_START, loop_start);
            res = aaxBufferSetSetup(buffer, AAX_LOOP_END, loop_end);
            testForState(res, "aaxBufferSetLoopPoints");

            /** emitter */
            emitter = aaxEmitterCreate();
            testForError(emitter, "Unable to create a new emitter\n");

            res = aaxEmitterAddBuffer(emitter, buffer);
            testForState(res, "aaxEmitterAddBuffer");

            num = getNumEmitters(argc, argv);
            res = aaxEmitterSetMode(emitter, AAX_LOOPING, num);
            testForState(res, "aaxEmitterSetLooping");

            /** mixer */
            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            res = aaxMixerRegisterEmitter(config, emitter);
            testForState(res, "aaxMixerRegisterEmitter");

            res = aaxMixerSetState(config, AAX_PLAYING);
            testForState(res, "aaxMixerStart");

            /** schedule the emitter for playback */
            res = aaxEmitterSetState(emitter, AAX_PLAYING);
            testForState(res, "aaxEmitterStart");

            q = 0;
            set_mode(1);
            do
            {
                msecSleep(50);
                dt += 0.05f;

                if (!stopped && dt > (LOOP_START_SEC+3*LOOP_END_SEC))
                {
                    res = aaxEmitterSetState(emitter, AAX_STOPPED);
                    testForState(res, "aaxEmitterStop");
                    stopped = AAX_TRUE;
                }

                if (++q > 10)
                {
                    unsigned long offs;
                    float off_s;
                    q = 0;

                    off_s = aaxEmitterGetOffsetSec(emitter);
                    offs = aaxEmitterGetOffset(emitter, AAX_SAMPLES);
                    printf("playing time: %5.2f, buffer position: %5.2f "
                           "(%li samples)\n", dt, off_s, offs);
                }
                state = aaxEmitterGetState(emitter);
            }
            while (!get_key() && state != AAX_PROCESSED);
            set_mode(0);

            res = aaxMixerDeregisterEmitter(config, emitter);
            res = aaxEmitterDestroy(emitter);
            res = aaxBufferDestroy(buffer);
        }
    }

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    return 0;
}
