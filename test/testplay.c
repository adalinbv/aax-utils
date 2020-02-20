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


#define SLEEP_TIME			50e-3f
#define SLIDE_TIME			7.0f
#define FILE_PATH			SRC_PATH"/tictac.wav"

int main(int argc, char **argv)
{
    char *devname, *infile;
    aaxConfig config;
    char verbose = 0;
    char fm = 0;
    int res;

    if (getCommandLineOption(argc, argv, "-v") ||
        getCommandLineOption(argc, argv, "--verbose"))
    {
        verbose = 1;
    }

    if (getCommandLineOption(argc, argv, "--fm")) {
        fm = 1;
    }

    devname = getDeviceName(argc, argv);
    infile = getInputFile(argc, argv, FILE_PATH);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config)
    {
        aaxBuffer buffer;
        char *ofile;

        res = aaxMixerSetSetup(config, AAX_REFRESH_RATE, 90.0f);
        testForState(res, "aaxMixerSetSetup");

         if (fm) {
            res = aaxMixerSetSetup(config, AAX_CAPABILITIES, AAX_RENDER_SYNTHESIZER);
         }

        buffer = bufferFromFile(config, infile);
        testForError(buffer, "Unable to create a buffer");

        if (verbose)
        {
            printf("Sample frequency: %i Hz\n", aaxBufferGetSetup(buffer, AAX_FREQUENCY));
            printf("Base frequency  : %i Hz\n", aaxBufferGetSetup(buffer, AAX_UPDATE_RATE));
        }

        ofile = getOutputFile(argc, argv, NULL);
        if (!ofile && buffer)
        {
            float gain, gain2, gain_step, gain_time = SLIDE_TIME;
            float pitch, pitch2, pitch_time = SLIDE_TIME;
            int i, q, state, nsrc;
            aaxEmitter emitter[64];
            aaxFrame frame[4];
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

            pitch = getPitch(argc, argv);
            pitch2 = getPitchRange(argc, argv);


            /** emitter */
            for (q=0; q<nsrc; q++)
            {
                emitter[q] = aaxEmitterCreate();
                testForError(emitter[q], "Unable to create a new emitter");

                res = aaxEmitterSetMode(emitter[q], AAX_POSITION, AAX_ABSOLUTE);
                testForError(emitter[q], "Unable to set emitter mode");

                /* gain */
                filter = aaxFilterCreate(config, AAX_VOLUME_FILTER);
                testForError(filter, "Unable to create the volume filter");

                res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, gain);
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

            /** audio-frame */
            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            for (q=0; q<4; ++q)
            {
                frame[q] = aaxAudioFrameCreate(config);
                testForError(frame[q], "Unable to create a new audio frame");

                res = aaxMixerRegisterAudioFrame(config, frame[q]);
                testForState(res, "aaxMixerRegisterAudioFrame");

                res = aaxAudioFrameSetState(frame[q], AAX_PLAYING);
                testForState(res, "aaxAudioFrameStart");

                res = aaxAudioFrameAddBuffer(frame[q], buffer);
                // testForState(res, "aaxAudioFrameAddBuffer");
            }

            for (q=0; q<nsrc; q++)
            {
                res = aaxAudioFrameRegisterEmitter(frame[q % 4], emitter[q]);
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
            gain_step = (gain2-gain)/(gain_time/SLEEP_TIME);
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
                if (gain_step >0) {
                    gain = _MIN(gain+gain_step, gain2);
                } else {
                    gain = _MAX(gain+gain_step, gain2);
                }
                for (i=0; i<nsrc; i++)
                {
                    filter = aaxFilterCreate(config, AAX_VOLUME_FILTER);
                    testForError(filter, "Unable to create the volume filter");

                    res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, gain);
                    testForState(res, "aaxFilterSetParam");

                    res = aaxEmitterSetFilter(emitter[i], filter);
                    testForState(res, "aaxEmitterSetGain");
                    aaxFilterDestroy(filter);
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
                msecSleep(50);
                state = aaxEmitterGetState(emitter[0]);
            }
            while (state != AAX_PROCESSED);

            for (q=0; q<4; ++q) {
                res = aaxAudioFrameSetState(frame[q], AAX_STOPPED);
            }
            for (q=0; q<nsrc; ++q) {
                res = aaxAudioFrameDeregisterEmitter(frame[q % 4], emitter[q]);
            }
            for (q=0; q<4; ++q) {
                res = aaxMixerDeregisterAudioFrame(config, frame[q]);
            }
            res = aaxMixerSetState(config, AAX_STOPPED);
            for (q=0; q<4; ++q) {
                res = aaxAudioFrameDestroy(frame[q]);
            }
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
