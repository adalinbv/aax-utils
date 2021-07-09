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

#if 0
#include <cstring>
#include <string>
#endif

#include <aax/midi.h>

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
                switch (addr_high)
                {
                case GSMIDI_PARAMETER_CHANGE:
                    switch(addr)
                    {
                    case GSMIDI_GS_RESET:
                        byte = pull_byte();
                        CSV(", %d", byte);
                        if (byte != 0x00) break;

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
                        byte = pull_byte();
                        CSV(", %d", byte);
                        if (byte == 0x02)
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
                    case GSMIDI_REVERB_TYPE:
                    {
                        uint8_t type = pull_byte();
                        switch (type)
                        {
                        case GSMIDI_REVERB_ROOM1:
                        case GSMIDI_REVERB_ROOM2:
                        case GSMIDI_REVERB_ROOM3:
                        case GSMIDI_REVERB_HALL1:
                        case GSMIDI_REVERB_HALL2:
                        case GSMIDI_REVERB_PLATE:
                        case GSMIDI_REVERB_DELAY:
                        case GSMIDI_REVERB_PAN_DELAY:
/*
Reverb		Room1	Room2	Room3	Hall1	Hall2	Plate	Delay	PanDelay
Rev Level	64	64	64	64	64	64	64	64
Rev Character	0	1	2	3	4	5	6	7
Rev Pre-LPF	3	4	0	4	0	0	0	0
Rev Time	80	56	64	72	64	88	32	64
Rev Dly Fb	0	0	0	0	0	0	40	32
Rev PreDlyTm	0	0	0	0	0	0	0	0
*/
                        default:
                            LOG(99, "LOG: Unsupported GS reverb type: 0x%x (%d)\n",
                                type, type);
                            break;
                        }
                        break;
                    }
                    case GSMIDI_CHORUS_TYPE:
                    {
                        uint8_t type = pull_byte();
                        switch (type)
                        {
                        case GSMIDI_CHORUS1:
                        case GSMIDI_CHORUS2:
                        case GSMIDI_CHORUS3:
                        case GSMIDI_CHORUS4:
                        case GSMIDI_CHORUS_FEEDBACK:
                        case GSMIDI_FLANGER:
                        case GSMIDI_DELAY:
                        case GSMIDI_DELAY_FEEDBACK:
/*				Fb			Fb
		Chorus1	Chorus2	Chorus3	Chorus4	Chorus Flanger	SDelay	SDelay
Cho Level	64	64	64	64	64	64	64	64
Cho Pre-LPF	0	0	0	0	0	0	0	0
Cho Feedback	0	5	8	16	64	112	0	80
Cho Delay	112	80	80	64	127	127	127	12
7Cho Rate	3	9	3	9	2	1	0	0
Cho Depth	5	19	19	16	24	5	127	127
Cho To Rev	0	0	0	0	0	0	0	0
Cho To Dly	0	0	0	0	0	0	0	0
*/
                        default:
                            LOG(99, "LOG: Unsupported GS chorus type: 0x%x (%d)\n",
                                type, type);
                            break;
                        }
                        break;
                    }
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
