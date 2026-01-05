//
// Created by bened on 18/10/2025.
//

#pragma once

#include "Modules/ModuleBase.hpp"
#include "ADC/AdcReader.hpp"
#include "Core/EventTypes.hpp"

namespace Modules
{
    class AudioSamplingModule final : ModuleBase
    {
    public:
        explicit AudioSamplingModule(Adc::AdcReader& reader, AppPublisher& publisher, AppSubscriber &subscriber,
                               Logger& logger)
            : ModuleBase(publisher, subscriber), reader_(reader), logger_(logger)
        {
        }

        void Initialize()
        {
            this->reader_.Initialize(Constants::SamplingInterval_us);
            ModuleBase::Initialize();
        }

        void Start() const
        {
            this->reader_.Start([this](const int sample_rate_hz, const array<float, Constants::SamplingFrameSize>& frame)
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

        Adc::AdcReader& reader_;
        inline static Core::EventTypes::AudioFrame audioFrame{};
        Logger& logger_;
    };
}
