/*
 * Copyright (C) 2018-2021 by Erik Hofman.
 * Copyright (C) 2018-2021 by Adalin B.V.
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

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <time.h>

#if HAVE_EBUR128
#include <ebur128.h>
#endif

#include <xml.h>
#include <aax/aax.h>

#include <base/types.h>

#include "driver.h"
#include "wavfile.h"

#if defined(WIN32)
# define TEMP_DIR		getenv("TEMP")
# define PATH_SEPARATOR		'\\'
#else    /* !WIN32 */
# define TEMP_DIR		"/tmp"
# define PATH_SEPARATOR		'/'
#endif

#define SIMPLIFY		0x01
#define NO_LAYER_SUPPORT	0x02

static char debug = 0;
static float freq = 220.0f;
static char* false_const = "false";

static char *sound_name = NULL;
static int sound_bank = 0;
static int sound_program = 0;
static float sound_frequency = 0.0f;

aaxVec3d EmitterPos = { 0.0,  0.0,  0.0  };
aaxVec3f EmitterDir = { 0.0f, 0.0f, 1.0f };

aaxVec3d SensorPos = { 0.0,  0.0,  0.0  };
aaxVec3f SensorAt = {  0.0f, 0.0f, 1.0f };
aaxVec3f SensorUp = {  0.0f, 1.0f, 0.0f };

static float _lin2log(float v) { return log10f(v); }
//  static float _log2lin(float v) { return powf(10.f,v); }
static float _lin2db(float v) { return 20.f*log10f(v); }
static float _db2lin(float v) { return _MINMAX(powf(10.f,v/20.f),0.f,10.f); }

static char* prttystr(char *s) {
    if (s) {
        char capital = 1;
        for (int i=0; s[i]; ++i) {
            if (isspace(s[i]) || s[i] == '(') capital = 1;
            else {
                if (capital) s[i] = toupper(s[i]);
                else s[i] = tolower(s[i]);
                capital = 0;
            }
        }
    }
    return s;
}

static char* lwrstr(char *s) {
    if (s) for (int i=0; s[i]; ++i) { s[i] = tolower(s[i]); }
    return s;
}

enum type_t
{
    WAVEFORM = 0,
    FILTER,
    EFFECT,
    EMITTER,
    FRAME
};

static const char* format_float3(float f)
{
    static char buf[32];

    if (f >= 100.0f) {
        snprintf(buf, 20, "%.1f", f);
    }
    else
    {
        snprintf(buf, 20, "%.3g", f);
        if (!strchr(buf, '.')) {
            strcat(buf, ".0");
        }
    }
    return buf;
}

static const char* format_float6(float f)
{
    static char buf[32];

    if (f >= 100.0f) {
        snprintf(buf, 20, "%.1f", f);
    }
    else
    {
        snprintf(buf, 20, "%.6g", f);
        if (!strchr(buf, '.')) {
            strcat(buf, ".0");
        }
    }
    return buf;
}

static float note2freq(uint8_t d) {
    return 440.0f*powf(2.0f, ((float)d-69.0f)/12.0f);
}

static uint8_t freq2note(float freq) {
    return 12.0f*log2f(freq/440.0f)+69.0f;
}

#if 0
static float note2pitch(uint8_t d, float freq) {
    return note2freq(d)/freq;
}

static uint8_t pitch2note(float pitch, float freq) {
    return freq2note(pitch*freq);
}
#endif

struct info_t
{
    char *path;

    float pan;
    int16_t program;
    int16_t bank;
    char* name;

    char *license;
    char *description;
    struct copyright_t
    {
        unsigned from, until;
        char *by;
    } copyright[2];

    struct note_t
    {
        uint8_t polyphony;
        uint8_t min, max, step;
    } note;

    int aftertouch_mode;
    float aftertouch_factor;
};

void fill_info(struct info_t *info, void *xid, const char *filename)
{
    void *xtid;
    char *ptr;

    ptr = strrchr(filename, PATH_SEPARATOR);
    if (ptr)
    {
        size_t size = ptr-filename;
        info->path = malloc(size+1);
        if (info->path)
        {
            memcpy(info->path, filename, size);
            info->path[size] = 0;
        }
    }

    info->pan = _MINMAX(xmlAttributeGetDouble(xid, "pan"), -1.0f, 1.0f);
    info->program = info->bank = -1;

    if (sound_name) {
        info->name = sound_name;
    } else {
        info->name = prttystr(xmlAttributeGetString(xid, "name"));
    }
    if (sound_bank) {
        info->bank = sound_bank;
    } else if (xmlAttributeExists(xid, "bank")) {
        info->bank = _MINMAX(xmlAttributeGetInt(xid, "bank"), 0, 127);
    }
    if (sound_program) {
        info->program = sound_program;
    } else if (xmlAttributeExists(xid, "program")) {
        info->program = _MINMAX(xmlAttributeGetInt(xid, "program"), 0, 127);
    }

    xtid = xmlNodeGet(xid, "aftertouch");
    if (xtid)
    {
        info->aftertouch_mode = xmlAttributeGetInt(xtid, "mode");
        info->aftertouch_factor = xmlAttributeGetDouble(xtid, "sensitivity");
        xmlFree(xtid);
    }

    xtid = xmlNodeGet(xid, "note");
    if (xtid)
    {
        info->note.polyphony = _MAX(xmlAttributeGetInt(xtid, "polyphony"), 0);
        info->note.min = _MINMAX(xmlAttributeGetInt(xtid, "min"), 0, 127);
        info->note.max = _MINMAX(xmlAttributeGetInt(xtid, "max"), 0, 127);
        info->note.step = _MAX(xmlAttributeGetInt(xtid, "step"), 0);
        xmlFree(xtid);
    }
    if (info->note.polyphony == 0) info->note.polyphony = 1;

    xtid = xmlNodeGet(xid, "license");
    if (xtid)
    {
        unsigned int c, cnum = xmlNodeGetNum(xid, "copyright");
        void *xcid = xmlMarkId(xtid);

        for (c=0; c<cnum; c++)
        {
            if (c == 2) break;
            if (xmlNodeGetPos(xid, xcid, "copyright", c) != 0)
            {
                info->copyright[c].from = xmlAttributeGetInt(xcid, "from");
                info->copyright[c].until = xmlAttributeGetInt(xcid, "until");
                info->copyright[c].by = xmlAttributeGetString(xcid, "by");
            }
        }
        info->license = xmlAttributeGetString(xtid, "type");
        xmlFree(xcid);
        xmlFree(xtid);
    }

    xtid = xmlNodeGet(xid, "description");
    if (xtid) {
        info->description = xmlAttributeGetString(xtid, "text");
    }
}

void print_info(struct info_t *info, FILE *output, char commons)
{
    struct tm* tm_info;
    time_t timer;
    char year[5];
    int c, i;

    time(&timer);
    tm_info = localtime(&timer);
    strftime(year, 5, "%Y", tm_info);

    fprintf(output, " <info");
    if (info->name) fprintf(output, " name=\"%s\"", info->name);
    if (info->bank >= 0) fprintf(output, " bank=\"%i\"", info->bank);
    if (info->program >= 0) fprintf(output, " program=\"%i\"", info->program);
    fprintf(output, ">\n");

    if (commons & 0x80) {
        fprintf(output, "  <license type=\"Attribution-ShareAlike 4.0 International\"/>\n");
    } else if (info->license && strcmp(info->license, "Attribution-ShareAlike 4.0 International")) {
        fprintf(output, "  <license type=\"%s\"/>\n", info->license);
    }
    else
    {
        if (commons & 0x7f) {
            fprintf(output, "  <license type=\"Attribution-ShareAlike 4.0 International\"/>\n");
        } else {
            fprintf(output, "  <license type=\"Proprietary/Commercial\"/>\n");
        }
    }

    c = 0;
    for (i=0; i<2; ++i)
    {
       if (info->copyright[i].by)
       {
           fprintf(output, "  <copyright from=\"%i\" until=\"%s\" by=\"%s\"/>\n", info->copyright[i].from, year, info->copyright[i].by);
           c++;
       }
    }
    if (c == 0)
    {
        fprintf(output, "  <copyright from=\"2017\" until=\"%s\" by=\"Erik Hofman\"/>\n", year);
        fprintf(output, "  <copyright from=\"2017\" until=\"%s\" by=\"Adalin B.V.\"/>\n", year);
    }

    if (info->description) {
        fprintf(output, "  <description text=\"%s\"/>\n", info->description);
    }

    if (info->note.polyphony)
    {
        fprintf(output, "  <note polyphony=\"%i\"", info->note.polyphony);
        if (info->note.min) fprintf(output, " min=\"%i\"", info->note.min);
        if (info->note.max) fprintf(output, " max=\"%i\"", info->note.max);
        if (info->note.step) fprintf(output, " step=\"%i\"", info->note.step);
        fprintf(output, "/>\n");
    }

    if (info->aftertouch_mode || info->aftertouch_factor) {
        fprintf(output, "  <aftertouch mode=\"%i\"", info->aftertouch_mode);
        if (info->aftertouch_factor) {
           fprintf(output, " sensitivity=\"%2.1f\"", info->aftertouch_factor);
        }
        fprintf(output, "/>\n");
    }
    fprintf(output, " </info>\n\n");
}

void free_info(struct info_t *info)
{
    int i;

    assert(info);

    if (info->path) free(info->path);
    if (!sound_name && info->name) xmlFree(info->name);
    if (info->description) xmlFree(info->description);
    if (info->license) xmlFree(info->license);
    for (i=0; i<2; ++i) {
       if (info->copyright[i].by) xmlFree(info->copyright[i].by);
    }

}

struct dsp_t
{
    enum type_t dtype;
    int eff_type;
    char *type;
    char *src;
    char *repeat;
    int stereo;
    int optional;
    float release_time;
    char env;

    uint8_t no_slots;
    struct slot_t
    {
        char *state;
        struct param_t
        {
            float value;
            float pitch;
            float adjust;
        } param[4];
    } slot[4];
};

float fill_dsp(struct dsp_t *dsp, void *xid, enum type_t t, char final, float env_fact, char simplify, char emitter, char layer)
{
    char *keep_volume = getenv("KEEP_VOLUME");
    unsigned int s, snum;
    float f, max = 0.0f;
    char distortion = 0;
    char distance = 0;
    char env = 0;
    void *xsid;

    dsp->dtype = t;
    dsp->type = lwrstr(xmlAttributeGetString(xid, "type"));
    dsp->eff_type = (t == FILTER) ? aaxFilterGetByName(NULL, dsp->type) :
                                    aaxEffectGetByName(NULL, dsp->type);
    if (!final && (!strcasecmp(dsp->type, "volume") ||
                   !strcasecmp(dsp->type, "timed-gain") ||
                   !strcasecmp(dsp->type, "dynamic-gain")))
    {
        dsp->src = false_const;
    }
    else if (!strcasecmp(dsp->type, "dynamic-timbre") &&
             (simplify & NO_LAYER_SUPPORT))
    {
        dsp->src = false_const;
    }
    else
    {
        dsp->src = lwrstr(xmlAttributeGetString(xid, "src"));
        if (!emitter && !layer && final)
        {
            if (dsp->src)
            {
               int slen = strlen(dsp->src);
               int tlen = strlen("timed");
               int itlen = strlen("inverse-timed");
               if ((slen >= tlen && !strcmp(dsp->src+slen-tlen, "timed")) ||
                  (slen >= itlen && !strcmp(dsp->src+slen-itlen, "inverse-timed")))
               {
                   printf("\033[0;31mWarning:\033[0m timed transision filters "
                          "and effects are le-shot only and therefore\n\t of "
                          "little use inside audio-frames.\n");
               }
            }

            if (!emitter && !strcasecmp(dsp->type, "frequency"))
            {
               int slen = dsp->src ? strlen(dsp->src) : 0;
               int tlen = strlen("envelope");
               int itlen = strlen("inverse-envelope");
                if (!slen ||
                   ((slen >= tlen && strcmp(dsp->src+slen-tlen, "envelope")) ||
                    (slen >= itlen && strcmp(dsp->src+slen-itlen, "inverse-envelope")))) {
                  printf("\033[0;31mWarning:\033[0m A frequency filter is "
                         "defined in an autio frame\n\t\tConsider using a one,"
                         " two or three band equalizer.\n");
               }
            }
        }
    }
    dsp->stereo = xmlAttributeGetBool(xid, "stereo");
    dsp->repeat = lwrstr(xmlAttributeGetString(xid, "repeat"));
    dsp->optional = xmlAttributeGetBool(xid, "optional");
    if (!strcasecmp(dsp->type, "timed-gain")) {
        if (xmlAttributeExists(xid, "release-time")) {
           f = _MAX(xmlAttributeGetDouble(xid, "release-time"), 0.0f);
        } else {
           f = _MAX(xmlAttributeGetDouble(xid, "release-factor")/2.5f, 0.0f);
        }
        dsp->release_time = f;
        env = 1;
    } else if (!strcasecmp(dsp->type, "distortion")) {
        distortion = 1;
    }
    else if (!strcasecmp(dsp->type, "distance")) {
       distance = 1;
    }
    dsp->env = env;

    xsid = xmlMarkId(xid);
    dsp->no_slots = snum = xmlNodeGetNum(xid, "slot");
    for (s=0; s<snum; s++)
    {
        if (xmlNodeGetPos(xid, xsid, "slot", s) != 0)
        {
            unsigned int p, pnum = xmlNodeGetNum(xsid, "param");
            void *xpid = xmlMarkId(xsid);
            int sn = s;

            if (xmlAttributeExists(xsid, "n")) {
                sn = _MINMAX(xmlAttributeGetInt(xsid, "n"), 0, 3);
            }

            dsp->slot[sn].state = lwrstr(xmlAttributeGetString(xsid, "src"));
            for (p=0; p<pnum; p++)
            {
                if (xmlNodeGetPos(xsid, xpid, "param", p) != 0)
                {
                    float adjust, value, v;
                    int pn = p;

                    if (xmlAttributeExists(xpid, "n")) {
                        pn = _MINMAX(xmlAttributeGetInt(xpid, "n"), 0, 3);
                    }

                    f = _MAX(xmlAttributeGetDouble(xpid, "pitch"), 0.0f);
                    dsp->slot[sn].param[pn].pitch = f;

                    if (dsp->slot[sn].param[pn].adjust == 0.0f) {
                       adjust = xmlAttributeGetDouble(xpid, "auto-sustain");
                    }

                    adjust = xmlAttributeGetDouble(xpid, "auto");
                    value = xmlGetDouble(xpid);
                    v = _MAX(value - adjust*_lin2log(220.0f), 0.01f);

                    if (env && (p % 2 == 0) && v > max) max = v;

                    if (simplify & SIMPLIFY)
                    {
                        if (adjust) {
                            value = v;
                        }
                        adjust = 0.0f;
                    }

                    if (!keep_volume && env && (pn % 2) == 0)
                    {
                        adjust *= env_fact;
                        value *= env_fact;
                    }

                    dsp->slot[sn].param[pn].adjust = adjust;
                    dsp->slot[sn].param[pn].value = value;
                }
            }
            xmlFree(xpid);
        }
    }

    if (!distance)
    {
// TODO: add a missing distance filter
    }

    xmlFree(xsid);

    return max;
}

void print_dsp(struct dsp_t *dsp, struct info_t *info, FILE *output, char simplify)
{
    char *ident = simplify ? "  " : "   ";
    unsigned int s, p;

    if (dsp->src == false_const) {
        return;
    }

    if (dsp->dtype == FILTER) {
        fprintf(output, "%s<filter type=\"%s\"", ident, dsp->type);
    } else {
        fprintf(output, "%s<effect type=\"%s\"", ident, dsp->type);
    }
    if (dsp->src && (strcmp(dsp->src, "12db") && strcmp(dsp->src, "true"))) {
        fprintf(output, " src=\"%s\"", dsp->src);
    }
    if (dsp->repeat) fprintf(output, " repeat=\"%s\"", dsp->repeat);
    if (dsp->stereo) fprintf(output, " stereo=\"true\"");
    if (dsp->optional) fprintf(output, " optional=\"true\"");
    if (dsp->release_time > 0.01f) {
        fprintf(output, " release-time=\"%s\"", format_float3(dsp->release_time));
    }
    fprintf(output, ">\n");

    for(s=0; s<dsp->no_slots; ++s)
    {
        if (dsp->slot[s].state) {
            fprintf(output, "%s <slot n=\"%i\" src=\"%s\">\n", ident, s, dsp->slot[s].state);
        } else {
            fprintf(output, "%s <slot n=\"%i\">\n", ident, s);
        }
        for(p=0; p<4; ++p)
        {
            float adjust = dsp->slot[s].param[p].adjust;
            float pitch = dsp->slot[s].param[p].pitch;

            fprintf(output, "%s  <param n=\"%i\"", ident, p);
            if (pitch)
            {
                fprintf(output, " pitch=\"%s\"", format_float3(pitch));
                dsp->slot[s].param[p].value = freq*pitch;
            }
            if (adjust)
            {
                fprintf(output, " auto=\"%s\"", format_float3(adjust));

                if (info->note.min && info->note.max)
                {
                    float freq1 = note2freq(info->note.min);
                    float freq2 = note2freq(info->note.max);
                    float value = dsp->slot[s].param[p].value;
                    float lin1 = _MAX(value - adjust*_lin2log(freq1), 0.01f);
                    float lin2 = _MAX(value - adjust*_lin2log(freq2), 0.01f);

                    if (dsp->env && (p % 2) == 0)
                    {
                        fprintf(output, " min=\"%s\"", format_float3(lin1));
                        fprintf(output, " max=\"%s\"", format_float3(lin2));
                    }
                    fprintf(output, ">%s</param>", format_float3(value));

                    if (debug)
                    {
                        float lin = _MAX(value - adjust*_lin2log(freq), 0.01f);

                        fprintf(output, "  <!-- %iHz: %s", (int)freq1, format_float3(lin1));
                        fprintf(output, " - %iHz: %s" , (int)freq2, format_float3(lin2));
                        fprintf(output, ", %iHz: %s -->\n", (int)freq, format_float3(lin));
                    }
                }
                else
                {
                    float value = dsp->slot[s].param[p].value;

                    fprintf(output, ">%s</param>",  format_float3(value));

                    if (debug)
                    {
                        float lin = _MAX(value - adjust*_lin2log(freq), 0.01f);
                        fprintf(output, "  <!-- %s -->", format_float3(lin));
                    }
                }
            }
            else {
                fprintf(output, ">%s</param>", format_float3(dsp->slot[s].param[p].value));
            }
            fprintf(output, "\n");
        }
        fprintf(output, "%s </slot>\n", ident);
    }

    if (dsp->dtype == FILTER) {
        fprintf(output, "%s</filter>\n", ident);
    } else {
        fprintf(output, "%s</effect>\n", ident);
    }
}

void free_dsp(struct dsp_t *dsp)
{
    if (dsp->type) xmlFree(dsp->type);
    if (dsp->repeat) xmlFree(dsp->repeat);
    if (dsp->src != false_const) xmlFree(dsp->src);
}

struct waveform_t
{
    char *src;
    char *processing;
    float ratio;
    float pitch;
    float staticity;
    float random;
    int voices;
    float spread;
    char phasing;
    float phase;
};

char fill_waveform(struct waveform_t *wave, void *xid, char simplify)
{
    wave->src = lwrstr(xmlAttributeGetString(xid, "src"));
    wave->processing = lwrstr(xmlAttributeGetString(xid, "processing"));
    wave->ratio = xmlAttributeGetDouble(xid, "ratio");
    wave->pitch = _MAX(xmlAttributeGetDouble(xid, "pitch"), 0.0f);
    wave->staticity =_MINMAX(xmlAttributeGetDouble(xid, "staticity"),0.0f,1.0f);
    wave->random =_MINMAX(xmlAttributeGetDouble(xid, "random"),0.0f,1.0f);
    wave->phase = _MINMAX(xmlAttributeGetDouble(xid, "phase"), 0.0f,1.0f);
    if (!(simplify & SIMPLIFY))
    {
        wave->voices = _MIN(abs(xmlAttributeGetInt(xid, "voices")), 9);
        wave->spread = _MINMAX(xmlAttributeGetDouble(xid, "spread"), 0.0f,1.0f);
        wave->phasing = xmlAttributeGetBool(xid, "phasing");
    }

    return wave->src ? (strstr(wave->src, "noise") ? 1 : 0) : 0;
}

void print_waveform(struct waveform_t *wave, FILE *output, char simplify)
{
    char *ident = simplify ? "  " : "   ";

    fprintf(output, "%s<waveform src=\"%s\"", ident, wave->src);
    if (wave->processing) fprintf(output, " processing=\"%s\"", wave->processing);
    if (wave->ratio) {
        if (wave->processing && !strcasecmp(wave->processing, "mix") && wave->ratio != 0.5f) {
            fprintf(output, " ratio=\"%s\"", format_float6(wave->ratio));
        } else if (wave->ratio != 1.0f) {
            fprintf(output, " ratio=\"%s\"", format_float6(wave->ratio));
        }
    }
    if (wave->pitch && wave->pitch != 1.0f) fprintf(output, " pitch=\"%s\"", format_float6(wave->pitch));
    if (wave->phase) fprintf(output, " phase=\"%s\"", format_float3(wave->phase));
    if (wave->random > 0) fprintf(output, " random=\"%s\"", format_float3(wave->random));
    if (wave->staticity > 0) fprintf(output, " staticity=\"%s\"", format_float3(wave->staticity));
    if (wave->voices > 1)
    {
        fprintf(output, " voices=\"%i\"", wave->voices);
        if (wave->spread) {
            fprintf(output, " spread=\"%s\"", format_float3(wave->spread));
            if (wave->phasing) fprintf(output, " phasing=\"true\"");
        }
    }
    fprintf(output, "/>\n");
}

void free_waveform(struct waveform_t *wave)
{
    if (wave->src) xmlFree(wave->src);
    if (wave->processing) xmlFree(wave->processing);
}

struct layer_t
{
    float ratio;
    float pitch;

    int voices;
    float spread;
    char phasing;

    uint8_t no_entries;
    struct entry_t
    {
        enum type_t type;
        union
        {
            struct waveform_t waveform;
            struct dsp_t dsp;
        } slot;
    } entry[32];
};

struct sound_t
{
    int mode;
    float gain, db;
    float frequency;
    float duration;
    int voices;
    float spread;
    char phasing;

    float loop_start;
    float loop_end;
    char *file;

    uint8_t no_layers;
    struct layer_t layer[3];
};

char fill_layers(struct sound_t *sound, void *xid, char simplify)
{
    unsigned int l, layers;
    void *xlid, *xsid;
    int p, s, smax;
    char noise;

    layers = xmlNodeGetNum(xid, "layer");
    if (layers == 0) // backwards compatibility (pre version 3.10);
    {
       layers = 1;
       xlid = xid;
    }
    else
    {
       layers = _MIN(layers, 2);
       xlid = xmlMarkId(xid);
    }

    noise = 0;
    sound->no_layers = (simplify & NO_LAYER_SUPPORT) ? 1 : layers;
    for (l=0; l<sound->no_layers; ++l)
    {
        struct layer_t *layer = &sound->layer[l];

        if (xlid != xid) {
            if (!xmlNodeGetPos(xid, xlid, "layer", l)) continue;
        }

        layer->ratio = xmlAttributeGetDouble(xlid, "ratio");
        layer->pitch= xmlAttributeGetDouble(xlid, "pitch");

        if (!(simplify & SIMPLIFY))
        {
            layer->voices = _MIN(abs(xmlAttributeGetInt(xlid, "voices")), 9);
            layer->spread = _MINMAX(xmlAttributeGetDouble(xlid, "spread"),0.0f,1.0f);
            layer->phasing = xmlAttributeGetBool(xlid, "phasing");
        }

        p = 0;
        noise = 0;
        xsid = xmlMarkId(xlid);
        smax = xmlNodeGetNum(xlid, "*");
        layer->no_entries = smax;

        for (s=0; s<smax; s++)
        {
            if (xmlNodeGetPos(xlid, xsid, "*", s) != 0)
            {
                char *name = xmlNodeGetName(xsid);
                if (!strcasecmp(name, "waveform"))
                {
                    layer->entry[p].type = WAVEFORM;
                    noise |= fill_waveform(&layer->entry[p++].slot.waveform,
                                           xsid, simplify);
                }
                else if (!strcasecmp(name, "filter"))
                {
                    layer->entry[p].type = FILTER;
                    fill_dsp(&layer->entry[p++].slot.dsp, xsid, FILTER, 1, 1.0f,
                            0, 0, 1);
                }
                else if (!strcasecmp(name, "effect"))
                {
                    layer->entry[p].type = EFFECT;
                    fill_dsp(&layer->entry[p++].slot.dsp, xsid, EFFECT, 1, 1.0f,
                             0, 0, 1);
                }
                xmlFree(name);
            }

            // Layer voices replaces sound voices
            if (layer->voices) {
                sound->voices = 0;
            }
        }
    }
    if (xlid != xid) xmlFree(xlid);

    return noise;
}

void print_layers(struct sound_t *sound, struct info_t *info, FILE *output)
{
    char simplify = sound->no_layers <= 1;
    unsigned int e, l;

    for (l=0; l<sound->no_layers; ++l)
    {
        struct layer_t *layer = &sound->layer[l];

        if (!simplify) fprintf(output, "  <layer");

        if (layer->ratio != 0.0 && layer->ratio != 1.0) {
            fprintf(output, " ratio=\"%s\"", format_float3(layer->ratio));
        }
        if (layer->pitch != 0.0  && layer->pitch != 1.0) {
            fprintf(output, " pitch=\"%s\"", format_float3(layer->pitch));
        }

        if (sound->no_layers > 1 && layer->voices > 1)
        {
            fprintf(output, " voices=\"%i\"", layer->voices);
            if (layer->spread) {
                fprintf(output, " spread=\"%s\"", format_float3(layer->spread));
                if (layer->phasing) fprintf(output, " phasing=\"true\"");
            }
        }

        if (!simplify) fprintf(output, ">\n");

        for (e=0; e<layer->no_entries; ++e)
        {
            if (layer->entry[e].type == WAVEFORM) {
                print_waveform(&layer->entry[e].slot.waveform, output, simplify);
            } else {
                print_dsp(&layer->entry[e].slot.dsp, info, output, simplify);
            }
        }
        if (!simplify) fprintf(output, "  </layer>\n");
    }
}

void fill_sound(struct sound_t *sound, struct info_t *info, void *xid, float gain, float db, char simplify, char emitter)
{
    char noise;

    if (!info->program && xmlAttributeExists(xid, "program")) {
        info->program = _MINMAX(xmlAttributeGetInt(xid, "program"), 0, 127);
    }
    if (!info->bank && xmlAttributeExists(xid, "bank")) {
        info->bank = _MINMAX(xmlAttributeGetInt(xid, "bank"), 0, 127);
    }
    if (!info->name && xmlAttributeExists(xid, "name")) {
        info->name = xmlAttributeGetString(xid, "name");
    }

    if (xmlAttributeExists(xid, "live")) {
        sound->mode = xmlAttributeGetBool(xid, "live") ? 1 : -1;
    } else if (xmlAttributeExists(xid, "mode")) {
        sound->mode = _MINMAX(xmlAttributeGetInt(xid, "mode"), 0, 2);
    }

    if (xmlAttributeExists(xid, "fixed-gain")) {
        sound->gain = -1.0*_MAX(xmlAttributeGetDouble(xid, "fixed-gain"), 0.0f);
    } else if (gain == 1.0f) {
        sound->gain = _MAX(xmlAttributeGetDouble(xid, "gain"), 0.0f);
    } else {
        sound->gain = gain;
    }
#if 0
    if (sound->db == -AAX_FPINFINITE) {
        sound->db = _MIN(xmlAttributeGetDouble(xid, "db"), 0.0f);
    } else {
        sound->db = db;
    }
#endif

    if (xmlAttributeExists(xid, "file")) {
        sound->file = xmlAttributeGetString(xid, "file");
    }
    sound->loop_start = xmlAttributeGetDouble(xid, "loop-start");
    if (xmlAttributeExists(xid, "loop-end")) {
        sound->loop_end = xmlAttributeGetDouble(xid, "loop-end");
    }

    if (sound_frequency == 0.0f) {
       sound->frequency = _MINMAX(xmlAttributeGetDouble(xid, "frequency"), 8.176f, 12543.854f);
    } else {
       sound->frequency = sound_frequency;
    }
    if (xmlAttributeGetDouble(xid, "duration")) {
        sound->duration = _MAX(xmlAttributeGetDouble(xid, "duration"), 0.0f);
    } else {
        sound->duration = 1.0f;
    }
    if (!(simplify & SIMPLIFY))
    {
        sound->voices = _MIN(abs(xmlAttributeGetInt(xid, "voices")), 9);
        sound->spread = _MINMAX(xmlAttributeGetDouble(xid, "spread"),0.0f,1.0f);
        sound->phasing = xmlAttributeGetBool(xid, "phasing");
    }

    noise = fill_layers(sound, xid, simplify);
    if (noise) sound->duration = _MAX(sound->duration, 0.3f);
}

void print_sound(struct sound_t *sound, struct info_t *info, FILE *output, char tmp, const char *section)
{
    fprintf(output, " <%s", section);
    if (sound->mode) {
        fprintf(output, " mode=\"%i\"", sound->mode);
    }

    if (sound->file)
    {
        if (tmp) {
            fprintf(output, " file=\"%s/%s\"", info->path, sound->file);
        } else {
            fprintf(output, " file=\"%s\"", sound->file);
        }
        if (sound->gain == 0.0f) {
            sound->gain = 1.0f;
        }
    }

    if (!tmp)
    {
       if (sound->gain < 0.0) {
           fprintf(output, " fixed-gain=\"%3.2f\"", -1.0*sound->gain);
       } else {
           fprintf(output, " gain=\"%3.2f\"", sound->gain);
       }
       fprintf(output, " db=\"%3.1f\"", sound->db);
    }

    if (sound->loop_start > 0) {
        fprintf(output, " loop-start=\"%g\"", sound->loop_start);
    }

    if (sound->loop_end > 0) {
        fprintf(output, " loop-start=\"%g\"", sound->loop_end);
    }

    if (sound->frequency)
    {
        uint8_t note;

        freq = sound->frequency;
        if (info->note.min && info->note.max)
        {
            note = _MINMAX(freq2note(freq), info->note.min, info->note.max);
            freq = sound->frequency = note2freq(note);
        }
        fprintf(output, " frequency=\"%g\"", sound->frequency);
    }

    if (sound->duration && sound->duration != 1.0f) {
        fprintf(output, " duration=\"%s\"", format_float3(sound->duration));
    }

    if (sound->voices > 1)
    {
        fprintf(output, " voices=\"%i\"", sound->voices);
        if (sound->spread) {
            fprintf(output, " spread=\"%s\"", format_float3(sound->spread));
            if (sound->phasing) fprintf(output, " phasing=\"true\"");
        }
    }

    if (sound->no_layers <= 1)
    {
        struct layer_t *layer = &sound->layer[0];

        if (layer->ratio != 0.0 && layer->ratio != 1.0) {
            fprintf(output, " ratio=\"%s\"", format_float3(layer->ratio));
        }
        if (layer->pitch != 0.0  && layer->pitch != 1.0) {
            fprintf(output, " pitch=\"%s\"", format_float3(layer->pitch));
        }

        if (layer->voices > 1)
        {
            fprintf(output, " voices=\"%i\"", layer->voices);
            if (layer->spread) {
                fprintf(output, " spread=\"%s\"", format_float3(layer->spread));
                if (layer->phasing) fprintf(output, " phasing=\"true\"");
            }
        }
    }

    fprintf(output, ">\n");

    print_layers(sound, info, output);

    fprintf(output, " </%s>\n\n", section);
}

void free_sound(struct sound_t *sound)
{
    if (sound->file) free(sound->file);
    assert(sound);
}

struct object_t		// emitter and audioframe
{
    char *mode;
    int looping;
    float pan;

    int freqfilter;
    int equalizer;

    uint8_t no_dsps;
    struct dsp_t dsp[16];
};

float fill_object(struct object_t *obj, void *xid, float env_fact, char final, char simplify, char emitter)
{
    unsigned int p, d, dnum;
    float max = 0.0f;
    void *xdid;

    obj->freqfilter = -1;
    obj->equalizer = -1;
    obj->mode = lwrstr(xmlAttributeGetString(xid, "mode"));
    obj->looping = xmlAttributeGetBool(xid, "looping");
    obj->pan = _MINMAX(xmlAttributeGetDouble(xid, "pan"), -1.0f, 1.0f);

    p = 0;
    xdid = xmlMarkId(xid);
    dnum = xmlNodeGetNum(xdid, "filter");
    for (d=0; d<dnum; d++)
    {
        if (xmlNodeGetPos(xid, xdid, "filter", d) != 0)
        {
            char *type = lwrstr(xmlAttributeGetString(xdid, "type"));
            if (!type) continue;

            if (!emitter && !strcasecmp(type, "frequency")) {
               obj->freqfilter = d;
            }
            if (!emitter && !strcasecmp(type, "equalizer")) {
               obj->equalizer = d;
            }

            if (!(simplify & SIMPLIFY) || !emitter
                  || strcasecmp(type, "frequency"))
            {
                float m = fill_dsp(&obj->dsp[p], xdid, FILTER, final, env_fact, simplify, emitter, 0);
                if (!max) max = m;

                int n;
                for (n=0; n < p; ++n) {
                    if (obj->dsp[n].eff_type == obj->dsp[p].eff_type &&
                        obj->dsp[n].dtype == obj->dsp[p].dtype) {
                        printf("\033[0;31mWarning:\033[0m %s filter is defined mutiple times.\n", type);
                    }
                }
                p++;
            }
            free(type);
        }
    }
    xmlFree(xdid);

    xdid = xmlMarkId(xid);
    dnum = xmlNodeGetNum(xdid, "effect");
    for (d=0; d<dnum; d++)
    {
        if (xmlNodeGetPos(xid, xdid, "effect", d) != 0)
        {
            char *type = lwrstr(xmlAttributeGetString(xdid, "type"));
            if (!type) continue;

            if (!(simplify & SIMPLIFY)
                || (!emitter && (!strcasecmp(type, "distortion")
                || !strcasecmp(type, "ringmodulator")))
                || (emitter && !strcasecmp(type, "timed-pitch")))
            {
                float m = fill_dsp(&obj->dsp[p], xdid, EFFECT, final, env_fact, simplify, emitter, 0);
                if (!max) max = m;

                int n;
                for (n=0; n < p; ++n) {
                    if (obj->dsp[n].eff_type == obj->dsp[p].eff_type &&
                        obj->dsp[n].dtype == obj->dsp[p].dtype) {
                        printf("\033[0;31mWarning:\033[0m %s effect is defined mutiple times.\n", type);
                    }
                }
                p++;
            }
            free(type);
        }
    }
    xmlFree(xdid);
    obj->no_dsps = p;

    return max;
}

void print_object(struct object_t *obj, enum type_t type, struct info_t *info, FILE *output)
{
    unsigned int d;

    if (type == FRAME)
    {
//      if (!obj->no_dsps) return;
        fprintf(output, " <audioframe");
    }
    else {
        fprintf(output, " <emitter");
    }

    if (obj->mode) fprintf(output, " mode=\"%s\"", obj->mode);
    if (obj->looping) fprintf(output, " looping=\"true\"");
    if (type == FRAME && info->pan) {
        fprintf(output, " pan=\"%s\"", format_float3(-info->pan));
    }
    if (obj->pan) fprintf(output, " pan=\"%s\"", format_float3(obj->pan));

    if (obj->no_dsps)
    {
        fprintf(output, ">\n");

        for (d=0; d<obj->no_dsps; ++d) {
            print_dsp(&obj->dsp[d], info, output, 1);
        }

        if (type == EMITTER) {
            fprintf(output, " </emitter>\n\n");
        } else {
            fprintf(output, " </audioframe>\n\n");
        }
    }
    else {
        fprintf(output, "/>\n\n");
    }
}

void free_object(struct object_t *obj)
{
    if (obj->mode) xmlFree(obj->mode);
}

struct aax_t
{
    struct info_t info;
    struct sound_t fm;
    struct sound_t sound;
    struct object_t emitter;
    struct object_t audioframe;
    char add_fm;
};

int get_info(struct aax_t *aax, const char *filename)
{
    int rv = AAX_TRUE;
    void *xid;

    memset(aax, 0, sizeof(struct aax_t));
    xid = xmlOpen(filename);
    if (xid)
    {
        void *xaid = xmlNodeGet(xid, "/aeonwave");
        if (xaid)
        {
            void *xtid = xmlNodeGet(xaid, "instrument");
            if (!xtid) xtid = xmlNodeGet(xaid, "info");
            if (xtid)
            {
                fill_info(&aax->info, xtid, filename);
                xmlFree(xtid);
            }
            xmlFree(xaid);
        }
        else
        {
            printf("%s does not seem to be AAXS compatible.\n", filename);
            rv = AAX_FALSE;
        }
        xmlClose(xid);
    }
    else
    {
        printf("%s not found.\n", filename);
        rv = AAX_FALSE;
    }
    return rv;
}

int fill_aax(struct aax_t *aax, const char *filename, char simplify, float gain, float db, float env_fact, char final)
{
    int rv = AAX_TRUE;
    void *xid;

    memset(aax, 0, sizeof(struct aax_t));
    xid = xmlOpen(filename);
    if (xid)
    {
        void *xaid = xmlNodeGet(xid, "/aeonwave");
        if (xaid)
        {
            void *xtid = xmlNodeGet(xaid, "instrument");
            if (!xtid) xtid = xmlNodeGet(xaid, "info");
            if (xtid)
            {
                fill_info(&aax->info, xtid, filename);
                xmlFree(xtid);
            }

            xtid = xmlNodeGet(xaid, "fm");
            if (xtid)
            {
                aax->add_fm = AAX_TRUE;
                fill_sound(&aax->fm, &aax->info, xtid, gain, db, 0, 1);
                xmlFree(xtid);
            }
            else {
               aax->add_fm = AAX_FALSE;
            }

            xtid = xmlNodeGet(xaid, "sound");
            if (xtid)
            {
                fill_sound(&aax->sound, &aax->info, xtid, gain, db, simplify, 1);
                xmlFree(xtid);
            }
            else
            {
                printf("The file does not contain a sound section\n");
                rv = AAX_FALSE;
            }

            xtid = xmlNodeGet(xaid, "emitter");
            if (xtid)
            {
                float m = fill_object(&aax->emitter, xtid, env_fact, final, simplify, 1);
                if (m > 0) aax->sound.db = _lin2db(1.0f/m);
                xmlFree(xtid);
            }

            xtid = xmlNodeGet(xaid, "audioframe");
            if (xtid)
            {
                float m = fill_object(&aax->audioframe, xtid, -1.f, final, simplify, 0);
                if (m < 0) aax->sound.db -= m;
                xmlFree(xtid);
            }

            xmlFree(xaid);
        }
        else
        {
            printf("%s does not seem to be AAXS compatible.\n", filename);
            rv = AAX_FALSE;
        }
        xmlClose(xid);
    }
    else
    {
        printf("%s not found.\n", filename);
        rv = AAX_FALSE;
    }
    return rv;
}

void print_aax(struct aax_t *aax, const char *outfile, char commons, char tmp)
{
    FILE *output;
    struct tm* tm_info;
    time_t timer;
    char year[5];

    time(&timer);
    tm_info = localtime(&timer);
    strftime(year, 5, "%Y", tm_info);

    if (outfile)
    {
        output = fopen(outfile, "w");
        testForError(output, "Output file could not be created.");
    }
    else {
        output = stdout;
    }

    fprintf(output, "<?xml version=\"1.0\"?>\n\n");

    fprintf(output, "<!--\n");
    fprintf(output, " * Copyright (C) 2017-%s by Erik Hofman.\n", year);
    fprintf(output, " * Copyright (C) 2017-%s by Adalin B.V.\n", year);
    fprintf(output, " * All rights reserved.\n");
    if ((!aax->info.license || (commons & 0x80) || !strcmp(aax->info.license, "Attribution-ShareAlike 4.0 International")) && commons)
    {
        fprintf(output, " *\n");
        fprintf(output, " * This file is part of AeonWave and covered by the\n");
        fprintf(output, " * Creative Commons Attribution-ShareAlike 4.0 International Public License\n");
        fprintf(output, " * https://creativecommons.org/licenses/by-sa/4.0/legalcode\n");
    }
    else
    {
        fprintf(output, " *\n");
        fprintf(output, " * This is UNPUBLISHED PROPRIETARY SOURCE CODE; the contents of this file may\n");
        fprintf(output, " * not be disclosed to third parties, copied or duplicated in any form, in\n");
        fprintf(output, " * whole or in part, without the prior written permission of the author.\n");
    }
    fprintf(output, "-->\n\n");

    fprintf(output, "<aeonwave>\n\n");
    print_info(&aax->info, output, commons);
    if (aax->add_fm) {
        print_sound(&aax->fm, &aax->info, output, tmp, "fm");
    }
    print_sound(&aax->sound, &aax->info, output, tmp, "sound");
    print_object(&aax->emitter, EMITTER, &aax->info, output);
    print_object(&aax->audioframe, FRAME, &aax->info, output);
    fprintf(output, "</aeonwave>\n");

    if (outfile) {
        fclose(output);
    }
}

void free_aax(struct aax_t *aax)
{
    free_info(&aax->info);
    free_sound(&aax->fm);
    free_sound(&aax->sound);
    free_object(&aax->emitter);
    free_object(&aax->audioframe);
}

float calculate_loudness(char *infile, struct aax_t *aax, char simplify, char commons, float *db, float *gain, float freq)
{
    char tmpfile[128], aaxsfile[128];
    char *ptr;
    float dt, step, fval;
    double loudness, peak;
    aaxBuffer buffer;
    aaxConfig config;
    aaxEmitter emitter;
    aaxFrame frame;
    aaxMtx4d mtx64;
    void **data;
    int state;
    int res;

    ptr = strrchr(infile, PATH_SEPARATOR);
    if (!ptr) ptr = infile;
    else ptr++;
    snprintf(aaxsfile, 120, "%s/%s.aaxs", TEMP_DIR, ptr);
    snprintf(tmpfile, 120, "AeonWave on Audio Files: %s/%s.wav", TEMP_DIR, ptr);

    config = aaxDriverOpenByName(tmpfile, AAX_MODE_WRITE_STEREO);
    testForError(config, "unable to open the temporary file.");

    res = aaxMixerSetSetup(config, AAX_FORMAT, AAX_FLOAT);
    testForState(res, "aaxMixerSetSetup, format");

    res = aaxMixerSetSetup(config, AAX_TRACKS, 1);
    testForState(res, "aaxMixerSetSetup, no_tracks");

    res = aaxMixerSetSetup(config, AAX_REFRESH_RATE, 90);
    testForState(res, "aaxMixerSetSetup, refresh rate");

    res = aaxMixerSetState(config, AAX_INITIALIZED);
    testForState(res, "aaxMixerInit");

    /** sensor settings */
    res = aaxMatrix64SetOrientation(mtx64, SensorPos, SensorAt, SensorUp);
    testForState(res, "aaxMatrix64SetOrientation");

    res = aaxMatrix64Inverse(mtx64);
    testForState(res, "aaaxMatrix64Inverse");
    res = aaxSensorSetMatrix64(config, mtx64);
    testForState(res, "aaxSensorSetMatrix64");

    if (fill_aax(aax, infile, simplify, 1.0f, 1.0f, -AAX_FPINFINITE, 0))
    {
        float pitch = 1.0f;
        aaxEffect effect;

        print_aax(aax, aaxsfile, commons, 1);
        *gain = aax->sound.gain;
        *db = aax->sound.db;
        free_aax(aax);

        /* buffer, defaults to processing the sound section of AAXS files */
        buffer = aaxBufferReadFromStream(config, aaxsfile);
        testForError(buffer, "Unable to create a buffer from an aaxs file.");

        float base_freq = aaxBufferGetSetup(buffer, AAX_UPDATE_RATE);
        if (freq > 0.0f) pitch = freq / base_freq;
        else freq = base_freq;

        /* emitter */
        emitter = aaxEmitterCreate();
        testForError(emitter, "Unable to create a new emitter");

        res = aaxEmitterAddBuffer(emitter, buffer);
        testForState(res, "aaxEmitterAddBuffer");

        res = aaxMatrix64SetDirection(mtx64, EmitterPos, EmitterDir);
        testForState(res, "aaxMatrix64SetDirection");

        res = aaxEmitterSetMatrix64(emitter, mtx64);
        testForState(res, "aaxEmitterSetMatrix64");

        /* pitch */
        effect = aaxEffectCreate(config, AAX_PITCH_EFFECT);
        testForError(effect, "Unable to create the pitch effect");

        res = aaxEffectSetParam(effect, AAX_PITCH, AAX_LINEAR, pitch);
        testForState(res, "aaxEffectSetParam");

        res = aaxEmitterSetEffect(emitter, effect);
        testForState(res, "aaxEmitterSetPitch");

        aaxEffectDestroy(effect);

        /* frame */
        frame = aaxAudioFrameCreate(config);
        testForError(frame, "Unable to create a new audio frame");

        res = aaxMixerRegisterAudioFrame(config, frame);
        testForState(res, "aaxMixerRegisterAudioFrame");

        res = aaxAudioFrameRegisterEmitter(frame, emitter);
        testForState(res, "aaxAudioFrameRegisterEmitter");

        res = aaxAudioFrameAddBuffer(frame, buffer);

        /* playback */
        res = aaxEmitterSetState(emitter, AAX_PLAYING);
        testForState(res, "aaxEmitterStart");

        res = aaxAudioFrameSetState(frame, AAX_PLAYING);
        testForState(res, "aaxAudioFrameStart");

        res = aaxMixerSetState(config, AAX_PLAYING);
        testForState(res, "aaxMixerStart");

        dt = 0.0f;
        step = 1.0f/aaxMixerGetSetup(config, AAX_REFRESH_RATE);
        do
        {
            aaxMixerSetState(config, AAX_UPDATE);
            dt += step;
        }
        while (dt < 2.5f && aaxEmitterGetState(emitter) == AAX_PLAYING);

        res = aaxEmitterSetState(emitter, AAX_STOPPED);
        do
        {
            aaxMixerSetState(config, AAX_UPDATE);
            msecSleep(50);
            state = aaxEmitterGetState(emitter);
        }
        while (state != AAX_PROCESSED);

        res = aaxAudioFrameSetState(frame, AAX_STOPPED);
        res = aaxAudioFrameDeregisterEmitter(frame, emitter);
        res = aaxMixerDeregisterAudioFrame(config, frame);
        res = aaxMixerSetState(config, AAX_STOPPED);
        res = aaxAudioFrameDestroy(frame);
        res = aaxEmitterDestroy(emitter);
        res = aaxBufferDestroy(buffer);

        res = aaxDriverClose(config);
        res = aaxDriverDestroy(config);

        config = aaxDriverOpenByName("None", AAX_MODE_WRITE_STEREO);
        testForError(config, "No default audio device available.");

        snprintf(tmpfile, 120, "%s/%s.wav", TEMP_DIR, ptr);
        buffer = aaxBufferReadFromStream(config, tmpfile);
        testForError(buffer, "Unable to read the buffer.");;

        peak = loudness = 0.0;
        aaxBufferSetSetup(buffer, AAX_FORMAT, AAX_FLOAT);
        data = aaxBufferGetData(buffer);
        if (data)
        {
            float *bdata = data[0];
            size_t no_samples = aaxBufferGetSetup(buffer, AAX_NO_SAMPLES);
#if HAVE_EBUR128
            size_t tracks = aaxBufferGetSetup(buffer, AAX_TRACKS);
            size_t freq = aaxBufferGetSetup(buffer, AAX_FREQUENCY);
            ebur128_state *st;

            st = ebur128_init(tracks, freq, EBUR128_MODE_I|EBUR128_MODE_SAMPLE_PEAK);
            if (st)
            {
                int res;

                ebur128_add_frames_float(st, bdata, no_samples);
                res = ebur128_loudness_global(st, &loudness);
                ebur128_sample_peak(st, 0, &peak);
                ebur128_destroy(&st);

                if (res != EBUR128_SUCCESS || loudness == -HUGE_VAL) {
                    loudness = 0.0;
                }
            }
#endif
            if (loudness == 0.0)
            {
                double rms_total = 0.0;
                size_t j = no_samples;
                do
                {
                    float samp = (float)*bdata++;
                    rms_total += samp*samp;
                }
                while (--j);

                loudness = sqrt(rms_total/no_samples);
                loudness = _lin2db(loudness);
            }
            aaxFree(data);
        }
        aaxBufferDestroy(buffer);

        aaxDriverClose(config);
        aaxDriverDestroy(config);

#if 1
        fval = 6.0f*_MAX(peak, 0.1f) * _db2lin(-24.0f - loudness);
#else
        fval = _db2lin(-16.0f - loudness);
#endif

        printf("%-42s: %4.0f Hz, R128: % -3.1f", infile, freq, loudness);
        printf(", new gain: %4.1f\n", (*gain > 0.0f) ? fval : -*gain);

        remove(aaxsfile);
        remove(tmpfile);
    }
    else
    {
        aaxDriverClose(config);
        aaxDriverDestroy(config);
        fval = 0.0f;
    }

    return fval;
}

void help()
{
    printf("aaxsstandardize version %i.%i.%i\n\n", AAX_UTILS_MAJOR_VERSION,
                                                    AAX_UTILS_MINOR_VERSION,
                                                    AAX_UTILS_MICRO_VERSION);
    printf("Usage: aaxsstandardize [options]\n");
    printf("Reads a user generated .aaxs configuration file and outputs a\n");
    printf("standardized version of the file.\n");

    printf("\nOptions:\n");
    printf(" -i, --input <file>\t\tthe .aaxs configuration file to standardize.\n");
    printf(" -o, --output <file>\t\twrite the new .aaxs configuration to this file.\n");
    printf("     --debug\t\t\tAdd some debug information to the AAXS file.\n");
    printf("     --auto-gain\t\tApply auto gain changes.\n");
    printf("     --no-layers\t\tDo not add layers.\n");
    printf("     --omit-cc-by\t\tDo not add the CC-BY license reference.\n");
    printf("     --force-cc-by\t\tForce the CC-BY license reference.\n");
    printf("     --force-simplify\t\tForce simplification of the configuration file.\n");
    printf(" -h, --help\t\t\tprint this message and exit\n");

    printf("\nWhen no output file is specified then stdout will be used.\n");

    printf("\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    char *infile, *outfile;
    char agc = 0;
    char simplify = 0;
    char commons = 1;
    char *arg;

    if (argc == 1 || getCommandLineOption(argc, argv, "-h") ||
                    getCommandLineOption(argc, argv, "--help"))
    {
        help();
    }

    sound_name = getCommandLineOption(argc, argv, "--name");
    arg = getCommandLineOption(argc, argv, "--bank");
    if (arg) sound_bank = atoi(arg);
    arg = getCommandLineOption(argc, argv, "--program");
    if (arg) sound_program = atoi(arg);
    arg = getCommandLineOption(argc, argv, "--frequency");
    if (arg) sound_frequency = atof(arg);

    if (getCommandLineOption(argc, argv, "--omit-cc-by")) {
        commons = 0;
    } else if (getCommandLineOption(argc, argv, "--force-cc-by")) {
        commons |= 0x80;
    }

    if (getCommandLineOption(argc, argv, "--auto-gain")) {
        agc = 1;
    }

    if (getCommandLineOption(argc, argv, "--no-layers")) {
        simplify |= NO_LAYER_SUPPORT;
    }
    if (getCommandLineOption(argc, argv, "--force-simplify")) {
        simplify |= (SIMPLIFY | NO_LAYER_SUPPORT);
    }

    if (getCommandLineOption(argc, argv, "--debug")) {
        debug = 1;
    }

    infile = getInputFile(argc, argv, NULL);
    outfile = getOutputFile(argc, argv, NULL);
    if (infile)
    {
        float env_fact_fm = 1.0f;
        float fval, db, gain, env_fact;
        struct aax_t aax;

#if 0
        setenv("AAX_RENDER_MODE", "synthesizer", 1);
        fval = calculate_loudness(infile, &aax, simplify, commons, &db, &gain);
        unsetenv("AAX_RENDER_MODE");

        if (fval == 0.0f) exit(-1);

        env_fact_fm = 1.0f;
        if (gain > 0.0f && fabsf(gain-fval) > 0.1f) {
            env_fact_fm = gain/fval;
        }
        env_fact_fm *= getGain(argc, argv);
#endif

        get_info(&aax, infile);
        if (aax.info.note.min && aax.info.note.max)
        {
            float freq1 = note2freq(aax.info.note.min);
            float freq2 = note2freq(aax.info.note.max);
            fval = calculate_loudness(infile, &aax, simplify, commons,
                                      &db, &gain, freq1);
            fval += calculate_loudness(infile, &aax, simplify, commons,
                                       &db, &gain, freq2);
            fval *= 0.5f;
        }
        else {
            fval = calculate_loudness(infile, &aax, simplify, commons,
                                      &db, &gain, 0.0f);
        }

        env_fact = 1.0f;
        if (agc && gain > 0.0f && fabsf(gain-fval) > 0.1f)
        {
            env_fact = gain/fval;
            gain = fval;
        }
        env_fact *= getGain(argc, argv);

        fill_aax(&aax, infile, simplify, gain, db, env_fact, 1);
        aax.fm.db = _lin2db(gain/env_fact_fm);
        print_aax(&aax, outfile, commons, 0);
        free_aax(&aax);
    }
    else {
        help();
    }

    return 0;
}
