//
// Created by bened on 19/10/2025.
//

#pragma once

#include "Threads/SubscriberBase.hpp"
#include "Utils/Logger.hpp"
#include "SignalProcessing/LpFilter.hpp"
#include "SignalProcessing/FftProcessor.hpp"
#include "SignalProcessing/BeatDetector.hpp"
#include "arm_math.h"

using namespace SignalProcessing;

namespace Threads
{
    template <size_t FrameSize>
    class AudioProcessing final : SubscriberBase<AudioFrame>
    {
    public:
        AudioProcessing(MessageBus &message_bus, ThreadWorker &threadWorker, LpFilter<FrameSize>& filter,
            FftProcessor<FrameSize>& fftProcessor, BeatDetector<FrameSize, 2>& beatDetector, Logger &logger)
        : SubscriberBase(message_bus, threadWorker), logger_(logger), filter_(filter),
          fftProcessor_(fftProcessor), beatDetector_(beatDetector)
    {
        }

        void Initialize(const int prio) override
        {
            SubscriberBase::Initialize(prio);
            timing_init();
            this->filter_.Initialize();
            this->beatDetector_.Initialize(10000, {40, 130}, {300, 750});
            auto ret = this->fftProcessor_.Initialize();
            if (ret != ARM_MATH_SUCCESS)
            {
                this->logger_.error("FFT module could not be initialized: %d.", ret);
                return;
            }

            this->logger_.info("Processing module initialized.");
        }

        void Start() override
        {
            SubscriberBase::Start();
            timing_start();
            this->logger_.info("Processing module started.");
        }

    protected:
        void Notify(AudioFrame& event) override
        {
            /* Filter */
            this->filter_.Process(event.samples, this->filterOut_);
            /* Apply FFT */
            this->fftProcessor_.Process(this->filterOut_, this->fft_power);

            /* Apply Beat detection */
            array<bool, 2> flags;
            auto beat = this->beatDetector_.Process(this->fft_power, flags);
            if (!beat)
            {
                return;
            }

            auto beatEvent = BeatEvent();
            auto timestamp = Timestamp();
            timestamp.nSec = timing_cycles_to_ns(timing_counter_get());
            beatEvent.ts = timestamp;
            beatEvent.bands = flags;
            message_bus.Publish(beatEvent);
        }

        Logger& logger_;
        LpFilter<FrameSize>& filter_;
        FftProcessor<FrameSize>& fftProcessor_;
        BeatDetector<FrameSize, 2>& beatDetector_;

        array<float, FrameSize> filterOut_{};
        array<float, FrameSize/2> fft_power{};
    };
}
