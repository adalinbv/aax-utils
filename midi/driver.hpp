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

#ifndef __AAX_MIDIDriverDRIVER
#define __AAX_MIDIDriverDRIVER

#include <sys/stat.h>
#include <climits>

#include <map>

#include <midi/shared.hpp>

namespace aax
{

enum {
    MIDI_POLYPHONIC = 3,
    MIDI_MONOPHONIC
};

struct wide_t
{
    int wide;
    float spread;
};

using inst_t = std::pair<std::string,struct wide_t>;

class MIDIInstrument;


class MIDIDriver : public AeonWave
{
private:
    using _patch_t = std::map<uint8_t,std::pair<uint8_t,std::string>>;
    using _channel_map_t = std::map<uint16_t,std::shared_ptr<MIDIInstrument>>;

public:
    MIDIDriver(const char* n, const char *tnames = nullptr,
         enum aaxRenderMode m=AAX_MODE_WRITE_STEREO);
    MIDIDriver(const char* n, enum aaxRenderMode m=AAX_MODE_WRITE_STEREO) :
        MIDIDriver(n, nullptr, m) {}

    virtual ~MIDIDriver() {
        AeonWave::remove(reverb);
        for (auto it : buffers) {
            aaxBufferDestroy(*it.second.second); it.second.first = 0;
        }
    }

    bool process(uint8_t channel, uint8_t message, uint8_t key, uint8_t velocity, bool omni, float pitch=1.0f);

    MIDIInstrument& new_channel(uint8_t channel, uint16_t bank, uint8_t program);

    MIDIInstrument& channel(uint16_t channel_no);

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
    inline char get_verbose() { return csv ? 0 : verbose; }

    inline void set_csv(char v) { csv = v; }
    inline char get_csv() { return csv; }

    inline void set_lyrics(bool v) { lyrics = v; }
    inline bool get_lyrics() { return lyrics; }

    inline void set_format(uint16_t fmt) { format = fmt; }
    inline uint16_t get_format() { return format; }

    inline void set_tempo(uint32_t t) { tempo = t; uSPP = t/PPQN; }

    inline void set_uspp(uint32_t uspp) { uSPP = uspp; }
    inline int32_t get_uspp() { return uSPP; }

    inline void set_ppqn(uint16_t ppqn) { PPQN = ppqn; }
    inline uint16_t get_ppqn() { return PPQN; }

    /* chorus */
    void set_chorus(const char *t);
    void set_chorus_type(uint8_t value);
    void set_chorus_level(uint16_t part_no, float lvl);
    void set_chorus_depth(float depth);
    void set_chorus_feedback(float feedback);
    void set_chorus_rate(float rate);

    /* reverb */
    void set_reverb(const char *t);
    void set_reverb_type(uint8_t value);
    void set_reverb_cutoff_frequency(float value);
    void set_reverb_decay_level(float value);
    void set_reverb_decay_depth(float value);
    void set_reverb_time_rt60(float value);
    void set_reverb_delay_depth(float value);

    void set_reverb_level(uint16_t part_no, float value);

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

    MIDIDriver &midi = *this;
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

    std::vector<uint16_t> missing_drum_bank;
    std::vector<uint16_t> missing_instrument_bank;

    inline bool is_avail(std::vector<uint16_t>& vec, uint16_t item) {
        return (std::find(vec.begin(), vec.end(), item) != vec.end());
    }

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

    uint16_t PPQN = 24;
    uint32_t tempo = 500000;
    uint32_t uSPP = tempo/PPQN;
    uint16_t format = 0;

    enum aaxCapabilities instrument_mode = AAX_RENDER_NORMAL;
    uint8_t mode = MIDI_MODE0;
    bool initialize = false;
    char verbose = 0;
    bool lyrics = false;
    bool grep_mode = false;
    bool mono = false;
    bool csv = false;

    uint8_t reverb_type = 4;
    Param reverb_decay_level = 0.66f;
    Param reverb_decay_depth = 0.3f;
    Param reverb_cutoff_frequency = 790.0f;
    Status reverb_state = AAX_FALSE;
    aax::Mixer reverb = aax::Mixer(*this);
};

} // namespace aax


#endif
