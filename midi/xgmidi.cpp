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

#include <midi/stream.hpp>
#include <midi/driver.hpp>

using namespace aax;

bool MIDIStream::process_XG_sysex(uint64_t size)
{
    bool rv = false;
    uint64_t offs = offset();
    uint8_t type, devno;
    uint8_t byte;

    type = pull_byte();
    CSV(", %d", type);
    devno = type & 0xF;
    switch (type & 0xF0)
    {
    case XGMIDI_BULK_DUMP:
        break;
    case XGMIDI_PARAMETER_CHANGE:
        byte = pull_byte();
        CSV(", %d", byte);
        switch(byte)
        {
        case XGMIDI_MODEL_ID:
        {
            uint8_t addr_high = pull_byte();
            uint8_t addr_mid = pull_byte();
            uint8_t addr_low = pull_byte();
            uint16_t addr = addr_mid << 8 | addr_low;
            CSV(", %d, %d, %d", addr_high, addr_mid, addr_low);
            switch (addr_high)
            {
            case XGMIDI_SYSTEM:
                if (addr == XGMIDI_SYSTEM_ON)
                {
                    byte = pull_byte();
                    CSV(", %d", byte);
                    if (byte == 0x00) {
                        midi.set_mode(MIDI_EXTENDED_GENERAL_MIDI);
                    }
                    rv = true;
                }
                break;
            case XGMIDI_EFFECT1:
                switch (addr)
                {
                case XGMIDI_REVERB_TYPE:
                {
                    uint16_t type = pull_byte() << 8 | pull_byte();
                    switch (type)
                    {
                    case XGMIDI_REVERB_HALL1:
                        midi.set_reverb("reverb/concerthall");
                        INFO("Switching to Concert Hall Reveberation");
                        break;
                    case XGMIDI_REVERB_HALL2:
                        midi.set_reverb("reverb/concerthall-large");
                        INFO("Switching to Lrge Concert Hall reveberation");
                        break;
                    case XGMIDI_REVERB_ROOM1:
                        midi.set_reverb("reverb/room-small");
                        INFO("Switching to Small Room reveberation");
                        break;
                    case XGMIDI_REVERB_ROOM2:
                        midi.set_reverb("reverb/room-medium");
                        INFO("Switching to Medium Room reveberation");
                        break;
                    case XGMIDI_REVERB_ROOM3:
                        midi.set_reverb("reverb/room-large");
                        INFO("Switching to Large Room reveberation");
                        break;
                    case XGMIDI_REVERB_STAGE1:
                       midi.set_reverb("reverb/concerthall");
                       INFO("Switching to Stage reveberation");
                       break;
                    case XGMIDI_REVERB_STAGE2:
                       midi.set_reverb("reverb/concerthall-large");
                       INFO("Switching to Large Stage reveberation");
                       break;
                    case XGMIDI_REVERB_PLATE:
                        midi.set_reverb("reverb/plate");
                        INFO("Switching to Plate reveberation");
                        break;
                    case XGMIDI_REVERB_WHITE_ROOM:
                       midi.set_reverb("reverb/bathroom");
                       INFO("Switching to White Room reveberation");
                       break;
                    case XGMIDI_REVERB_TUNNEL:
                       midi.set_reverb("reverb/room-empty");
                       INFO("Switching to Tunnel reveberation");
                       break;
                    case XGMIDI_REVERB_CANYON:
                       midi.set_reverb("reverb/arena");
                       INFO("Switching to Canyon reveberation");
                       break;
                    case XGMIDI_REVERB_BASEMENT:
                       midi.set_reverb("reverb/room-small");
                       INFO("Switching to Basement reveberation");
                       break;
                    default:
                        break;
                    }
                    break;
                }
                case XGMIDI_CHORUS_TYPE:
                {
                    uint16_t type = pull_byte() << 8 | pull_byte();
                    switch (type)
                    {
                    case XGMIDI_CHORUS1:
                        midi.set_chorus("chorus/chorus1");
                        INFO("Switching to type 1 chorus");
                        break;
                    case XGMIDI_CHORUS2:
                        midi.set_chorus("chorus/chorus2");
                        INFO("Switching to type 2 chorus");
                        break;
                    case XGMIDI_CHORUS3:
                        midi.set_chorus("chorus/chorus3");
                        INFO("Switching to type 3chorus");
                        break;
                    case XGMIDI_CHORUS4:
                        midi.set_chorus("chorus/chorus4");
                        INFO("Switching to type 4 chorus");
                        break;
                    case XGMIDI_CELESTE1:
                        midi.set_chorus("chorus/chorus1");
                        INFO("Switching to type 1 celeste");
                        break;
                    case XGMIDI_CELESTE2:
                        midi.set_chorus("chorus/chorus2");
                        INFO("Switching to type 2 celeste");
                        break;
                    case XGMIDI_CELESTE3:
                        midi.set_chorus("chorus/chorus3");
                        INFO("Switching to type 3 celeste");
                        break;
                    case XGMIDI_CELESTE4:
                        midi.set_chorus("chorus/chorus4");
                        INFO("Switching to type 4 celeste");
                        break;
                    case XGMIDI_FLANGING1:
                        midi.set_chorus("chorus/flanger");
                        INFO("Switching to type 1 flanging");
                        break;
                    case XGMIDI_FLANGING2:
                        midi.set_chorus("chorus/flanger2");
                        INFO("Switching to type 2 flanging");
                        break;
                    case XGMIDI_FLANGING3:
                        midi.set_chorus("chorus/flanger3");
                        INFO("Switching to type 3 flanging");
                        break;
                    case XGMIDI_SYMPHONIC:
                        midi.set_chorus("chorus/symphony");
                        INFO("Switching to symphony");
                        break;
                    case XGMIDI_PHASING:  
                        midi.set_chorus("chorus/phaser");
                        INFO("Switching to phasing");
                        break;
                    default:
                        break;
                    }
                    break;
                }
                case XGMIDI_VARIATION_TYPE:
                {
                    uint16_t type = pull_byte() << 8 | pull_byte();
                    break;
                }
                default:
                    break;
                }
                break;
            case XGMIDI_EFFECT2:
            case XGMIDI_MULTI_EQ:
            default:
                LOG(99, "LOG: Unsupported XG sysex parameter type: 0x%x (%d)\n",
                              type, type);
                break;
            }
            break;
        }
        case XGMIDI_MASTER_TUNING:
        {
            uint8_t addr_high = pull_byte();
            uint8_t addr_mid = pull_byte();
            uint8_t addr_low = pull_byte();
            uint32_t addr = addr_high << 16 | addr_mid << 8 | addr_low;
            if (addr == 0x300000)
            {
                uint8_t mm = pull_byte();
                uint8_t ll = pull_byte();
                uint8_t cc = pull_byte();
                uint16_t tuning = mm << 7 | ll;
                float pitch = (float)tuning-8192.0f;
                if (pitch < 0) pitch /= 8192.0f;
                else pitch /= 8191.0f;
                midi.set_tuning(pitch);
                rv = true;
            }
        }
        default:
            LOG(99, "LOG: Unsupported XG sysex parameter: 0x%x (%d)\n", byte, byte);
                break;
        }
        break;
    default:
        LOG(99, "LOG: Unsupported XG sysex type: 0x%x (%d)\n", type, type);
        break;
    }

#if 0
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
#endif

    return rv;
}

