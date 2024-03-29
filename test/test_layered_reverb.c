/*
 * Copyright (C) 2008-2020 by Erik Hofman.
 * Copyright (C) 2009-2020 by Adalin B.V.
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

#define FILE_PATH		SRC_PATH"/tictac.wav"
const char *room = "<?xml version='1.0'?> \
<aeonwave> \
 <audioframe> \
  <effect type='reverb'> \
   <slot n='0'> \
    <param n='0'>2200.0</param> \
    <param n='1'>0.027</param> \
    <param n='2'>1.0</param> \
    <param n='3'>0.15</param> \
   </slot> \
  </effect> \
 </audioframe> \
 <mixer> \
  <effect type='reverb'> \
   <slot n='0'> \
    <param n='0'>2200.0</param> \
    <param n='1'>0.027</param> \
    <param n='2'>1.0</param> \
    <param n='3'>0.15</param> \
   </slot> \
  </effect> \
 </mixer> \
</aeonwave>";

int main(int argc, char **argv)
{
    char *devname, *infile;
    aaxBuffer buffer;
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
            aaxBuffer xbuffer;
            aaxEmitter emitter;
            aaxFilter filter;
            aaxEffect effect;
            aaxFrame reverb;
            aaxFrame frame;
            float dt = 0.0f;
            int q, state;
            float pitch;
            float gain;

            /** emitter */
            emitter = aaxEmitterCreate();
            testForError(emitter, "Unable to create a new emitter");

            res = aaxEmitterSetMode(emitter, AAX_LOOPING, AAX_TRUE);
            testForState(res, "aaxEmitterSetMode");

            res = aaxEmitterAddBuffer(emitter, buffer);
            testForState(res, "aaxEmitterAddBuffer");

            /* pitch */
            pitch = getPitch(argc, argv);
            effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
            testForError(effect, "Unable to create the pitch effect");

            res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR, pitch);
            testForState(res, "aaxEffectSetParam");

            res = aaxEmitterSetEffect(emitter, effect);
            testForState(res, "aaxEmitterSetPitch");
            aaxEffectDestroy(effect);

            /* gain */
            gain = getGain(argc, argv);
            filter = aaxFilterCreate(config, AAX_VOLUME_FILTER);
            testForError(filter, "Unable to create the volume filter");

            res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, gain);
            testForState(res, "aaxFilterSetParam");

            res = aaxEmitterSetFilter(emitter, filter);
            testForState(res, "aaxEmitterSetGain");
            aaxFilterDestroy(filter);

            /* audioframe */
            frame = aaxAudioFrameCreate(config);
            testForError(frame, "Unable to create a new audio-frame");

            res = aaxAudioFrameRegisterEmitter(frame, emitter);
            testForState(res, "aaxAudioFrameRegisterEmitter");

            res = aaxAudioFrameSetState(frame, AAX_PLAYING);
            testForState(res, "aaxAudioFrameStart");

            /* reverb audioframe */
            reverb = aaxAudioFrameCreate(config);
            testForError(reverb, "Unable to create a new audio-frame");

            res = aaxAudioFrameRegisterAudioFrame(reverb, frame);
            testForState(res, "aaxAudioFrameRegisterAudioFrame");

            res = aaxAudioFrameSetState(reverb, AAX_PLAYING);
            testForState(res, "aaxAudioFrameStart");

            /** mixer */
            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            res = aaxMixerRegisterAudioFrame(config, reverb);
            testForState(res, "aaxMixerRegisterAudioFrame");

            res = aaxMixerSetState(config, AAX_PLAYING);
            testForState(res, "aaxMixerStart");

            /* reverb */
            xbuffer = setFiltersEffects(argc, argv, config, NULL,
                                        reverb, emitter, room);
            if (xbuffer)
            {
               res = aaxAudioFrameAddBuffer(frame, xbuffer);
               testForState(res, "aaxAudioFrameAddBuffer");

               effect = aaxAudioFrameGetEffect(frame, AAX_REVERB_EFFECT);
               testForError(effect, "aaxAudioFrameGetEffect");

               res = aaxEffectSetState(effect, AAX_EFFECT_1ST_ORDER);
               testForState(res, "aaxEffectSetState");

               res = aaxAudioFrameSetEffect(frame, effect);
               testForState(res, "aaxAudioFrameSetEffect");

               res = aaxEffectDestroy(effect);
               testForState(res, "aaxEffectDestroy");

               effect = aaxAudioFrameGetEffect(reverb, AAX_REVERB_EFFECT);
               testForError(effect, "aaxAudioFrameGetEffect");

               res = aaxEffectSetState(effect, AAX_EFFECT_2ND_ORDER);
               testForState(res, "aaxEffectSetState");

               res = aaxAudioFrameSetEffect(reverb, effect);
               testForState(res, "aaxMixerSetEffect");

               res = aaxEffectDestroy(effect);
               testForState(res, "aaxEffectDestroy");
            }

            /** schedule the emitter for playback */
            res = aaxEmitterSetState(emitter, AAX_PLAYING);
            testForState(res, "aaxEmitterStart");

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
            while (state == AAX_PLAYING);
            set_mode(0);

            if (xbuffer) aaxBufferDestroy(xbuffer);
            res = aaxAudioFrameDeregisterEmitter(frame, emitter);
            res = aaxAudioFrameDeregisterAudioFrame(reverb, frame);
            res = aaxMixerDeregisterAudioFrame(config, reverb);
            res = aaxMixerSetState(config, AAX_STOPPED);
            res = aaxEmitterDestroy(emitter);
            res = aaxBufferDestroy(buffer);
        }
    }

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    return 0;
}
