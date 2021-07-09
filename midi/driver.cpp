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

#include <regex>
#include <iostream>

#include <aax/strings.hpp>
#include <xml.h>

#include <midi/shared.hpp>
#include <midi/file.hpp>
#include <midi/driver.hpp>
#include <midi/instrument.hpp>

using namespace aax;

MIDIDriver::MIDIDriver(const char* n, const char *selections, enum aaxRenderMode m)
        : AeonWave(n, m)
{
    if (*this) {
        set_path();
    }
    else
    {
        if (n) {
            throw(std::runtime_error("Unable to open device '"+std::string(n)+"'"));
        } else {
            throw(std::runtime_error("Unable to open the default device"));
        }
        return;
    }

    if (selections)
    {
        std::string s(selections);
        std::regex regex{R"(,+)"}; // split on a comma
        std::sregex_token_iterator it{s.begin(), s.end(), regex, -1};
        selection = std::vector<std::string>{it, {}};

        for(auto s : selection) {
            uint16_t t = atoi(s.c_str());
            if (t) active_track.push_back(t);
        }
    }

    reverb.tie(reverb_decay_depth, AAX_REVERB_EFFECT, AAX_DECAY_DEPTH);
    reverb.tie(reverb_cutoff_frequency, AAX_REVERB_EFFECT, AAX_CUTOFF_FREQUENCY);
    reverb.tie(reverb_state, AAX_REVERB_EFFECT);
}

void
MIDIDriver::set_path()
{
    path = AeonWave::info(AAX_SHARED_DATA_DIR);

    std::string name = path;
    if (instrument_mode == AAX_RENDER_NORMAL) {
        name.append("/ultrasynth/");
    }
    if (midi.exists(name))
    {
        path = name;
        AeonWave::set(AAX_SHARED_DATA_DIR, path.c_str());
    }
}

void
MIDIDriver::start()
{
    reverb_state = AAX_REVERB_2ND_ORDER;
    set_reverb("reverb/concerthall-large");
    reverb.set(AAX_INITIALIZED);
    reverb.set(AAX_PLAYING);
    AeonWave::add(reverb);
    midi.set_gain(1.0f);
    midi.set(AAX_PLAYING);
}

void
MIDIDriver::stop()
{
    reverb.set(AAX_PLAYING);
    midi.set(AAX_PLAYING);
}

void
MIDIDriver::rewind()
{
    channels.clear();
    uSPP = tempo/PPQN;

    for (const auto& it : reverb_channels)
    {
        reverb.remove(*it.second);
        AeonWave::add(*it.second);
    }
    reverb_channels.clear();
}

void MIDIDriver::finish(uint8_t n)
{
    auto it = channels.find(n);
    if (it == channels.end()) return;

    if (it->second->finished() == false) {
        it->second->finish();
    }
}

bool
MIDIDriver::finished(uint8_t n)
{
    auto it = channels.find(n);
    if (it == channels.end()) return true;
    return it->second->finished();
}

void
MIDIDriver::set_gain(float g)
{
    aax::dsp dsp = AeonWave::get(AAX_VOLUME_FILTER);
    dsp.set(AAX_GAIN, g);
//  dsp.set(AAX_AGC_RESPONSE_RATE, 1.5f);
    AeonWave::set(dsp);
}

bool
MIDIDriver::is_drums(uint8_t n)
{
    auto it = channels.find(n);
    if (it == channels.end()) return false;
    return it->second->is_drums();
}

void
MIDIDriver::set_balance(float b)
{
    Matrix64 m;
    m.rotate(1.57*b, 0.0, 1.0, 0.0);
    m.inverse();
    AeonWave::matrix(m);
}

void
MIDIDriver::set_chorus_type(uint8_t value)
{
    switch(value)
    {
    case 0:
        midi.set_chorus("chorus/chorus1");
        break;
    case 1:
        midi.set_chorus("chorus/chorus2");
        break;
    case 2:
        midi.set_chorus("chorus/chorus3");
        break;
    case 3:
        midi.set_chorus("chorus/chorus4");
        break;
    case 4:
        midi.set_chorus("chorus/chorus_freedback");
        break;
    case 5:
        midi.set_chorus("chorus/flanger");
        break;
    default:
        break;
    }
}

void
MIDIDriver::set_chorus(const char *t)
{
    Buffer& buf = AeonWave::buffer(t);
    for(auto& it : channels) {
        it.second->add(buf);
    }
}

void
MIDIDriver::set_chorus_level(float lvl)
{
    for(auto& it : channels) {
        it.second->set_chorus_level(lvl);
    }
}

void
MIDIDriver::set_chorus_depth(float ms)
{
    float us = 1e-3f*ms;
    for(auto& it : channels) {
        it.second->set_chorus_depth(us);
    }
}

void
MIDIDriver::set_chorus_rate(float rate)
{
    for(auto& it : channels) {
        it.second->set_chorus_rate(rate);
    }
}

void
MIDIDriver::set_reverb(const char *t)
{
    Buffer& buf = AeonWave::buffer(t);
    reverb.add(buf);
    for(auto& it : channels) {
        it.second->set_reverb(buf);
    }
}

void
MIDIDriver::set_reverb_type(uint8_t value)
{
    reverb_type = value;
    switch (value)
    {
    case 0:
        midi.set_reverb("reverb/room-small");
        INFO("Switching to Small Room reveberation");
        break;
    case 1:
        midi.set_reverb("reverb/room-medium");
        INFO("Switching to Medium Room reveberation");
        break;
    case 2:
        midi.set_reverb("reverb/room-large");
        INFO("Switching to Large Room reveberation");
        break;
    case 3:
        midi.set_reverb("reverb/concerthall");
        INFO("Switching to Concert Hall Reveberation");
        break;
    case 4:
        midi.set_reverb("reverb/concerthall-large");
        INFO("Switching to Lrge Concert Hall reveberation");
        break;
    case 8:
        midi.set_reverb("reverb/plate");
        INFO("Switching to Plate reveberation");
        break;
    default:
        break;
    }
}

void
MIDIDriver::set_reverb_level(uint8_t channel, float val)
{
    if (val)
    {
        midi.channel(channel).set_reverb_level(val);

        auto it = reverb_channels.find(channel);
        if (it == reverb_channels.end())
        {
            it = channels.find(channel);
            if (it != channels.end() && it->second)
            {
                AeonWave::remove(*it->second);
                reverb.add(*it->second);
                reverb_channels[it->first] = it->second;
            }
        }
    }
    else
    {
        auto it = reverb_channels.find(channel);
        if (it != reverb_channels.end() && it->second)
        {
            reverb.remove(*it->second);
            AeonWave::add(*it->second);
        }
    }
}

void MIDIDriver::set_reverb_cutoff(uint8_t channel, float value) {
    midi.channel(channel).set_reverb_cutoff(value);
}
void MIDIDriver::set_reverb_time_rt60(uint8_t channel, float value) {
    midi.channel(channel).set_reverb_time_rt60(value);
}
void MIDIDriver::set_reverb_delay_depth(uint8_t channel, float value) {
    midi.channel(channel).set_reverb_delay_depth(value);
}
void MIDIDriver::set_reverb_decay_level(uint8_t channel, float value) {
    midi.channel(channel).set_reverb_decay_level(value);
}

/*
 * Create map of instrument banks and program numbers with their associated
 * file names from the XML files for a quick access during playback.
 */
void
MIDIDriver::read_instruments(std::string gmmidi, std::string gmdrums)
{
    const char *filename, *type = "instrument";
    auto imap = instruments;

    std::string iname;
    if (!gmmidi.empty())
    {
        iname = gmmidi;

        struct stat buffer;
        if (stat(iname.c_str(), &buffer) != 0)
        {
           iname = path;
           iname.append("/");
           iname.append(gmmidi);
        }
    } else {
        iname = path;
        iname.append("/");
        iname.append(instr);
    }

    filename = iname.c_str();
    for(unsigned int id=0; id<2; ++id)
    {
        void *xid = xmlOpen(filename);
        if (xid)
        {
            void *xaid = xmlNodeGet(xid, "aeonwave");
            void *xmid = nullptr;
            char file[64] = "";

            if (xaid)
            {
                if (xmlAttributeExists(xaid, "rate"))
                {
                    unsigned int rate = xmlAttributeGetInt(xaid, "rate");
                    if (rate >= 25 && rate <= 200) {
                       refresh_rate = rate;
                    }
                }
                if (xmlAttributeExists(xaid, "polyphony"))
                {
                    polyphony =  xmlAttributeGetInt(xaid, "polyphony");
                    if (polyphony < 32) polyphony = 32;
                }
                xmid = xmlNodeGet(xaid, "midi");
            }

            if (xmid)
            {
                if (xmlAttributeExists(xmid, "name"))
                {
                    char *set = xmlAttributeGetString(xmid, "name");
                    if (set && strlen(set) != 0) {
                        patch_set = set;
                    }
                    xmlFree(set);
                }

                if (xmlAttributeExists(xmid, "mode"))
                {
                   if (!xmlAttributeCompareString(xmid, "mode", "synthesizer"))
                   {
                       instrument_mode = AAX_RENDER_SYNTHESIZER;
                   }
                   else if (!xmlAttributeCompareString(xmid, "mode", "arcade"))
                   {
                       instrument_mode = AAX_RENDER_ARCADE;
                   }
                }

                if (xmlAttributeExists(xmid, "version"))
                {
                    char *set = xmlAttributeGetString(xmid, "version");
                    if (set && strlen(set) != 0) {
                        patch_version = set;
                    }
                    xmlFree(set);
                }

                if (xmlAttributeExists(xmid, "file")) {
                    effects = xmlAttributeGetString(xmid, "file");
                }

                unsigned int bnum = xmlNodeGetNum(xmid, "bank");
                void *xbid = xmlMarkId(xmid);
                for (unsigned int b=0; b<bnum; b++)
                {
                    if (xmlNodeGetPos(xmid, xbid, "bank", b) != 0)
                    {
                        unsigned int slen, inum = xmlNodeGetNum(xbid, type);
                        void *xiid = xmlMarkId(xbid);
                        unsigned int bank_no;

                        bank_no = xmlAttributeGetInt(xbid, "n") << 7;
                        bank_no += xmlAttributeGetInt(xbid, "l");

                        // bank audio-frame filter and effects file
                        slen = xmlAttributeCopyString(xbid, "file", file, 64);
                        if (slen)
                        {
                            file[slen] = 0;
                            frames.insert({bank_no,std::string(file)});
                        }

                        auto bank = imap[bank_no];
                        for (unsigned int i=0; i<inum; i++)
                        {
                            if (xmlNodeGetPos(xbid, xiid, type, i) != 0)
                            {
                                long int n = xmlAttributeGetInt(xiid, "n");
                                float spread = 1.0f;
                                int wide = 0;

                                if (simd64) {
                                    wide = xmlAttributeGetInt(xiid, "wide");
                                }
                                if (!wide && xmlAttributeGetBool(xiid, "wide"))
                                {
                                    wide = 1;
                                }
                                if (xmlAttributeExists(xiid, "spread")) {
                                   spread = xmlAttributeGetDouble(xiid, "spread");
                                }

                                // instrument file-name
                                slen = xmlAttributeCopyString(xiid, "file",
                                                              file, 64);
                                if (slen)
                                {
                                    file[slen] = 0;
                                    bank.insert({n,{file,{wide,spread}}});

                                    _patch_t p;
                                    p.insert({0,{i,file}});

                                    patches.insert({file,p});
//                                  if (id == 0) printf("{%x, {%i, {%s, %i}}}\n", bank_no, n, file, wide);
                                }
                                else
                                {
                                    slen = xmlAttributeCopyString(xiid, "patch",
                                                                  file, 64);
                                    if (slen)
                                    {
                                        file[slen] = 0;
                                        bank.insert({n,{file,{wide,spread}}});

                                        add_patch(file);
                                    }
                                }
                            }
                        }
                        imap[bank_no] = bank;
                        xmlFree(xiid);
                    }
                }
                xmlFree(xbid);
                xmlFree(xmid);
                xmlFree(xaid);
            }
            else {
                ERROR("aeonwave/midi not found in: " << filename);
            }
            xmlClose(xid);
        }
        else {
            ERROR("Unable to open: " << filename);
        }

        if (id == 0)
        {
            instruments = std::move(imap);

            // next up: drums
            if (!gmdrums.empty())
            {
                iname = gmdrums;

                struct stat buffer;
                if (stat(iname.c_str(), &buffer) != 0)
                {
                   iname = path;
                   iname.append("/");
                   iname.append(gmmidi);
                }
            } else {
                iname = path;
                iname.append("/");
                iname.append(drum);
            }
            filename = iname.c_str();
            type = "drum";
            imap = drums;
        }
        else {
            drums = std::move(imap);
        }
    }
}

void
MIDIDriver::add_patch(const char *file)
{
    const char *path = midi.info(AAX_SHARED_DATA_DIR);

    std::string xmlfile(path);
    xmlfile.append("/");
    xmlfile.append(file);
    xmlfile.append(".xml");

    void *xid = xmlOpen(xmlfile.c_str());
    if (xid)
    {
        void *xlid = xmlNodeGet(xid, "instrument/layer");
        if (xlid)
        {
            unsigned int pnum = xmlNodeGetNum(xlid, "patch");
            void *xpid = xmlMarkId(xlid);
            _patch_t p;
            for (unsigned int i=0; i<pnum; i++)
            {
                if (xmlNodeGetPos(xlid, xpid, "patch", i) != 0)
                {
                    unsigned int slen;
                    char file[64] = "";

                    slen = xmlAttributeCopyString(xpid, "file", file, 64);
                    if (slen)
                    {
                        uint8_t max = xmlAttributeGetInt(xpid, "max");
                        file[slen] = 0;

                        p.insert({max,{i,file}});
                    }
                }
            }
            patches.insert({file,p});

            xmlFree(xpid);
            xmlFree(xlid);
        }
        xmlFree(xid);
    }
}

/*
 * For drum mapping the program_no is stored in the bank number of the map
 * and the key_no in the program number of the map.
 */
const inst_t
MIDIDriver::get_drum(uint16_t program_no, uint8_t key_no, bool all)
{
    uint16_t prev_program_no = program_no;
    uint16_t req_program_no = program_no;
    do
    {
        auto itb = drums.find(program_no << 7);
        bool bank_found = (itb != drums.end());
        if (bank_found)
        {
            auto bank = itb->second;
            auto iti = bank.find(key_no);
            if (iti != bank.end())
            {
                if (all || selection.empty() || std::find(selection.begin(), selection.end(), iti->second.first) != selection.end()) {
                    return iti->second;
                } else {
                    return empty_map;
                }
            }
        }

        if (!prev_program_no && !program_no) {
            break;
        }
        prev_program_no = program_no;

        if ((program_no % 10) == 0) {
            program_no = 0;
        } else if ((program_no & 0xF8) == program_no) {
            program_no -= (program_no % 10);
        } else {
            program_no &= 0xF8;
        }

        if (bank_found) {
            DISPLAY(4, "Drum %i not found in bank %i, trying bank: %i\n",
                    key_no,  prev_program_no, program_no);
        } else if (!is_avail(missing_drum_bank, prev_program_no)) {
            DISPLAY(4, "Drum bank %i not found, trying %i\n",
                        prev_program_no, program_no);
            missing_drum_bank.push_back(prev_program_no);
        }
    }
    while (true);

    auto itb = drums.find(req_program_no << 7);
    auto& bank = itb->second;
    bank.insert({key_no,empty_map});

    return empty_map;
}

const inst_t
MIDIDriver::get_instrument(uint16_t bank_no, uint8_t program_no, bool all)
{
    uint16_t prev_bank_no = bank_no;
    uint16_t req_bank_no = bank_no;

    do
    {
        auto itb = instruments.find(bank_no);
        bool bank_found = (itb != instruments.end());
        if (bank_found)
        {
            auto bank = itb->second;
            auto iti = bank.find(program_no);
            if (iti != bank.end())
            {
                if (all || selection.empty() || std::find(selection.begin(), selection.end(), iti->second.first) != selection.end()) {
                    return iti->second;
                } else {
                    return empty_map;
                }
            }
        }

        if (!prev_bank_no && !bank_no) {
            break;
        }
        prev_bank_no = bank_no;

        switch (midi.get_mode())
        {
        case MIDI_EXTENDED_GENERAL_MIDI:
            if (bank_no & 0x7F) {          // switch to MSB only
                bank_no &= ~0x7F;
            } else if (bank_no > 0) {      // fall back to bank-0, (GM-MIDIDriver 1)
                bank_no = 0;
            }
        case MIDI_GENERAL_STANDARD:
            // Some sequencers mistakenly set the LSB for GS-MIDIDriver
            if (bank_no & 0x7F) {
                 bank_no = (bank_no & 0x7F) << 7;
            } else if (bank_no > 0) {      // fall back to bank-0, (GM-MIDIDriver 1)
                bank_no = 0;
            }
            break;
        default: // General MIDIDriver or unspecified
            if (bank_no & 0x7F) {          // LSB (XG-MIDIDriver)
                bank_no &= ~0x7F;
            } else if (bank_no & 0x3F80) { // MSB (GS-MIDIDriver / GM-MIDIDriver 2)
                bank_no &= ~0x3F80;
            } else if (bank_no > 0) {      // fall back to bank-0, (GM-MIDIDriver 1)
                bank_no = 0;
            }
            break;
        }

        if (bank_found) {
            DISPLAY(4, "Instrument %i not found in bank %i/%i, trying: %i/%i\n",
                     program_no, prev_bank_no >> 7, prev_bank_no & 0x7F,
                     bank_no >> 7, bank_no & 0x7F);
        } else if (!is_avail(missing_instrument_bank, prev_bank_no)) {
            DISPLAY(4, "Instrument bank %i/%i not found, trying %i/%i\n",
                        prev_bank_no >> 7, prev_bank_no & 0x7F,
                        bank_no >> 7, bank_no & 0x7F);
            missing_instrument_bank.push_back(prev_bank_no);
        }
    }
    while (true);

    auto itb = instruments.find(req_bank_no);
    auto& bank = itb->second;
    bank.insert({program_no,empty_map});

    return empty_map;
}

void
MIDIDriver::grep(std::string& filename, const char *grep)
{
    if (midi.get_csv()) return;

    std::string s(grep);
    std::regex regex{R"(,+)"}; // split on a comma
    std::sregex_token_iterator it{s.begin(), s.end(), regex, -1};
    auto selection = std::vector<std::string>{it, {}};

    bool found = false;
    for (auto it : loaded)
    {
        for (auto greps : selection)
        {
            if (it.find(greps) != std::string::npos)
            {
                if (!found) {
                    printf("%s found:\n", filename.c_str());
                    found =  true;
                }
                printf("    %s\n", it.c_str());
            }
        }
    }
}

MIDIInstrument&
MIDIDriver::new_channel(uint8_t track_no, uint16_t bank_no, uint8_t program_no)
{
    bool drums = is_drums(track_no);
    auto it = channels.find(track_no);
    if (!drums && it != channels.end())
    {
        if (it->second) AeonWave::remove(*it->second);
        channels.erase(it);
    }

    int level = 0;
    std::string name = "";
    if (drums && !frames.empty())
    {
        auto it = frames.find(program_no);
        if (it != frames.end()) {
            level = it->first;
            name = it->second;
        }
    }

    Buffer& buffer = midi.buffer(name, level);
    if (buffer) {
        buffer.set(AAX_CAPABILITIES, int(instrument_mode));
    }

    it = channels.find(track_no);
    if (it == channels.end())
    {
        try {
            auto ret = channels.insert(
                { track_no, std::shared_ptr<MIDIInstrument>(
                                    new MIDIInstrument(*this, buffer, track_no,
                                              bank_no, program_no, drums))
                } );
            it = ret.first;
            AeonWave::add(*it->second);
        } catch(const std::invalid_argument& e) {
            throw(e);
        }
    }

    MIDIInstrument& rv = *it->second;
    rv.set_program_no(program_no);
    rv.set_bank_no(bank_no);

    char *env = getenv("AAX_KEY_FINISH");
    if (env && atoi(env)) {
        rv.set_key_finish(true);
    }

    return rv;
}

MIDIInstrument&
MIDIDriver::channel(uint16_t track_no)
{
    auto it = channels.find(track_no);
    if (it != channels.end()) {
        return *it->second;
    }
    return new_channel(track_no, 0, 0);
}

/**
 * Note Off messages are ignored on Rhythm Channels, with the exception of the
 * ORCHESTRA SET (specifically, Note number 88) and the SFX SET
 * (Note numbers 47-84).
 *
 * Some percussion timbres require a mutually exclusive Note On/Off assignment.
 * For example, when a Note On message for Note number 42 (Closed Hi Hat) is
 * received while Note number 46 (Open Hi Hat) is sounding, Note number 46 is
 * promptly muted and Note number 42 sounds.
 *
 * <Standard Set> (1)
 * Scratch Push(29)  | Scratch Pull(30)
 * Closed HH(42)     | Pedal HH(44)     | Open HH(46)
 * Short Whistle(71) | Long Whistle(72)
 * Short Guiro(73)   | Long Guiro(74)
 * Mute Cuica(78)    | Open Cuica(79)
 * Mute Triangle(80) | Open Triangle(81)
 * Mute Surdo(86)    | Open Surdo(87)
 *
 * <Analog Set> (26)
 * Analog CHH 1(42) | Analog C HH 2(44) | Analog OHH (46)
 *
 * <Orchestra Set> (49)
 * Closed HH 2(27) | Pedal HH (28) | Open HH 2 (29)
 *
 * <SFX Set> (57)
 * Scratch Push(41) | Scratch Pull (42)
 */
bool
MIDIDriver::process(uint8_t track_no, uint8_t message, uint8_t key, uint8_t velocity, bool omni, float pitch)
{
    // Omni mode: Device responds to MIDIDriver data regardless of channel
    if (message == MIDI_NOTE_ON && velocity) {
        if (is_track_active(track_no)) {
            try {
                channel(track_no).play(key, velocity, pitch);
            } catch (const std::runtime_error &e) {
                throw(e);
            }
        }
    }
    else
    {
        if (message == MIDI_NOTE_ON) {
            velocity = 64;
        }
        channel(track_no).stop(key, velocity);
    }
    return true;
}

