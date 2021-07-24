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

#include <aax/midi.h>

#include <midi/instrument.hpp>
#include <midi/stream.hpp>
#include <midi/driver.hpp>

using namespace aax;

bool MIDIStream::process_GM_sysex_non_realtime(uint64_t size)
{
    bool rv = true;
    uint64_t offs = offset();
    uint8_t type, devno;
    uint8_t byte;

#if 0
 printf(" System Exclusive:");
 push_byte(); push_byte(); push_byte();
 while ((byte = pull_byte()) != MIDI_SYSTEM_EXCLUSIVE_END) printf(" %x", byte);
 printf("\n");
 byte_stream::rewind( offset() - offs);
#endif

    // GM1 reset: F0 7E 7F 09 01 F7
    // GM2 reset: F0 7E 7F 09 03 F7
    byte = pull_byte();
    CSV(", %d", byte);
    if (byte == GMMIDI_BROADCAST)
    {
        byte = pull_byte();
        CSV(", %d", byte);
        switch(byte)
        {
        case GENERAL_MIDI_SYSTEM:
            byte = pull_byte();
            CSV(", %d", byte);
            midi.set_mode(byte);
            switch(byte)
            {
            case GMMIDI_GM_RESET:
                midi.process(channel_no, MIDI_NOTE_OFF, 0, 0, true);
                midi.set_mode(MIDI_GENERAL_MIDI1);
                break;
            case 0x02:
                // midi.set_mode(MIDI_MODE0);
                break;
            case GMMIDI_GM2_RESET:
                midi.process(channel_no, MIDI_NOTE_OFF, 0, 0, true);
                midi.set_mode(MIDI_GENERAL_MIDI2);
                break;
            default:
                break;
            }
            break;
        case MIDI_EOF:
        case MIDI_WAIT:
        case MIDI_CANCEL:
        case MIDI_NAK:
        case MIDI_ACK:
            break;
        default:
            break;
        }
    }

    return rv;
}

bool MIDIStream::process_GM_sysex_realtime(uint64_t size)
{
    bool rv = true;
    uint64_t offs = offset();
    uint8_t type, devno;
    uint8_t byte;

#if 0
 printf(" System Exclusive:");
 push_byte(); push_byte(); push_byte();
 while ((byte = pull_byte()) != MIDI_SYSTEM_EXCLUSIVE_END) printf(" %x", byte);
 printf("\n");
 byte_stream::rewind( offset() - offs);
#endif

    byte = pull_byte();
    CSV(", %d", byte);
    switch(byte)
    {
    case MIDI_BROADCAST:
    {
        byte = pull_byte();
        CSV(", %d", byte);
        case MIDI_DEVICE_CONTROL:
            byte = pull_byte();
            CSV(", %d", byte);
            switch(byte)
            {
            case MIDI_DEVICE_VOLUME:
            {
                float v;
                byte = pull_byte();
                CSV(", %d", byte);
                v = (float)byte;
                byte = pull_byte();
                CSV(", %d", byte);
                v += (float)((uint16_t)(byte << 7));
                v /= (127.0f*127.0f);
                midi.set_gain(v);
                break;
            }
            case MIDI_DEVICE_BALANCE:
                byte = pull_byte();
                CSV(", %d", byte);
                midi.set_balance(((float)byte-64.0f)/64.0f);
                break;
            case MIDI_DEVICE_FINE_TUNING:
            {
                uint16_t tuning;
                float pitch;

                byte = pull_byte();
                CSV(", %d", byte);
                tuning = byte;

                byte = pull_byte();
                CSV(", %d", byte);
                tuning |= byte << 7;

                pitch = (float)tuning-8192.0f;
                if (pitch < 0) pitch /= 8192.0f;
                else pitch /= 8191.0f;
                midi.set_tuning(pitch);
                break;
            }
            case MIDI_DEVICE_COARSE_TUNING:
            {
                float pitch;

                byte = pull_byte();     // lsb, always zero
                CSV(", %d", byte);

                byte = pull_byte();     // msb
                CSV(", %d", byte);

                pitch = (float)byte-64.0f;
                if (pitch < 0) pitch /= 64.0f;
                else pitch /= 63.0f;
                midi.set_tuning(pitch);
                break;
            }
            case MIDI_GLOBAL_PARAMETER_CONTROL:
            {
                uint8_t path_len, id_width, val_width;
                uint8_t param, value;
                uint16_t slot_path;

                path_len = pull_byte();
                CSV(", %d", path_len);

                id_width = pull_byte();
                CSV(", %d", id_width);

                val_width = pull_byte();
                CSV(", %d", val_width);

                slot_path = pull_byte();
                CSV(", %d", slot_path);

                byte = pull_byte();
                slot_path |= byte << 7;
                CSV(", %d", byte);

                param =  pull_byte();
                CSV(", %d", param);

                value = pull_byte();
                CSV(", %d", value);

                switch(slot_path)
                {
                case MIDI_CHORUS_PARAMETER:
                    switch(param)
                    {
                    case 0:     // CHORUS_TYPE
                        midi.set_chorus_type(value);
                        break;
                    case 1:     // CHORUS_MOD_RATE
                        // the modulation frequency in Hz
                        midi.set_chorus_rate(0.122f*value);
                        break;
                    case 2:     // CHORUS_MOD_DEPTH
                        // the peak-to-peak swing of the modulation in ms
                        midi.set_chorus_depth(((value+1.0f)/3.2f)*1e-3f);
                        break;
                    case 3:     // CHORUS_FEEDBACK
                        // the amount of feedback from Chorus output in %
                        midi.set_chorus_feedback(0.763f*value*1e-2f);
                        break;
                    case 4:     // CHORUS_SEND_TO_REVERB
                        // the send level from Chorus to Reverb in %
                        midi.send_chorus_to_reverb(0.787f*value*1e-2f);
                        break;
                    default:
                        LOG(99, "LOG: Unsupported chorus parameter: %x\n",
                                 param);
                        break;
                    }
                    break;
                case MIDI_REVERB_PARAMETER:
                    switch(param)
                    {
                    case 0:     // Reverb Type
                        midi.set_reverb_type(value);
                        break;
                    case 1:     //Reverb Time
                        midi.set_reverb_time_rt60(expf((value-40)*0.025f));
                        break;
                    default:
                        LOG(99, "LOG: Unsupported reverb parameter: %x\n",
                                param);
                        break;
                    }
                    break;
                default:
                    break;
                }
                break;
            }
            default:
                LOG(99, "LOG: Unsupported sysex parameter: %x\n", byte);
                byte = pull_byte();
                CSV(", %d", byte);
                break;
            }
        break;
    }
    default:
        LOG(99, "LOG: Unknown sysex device id: %x\n", byte);
        break;
    }

    return rv;
}

