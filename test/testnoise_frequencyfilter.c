/*
 * Copyright (C) 2008-2016 by Erik Hofman.
 * Copyright (C) 2009-2016 by Adalin B.V.
 * All rights reserved.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>	// getenv
#include <errno.h>	// EINTR


#include <aax/aax.h>

#include "base/timer.h"

#define	SAMPLE_FREQ		48000
#define FILTER_FREQUENCY	1000

// Order: AAX_6DB_OCT, AAX_12DB_OCT, AAX_24DB_OCT, AAX_36DB_OCT, AAX_48DB_OCT
// Type:  AAX_BUTTERWORTH, AAX_BESSEL
#define Q			1.0f
#define LF_GAIN			1.0f
#define HF_GAIN			0.0f
#define FILTER_ORDER		AAX_48DB_OCT
#define FILTER_TYPE		AAX_BUTTERWORTH
#define FILTER_STATE		(FILTER_TYPE|FILTER_ORDER)

void
testForError(void *p, char *s)
{
    if (p == NULL)
    {
        int err = aaxGetErrorNo();
        printf("\nError: %s\n", s);
        if (err) {
            printf("%s\n\n", aaxGetErrorString(err));
        }
        exit(-1);
    }
}

void
testForState(int res, const char *func)
{
    if (res != AAX_TRUE)
    {
        int err = aaxGetErrorNo();
        printf("%s:\t\t%i\n", func, res);
        printf("(%i) %s\n\n", err, aaxGetErrorString(err));
        exit(-1);
    }
}

int main()
{
    char *tmp, devname[128], filename[64];
    aaxConfig config;
    int res = 0;

    tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = "/tmp";

    snprintf(filename, 64, "%s/whitenoise.wav", tmp);
    snprintf(devname, 128, "AeonWave on Audio Files: %s", filename);

    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "No default audio device available.");

    if (config)
    {
        int no_samples;
        aaxEmitter emitter;
        aaxBuffer buffer;
        aaxFilter filter;
        float dt;

        if (aaxIsFilterSupported(config, "AAX_frequency_filter") == 0) {
           printf("AAX_frequency_filter not supported\n");
        }
        if (aaxIsFilterSupported(config, "AAX_frequency_filter_1.0") == 0) {
           printf("AAX_frequency_filter version 1.0 not supported\n");
        }
        if (aaxIsFilterSupported(config, "AAX_frequency_filter_1.1") == 0) {
           printf("AAX_frequency_filter version 1.1 not supported\n");
        }

        no_samples = (unsigned int)(4*SAMPLE_FREQ);
        buffer = aaxBufferCreate(config, no_samples, 1, AAX_PCM16S);
        testForError(buffer, "Unable to generate buffer\n");

        res = aaxBufferSetSetup(buffer, AAX_FREQUENCY, SAMPLE_FREQ);
        testForState(res, "aaxBufferSetFrequency");

        res = aaxBufferProcessWaveform(buffer, 0.0f, AAX_WHITE_NOISE, 1.0f,
                                       AAX_OVERWRITE);
        testForState(res, "aaxBufferProcessWaveform");

        /** mixer */
        res = aaxMixerSetState(config, AAX_INITIALIZED);
        testForState(res, "aaxMixerInit");

        res = aaxMixerSetState(config, AAX_PLAYING);
        testForState(res, "aaxMixerStart");

        /** emitter */
        emitter = aaxEmitterCreate();
        testForError(emitter, "Unable to create a new emitter\n");

        res = aaxEmitterAddBuffer(emitter, buffer);
        testForState(res, "aaxEmitterAddBuffer");

        res = aaxEmitterSetMode(emitter, AAX_LOOPING, AAX_TRUE);
        testForState(res, "aaxEmitterSetMode");

        /* frequency filter */
        filter = aaxFilterCreate(config, AAX_FREQUENCY_FILTER);
        testForError(filter, "aaxFilterCreate");

        res = aaxFilterSetSlot(filter, 0, AAX_LINEAR, FILTER_FREQUENCY,
                                             LF_GAIN, HF_GAIN, Q);
        testForState(res, "aaxFilterSetSlot");

        res = aaxFilterSetState(filter, FILTER_STATE);
        testForState(res, "aaxFilterSetState");

        res = aaxEmitterSetFilter(emitter, filter);
        testForState(res, "aaxEmitterSetFilter");

        res = aaxFilterDestroy(filter);

        res = aaxMixerRegisterEmitter(config, emitter);
        testForState(res, "aaxMixerRegisterEmitter");

        printf("writing white noise to: %s\n", filename);
        res = aaxEmitterSetState(emitter, AAX_PLAYING);
        testForState(res, "aaxEmitterStart");

        dt = 0.0f;
        do
        {
            dt += 0.05f;
            msecSleep(50);
            aaxEmitterGetState(emitter);
        }
        while (dt < 1.0f);;

        res = aaxEmitterSetState(emitter, AAX_PROCESSED);
        testForState(res, "aaxEmitterStop");

        res = aaxEmitterRemoveBuffer(emitter);
        testForState(res, "aaxEmitterRemoveBuffer");

        res = aaxBufferDestroy(buffer);
        testForState(res, "aaxBufferDestroy");

        res = aaxMixerDeregisterEmitter(config, emitter);
        res = aaxMixerSetState(config, AAX_STOPPED);
        res = aaxEmitterDestroy(emitter);
    }

    res = aaxDriverClose(config);
    res = aaxDriverDestroy(config);

    return res;
}
