//
// Created by bened on 11/11/2025.
//

#pragma once

namespace SignalProcessing
{
    class SignalProcessingBase
    {
    protected:
        virtual ~SignalProcessingBase() = default;

    public:
        virtual int Initialize() = 0;
        virtual bool Process(array<float, Constants::SamplingFrameSize>& samples) = 0;
    };
}
