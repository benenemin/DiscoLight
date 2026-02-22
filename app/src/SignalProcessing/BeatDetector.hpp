//
// Created by bened on 30/11/2025.
//

#pragma once

#include <algorithm>
#include <array>
#include <cmath>

#include <dsp/statistics_functions.h>

#include "Constants.hpp"
#include "FftProcessor.hpp"
#include "LpFilter.hpp"
#include "SignalProcessingBase.hpp"

namespace SignalProcessing
{
class BeatDetector final : public SignalProcessingBase
{
public:
    explicit BeatDetector(FftProcessor& fft_processor, LpFilter& filter)
        : filter_(filter), fft_processor_(fft_processor)
    {
        constexpr uint32_t band_size =
            (Constants::SamplingFrameSize / 2 != 0u) ? (10000 / Constants::SamplingFrameSize / 2) : 0u;
        lo_ = (band_size != 0u) ? (30 / band_size) : 0u;
        hi_ = (band_size != 0u) ? (140 / band_size) : 0u;
    }

    int Initialize() override
    {
        filter_.Initialize();
        return fft_processor_.Initialize();
    }

    bool Process(std::array<float, Constants::SamplingFrameSize>& samples) override
    {
        filter_.Process(samples, filter_output_);
        fft_processor_.Process(filter_output_, fft_power_);

        float acc = 0.0f;
        const size_t width = hi_ - lo_;
        for (size_t i = lo_; i < hi_; ++i)
        {
            acc += std::isnan(fft_power_[i]) ? 0.0f : fft_power_[i];
        }
        const float band_energy = width != 0 ? (acc / static_cast<float>(width)) : 0.0f;

        // Keep current auto-gain behavior.
        UpdateAutoGain(band_energy);
        return DetectBeat(band_energy);
    }

private:
    bool DetectBeat(const float energy)
    {
        beat_history_[beat_history_index_] = energy;
        beat_history_index_ = (beat_history_index_ + 1) % kHistorySize;

        beat_average_ = 0.0f;
        for (int i = 0; i < kHistorySize; ++i)
        {
            beat_average_ += beat_history_[i];
        }
        beat_average_ /= kHistorySize;

        beat_variance_ = 0.0f;
        for (int i = 0; i < kHistorySize; ++i)
        {
            const float diff = beat_history_[i] - beat_average_;
            beat_variance_ += diff * diff;
        }
        beat_variance_ /= kHistorySize;

        const auto threshold = beat_average_ + (beat_sensitivity_ * std::sqrt(beat_variance_));
        is_beat_ = energy > threshold && energy > 0.01f;
        const bool beat = is_beat_ && !was_beat_;
        was_beat_ = is_beat_;
        return beat;
    }

    void UpdateAutoGain(const float level)
    {
        static float target_level = 0.7f;
        static float avg_level = 0.0f;

        avg_level = avg_level * 0.95f + level * 0.05f;
        if (avg_level > 0.01f)
        {
            float gain_adjust = target_level / avg_level;
            gain_adjust = std::max(0.5f, std::min(gain_adjust, 2.0f));
            auto_gain_value_ = auto_gain_value_ * 0.9f + gain_adjust * 0.1f;
        }
    }

    static constexpr int kHistorySize = 40;
    float beat_history_[kHistorySize]{0.0f};
    int beat_history_index_{0};
    float beat_average_{0.0f};
    float beat_variance_{0.0f};
    bool is_beat_{false};
    bool was_beat_{false};
    float auto_gain_value_{1.0f};
    float beat_sensitivity_{0.9f};

    std::array<float, Constants::SamplingFrameSize> filter_output_{};
    std::array<float, Constants::SamplingFrameSize / 2> fft_power_{};
    LpFilter& filter_;
    FftProcessor& fft_processor_;

    size_t lo_{0};
    size_t hi_{0};
};
} // namespace SignalProcessing
