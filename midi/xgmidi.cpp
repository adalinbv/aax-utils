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
    case 0x00:
    case 0x10:
        byte = pull_byte();
        CSV(", %d", byte);
        switch(byte)
        {
        case XGMIDI_PARAMETER_CHANGE:
        {
            uint8_t type = pull_byte();
            CSV(", %d", type);
            switch (type)
            {
            case XGMIDI_SYSTEM:
            {
                uint32_t addr = (pull_byte() << 8) | pull_byte();
                CSV(", %d, %d", addr >> 8, addr & 0xf);
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
            }
            case XGMIDI_EFFECT1:
            {
                uint8_t addr_high = pull_byte();
                uint8_t addr_mid = pull_byte();
                uint8_t addr_low = pull_byte();
                uint32_t addr = addr_high << 8 | addr_mid;

                switch (addr)
                {
                case XGMIDI_REVERB_HALL1:
                    break;
                default:
                    break;
                }
                break;
            }
            case XGMIDI_EFFECT2:
            default:
                LOG(99, "LOG: Unsupported XG sysex parameter type: 0x%x (%d)\n", type, type);
                break;
            }
            break;
        }
        case XGMIDI_MASTER_TUNING:
        {
            uint32_t addr = (pull_byte() << 16) | (pull_byte() << 8) | pull_byte();
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

