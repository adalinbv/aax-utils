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

#include <cstring>
#include <cassert>

#include <fstream>

#include <aax/strings.hpp>

#include <midi/stream.hpp>
#include <midi/file.hpp>

using namespace aax;

MIDIFile::MIDIFile(const char *devname, const char *filename,
                   const char *selection, enum aaxRenderMode mode,
                   const char *config)
    : MIDIDriver(devname, selection, mode), file(filename)
{
    struct stat info;
    if (stat(filename, &info) != 0 || (info.st_mode & S_IFDIR)) {
        ERROR("Error opening: " << filename);
        return;
    }

    std::ifstream file(filename, std::ios::in|std::ios::binary|std::ios::ate);
    ssize_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (config)
    {
        static const char *prefix = "gmmidi-";
        static const char *ext = ".xml";
        gmmidi = config;

        if (gmmidi.compare(0, strlen(prefix), prefix)) {
            gmmidi.insert(0, prefix);
        }
        if (gmmidi.compare(gmmidi.length()-strlen(ext), strlen(ext), ext)) {
            gmmidi.append(ext);
        }
    }

    if (size > 0)
    {
        midi_data.reserve(size);
        if (midi_data.capacity() == size)
        {
            std::streamsize fileSize = size;
            if (file.read((char*)midi_data.data(), fileSize))
            {
                buffer_map<uint8_t> map(midi_data.data(), size);
                byte_stream stream(map);

                try
                {
                    uint32_t size, header = stream.pull_long();
                    uint16_t format, track_no = 0;
                    uint16_t PPQN = 0;

                    if (header == 0x4d546864) // "MThd"
                    {
                        size = stream.pull_long();
                        if (size != 6) return;

                        format = stream.pull_word();
                        if (format != 0 && format != 1) return;

                        no_tracks = stream.pull_word();
                        if (format == 0 && no_tracks != 1) return;

                        midi.set_format(format);

                        PPQN = stream.pull_word();
                        if (PPQN & 0x8000) // SMPTE
                        {
                            uint8_t fps = (PPQN >> 8) & 0xff;
                            uint8_t resolution = PPQN & 0xff;
                            if (fps == 232) fps = 24;
                            else if (fps == 231) fps = 25;
                            else if (fps == 227) fps = 29;
                            else if (fps == 226) fps = 30;
                            else fps = 0;
                            PPQN = fps*resolution;
                        }
                        midi.set_ppqn(PPQN);
                    }

                    while (stream.remaining() > sizeof(header))
                    {
                        header = stream.pull_long();
                        if (header == 0x4d54726b) // "MTrk"
                        {
                            uint32_t length = stream.pull_long();
                            if (length >= sizeof(uint32_t) &&
                                length <= stream.remaining())
                            {
                                streams.push_back(std::shared_ptr<MIDIStream>(
                                                   new MIDIStream(*this, stream,
                                                         length, track_no++)));
                                stream.forward(length);
                            }
                        }
                        else {
                            break;
                        }
                    }
                    no_tracks = track_no;

                    PRINT_CSV("0, 0, Header, 0, %d, %d\n", no_tracks, PPQN);
                    for (track_no=0; track_no<no_tracks; ++track_no) {
                        PRINT_CSV("%d, 0, Start_track\n", track_no+1);
                    }
                } catch (const std::overflow_error& e) {
                    ERROR("Error while processing the MIDI file: "
                              << e.what());
                }
            }
            else {
                ERROR("Error: Unable to open: " << filename);
            }
        }
        else if (!midi_data.size()) {
            ERROR("Error: Out of memory.");
        }
    }
    else {
        ERROR("Error: Unable to open: " << filename);
    }
}

void
MIDIFile::initialize(const char *grep)
{
    char *env = getenv("AAX_MIDI_MODE");
    double eps;
    clock_t t;

    if (env)
    {
        if (!strcasecmp(env, "synthesizer")) {
            midi.set_capabilities(AAX_RENDER_SYNTHESIZER);
        } else if (!strcasecmp(env, "arcade")) {
            midi.set_capabilities(AAX_RENDER_ARCADE);
        }
    }

    if (!gmmidi.empty() || gmdrums.empty()) {
       midi.read_instruments(gmmidi, gmdrums);
    }
    midi.read_instruments();

    midi.set_grep(grep);
    midi.set_initialize(true);
    duration_sec = 0.0f;

    uint64_t time_parts = 0;
    uint32_t wait_parts = 1000000;
    t = clock();
    try {
        while (process(time_parts, wait_parts))
        {
            time_parts += wait_parts;
            duration_sec += wait_parts*midi.get_uspp()*1e-6f;
        }
    } catch (const std::runtime_error &e) {
       throw(e);
    }
    eps = (double)(clock() - t)/ CLOCKS_PER_SEC;

    midi.set_initialize(false);

    if (!grep)
    {
        char *rrate = getenv("AAX_MIDI_REFRESH_RATE");
        rewind();
        pos_sec = 0;

        float refrate;
        if (rrate) refrate = atof(rrate);
        else if (midi.get_refresh_rate() > 0.0f) refrate = midi.get_refresh_rate();
        else if (simd64 && cores >=4) refrate = 90.0f;
        else if (simd && cores >= 4) refrate = 60.0f;
        else refrate = 45.0f;

        midi.set(AAX_REFRESH_RATE, refrate);
        if (midi.get_polyphony() < UINT_MAX) {
            midi.set(AAX_MONO_EMITTERS, midi.get_polyphony());
        }

        midi.set(AAX_INITIALIZED);
        if (midi.get_effects().length())
        {
           Buffer &buffer = midi.buffer(midi.get_effects(), 0);
           Sensor::add(buffer);
        }

        if (midi.get_verbose())
        {

            MESSAGE("Frequency : %i Hz\n", midi.get(AAX_FREQUENCY));
            MESSAGE("Upd. rate : %i Hz\n", midi.get(AAX_REFRESH_RATE));
            MESSAGE("Init time : %.1f ms\n", eps*1000.0f);

            unsigned int polyphony = midi.get(AAX_MONO_EMITTERS);
            if (polyphony == UINT_MAX) {
                MESSAGE("Polyphony : unlimited\n");
            } else {
                MESSAGE("Polyphony : %u\n", midi.get(AAX_MONO_EMITTERS));
            }

            enum aaxRenderMode render_mode = aaxRenderMode(midi.render_mode());
            std::string r = "Rendering : ";
            if (cores < 4 || !simd) {
                r += " mono playback";
                midi.set_mono(true);
            } else {
                r += to_string(render_mode);
            }
            if (midi_mode) {
                r += ", ";
                r += to_string(aaxCapabilities(midi_mode));
            }
            MESSAGE("Rendering : %s\n", r.c_str());
            MESSAGE("Patch set : %s", midi.get_patch_set().c_str());
            MESSAGE(" instrument set version %s\n", midi.get_patch_version().c_str());
            MESSAGE("Directory : %s\n", midi.info(AAX_SHARED_DATA_DIR));

            int hour, minutes, seconds;
            unsigned int format = midi.get_format();
            if (format >= MIDI_FILE_FORMAT_MAX) format = MIDI_FILE_FORMAT_MAX;
            MESSAGE("Format    : %s\n", format_name[format].c_str());

            unsigned int mode = midi.get_mode();
            assert(mode < MIDI_MODE_MAX);
            MESSAGE("MIDI Mode : %s\n", mode_name[mode].c_str());

            seconds = duration_sec;
            hour = seconds/(60*60);
            seconds -= hour*60*60;
            minutes = seconds/60;
            seconds -= minutes*60;
            if (hour) {
                MESSAGE("Duration  : %02i:%02i:%02i hours\n", hour, minutes, seconds);
            } else {
                MESSAGE("Duration  : %02i:%02i minutes\n", minutes, seconds);
            }
        }
    }
    else {
        midi.grep(file, grep);
    }
}

void
MIDIFile::rewind()
{
    midi.rewind();
    midi.set_lyrics(false);
    for (auto it : streams) {
        it->rewind();
    }
}

bool
MIDIFile::process(uint64_t time_parts, uint32_t& next)
{
    uint32_t elapsed_parts = next;
    uint32_t wait_parts;
    bool rv = false;

    next = UINT_MAX;
    for (size_t t=0; t<no_tracks; ++t)
    {
        wait_parts = next;

        try {
            bool res = streams[t]->process(time_parts, elapsed_parts, wait_parts);
            if (t || !midi.get_format()) rv |= res;
        } catch (const std::runtime_error &e) {
            throw(e);
            break;
        }

        if (next > wait_parts) {
            next = wait_parts;
        }
    }

    if (next == UINT_MAX) {
        next = 100;
    }

    if (midi.get_verbose() && !midi.get_lyrics())
    {
        int hour, minutes, seconds;

        pos_sec += elapsed_parts*midi.get_uspp()*1e-6f;

        seconds = pos_sec;
        hour = seconds/(60*60);
        seconds -= hour*60*60;
        minutes = seconds/60;
        seconds -= minutes*60;
        if (hour) {
            MESSAGE("pos: %02i:%02i:%02i hours\r", hour, minutes, seconds);
        } else {
            MESSAGE("pos: %02i:%02i minutes\r", minutes, seconds);
        }
        if (!rv) MESSAGE("\n\n");
        fflush(stdout);
    }

    if (!rv) CSV("0, 0, End_of_file\n");

    return rv;
}
