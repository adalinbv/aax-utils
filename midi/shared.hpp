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

#ifndef __AAX_MIDISHARED
#define __AAX_MIDISHARED

#include <aax/aeonwave.hpp>

# define CSV_TEXT(...) if(midi.get_csv()) { \
  char s[256]; snprintf(s, 256, __VA_ARGS__); \
  for (int i=0; i<strlen(s); ++i) { \
    if (s[i] == '\"') printf("\"\""); \
    else if ((s[i]<' ') || ((s[i]>'~') && (s[i]<=160))) printf("\\%03o", s[i]); \
    else printf("%c", s[i]); } \
  printf("\"\n"); \
} while(0);
# define PRINT_CSV(...) \
  if(midi.get_csv()) printf(__VA_ARGS__);
# define CSV(...) \
  if(midi.get_initialize()) PRINT_CSV(__VA_ARGS__)

#define DISPLAY(l,...) \
  if(midi.get_initialize() && l <= midi.get_verbose()) printf(__VA_ARGS__)
#define MESSAGE(...) \
  if(!midi.get_initialize() && midi.get_verbose() >= 1) printf(__VA_ARGS__)
#define INFO(s) \
  if(!midi.get_initialize() && midi.get_verbose() >= 1 && !midi.get_lyrics()) \
      printf("%-79s\n", (s))
#define LOG(l,...) \
  if(midi.get_initialize() && l == midi.get_verbose()) printf(__VA_ARGS__)
#define ERROR(...) \
  if(!midi.get_csv()) { std::cerr << __VA_ARGS__ << std::endl; }
#define FLUSH() \
  if (!midi.get_initialize() && midi.get_verbose() > 0) fflush(stdout)


namespace aax
{
#define MIDI_DRUMS_CHANNEL              0x9
#define MIDI_FILE_FORMAT_MAX            0x3

enum {
    MIDI_MODE0 = 0,
    MIDI_GENERAL_MIDI1,
    MIDI_GENERAL_MIDI2,
    MIDI_GENERAL_STANDARD,
    MIDI_EXTENDED_GENERAL_MIDI,

    MIDI_MODE_MAX
};

/* GSMIDI */
#define GSMIDI_DATA_REQUEST1					0x11
#define GSMIDI_DATA_SET1					0x12

/* parameter type */
#define GSMIDI_SYSTEM						0x10

#define GSMIDI_PARAMETER_CHANGE					0x40
#define GSMIDI_MODEL_GS						0x42

/* GS parameter map */
#define GSMIDI_GS_RESET						0x007f
#define GSMIDI_REVERB_TYPE					0x0130
#define GSMIDI_CHORUS_TYPE					0x0138
#define GSMIDI_EQUALIZER_TYPE					0x0200
#define GSMIDI_MULTI_PART					0x1100
#define GSMIDI_DRUM_PART9					0x1915
#define GSMIDI_DRUM_PART11					0x1a15

/* Reverb */
#define GSMIDI_REVERB_ROOM1					0x00
#define GSMIDI_REVERB_ROOM2					0x01
#define GSMIDI_REVERB_ROOM3					0x02
#define GSMIDI_REVERB_HALL1					0x03
#define GSMIDI_REVERB_HALL2					0x04
#define GSMIDI_REVERB_PLATE					0x05
#define GSMIDI_REVERB_DELAY					0x06
#define GSMIDI_REVERB_PAN_DELAY					0x07

/* Chorus */
#define GSMIDI_CHORUS1						0x00
#define GSMIDI_CHORUS2						0x01
#define GSMIDI_CHORUS3						0x02
#define GSMIDI_CHORUS4						0x03
#define GSMIDI_CHORUS_FEEDBACK					0x04
#define GSMIDI_FLANGER						0x05
#define GSMIDI_DELAY						0x06
#define GSMIDI_DELAY_FEEDBACK					0x07

/* EQ */
#define GSMIDI_EQUALIZER_FREQUENCY_LOW				0x0201
#define GSMIDI_EQUALIZER_GAIN_LOW
#define GSMIDI_EQUALIZER_FREQUENCY_HIGH
#define GSMIDI_EQUALIZER_GAIN_HIGH

/* XGMIDI */
/* categroy */
#define XGMIDI_BULK_DUMP					0x00
#define XGMIDI_PARAMETER_CHANGE					0x10
#define XGMIDI_DUMP_REQUEST					0x20
#define XGMIDI_PARAMETER_REQUEST				0x30

/* parameter category */
#define XGMIDI_MASTER_TUNING					0x27	// 39
#define XGMIDI_BLOCK1						0x29	// 41
#define XGMIDI_BLOCK2						0x3f	// 53
#define XGMIDI_MODEL_MU100_SET					0x49
#define XGMIDI_MODEL_VL70					0x47
#define XGMIDI_MODEL_MU100_MODIFY				0x59
#define XGMIDI_MODEL_XG						0x4c	// 76

/* parameter effect type */
#define XGMIDI_SYSTEM						0x00
#define XGMIDI_EFFECT1						0x02
#define XGMIDI_EFFECT2						0x03
#define XGMIDI_DISPLAY_DATA					0x06
#define XGMIDI_MULTI_PART					0x08
#define XGMIDI_A_D_PART						0x10
#define XGMIDI_A_D_SETUP					0x11
#define XGMIDI_DRUM_SETUP					0x30
#define XGMIDI_MULTI_EQ						0x40

/* XG Effect Map */
/* system */
#define XGMIDI_MASTER_TUNE					0x0000
#define XGMIDI_MASTER_VOLUME					0x0004
#define XGMIDI_ATTENUATOR					0x0005
#define XGMIDI_TRANSPOSE					0x0006
#define XGMIDI_DRUM_SETUP_RESET					0x007d
#define XGMIDI_SYSTEM_ON					0x007e

/* effect1 address (0x02) */
#define XGMIDI_REVERB_TYPE					0x0100
#define XGMIDI_REVERB_PARAMETER1				0x0102
#define XGMIDI_REVERB_PARAMETER2				0x0103
#define XGMIDI_REVERB_PARAMETER3				0x0104
#define XGMIDI_REVERB_PARAMETER4				0x0105
#define XGMIDI_REVERB_PARAMETER5				0x0106
#define XGMIDI_REVERB_PARAMETER6				0x0107
#define XGMIDI_REVERB_PARAMETER7				0x0108
#define XGMIDI_REVERB_PARAMETER8				0x0109
#define XGMIDI_REVERB_PARAMETER9				0x010a
#define XGMIDI_REVERB_PARAMETER10				0x010b
#define XGMIDI_REVERB_RETURN					0x010c
#define XGMIDI_REVERB_PAN					0x010d

#define XGMIDI_REVERB_PARAMETER11				0x0110
#define XGMIDI_REVERB_PARAMETER12				0x0111
#define XGMIDI_REVERB_PARAMETER13				0x0112
#define XGMIDI_REVERB_PARAMETER14				0x0113
#define XGMIDI_REVERB_PARAMETER15				0x0114
#define XGMIDI_REVERB_PARAMETER16				0x0115

#define XGMIDI_CHORUS_TYPE					0x0120
#define XGMIDI_CHORUS_PARAMETER1				0x0122
#define XGMIDI_CHORUS_PARAMETER2				0x0123
#define XGMIDI_CHORUS_PARAMETER3				0x0124
#define XGMIDI_CHORUS_PARAMETER4				0x0125
#define XGMIDI_CHORUS_PARAMETER5				0x0126
#define XGMIDI_CHORUS_PARAMETER6				0x0127
#define XGMIDI_CHORUS_PARAMETER7				0x0128
#define XGMIDI_CHORUS_PARAMETER8				0x0129
#define XGMIDI_CHORUS_PARAMETER9				0x012a
#define XGMIDI_CHORUS_PARAMETER10				0x012b
#define XGMIDI_CHORUS_RETURN					0x012c
#define XGMIDI_CHORUS_PAN					0x012d
#define XGMIDI_CHORUS_SEND_TO_REVERB				0x012e

#define XGMIDI_CHORUS_PARAMETER11				0x0130
#define XGMIDI_CHORUS_PARAMETER12				0x0131
#define XGMIDI_CHORUS_PARAMETER13				0x0132
#define XGMIDI_CHORUS_PARAMETER14				0x0133
#define XGMIDI_CHORUS_PARAMETER15				0x0134
#define XGMIDI_CHORUS_PARAMETER16				0x0135

#define XGMIDI_VARIATION_TYPE					0x0140
#define XGMIDI_VARIATION_PARAMETER1				0x0142
#define XGMIDI_VARIATION_PARAMETER2				0x0144
#define XGMIDI_VARIATION_PARAMETER3				0x0146
#define XGMIDI_VARIATION_PARAMETER4				0x0148
#define XGMIDI_VARIATION_PARAMETER5				0x014a
#define XGMIDI_VARIATION_PARAMETER6				0x014c
#define XGMIDI_VARIATION_PARAMETER7				0x014e
#define XGMIDI_VARIATION_PARAMETER8				0x0150
#define XGMIDI_VARIATION_PARAMETER9				0x0152
#define XGMIDI_VARIATION_PARAMETER10				0x0154
#define XGMIDI_VARIATION_RETURN					0x0156
#define XGMIDI_VARIATION_PAN					0x0157
#define XGMIDI_VARIATION_SEND_TO_REVERB				0x0158
#define XGMIDI_VARIATION_SEND_TO_CHORUS				0x0159
#define XGMIDI_VARIATION_CONNECTION				0x015a
#define XGMIDI_VARIATION_PART_NUMBER				0x015b
#define XGMIDI_MW_VARIATION_CONTROL_DEPTH			0x015c
#define XGMIDI_BEND_VARIATION_CONTROL_DEPTH			0x015d
#define XGMIDI_CAT_VARIATION_CONTROL_DEPTH			0x015e
#define XGMIDI_AC1_VARIATION_CONTROL_DEPTH			0x015f
#define XGMIDI_AC2_VARIATION_CONTROL_DEPTH			0x0160

#define XGMIDI_VARIATION_PARAMETER11				0x0170
#define XGMIDI_VARIATION_PARAMETER12				0x0171
#define XGMIDI_VARIATION_PARAMETER13				0x0172
#define XGMIDI_VARIATION_PARAMETER14				0x0173
#define XGMIDI_VARIATION_PARAMETER15				0x0174
#define XGMIDI_VARIATION_PARAMETER16				0x0175

#define XGMIDI_MULTI_EQ_TYPE					0x4000
#define XGMIDI_MULTI_EQ_GAIN1					0x4001
#define XGMIDI_MULTI_EQ_FREQUENCY1				0x4002
#define XGMIDI_MULTI_EQ_Q1					0x4003
#define XGMIDI_MULTI_EQ_SHAPE1					0x4004
#define XGMIDI_MULTI_EQ_GAIN2					0x4005
#define XGMIDI_MULTI_EQ_FREQUENCY2				0x4006
#define XGMIDI_MULTI_EQ_Q2					0x4007
#define XGMIDI_MULTI_EQ_GAIN3					0x4009
#define XGMIDI_MULTI_EQ_FREQUENCY3				0x400a
#define XGMIDI_MULTI_EQ_Q3					0x400b
#define XGMIDI_MULTI_EQ_GAIN4					0x400d
#define XGMIDI_MULTI_EQ_FREQUENCY4				0x400e
#define XGMIDI_MULTI_EQ_Q4					0x400f
#define XGMIDI_MULTI_EQ_GAIN5					0x4011
#define XGMIDI_MULTI_EQ_FREQUENCY5				0x4012
#define XGMIDI_MULTI_EQ_Q5					0x4013
#define XGMIDI_MULTI_EQ_SHAPE5					0x4014

/* reverb type */
#define XGMIDI_REVERB_HALL1					0x0100
#define XGMIDI_REVERB_HALL2 					0x0101
#define XGMIDI_REVERB_ROOM1					0x0200
#define XGMIDI_REVERB_ROOM2					0x0201
#define XGMIDI_REVERB_ROOM3					0x0202
#define XGMIDI_REVERB_STAGE1					0x0300
#define XGMIDI_REVERB_STAGE2					0x0301
#define XGMIDI_REVERB_PLATE					0x0400
#define XGMIDI_REVERB_WHITE_ROOM				0x1000
#define XGMIDI_REVERB_TUNNEL					0x1100
#define XGMIDI_REVERB_CANYON					0x1200
#define XGMIDI_REVERB_BASEMENT					0x1300

/* chorus type */
#define XGMIDI_CHORUS1						0x4100
#define XGMIDI_CHORUS2						0x4101
#define XGMIDI_CHORUS3						0x4102
#define XGMIDI_CHORUS4						0x4108
#define XGMIDI_CELESTE1						0x4200
#define XGMIDI_CELESTE2						0x4201
#define XGMIDI_CELESTE3						0x4202
#define XGMIDI_CELESTE4						0x4208
#define XGMIDI_FLANGING1					0x4300
#define XGMIDI_FLANGING2					0x4301
#define XGMIDI_FLANGING3					0x4308
#define XGMIDI_SYMPHONIC					0x4400
#define XGMIDI_PHASING						0x4800

/* variation type */
#define XGMIDI_DELAY_LCR					0x0500
#define XGMIDI_DELAY_LR						0x0600
#define XGMIDI_ECHO						0x0700
#define XGMIDI_X_DELAY						0x0800
#define XGMIDI_ER1						0x0900
#define XGMIDI_ER2						0x0901
#define XGMIDI_GATED_REVERB					0x0a00
#define XGMIDI_REVERSE_GATE					0x0b00
#define XGMIDI_THRU						0x4000
#define XGMIDI_ROTARY_SPEAKER					0x4500
#define XGMIDI_TREMOLO						0x4600
#define XGMIDI_AUTO_PAN						0x4700
#define XGMIDI_DISTORTION					0x4900
#define XGMIDI_OVER_DRIVE					0x4a00
#define XGMIDI_AMP_SIMULATOR					0x4b00
#define XGMIDI_3_BAND_EQ					0x4c00
#define XGMIDI_2_BAND_EQ					0x4d00
#define XGMIDI_AUTO_WAH						0x4e00
#define XGMIDI_PITCH_CHANGE					0x5000
#define XGMIDI_AURAL_ENHANCER					0x5100
#define XGMIDI_TOUCH_WAH					0x5200
#define XGMIDI_TOUCH_WAH_DIST					0x5201
#define XGMIDI_COMPRESSOR					0x5300
#define XGMIDI_NOISE_GATE					0x5400
#define XGMIDI_VOICE_CANCEL					0x5500

/* A/D part, addr_mid = channel no. */

/* Multi part */
#define XGMIDI_BANK_SELECT_MSB					0x01
#define XGMIDI_BANK_SELECT_LSB					0x02
#define XGMIDI_PROGRAM_NUMBER					0x03
#define XGMIDI_RECV_CHANNEL					0x04
#define XGMIDI_MONO_POLY_MODE					0x05
#define XGMIDI_KEY_ON_ASSIGN					0x06
#define XGMIDI_PART_MODE					0x07
#define XGMIDI_NOTE_SHIFT					0x08
#define XGMIDI_DETUNE						0x09
#define XGMIDI_VOLUME						0x0b
#define XGMIDI_VELOCITY_SENSE_DEPTH				0x0c
#define XGMIDI_VELOCITY_SENSE_OFFSET				0x0d
#define XGMIDI_PAN						0x0e
#define XGMIDI_NOTE_LIMIT_LOW					0x0f
#define XGMIDI_NOTE_LIMIT_HIGH					0x10
#define XGMIDI_DRY_LEVEL					0x11
#define XGMIDI_CHORUS_SEND					0x12
#define XGMIDI_REVERB_SEND					0x13
#define XGMIDI_VARIATION_SEND					0x14
#define XGMIDI_VIBRATO_RATE					0x15
#define XGMIDI_VIBRATO_DEPTH					0x16
#define XGMIDI_VIBRATO_DELAY					0x17
#define XGMIDI_FILTER_CUTOFF_FREQUENCY				0x18
#define XGMIDI_FILTER_RESONANCE					0x19
#define XGMIDI_EG_ATTACK_TIME					0x1a
#define XGMIDI_EG_DECAY_TIME					0x1b
#define XGMIDI_EG_RELEASE_TIME					0x1c
#define XGMIDI_MW_PITCH_CONTROL					0x1d
#define XGMIDI_MW_FILTER_CONTROL				0x1e
#define XGMIDI_MW_AMPLITUDE_CONTROL				0x1f
#define XGMIDI_MW_LFO_PMOD_DEPTH				0x20
#define XGMIDI_MW_LFO_FMOD_DEPTH				0x21
#define XGMIDI_MW_LFO_AMOD_DEPTH				0x22
#define XGMIDI_BEND_PITCH_CONTROL				0x23
#define XGMIDI_BEND_FILTER_CONTROL				0x24
#define XGMIDI_BEND_AMPLITUDE_CONTROL				0x25
#define XGMIDI_BEND_LFO_PMOD_DEPTH				0x26
#define XGMIDI_BEND_LFO_FMOD_DEPTH				0x27
#define XGMIDI_BEND_LFO_AMOD_DEPTH				0x28

} // namespace aax

#endif // __AAX_MIDISHARED
