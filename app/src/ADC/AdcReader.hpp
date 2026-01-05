//
// Created by bened on 11/10/2025.
//

#pragma once

#include <array>
#include <dsp/fast_math_functions.h>
#include <dsp/statistics_functions.h>

#include "Utils/Logger.hpp"
#include "Utils/PeriodicTimer.hpp"
#include "zephyr/kernel.h"
#include "zephyr/drivers/adc.h"
#include "zephyr/timing/timing.h"

using namespace std;
using namespace Utils;

namespace Adc
{
    class AdcReader
    {
    public:
        using NotifyFrameReady = function<void(int sampleRate_hz, array<float, Constants::SamplingFrameSize>& out)>;

        explicit AdcReader(const adc_dt_spec* spec, PeriodicTimer& timer, Logger& logger)
            : _spec(spec), _timer(timer), logger_(logger)
        {
            this->_sequence = {
                .buffer = &this->_sample_buffer,
                .buffer_size = sizeof(this->_sample_buffer)
            };
        }

        int Initialize(const int interval_us)
        {
            this->sampleInterval_us = interval_us;
            this->sampleRate_hz = (1'000'000 + interval_us / 2) / interval_us;

            k_mutex_init(&this->_mutex);
            const auto res = adc_channel_setup_dt(this->_spec);
            adc_sequence_init_dt(this->_spec, &this->_sequence);
            if (res != 0)
            {
                this->logger_.error("Adc initialization failed with error: %d.", res);
                return res;
            }

            this->_timer.init([this] { ReadSample(); });

            timing_init();

            this->logger_.info("Adc initialized.");
            return res;
        }

        void Start(const NotifyFrameReady& notify)
        {
            this->_notifyFrameReady = notify;
            this->_timer.start(this->sampleInterval_us);
            timing_start();
            this->logger_.info("Adc started.");
        }

        void ReadFrame()
        {
            arm_mean_f32(this->_frame.data(), this->_frame.size(), &this->offset_);

            k_mutex_lock(&this->_mutex, K_FOREVER);
            this->_notifyFrameReady(this->sampleRate_hz, this->_frame);
            k_mutex_unlock(&this->_mutex);
        }

    private:
        void ReadSample()
        {
            // auto now = timing_counter_get();
            // this->logger_.info("sample time: %d", timing_cycles_to_ns(timing_cycles_get(&this->_timing, &now)));
            // this->_timing = now;

            if (this->_sample_count >= _frame.size())
            {
                ReadFrame();
                this->_sample_count = 0;
            }

            auto value = static_cast<int>(_sample_buffer);
            if (adc_read_dt(this->_spec, &this->_sequence) < 0)
            {
                logger_.error("ADC reading failed with error: %d.", value);
            }

            if (adc_raw_to_microvolts_dt(this->_spec, &value) < 0)
            {
                logger_.error("value in mV not available: %d.", value);
            }

            k_mutex_lock(&this->_mutex, K_NO_WAIT);
            this->_frame[this->_sample_count] = value / 1'000'000 - this->offset_;
            k_mutex_unlock(&this->_mutex);

            ++this->_sample_count;
        }

        const adc_dt_spec* _spec;
        PeriodicTimer& _timer;
        Logger& logger_;

        NotifyFrameReady _notifyFrameReady{};
        adc_sequence _sequence{};
        uint16_t _sample_buffer{};
        size_t _sample_count{};
        adc_sequence_options _sequence_options{};
        array<float, Constants::SamplingFrameSize> _frame{};
        k_mutex _mutex{};
        int sampleInterval_us{};
        int sampleRate_hz{};

        bool calibrated{false};
        float offset_ = 0;
        timing_t _timing{};
    };
}
