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

/** XGMIDI defaults (reverb, chorus, distortion and EQ):
 *
 * param:       1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16
 *              --------------------------------------------------------------
 * HALL1        18  10  8   13  49  0   0   0   0   40  0   4   50  8   64  0
 * HALL2        25  10  28  6   46  0   0   0   0   40  13  3   74  7   64  0
 * ROOM1        5   10  16  4   49  0   0   0   0   40  5   3   64  8   64  0
 * ROOM2        12  10  5   4   38  0   0   0   0   40  0   4   50  8   64  0
 * ROOM3        9   10  47  5   36  0   0   0   0   40  0   4   60  8   64  0
 * STAGE1       19  10  16  7   54  0   0   0   0   40  0   3   64  6   64  0
 * STAGE2       11  10  16  7   51  0   0   0   0   40  2   2   64  6   64  0
 * PLATE        25  10  6   8   49  0   0   0   0   40  2   3   64  5   64  0
 * WHITEROOM    9   5   11  0   46  30  50  70  7   40  34  4   64  7   64  0
 * TUNNEL       48  6   19  0   44  33  52  70  16  40  20  4   64  7   64  0
 * CANYON       59  6   63  0   45  34  62  91  13  40  25  4   64  4   64  0
 * BASEMENT     3   6   3   0   34  26  29  59  15  40  32  4   64  8   64  0
 *
 * CHORUS1      6   54  77  106 0   28  64  46  64  64  46  64  10  0   0   0
 * CHORUS2      8   63  64  30  0   28  62  42  58  64  46  64  10  0   0   0
 * CHORUS3      4   44  64  110 0   28  64  46  66  64  46  64  10  0   0   0
 * CHORUS4      9   32  69  104 0   28  64  46  64  64  46  64  10  0   1   0
 * CELESTE1     12  32  64  0   0   28  64  46  64  127 40  68  10  0   0   0
 * CELESTE2     28  18  90  2   0   28  62  42  60  84  40  68  10  0   0   0
 * CELESTE3     4   63  44  2   0   28  64  46  68  127 40  68  10  0   0   0
 * CELESTE4     8   29  64  0   0   28  64  51  66  127 40  68  10  0   1   0
 * FLANGER1     14  14  104 2   0   28  64  46  64  96  40  64  10  4   0   0
 * FLANGER2     32  17  26  2   0   28  64  46  60  96  40  64  10  4   0   0
 * FLANGER3     4   109 109 2   0   28  64  46  64  127 40  64  10  4   0   0
 * PHASER1      8   111 74  104 0   28  64  46  64  64  6   1   64  0   0   0
 * PHASER2      8   111 74  104 0   28  64  46  64  64  5   1   4   0   0   0
 *
 * DISTORTION   40  20  72  53  48  0   43  74  10  127 120 0   0   0   0   0
 * OVERDRIVE    29  24  68  45  55  0   41  72  10  127 104 0   0   0   0   0
 *
 * 3-BAND EQ	70  34  60  10  70  28  46  0   0   127 0   0   0   0   0   0
 * 2-BAND EQ    28  70  46  70  0   0   0   0   0   127 34  64  10  0   0   0
 *
 */

namespace aax
{

static float XGMIDI_LFO_table[128] = {	// Hz
 0.f, 0.08f, 0.08f, 0.16f, 0.16f, 0.25f, 0.25f, 0.33f, 0.33f, 0.42f, 0.42f,
 0.5f, 0.5f, 0.58f, 0.58f, 0.67f, 0.67f, 0.75f, 0.75f, 0.84, 0.84f, 0.92f,
 0.92f, 1.f, 1.f, 1.09f, 1.09f, 1.17f, 1.17f, 1.26f, 1.26f, 1.34f, 1.34f,
 1.43f, 1.43f, 1.51f, 1.51f, 1.59f, 1.59f, 1.68f, 1.68f, 1.76f, 1.76f, 1.85f,
 1.85f, 1.93f, 1.93f, 2.01f, 2.01f, 2.1f, 2.1f, 2.18f, 2.18f, 2.27f, 2.27f,
 2.35f, 2.35f, 2.43f, 2.43f, 2.52f, 2.52f, 2.6f, 2.6f, 2.69f, 2.69f, 2.77f,
 2.86f, 2.94f, 3.02f, 3.11f, 3.19f, 3.28f, 3.36f, 3.44f, 3.53f, 3.61f, 3.7f,
 3.86f, 4.03f, 4.2f, 4.37f, 4.54f, 4.71f, 4.87f, 5.04f, 5.21f, 5.38f, 5.55f,
 5.72f, 6.05f, 6.39f, 6.72f, 7.06f, 7.4f, 7.73f, 8.07f, 8.1f, 8.74f, 9.08f,
 9.42f, 8.75f, 10.f, 11.4f, 12.1f, 12.7f, 13.4f, 14.1f, 14.8f, 15.4f, 16.1f,
 16.8f, 17.4f, 18.1f, 19.5f, 20.8f, 22.2f, 23.5f, 24.8f, 26.2f, 27.5f, 28.9f,
 30.2f, 31.6f, 32.9f, 34.3f, 37.f, 39.7f
};

static float XGMIDI_delay_offset_table[128] = {
 0.f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.f, 1.1f, 1.2f,
 1.3f, 1.4f, 1.5f, 1.6f, 1.7f, 1.8f, 1.9f, 2.f, 2.1f, 2.2f, 2.3f, 2.4f, 2.5f,
 2.6f, 2.7f, 2.8f, 2.9f, 3.f, 3.1f, 3.2f, 3.3f, 3.4f, 3.5f, 3.6f, 3.7f, 3.8f,
 3.9f, 4.f, 4.1f, 4.2f, 4.3f, 4.4f, 4.5f, 4.6f, 4.7f, 4.8f, 4.9f, 5.f, 5.1f,
 5.2f, 5.3f, 5.4f, 5.5f, 5.6f, 5.7f, 5.8f, 5.9f, 6.f, 6.1f, 6.2f, 6.3f, 6.4f,
 6.5f, 6.6f, 6.7f, 6.8f, 6.9f, 7.f, 7.1f, 7.2f, 7.3f, 7.4f, 7.5f, 7.6f, 7.7f,
 7.8f, 7.9f, 8.f, 8.1f, 8.2f, 8.3f, 8.4f, 8.5f, 8.6f, 8.7f, 8.8f, 8.9f, 9.f,
 9.1f, 9.2f, 9.3f, 9.4f, 9.5f, 9.6f, 9.7f, 9.8f, 9.9f, 10.f, 11.1f, 12.2f,
 13.3f, 14.4f, 15.5f, 17.1f, 18.6f, 20.2f, 21.8f, 23.3f, 24.9f, 26.5f, 28.f,
 29.6f, 31.2f, 32.8f, 34.3f, 35.9f, 37.f, 39.f, 40.6f, 42.2f, 43.7f, 45.3f,
 46.9f, 48.4f, 50.f
};

static float XGMIDI_delay_time_table[128] = {	// ms
 0.1f, 1.7f, 3.2f, 4.8f, 6.4f, 8.0f, 9.5f, 11.1f, 12.7f, 14.3f, 15.8f, 17.4f,
 19.f, 2.6f, 22.1f, 23.7f, 25.3f, 26.9f, 28.4f, 30.f, 31.6f, 33.2f, 34.7f,
 36.3f, 37.9f, 39.5f, 41.f, 42.6f, 44.2f, 45.7f, 47.3f, 48.9f, 50.5f, 52.f,
 53.6f, 55.2f, 56.8f, 58.3f, 59.9f, 61.5f, 63.1f, 64.4f, 66.2f, 67.8f, 69.4f,
 70.9f, 72.5f, 74.1f, 75.7f, 77.2f, 78.8f, 80.4f, 81.9f, 83.5f, 85.1f, 86.7f,
 88.2f, 89.8f, 91.4f, 93.f, 94.5f, 96.1f, 97.7f, 99.3f, 100.8f, 102.4f, 104.f,
 105.6f, 107.1f, 108.7f, 110.3f, 111.9f, 113.4f, 115.f, 116.6f, 118.2f, 119.7f,
 121.3f, 122.9f, 124.4f, 126.f, 127.6f, 129.2f, 130.7f, 132.3f, 133.9f, 135.5f,
 137.f, 138.6f, 1402.f, 141.8f, 143.3f, 144.9f, 146.5f, 148.1f, 149.6f, 151.2f,
 152.8f, 154.4f, 155.9f, 157.5f, 159.1f, 160.6f, 162.2f, 163.8f, 165.4f,
 166.9f, 168.5f, 170.1f, 171.7f, 173.2f, 174.8f, 176.4f, 178.f, 179.5f, 181.1f,
 182.7f, 184.3f, 185.8f, 187.4f, 189.f, 190.6f, 192.1f, 193.7f, 195.3f, 169.6f,
 198.4f, 200.f
};

static float XGMIDI_EQ_frequency_table[61] = {
 20.f, 22.f, 25.f, 28.f, 32.f, 36.f, 40.f, 45.f, 50.f, 56.f, 63.f, 70.f, 80.f,
 90.f, 100.f, 110.f, 125.f, 140.f, 160.f, 180.f, 200.f, 225.f, 250.f, 280.f,
 315.f, 355.f, 400.f, 450.f, 500.f, 560.f, 630.f, 700.f, 800.f, 900.f, 1000.f,
 1100.f, 1200.f, 1400.f, 1600.f, 1800.f, 2000.f, 2200.f, 2500.f, 2800.f,
 3200.f, 3600.f, 4000.f, 4500.f, 5000.f, 5600.f, 6300.f, 7000.0f, 8000.f,
 9000.f, 10000.f, 11000.0f, 12000.f, 14000.f, 16000.f, 18000.f, 20000.f
};

static float XGMIDI_room_size_table[45] = {
 0.1f, 0.3f, 0.4f, 0.6f, .7f, 0.9f, 1.f, 1.2f, 1.4f, 1.5f, 1.7f, 1.8f, 2.f,
 2.1f, 2.3f, 2.5f, 2.6f, 2.8f, 2.9f, 3.1f, 4.2f, 3.4f, 3.5f, 3.7f, 3.9f, 4.f,
 4.2f, 4.3f, 4.5f, 4.6f, 4.8f, 5.f, 5.1f, 5.3f, 5.4f, 5.6f, 5.7f, 5.9f, 6.1f,
 6.2f, 6.4f, 6.5f, 6.7f, 6.8f, 7.f
};

static float XGMIDI_compressor_release_time_table[16] = {
 10.f, 15.f, 25.f, 35.f, 45.f, 55.f, 65.f, 75.f, 85.f, 100.f, 115.f, 140.f,
 170.f, 230.f, 340.f, 680.f
};

static float XGMIDI_compressor_ratio_table[8] = {
 1.f, 1.5f, 2.f, 3.f, 5.f, 7.f, 10.f, 20.0f
};

static float XGMIDI_reverb_dimensions[105] = {
 0.5f, 0.8f, 1.f, 1.3f, 1.5f, 1.8f, 2.f, 2.3f, 2.6f, 2.8f, 3.1f, 3.3f, 3.6f,
 3.9f, 4.1f, 4.4f, 4.6f, 4.9f, 5.2f, 5.4f, 5.7f, 5.9f, 6.2f, 6.5f, 6.7f, 7.f,
 7.2f, 7.5f, 7.8f, 8.f, 8.3f, 8.6f, 8.8f, 9.1f, 9.4f, 9.6f, 9.9f, 10.2f, 10.4f,
 10.7f, 11.f, 11.2f, 11.5f, 11.8f, 12.1f, 12.3f, 12.6f, 12.9f, 13.1f, 3.4f,
 14.f, 14.2f, 14.5f, 14.8f, 15.1f, 15.4f, 15.6f, 15.9f, 16.2f, 16.5f, 16.8f,
 17.1f, 17.3f, 17.6f, 17.9f, 18.2f, 18.5f, 18.8f, 19.1f, 19.4f, 19.7f, 2.0f,
 20.5f, 20.8f, 21.1f, 22.f, 22.4f, 22.7f, 23.f, 23.3f, 23.6f, 23.9f, 24.2f,
 24.5f, 24.9f, 25.2f, 25.5f, 25.8f, 27.1f, 27.5f, 27.8f, 28.1f, 28.5f, 28.8f,
 29.2f, 29.5f, 29.9f, 30.2f
};

}

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
                        rv = true;
                        break;
                    case XGMIDI_REVERB_HALL2:
                        midi.set_reverb("reverb/concerthall-large");
                        INFO("Switching to Lrge Concert Hall reveberation");
                        rv = true;
                        break;
                    case XGMIDI_REVERB_ROOM1:
                        midi.set_reverb("reverb/room-small");
                        INFO("Switching to Small Room reveberation");
                        rv = true;
                        break;
                    case XGMIDI_REVERB_ROOM2:
                        midi.set_reverb("reverb/room-medium");
                        INFO("Switching to Medium Room reveberation");
                        rv = true;
                        break;
                    case XGMIDI_REVERB_ROOM3:
                        midi.set_reverb("reverb/room-large");
                        INFO("Switching to Large Room reveberation");
                        rv = true;
                        break;
                    case XGMIDI_REVERB_STAGE1:
                       midi.set_reverb("reverb/concerthall");
                       INFO("Switching to Stage reveberation");
                        rv = true;
                       break;
                    case XGMIDI_REVERB_STAGE2:
                       midi.set_reverb("reverb/concerthall-large");
                       INFO("Switching to Large Stage reveberation");
                        rv = true;
                       break;
                    case XGMIDI_REVERB_PLATE:
                        midi.set_reverb("reverb/plate");
                        INFO("Switching to Plate reveberation");
                        rv = true;
                        break;
                    case XGMIDI_REVERB_WHITE_ROOM:
                       midi.set_reverb("reverb/bathroom");
                       INFO("Switching to White Room reveberation");
                        rv = true;
                       break;
                    case XGMIDI_REVERB_TUNNEL:
                       midi.set_reverb("reverb/room-empty");
                       INFO("Switching to Tunnel reveberation");
                        rv = true;
                       break;
                    case XGMIDI_REVERB_CANYON:
                       midi.set_reverb("reverb/arena");
                       INFO("Switching to Canyon reveberation");
                        rv = true;
                       break;
                    case XGMIDI_REVERB_BASEMENT:
                       midi.set_reverb("reverb/room-small");
                       INFO("Switching to Basement reveberation");
                        rv = true;
                       break;
                    default:
                        LOG(99, "LOG: Unsupported XG reverb type: %d\n", type);
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
                        rv = true;
                        break;
                    case XGMIDI_CHORUS2:
                        midi.set_chorus("chorus/chorus2");
                        INFO("Switching to type 2 chorus");
                        rv = true;
                        break;
                    case XGMIDI_CHORUS3:
                        midi.set_chorus("chorus/chorus3");
                        INFO("Switching to type 3chorus");
                        rv = true;
                        break;
                    case XGMIDI_CHORUS4:
                        midi.set_chorus("chorus/chorus4");
                        INFO("Switching to type 4 chorus");
                        rv = true;
                        break;
                    case XGMIDI_CELESTE1:
                        midi.set_chorus("chorus/chorus1");
                        INFO("Switching to type 1 celeste");
                        rv = true;
                        break;
                    case XGMIDI_CELESTE2:
                        midi.set_chorus("chorus/chorus2");
                        INFO("Switching to type 2 celeste");
                        rv = true;
                        break;
                    case XGMIDI_CELESTE3:
                        midi.set_chorus("chorus/chorus3");
                        INFO("Switching to type 3 celeste");
                        rv = true;
                        break;
                    case XGMIDI_CELESTE4:
                        midi.set_chorus("chorus/chorus4");
                        INFO("Switching to type 4 celeste");
                        rv = true;
                        break;
                    case XGMIDI_FLANGING1:
                        midi.set_chorus("chorus/flanger");
                        INFO("Switching to type 1 flanging");
                        rv = true;
                        break;
                    case XGMIDI_FLANGING2:
                        midi.set_chorus("chorus/flanger2");
                        INFO("Switching to type 2 flanging");
                        rv = true;
                        break;
                    case XGMIDI_FLANGING3:
                        midi.set_chorus("chorus/flanger3");
                        INFO("Switching to type 3 flanging");
                        rv = true;
                        break;
                    case XGMIDI_SYMPHONIC:
                        midi.set_chorus("chorus/symphony");
                        INFO("Switching to symphony");
                        rv = true;
                        break;
                    case XGMIDI_PHASING:  
                        midi.set_chorus("chorus/phaser");
                        INFO("Switching to phasing");
                        rv = true;
                        break;
                    default:
                        LOG(99, "LOG: Unsupported XG chorus type: %d\n", type);
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
                    LOG(99, "LOG: Unsupported XG Effect1 address: %d %d\n",
                            addr_mid, addr_low);
                    break;
                }
                break;
            case XGMIDI_EFFECT2:
                LOG(99, "LOG: Unsupported XG sysex type: Effect2\n");
                break;
            case XGMIDI_MULTI_EQ:
                LOG(99, "LOG: Unsupported XG sysex type: Multi EQ\n");
                break;
            case XGMIDI_MULTI_PART:
                LOG(99, "LOG: Unsupported XG sysex type: Multi Part\n");
                break;
            case XGMIDI_A_D_PART:
                LOG(99, "LOG: Unsupported XG sysex type: A/D Part\n");
                break;
            case XGMIDI_A_D_SETUP:
                LOG(99, "LOG: Unsupported XG sysex type: A/D Setup\n");
                break;
            case XGMIDI_DRUM_SETUP:
                LOG(99, "LOG: Unsupported XG sysex type: Drum Setup\n");
                break;
            case XGMIDI_DISPLAY_DATA:
            {
                std::string text;
                for (int i=offset()-offs; i<size-1; ++i) {
                    toUTF8(text, pull_byte());
                }
                MESSAGE("Display: %s\n", text.c_str());
                break;
            }
            default:
                LOG(99, "LOG: Unsupported XG sysex effect type: 0x%x (%d)\n",
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
            break;
        }
        default:
            LOG(99, "LOG: Unsupported XG sysex parameter category: 0x%x (%d)\n", byte, byte);
                break;
        }
        break;
    default:
        LOG(99, "LOG: Unsupported XG category type: 0x%x (%d)\n", type, type);
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

