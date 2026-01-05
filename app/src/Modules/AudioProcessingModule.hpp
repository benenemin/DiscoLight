//
// Created by bened on 19/10/2025.
//

#pragma once

#include "Utils/Logger.hpp"
#include "SignalProcessing/BeatDetector.hpp"
#include "arm_math.h"
#include "Core/EventTypes.hpp"

using namespace SignalProcessing;

namespace Modules
{
    class AudioProcessingModule final
    {
    public:
        AudioProcessingModule(AppPublisher& publisher, AppSubscriber& subscriber,
                        SignalProcessingBase& signalProcessor, Logger& logger)
            : logger_(logger), signalProcessor_(signalProcessor), publisher_(publisher), subscriber_(subscriber)
        {
        }

        void Initialize()
        {
            timing_init();
            auto ret = signalProcessor_.Initialize();
            if (ret != ARM_MATH_SUCCESS)
            {
                this->logger_.error("FFT module could not be initialized: %d.", ret);
                return;
            }

            this->logger_.info("Processing module initialized.");
        }

        void Start()
        {
            timing_start();
            subscriber_.Subscribe<Core::EventTypes::AudioFrame>([&](Core::EventTypes::AudioFrame& event)
            {
                Notify(event);
            });
            this->logger_.info("Processing module started.");
        }

    protected:
        void Notify(Core::EventTypes::AudioFrame& event)
        {
            auto beat = this->signalProcessor_.Process(event.samples);
            if (!beat)
            {
                return;
            }
            auto beatEvent = Core::EventTypes::BeatEvent();
            auto timestamp = Timestamp();
            timestamp.nSec = timing_cycles_to_ns(timing_counter_get());
            beatEvent.ts = timestamp;
            if (const auto err = publisher_.Publish(beatEvent))
            {
                this->logger_.error("Error publishing beat event: %d", err);
            }
        }

        Logger& logger_;

        SignalProcessingBase& signalProcessor_;
        AppPublisher& publisher_;
        AppSubscriber& subscriber_;
    };
}
