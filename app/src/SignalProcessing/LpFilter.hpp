#pragma once

#include <array>

#include <dsp/filtering_functions.h>

#include "Constants.hpp"

namespace SignalProcessing
{
class LpFilter final
{
public:
    LpFilter() = default;

    void Initialize()
    {
        arm_biquad_cascade_df2T_init_f32(&state_, kNumStageIir, coefficients_.data(), iir_state_.data());
    }

    void Process(const std::array<float, Constants::SamplingFrameSize>& input,
                 std::array<float, Constants::SamplingFrameSize>& output) const
    {
        arm_biquad_cascade_df2T_f32(&state_, input.data(), output.data(), Constants::SamplingFrameSize);
    }

private:
    static constexpr size_t kNumStageIir = 1;
    static constexpr size_t kNumStageCoef = 5;
    static constexpr size_t kNumOrderIir = kNumStageIir * 2;

    arm_biquad_cascade_df2T_instance_f32 state_{};
    std::array<float, kNumOrderIir> iir_state_{};
    std::array<float, kNumStageIir * kNumStageCoef> coefficients_{
        0.19952362386506783f,
        0.39904724773013567f,
        0.19952362386506783f,
        -0.35691870886351296f,
        0.15501320432378424f}; // b10, b11, b12, a11, a12
};
} // namespace SignalProcessing
