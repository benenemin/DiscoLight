//
// Created by bened on 18/10/2025.
//

#pragma once
#include "ADC/AdcReader.hpp"

namespace Threads
{
    template<size_t FrameSize>
    class AudioSampling
    {
    public:

        explicit AudioSampling(Adc::AdcReader<FrameSize> &reader, Core::MessageBus &message_bus, Logger &logger)
            : reader_(reader), message_bus_(message_bus), logger_(logger)
        {
        }

        void Initialize(const int samplingInterval_us) const
        {
            this->reader_.Initialize(samplingInterval_us);
        }

        void Start() const
        {
            this->reader_.Start([this](int sample_rate_hz, const array<float, FrameSize> &frame)
            {
                const Core::EventTypes::AudioFrame audioFrame =
                    {
                    .sample_rate_hz = sample_rate_hz,
                    .samples = frame
                    };

                message_bus_.Publish(audioFrame);
            });

            this->logger_.info("Audio sampling thread started.");
        }

        Adc::AdcReader<FrameSize> &reader_;
        Core::MessageBus& message_bus_;
        Logger& logger_;
    };
}
