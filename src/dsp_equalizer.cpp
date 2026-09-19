#include "dsp_equalizer.h"
#include <algorithm>

namespace muisc {

static constexpr float kPi = 3.14159265358979323846f;

BiquadFilter::BiquadFilter() {
    reset();
}

void BiquadFilter::reset() {
    b0_ = 1.0f;
    b1_ = 0.0f;
    b2_ = 0.0f;
    a1_ = 0.0f;
    a2_ = 0.0f;
    x1_ = 0.0f;
    x2_ = 0.0f;
    y1_ = 0.0f;
    y2_ = 0.0f;
}

void BiquadFilter::set_low_shelf(float sample_rate, float cutoff_freq, float gain_db, float q) {
    if (sample_rate <= 0.0f) sample_rate = 44100.0f;
    cutoff_freq = std::clamp(cutoff_freq, 10.0f, sample_rate * 0.49f);
    if (std::abs(gain_db) < 0.05f) {
        reset();
        return;
    }

    float a = std::pow(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * kPi * cutoff_freq / sample_rate;
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alpha = (sinw0 / 2.0f) * std::sqrt((a + 1.0f / a) * (1.0f / q - 1.0f) + 2.0f);
    float beta = 2.0f * std::sqrt(a) * alpha;

    float b0 = a * ((a + 1.0f) - (a - 1.0f) * cosw0 + beta);
    float b1 = 2.0f * a * ((a - 1.0f) - (a + 1.0f) * cosw0);
    float b2 = a * ((a + 1.0f) - (a - 1.0f) * cosw0 - beta);
    float a0 = (a + 1.0f) + (a - 1.0f) * cosw0 + beta;
    float a1 = -2.0f * ((a - 1.0f) + (a + 1.0f) * cosw0);
    float a2 = (a + 1.0f) + (a - 1.0f) * cosw0 - beta;

    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
    a1_ = a1 / a0;
    a2_ = a2 / a0;
}

void BiquadFilter::set_high_shelf(float sample_rate, float cutoff_freq, float gain_db, float q) {
    if (sample_rate <= 0.0f) sample_rate = 44100.0f;
    cutoff_freq = std::clamp(cutoff_freq, 10.0f, sample_rate * 0.49f);
    if (std::abs(gain_db) < 0.05f) {
        reset();
        return;
    }

    float a = std::pow(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * kPi * cutoff_freq / sample_rate;
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alpha = (sinw0 / 2.0f) * std::sqrt((a + 1.0f / a) * (1.0f / q - 1.0f) + 2.0f);
    float beta = 2.0f * std::sqrt(a) * alpha;

    float b0 = a * ((a + 1.0f) + (a - 1.0f) * cosw0 + beta);
    float b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosw0);
    float b2 = a * ((a + 1.0f) + (a - 1.0f) * cosw0 - beta);
    float a0 = (a + 1.0f) - (a - 1.0f) * cosw0 + beta;
    float a1 = 2.0f * ((a - 1.0f) - (a + 1.0f) * cosw0);
    float a2 = (a + 1.0f) - (a - 1.0f) * cosw0 - beta;

    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
    a1_ = a1 / a0;
    a2_ = a2 / a0;
}

void BiquadFilter::set_peaking(float sample_rate, float center_freq, float gain_db, float q) {
    if (sample_rate <= 0.0f) sample_rate = 44100.0f;
    center_freq = std::clamp(center_freq, 10.0f, sample_rate * 0.49f);
    if (std::abs(gain_db) < 0.05f) {
        reset();
        return;
    }

    float a = std::pow(10.0f, gain_db / 40.0f);
    float w0 = 2.0f * kPi * center_freq / sample_rate;
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alpha = sinw0 / (2.0f * q);

    float b0 = 1.0f + alpha * a;
    float b1 = -2.0f * cosw0;
    float b2 = 1.0f - alpha * a;
    float a0 = 1.0f + alpha / a;
    float a1 = -2.0f * cosw0;
    float a2 = 1.0f - alpha / a;

    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
    a1_ = a1 / a0;
    a2_ = a2 / a0;
}

DspEqualizer::DspEqualizer(float sample_rate) : sample_rate_(sample_rate) {
    configure_filters();
}

void DspEqualizer::set_sample_rate(float sample_rate) {
    if (sample_rate > 0.0f && sample_rate != sample_rate_) {
        sample_rate_ = sample_rate;
        configure_filters();
    }
}

void DspEqualizer::set_preset(EqPreset preset) {
    current_preset_ = preset;
    configure_filters();
}

void DspEqualizer::cycle_preset() {
    int next = (static_cast<int>(current_preset_) + 1) % static_cast<int>(EqPreset::Count);
    set_preset(static_cast<EqPreset>(next));
}

const char* DspEqualizer::preset_name() const {
    return preset_name(current_preset_);
}

const char* DspEqualizer::preset_name(EqPreset p) {
    switch (p) {
        case EqPreset::Flat: return "Flat";
        case EqPreset::BassBoost: return "Bass Boost";
        case EqPreset::VocalBoost: return "Vocal Boost";
        case EqPreset::TrebleBoost: return "Treble Boost";
        case EqPreset::Electronic: return "Electronic";
        default: return "Flat";
    }
}

void DspEqualizer::configure_filters() {
    filter_l1_.reset();
    filter_l2_.reset();
    filter_r1_.reset();
    filter_r2_.reset();

    switch (current_preset_) {
        case EqPreset::Flat:
            // All reset to identity
            break;
        case EqPreset::BassBoost:
            // +6 dB low-shelf @ 110Hz, Q=0.9
            filter_l1_.set_low_shelf(sample_rate_, 110.0f, 6.0f, 0.9f);
            filter_r1_.set_low_shelf(sample_rate_, 110.0f, 6.0f, 0.9f);
            break;
        case EqPreset::VocalBoost:
            // +4.5 dB peaking @ 1800Hz, Q=1.0
            filter_l1_.set_peaking(sample_rate_, 1800.0f, 4.5f, 1.0f);
            filter_r1_.set_peaking(sample_rate_, 1800.0f, 4.5f, 1.0f);
            break;
        case EqPreset::TrebleBoost:
            // +5.5 dB high-shelf @ 7500Hz, Q=0.8
            filter_l1_.set_high_shelf(sample_rate_, 7500.0f, 5.5f, 0.8f);
            filter_r1_.set_high_shelf(sample_rate_, 7500.0f, 5.5f, 0.8f);
            break;
        case EqPreset::Electronic:
            // V-shaped curve: +5 dB bass @ 90Hz, +4.5 dB treble @ 8000Hz
            filter_l1_.set_low_shelf(sample_rate_, 90.0f, 5.0f, 0.85f);
            filter_r1_.set_low_shelf(sample_rate_, 90.0f, 5.0f, 0.85f);
            filter_l2_.set_high_shelf(sample_rate_, 8000.0f, 4.5f, 0.85f);
            filter_r2_.set_high_shelf(sample_rate_, 8000.0f, 4.5f, 0.85f);
            break;
        default:
            break;
    }
}

void DspEqualizer::process_interleaved(float* samples, size_t frame_count, int channels) {
    if (current_preset_ == EqPreset::Flat || !samples || frame_count == 0) return;

    if (channels == 2) {
        for (size_t i = 0; i < frame_count; ++i) {
            float l = samples[i * 2];
            float r = samples[i * 2 + 1];

            l = filter_l1_.process(l);
            l = filter_l2_.process(l);

            r = filter_r1_.process(r);
            r = filter_r2_.process(r);

            // Soft limiter / clamping to prevent harsh clipping
            samples[i * 2] = std::clamp(l, -1.0f, 1.0f);
            samples[i * 2 + 1] = std::clamp(r, -1.0f, 1.0f);
        }
    } else if (channels == 1) {
        for (size_t i = 0; i < frame_count; ++i) {
            float s = samples[i];
            s = filter_l1_.process(s);
            s = filter_l2_.process(s);
            samples[i] = std::clamp(s, -1.0f, 1.0f);
        }
    }
}

} // namespace muisc
