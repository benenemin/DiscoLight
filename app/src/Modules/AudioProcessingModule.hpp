//
// Created by bened on 19/10/2025.
//

#pragma once

#include <cstddef>

#include "arm_math.h"

#include "Core/EventTypes.hpp"
#include "SignalProcessing/SignalProcessingBase.hpp"
#include "Utils/Logger.hpp"
#include "Utils/MonotonicClock.hpp"

namespace Modules
{
    class AudioProcessingModule final
    {
    public:
        AudioProcessingModule(
            Core::EventTypes::AppPublisher& publisher,
            Core::EventTypes::AppSubscriber& subscriber,
            SignalProcessing::SignalProcessingBase& signal_processor,
            Utils::MonotonicClock& clock,
            Utils::Logger& logger)
            : publisher_(publisher),
              subscriber_(subscriber),
              signal_processor_(signal_processor),
              clock_(clock),
              logger_(logger)
        {
        }

        void Initialize()
        {
            logger_.info("Initializing audio processing module.");
            const auto ret = signal_processor_.Initialize();
            if (ret != ARM_MATH_SUCCESS)
            {
                logger_.error("FFT module could not be initialized: %d.", ret);
                initialized_ = false;
                return;
            }

            initialized_ = true;
            logger_.info("Processing module initialized.");
        }

        void Start()
        {
            if (!initialized_)
            {
                logger_.warning("Audio processing module is starting before successful initialization.");
            }

            const auto sub_ret = subscriber_.Subscribe<Core::EventTypes::AudioFrame>(
                [this](Core::EventTypes::AudioFrame& event)
            {
                OnAudioFrame(event);
            });
            if (sub_ret != 0)
            {
                logger_.error("Failed to subscribe to audio frames: %d", sub_ret);
                return;
            }
            logger_.info("Audio processing module subscribed to audio frames.");
            logger_.info("Processing module started.");
        }

    private:
        void OnAudioFrame(Core::EventTypes::AudioFrame& event)
        {
            ++processed_frames_;
            if (processed_frames_ % 100 == 0)
            {
                logger_.debug("Processed %d audio frames.", static_cast<int>(processed_frames_));
            }

            const auto beat = signal_processor_.Process(event.samples);
            if (!beat)
            {
                return;
            }

            ++detected_beats_;
            logger_.debug("Beat detected (count=%d).", static_cast<int>(detected_beats_));

            Core::EventTypes::BeatEvent beat_event{};
            beat_event.ts = clock_.Now();
            if (const auto err = publisher_.Publish(beat_event))
            {
                logger_.error("Error publishing beat event: %d", err);
            }
        }

        Core::EventTypes::AppPublisher& publisher_;
        Core::EventTypes::AppSubscriber& subscriber_;
        SignalProcessing::SignalProcessingBase& signal_processor_;
        Utils::MonotonicClock& clock_;
        Utils::Logger& logger_;
        bool initialized_{false};
        size_t processed_frames_{0};
        size_t detected_beats_{0};
    };
} // namespace Modules
