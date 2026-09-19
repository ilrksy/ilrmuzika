#pragma once
#include <vector>
#include <string>
#include <cmath>

namespace muisc {

enum class EqPreset {
    Flat = 0,
    BassBoost = 1,
    VocalBoost = 2,
    TrebleBoost = 3,
    Electronic = 4,
    Count = 5
};

class BiquadFilter {
public:
    BiquadFilter();
    void reset();
    void set_low_shelf(float sample_rate, float cutoff_freq, float gain_db, float q = 0.707f);
    void set_high_shelf(float sample_rate, float cutoff_freq, float gain_db, float q = 0.707f);
    void set_peaking(float sample_rate, float center_freq, float gain_db, float q = 1.0f);
    
    // Process a single sample
    inline float process(float in) {
        float out = b0_ * in + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        x2_ = x1_;
        x1_ = in;
        y2_ = y1_;
        y1_ = out;
        return out;
    }

private:
    float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f;
    float a1_ = 0.0f, a2_ = 0.0f;
    float x1_ = 0.0f, x2_ = 0.0f;
    float y1_ = 0.0f, y2_ = 0.0f;
};

class DspEqualizer {
public:
    DspEqualizer(float sample_rate = 44100.0f);

    void set_sample_rate(float sample_rate);
    void set_preset(EqPreset preset);
    void cycle_preset();
    EqPreset preset() const { return current_preset_; }
    const char* preset_name() const;
    static const char* preset_name(EqPreset p);

    // Process interleaved audio buffer in-place
    void process_interleaved(float* samples, size_t frame_count, int channels);

private:
    void configure_filters();

    float sample_rate_ = 44100.0f;
    EqPreset current_preset_ = EqPreset::Flat;

    // Up to 2 filter stages per channel (Left, Right)
    BiquadFilter filter_l1_;
    BiquadFilter filter_l2_;
    BiquadFilter filter_r1_;
    BiquadFilter filter_r2_;
};

} // namespace muisc
