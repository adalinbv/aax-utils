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
#include <stdlib.h>	// atof

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"


#define SLEEP_TIME			10e-3f
#define SLIDE_TIME			7.0f
#define FILE_PATH			SRC_PATH"/tictac.wav"

int main(int argc, char **argv)
{
    char *devname, *infile;
    aaxConfig config;
    float velocity;
    float gain;
    char verbose = 0;
    char repeat = 0;
    char fm = 0;
    char *env;
    int res;

    gain = getGain(argc, argv);

    if (getCommandLineOption(argc, argv, "-v") ||
        getCommandLineOption(argc, argv, "--verbose"))
    {
        verbose = 1;
    }

    if (getCommandLineOption(argc, argv, "--repeat")) {
        repeat = 5;
    }

    if (getCommandLineOption(argc, argv, "--fm")) {
        fm = 1;
    }

    env = getCommandLineOption(argc, argv, "--velocity");
    if (env) {
        velocity = atof(env);
    } else {
        velocity = 1.0f/1.27f;
    }

    devname = getDeviceName(argc, argv);
    infile = getInputFile(argc, argv, FILE_PATH);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config)
    {
        aaxBuffer buffer;
        aaxFilter filter;
        char *ofile;


        filter = aaxFilterCreate(config, AAX_VOLUME_FILTER);
        testForError(filter, "Unable to create the volume filter");

        res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, gain);
        testForState(res, "aaxFilterSetParam");

        res = aaxMixerSetFilter(config, filter);
        testForState(res, "aaxMixerSetGain");
        aaxFilterDestroy(filter);

        res = aaxMixerSetSetup(config, AAX_REFRESH_RATE, 90.0f);
        testForState(res, "aaxMixerSetSetup");

         if (fm) {
            res = aaxMixerSetSetup(config, AAX_CAPABILITIES, AAX_RENDER_SYNTHESIZER);
         }

        buffer = bufferFromFile(config, infile);
        testForError(buffer, "Unable to create a buffer");

        if (verbose)
        {
            printf("Sample frequency: %li Hz\n", aaxBufferGetSetup(buffer, AAX_FREQUENCY));
            printf("Base frequency  : %li Hz\n", aaxBufferGetSetup(buffer, AAX_UPDATE_RATE));
        }

        ofile = getOutputFile(argc, argv, NULL);
        if (!ofile && buffer)
        {
            float gain, gain2, gain_step, gain_time = SLIDE_TIME;
            float pitch, pitch2, pitch_time = SLIDE_TIME;
            int i, q, state, nsrc;
            aaxEmitter emitter[64];
            aaxFilter filter;
            aaxEffect effect;
            aaxBuffer xbuffer;
            float duration;
            float dt = 0.0f;

            xbuffer = setFiltersEffects(argc, argv, config, config, NULL, NULL, NULL);
            nsrc = _MINMAX(getNumEmitters(argc, argv), 1, 64);
            duration = getDuration(argc, argv);

            gain = getGain(argc, argv);
            gain2 = getGainRange(argc, argv);

            pitch = getFrequency(argc, argv);
            if (pitch)
            {
               float f = aaxBufferGetSetup(buffer, AAX_UPDATE_RATE);
               if (f) pitch /= f;
            }
            if (pitch == 0.0f) {
                pitch = getPitch(argc, argv);
            }
            pitch2 = getPitchRange(argc, argv);


            /** emitter */
            for (q=0; q<nsrc; q++)
            {
                emitter[q] = aaxEmitterCreate();
                testForError(emitter[q], "Unable to create a new emitter");

                res = aaxEmitterSetMode(emitter[q], AAX_POSITION, AAX_ABSOLUTE);
                testForError(emitter[q], "Unable to set emitter mode");

                if (velocity)
                {
                   res = aaxEmitterSetSetup(emitter[q],
                                          AAX_MIDI_ATTACK_VELOCITY_FACTOR,
                                          127.0f*_MINMAX(velocity, 0.0f, 1.0f));
                   testForError(emitter[q], "Unable to set velocity");
                }

                /* gain */
                filter = aaxEmitterGetFilter(emitter[q], AAX_VOLUME_FILTER);
                testForError(filter, "Unable to create the volume filter");

                res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, gain);
                testForState(res, "aaxFilterSetParam");

                res = aaxFilterSetParam(filter, AAX_MAX_GAIN, AAX_LINEAR, 16.f);
                testForState(res, "aaxFilterSetParam");

                res = aaxEmitterSetFilter(emitter[q], filter);
                testForState(res, "aaxEmitterSetGain");
                aaxFilterDestroy(filter);

                /* pitch */
                effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
                testForError(effect, "Unable to create the pitch effect");

                if (pitch2 == 0.0f) {
                   res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR, pitch);
                   testForState(res, "aaxEffectSetParam");
                } else {
                   res = aaxEffectSetSlot(effect, 0, AAX_LINEAR, pitch2, pitch2, pitch, pitch_time);
                   testForState(res, "aaxEffectSetParam");

                   res = aaxEffectSetState(effect, AAX_INVERSE|AAX_ENVELOPE_FOLLOW);
                   testForState(res, "aaxEffectSetState");
                }

                res = aaxEmitterSetEffect(emitter[q], effect);
                testForState(res, "aaxEmitterSetPitch");
                aaxEffectDestroy(effect);

                /* buffer */
                res = aaxEmitterAddBuffer(emitter[q], buffer);
                testForState(res, "aaxEmitterAddBuffer");

//              res = aaxEmitterSetMode(emitter[q], AAX_LOOPING, AAX_TRUE);
                testForState(res, "aaxEmitterSetMode");
            }

            for (q=0; q<nsrc; q++)
            {
                res = aaxMixerRegisterEmitter(config, emitter[q]);
                testForState(res, "aaxAudioFrameRegisterEmitter");
            }

            /** mixer */
            res = aaxMixerAddBuffer(config, buffer);
            // testForState(res, "aaxMixerAddBuffer");

            if (xbuffer) {
                res = aaxMixerAddBuffer(config, xbuffer);
            }

            res = aaxMixerSetState(config, AAX_PLAYING);
            testForState(res, "aaxMixerStart");

            for (q=0; q<nsrc; q++)
            {
                /** schedule the emitter for playback */
                res = aaxEmitterSetState(emitter[q], AAX_PLAYING);
                testForState(res, "aaxEmitterStart");
            }

            printf("Playing sound for %3.1f seconds or until a key is pressed\n", duration);
            q = 0;
            set_mode(1);
            if (gain2 == 1.0f) {
                gain_step = 0.0f;
            } else { 
                gain_step = (gain2-gain)/(gain_time/SLEEP_TIME);
            }
            do
            {
                msecSleep(SLEEP_TIME*1000);
                dt += SLEEP_TIME;

                if (++q > 10)
                {
                    unsigned long offs, offs_bytes;
                    float off_s;
                    q = 0;

                    off_s = aaxEmitterGetOffsetSec(emitter[0]);
                    offs = aaxEmitterGetOffset(emitter[0], AAX_SAMPLES);
                    offs_bytes = aaxEmitterGetOffset(emitter[0], AAX_BYTES);
                    printf("playing time: %5.2f, buffer position: %5.2f "
                           "(%li samples/ %li bytes)\n", dt, off_s,
                           offs, offs_bytes);
                }
                state = aaxEmitterGetState(emitter[0]);

                if (get_key()) break;

                /* gain */
                if (gain_step > 0.0f) {
                    gain = _MIN(gain+gain_step, gain2);
                } else if (gain_step < 0.0f) {
                    gain = _MAX(gain+gain_step, gain2);
                }
                for (i=0; i<nsrc; i++)
                {
                    filter = aaxEmitterGetFilter(emitter[i], AAX_VOLUME_FILTER);
                    testForError(filter, "Unable to create the volume filter");

                    res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, gain);
                    testForState(res, "aaxFilterSetParam");

                    res = aaxEmitterSetFilter(emitter[i], filter);
                    testForState(res, "aaxEmitterSetGain");
                    aaxFilterDestroy(filter);

                   if (repeat && dt > 0.1f) {
                      aaxEmitterSetState(emitter[i], AAX_INITIALIZED);
                      aaxEmitterSetState(emitter[i], AAX_PLAYING);
                   }
                }

                if (repeat && dt > 0.1f)
                {
                   dt = 0.0f;
                   repeat--;
                }
            }
            while ((dt < duration) && (state == AAX_PLAYING));
            set_mode(0);

            for (q=0; q<nsrc; ++q) {
                res = aaxEmitterSetState(emitter[q], AAX_STOPPED);
            }
            testForState(res, "aaxEmitterStop");

            do
            {
                unsigned long offs, offs_bytes;
                float off_s;

                off_s = aaxEmitterGetOffsetSec(emitter[0]);
                offs = aaxEmitterGetOffset(emitter[0], AAX_SAMPLES);
                offs_bytes = aaxEmitterGetOffset(emitter[0], AAX_BYTES);
                printf("playing time: %5.2f, buffer position: %5.2f "
                       "(%li samples/ %li bytes)\n", dt, off_s,
                       offs, offs_bytes);

                msecSleep(50);
                state = aaxEmitterGetState(emitter[0]);
                dt += 0.05f;
            }
            while (state != AAX_PROCESSED);

            for (q=0; q<nsrc; ++q) {
                res = aaxMixerDeregisterEmitter(config, emitter[q]);
            }
            res = aaxMixerSetState(config, AAX_STOPPED);
            for (q=0; q<nsrc; ++q) {
                res = aaxEmitterDestroy(emitter[q]);
            }
            res = aaxBufferDestroy(buffer);
            if (xbuffer) {
                res = aaxBufferDestroy(xbuffer);
            }
        }
        else if (buffer)
        {
           aaxBufferSetSetup(buffer, AAX_FORMAT, AAX_PCM16S);
           aaxBufferWriteToFile(buffer, ofile, AAX_OVERWRITE);
        }
    }

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);


    return 0;
}
