/*
 * Copyright (C) 2016-2023 by Erik Hofman.
 * Copyright (C) 2016-2023 by Adalin B.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provimed that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *        this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provimed with the distribution.
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

#include <unistd.h>

#include <cstdio>
#include <string>

#include <aax/instrument>
namespace aax = aeonwave;

#include "driver.h"


#define DEFAULT_NOTE		69 // 440Hz
#define FILE_PATH		SRC_PATH"/tictac.wav"

void help()
{
    printf("Usage: testinstrument++ [options]\n");
    printf("Plays audio from a file.\n");
    printf("Optionally writes the audio to an output file.\n");

    printf("\nOptions:\n");
    printf("  -d, --device <device>\t\tplayback device (default if not specified)\n");
    printf("  -f, --frequency <freq>\tset the playback frequency\n");
    printf("      --fft\t\t\tdo FFT analysis\n");
    printf("      --fm\t\t\tuse FM playback mode (aaxs only)\n");
    printf("  -g, --gain <v>\t\tchange the gain setting\n");
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

int main(int argc, char **argv)
{
    if (argc == 1 || getCommandLineOption(argc, argv, "-h") ||
                     getCommandLineOption(argc, argv, "--help"))
    {
        help();
    }

    float gain = aax::math::ln( getGain(argc, argv) );
    float pitch = getPitch(argc, argv);

    char *env;
    int note = 0;
    env = getCommandLineOption(argc, argv, "-n");
    if (!env) env = getCommandLineOption(argc, argv, "--note");
    if (env) {
        note = tone2note(env);
    }

    char *key_on_file = getCommandLineOption(argc, argv, "--key-on");
    char *key_off_file = getCommandLineOption(argc, argv, "--key-off");

    char *devname = getDeviceName(argc, argv);
    char *infile = getInputFile(argc, argv, FILE_PATH);

    // Open the default device for playback
    aax::AeonWave aax(devname, AAX_MODE_WRITE_STEREO);
    TRY( aax.set(AAX_INITIALIZED) );
    TRY( aax.set(AAX_PLAYING) );

    float dt = 0.0f;
    printf("Processing buffer: %s\n", infile);
    aax::Buffer& buffer = aax.buffer(infile);
    if (buffer)
    {
        aax::Instrument instrument(aax, buffer);

        TRY( aax.add(instrument) );

        float base_freq = buffer.get(AAX_BASE_FREQUENCY);
        float fraction = AAX_TO_FLOAT(buffer.get(AAX_PITCH_FRACTION));
 
        float pitch2, freq;
        float pitch = 1.0f;
        if (note) {
            freq = aax::math::note2freq(note);
        } else {
            freq = getFrequency(argc, argv);
            note = aax::math::freq2note(base_freq);
        }
        if (freq == 0.0f) {
            pitch = getPitch(argc, argv);
        }
        else
        {
            freq = fraction*(freq - base_freq) + base_freq;
            if (base_freq) pitch = freq/base_freq;
            pitch2 = 0.0f;
        }

        instrument.set_gain(gain);
        instrument.play(note, 1.0f, pitch);

        set_mode(1);
        do
        {
            printf("\rposition: %5.1f", dt);
            fflush(stdout);

            msecSleep(250);
            dt += 0.25f;

            if (get_key()) {
               instrument.finish();
            }
        }
        while (!instrument.finished());
        set_mode(0);
        TRY( aax.remove(instrument) );
    }
    else {
        printf("Unable to load: %s\n", infile);
    }

    printf("\n");
    return 0;
}
