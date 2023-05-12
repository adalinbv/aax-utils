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

#include <aax/aeonwave.hpp>

#include <base/geometry.h>
#include <base/types.h>
#include "driver.h"
#include "wavfile.h"

#define SAMPLE_FREQUENCY                22050

static const char* aaxs_data =
  "<aeonwave>"
  " <sound frequency=\"440\">"
  "  <waveform src=\"sine\"/>"
  " </sound>"
  " <emitter looping=\"true\"/>"
  "</aeonwave>";

int main(int argc, char **argv)
{
    int state, res;
    int rv = 0;

    char *devname = getDeviceName(argc, argv);
    aax::AeonWave config(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (!aax::is_valid(config, AAX_CONFIG_HD))
    {
        printf("Warning:\n");
        printf("  %s requires a registered version of AeonWave\n", argv[0]);
        printf("  Please visit http://www.adalin.com/buy_aeonwaveHD.html to ");
        printf("obtain\n  a product-key.\n\n");
        rv = -1;
    }

    if (config && (rv >= 0))
    {
        unsigned int no_samples;
        aax::Emitter emitter;
        aax::Buffer buffer;
        aax::Frame frame;
        aax::dsp dsp;
        int i;

        no_samples = (unsigned int)(1.0f*SAMPLE_FREQUENCY);
        buffer = aax::Buffer(config, no_samples, 1, AAX_AAXS16S);
        testForError(buffer, "Unable to generate buffer\n");

        res = buffer.set(AAX_FREQUENCY, SAMPLE_FREQUENCY);
        testForState(res, "aaxBufferSetFrequency");

        res = buffer.fill(aaxs_data);
        testForState(res, "aaxBufferSetData");

        /** mixer */
        res = config.set(AAX_INITIALIZED);
        testForState(res, "aaxMixerInit");

        res = config.set(AAX_PLAYING);
        testForState(res, "aaxMixerStart");


        /** frame */
        frame = aax::Frame(config);
        testForError(frame, "Unable to create a new audio-frame");

        res = frame.add(buffer);

        res = frame.set(AAX_PLAYING);
        testForState(res, "aaxAudioFrameStart");

        res = config.add(frame);
        testForState(res, "aaxMixerRegisterAudioFrame");

        /** emitter */
        emitter = aax::Emitter(AAX_STEREO);
        testForError(emitter, "Unable to create a new emitter\n");

        res = emitter.add(buffer);
        testForState(res, "aaxEmitterAddBuffer");

        res = frame.add(emitter);
        testForState(res, "aaxAudioFrameRegisterEmitter");

        /** schedule the emitter for playback */
        res = emitter.set(AAX_PLAYING);
        testForState(res, "aaxEmitterStart");

        i = 0;
        aax::Param pitch = 1.0f;
        emitter.tie(pitch, AAX_PITCH_EFFECT, AAX_PITCH);

        aax::Param depth = 0.0f;
        frame.tie(depth, AAX_REVERB_EFFECT, AAX_DECAY_DEPTH);

        do
        {
            msecSleep(50);
            if (i == 100) break;

            if (i == 10) {
                pitch = 0.8909f;			// G2
            }
            else if (i == 27) {
                pitch = 0.2973f;			// C1
            }
            i++;

            state = emitter.state();
        }
        while (state == AAX_PLAYING);
        emitter.untie(pitch);

        res = emitter.set(AAX_STOPPED);
        do
        {
            msecSleep(50);
            state = emitter.state();
        }
        while (state == AAX_PLAYING);


        res = emitter.set(AAX_STOPPED);
        res = emitter.remove_buffer();
        testForState(res, "aaxEmitterRemoveBuffer");

        res = frame.remove(emitter);
        res = frame.set(AAX_STOPPED);

        res = config.remove(frame);
        res = config.set(AAX_STOPPED);
    }

    return rv;
}
