//
// Created by bened on 01/11/historySize25.
//

#pragma once

#include "SignalProcessingBase.hpp"
#include "FftProcessor.hpp"
#include "LpFilter.hpp"

namespace SignalProcessing
{
    template<size_t FrameSize, size_t BeatBands = 2>
class BeatDetector final : public SignalProcessingBase<FrameSize>
{
        public:

        using Spectrum     = array<float, FrameSize/2>;
        using BandEnergies = array<float, BeatBands>;
        using Flags        = array<bool,  BeatBands>;
        using Band         = pair<size_t,size_t>;

        BeatDetector(FftProcessor<FrameSize>& fftProcessor, LpFilter<FrameSize>& filter, Logger &logger, int sampleRate, Band band1 = {40, 130}, Band band2 = {300, 750})
        : logger_(logger), sample_rate_(sampleRate), band1_(std::move(band1)), band2_(std::move(band2)),
          fftProcessor_(fftProcessor), filter_(filter)
    {

        }

        int Initialize() override
        {
            cfg_.band_freqs  = { band1_, band2_ };
            cfg_.alpha    = { -15.0f, -15.0f };    // variance sensitivity
            cfg_.beta     = {   1.55f,  1.55f };   // base multiplier
            cfg_.eps_frac = {   0.05f,  0.005f };  // suppress DC/bleed: 5% (bass), 0.5% (low-mid)
            cfg_.denom_eps = 1e-12f;               // safe tiny epsilon

            const uint32_t bandSize = (FrameSize/2 != 0u)
                                  ? (sample_rate_ / FrameSize/2)
                                  : 0u;
            for (size_t b = 0; b < BeatBands; ++b) {
                size_t lo = (bandSize ? (cfg_.band_freqs[b].first  / bandSize) : 0u);
                size_t hi = (bandSize ? (cfg_.band_freqs[b].second / bandSize) : 0u);
                lo = min<size_t>(lo, FrameSize/2);
                hi = min<size_t>(hi, FrameSize/2);
                if (hi <= lo) hi = min<size_t>(lo + 1, FrameSize/2);
                bands_bins_[b] = {lo, hi};    // [lo, hi)
            }

            this->filter_.Initialize();
            return this->fftProcessor_.Initialize();
        }

        bool Process(array<float, FrameSize> &samples) override
        {
            /* Filter */
            this->filter_.Process(samples, this->filterOut_);
            /* Apply FFT */
            this->fftProcessor_.Process(this->filterOut_, this->fft_power);

            // 1) Compute band energies (average of bins in range)
            for (size_t b = 0; b < BeatBands; ++b)
            {
                const auto [lo, hi] = bands_bins_[b];
                float acc = 0.0f;
                const size_t width = hi - lo;
                for (size_t i = lo; i < hi; ++i) acc += isnan(this->fft_power[i]) ? 0.0f : this->fft_power[i];
                band_energy_[b] = (width ? (acc / static_cast<float>(width)) : 0.0f);
            }

            if (count_ == 0)
            {
                push_current_into_history_();
                return false;
            }

            // // 2) Compute mean/variance from PREVIOUS history (exclude current, like your code)
            bool any = false;

            for (size_t b = 0; b < BeatBands; ++b) {
                mean_[b] = sum_[b] / historySize;
                const float e2 = sumsq_[b] / 40.0f;
                float v = e2 - mean_[b]*mean_[b];     // population variance
                var_[b] = max(v, 0.0f);

                const float c = -0.0025714f*var_[b] + 1.5142857f;
                const bool beat = band_energy_[b] > c * mean_[b];
                if (b == 0)
                {
                    any = beat;
                    logger_.info("value: %f > %f", band_energy_[0], var_[b]);
                }
            }
            push_current_into_history_();
            return any;
        }
    private:

        struct Config {
            // Frequency ranges for each band (Hz): [low, high)
            array<pair<uint32_t,uint32_t>, BeatBands> band_freqs;
            // (E[b] - epsilon[b]) > (alpha[b]*Var[b] + beta[b]) * Mean[b]
            array<float, BeatBands> eps_frac  {};  // small DC/bleed offsets
            array<float, BeatBands> alpha    {};  // usually negative (e.g., -15)
            array<float, BeatBands> beta     {};  // e.g., +1.55

            float denom_eps = 1e-12f;
        };

        void push_current_into_history_() noexcept {
            if (count_ == historySize) {
                // Remove oldest from sums
                const auto& old = history_[head_];
                for (std::size_t b = 0; b < BeatBands; ++b) {
                    sum_[b]   -= old[b];
                    sumsq_[b] -= old[b]*old[b];
                }
            }
            else {
                ++count_;
            }
            // Insert new
            history_[head_] = band_energy_;
            for (std::size_t b = 0; b < BeatBands; ++b) {
                const float e = band_energy_[b];
                sum_[b]   += e;
                sumsq_[b] += e*e;
            }
            head_ = (head_ + 1) % historySize;
        }
        BandEnergies band_energy_{};
        BandEnergies sum_{}, sumsq_{};
        BandEnergies mean_{}, var_{};

        Config cfg_{};
        array<Band, BeatBands>bands_bins_{};
        const size_t historySize = 10;
        std::array<BandEnergies, 10> history_{}; // ring buffer
        array<float, FrameSize> filterOut_{};
        array<float, FrameSize/2> fft_power{};
        std::size_t head_  = 0;
        size_t count_ = 0;
        Logger& logger_;
        int sample_rate_;
        Band band1_;
        Band band2_;
        FftProcessor<FrameSize>& fftProcessor_;
        LpFilter<FrameSize>& filter_;
};
}
