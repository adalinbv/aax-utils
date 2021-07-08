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

#ifndef __AAX_MIDISTREAM
#define __AAX_MIDISTREAM

#include <map>

// #include <aax/midi.h>

#include <aax/byte_stream.hpp>

#include <midi/shared.hpp>


namespace aax
{

class MIDIDriver;

struct param_t
{
   uint8_t coarse;
   uint8_t fine;
};

class MIDIInstrument;

class MIDIStream : public byte_stream
{
public:
    MIDIStream() = default;

    MIDIStream(MIDIDriver& ptr, byte_stream& stream, size_t len,  uint16_t track)
        : byte_stream(stream, len), midi(ptr), track_no(track)
    {
        timestamp_parts = pull_message()*24/600000;
    }

    MIDIStream(const MIDIStream&) = default;

    virtual ~MIDIStream() = default;

    void rewind();
    bool process(uint64_t, uint32_t&, uint32_t&);

    MIDIDriver& midi;
private:
    float key2pitch(MIDIInstrument& channel, uint16_t key);
    int16_t get_key(MIDIInstrument& channel, int16_t key);
    float get_pitch(MIDIInstrument& channel);
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

    const std::string type_name[9] = {
        "Sequencey", "Text", "Copyright", "Title", "Instrument", "Lyrics",
        "Marker", "Cue", "Device"
    };
    const std::string csv_name[9] = {
        "Sequence_number", "Text_t", "Copyright_t", "Title_t",
        "Instrument_name_t", "Lyrics_t", "Marker_t", "Cue_point_t",
        "Device_name_t"
    };

    bool process_control(uint8_t);
    bool process_meta();
    bool process_sysex();

    bool process_GS_sysex(uint64_t);

    void display_XG_data(uint32_t, uint8_t, std::string&);
    bool process_XG_sysex(uint64_t);
    uint8_t XG_part_no[32] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
       17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    };
};

} // namespace aax


#endif // __AAX_MIDISTREAM
