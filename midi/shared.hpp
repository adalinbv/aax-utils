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

# define CSV_TEXT(...) if(midi.get_verbose() == 1) { \
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

/* yamaha */
#define YAMAHA_PARAMETER_CHANGE					0x4c	// 76

#define YAMAHA_EFFECT1						0x02
#define YAMAHA_INSERTION_EFFECT					0x03
#define YAMAHA_XG_SYSTEM_ON					0x7e	// 126

/* XG Effect Map (page 16 of the spec) */
/* reverb */
#define YAMAHA_XG_REVERB_HALL1					0x0100
#define YAMAHA_XG_REVERB_HALL2 					0x0101
#define YAMAHA_XG_REVERB_ROOM1					0x0200
#define YAMAHA_XG_REVERB_ROOM2					0x0201
#define YAMAHA_XG_REVERB_ROOM3					0x0202
#define YAMAHA_XG_REVERB_STAGE1					0x0300
#define YAMAHA_XG_REVERB_STAGE2					0x0301
#define YAMAHA_XG_REVERB_PLATE					0x0400
#define YAMAHA_XG_REVERB_WHITE_ROOM				0x1000
#define YAMAHA_XG_REVERB_TUNNEL					0x1100
#define YAMAHA_XG_REVERB_CANYON					0x1200
#define YAMAHA_XG_REVERB_BASEMENT				0x1300

/* chorus */
#define YAMAHA_XG_CHORUS1					0x4100
#define YAMAHA_XG_CHORUS2					0x4101
#define YAMAHA_XG_CHORUS3					0x4102
#define YAMAHA_XG_CHORUS4					0x4108
#define YAMAHA_XG_CELESTE1					0x4200
#define YAMAHA_XG_CELESTE2					0x4201
#define YAMAHA_XG_CELESTE3					0x4202
#define YAMAHA_XG_CELESTE4					0x4208
#define YAMAHA_XG_FLANGING1					0x4300
#define YAMAHA_XG_FLANGING2					0x4301
#define YAMAHA_XG_FLANGING3					0x4308
#define YAMAHA_XG_SYMPHONIC					0x4400
#define YAMAHA_XG_PHASING					0x4800

/* variation */
#define YAMAHA_XG_DELAY_LCR					0x0500
#define YAMAHA_XG_DELAY_LR					0x0600
#define YAMAHA_XG_ECHO						0x0700
#define YAMAHA_XG_X_DELAY					0x0800
#define YAMAHA_XG_ER1						0x0900
#define YAMAHA_XG_ER2						0x0901
#define YAMAHA_XG_GATED_REVERB					0x0a00
#define YAMAHA_XG_REVERSE_GATE					0x0b00
#define YAMAHA_XG_THRU						0x4000
#define YAMAHA_XG_ROTARY_SPEAKER				0x4500
#define YAMAHA_XG_TREMOLO					0x4600
#define YAMAHA_XG_AUTO_PAN					0x4700
#define YAMAHA_XG_DISTORTION					0x4900
#define YAMAHA_XG_OVER_DRIVE					0x4a00
#define YAMAHA_XG_AMP_SIMULATOR					0x4b00
#define YAMAHA_XG_3_BAND_EQ					0x4c00
#define YAMAHA_XG_2_BAND_EQ					0x4d00
#define YAMAHA_XG_AUTO_WAH					0x4e00
#define YAMAHA_XG_PITCH_CHANGE					0x5000
#define YAMAHA_XG_AURAL_ENHANCER				0x5100
#define YAMAHA_XG_TOUCH_WAH					0x5200
#define YAMAHA_XG_TOUCH_WAH_DIST				0x5201
#define YAMAHA_XG_COMPRESSOR					0x5300
#define YAMAHA_XG_NOISE_GATE					0x5400
#define YAMAHA_XG_VOICE_CANCEL					0x5500


} // namespace aax

#endif // __AAX_MIDISHARED
