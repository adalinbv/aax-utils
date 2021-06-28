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

#include <aax/aeonwave.hpp>
#include <aax/instrument.hpp>
#include <aax/buffer_map.hpp>
#include <aax/byte_stream.hpp>
#include <aax/midi.h>

#include <aax/byte_stream.hpp>

#include <midi/shared.hpp>
#include <midi/midi.hpp>

namespace aax
{

struct param_t
{
   uint8_t coarse;
   uint8_t fine;
};

class MIDITrack : public Instrument
{
private:
    MIDITrack(const MIDITrack&) = delete;

    MIDITrack& operator=(const MIDITrack&) = delete;

public:
    MIDITrack(MIDI& ptr, Buffer &buffer, uint8_t channel,
                uint16_t bank, uint8_t program, bool is_drums);

    MIDITrack(MIDITrack&&) = default;

    virtual ~MIDITrack() = default;

    MIDITrack& operator=(MIDITrack&&) = default;

    void play(uint8_t key_no, uint8_t velocity, float pitch);

    inline void set_drums(bool d = true) { drum_channel = d; }
    inline bool is_drums() { return drum_channel; }

    inline uint16_t get_channel_no() { return channel_no; }
    inline void set_channel_no(uint16_t channel) { channel_no = channel; }

    inline uint8_t get_program_no() { return program_no; }
    inline void set_program_no(uint8_t program) { program_no = program; }

    inline uint16_t get_bank_no() { return bank_no; }
    inline void set_bank_no(uint16_t bank) { bank_no = bank; }

    inline void set_tuning(float pitch) { tuning = powf(2.0f, pitch/12.0f); }
    inline float get_tuning() { return tuning; }

    inline void set_semi_tones(float s) { semi_tones = s; }
    inline float get_semi_tones() { return semi_tones; }

    inline void set_modulation_depth(float d) { modulation_range = d; }
    inline float get_modulation_depth() { return modulation_range; }

    inline bool get_pressure_volume_bend() { return pressure_volume_bend; }
    inline bool get_pressure_pitch_bend() { return pressure_pitch_bend; }
    inline float get_aftertouch_sensitivity() { return pressure_sensitivity; }

    inline void set_track_name(std::string& tname) { track_name = tname; }

    inline Buffer& get_buffer(uint8_t key) {
        auto it = name_map.find(key);
        if (it == name_map.end()) return nullBuffer;
        return it->second;
    }

private:
    std::pair<uint8_t,std::string> get_patch(std::string& name, uint8_t& key);

    std::map<uint8_t,Buffer&> name_map;
    std::string track_name;

    MIDI &midi;

    Buffer nullBuffer;

    float tuning = 1.0f;
    float modulation_range = 2.0f;
    float pressure_sensitivity = 1.0f;
    float semi_tones = 2.0f;

    uint16_t bank_no = 0;
    uint16_t channel_no = 0;
    uint8_t program_no = 0;

    bool drum_channel = false;
    bool pressure_volume_bend = true;
    bool pressure_pitch_bend = false;
};


class MIDIStream : public byte_stream
{
public:
    MIDIStream() = default;

    MIDIStream(MIDI& ptr, byte_stream& stream, size_t len,  uint16_t track)
        : byte_stream(stream, len), midi(ptr), track_no(track)
    {
        timestamp_parts = pull_message()*24/600000;
    }

    MIDIStream(const MIDIStream&) = default;

    virtual ~MIDIStream() = default;

    void rewind();
    bool process(uint64_t, uint32_t&, uint32_t&);
    bool process_control(uint8_t);
    bool process_sysex();
    bool process_meta();

    MIDI& midi;
private:
    inline float key2pitch(MIDITrack& channel, uint16_t key) {
        auto& buffer = channel.get_buffer(key);
        float frequency = buffer.get(AAX_UPDATE_RATE);
        return 440.0f*powf(2.0f, (float(key)-69.0f)/12.0f)/frequency;
    }
    inline int16_t get_key(MIDITrack& channel, int16_t key) {
        if (!channel.is_drums()) {
            return (key-0x20) + param[MIDI_CHANNEL_COARSE_TUNING].coarse;
        }
        return key;
    }
    float get_pitch(MIDITrack& channel);
    float cents2pitch(float p, uint8_t channel);
    float cents2modulation(float p, uint8_t channel);

    inline void toUTF8(std::string& text, uint8_t c) {
       if (c < 128) {
          if (c == '\r') text += '\n';
          else text += c;
       }
       else { text += 0xc2+(c > 0xbf); text += (c & 0x3f)+0x80; }
    }

    uint32_t pull_message();
    bool registered_param(uint8_t, uint8_t, uint8_t);
    bool registered_param_3d(uint8_t, uint8_t, uint8_t);

    uint8_t mode = 0;
    uint8_t track_no = 0;
    uint16_t channel_no = 0;
    uint8_t program_no = 0;
    uint16_t bank_no = 0;

    uint8_t previous = 0;
    uint32_t wait_parts = 1;
    uint64_t timestamp_parts = 0;
    bool polyphony = true;
    bool omni = true;

    bool rpn_enabled = true;
    bool registered = false;
    uint8_t prev_controller = 0;
    uint16_t msb_type = 0;
    uint16_t lsb_type = 0;
    std::map<uint16_t,struct param_t> param = {
        {0, { 2, 0 }}, {1, { 0x40, 0 }}, {2, { 0x20, 0 }}, {3, { 0, 0 }},
        {4, { 0, 0 }}, {5, { 1, 0 }}, {6, { 0, 0 }}
    };
    std::map<uint16_t,struct param_t> param_3d;

    const std::string type_name[7] = {
        "Text", "Copyright", "Track", "Instrument", "Lyrics", "Marker", "Cue"
    };
    const std::string csv_name[9] = {
        "Sequence_number", "Text_t", "Copyright_t", "Title_t",
        "Instrument_name_t", "Lyrics_t", "Marker_t", "Cue_point_t",
        "Device_name_t"
    };
};


class MIDIFile : public MIDI
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
