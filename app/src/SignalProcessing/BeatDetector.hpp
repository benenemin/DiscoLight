//
// Created by bened on 01/11/2025.
//

#pragma once

namespace SignalProcessing
{
    template<size_t FrameSize, size_t BeatBands>
class BeatDetector
{
        public:

        using Spectrum     = array<float, FrameSize/2>;
        using BandEnergies = array<float, BeatBands>;
        using Flags        = array<bool,  BeatBands>;
        using Band         = pair<size_t,size_t>;

        BeatDetector(Utils::Logger &logger)
        : logger_(logger)
        {

        }

        void Initialize(int sampleRate, Band band1, Band band2)
        {
            cfg_.band_freqs  = { band1, band2 };
            cfg_.alpha    = { -15.0f, -15.0f };    // variance sensitivity
            cfg_.beta     = {   1.55f,  1.55f };   // base multiplier
            cfg_.eps_frac = {   0.05f,  0.005f };  // suppress DC/bleed: 5% (bass), 0.5% (low-mid)
            cfg_.denom_eps = 1e-12f;               // safe tiny epsilon

            const uint32_t bandSize = (FrameSize/2 != 0u)
                                  ? (sampleRate / FrameSize/2)
                                  : 0u;
            for (size_t b = 0; b < BeatBands; ++b) {
                size_t lo = (bandSize ? (cfg_.band_freqs[b].first  / bandSize) : 0u);
                size_t hi = (bandSize ? (cfg_.band_freqs[b].second / bandSize) : 0u);
                lo = min<size_t>(lo, FrameSize/2);
                hi = min<size_t>(hi, FrameSize/2);
                if (hi <= lo) hi = min<size_t>(lo + 1, FrameSize/2);
                bands_bins_[b] = {lo, hi};    // [lo, hi)
            }
        }

        bool Process(const Spectrum& spectrum, Flags& beats_out)
        {
            // 1) Compute band energies (average of bins in range)
            for (size_t b = 0; b < BeatBands; ++b)
            {
                const auto [lo, hi] = bands_bins_[b];
                float acc = 0.0f;
                const size_t width = hi - lo;
                for (size_t i = lo; i < hi; ++i) acc += isnan(spectrum[i]) ? 0.0f : spectrum[i];
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
                mean_[b] = sum_[b] / 20.0f;
                const float e2 = sumsq_[b] / 40.0f;
                float v = e2 - mean_[b]*mean_[b];     // population variance
                var_[b] = (v > 0.0f) ? v : 0.0f;

                const float c = -0.0025714f*var_[b] + 1.5142857f;
                const bool beat = band_energy_[b] > c * mean_[b];
                beats_out[b] = beat;
                any |= beat;
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
            if (count_ == 20) {
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
            head_ = (head_ + 1) % 20;
        }

        BandEnergies band_energy_{};
        BandEnergies sum_{}, sumsq_{};
        BandEnergies mean_{}, var_{};

        Config cfg_{};
        array<Band, BeatBands>bands_bins_{};
        std::array<BandEnergies, 20> history_{}; // ring buffer
        std::size_t head_  = 0;
        size_t count_ = 0;
        Utils::Logger& logger_;
};
}
