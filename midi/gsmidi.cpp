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

#include <midi/instrument.hpp>
#include <midi/stream.hpp>
#include <midi/driver.hpp>


using namespace aax;



bool MIDIStream::process_GS_sysex(uint64_t size)
{
    bool rv = false;
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

    type = pull_byte();
    CSV(", %d", type);
    devno = type & 0xF;
    switch (type & 0xF0)
    {
    case GSMIDI_SYSTEM:
        byte = pull_byte();
        CSV(", %d", byte);
        switch (byte)
        {
        case GSMIDI_MODEL_GS:
            byte = pull_byte();
            CSV(", %d", byte);
            switch (byte)
            {
            case GSMIDI_DATA_SET1:
            {
                uint8_t addr_high = pull_byte();
                uint8_t addr_mid = pull_byte();
                uint8_t addr_low = pull_byte();
                uint16_t addr = addr_mid << 8 | addr_low;
                uint8_t value = pull_byte();
                CSV(", %d", value);
                switch (addr_high)
                {
                case GSMIDI_PARAMETER_CHANGE:
                    switch(addr)
                    {
                    case GSMIDI_GS_RESET:
                        if (value != 0x00) break;

                        byte = pull_byte();
                        CSV(", %d", byte);
                        if (byte == 0x41) // checksum
                        {
                            midi.set_mode(MIDI_GENERAL_STANDARD);
                            rv = true;
                        }
                        break;
                    case GSMIDI_DRUM_PART1:
                    case GSMIDI_DRUM_PART2:
                    case GSMIDI_DRUM_PART3:
                    case GSMIDI_DRUM_PART4:
                    case GSMIDI_DRUM_PART5:
                    case GSMIDI_DRUM_PART6:
                    case GSMIDI_DRUM_PART7:
                    case GSMIDI_DRUM_PART8:
                    case GSMIDI_DRUM_PART9:
                    case GSMIDI_DRUM_PART11:
                    case GSMIDI_DRUM_PART12:
                    case GSMIDI_DRUM_PART13:
                    case GSMIDI_DRUM_PART14:
                    case GSMIDI_DRUM_PART15:
                    case GSMIDI_DRUM_PART16:
                    {
                        uint8_t part_no = addr_mid & 0xf;
                        if (value == 0x02)
                        {
                            byte = pull_byte();
                            CSV(",%d", byte); 
                            if (byte == 0x10) {
                                midi.channel(part_no).set_drums(true);
                                rv = true;
                            }
                        }
                        break;
                    }
                    case GSMIDI_REVERB_MACRO:
                    case GSMIDI_REVERB_CHARACTER:
                        switch (value)
                        {
                        case GSMIDI_REVERB_ROOM1:
                            midi.set_reverb("reverb/room-small");
                            INFO("Switching to Small Room reveberation");
                            break;
                        case GSMIDI_REVERB_ROOM2:
                            midi.set_reverb("reverb/room-medium");
                            INFO("Switching to Medium Room reveberation");
                            break;
                        case GSMIDI_REVERB_ROOM3:
                            midi.set_reverb("reverb/room-large");
                            INFO("Switching to Large Room reveberation");
                            break;
                        case GSMIDI_REVERB_HALL1:
                            midi.set_reverb("reverb/concerthall");
                            INFO("Switching to Concert Hall Reveberation");
                            break;
                        case GSMIDI_REVERB_HALL2:
                            midi.set_reverb("reverb/concerthall-large");
                            INFO("Switching to Large Concert Hall reveberation");
                            break;
                        case GSMIDI_REVERB_PLATE:
                            midi.set_reverb("reverb/plate");
                            INFO("Switching to Plate reveberation");
                            break;
                        case GSMIDI_REVERB_DELAY:
                        case GSMIDI_REVERB_PAN_DELAY:
                        default:
                            LOG(99, "LOG: Unsupported reverb type: 0x%x (%d)\n",
                                    type, type);
                            break;
                        }
                        break;
                    case GSMIDI_REVERB_PRE_LPF:
                    {
                        float val = value/7.0f;
                        float fc = 22000.0f - _log2lin(val*_lin2log(22000.0f));
                        midi.set_reverb_cutoff_frequency(fc);
                        break;
                    }
                    case GSMIDI_REVERB_LEVEL:
                    {
                        float val = (float)value/127.0f;
                        midi.set_reverb_level(track_no, val);
                        break;
                    }
                    case GSMIDI_REVERB_TIME:
                    {
                        float reverb_time = 0.7f*value/127.0f;
                        midi.set_reverb_time_rt60(reverb_time);
                        break;
                    }
                    case GSMIDI_REVERB_DELAY_FEEDBACK:
                        LOG(99, "LOG: Unsupported Reverb Delay Feedback\n");
                        break;
                    case GSMIDI_CHORUS_MACRO:
                        switch (value)
                        {
                        case GSMIDI_CHORUS1:
                            midi.set_chorus("chorus/chorus1");
                            INFO("Switching to GS type 1 chorus");
                            break;
                        case GSMIDI_CHORUS2:
                            midi.set_chorus("chorus/chorus2");
                            INFO("Switching to GS type 2 chorus");
                            break;
                        case GSMIDI_CHORUS3:
                            midi.set_chorus("chorus/chorus3");
                            INFO("Switching to GS type 3 chorus");
                            break;
                        case GSMIDI_CHORUS4:
                            midi.set_chorus("chorus/chorus4");
                            INFO("Switching to GS type 4 chorus");
                            break;
                        case GSMIDI_FEEDBACK_CHORUS:
                            midi.set_chorus("chorus/chorus_freedback");
                            INFO("Switching to GS feedback chorus");
                            break;
                        case GSMIDI_FLANGER:
                            midi.set_chorus("chorus/flanger");
                            INFO("Switching to GS flanging");
                            break;
                        case GSMIDI_DELAY:
                            midi.set_chorus("chorus/delay");
                            INFO("Switching to GS short delay");
                            break;
                        case GSMIDI_DELAY_FEEDBACK:
                            midi.set_chorus("chorus/delay_feedback");
                            INFO("Switching to GS short delay with feedback");
                            break;
                        default:
                            LOG(99, "LOG: Unsupported GS chorus type: 0x%x (%d)\n",
                                type, type);
                            break;
                        }
                        break;
                    case GSMIDI_CHORUS_PRE_LPF:
                        LOG(99, "LOG: Unsupported GS Chorus Pre-LPF\n");
                        break;
                    case GSMIDI_CHORUS_LEVEL:
                    {
                        float val = (float)value/127.0f;
                        midi.set_chorus_level(track_no, val);
                        break;
                    }
                    case GSMIDI_CHORUS_FEEDBACK:
                        midi.set_chorus_feedback(0.763f*value*1e-2f);
                        break;
                    case GSMIDI_CHORUS_DELAY:
                        LOG(99, "LOG: Unsupported GS Chorus Delay\n");
                        break;
                    case GSMIDI_CHORUS_RATE:
                    {
                        float val = (float)value/127.0f;
                        midi.set_chorus_rate(val);
                        break;
                    }
                    case GSMIDI_CHORUS_DEPTH:
                    {
                        float val = (float)value/127.0f;
                        midi.set_chorus_depth(val);
                        break;
                    }
                    case GSMIDI_CHORUS_SEND_LEVEL_TO_REVERB:
                        midi.send_chorus_to_reverb(value/127.0f);
                        break;
                    default:
                        LOG(99, "LOG: Unsupported GS address: 0x%x 0x%x (%d %d)\n",
                                addr_mid, addr_low, addr_mid, addr_low);
                        break;
                    }
                    break;
                default:
                    LOG(99, "LOG: Unsupported GS sysex effect type: 0x%x (%d)\n",
                            addr_high, addr_high);
                   break;
                }
                break;
            }
            case GSMIDI_DATA_REQUEST1:
            default:
                LOG(99, "LOG: Unsupported GS sysex parameter category: 0x%x (%d)\n",
                     byte, byte);
                break;
            }
            break;
        default:
            LOG(99, "LOG: Unsupported GS sysex Model ID: 0x%x (%d)\n",
                     byte, byte);
            break;
        }
        break;
    default:
        LOG(99, "LOG: Unsupported GS category type: 0x%x (%d)\n", byte, byte);
        break;
    }

    return rv;
};
