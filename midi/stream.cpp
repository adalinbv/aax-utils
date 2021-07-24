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

#include <string>

#include <aax/midi.h>

#include <midi/instrument.hpp>
#include <midi/stream.hpp>
#include <midi/driver.hpp>

using namespace aax;

float
MIDIStream::key2pitch(MIDIInstrument& channel, uint16_t key)
{
    auto& buffer = channel.get_buffer(key);
    float frequency = buffer.get(AAX_UPDATE_RATE);
    return 440.0f*powf(2.0f, (float(key)-69.0f)/12.0f)/frequency;
}

int16_t
MIDIStream::get_key(MIDIInstrument& channel, int16_t key)
{
    if (!channel.is_drums()) {
        return (key-0x20) + param[MIDI_CHANNEL_COARSE_TUNING].coarse;
    }
    return key;
}


float
MIDIStream::get_pitch(MIDIInstrument& channel)
{
    float pitch = 1.0f;
    if (!channel.is_drums()) {
        pitch = channel.get_tuning();
        pitch *= midi.get_tuning();
    }
    return pitch;
}

float
MIDIStream::cents2pitch(float p, uint8_t channel)
{
    float r = midi.channel(channel).get_semi_tones();
    return powf(2.0f, p*r/12.0f);
}

float
MIDIStream::cents2modulation(float p, uint8_t channel)
{
    float r = midi.channel(channel).get_modulation_depth();
    return powf(2.0f, p*r/12.0f);
}

// Variable-length quantity
uint32_t
MIDIStream::pull_message()
{
    uint32_t rv = 0;

    for (int i=0; i<4; ++i)
    {
        uint8_t byte = pull_byte();

        rv = (rv << 7) | (byte & 0x7f);
        if ((byte & 0x80) == 0) {
            break;
        }
    }

    return rv;
}


// https://www.midi.org/specifications-old/item/table-3-control-change-messages-data-bytes-2
bool
MIDIStream::registered_param(uint8_t channel, uint8_t controller, uint8_t value)
{
    uint16_t type = value;
    bool data = false;
    bool rv = true;

#if 0
 value = pull_byte();
 printf("\t1: %x %x %x %x ", 0xb0|channel, controller, type, value);
 uint8_t *p = (uint8_t*)*this;
 p += offset();
 for (int i=0; i<20; ++i) printf("%x ", p[i]);
 printf("\n");
 push_byte();
#endif

    if (controller != prev_controller)
    {
        prev_controller = controller;
        rpn_enabled = true;
    }

    switch(controller)
    {
    case MIDI_REGISTERED_PARAM_COARSE:
        msb_type = type;
        break;
    case MIDI_REGISTERED_PARAM_FINE:
        lsb_type = type;
        break;
    case MIDI_DATA_ENTRY:
        if (rpn_enabled && registered)
        {
            param[msb_type].coarse = value;
            data = true;
        }
        break;
    case MIDI_DATA_ENTRY|MIDI_FINE:
        if (rpn_enabled && registered)
        {
            param[lsb_type].fine = value;
            data = true;
        }
        break;
    case MIDI_DATA_INCREMENT:
        if (rpn_enabled)
        {
            type = msb_type << 8 | lsb_type;
            if (++param[type].fine == 128) {
                param[type].coarse++;
                param[type].fine = 0;
            }
        }
        break;
    case MIDI_DATA_DECREMENT:
        if (rpn_enabled)
        {
            type = msb_type << 8 | lsb_type;
            if (param[type].fine == 0) {
                param[type].coarse--;
                param[type].fine = 127;
            } else {
                param[type].fine--;
            }
        }
        break;
    case MIDI_UNREGISTERED_PARAM_FINE:
    case MIDI_UNREGISTERED_PARAM_COARSE:
        break;
    default:
        LOG(99, "LOG: Unsupported registered parameter controller: %x\n", controller);
        rv = false;
        break;
    }

    if (data)
    {
        type = msb_type << 8 | lsb_type;
        switch(type)
        {
        case MIDI_PITCH_BEND_SENSITIVITY:
        {
            float val;
            val = (float)param[MIDI_PITCH_BEND_SENSITIVITY].coarse +
                  (float)param[MIDI_PITCH_BEND_SENSITIVITY].fine*0.01f;
            midi.channel(channel).set_semi_tones(val);
            break;
        }
        case MIDI_MODULATION_DEPTH_RANGE:
        {
            float val;
            val = (float)param[MIDI_MODULATION_DEPTH_RANGE].coarse +
                  (float)param[MIDI_MODULATION_DEPTH_RANGE].fine*0.01f;
            midi.channel(channel).set_modulation_depth(val);
            break;
        }
        case MIDI_CHANNEL_FINE_TUNING:
        {
            uint16_t tuning = param[MIDI_CHANNEL_FINE_TUNING].coarse << 7
                              | param[MIDI_CHANNEL_FINE_TUNING].fine;
            float pitch = (float)tuning-8192.0f;
            if (pitch < 0) pitch /= 8192.0f;
            else pitch /= 8191.0f;
            midi.channel(channel).set_tuning(pitch);
            break;
        }
        case MIDI_CHANNEL_COARSE_TUNING:
            // This is handled by MIDI_NOTE_ON and MIDI_NOTE_OFF
            break;
#if AAX_PATCH_LEVEL > 210112
        case MIDI_NULL_FUNCTION_NUMBER:
            // disable the data entry, data increment, and data decrement
            // controllers until a new RPN or NRPN is selected.
            rpn_enabled = false;
            break;
        case MIDI_MPE_CONFIGURATION_MESSAGE:
#endif
        case MIDI_TUNING_PROGRAM_CHANGE:
        case MIDI_TUNING_BANK_SELECT:
            break;
        case MIDI_PARAMETER_RESET:
            midi.channel(channel).set_semi_tones(2.0f);
            break;
        default:
            LOG(99, "LOG: Unsupported registered parameter type: 0x%x/0x%x\n",
                     msb_type, lsb_type);
            break;
        }
    }

#if 0
 printf("\t9: ");
 p = (uint8_t*)*this;
 p += offset();
 for (int i=0; i<20; ++i) printf("%x ", p[i]);
 printf("\n");
#endif

    return rv;
}

void
MIDIStream::rewind()
{
    byte_stream::rewind();
    timestamp_parts = pull_message()*24/600000;
    wait_parts = 1;

    program_no = 0;
    bank_no = 0;
    previous = 0;
    polyphony = true;
    omni = true;
}

bool
MIDIStream::process(uint64_t time_offs_parts, uint32_t& elapsed_parts, uint32_t& next)
{
    bool rv = !eof();

    if (eof())
    {
        if (midi.get_format() && !track_no) return rv;
        return !midi.finished(channel_no);
    }

    if (elapsed_parts < wait_parts)
    {
        wait_parts -= elapsed_parts;
        next = wait_parts;
        return rv;
    }

    while (!eof() && (timestamp_parts <= time_offs_parts))
    {
        CSV("%d, %ld, ", track_no+1, timestamp_parts);

        // Handle running status; if the next byte is a data byte
        // reuse the last command seen in the track
        uint32_t message = pull_byte();
        if ((message & 0x80) == 0)
        {
            push_byte();
            message = previous;
        }
        else if ((message & 0xF0) != 0xF0)
        {
            // System messages and file meta-events (all of which are in the
            // 0xF0-0xFF range) are not saved, as it is possible to carry a
            // running status across them.
            previous = message;
        }

        rv = true;
        switch(message)
        {
        case MIDI_SYSTEM_EXCLUSIVE_END:
            CSV("%d", message);
            break;
        case MIDI_SYSTEM_EXCLUSIVE:
            process_sysex();
            break;
        case MIDI_FILE_META_EVENT:
            process_meta();
            break;
        default:
        {
            uint8_t channel_no = message & 0xf;
            auto& channel = midi.channel(channel_no);
            switch(message & 0xf0)
            {
            case MIDI_NOTE_ON:
            {
                int16_t key = get_key(channel, pull_byte());
                uint8_t velocity = pull_byte();
                float pitch = get_pitch(channel);
                try {
                    midi.process(channel_no, message & 0xf0, key, velocity, omni, pitch);
                } catch (const std::runtime_error &e) {
                    throw(e);
                }
                CSV("Note_on_c, %d, %d, %d\n", channel_no, key, velocity);
                break;
            }
            case MIDI_NOTE_OFF:
            {
                int16_t key = get_key(channel, pull_byte());
                uint8_t velocity = pull_byte();
                midi.process(channel_no, message & 0xf0, key, velocity, omni);
                CSV("Note_off_c, %d, %d, %d\n", channel_no, key, velocity);
                break;
            }
            case MIDI_POLYPHONIC_AFTERTOUCH:
            {
                uint8_t key = get_key(channel, pull_byte());
                uint8_t pressure = pull_byte();
                if (!channel.is_drums())
                {
                    float s = channel.get_aftertouch_sensitivity();
                    if (channel.get_pressure_pitch_bend()) {
                        channel.set_pitch(key, cents2pitch(s*pressure/127.0f, channel_no));
                    }
                    if (channel.get_pressure_volume_bend()) {
                        channel.set_pressure(key, 1.0f-0.33f*pressure/127.0f);
                    }
                }
                CSV("Poly_aftertouch_c, %d, %d, %d\n", channel_no, key, pressure);
                break;
            }
            case MIDI_CHANNEL_AFTERTOUCH:
            {
                uint8_t pressure = pull_byte();
                if (!channel.is_drums())
                {
                    float s = channel.get_aftertouch_sensitivity();
                    if (channel.get_pressure_pitch_bend()) {
                        channel.set_pitch(cents2pitch(s*pressure/127.0f, channel_no));
                    }
                    if (channel.get_pressure_volume_bend()) {
                        channel.set_pressure(1.0f-0.33f*pressure/127.0f);
                    }
                }
                CSV("Channel_aftertouch_c, %d, %d\n", channel_no, pressure);
                break;
            }
            case MIDI_PITCH_BEND:
            {
                int16_t pitch = pull_byte() | pull_byte() << 7;
                float pitch_bend = (float)pitch-8192.0f;
                if (pitch_bend < 0) pitch_bend /= 8192.0f;
                else pitch_bend /= 8191.0f;
                pitch_bend = cents2pitch(pitch_bend, channel_no);
                channel.set_pitch(pitch_bend);
                CSV("Pitch_bend_c, %d, %d\n", channel_no, pitch);
                break;
            }
            case MIDI_CONTROL_CHANGE:
            {
                process_control(channel_no);
                break;
            }
            case MIDI_PROGRAM_CHANGE:
            {
                uint8_t program_no = pull_byte();
                CSV("Program_c, %d, %d\n", channel_no, program_no);
                try {
                    midi.new_channel(channel_no, bank_no, program_no);
                } catch(const std::invalid_argument& e) {
                    ERROR("Error: " << e.what());
                }
                break;
            }
            case MIDI_SYSTEM:
                switch(channel_no)
                {
                case MIDI_TIMING_CODE:
                    pull_byte();
                    break;
                case MIDI_POSITION_POINTER:
                    pull_byte();
                    pull_byte();
                    break;
                case MIDI_SONG_SELECT:
                    pull_byte();
                    break;
                case MIDI_TUNE_REQUEST:
                    break;
                case MIDI_SYSTEM_RESET:
#if 0
                    omni = true;
                    polyphony = true;
                    for(auto& it : midi.channel())
                    {
                        midi.process(it.first, MIDI_NOTE_OFF, 0, 0, true);
                        midi.channel(channel).set_semi_tones(2.0f);
                    }
#endif
                    break;
                case MIDI_TIMING_CLOCK:
                case MIDI_START:
                case MIDI_CONTINUE:
                case MIDI_STOP:
                case MIDI_ACTIVE_SENSE:
                    break;
                default:
                    LOG(99, "LOG: Unsupported real-time System message: 0x%x - %d\n", message, channel_no);
                    break;
                }
                break;
            default:
                LOG(99, "LOG: Unsupported message: 0x%x\n", message);
                break;
            }
            break;
        } // switch
        } // default

        if (!eof())
        {
            wait_parts = pull_message();
            timestamp_parts += wait_parts;
        }
    } // while (!eof() && (timestamp_parts <= time_offs_parts))
    next = wait_parts;

    return rv;
}

bool MIDIStream::process_control(uint8_t track_no)
{
    auto& channel = midi.channel(track_no);
    bool rv = true;

    // http://midi.teragonaudio.com/tech/midispec/ctllist.htm
    uint8_t controller = pull_byte();
    uint8_t value = pull_byte();
    CSV("Control_c, %d, %d, %d\n", track_no, controller, value);
    switch(controller)
    {
    case MIDI_ALL_CONTROLLERS_OFF:
        channel.set_modulation(0.0f);
        channel.set_expression(1.0f);
        channel.set_hold(false);
//      channel.set_portamento(false);
        channel.set_sustain(false);
        channel.set_soft(false);
        channel.set_semi_tones(2.0f);
        channel.set_pitch(1.0f);
        msb_type = lsb_type = 0x7F;
        // channel.set_gain(100.0f/127.0f);
        // channel.set_pan(0.0f);
        // intentional falltrough
    case MIDI_MONO_ALL_NOTES_OFF:
        midi.process(track_no, MIDI_NOTE_OFF, 0, 0, true);
        if (value == 1) {
            mode = MIDI_MONOPHONIC;
            channel.set_monophonic(true);
        }
        break;
    case MIDI_POLY_ALL_NOTES_OFF:
        midi.process(track_no, MIDI_NOTE_OFF, 0, 0, true);
        channel.set_monophonic(false);
        mode = MIDI_POLYPHONIC;
        break;
    case MIDI_ALL_SOUND_OFF:
        midi.process(track_no, MIDI_NOTE_OFF, 0, 0, true);
        break;
    case MIDI_OMNI_OFF:
        midi.process(track_no, MIDI_NOTE_OFF, 0, 0, true);
        omni = false;
        break;
    case MIDI_OMNI_ON:
        midi.process(track_no, MIDI_NOTE_OFF, 0, 0, true);
        omni = true;
        break;
    case MIDI_BANK_SELECT:
        if (value == MIDI_BANK_RYTHM) {
            channel.set_drums(true);
        } else if (value == MIDI_BANK_MELODY) {
            channel.set_drums(false);
        }
        bank_no = (uint16_t)value << 7;
        break;
    case MIDI_BANK_SELECT|MIDI_FINE:
        bank_no += value;
        break;
    case MIDI_FOOT_CONTROLLER:
    case MIDI_BREATH_CONTROLLER:
#if 0
        if (!channel.is_drums()) {
            channel.set_pressure(1.0f-0.33f*value/127.0f);
        }
#else
        if (!channel.is_drums())
        {
            float s = channel.get_aftertouch_sensitivity();
            if (channel.get_pressure_pitch_bend()) {
                channel.set_pitch(cents2pitch(s*value/127.0f, track_no));
            }
            if (channel.get_pressure_volume_bend()) {
                channel.set_pressure(1.0f-0.33f*value/127.0f);
            }
        }
#endif
        break;
    case MIDI_BALANCE:
    case MIDI_PAN:
        if (!midi.get_mono()) {
            channel.set_pan(((float)value-64.f)/64.f);
        }
        break;
    case MIDI_EXPRESSION:
        // When Expression is at 100% then the volume represents
        // the true setting of Volume Controller. Lower values of
        // Expression begin to subtract from the volume. When
        // Expression is 0% then volume is off.
        channel.set_expression((float)value/127.0f);
        break;
    case MIDI_MODULATION_DEPTH:
    {
        float depth = (float)(value << 7)/16383.0f;
        depth = cents2modulation(depth, track_no) - 1.0f;
        channel.set_modulation(depth);
        break;
    }
    case MIDI_CELESTE_EFFECT_DEPTH:
    {
        float level = (float)value/127.0f;
        level = cents2pitch(level, track_no);
        channel.set_detune(level);
        break;
    }
    case MIDI_CHANNEL_VOLUME:
        channel.set_gain((float)value/127.0f);
        break;
    case MIDI_ALL_NOTES_OFF:
        for(auto& it : midi.channel())
        {
            midi.process(it.first, MIDI_NOTE_OFF, 0, 0, true);
            channel.set_semi_tones(2.0f);
        }
        break;
    case MIDI_UNREGISTERED_PARAM_COARSE:
    case MIDI_UNREGISTERED_PARAM_FINE:
        registered = false;
        registered_param(track_no, controller, value);
        break;
    case MIDI_REGISTERED_PARAM_COARSE:
    case MIDI_REGISTERED_PARAM_FINE:
        registered = true;
        registered_param(track_no, controller, value);
        break;
    case MIDI_DATA_ENTRY:
    case MIDI_DATA_ENTRY|MIDI_FINE:
    case MIDI_DATA_INCREMENT:
    case MIDI_DATA_DECREMENT:
        registered_param(track_no, controller, value);
        break;
    case MIDI_SOFT_PEDAL_SWITCH:
        channel.set_soft((float)value/127.0f);
        break;
    case MIDI_LEGATO_SWITCH:
#if (AAX_PATCH_LEVEL > 210516)
        channel.set_legato(value >= 0x40);
#endif
        break;
    case MIDI_DAMPER_PEDAL_SWITCH:
        channel.set_hold(value >= 0x40);
        break;
    case MIDI_SOSTENUTO_SWITCH:
        channel.set_sustain(value >= 0x40);
        break;
    case MIDI_REVERB_SEND_LEVEL:
        midi.set_reverb_level(track_no, (float)value/127.0f);
        break;
    case MIDI_CHORUS_SEND_LEVEL:
        midi.set_chorus_level(track_no, (float)value/127.0f);
        break;
    case MIDI_FILTER_RESONANCE:
    {
        float val = -1.0f+(float)value/16.0f; // relative: 0.0 - 8.0
        channel.set_filter_resonance(val);
        break;
    }
    case MIDI_CUTOFF:       // Brightness
    {
        float val = (float)value/64.0f;
        if (val < 1.0f) val = 0.5f + 0.5f*val;
        channel.set_filter_cutoff(val);
        break;
    }
    case MIDI_VIBRATO_RATE:
        channel.set_vibrato_rate(0.5f + (float)value/64.0f);
        break;
    case MIDI_VIBRATO_DEPTH:
        channel.set_vibrato_depth((float)value/64.0f);
        break;
    case MIDI_VIBRATO_DELAY:
        channel.set_vibrato_delay((float)value/64.0f);
        break;
    case MIDI_PORTAMENTO_CONTROL:
    {
        int16_t key = get_key(channel, value);
        float pitch = get_pitch(channel)*key2pitch(channel, key);
        channel.set_pitch_start(pitch);
        break;
    }
    case MIDI_PORTAMENTO_TIME:
    {
        float v = value/127.0f;
        float time = 0.0625f + 15.0f*v*(v*v*v - v*v + v);
#if AAX_PATCH_LEVEL > 210112
        channel.set_pitch_transition_time(time);
#endif
        break;
    }
    case MIDI_PORTAMENTO_TIME|MIDI_FINE:
    {
        float val = value/127.0f;
        break;
    }
    case MIDI_PORTAMENTO_SWITCH:
#if AAX_PATCH_LEVEL > 210112
        channel.set_pitch_slide_state(value >= 0x40);
#endif
        break;
    case MIDI_RELEASE_TIME:
        channel.set_release_time(value);
        break;
    case MIDI_ATTACK_TIME:
        channel.set_attack_time(value);
        break;
    case MIDI_DECAY_TIME:
        channel.set_decay_time(value);
        break;
    case MIDI_TREMOLO_EFFECT_DEPTH:
        channel.set_tremolo_depth((float)value/64.0f);
        break;
    case MIDI_PHASER_EFFECT_DEPTH:
        channel.set_phaser_depth((float)value/64.0f);
        break;
#if AAX_PATCH_LEVEL > 210112
    case MIDI_MIDI_MODULATION_VELOCITY:
        LOG(99, "LOG: Modulation Velocity control change not supported.\n");
        break;
    case MIDI_SOFT_RELEASE:
        LOG(99, "LOG: Soft Release control change not supported.\n");
        break;
#endif
    case MIDI_HOLD2:
        // lengthens the release time of the playing notes
        // Unlike the other Hold Pedal controller, this pedal
        // doesn't permanently sustain the note's sound until
        // the musician releases the pedal.
        LOG(99, "LOG: Hold 2 control change not supported.\n");
        break;
    case MIDI_PAN|MIDI_FINE:
        LOG(99, "LOG: Pan Fine control change not supported.\n");
        break;
    case MIDI_EXPRESSION|MIDI_FINE:
        LOG(99, "LOG: Expression Fine control change not supported.\n");
        break;
    case MIDI_BREATH_CONTROLLER|MIDI_FINE:
        LOG(99, "LOG: Breath Controller Fine control change not supported.\n");
        break;
    case MIDI_BALANCE|MIDI_FINE:
        LOG(99, "LOG: Balance Fine control change not supported.\n");
        break;
    case MIDI_SOUND_VARIATION:
        // Any parameter associated with the circuitry that produces
        // sound.
        LOG(99, "LOG: Sound Variation control change not supported.\n");
        break;
    case MIDI_HIGHRES_VELOCITY_PREFIX:
        LOG(99, "LOG: Highres Velocity control change not supported.\n");
        break;
    case MIDI_SOUND_CONTROL10:
    case MIDI_GENERAL_PURPOSE_CONTROL1:
    case MIDI_GENERAL_PURPOSE_CONTROL2:
    case MIDI_GENERAL_PURPOSE_CONTROL3:
    case MIDI_GENERAL_PURPOSE_CONTROL4:
    case MIDI_GENERAL_PURPOSE_CONTROL5:
    case MIDI_GENERAL_PURPOSE_CONTROL6:
    case MIDI_GENERAL_PURPOSE_CONTROL7:
    case MIDI_GENERAL_PURPOSE_CONTROL8:
        LOG(99, "LOG: Unsupported general purpose control change: 0x%x (%i), ch: %u, value: %u\n", controller, controller, track_no, value);
        break;
    default:
        LOG(99, "LOG: Unsupported unkown control change: 0x%x (%i)\n", controller, controller);
        break;
    }

    return rv;
}

bool MIDIStream::process_sysex()
{
    bool rv = true;
    uint64_t size = pull_message();
    uint64_t offs = offset();
    uint8_t byte = pull_byte();
    const char *s = nullptr;

#if 0
 printf(" System Exclusive:");
 push_byte(); push_byte(); push_byte();
 while ((byte = pull_byte()) != MIDI_SYSTEM_EXCLUSIVE_END) printf(" %x", byte);
 printf("\n");
 byte_stream::rewind( offset() - offs);
#endif
    CSV("System_exclusive, %lu, %d", size, byte);
    switch(byte)
    {
    case MIDI_SYSTEM_EXCLUSIVE_ROLAND:
        process_GS_sysex(size-2);
        break;
    case MIDI_SYSTEM_EXCLUSIVE_YAMAHA:
        process_XG_sysex(size-2);
        break;
    case MIDI_SYSTEM_EXCLUSIVE_NON_REALTIME:
        process_GM_sysex_non_realtime(size-2);
        break;
    case MIDI_SYSTEM_EXCLUSIVE_REALTIME:
        process_GM_sysex_realtime(size-2);
        break;
    case MIDI_SYSTEM_EXCLUSIVE_E_MU:
        LOG(99, "Unsupported sysex vendor: E-Mu\n");
        break;
    case MIDI_SYSTEM_EXCLUSIVE_KORG:
        LOG(99, "Unsupported sysex vendor: Korg\n");
        break;
    case MIDI_SYSTEM_EXCLUSIVE_CASIO:
        LOG(99, "Unsupported sysex vendor: Casio\n");
        break;
    default:
        LOG(99, "Unknown sysex vendor ID: %x (%i)\n", byte, byte);
        break;
    }

    size -= (offset() - offs);
    if (size)
    {
        if (midi.get_csv())
        {
            while (size--) CSV(", %d", pull_byte());
            CSV("\n");
        }
        else forward(size);
    }

    return rv;
}

bool MIDIStream::process_meta()
{
    bool rv = true;
    std::string text;
    uint8_t meta = pull_byte();
    uint64_t size = pull_message();
    uint64_t offs = offset();
    uint8_t c;
#if 0
    forward(size);
#else
    switch(meta)
    {
    case MIDI_TRACK_NAME:
    {
        auto selections = midi.get_selections();
        for (int i=0; i<size; ++i) {
           toUTF8(text, pull_byte());
        }
        MESSAGE("%-7s %2i: %s\n", type_name[meta].c_str(), track_no, text.c_str());
        CSV("%s, \"", csv_name[meta].c_str());
        CSV_TEXT("%s", text.c_str());
        CSV("\"\n");
        midi.channel(channel_no).set_track_name(text);
        if (std::find(selections.begin(), selections.end(), text) != selections.end()) {
            midi.set_track_active(track_no);
        }
        break;
    }
    case MIDI_COPYRIGHT:
    case MIDI_INSTRUMENT_NAME:
        for (int i=0; i<size; ++i) {
           toUTF8(text, pull_byte());
        }
        MESSAGE("%-10s: %s\n", type_name[meta].c_str(), text.c_str());
        CSV("%s, \"", csv_name[meta].c_str());
        CSV_TEXT("%s", text.c_str());
        CSV("\"\n");
        break;
    case MIDI_TEXT:
        for (int i=0; i<size; ++i) {
            toUTF8(text, pull_byte());
        }
        if (text.front() == '\\') {
            midi.set_lyrics(true);
        }
        if (!midi.get_lyrics()) {
            DISPLAY(3, "Text: ");
            if (size > 64) DISPLAY(4, "\n");
            DISPLAY(3, "%s\n", text.c_str());
        } else {
            if (text.front() == '\\') {
                MESSAGE("\n\n");
                midi.set_lyrics(true);
                text.front() = ' ';
             }
            else if (text.front() == '/') {
            MESSAGE("\n");
                text.front() = ' ';
            }
            MESSAGE("%s", text.c_str()); FLUSH();
            if (size > 64) MESSAGE("\n");
        }
        CSV("%s, \"", csv_name[meta].c_str());
        CSV_TEXT("%s", text.c_str());
        CSV("\"\n");
        break;
    case MIDI_LYRICS:
        midi.set_lyrics(true);
        for (int i=0; i<size; ++i) {
           toUTF8(text, pull_byte());
        }
        MESSAGE("%s", text.c_str()); FLUSH();
        CSV("%s, \"", csv_name[meta].c_str());
        CSV_TEXT("%s", text.c_str());
        CSV("\"\n");
        break;
    case MIDI_MARKER:
        for (int i=0; i<size; ++i) {
           toUTF8(text, pull_byte());
        }
        MESSAGE("%s: %s\n", type_name[meta].c_str(), text.c_str());
        CSV("%s, \"", csv_name[meta].c_str());
        CSV_TEXT("%s", text.c_str());
        CSV("\"\n");
        break;
    case MIDI_CUE_POINT:
        for (int i=0; i<size; ++i) {
           toUTF8(text, pull_byte());
        }
        MESSAGE("%s: %s", type_name[meta].c_str(), text.c_str());
        CSV("%s, \"", csv_name[meta].c_str());
        CSV_TEXT("%s", text.c_str());
        CSV("\"\n");
        break;
    case MIDI_DEVICE_NAME:
        for (int i=0; i<size; ++i) {
           toUTF8(text, pull_byte());
        }
        MESSAGE("%s", text.c_str());
        CSV("%s, \"", csv_name[meta].c_str());
        CSV_TEXT("%s", text.c_str());
        CSV("\"\n");
        break;
    case MIDI_CHANNEL_PREFIX:
        c = pull_byte();
        channel_no = (channel_no & 0xFF00) | c;
        CSV("%s, %d\n", "Channel_prefix", c);
        break;
    case MIDI_PORT_PREFERENCE:
        c = pull_byte();
        channel_no = (channel_no & 0xFF) | c << 16;
        CSV("%s, %d\n", "MIDI_port", c);
        break;
    case MIDI_END_OF_TRACK:
        CSV("%s\n", "End_track");
        forward();
        break;
    case MIDI_SET_TEMPO:
    {
        uint32_t tempo;
        tempo = (pull_byte() << 16) | (pull_byte() << 8) | pull_byte();
        midi.set_tempo(tempo);
        CSV("%s, %d\n", "Tempo", tempo);
        break;
    }
    case MIDI_SEQUENCE_NUMBER:        // sequencer software only
    {
        uint8_t mm = pull_byte();
        uint8_t ll = pull_byte();
        CSV("%s, %d\n", csv_name[meta].c_str(), (mm << 8) | ll);
        break;
    }
    case MIDI_TIME_SIGNATURE:
    {   // the signature as notated on sheet music.
        uint8_t nn = pull_byte();
        uint8_t dd = pull_byte();
        uint8_t cc = pull_byte(); // 1 << cc
        uint8_t bb = pull_byte();
        uint16_t QN = 100000.0f / (float)cc;
        CSV("%s, %d, %d, %d, %d\n", "Time_signature",
                                    nn, dd, cc, bb);
        break;
    }
    case MIDI_SMPTE_OFFSET:
    {
        uint8_t hr = pull_byte(); // 0sshhhhha: ss is the frame rate
        uint8_t mn = pull_byte(); // ss = 00: 24 frames per second
        uint8_t se = pull_byte(); // ss = 01: 25 frames per second
        uint8_t fr = pull_byte(); // ss = 10: 29.97 frames per second
        uint8_t ff = pull_byte(); // ss = 11: 30 frames per second
        CSV( "%s, %d, %d, %d, %d, %d\n", "SMPTE_offset",
                                         hr, mn, se, fr, ff);
        break;
    }
    case MIDI_KEY_SIGNATURE:
    {
        int8_t sf = pull_byte();
        uint8_t mi = pull_byte();
        CSV("%s, %d, \"%s\"\n", "Key_signature",
                                sf, mi ? "minor" : "major");
        break;
    }
    case MIDI_SEQUENCERSPECIFICMETAEVENT:
        for (int i=0; i<size; ++i) {
           text += pull_byte();
        }
        CSV("%s, %lu", "Sequencer_specific", size);
        for (int i=0; i<size; ++i) {
            CSV(", %d", text[i]);
        }
        CSV("\n");
        break;
    default:        // unsupported
        for (int i=0; i<size; ++i) {
           text += pull_byte();
        }
        CSV("%s, %d, %lu", "Unknown_meta_event", meta, size);
        for (int i=0; i<size; ++i) {
            CSV(", %d", text[i]);
        }
        CSV("\n");
        LOG(99, "LOG: Unknown.meta.event: %i (0x%x)\n", meta, meta);
        break;
    }

    if (meta != MIDI_END_OF_TRACK) {
        size -= (offset() - offs);
        if (size) forward(size);
    }
#endif

    return rv;
}

