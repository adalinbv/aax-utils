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

#ifndef __AAX_MIDIINSTRUMENT
#define __AAX_MIDIINSTRUMENT

#include <map>

#include <aax/instrument.hpp>

namespace aax
{

class MIDIDriver;

class MIDIInstrument : public Instrument
{
private:
    MIDIInstrument(const MIDIInstrument&) = delete;

    MIDIInstrument& operator=(const MIDIInstrument&) = delete;

public:
    MIDIInstrument(MIDIDriver& ptr, Buffer &buffer, uint8_t channel,
                uint16_t bank, uint8_t program, bool is_drums);

    MIDIInstrument(MIDIInstrument&&) = default;

    virtual ~MIDIInstrument() = default;

    MIDIInstrument& operator=(MIDIInstrument&&) = default;

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

    MIDIDriver &midi;

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

} // namespace aax


#endif // __AAX_MIDIINSTRUMENT
