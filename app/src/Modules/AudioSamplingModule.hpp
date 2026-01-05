//
// Created by bened on 18/10/2025.
//

#pragma once
#include "ADC/AdcReader.hpp"
#include "Core/EventTypes.hpp"

namespace Modules
{
    template <size_t FrameSize>
    class AudioSampling
    {
    public:
        explicit AudioSampling(Adc::AdcReader<FrameSize>& reader, Core::EventTypes::AppPublisher& publisher,
                               Logger& logger)
            : reader_(reader), publisher_(publisher), logger_(logger)
        {
        }

        void Initialize(const int samplingInterval_us) const
        {
            this->reader_.Initialize(samplingInterval_us);
        }

        void Start()
        {
            this->reader_.Start([this](const int sample_rate_hz, array<float, FrameSize>& frame)
            {
                audioFrame.sample_rate_hz = sample_rate_hz;
                audioFrame.samples = frame;

                if (const auto err = publisher_.Publish(audioFrame))
                {
                    this->logger_.error("failed to publish audio frame: %d", err);
                }
            });

            this->logger_.info("Audio sampling module started.");
        }

        Adc::AdcReader<FrameSize>& reader_;
        Core::EventTypes::AppPublisher& publisher_;
        inline static Core::EventTypes::AudioFrame audioFrame{};
        Logger& logger_;
    };
}
