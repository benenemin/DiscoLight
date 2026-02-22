//
// Created by bened on 11/11/2025.
//

#pragma once

#include <array>

#include "Constants.hpp"

namespace SignalProcessing
{
    class SignalProcessingBase
    {
    protected:
        virtual ~SignalProcessingBase() = default;

    public:
        virtual int Initialize() = 0;
        virtual bool Process(std::array<float, Constants::SamplingFrameSize>& samples) = 0;
    };
} // namespace SignalProcessing
