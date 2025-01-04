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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"

#define ENABLE_MIXER_PHASING		1
#define ENABLE_MIXER_CHORUS		1
#define ENABLE_MIXER_FLANGING		1


#define FILE_PATH			SRC_PATH"/tictac.wav"
#define _DELAY				120
#define DELAY                           \
    deg = 0; set_mode(1);		\
    while(deg < _DELAY) { msecSleep(50); deg++; \
        if(get_key()) { set_mode(0); exit(0); } \
    }; set_mode(0);

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
        float pitch;

        if (buffer)
        {
            aaxEmitter emitter;
            aaxEffect effect;
            aaxFilter filter;
            aaxFrame frame;
            int q, deg = 0;

            /** mixer */
            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            res = aaxMixerSetState(config, AAX_PLAYING);
            testForState(res, "aaxMixerStart");

            /** AudioFrame */
            frame = aaxAudioFrameCreate(config);
            testForError(config, "aaxAudioFrameCreate");

            res = aaxMixerRegisterAudioFrame(config, frame);
            testForState(res, "aaxMixerRegisterAudioFrame");

            res = aaxAudioFrameSetState(frame, AAX_PLAYING);
            testForState(res, "aaxAudioFrameStart");

            /** emitter */
            emitter = aaxEmitterCreate();
            testForError(emitter, "Unable to create a new emitter\n");

            res = aaxEmitterSetMode(emitter, AAX_POSITION, AAX_RELATIVE);
            testForState(res, "aaxEmitterSetMode");

            res = aaxEmitterSetMode(emitter, AAX_LOOPING, AAX_TRUE);
            testForState(res, "aaxEmitterSetLooping");

            /* pitch */
            pitch = getPitch(argc, argv);
            effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
            testForError(effect, "Unable to create the pitch effect");

            res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR, pitch);
            testForState(res, "aaxEffectSetParam");

            res = aaxEmitterSetEffect(emitter, effect);
            testForState(res, "aaxEmitterSetPitch");
            aaxEffectDestroy(effect);

            /* gain*/
            filter = aaxFilterCreate(config, AAX_VOLUME_FILTER);
            testForError(filter, "Unable to create the volume filter");

            res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, 0.7f);
            testForState(res, "aaxFilterSetParam");

            res = aaxEmitterSetFilter(emitter, filter);
            testForState(res, "aaxEmitterSetGain");
            aaxFilterDestroy(filter);

            res = aaxAudioFrameRegisterEmitter(frame, emitter);
            testForState(res, "aaxMixerRegisterEmitter");

            res = aaxEmitterAddBuffer(emitter, buffer);
            testForState(res, "aaxEmitterAddBuffer");

            /** schedule the emitter for playback */
            res = aaxEmitterSetState(emitter, AAX_PLAYING);
            testForState(res, "aaxEmitterStart");

# if ENABLE_MIXER_PHASING
            /* phasing effect */
            printf("mixer phasing.. (square wave)\n");
            effect = aaxMixerGetEffect(config, AAX_PHASING_EFFECT);
            res = aaxEffectSetSlot(effect, 0, AAX_LINEAR,
                                              1.0f, 0.5f, 1.0f, 0.0f);
            res = aaxEffectSetState(effect, AAX_SQUARE);
            testForError(effect, "aaxEffectCreate");
            res = aaxMixerSetEffect(config, effect);
            res = aaxEffectDestroy(effect);
            testForState(res, "aaxMixerSetEffect");

            DELAY;
# endif

# if ENABLE_MIXER_CHORUS
            /* chorus effect */
            printf("mixer chorus.. (sawtooth wave)\n");
            effect = aaxMixerGetEffect(config, AAX_CHORUS_EFFECT);
            res = aaxEffectSetSlot(effect, 0, AAX_LINEAR,
                                              1.0f, 2.5f, 1.0f, 0.0f);
            res = aaxEffectSetState(effect, AAX_SAWTOOTH);
            testForError(effect, "aaxEffectCreate");
            res = aaxMixerSetEffect(config, effect);
            res = aaxEffectDestroy(effect);
            testForState(res, "aaxMixerSetEffect");

            DELAY;
# endif

# if ENABLE_MIXER_FLANGING
            /* flanging effect */
            printf("mixer flanging.. (sawtooth wave)\n");
            effect = aaxMixerGetEffect(config, AAX_FLANGING_EFFECT);
            res = aaxEffectSetSlot(effect, 0, AAX_LINEAR,
                                              0.88f, 0.08f, 1.0f, 0.0f);
            res = aaxEffectSetState(effect, AAX_SAWTOOTH);
            testForError(effect, "aaxEffectCreate");
            res = aaxMixerSetEffect(config, effect);
            res = aaxEffectDestroy(effect);
            testForState(res, "aaxMixerSetEffect");

            DELAY;
# endif

            res = aaxEmitterSetState(emitter, AAX_PROCESSED);
            testForState(res, "aaxEmitterStop");

            res = aaxAudioFrameDeregisterEmitter(frame, emitter);
            testForState(res, "aaxAudioFrameDeregisterEmitter");

            res = aaxEmitterDestroy(emitter);
            testForState(res, "aaxEmitterDestroy");

            res = aaxBufferDestroy(buffer);
            testForState(res, "aaxBufferDestroy");

            res = aaxAudioFrameSetState(frame, AAX_STOPPED);
            testForState(res, "aaxAudioFrameStop");

            res = aaxMixerDeregisterAudioFrame(config, frame);
            testForState(res, "aaxMixerDeregisterAudioFrame");

            res = aaxAudioFrameDestroy(frame);
            testForState(res, "aaxAudioFrameDestroy");

            res = aaxMixerSetState(config, AAX_STOPPED);
            testForState(res, "aaxMixerStop");
        }

    }

    res = aaxDriverClose(config);
    testForState(res, "aaxDriverClose");

    res = aaxDriverDestroy(config);
    testForState(res, "aaxDriverDestroy");

    return 0;
}
