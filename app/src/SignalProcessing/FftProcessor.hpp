//
// Created by bened on 01/11/2025.
//

#pragma once

#include <array>

#include "arm_math.h"
#include "Constants.hpp"

namespace SignalProcessing
{
    class FftProcessor final
    {
    public:
        FftProcessor()
        {
        }

        int Initialize()
        {
            return arm_rfft_fast_init_f32(&this->rFFT_, Constants::SamplingFrameSize);
        }

        void Process(std::array<float, Constants::SamplingFrameSize>& input,
                     std::array<float, Constants::SamplingFrameSize / 2>& output)
        {
            arm_rfft_fast_f32(&this->rFFT_, input.data(), this->buffer.data(), 0);
            arm_cmplx_mag_f32(this->buffer.data(), output.data(), output.size());

            // smooth outliers in extrema and DC
            output[0] = 0;
            output[Constants::SamplingFrameSize / 2 - 1] = 0;
            output[Constants::SamplingFrameSize / 2 - 2] = 0;
        }

    private:
        arm_rfft_fast_instance_f32 rFFT_{};
        std::array<float, Constants::SamplingFrameSize> buffer{};
    };
} // namespace SignalProcessing
