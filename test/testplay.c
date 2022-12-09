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
    printf("Usage: testplay [options]\n");
    printf("Plays audio from a file.\n");
    printf("Optionally writes the audio to an output file.\n");

    printf("\nOptions:\n");
    printf("  -d, --device <device>\t\tplayback device (default if not specified)\n");
    printf("  -f, --frequency <freq>\tset the playback frequency\n");
    printf("      --fft\t\t\tdo FFT analysis\n");
    printf("      --fm\t\t\tuse FM playback mode (aaxs only)\n");
    printf("  -g, --gain <v>[:<dt>[-<v>..]\tchange the gain setting\n");
    printf("  -i, --input <file>\t\tplayback audio from a file\n");
    printf("      --key-on <file>\t\tSet the key-on sample file\n");
    printf("      --key-off <file>\t\tSet the key-off sample file\n");
    printf("  -n, --note <note>\t\tset the playback note\n");
    printf("  -o, --output <file>\t\talso write to an audio file (optional)\n");
    printf("      --repeat <num>\t\tset the number of repeats\n");
    printf("      --velocity <vel>\t\tset the note velocity\n");
    printf("  -p, --pitch <pitch>\t\tset the playback pitch\n");
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

inline unsigned
get_pow2(uint32_t n)
{
#if defined(__GNUC__)
    return 1 << (32 -__builtin_clz(n-1));
#else
   unsigned y, x = n;

   --x;
   x |= x >> 16;
   x |= x >> 8;
   x |= x >> 4;
   x |= x >> 2;
   x |= x >> 1;
   ++x;

   y = n >> 1;
   if (y < (x-n)) x >>= 1;

   return x;
#endif
}

float
note2freq(uint32_t d) {
    return 440.0f*powf(2.0f, ((float)d-69.0f)/12.0f);
}

inline uint32_t
freq2note(float f) {
    return roundf(12.0f*(logf(f/440.0f)/logf(2.0f))+69.0f);
}

uint32_t
tone2note(const char *t)
{
    uint32_t rv = 0;

    if (strlen(t) >= 2)
    {
        int letter, accidental, octave;

        if (isalpha(*t))
        {
            letter = toupper(*t++);
            accidental = (*t == '#') ? 1 : 0;
            if (accidental) t++;
            octave = atoi(t);

            rv = ((letter - 65) + 5) % 7;

            rv = 2*(((letter - 65) + 5) % 7);
            if (rv > 4) --rv;

            rv += accidental;
            rv += (octave+2)*12;
        }
        else {
           rv = atoi(t);
        }
    }

    return rv;
}

void
analyze_fft(aaxBuffer buffer, char verbose)
{
    float **data;

    aaxBufferSetSetup(buffer, AAX_FORMAT, AAX_FLOAT);
    data = (float**)aaxBufferGetData(buffer);
    if (data)
    {
        int max_samples, no_samples = aaxBufferGetSetup(buffer, AAX_NO_SAMPLES);
        int fs = aaxBufferGetSetup(buffer, AAX_FREQUENCY);
        float *ffttmp, *fftout, *realin;
        int block_size, size;

        // block length
        max_samples = no_samples;
        block_size = BLOCK_SIZE;
        if (block_size > no_samples) {
            block_size = get_pow2(no_samples)/2;
        }
        if (block_size < 2048)
        {
            printf("\nNot eough data for FFT analysis\n");
            return;
        }

        size = sizeof(float)*2*(block_size+1);
        realin = pffft_aligned_malloc(size);
        fftout = pffft_aligned_malloc(size);
        ffttmp = pffft_aligned_malloc(size);
        if (realin && fftout && ffttmp)
        {
            PFFFT_Setup *fft = pffft_new_setup(block_size, PFFFT_REAL);
            float f, fn, fb = 0.0f;
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
            for (i=0; i<block_size; ++i)
            {
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

            fn = note2freq(freq2note(fb));
            if (verbose)
            {
                printf(" Buffer duration: ");
                f = (float)max_samples/fs;
                if (f > 1.0f) printf("%14.1f sec\n", f);
                else if (f > 1e-3f) printf("%14.1f ms\n", 1e3f*f);
                else printf("%14.1f us\n", 1e6f*f);

                printf(" Buffer number of samples: %5i\n", max_samples);
                printf(" Buffer sample frequency: %6i Hz\n", fs);
                printf(" Buffer base frequency  : %6.0f Hz\n", roundf(fb));
                printf(" Closest musical note   : %11.4f Hz\n", fn);

                printf("      Harmonics: ");
                for (i=1; i<MAX_HARMONICS; ++i) {
                    printf("%4.0f Hz ", roundf(i*fn));
                }
                printf("\n");

                if (verbose > 1)
                {
                    for (j=0; j<MAX_WAVES; ++j)
                    {
                        printf("      %9s:   ", names[j]);
                        for (i=1; i<MAX_HARMONICS; ++i) {
                            printf("%5.3g   ", harmonics[j][i]);
                        }
                        printf("\n");
                    }
                }
                printf("\n");
            }

            printf(" Frequency\tLevel\tHarmonic\n");
            printf("-----------\t-----\t--------\n");
            for (i=0; i<block_size; ++i)
            {
                float v = fftout[i];
                if (v*v > 0.01f)
                {
                    float f = (float)fs*i/block_size;
                    float h = f/fn;
                    printf("%7.1f Hz\t%-5.2g\t% 6.2f x %c\n", f, v, h,
                           (fabsf(h -roundf(h)) < 0.005f) ? '*' : ' ');
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
    char *devname, *infile;
    char *key_on_file = NULL;
    char *key_off_file = NULL;
    char *reffile = NULL;
    aaxConfig config;
    float velocity;
    float fraction;
    int base_freq[2];
    char verbose = 0;
    char repeat = 0;
    char fft = 0;
    char fm = 0;
    char *env;
    int note;
    int res;

    if (argc == 1 || getCommandLineOption(argc, argv, "-h") ||
                     getCommandLineOption(argc, argv, "--help"))
    {
        help();
    }

    env = getCommandLineOption(argc, argv, "-v");
    if (!env) env = getCommandLineOption(argc, argv, "--verbose");
    if (env)
    {
        verbose = atoi(env);
        if (!verbose) verbose = 1;
    }

    note = 0;
    env = getCommandLineOption(argc, argv, "-n");
    if (!env) env = getCommandLineOption(argc, argv, "--note");
    if (env) {
        note = tone2note(env);
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

    key_on_file = getCommandLineOption(argc, argv, "--key-on");
    key_off_file = getCommandLineOption(argc, argv, "--key-off");

//  reffile = getCommandLineOption(argc, argv, "--reference");
//  if (!reffile) reffile = getCommandLineOption(argc, argv, "-r");
//  if (!reffile) reffile = getenv("AAX_REFERENCE_FILE");

    devname = getDeviceName(argc, argv);
    infile = getInputFile(argc, argv, FILE_PATH);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config)
    {
        aaxBuffer key_on_buffer = NULL, key_off_buffer = NULL;
        aaxBuffer buffer, refbuf = NULL;
        char *ofile;

        res = aaxMixerSetSetup(config, AAX_REFRESH_RATE, 90.0f);
        testForState(res, "aaxMixerSetSetup");

         if (fm) {
            res = aaxMixerSetSetup(config, AAX_CAPABILITIES, AAX_RENDER_SYNTHESIZER);
         }

        buffer = aaxBufferReadFromStream(config, infile);
        testForError(buffer, "Unable to create a buffer");

        key_on_buffer = aaxBufferReadFromStream(config, key_on_file);
        key_off_buffer = aaxBufferReadFromStream(config, key_off_file);

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
            unsigned int start, end, tracks, max_samples;
            float f, fs;

            if (reffile)
            {
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

            max_samples = aaxBufferGetSetup(buffer, AAX_NO_SAMPLES);
            printf(" Buffer no. samples     : %5i\n", max_samples);

            fs = (float)aaxBufferGetSetup(buffer, AAX_FREQUENCY);
            f = fs/max_samples;
            printf(" Buffer duration        : ");
            f = (float)max_samples/fs;
            if (f > 1.0f) printf("%5.1f sec\n", f);
            else if (f > 1e-3f) printf("%5.1f ms\n", 1e3f*f);
            else printf("%5.1f us\n", 1e6f*f);
        }

        ofile = getOutputFile(argc, argv, NULL);
        if (!ofile && buffer)
        {
            float envelope[MAX_STAGES+1], envelope_step, envelope_time;
            float prevgain, gain;
            float freq, pitch[2], pitch2, pitch_time;
            aaxEmitter key_on = NULL, key_off = NULL;
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
            for (i=0; i<MAX_STAGES+1; ++i)
            {
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
            if (note) {
                freq = note2freq(note);
            } else {
                freq = getFrequency(argc, argv);
            }
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
                if (pitch2)
                {
                    printf("    to % .1f,     duration: %3.1f sec.\n",
                              pitch2, pitch_time);
                } else printf("\n");

                printf("Playback frequency: %5.1f Hz", pitch[0]*base_freq[0]);
                if (pitch2 != 0.0f)
                {
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

            /** Key-On and Key-Off */
            if (key_on_buffer)
            {   
                key_on = aaxEmitterCreate();
                testForError(emitter, "Unable to create a new key-on emitter");

                res = aaxEmitterSetMatrix64(key_on, mtx64);
                testForState(res, "key-on aaxEmitterSetMatrix64");

                res = aaxEmitterSetMode(key_on, AAX_POSITION, AAX_ABSOLUTE);
                testForState(res, "Unable to set key-on emitter mode");

                res = aaxEmitterAddBuffer(key_on, key_on_buffer);
                testForState(res, "aaxEmitterAddBuffer key-on");

                /* pitch */
                effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
                testForError(effect, "Unable to create the pitch effect");

                res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR,
                                                pitch[0]);
                testForState(res, "aaxEffectSetParam");

                res = aaxEffectSetState(effect, AAX_ENVELOPE_FOLLOW);
                testForState(res, "aaxEffectSetState");

                res = aaxEmitterSetEffect(key_on, effect);
                testForState(res, "aaxEmitterSetPitch");
            }   

            if (key_off_buffer)
            {
                key_off = aaxEmitterCreate();
                testForError(emitter, "Unable to create a new key-off emitter");

                res = aaxEmitterSetMatrix64(key_off, mtx64);
                testForState(res, "key-off aaxEmitterSetMatrix64");

                res = aaxEmitterSetMode(key_off, AAX_POSITION, AAX_ABSOLUTE);
                testForState(res, "Unable to set key-off emitter mode");

                res = aaxEmitterAddBuffer(key_off, key_off_buffer);
                testForState(res, "aaxEmitterAddBuffer key-off");

                /* pitch */
                effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
                testForError(effect, "Unable to create the pitch effect");

                res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR,
                                                pitch[0]);
                testForState(res, "aaxEffectSetParam");

                res = aaxEffectSetState(effect, AAX_ENVELOPE_FOLLOW);
                testForState(res, "aaxEffectSetState");

                res = aaxEmitterSetEffect(key_off, effect);
                testForState(res, "aaxEmitterSetPitch");
            }

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
                testForState(res, "aaxAudioFrameRegisterEmitter reference");
            }

            if (key_on_buffer)
            {
                res = aaxAudioFrameRegisterEmitter(frame, key_on);
                testForState(res, "aaxAudioFrameRegisterEmitter key-on");
            }

            if (key_off_buffer)
            {
                res = aaxAudioFrameRegisterEmitter(frame, key_off);
                testForState(res, "aaxAudioFrameRegisterEmitter key-off");
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

                if (key_on_buffer)
                {
                    res = aaxEmitterSetState(key_on, AAX_INITIALIZED);
                    testForState(res, "aaxEmitterinit key-on");
                    res = aaxEmitterSetState(key_on, AAX_PLAYING);
                    testForState(res, "aaxEmitterStart key-on");
                }
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
                    }
                    else
                    {
                        aaxEmitterSetState(emitter, AAX_STOPPED);
                        if (key_off_buffer)
                        {
                            res = aaxEmitterSetState(key_off, AAX_INITIALIZED);
                            testForState(res, "aaxEmitterinit key-off");
                            res = aaxEmitterSetState(key_off, AAX_PLAYING);
                            testForState(res, "aaxEmitterStart key-off");
                        }
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
