/*
 * Copyright (C) 2008-2013 by Erik Hofman.
 * Copyright (C) 2009-2013 by Adalin B.V.
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
#include <stdlib.h>

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"

#define IFILE_PATH		SRC_PATH"/stereo.mp3"
#define OFILE_PATH		"aaxout.wav"

void
help()
{
    printf("Usage:\n");
    printf("  aaxplay -i <filename> (-d <playcbakc device>)\n");
    printf("  aaxplay -c <capture device> (-d <playcbakc device>)\n\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    char *devname, *idevname;
    char ibuf[256], obuf[256];
    char *infile, *outfile;
    aaxConfig config = NULL;
    aaxConfig record = NULL;
    aaxConfig file = NULL;
    int res, rv = 0;

    if (argc == 1) {
        help();
    }

    idevname = getCaptureName(argc, argv);
    infile = getInputFile(argc, argv, IFILE_PATH);
    if (!idevname)
    {
       if (infile)
       {
           snprintf(ibuf, 256, "AeonWave on Audio Files: %s", infile);
           idevname = ibuf;
       }
       else {
          help();
       }
    }

    devname = getDeviceName(argc, argv);
    config = aaxDriverOpenByName(devname, AAX_MODE_WRITE_STEREO);
    testForError(config, "Audio output device is not available.");
    if (!config || !aaxIsValid(config, AAX_CONFIG_HD))
    {
    // TODO: fall back to buffer streaming mode
        if (!config) {
           printf("Warning: %s\n", aaxGetErrorString(aaxGetErrorNo()));
        }
        else
        {
            printf("Warning:\n");
            printf("  %s requires a registered version of AeonWave\n", argv[0]);
            printf("  Please visit http://www.adalin.com/buy_aeonwaveHD.html to ");
            printf("obtain\n  a product-key.\n\n");
        }
        exit(-2);
    }
    else
    {
        record = aaxDriverOpenByName(idevname, AAX_MODE_READ);
        if (!record)
        {
            printf("File not found: %s\n", infile);
            exit(-1);
        }
        printf("Playing: %s\n", idevname ? idevname : infile);
    }

    outfile = getOutputFile(argc, argv, NULL);
    if (outfile)
    {
        snprintf(obuf, 256, "AeonWave on Audio Files: %s", outfile);
        file = aaxDriverOpenByName(obuf, AAX_MODE_WRITE_STEREO);
    }
    else {
        file = NULL;
    }

    if (config && record && (rv >= 0))
    {
        char *fparam = getCommandLineOption(argc, argv, "-f");
        float pitch = getPitch(argc, argv);
        aaxFrame frame = NULL;
        aaxEffect effect;
        aaxFilter filter;
        int state;

        /** mixer */
        res = aaxMixerSetSetup(config, AAX_REFRESHRATE, 64);
        testForState(res, "aaxMixerSetSetup");

        res = aaxMixerSetState(config, AAX_INITIALIZED);
        testForState(res, "aaxMixerInit");

        res = aaxMixerSetState(config, AAX_PLAYING);
        testForState(res, "aaxMixerStart");

        if (fparam)
        {
            printf("  using audio-frames\n");

            /** audio frame */
            frame = aaxAudioFrameCreate(config);
            testForError(frame, "Unable to create a new audio frame\n");   

            /** register audio frame */
            res = aaxMixerRegisterAudioFrame(config, frame);
            testForState(res, "aaxMixerRegisterAudioFrame");

            res = aaxAudioFrameSetState(frame, AAX_PLAYING);
            testForState(res, "aaxAudioFrameSetState");

            if (pitch != 1.0f)
            {
#if 0
                effect = aaxAudioFrameGetEffect(frame, AAX_PITCH_EFFECT);
                testForError(effect, "aaxEffectCreate");

                aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR, pitch);
#else
                effect = aaxAudioFrameGetEffect(frame,AAX_DYNAMIC_PITCH_EFFECT);
                testForError(effect, "aaxEffectCreate");

                effect = aaxEffectSetSlot(effect, 0, AAX_LINEAR,
                                          0.0f, 0.025f, pitch, 0.0f);
                testForError(effect, "aaxEffectSetSlot");

                effect = aaxEffectSetState(effect, AAX_SINE_WAVE);
                testForError(effect, "aaxEffectSetState");
#endif
                res = aaxAudioFrameSetEffect(frame, effect);
                testForState(res, " aaxAudioFrameSetEffect");

                aaxEffectDestroy(effect);
                testForState(res, "aaxEffectDestroy");
            }

            /** sensor */
            res = aaxAudioFrameRegisterSensor(frame, record);
            testForState(res, "aaxAudioFrameRegisterSensor");
        }
        else
        {
#if 0
            effect = aaxMixerGetEffect(config, AAX_PITCH_EFFECT);
            testForError(effect, "aaxEffectCreate");

            aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR, pitch);

            res = aaxMixerSetEffect(record, effect);
            testForState(res, "aaxEmitterSetEffect");

            res = aaxEffectDestroy(effect);
            testForState(res, "aaxEffectDestroy");
#endif

            /** sensor */
            res = aaxMixerRegisterSensor(config, record);
            testForState(res, "aaxMixerRegisterSensor");
        }

        if (pitch != 1.0f)
        {
            effect = aaxMixerGetEffect(record, AAX_DYNAMIC_PITCH_EFFECT);
            testForError(effect, "aaxEffectCreate");

            effect = aaxEffectSetSlot(effect, 0, AAX_LINEAR,
                                      0.0f, frame ? 0.5f : 0.06f, pitch, 0.0f);
            testForError(effect, "aaxEffectSetSlot");

            effect = aaxEffectSetState(effect, AAX_TRIANGLE_WAVE);
            testForError(effect, "aaxEffectSetState");

            res = aaxMixerSetEffect(record, effect);
            testForState(res, "aaxEmitterSetEffect");

            res = aaxEffectDestroy(effect);
            testForState(res, "aaxEffectDestroy");
        }

        if (file)
        {
            res = aaxMixerRegisterSensor(config, file);
            testForState(res, "aaxMixerRegisterSensor file out");
        }

        /** set capturing Auto-Gain Control (AGC): 0dB */
        filter = aaxMixerGetFilter(record, AAX_VOLUME_FILTER);
        if (filter)
        {
            aaxFilterSetParam(filter, AAX_AGC_RESPONSE_RATE, AAX_LINEAR, 1.5f);
            aaxMixerSetFilter(record, filter);
            res = aaxFilterDestroy(filter);
        }

        /** must be called after aaxMixerRegisterSensor */
        res = aaxMixerSetState(record, AAX_INITIALIZED);
        testForState(res, "aaxMixerSetInitialize");

        res = aaxSensorSetState(record, AAX_CAPTURING);
        testForState(res, "aaxSensorCaptureStart");

        if (file)
        {
            res = aaxMixerSetState(file, AAX_INITIALIZED);
            testForState(res, "aaxMixerSetInitialize");

            res = aaxMixerSetState(file, AAX_PLAYING);
            testForState(res, "aaxSensorCaptureStart");
        }

        set_mode(1);
        do
        {
            if (get_key()) break;

            msecSleep(100);
            state = aaxMixerGetState(record);
        }
        while (state == AAX_PLAYING);
        set_mode(0);

        res = aaxMixerSetState(config, AAX_STOPPED);
        testForState(res, "aaxMixerSetState");

        if (frame)
        {
            res = aaxAudioFrameSetState(frame, AAX_STOPPED);
            testForState(res, "aaxAudioFrameSetState");
        }

        res = aaxSensorSetState(record, AAX_STOPPED);
        testForState(res, "aaxSensorCaptureStop");

        if (frame)
        {
            res = aaxAudioFrameDeregisterSensor(frame, record);
            testForState(res, "aaxAudioFrameDeregisterSensor");

            res = aaxMixerDeregisterAudioFrame(config, frame);
            testForState(res, "aaxMixerDeregisterAudioFrame");

            res = aaxAudioFrameDestroy(frame);
            testForState(res, "aaxAudioFrameDestroy");
        }
        else
        {
            res = aaxMixerDeregisterSensor(config, record);
            testForState(res, "aaxMixerDeregisterSensor");
        }
    }
    else {
        printf("Unable to open capture device.\n");
    }

    if (record)
    {
        res = aaxDriverClose(record);
        res = aaxDriverDestroy(record);
    }

    if (file)
    {
        res = aaxMixerSetState(file, AAX_STOPPED);
        testForState(res, "aaxMixerSetState");

        res = aaxMixerDeregisterSensor(config, file);
        testForState(res, "aaxMixerRegisterSensor file out");

        res = aaxDriverClose(file);
        testForState(res, "aaxDriverClose");

        res = aaxDriverDestroy(file);
        testForState(res, "aaxDriverDestroy");
    }

    res = aaxDriverClose(config);
    testForState(res, "aaxDriverClose");

    res = aaxDriverDestroy(config);
    testForState(res, "aaxDriverDestroy");

    return rv;
}

