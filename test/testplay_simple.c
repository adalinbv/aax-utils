/*
 * Copyright (C) 2008-2022 by Erik Hofman.
 * Copyright (C) 2009-2022 by Adalin B.V.
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
#include <ctype.h>
#include <math.h>
#include <stdlib.h>	// atof, getenv

#include <aax/aax.h>

#include "3rdparty/pffft.h"
#include "base/types.h"
#include "driver.h"
#include "wavfile.h"


#define MAX_STAGES			4
#define SLEEP_TIME			10e-3f
#define SLIDE_TIME			7.0f
#define	BLOCK_SIZE			4096
#define FILE_PATH			SRC_PATH"/tictac.wav"

aaxVec3d EmitterPos = { 0.0,  0.0,  0.0  };
aaxVec3f EmitterDir = { 0.0f, 0.0f, 1.0f };

aaxVec3d SensorPos = { 0.0,  0.0,  0.0  };
aaxVec3f SensorAt = {  0.0f, 0.0f, 1.0f };
aaxVec3f SensorUp = {  0.0f, 1.0f, 0.0f };

void help()
{
    printf("Usage: testplay_simple [options]\n");
    printf("Plays audio from a file.\n");
    printf("Optionally writes the audio to an output file.\n");

    printf("\nOptions:\n");
    printf("  -d, --device <device>\t\tplayback device (default if not specified)\n");
    printf("  -i, --input <file>\t\tplayback audio from a file\n");
    printf("  -h, --help\t\t\tprint this message and exit\n");

    printf("\n");

    exit(-1);
}

int main(int argc, char **argv)
{
    char *devname, *infile;
    aaxConfig config;
    int res;

    if (argc == 1 || getCommandLineOption(argc, argv, "-h") ||
                     getCommandLineOption(argc, argv, "--help"))
    {
        help();
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

        buffer = aaxBufferReadFromStream(config, infile);
        testForError(buffer, "Unable to create a buffer");

        ofile = getOutputFile(argc, argv, NULL);
        if (!ofile && buffer)
        {
            aaxEmitter emitter;
            int i, state, key;
            int paused = AAX_FALSE;
            aaxFrame frame;
            aaxMtx4d mtx64;
            float duration;
            float dt = 0.0f;

            duration = getDuration(argc, argv);

            /** emitter */
            emitter = aaxEmitterCreate();
            testForError(emitter, "Unable to create a new emitter");

            res = aaxMatrix64SetDirection(mtx64, EmitterPos, EmitterDir);
            testForState(res, "aaxMatrix64SetDirection");

            res = aaxEmitterSetMatrix64(emitter, mtx64);
            testForState(res, "aaxEmitterSetMatrix64");

            res = aaxEmitterSetMode(emitter, AAX_POSITION, AAX_ABSOLUTE);
            testForState(res, "Unable to set emitter mode");

            /* buffer */
            res = aaxEmitterAddBuffer(emitter, buffer);
            testForState(res, "aaxEmitterAddBuffer");

            /** audio-frame */
            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            frame = aaxAudioFrameCreate(config);
            testForError(frame, "Unable to create a new audio frame");

            res = aaxMixerRegisterAudioFrame(config, frame);
            testForState(res, "aaxMixerRegisterAudioFrame");

            res = aaxAudioFrameSetState(frame, AAX_PLAYING);
            testForState(res, "aaxAudioFrameStart");

            /* audio-frame buffer */
            res = aaxAudioFrameAddBuffer(frame, buffer);
            // testForState(res, "aaxAudioFrameAddBuffer");

            res = aaxAudioFrameRegisterEmitter(frame, emitter);
            testForState(res, "aaxAudioFrameRegisterEmitter");

            /** sensor settings */
            res = aaxMatrix64SetOrientation(mtx64, SensorPos,
                                               SensorAt, SensorUp);
            testForState(res, "aaxMatrix64SetOrientation");

            res = aaxMatrix64Inverse(mtx64);
            testForState(res, "aaaxMatrix64Inverse");
            res = aaxSensorSetMatrix64(config, mtx64);
            testForState(res, "aaxSensorSetMatrix64");

            /** mixer */
            res = aaxMixerAddBuffer(config, buffer);
            // testForState(res, "aaxMixerAddBuffer");

            res = aaxMixerSetState(config, AAX_PLAYING);
            testForState(res, "aaxMixerStart");

            /** schedule the emitter for playback */
            res = aaxEmitterSetState(emitter, AAX_INITIALIZED);
            testForState(res, "aaxEmitterinit");
            res = aaxEmitterSetState(emitter, AAX_PLAYING);
            testForState(res, "aaxEmitterStart");

            dt = (float)aaxBufferGetSetup(buffer, AAX_NO_SAMPLES);
            dt /= (float)aaxBufferGetSetup(buffer, AAX_FREQUENCY);
            if (duration > dt && !aaxBufferGetSetup(buffer, AAX_LOOP_COUNT)) {
               duration = dt;
            }

            printf("Playing sound for %3.1f seconds of %3.1f seconds, "
                   "or until a key is pressed\n", duration, dt);
            i = 0;
            dt = 0.0f;
            set_mode(1);
            do
            {
                msecSleep(SLEEP_TIME*1000);
                dt += SLEEP_TIME;

                if (!paused && ++i > 10)
                {
                    unsigned long offs, offs_bytes;
                    float off_s;
                    i = 0;

                    off_s = aaxEmitterGetOffsetSec(emitter);
                    offs = aaxEmitterGetOffset(emitter, AAX_SAMPLES);
                    offs_bytes = aaxEmitterGetOffset(emitter, AAX_BYTES);
                    printf("playing time: %5.2f, buffer position: %5.2f "
                           "(%li samples/ %li bytes)\n", dt, off_s,
                           offs, offs_bytes);
                }

                key = get_key();
                if (key)
                {
                    if (key == SPACE_KEY)
                    {
                        if (paused)
                        {
                            aaxMixerSetState(config, AAX_PLAYING);
                            printf("Restart playback.\n");
                            paused = AAX_FALSE;
                        }
                        else
                        {
                            aaxMixerSetState(config, AAX_SUSPENDED);
                            printf("Pause playback.\n");
                            paused = AAX_TRUE;
                        }
                    }
                    else if (key == TAB_KEY)
                    {
                        aaxEmitterSetState(emitter, AAX_INITIALIZED);
                        aaxEmitterSetState(emitter, AAX_PLAYING);
                        printf("Switching to the regular file.\n");
                        dt = 0.0f;
                    }
                    else if (key == ESCAPE_KEY) {
                        break;
                    }
                    else
                    {
                        aaxEmitterSetState(emitter, AAX_STOPPED);
                    }
                }

                state = aaxEmitterGetState(emitter);
            }
            while ((dt < duration) && (state != AAX_PROCESSED));
            set_mode(0);

            res = aaxEmitterSetState(emitter, AAX_STOPPED);
            testForState(res, "aaxEmitterStop");

            res = aaxEmitterSetState(emitter, AAX_PROCESSED);
            testForState(res, "aaxEmitterSetState");

            res = aaxAudioFrameDeregisterEmitter(frame, emitter);
            testForState(res, "aaxAudioFrameDeregisterEmitter");

            res = aaxEmitterDestroy(emitter);
            testForState(res, "aaxEmitterDestroy");

            res = aaxAudioFrameSetState(frame, AAX_STOPPED);
            testForState(res, "aaxAudioFrameSetState");

            res = aaxMixerDeregisterAudioFrame(config, frame);
            testForState(res, "aaxAudioFrameDeregisterAudioFrame");

            res = aaxAudioFrameDestroy(frame);
            testForState(res, "aaxAudioFrameDestroy");

            res = aaxMixerSetState(config, AAX_STOPPED);

            res = aaxBufferDestroy(buffer);
            testForState(res, "aaxBufferDestroy");
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
