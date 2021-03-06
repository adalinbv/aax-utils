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


#define FILE1_PATH			SRC_PATH"/midi/telephone.aaxs"
#define FILE2_PATH			SRC_PATH"/alarm.aaxs"
#define FILE3_PATH			SRC_PATH"/whitenoise-filtered.aaxs"

int main(int argc, char **argv)
{
    char *devname, *infile;
    aaxBuffer buffer[3];
    aaxConfig config;
    int res;

    devname = getDeviceName(argc, argv);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    infile = getInputFile(argc, argv, FILE1_PATH);
    buffer[0] = bufferFromFile(config, infile);

    infile = getInputFile(argc, argv, FILE2_PATH);
    buffer[1] = bufferFromFile(config, infile);

    infile = getInputFile(argc, argv, FILE3_PATH);
    buffer[2] = bufferFromFile(config, infile);

    if (config && buffer[0] && buffer[1] && buffer[2])
    {
        aaxEmitter emitter;
        aaxEffect effect;
        float pitch, duration;
        float dt = 0.0f;
        int q, state;

        res = aaxMixerSetSetup(config, AAX_REFRESH_RATE, 90.0f);
        testForState(res, "aaxMixerSetSetup");

        duration = getDuration(argc, argv);

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

        res = aaxEmitterAddBuffer(emitter, buffer[0]);
        testForState(res, "aaxEmitterAddBuffer 0");
        res = aaxEmitterAddBuffer(emitter, buffer[1]);
        testForState(res, "aaxEmitterAddBuffer 1");
        res = aaxEmitterAddBuffer(emitter, buffer[2]);
        testForState(res, "aaxEmitterAddBuffer 2");

        /** mixer */
        res = aaxMixerSetState(config, AAX_INITIALIZED);
        testForState(res, "aaxMixerInit");

        res = aaxMixerAddBuffer(config, buffer[0]);
        testForState(res, "aaxMixerAddBuffer");

        res = aaxMixerRegisterEmitter(config, emitter);
        testForState(res, "aaxMixerRegisterEmitter");

        res = aaxMixerSetState(config, AAX_PLAYING);
        testForState(res, "aaxMixerStart");

        /** schedule the emitter for playback */
        res = aaxEmitterSetState(emitter, AAX_PLAYING);
        testForState(res, "aaxEmitterStart");

        printf("Playing sound for %3.1f seconds or until a key is pressed\n", duration);
        q = 0;
        set_mode(1);
        do
        {
            msecSleep(50);
            dt += 0.05f;

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
            state = aaxEmitterGetState(emitter);

            if (get_key()) break;
        }
        while ((dt < duration) && (state == AAX_PLAYING));
        set_mode(0);

        res = aaxEmitterSetState(emitter, AAX_STOPPED);
        testForState(res, "aaxEmitterStop");

        do
        {
            msecSleep(50);
            state = aaxEmitterGetState(emitter);
        }
        while (state != AAX_PROCESSED);

        res = aaxMixerDeregisterEmitter(config, emitter);
        testForState(res, "aaxMixerDeregisterEmitter");

        res = aaxMixerSetState(config, AAX_STOPPED);
        testForState(res, "aaxMixerStop");

        res = aaxEmitterDestroy(emitter);
        testForState(res, "aaxEmitterDestroy");

        res = aaxBufferDestroy(buffer[2]);
        testForState(res, "aaxBufferDestroy");

        res = aaxBufferDestroy(buffer[1]);
        testForState(res, "aaxBufferDestroy 1");

        res = aaxBufferDestroy(buffer[0]);
        testForState(res, "aaxBufferDestroy 0");

    }

    res = aaxDriverClose(config);
    testForState(res, "aaxDriverClose");

    res = aaxDriverDestroy(config);
    testForState(res, "aaxDriverDestroy");



    return 0;
}
