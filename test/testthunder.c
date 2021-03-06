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
#include <stdlib.h>

#include <aax/aax.h>

#include "base/types.h"
#include "driver.h"
#include "wavfile.h"

#define MODE			AAX_ABSOLUTE

#define FILE_PATH		SRC_PATH"/thunder.aaxs"
#define SXPOS			0.0f
#define SYPOS			0.0f
#define SZPOS			0.0f

#define INITIAL_DIST		2000.0f
#define EXPOS			(SXPOS-INITIAL_DIST)
#define EYPOS			(SYPOS)
#define EZPOS			(SZPOS)

aaxVec3f EmitterDir = {  1.0f,  0.0f,  0.0f };
aaxVec3f EmitterVel = {  0.0f,  0.0f,  0.0f };
aaxVec3d EmitterPos = { EXPOS, EYPOS, EZPOS };

aaxVec3d SensorPos  = { SXPOS, SYPOS, SZPOS };
aaxVec3f SensorVel  = {  0.0f,  0.0f,  0.0f };
aaxVec3f SensorAt   = {  0.0f,  0.0f, -1.0f };
aaxVec3f SensorUp   = {  0.0f,  1.0f,  0.0f };

int main(int argc, char **argv)
{
    enum aaxRenderMode mode;
    char *devname, *infile;
    aaxConfig config;
    int res;

    mode = getMode(argc, argv);
    devname = getDeviceName(argc, argv);
    infile = getInputFile(argc, argv, FILE_PATH);
    config = aaxDriverOpenByName(devname, mode);
    if (config)
    {
        aaxBuffer buffer = bufferFromFile(config, infile);
        if (buffer)
        {
            char *dparam = getCommandLineOption(argc, argv, "-m");
            char *fparam = getCommandLineOption(argc, argv, "-f");
            aaxEmitter emitter;
            aaxFilter filter;
            aaxEffect effect;
            aaxFrame frame;
            aaxMtx4d mtx64;
            float dist;

            dist = INITIAL_DIST;

            /** mixer */
            res = aaxMixerSetState(config, AAX_INITIALIZED);
            testForState(res, "aaxMixerInit");

            res = aaxMixerSetState(config, AAX_PLAYING);
            testForState(res, "aaxMixerStart");

            /** scenery settings */

            /** doppler settings */
            effect = aaxEffectCreate(config, AAX_VELOCITY_EFFECT);
            testForError(effect, "Unable to create the velocity effect");

            res = aaxEffectSetParam(effect, AAX_SOUND_VELOCITY, AAX_LINEAR, 343.4f);
            testForState(res, "aaxScenerySetSoundVelocity");

            res = aaxScenerySetEffect(config, effect);
            testForState(res, "aaxScenerySetEffect");
            aaxEffectDestroy(effect);

            /** sensor settings */
            res = aaxMatrix64SetOrientation(mtx64, SensorPos,
                                               SensorAt, SensorUp);
            testForState(res, "aaxMatrix64SetOrientation");

            res = aaxMatrix64Inverse(mtx64);
            testForState(res, "aaxMatrix64Inverse");

            res = aaxSensorSetMatrix64(config, mtx64);
            testForState(res, "aaxSensorSetMatrix64");

            res = aaxSensorSetVelocity(config, SensorVel);
            testForState(res, "aaxSensorSetVelocity");

            /** emitter */
            emitter = aaxEmitterCreate();
            testForError(emitter, "Unable to create a new emitter\n");

            res = aaxEmitterAddBuffer(emitter, buffer);
            testForState(res, "aaxEmitterAddBuffer");

            res = aaxEmitterSetMode(emitter, AAX_POSITION, MODE);
            testForState(res, "aaxEmitterSetMode");

            res = aaxEmitterSetMode(emitter, AAX_LOOPING, AAX_FALSE);
            testForState(res, "aaxEmitterSetLooping");

            res = aaxEmitterSetVelocity(emitter, EmitterVel);
            testForState(res, "aaxEmitterSetVelocity");

            /* distance filter */
            filter = aaxFilterCreate(config, AAX_DISTANCE_FILTER);
            testForError(filter, "Unable to create the distance filter");

            res = aaxFilterSetParam(filter, AAX_REF_DISTANCE, AAX_LINEAR, 5000.0f);
            testForState(res, "aaxEmitterSetReferenceDistance");

            res = aaxFilterSetParam(filter, AAX_MAX_DISTANCE, AAX_LINEAR, 5000.0f);
            testForState(res, "aaxEmitterSetMaxDistance");

            res = aaxEmitterSetFilter(emitter, filter);
            testForState(res, "aaxScenerySetDistanceModel");
            aaxFilterDestroy(filter);

            if (dparam)
            {
               dist = atof(dparam);
               EmitterPos[0] = SXPOS-dist;
            }
            printf("Emitter distance: %f m\n", dist);
            res = aaxMatrix64SetDirection(mtx64, EmitterPos, EmitterDir);
            testForState(res, "aaxMatrix64SetDirection");

            res = aaxEmitterSetMatrix64(emitter, mtx64);
            testForState(res, "aaxEmitterSetMatrix64");

            if (!fparam)
            {
                res = aaxMixerRegisterEmitter(config, emitter);
                testForState(res, "aaxMixerRegisterEmitter");
            }
            else
            {
                frame = aaxAudioFrameCreate(config);
                testForError(frame, "Unable to create a new frame");

                res = aaxMixerRegisterAudioFrame(config, frame);
                testForState(res, "aaxMixerRegisterAudioFrame");

                res = aaxAudioFrameRegisterEmitter(frame, emitter);
                testForState(res, "aaxAudioFrameRegisterEmitter");

                res = aaxAudioFrameSetState(frame, AAX_PLAYING);
                testForState(res, "aaxAudioFrameSetState");
            }

            /** schedule the emitter for playback */
            printf("Thunder start\n");
            res = aaxEmitterSetState(emitter, AAX_PLAYING);
            testForState(res, "aaxEmitterStart");

            do {
                msecSleep(500);
            }
            while (aaxEmitterGetState(emitter) == AAX_PLAYING);

            res = aaxEmitterSetState(emitter, AAX_STOPPED);
            testForState(res, "aaxEmitterStop");

            /*
             * We need to wait until the emitter has been processed.
             * This is necessary because the sound could still be playing
             * due to distance delay.
             */
            while (aaxEmitterGetState(emitter) != AAX_PROCESSED) {
               msecSleep(100);
            }

            if (!fparam)
            {
                res = aaxMixerDeregisterEmitter(config, emitter);
                testForState(res, "aaxMixerDeregisterEmitter");
            }
            else
            {
            }

            res = aaxEmitterDestroy(emitter);
            testForState(res, "aaxEmitterDestroy");

            res = aaxBufferDestroy(buffer);
            testForState(res, "aaxBufferDestroy");

            res = aaxMixerSetState(config, AAX_STOPPED);
            testForState(res, "aaxMixerStop");
        }
    }

    res = aaxDriverClose(config);
    testForState(res, "aaxDriverClose");

    res = aaxDriverDestroy(config);
    testForState(res, "aaxDriverDestroy");

    return 0;
}
