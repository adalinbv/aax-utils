/*
 * Copyright (C) 2018-2020 by Erik Hofman.
 * Copyright (C) 2018-2020 by Adalin B.V.
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

#ifndef __AAX_MIDI
#define __AAX_MIDI

#include <sys/stat.h>

#include <climits>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <memory>
#include <vector>
#include <map>

#include <aax/aeonwave.hpp>
#include <aax/instrument.hpp>
#include <aax/buffer_map.hpp>
#include <aax/byte_stream.hpp>
#include <aax/midi.h>


namespace aax
{
#define MIDI_DRUMS_CHANNEL		0x9
#define MIDI_FILE_FORMAT_MAX		0x3

enum {
    MIDI_MODE0 = 0,
    MIDI_GENERAL_MIDI1,
    MIDI_GENERAL_MIDI2,
    MIDI_GENERAL_STANDARD,
    MIDI_XG_MIDI,

    MIDI_MODE_MAX
};

enum {
    MIDI_POLYPHONIC = 3,
    MIDI_MONOPHONIC
};

struct param_t
{
   uint8_t coarse;
   uint8_t fine;
};

struct wide_t
{
    int wide;
    float spread;
};

using inst_t = std::pair<std::string,struct wide_t>;

class MIDIChannel;

class MIDI : public AeonWave
{
private:
    using _patch_t = std::map<uint8_t,std::pair<uint8_t,std::string>>;
    using _channel_map_t = std::map<uint16_t,std::shared_ptr<MIDIChannel>>;

public:
    MIDI(const char* n, const char *tnames = nullptr,
         enum aaxRenderMode m=AAX_MODE_WRITE_STEREO);
    MIDI(const char* n, enum aaxRenderMode m=AAX_MODE_WRITE_STEREO) :
        MIDI(n, nullptr, m) {}

    virtual ~MIDI() {
        AeonWave::remove(reverb);
        for (auto it : buffers) {
            aaxBufferDestroy(*it.second.second); it.second.first = 0;
        }
    }

    bool process(uint8_t channel, uint8_t message, uint8_t key, uint8_t velocity, bool omni, float pitch=1.0f);

    MIDIChannel& new_channel(uint8_t channel, uint16_t bank, uint8_t program);

    MIDIChannel& channel(uint8_t channel_no);

    inline _channel_map_t& channel() {
        return channels;
    }

    inline void set_drum_file(std::string p) { drum = p; }
    inline void set_instrument_file(std::string p) { instr = p; }
    inline void set_file_path(std::string p) {
        set(AAX_SHARED_DATA_DIR, p.c_str()); path = p;
    }

    inline const std::string& get_patch_set() { return patch_set; }
    inline const std::string& get_patch_version() { return patch_version; }
    inline std::vector<std::string>& get_selections() { return selection; }

    inline void set_track_active(uint16_t t) {
        active_track.push_back(t);
    }
    inline uint16_t no_active_tracks() { return active_track.size(); }
    inline bool is_track_active(uint16_t t) {
        return active_track.empty() ? true : std::find(active_track.begin(), active_track.end(), t) != active_track.end();
    }

    void read_instruments(std::string gmidi=std::string(), std::string gmdrums=std::string());

    void grep(std::string& filename, const char *grep);
    inline void load(std::string& name) { loaded.push_back(name); }

    void start();
    void stop();
    void rewind();

    void finish(uint8_t n);
    bool finished(uint8_t n);

    void set_gain(float);
    void set_balance(float);

    bool is_drums(uint8_t);

    inline void set_capabilities(enum aaxCapabilities m) {
        instrument_mode = m; set(AAX_CAPABILITIES, m); set_path();
    }

    inline std::string& get_effects() { return effects; }

    inline unsigned int get_refresh_rate() { return refresh_rate; }
    inline unsigned int get_polyphony() { return polyphony; }

    inline void set_tuning(float pitch) { tuning = powf(2.0f, pitch/12.0f); }
    inline float get_tuning() { return tuning; }

    inline void set_mode(uint8_t m) { if (m > mode) mode = m; }
    inline uint8_t get_mode() { return mode; }

    inline void set_grep(bool g) { grep_mode = g; }
    inline bool get_grep() { return grep_mode; }

    const inst_t get_drum(uint16_t program, uint8_t key, bool all=false);
    const inst_t get_instrument(uint16_t bank, uint8_t program, bool all=false);
    std::map<std::string,_patch_t>& get_patches() { return patches; }

    inline void set_initialize(bool i) { initialize = i; };
    inline bool get_initialize() { return initialize; }

    inline void set_mono(bool m) { mono = m; }
    inline bool get_mono() { return mono; }

    inline void set_verbose(char v) { verbose = v; }
    inline char get_verbose() { return verbose; }

    inline void set_lyrics(bool v) { lyrics = v; }
    inline bool get_lyrics() { return lyrics; }

    inline void set_format(uint16_t fmt) { format = fmt; }
    inline uint16_t get_format() { return format; }

    inline void set_tempo(uint32_t tempo) { uSPP = tempo/PPQN; }

    inline void set_uspp(uint32_t uspp) { uSPP = uspp; }
    inline int32_t get_uspp() { return uSPP; }

    inline void set_ppqn(uint16_t ppqn) { PPQN = ppqn; }
    inline uint16_t get_ppqn() { return PPQN; }

    void set_chorus(const char *t);
    void set_chorus_level(float lvl);
    void set_chorus_depth(float depth);
    void set_chorus_rate(float rate);

    void set_reverb(const char *t);
    void set_reverb_level(uint8_t channel, uint8_t value);
    void set_reverb_type(uint8_t value);
    inline void set_decay_depth(float rt) { reverb_decay_depth = 0.1f*rt; }

    // ** buffer management ******
    Buffer& buffer(std::string& name, int level=0) {
        if (level) { name = name + "?patch=" + std::to_string(level); }
        auto it = buffers.find(name);
        if (it == buffers.end()) {
            std::shared_ptr<Buffer> b(new Buffer(ptr,name.c_str(),false,true));
            if (b) {
                auto ret = buffers.insert({name,{0,b}});
                it = ret.first;
            } else {
                return nullBuffer;
            }
        }
        it->second.first++;
        return *it->second.second;
    }
    void destroy(Buffer& b) {
        for(auto it=buffers.begin(); it!=buffers.end(); ++it)
        {
            if ((*it->second.second == b) && it->second.first && !(--it->second.first)) {
                aaxBufferDestroy(*it->second.second);
                buffers.erase(it); break;
            }
        }
    }
    bool buffer_avail(std::string &name) {
        auto it = buffers.find(name);
        if (it == buffers.end()) return false;
        return true;
    }

    bool exists(const std::string& path) {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    MIDI &midi = *this;
    int capabilities = midi.get(AAX_CAPABILITIES);
    int cores = (capabilities & AAX_CPU_CORES)+1;
    int midi_mode = (capabilities & AAX_RENDER_MASK);
    int simd64 = (capabilities & AAX_SIMD256);
    int simd = (capabilities & AAX_SIMD);
private:
    void set_path();

    std::string preset_file(aaxConfig c, std::string& name) {
        std::string rv = midi.info(AAX_SHARED_DATA_DIR);
        rv.append("/"); rv.append(name);
        return rv;
    }

    std::string aaxs_file(aaxConfig c, std::string& name) {
        std::string rv = midi.info(AAX_SHARED_DATA_DIR);
        rv.append("/"); rv.append(name); rv.append(".aaxs");
        return rv;
    }

    void add_patch(const char *patch);

    std::string patch_set = "default";
    std::string patch_version = "1.0.0";

    std::string effects;
    std::string track_name;
    _channel_map_t channels;
    _channel_map_t reverb_channels;
    std::map<uint16_t,std::string> frames;

    std::map<uint16_t,std::map<uint16_t,inst_t>> drums;
    std::map<uint16_t,std::map<uint16_t,inst_t>> instruments;
    std::map<std::string,_patch_t> patches;

    std::unordered_map<std::string,std::pair<size_t,std::shared_ptr<Buffer>>> buffers;

    Buffer nullBuffer;

    std::vector<std::string> loaded;

    std::vector<std::string> selection;
    std::vector<uint16_t> active_track;

    inst_t empty_map = {"",{0,1.0f}};
    std::string instr = "gmmidi.xml";
    std::string drum = "gmdrums.xml";
    std::string path;

    float tuning = 1.0f;

    unsigned int refresh_rate = 0;
    unsigned int polyphony = UINT_MAX;

    uint32_t uSPP = 500000/24;
    uint16_t format = 0;
    uint16_t PPQN = 24;

    enum aaxCapabilities instrument_mode = AAX_RENDER_NORMAL;
    uint8_t mode = MIDI_MODE0;
    bool initialize = false;
    char verbose = 0;
    bool lyrics = false;
    bool grep_mode = false;
    bool mono = false;

    uint8_t reverb_type = 4;
    Param reverb_decay_depth = 0.15f;
    Param reverb_cutoff_frequency = 790.0f;
    Status reverb_state = AAX_FALSE;
    aax::Mixer reverb = aax::Mixer(*this);
};


class MIDIChannel : public Instrument
{
private:
    MIDIChannel(const MIDIChannel&) = delete;

    MIDIChannel& operator=(const MIDIChannel&) = delete;

public:
    MIDIChannel(MIDI& ptr, Buffer &buffer, uint8_t channel,
                uint16_t bank, uint8_t program, bool is_drums)
       : Instrument(ptr, channel == MIDI_DRUMS_CHANNEL), midi(ptr),
         channel_no(channel), bank_no(bank), program_no(program),
         drum_channel(channel == MIDI_DRUMS_CHANNEL ? true : is_drums)
    {
        if (drum_channel && buffer) {
           Mixer::add(buffer);
        }
        Mixer::set(AAX_PLAYING);
    }

    MIDIChannel(MIDIChannel&&) = default;

    virtual ~MIDIChannel() = default;

    MIDIChannel& operator=(MIDIChannel&&) = default;

    void play(uint8_t key_no, uint8_t velocity, float pitch);

    inline void set_drums(bool d = true) { drum_channel = d; }
    inline bool is_drums() { return drum_channel; }

    inline uint8_t get_channel_no() { return channel_no; }
    inline void set_channel_no(uint8_t channel) { channel_no = channel; }

    inline uint8_t get_program_no() { return program_no; }
    inline void set_program_no(uint8_t program) { program_no = program; }

    inline uint16_t get_bank_no() { return bank_no; }
    inline void set_bank_no(uint16_t bank) { bank_no = bank; }

    inline void set_tuning(float pitch) { tuning = powf(2.0f, pitch/12.0f); }
    inline float get_tuning() { return tuning; }

    inline void set_semi_tones(float s) { semi_tones = s; }
    inline float get_semi_tones() { return semi_tones; }

    inline void set_modulation_depth(float d) { modulation_range = d; }
    inline float get_modulation_depth() { return modulation_range; }

    inline bool get_pressure_volume_bend() { return pressure_volume_bend; }
    inline bool get_pressure_pitch_bend() { return pressure_pitch_bend; }
    inline float get_aftertouch_sensitivity() { return pressure_sensitivity; }

    inline void set_track_name(std::string& tname) { track_name = tname; }

private:
    std::pair<uint8_t,std::string> get_patch(std::string& name, uint8_t& key);

    std::map<uint8_t,Buffer&> name_map;
    std::string track_name;

    MIDI &midi;

    Buffer nullBuffer;

    float tuning = 1.0f;
    float modulation_range = 2.0f;
    float pressure_sensitivity = 1.0f;
    float semi_tones = 2.0f;

    uint16_t bank_no = 0;
    uint8_t channel_no = 0;
    uint8_t program_no = 0;

    bool drum_channel = false;
    bool pressure_volume_bend = true;
    bool pressure_pitch_bend = false;
};


class MIDITrack : public byte_stream
{
public:
    MIDITrack() = default;

    MIDITrack(MIDI& ptr, byte_stream& stream, size_t len,  uint16_t track)
        : byte_stream(stream, len), midi(ptr), channel_no(track)
    {
        timestamp_parts = pull_message()*24/600000;
    }

    MIDITrack(const MIDITrack&) = default;

    virtual ~MIDITrack() = default;

    void rewind();
    bool process(uint64_t, uint32_t&, uint32_t&);

    MIDI& midi;
private:
    inline float cents2pitch(float p, uint8_t channel) {
        float r = midi.channel(channel).get_semi_tones();
        return powf(2.0f, p*r/12.0f);
    }
    inline float cents2modulation(float p, uint8_t channel) {
        float r = midi.channel(channel).get_modulation_depth();
        return powf(2.0f, p*r/12.0f);
    }
    inline void toUTF8(std::string& text, uint8_t c) {
       if (c < 128) text += c;
       else { text += 0xc2+(c > 0xbf); text += (c & 0x3f)+0x80; }
    }

    uint32_t pull_message();
    bool registered_param(uint8_t, uint8_t, uint8_t);

    uint8_t mode = 0;
    uint8_t channel_no = 0;
    uint8_t program_no = 0;
    uint16_t bank_no = 0;
    int16_t track_no = -1;

    uint8_t previous = 0;
    uint32_t wait_parts = 1;
    uint64_t timestamp_parts = 0;
    bool polyphony = true;
    bool omni = true;

    bool registered = false;
    uint16_t msb_type = 0;
    uint16_t lsb_type = 0;
    struct param_t param[MAX_REGISTERED_PARAM+1] = {
        { 2, 0 }, { 0x40, 0 }, { 0x20, 0 }, { 0, 0 }, { 0, 0 }, { 1, 0 }
    };

    const std::string type_name[5] = {
        "Text", "Copyright", "Track", "Instrument", "Lyrics"
    };
    const std::string csv_name[5] = {
        "Text_t", "Copyright_t", "Title_t", "Instrument_name_t", "Lyrics_t"
    };
};


class MIDIFile : public MIDI
{
public:
    MIDIFile(const char *filename) : MIDIFile(nullptr, filename) {}

    MIDIFile(const char *devname, const char *filename, const char *track=nullptr, enum aaxRenderMode mode=AAX_MODE_WRITE_STEREO, const char *config=nullptr);

    explicit MIDIFile(std::string& devname, std::string& filename)
       :  MIDIFile(devname.c_str(), filename.c_str()) {}

    virtual ~MIDIFile() = default;

    inline operator bool() {
        return midi_data.capacity();
    }

    void initialize(const char *grep = nullptr);
    inline void start() { midi.start(); }
    inline void stop() { midi.stop(); }
    void rewind();

    inline float get_duration_sec() { return duration_sec; }
    inline float get_pos_sec() { return pos_sec; }

    bool process(uint64_t, uint32_t&);

private:
    std::string file;
    std::string gmmidi;
    std::string gmdrums;
    std::vector<uint8_t> midi_data;
    std::vector<std::shared_ptr<MIDITrack>> track;

    uint16_t no_tracks = 0;
    float duration_sec = 0.0f;
    float pos_sec = 0.0f;

    const std::string format_name[MIDI_FILE_FORMAT_MAX+1] = {
        "MIDI File 0", "MIDI File 1", "MIDI File 2",
        "Unknown MIDI File format"
    };
    const std::string mode_name[MIDI_MODE_MAX] = {
        "MIDI", "General MIDI 1.0", "General MIDI 2.0", "GS MIDI", "XG MIDI"
    };
};

} // namespace aax


#endif
