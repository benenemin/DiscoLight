//
// Created by bened on 18/10/2025.
//

#pragma once

#include <cstddef>

#include "ADC/AdcReader.hpp"
#include "Core/EventTypes.hpp"

namespace Modules
{
    class AudioSamplingModule final
    {
    public:
        explicit AudioSamplingModule(
            Adc::AdcReader& reader,
            Core::EventTypes::AppPublisher& publisher,
            Utils::Logger& logger)
            : reader_(reader), publisher_(publisher), logger_(logger)
        {
        }

        void Initialize()
        {
            logger_.info("Initializing audio sampling module.");
            const auto ret = reader_.Initialize(Constants::SamplingInterval_us);
            if (ret != 0)
            {
                logger_.error("Audio sampling module initialization failed: %d", ret);
                return;
            }

            logger_.info("Audio sampling module initialized.");
        }

        void Start()
        {
            logger_.info("Starting audio sampling module.");
            reader_.Start([this](const int sample_rate_hz,
                                 const std::array<float, Constants::SamplingFrameSize>& frame)
            {
                audio_frame_.sample_rate_hz = sample_rate_hz;
                audio_frame_.samples = frame;

                if (const auto err = publisher_.Publish(audio_frame_))
                {
                    logger_.error("failed to publish audio frame: %d", err);
                    return;
                }

                ++published_frames_;
                if (published_frames_ % 100 == 0)
                {
                    logger_.debug("Published %d audio frames.", static_cast<int>(published_frames_));
                }
            });

            logger_.info("Audio sampling module started.");
        }

    private:
        Adc::AdcReader& reader_;
        Core::EventTypes::AppPublisher& publisher_;
        Core::EventTypes::AudioFrame audio_frame_{};
        Utils::Logger& logger_;
        size_t published_frames_{0};
    };
} // namespace Modules
