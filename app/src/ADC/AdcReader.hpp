//
// Created by bened on 11/10/2025.
//

#pragma once

#include <array>
#include <functional>

#include <dsp/fast_math_functions.h>
#include <dsp/statistics_functions.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

#include "Constants.hpp"
#include "Utils/Logger.hpp"
#include "Utils/PeriodicTimer.hpp"

namespace Adc
{
class AdcReader final
{
public:
    using NotifyFrameReady =
        std::function<void(int sample_rate_hz,
                           const std::array<float, Constants::SamplingFrameSize>& frame)>;

    explicit AdcReader(const adc_dt_spec* spec, Utils::PeriodicTimer& timer, Utils::Logger& logger)
        : spec_(spec), timer_(timer), logger_(logger)
    {
        sequence_ = {
            .buffer = &sample_buffer_,
            .buffer_size = sizeof(sample_buffer_),
        };
    }

    int Initialize(const int interval_us)
    {
        sample_interval_us_ = interval_us;
        sample_rate_hz_ = (1'000'000 + interval_us / 2) / interval_us;
        logger_.info("Initializing ADC reader (interval=%d us, sample_rate=%d Hz).",
                     sample_interval_us_, sample_rate_hz_);

        k_mutex_init(&mutex_);
        const int setup_result = adc_channel_setup_dt(spec_);
        adc_sequence_init_dt(spec_, &sequence_);
        if (setup_result != 0)
        {
            logger_.error("Adc initialization failed with error: %d.", setup_result);
            return setup_result;
        }

        timer_.init([this] { ReadSample(); });
        logger_.info("Adc initialized.");
        return 0;
    }

    void Start(const NotifyFrameReady& notify)
    {
        notify_frame_ready_ = notify;
        if (!notify_frame_ready_)
        {
            logger_.warning("ADC reader started without a frame callback.");
        }
        timer_.start(sample_interval_us_);
        logger_.info("Adc started (interval=%d us).", sample_interval_us_);
    }

private:
    void ReadFrame()
    {
        arm_mean_f32(frame_.data(), frame_.size(), &offset_);

        k_mutex_lock(&mutex_, K_FOREVER);
        if (notify_frame_ready_)
        {
            notify_frame_ready_(sample_rate_hz_, frame_);
            ++frames_emitted_;
            if (frames_emitted_ % 100 == 0)
            {
                logger_.debug("Published %d ADC frames.", static_cast<int>(frames_emitted_));
            }
        }
        k_mutex_unlock(&mutex_);
    }

    void ReadSample()
    {
        if (sample_count_ >= frame_.size())
        {
            ReadFrame();
            sample_count_ = 0;
        }

        const int read_result = adc_read_dt(spec_, &sequence_);
        if (read_result != 0)
        {
            logger_.error("ADC reading failed with error: %d.", read_result);
            return;
        }

        int value_uv = static_cast<int>(sample_buffer_);
        const int conv_result = adc_raw_to_microvolts_dt(spec_, &value_uv);
        if (conv_result != 0)
        {
            logger_.error("value in mV not available: %d.", conv_result);
            return;
        }

        if (k_mutex_lock(&mutex_, K_NO_WAIT) == 0)
        {
            frame_[sample_count_] = value_uv / 1'000'000.0f - offset_;
            k_mutex_unlock(&mutex_);
        }
        else
        {
            ++dropped_samples_;
            if (dropped_samples_ % 256 == 0)
            {
                logger_.warning("Dropped %d ADC samples because frame buffer was busy.",
                                static_cast<int>(dropped_samples_));
            }
        }

        ++sample_count_;
    }

    const adc_dt_spec* spec_;
    Utils::PeriodicTimer& timer_;
    Utils::Logger& logger_;

    NotifyFrameReady notify_frame_ready_{};
    adc_sequence sequence_{};
    uint16_t sample_buffer_{};
    size_t sample_count_{};
    std::array<float, Constants::SamplingFrameSize> frame_{};
    k_mutex mutex_{};
    int sample_interval_us_{};
    int sample_rate_hz_{};
    float offset_{0.0f};
    size_t frames_emitted_{0};
    size_t dropped_samples_{0};
};
} // namespace Adc
