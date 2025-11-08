#pragma once
#include <dsp/filtering_functions.h>

namespace SignalProcessing
{
    template <size_t SampleSize>
    class LpFilter
    {
        public:

        LpFilter(): irrState()
        {
        }

        void Initialize()
        {
            arm_biquad_cascade_df2T_init_f32(&S, numStageIir, this->coefs.data(), this->irrState.data());
        }

        void Process(std::array<float, SampleSize>& input, std::array<float, SampleSize>& output)
        {
            arm_biquad_cascade_df2T_f32(&S, input.data(), output.data(), SampleSize);
        }

        private:

        static constexpr size_t numStageIir = 1;
        static constexpr size_t numStageCoef = 5;

        arm_biquad_cascade_df2T_instance_f32 S{};
        static constexpr size_t numOrderIir = numStageIir * 2;
        std::array<float, numOrderIir> irrState;
        std::array<float, numStageIir * numStageCoef> coefs {0.19952362386506783, 0.39904724773013567, 0.19952362386506783, -0.35691870886351296, 0.15501320432378424}; //b10, b11, b12, a11, a12
    };
};
