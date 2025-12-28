//
// Created by bened on 30/11/2025.
//

#pragma once

#include "FftProcessor.hpp"
#include "SignalProcessingBase.hpp"
#include "LpFilter.hpp"

namespace SignalProcessing
{
    template<size_t FrameSize>
    class BeatDetectorV2 final : public SignalProcessingBase<FrameSize>
    {
    public:
        explicit BeatDetectorV2(FftProcessor<FrameSize>& fftProcessor, LpFilter<FrameSize>& filter)
            : filter_(filter), fftProcessor_(fftProcessor)
        {
            const uint32_t bandSize = (FrameSize / 2 != 0u)
                                          ? (10000 / FrameSize / 2)
                                          : 0u;
            lo = (bandSize ? (30 / bandSize) : 0u);
            hi = (bandSize ? (140 / bandSize) : 0u);
        }

        int Initialize() override
        {
            this->filter_.Initialize();
            return this->fftProcessor_.Initialize();
        }

        bool Process(array<float, FrameSize>& samples) override
        {
            /* Filter */
            this->filter_.Process(samples, this->filterOut_);
            this->fftProcessor_.Process(this->filterOut_, this->fft_power);

            // 1) Compute band energies (average of bins in range)
            float acc = 0.0f;
            const size_t width = hi - lo;
            for (size_t i = lo; i < hi; ++i) acc += isnan(this->fft_power[i]) ? 0.0f : this->fft_power[i];
            auto band_energy = (width ? (acc / static_cast<float>(width)) : 0.0f);



            // Get audio levels
            float rms = 0.0f;
            arm_rms_f32(samples.data(), samples.size(), &rms);
            rms = rms / 32768.0f;

            // Calculate peak
            int32_t maxSample = 0;
            for (size_t i = 0; i < FrameSize; i++) {
                const int32_t absSample = fabsf(samples[i]);
                if (absSample > maxSample) {
                    maxSample = absSample;
                }
            }

            const float peak = static_cast<float>(maxSample) / 32768.0f;
            peakLevel = peakLevel * 0.9f + peak * 0.1f;  // Smooth peak

            // Update auto gain
            updateAutoGain(band_energy);

            // Beat detection
            return detectBeat(band_energy);
        }

    private:
        // Beat detection algorithm
        bool detectBeat(float energy) {
            beatHistory[beatHistoryIndex] = energy;
            beatHistoryIndex = (beatHistoryIndex + 1) % hist_size;

            // Calculate average
            beatAverage = 0;
            for (int i = 0; i < hist_size; i++) {
                beatAverage += beatHistory[i];
            }
            beatAverage /= hist_size;

            // Calculate variance
            beatVariance = 0;
            for (int i = 0; i < hist_size; i++) {
                const float diff = beatHistory[i] - beatAverage;
                beatVariance += diff * diff;
            }
            beatVariance /= hist_size;

            // Detect beat
            const auto threshold = beatAverage + (beatSensitivity * sqrt(beatVariance));

            isBeat = energy > threshold;
            const auto beat = isBeat && !wasBeat;
            wasBeat = isBeat;
            return beat;
        }

        // Update auto gain
        void updateAutoGain(float level) {
            static float targetLevel = 0.7f;
            static float avgLevel = 0.0f;

            avgLevel = avgLevel * 0.95f + level * 0.05f;

            if (avgLevel > 0.01f) {
                float gainAdjust = targetLevel / avgLevel;
                gainAdjust = max(0.5f, min(gainAdjust, 2.0f));
                autoGainValue = autoGainValue * 0.9f + gainAdjust * 0.1f;
            }
        }

        // Audio processing variables
        static constexpr int hist_size = 40;
        float beatHistory[hist_size] = {0};  // Reduced from 43
        int beatHistoryIndex = 0;
        float beatAverage = 0;
        float beatVariance = 0;
        bool isBeat = false;
        bool wasBeat = false;
        float autoGainValue = 1.0f;
        float peakLevel = 0;
        float beatSensitivity = 0.9f;
        array<float, FrameSize> filterOut_{};
        array<float, FrameSize/2> fft_power{};
        LpFilter<FrameSize>& filter_;
        FftProcessor<FrameSize>& fftProcessor_;

        size_t lo = 0;
        size_t hi = 0;
    };
}