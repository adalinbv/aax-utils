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
    printf("Usage: testplay [options]\n");
    printf("Plays audio from a file.\n");
    printf("Optionally writes the audio to an output file.\n");

    printf("\nOptions:\n");
    printf("  -d, --device <device>\t\tplayback device (default if not specified)\n");
    printf("  -f, --frequency <freq>\tplayback frequency\n");
    printf("      --fft\t\tdo FFT analysis\n");
    printf("      --fm\t\t\tuse FM playback mode\n");
    printf("  -g, --gain <v>[:<dt>[-<v>..]\tchange the gain setting\n");
    printf("  -i, --input <file>\t\tplayback audio from a file\n");
    printf("  -o, --output <file>\t\talso write to an audio file (optional)\n");
    printf("      --repeat <num>\t\tset the number of repeats\n");
    printf("      --velocity <value>\tset the note velocity\n");
    printf("  -p, --pitch <pitch>\t\tSet the playback pitch\n");
    printf("  -v, --verbose [<level>]\tshow extra playback information\n");
    printf("  -h, --help\t\t\tprint this message and exit\n");

    printf("\n");

    exit(-1);
}

#define MAX_WAVES	5
#define MAX_HARMONICS	8
const char *names[MAX_WAVES] =
{
   "Sine", "Triangle", "Square", "Sawtooth", "Impulse"
};

float harmonics[MAX_WAVES][MAX_HARMONICS] =
{
   // sine
   { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
   // triangle
   { 1.0f, 0.0f, 0.111f, 0.0f, 0.04f, 0.0f, 0.02f, 0.0f },
   // square
   { 1.0f, 0.0f, 0.3f, 0.0f, 0.2f, 0.0f, 0.143f, 0.0f },
   // sawtooth
   { 1.0f, 0.5f, 0.3f, 0.25f, 0.2f, 0.167f, 0.143f, 0.125f },
   // impulse
   { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f }
};

void analyze_fft(aaxBuffer buffer, char verbose)
{
    float **data;

    aaxBufferSetSetup(buffer, AAX_FORMAT, AAX_FLOAT);
    data = (float**)aaxBufferGetData(buffer);
    if (data)
    {
        int no_samples = aaxBufferGetSetup(buffer, AAX_NO_SAMPLES);
        int fs = aaxBufferGetSetup(buffer, AAX_FREQUENCY);
        float *ffttmp, *fftout, *realin;
        int block_size, size;

        // block length
        block_size = BLOCK_SIZE;

        size = sizeof(float)*2*(block_size+1);
        realin = pffft_aligned_malloc(size);
        fftout = pffft_aligned_malloc(size);
        ffttmp = pffft_aligned_malloc(size);
        if (realin && fftout && ffttmp)
        {
            PFFFT_Setup *fft = pffft_new_setup(block_size, PFFFT_REAL);
            float f, fb = 0.0f;
            int i, j;

            memset(fftout, 0, size);

            // loop the buffer one block at a time and add all FFT results
            while (no_samples > block_size)
            {
                int conversion_num = _MIN(no_samples, block_size);

                memset(realin, 0, size);
                memcpy(realin, *data, sizeof(float)*conversion_num);

                *data += conversion_num;
                no_samples -= conversion_num;

                memset(ffttmp, 0, size);
                pffft_transform_ordered(fft, realin, ffttmp, NULL, PFFFT_FORWARD);

                // Add the real (sine) and imaginary (cosine) values and move
                // them to the front of the buffer. Phase is of no concern to us
                for (i=0; i<block_size; ++i) {
                   fftout[i] += fabsf(ffttmp[2*i]) + fabsf(ffttmp[2*i+1]);
                }
            }

            // start normalization
            f = 0.0f;
            for (i=0; i<block_size; ++i) {
                if (f < fabsf(fftout[i]))
                {
                    f = fabsf(fftout[i]);
                    fb = (float)fs*i/block_size; // base frequency;
                }
            }

            if (f > 0.0f)
            {
                f = 1.0f/f;
                for (i=0; i<block_size; ++i) {
                    fftout[i] *= f;
                }
            }
            // end normalization

            if (verbose)
            {
                printf(" Buffer sample frequency: %5i Hz\n", fs);
                printf(" Buffer base frequency  : %5.0f Hz\n", rintf(fb));
                printf("      Harmonics: ");
                for (i=2; i<MAX_HARMONICS; ++i) {
                    printf("%5.0f Hz ", rintf(i*fb));
                }
                printf("\n");

                if (verbose > 1)
                {
                    for (j=0; j<MAX_WAVES; ++j)
                    {
                        printf("      %9s:   ", names[j]);
                        for (i=2; i<MAX_HARMONICS; ++i) {
                            printf("%5.3g    ", harmonics[j][i]);
                        }
                        printf("\n");
                    }
                }
                printf("\n");
            }

            printf("Frequency\tSignal Level\n");
            printf("---------\t------------\n");
            for (i=0; i<block_size; ++i)
            {
                float v = fftout[i];
                if (v*v > 0.01f) {
                    printf("% 6.1f Hz:\t% 7.2f\n", (float)fs*i/block_size, v);
                }
            }

            pffft_aligned_free(ffttmp);
            pffft_aligned_free(fftout);
            pffft_aligned_free(realin);
            pffft_destroy_setup(fft);
        }

        aaxFree(data);
    }
}

int main(int argc, char **argv)
{
    char *devname, *infile, *reffile = NULL;
    aaxConfig config;
    float velocity;
    float fraction;
    int base_freq[2];
    char verbose = 0;
    char repeat = 0;
    char fft = 0;
    char fm = 0;
    char *env;
    int res;

    if (argc == 1 || getCommandLineOption(argc, argv, "-h") ||
                     getCommandLineOption(argc, argv, "--help"))
    {
        help();
    }

    env = getCommandLineOption(argc, argv, "-v");
    if (!env) env = getCommandLineOption(argc, argv, "--verbose");
    if (env) {
        verbose = atoi(env);
        if (!verbose) verbose = 1;
    }

    if (getCommandLineOption(argc, argv, "--repeat")) {
        repeat = 5;
    }

    if (getCommandLineOption(argc, argv, "--fft")) {
        fft = 1;
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

//  reffile = getCommandLineOption(argc, argv, "--reference");
//  if (!reffile) reffile = getCommandLineOption(argc, argv, "-r");
//  if (!reffile) reffile = getenv("AAX_REFERENCE_FILE");

    devname = getDeviceName(argc, argv);
    infile = getInputFile(argc, argv, FILE_PATH);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config)
    {
        aaxBuffer buffer, refbuf = NULL;
        char *ofile;

        res = aaxMixerSetSetup(config, AAX_REFRESH_RATE, 90.0f);
        testForState(res, "aaxMixerSetSetup");

         if (fm) {
            res = aaxMixerSetSetup(config, AAX_CAPABILITIES, AAX_RENDER_SYNTHESIZER);
         }

        buffer = aaxBufferReadFromStream(config, infile);
        testForError(buffer, "Unable to create a buffer");

        if (verbose)
        {
            const char *s;

            s = aaxDriverGetSetup(config, AAX_MUSIC_PERFORMER_STRING);
            if (s) printf(" Performer: %s\n", s);

            s = aaxDriverGetSetup(config, AAX_TRACK_TITLE_STRING);
            if (s) printf(" Title    : %s\n", s);

            s = aaxDriverGetSetup(config, AAX_ALBUM_NAME_STRING);
            if (s) printf(" Album    : %s\n", s);

            s = aaxDriverGetSetup(config, AAX_SONG_COMPOSER_STRING);
            if (s) printf(" Composer : %s\n", s);

            s = aaxDriverGetSetup(config, AAX_ORIGINAL_PERFORMER_STRING);
            if (s) printf(" Original : %s\n", s);

            s = aaxDriverGetSetup(config, AAX_MUSIC_GENRE_STRING);
            if (s) printf(" Genre    : %s\n", s);
            s = aaxDriverGetSetup(config, AAX_RELEASE_DATE_STRING);
            if (s) printf(" Release date: %s\n", s);

            s = aaxDriverGetSetup(config, AAX_TRACK_NUMBER_STRING);
            if (s) printf(" Track number: %s\n", s);

            s = aaxDriverGetSetup(config, AAX_SONG_COPYRIGHT_STRING);
            if (s) printf(" Copyright: %s\n", s);

            s = aaxDriverGetSetup(config, AAX_CONTACT_STRING);
            if (s) printf(" Website  : %s\n", s);

            s = aaxDriverGetSetup(config, AAX_SONG_COMMENT_STRING);
            if (s) printf(" Comment  : %s\n", s);
        }

        if (reffile)
        {
            refbuf = bufferFromFile(config, reffile);
            testForError(refbuf, "Unable to create the refbuf buffer");
        }

        base_freq[0] = aaxBufferGetSetup(buffer, AAX_UPDATE_RATE);
        fraction = 1e-6f*aaxBufferGetSetup(buffer, AAX_REFRESH_RATE);
        if (reffile) {
            base_freq[1] = aaxBufferGetSetup(refbuf, AAX_UPDATE_RATE);
        } else {
            base_freq[1] = 0.0f;
        }

        if (fft)
        {
            analyze_fft(buffer, verbose);
            goto done;
        }

        if (verbose)
        {
            unsigned int start, end, tracks;

            if (reffile) {
                printf("Reference file: %s\n", reffile);
                printf(" Buffer sample frequency: %5i Hz\n",
                        aaxBufferGetSetup(refbuf, AAX_FREQUENCY));
                printf(" Buffer base frequency  : %5i Hz\n", base_freq[1]);
                printf("\n");
            }
            printf("Base file: %s\n", infile);
            printf(" Buffer sample frequency: %5i Hz\n",
                    aaxBufferGetSetup(buffer, AAX_FREQUENCY));
            printf(" Buffer base frequency  : %5i Hz\n", base_freq[0]);
            printf(" Buffer pitch fraction  : %4.3f\n", fraction);

            tracks = aaxBufferGetSetup(buffer, AAX_TRACKS);
            printf(" Buffer no. tracks      : %i\n", tracks);

            start = aaxBufferGetSetup(buffer, AAX_LOOP_START);
            end = aaxBufferGetSetup(buffer, AAX_LOOP_END);
            printf(" Buffer loop start:     : %i\n", start);
            printf(" Buffer loop end:       : %i\n", end);
        }

        ofile = getOutputFile(argc, argv, NULL);
        if (!ofile && buffer)
        {
            float envelope[MAX_STAGES+1], envelope_step, envelope_time;
            float prevgain, gain;
            float freq, pitch[2], pitch2, pitch_time;
            aaxEmitter e, emitter, refem = NULL;
            int i, stage, max_stages, state, key;
            int paused = AAX_FALSE;
            int playref = AAX_FALSE;
            aaxFrame frame, frame2;
            aaxFilter filter;
            aaxEffect effect;
            aaxBuffer xbuffer;
            aaxMtx4d mtx64;
            float duration;
            float dt = 0.0f;

            duration = getDuration(argc, argv);

            /* envelope */
            stage = 0;
            max_stages = 0;
            for (i=0; i<MAX_STAGES+1; ++i) {
               envelope[i] = getEnvelopeStage(argc, argv, i);
               if (envelope[i] > 0.0f) max_stages++;
            }
            gain = prevgain = (envelope[0] > 0.0f) ? envelope[0] : 1.0f;

            envelope_time = getGainTime(argc, argv);
            if (envelope_time == 0.0f) envelope_time = SLIDE_TIME;

            if (envelope[stage+1] == 0.0f) {
                envelope_step = 0.0f;
            } else {
                envelope_step = (envelope[1]-envelope[0])/(envelope_time/SLEEP_TIME);
            }

            /* pitch */
            pitch_time = SLIDE_TIME;
            pitch[0] = pitch[1] = 1.0f;
            freq = getFrequency(argc, argv);
            if (freq == 0.0f)
            {
                pitch[0] = getPitch(argc, argv);
                pitch2 = getPitchRange(argc, argv);
                pitch_time = getPitchTime(argc, argv);
                if (pitch_time == 0.0f) pitch_time = SLIDE_TIME;
            }
            else
            {
                freq = fraction*(freq - base_freq[0]) + base_freq[0];
                if (base_freq[0]) pitch[0] = freq/base_freq[0];
                if (base_freq[1]) pitch[1] = freq/base_freq[1];
                pitch2 = 0.0f;
            }

            if (verbose)
            {
                if (max_stages > 1)
                {
                    printf("Playback envelope:\n");
                    printf("\tstage duration: %3.1f sec.\n", envelope_time);
                    printf("\tstage:");
                    for(i=0; i<max_stages; ++i) {
                        printf("\t%5i", i);
                    }
                    printf("\n");
                    printf("\tenvelope:");
                    for(i=0; i<max_stages; ++i) {
                        printf("\t%5.1f", envelope[i]);
                    }
                    printf("\n");
                }
                else {
                    printf("Playback envelope     : %5.1f\n", gain);
                }

                printf("Playback pitch    : %5.1f", pitch[0]);
                if (pitch2) {
                    printf("    to % .1f,     duration: %3.1f sec.\n",
                              pitch2, pitch_time);
                } else printf("\n");

                printf("Playback frequency: %5.1f Hz", pitch[0]*base_freq[0]);
                if (pitch2 != 0.0f) {
                    printf(" to %5.1f Hz, duration: %3.1f sec.\n",
                              pitch2*base_freq[0], pitch_time);
                } else printf("\n");
            }

            /** emitter */
            emitter = aaxEmitterCreate();
            testForError(emitter, "Unable to create a new emitter");

            res = aaxMatrix64SetDirection(mtx64, EmitterPos, EmitterDir);
            testForState(res, "aaxMatrix64SetDirection");

            res = aaxEmitterSetMatrix64(emitter, mtx64);
            testForState(res, "aaxEmitterSetMatrix64");

            if (velocity)
            {
               res = aaxEmitterSetSetup(emitter, AAX_VELOCITY_FACTOR,
                                        127.0f*_MINMAX(velocity, 0.0f, 1.0f));
//             testForState(res, "Unable to set velocity");
            }

            res = aaxEmitterSetMode(emitter, AAX_POSITION, AAX_ABSOLUTE);
            testForState(res, "Unable to set emitter mode");

            refem = emitter;
            if (reffile)
            {
                refem = aaxEmitterCreate();
                testForError(refem, "Unable to create a new emitter");

                res = aaxEmitterSetMatrix64(refem, mtx64);
                testForState(res, "aaxEmitterSetMatrix64");

                res = aaxEmitterSetMode(refem, AAX_POSITION, AAX_ABSOLUTE);
                testForState(res, "Unable to set emitter mode");

                if (velocity)
                {
                   res = aaxEmitterSetSetup(refem, AAX_VELOCITY_FACTOR,
                                          127.0f*_MINMAX(velocity, 0.0f, 1.0f));
//                 testForState(res, "Unable to set velocity");
                }
            }

            /* envelope */
            filter = aaxEmitterGetFilter(emitter, AAX_VOLUME_FILTER);
            testForError(filter, "Unable to create the volume filter");

            res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, gain);
            testForState(res, "aaxFilterSetParam");

            res = aaxFilterSetParam(filter, AAX_MAX_GAIN, AAX_LINEAR, 16.f);
            testForState(res, "aaxFilterSetParam");

            res = aaxEmitterSetFilter(emitter, filter);
            testForState(res, "aaxEmitterSetGain");

            if (reffile)
            {
                res = aaxEmitterSetFilter(refem, filter);
                testForState(res, "aaxEmitterSetGain");
            }
            aaxFilterDestroy(filter);

            /* pitch */
            effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
            testForError(effect, "Unable to create the pitch effect");

            if (pitch2 == 0.0f)
            {
                res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR,
                                                pitch[0]);
                testForState(res, "aaxEffectSetParam");
            }
            else
            {
                res = aaxEffectSetSlot(effect, 0, AAX_LINEAR, pitch[0],
                                                pitch2, pitch2, pitch_time);
                testForState(res, "aaxEffectSetParam");

                res = aaxEffectSetState(effect, AAX_ENVELOPE_FOLLOW);
                testForState(res, "aaxEffectSetState");
            }
            res = aaxEmitterSetEffect(emitter, effect);
            testForState(res, "aaxEmitterSetPitch");

            if (reffile)
            {
                res = aaxEffectSetSlot(effect, 0, AAX_LINEAR, pitch[1],
                                                  4.0f, 0.0f, 0.0f);
                testForState(res, "aaxEffectSetParam");

                res = aaxEmitterSetEffect(refem, effect);
                testForState(res, "aaxEmitterSetPitch");
            }
            aaxEffectDestroy(effect);

            /* buffer */
            res = aaxEmitterAddBuffer(emitter, buffer);
            if (reffile) {
                res = aaxEmitterAddBuffer(refem, refbuf);
            }
            testForState(res, "aaxEmitterAddBuffer");

            /** audio-frame */
            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            frame2 = aaxAudioFrameCreate(config);
            testForError(frame2, "Unable to create a new audio frame");

            res = aaxMixerRegisterAudioFrame(config, frame2);
            testForState(res, "aaxMixerRegisterAudioFrame");

            res = aaxAudioFrameSetState(frame2, AAX_PLAYING);
            testForState(res, "aaxAudioFrameStart");

            frame = aaxAudioFrameCreate(config);
            testForError(frame, "Unable to create a new audio frame");

            res = aaxAudioFrameRegisterAudioFrame(frame2, frame);
            testForState(res, "aaxMixerRegisterAudioFrame");

            res = aaxAudioFrameSetState(frame, AAX_PLAYING);
            testForState(res, "aaxAudioFrameStart");

            res = aaxAudioFrameAddBuffer(frame, buffer);
            // testForState(res, "aaxAudioFrameAddBuffer");

            res = aaxAudioFrameRegisterEmitter(frame, emitter);
            testForState(res, "aaxAudioFrameRegisterEmitter");

            if (reffile)
            {
                res = aaxAudioFrameRegisterEmitter(frame, refem);
                testForState(res, "aaxAudioFrameRegisterEmitter");
            }

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

            xbuffer = setFiltersEffects(argc, argv, config, NULL, frame, emitter, NULL);
            if (xbuffer) {
                res = aaxAudioFrameAddBuffer(frame2, xbuffer);
            }

            /** schedule the emitter for playback */
            if (playref)
            {
                res = aaxEmitterSetState(refem, AAX_INITIALIZED);
                testForState(res, "aaxEmitterinit");
                res = aaxEmitterSetState(refem, AAX_PLAYING);
                testForState(res, "aaxEmitterStart");
            } else {
                res = aaxEmitterSetState(emitter, AAX_INITIALIZED);
                testForState(res, "aaxEmitterinit");
                res = aaxEmitterSetState(emitter, AAX_PLAYING);
                testForState(res, "aaxEmitterStart");
            }

            dt = (float)aaxBufferGetSetup(buffer, AAX_NO_SAMPLES);
            dt /= (float)aaxBufferGetSetup(buffer, AAX_FREQUENCY);
            if (duration > dt && !aaxBufferGetSetup(buffer, AAX_LOOPING)) {
               duration = dt;
            }

            printf("Playing sound for %3.1f seconds of %3.1f seconds, "
                   "or until a key is pressed\n", duration, dt);
            i = 0;
            dt = 0.0f;
            set_mode(1);
            do
            {
                e = (!playref || !reffile) ? emitter : refem;

                msecSleep(SLEEP_TIME*1000);
                dt += SLEEP_TIME;

                if (!paused && ++i > 10)
                {
                    unsigned long offs, offs_bytes;
                    float off_s;
                    i = 0;

                    off_s = aaxEmitterGetOffsetSec(e);
                    offs = aaxEmitterGetOffset(e, AAX_SAMPLES);
                    offs_bytes = aaxEmitterGetOffset(e, AAX_BYTES);
                    printf("playing time: %5.2f, buffer position: %5.2f "
                           "(%li samples/ %li bytes)\n", dt, off_s,
                           offs, offs_bytes);
                }

                /* envelope */
                if (envelope_step > 0.0f) {
                    gain = _MIN(gain+envelope_step, envelope[stage+1]);
                } else if (envelope_step < 0.0f) {
                    gain = _MAX(gain+envelope_step, envelope[stage+1]);
                }

                if (gain != prevgain)
                {
                    filter = aaxEmitterGetFilter(emitter, AAX_VOLUME_FILTER);
                    testForError(filter, "Unable to create the volume filter");

                    res = aaxFilterSetParam(filter, AAX_GAIN, AAX_LINEAR, gain);
                    testForState(res, "aaxFilterSetParam");

                    res = aaxEmitterSetFilter(emitter, filter);
                    testForState(res, "aaxEmitterSetGain");
                    if (reffile)
                    {
                        res = aaxEmitterSetFilter(refem, filter);
                        testForState(res, "aaxEmitterSetGain");
                    }
                    aaxFilterDestroy(filter);

#if 0
                    if (repeat && dt > 0.1f)
                    {
                        aaxEmitterSetState(e, AAX_INITIALIZED);
                        aaxEmitterSetState(e, AAX_PLAYING);
                    }
#endif
                    prevgain = gain;
                }
                else if (stage < max_stages)
                {
                    stage++;
                    if (envelope[stage+1] == 0.0f) {
                        envelope_step = 0.0f;
                    } else {
                        envelope_step = (envelope[stage+1]-envelope[stage])/(envelope_time/SLEEP_TIME);
                        gain = envelope[stage];
                    }
                }

                if (repeat && dt > 0.1f)
                {
                    dt = 0.0f;
                    repeat--;
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
                        playref = !playref;
                        if (playref)
                        {
                            aaxEmitterSetState(emitter, AAX_PROCESSED);

                            if (reffile)
                            {
                                aaxEmitterSetState(refem, AAX_INITIALIZED);
                                aaxEmitterSetState(refem, AAX_PLAYING);
                                printf("Switching to the reference file.\n");
                            } else {
                                printf("No reference file specified.\n");
                            }
                        }
                        else
                        {
                            if (reffile) {
                                aaxEmitterSetState(refem, AAX_PROCESSED);
                            }

                            aaxEmitterSetState(emitter, AAX_INITIALIZED);
                            aaxEmitterSetState(emitter, AAX_PLAYING);
                            printf("Switching to the regular file.\n");
                        }
                        dt = 0.0f;
                    }
                    else if (key == ESCAPE_KEY) {
                        break;
                    } else {
                        aaxEmitterSetState(emitter, AAX_STOPPED);
                    }
                }

                if (playref && reffile) {
                    state = aaxEmitterGetState(refem);
                } else {
                    state = aaxEmitterGetState(emitter);
                }
            }
            while ((dt < duration) && (state != AAX_PROCESSED));
            set_mode(0);

            e = (!playref || !reffile) ? emitter : refem;
            res = aaxEmitterSetState(e, AAX_STOPPED);
            testForState(res, "aaxEmitterStop");

#if 0
            do
            {
                unsigned long offs, offs_bytes;
                float off_s;

                off_s = aaxEmitterGetOffsetSec(e);
                offs = aaxEmitterGetOffset(e, AAX_SAMPLES);
                offs_bytes = aaxEmitterGetOffset(e, AAX_BYTES);
                printf("2. playing time: %5.2f, buffer position: %5.2f "
                       "(%li samples/ %li bytes)\n", dt, off_s,
                       offs, offs_bytes);

                msecSleep(50);
                state = aaxEmitterGetState(e);
                dt += 0.05f;
            }
            while (state != AAX_PROCESSED);
#endif

            res = aaxEmitterSetState(emitter, AAX_PROCESSED);
            testForState(res, "aaxEmitterSetState");

            res = aaxAudioFrameDeregisterEmitter(frame, emitter);
            testForState(res, "aaxAudioFrameDeregisterEmitter");

            res = aaxEmitterDestroy(emitter);
            testForState(res, "aaxEmitterDestroy");

            if (reffile)
            {
                res = aaxEmitterSetState(refem, AAX_PROCESSED);
                testForState(res, "aaxEmitterSetState");

                res = aaxAudioFrameDeregisterEmitter(frame, refem);
                testForState(res, "aaxAudioFrameDeregisterEmitter");

                res = aaxEmitterDestroy(refem);
                testForState(res, "aaxEmitterDestroy");

                res = aaxBufferDestroy(refbuf);
                testForState(res, "aaxBufferDestroy");
            }

            res = aaxAudioFrameSetState(frame, AAX_STOPPED);
            testForState(res, "aaxAudioFrameSetState");

            res = aaxAudioFrameDeregisterAudioFrame(frame2, frame);
            testForState(res, "aaxAudioFrameDeregisterAudioFrame");

            res = aaxAudioFrameDestroy(frame);
            testForState(res, "aaxAudioFrameDestroy");

            res = aaxAudioFrameSetState(frame2, AAX_STOPPED);
            testForState(res, "aaxAudioFrameSetState");

            res = aaxMixerDeregisterAudioFrame(config, frame2);
            testForState(res, "aaxAudioFrameDeregisterAudioFrame");

            res = aaxAudioFrameDestroy(frame2);
            testForState(res, "aaxAudioFrameDestroy");

            res = aaxMixerSetState(config, AAX_STOPPED);

            if (xbuffer) {
                res = aaxBufferDestroy(xbuffer);
            }

done:
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
