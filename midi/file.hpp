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

#ifndef __AAX_MIDIFILE
#define __AAX_MIDIFILE

#include <map>

#include <aax/midi.h>

#include <aax/byte_stream.hpp>

#include <midi/shared.hpp>
#include <midi/driver.hpp>

namespace aax
{

class MIDIStream;

class MIDIFile : public MIDIDriver
{
public:
    MIDIFile(const char *filename) : MIDIFile(nullptr, filename) {}

    MIDIFile(const char *devname, const char *filename, const char *track=nullptr, enum aaxRenderMode mode=AAX_MODE_WRITE_STEREO, const char *config=nullptr);

    explicit MIDIFile(std::string& devname, std::string& filename)
       :  MIDIFile(devname.c_str(), filename.c_str()) {}

    virtual ~MIDIFile() = default;

    inline operator bool() {
        return midi_data.capacity();
    }

    void initialize(const char *grep = nullptr);
    inline void start() { midi.start(); }
    inline void stop() { midi.stop(); }
    void rewind();

    inline float get_duration_sec() { return duration_sec; }
    inline float get_pos_sec() { return pos_sec; }

    bool process(uint64_t, uint32_t&);

private:
    std::string file;
    std::string gmmidi;
    std::string gmdrums;
    std::vector<uint8_t> midi_data;
    std::vector<std::shared_ptr<MIDIStream>> streams;

    uint16_t no_tracks = 0;
    float duration_sec = 0.0f;
    float pos_sec = 0.0f;

    const std::string format_name[MIDI_FILE_FORMAT_MAX+1] = {
        "MIDI File 0", "MIDI File 1", "MIDI File 2",
        "Unknown MIDI File format"
    };
    const std::string mode_name[MIDI_MODE_MAX] = {
        "MIDI", "General MIDI 1.0", "General MIDI 2.0", "GS MIDI", "XG MIDI"
    };
};

} // namespace aax


#endif // __AAX_MIDIFILE
