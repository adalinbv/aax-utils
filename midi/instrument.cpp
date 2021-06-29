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

#include <cassert>

#include <midi/file.hpp>
#include <midi/driver.hpp>

using namespace aax;

MIDIInstrument::MIDIInstrument(MIDIDriver& ptr, Buffer &buffer, uint8_t channel,
            uint16_t bank, uint8_t program, bool is_drums)
   : Instrument(ptr, channel == MIDI_DRUMS_CHANNEL), midi(ptr),
     channel_no(channel), bank_no(bank), program_no(program),
     drum_channel(channel == MIDI_DRUMS_CHANNEL ? true : is_drums)
{
    if (drum_channel && buffer) {
       Mixer::add(buffer);
    }
    Mixer::set(AAX_PLAYING);
}


std::pair<uint8_t,std::string>
MIDIInstrument::get_patch(std::string& name, uint8_t& key_no)
{
    auto patches = midi.get_patches();
    auto it = patches.find(name);
    if (it != patches.end())
    {
        auto patch = it->second.upper_bound(key_no);
        if (patch != it->second.end()) {
            return patch->second;
        }
    }

    key_no = 255;
    return {0,name};
}

void
MIDIInstrument::play(uint8_t key_no, uint8_t velocity, float pitch)
{
    assert (velocity);

    bool all = midi.no_active_tracks() > 0;
    auto it = name_map.begin();
    if (midi.channel(channel_no).is_drums())
    {
        it = name_map.find(key_no);
        if (it == name_map.end())
        {
            auto inst = midi.get_drum(program_no, key_no, all);
            std::string name = inst.first;
            if (!name.empty() && name != "")
            {
                if (!midi.buffer_avail(name))
                {
                    DISPLAY(2, "Loading drum:  %3i bank: %3i/%3i, program: %3i: %s\n",
                             key_no, bank_no >> 7, bank_no & 0x7F,
                             program_no, name.c_str());
                    midi.load(name);
                }

                if (midi.get_grep())
                {
                   auto ret = name_map.insert({key_no,nullBuffer});
                   it = ret.first;
                }
                else
                {
                    Buffer& buffer = midi.buffer(name);
                    if (buffer)
                    {
                        auto ret = name_map.insert({key_no,buffer});
                        it = ret.first;
                    }
                    else {
                        throw(std::invalid_argument("Instrument file "+name+" could not load"));
                    }
                }
            }
        }
    }
    else
    {
        uint8_t key = key_no;
        it = name_map.upper_bound(key);
        if (it == name_map.end())
        {
            auto inst = midi.get_instrument(bank_no, program_no, all);
            auto patch = get_patch(inst.first, key);
            std::string patch_name = patch.second;
            uint8_t level = patch.first;
            if (!patch_name.empty())
            {
                if (!midi.buffer_avail(patch_name)) {
                    DISPLAY(2, "Loading instrument bank: %3i/%3i, program: %3i: %s\n",
                             bank_no >> 7, bank_no & 0x7F, program_no,
                             inst.first.c_str());
                    midi.load(patch_name);
                }

                if (midi.get_grep())
                {
                   auto ret = name_map.insert({key,nullBuffer});
                   it = ret.first;
                }
                else
                {
                    Buffer& buffer = midi.buffer(patch_name, level);
                    if (buffer)
                    {
                        auto ret = name_map.insert({key,buffer});
                        it = ret.first;

                        // mode == 0: volume bend only
                        // mode == 1: pitch bend only
                        // mode == 2: volume and pitch bend
                        int pressure_mode = buffer.get(AAX_PRESSURE_MODE);
                        if (pressure_mode == 0 || pressure_mode == 2) {
                           pressure_volume_bend = true;
                        }
                        if (pressure_mode > 0) {
                           pressure_pitch_bend = true;
                        }

                        // AAX_AFTERTOUCH_SENSITIVITY == AAX_VELOCITY_FACTOR
                        pressure_sensitivity = 0.01f*buffer.get(AAX_VELOCITY_FACTOR);
                    }
                    midi.channel(channel_no).set_wide(inst.second.wide);
                    midi.channel(channel_no).set_spread(inst.second.spread);
                }
            }
        }
    }

    if (!midi.get_initialize() && it != name_map.end())
    {
        if (midi.channel(channel_no).is_drums())
        {
            switch(program_no)
            {
            case 0:	// Standard Set
            case 16:	// Power set
            case 32:	// Jazz set
            case 40:	// Brush set
                switch(key_no)
                {
                case 29: // EXC7
                    Instrument::stop(30);
                    break;
                case 30: // EXC7
                    Instrument::stop(29);
                    break;
                case 42: // EXC1
                    Instrument::stop(44);
                    Instrument::stop(46);
                    break;
                case 44: // EXC1
                    Instrument::stop(42);
                    Instrument::stop(46);
                    break;
                case 46: // EXC1
                    Instrument::stop(42);
                    Instrument::stop(44);
                    break;
                case 71: // EXC2
                    Instrument::stop(72);
                    break;
                case 72: // EXC2
                    Instrument::stop(71);
                    break;
                case 73: // EXC3
                    Instrument::stop(74);
                    break;
                case 74: // EXC3
                    Instrument::stop(73);
                    break;
                case 78: // EXC4
                    Instrument::stop(79);
                    break;
                case 79: // EXC4
                    Instrument::stop(78);
                    break;
                case 80: // EXC5
                    Instrument::stop(81);
                    break;
                case 81: // EXC5
                    Instrument::stop(80);
                    break;
                case 86: // EXC6
                    Instrument::stop(87);
                    break;
                case 87: // EXC6
                    Instrument::stop(86);
                    break;
                default:
                    break;
                }
                break;
            case 26:	// Analog Set
                switch(key_no)
                {
                case 42: // EXC1
                    Instrument::stop(44);
                    Instrument::stop(46);
                    break;
                case 44: // EXC1
                    Instrument::stop(42);
                    Instrument::stop(46);
                    break;
                case 46: // EXC1
                    Instrument::stop(42);
                    Instrument::stop(44);
                    break;
                default:
                    break;
                }
                break;
            case 48:	// Orchestra Set
                switch(key_no)
                {
                case 27: // EXC1
                    Instrument::stop(28);
                    Instrument::stop(29);
                    break;
                case 28: // EXC1
                    Instrument::stop(27);
                    Instrument::stop(29);
                    break;
                case 29: // EXC1
                    Instrument::stop(27);
                    Instrument::stop(28);
                    break;
                case 42: // EXC1
                    Instrument::stop(44);
                    Instrument::stop(46);
                    break;
                case 44: // EXC1
                    Instrument::stop(42);
                    Instrument::stop(46);
                    break;
                case 46: // EXC1
                    Instrument::stop(42);
                    Instrument::stop(44);
                    break;
                default:
                    break;
                }
                break;
            case 57:	// SFX Set
                switch(key_no)
                {
                case 41: // EXC7
                    Instrument::stop(42);
                    break;
                case 42: // EXC7
                    Instrument::stop(41);
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
        }

        Instrument::play(key_no, velocity/127.0f, it->second, pitch);
    } else {
//      throw(std::invalid_argument("Instrument file "+name+" not found"));
    }
}

