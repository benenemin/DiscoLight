//
// Created by bened on 11/11/2025.
//

#pragma once

namespace SignalProcessing
{
class LevelPeakDetector {
public:
    struct Params {
        // Time constants (ms)
        float fast_tau_ms = 12.0f;     // responds to quick changes
        float slow_tau_ms = 160.0f;    // tracks overall loudness

        // Decision (dimensionless, scale-invariant)
        float rise_threshold = 0.45f;  // trigger when norm_lift >= this
        float fall_threshold = 0.2f;  // reset when norm_lift <= this (hysteresis)

        // Refractory (no new beats) to avoid doubles on one hit
        uint32_t min_interval_ms = 120;

        // Safety
        float denom_eps = 1e-6f;       // to avoid 0/0 at silence
        float level_clip = 10.0f;      // optional: clamp extreme levels (linear units)
    };

    // fs_hz: input sample rate
    // hop_samples: how many samples you process per call to processBlock()
    explicit LevelPeakDetector(uint32_t fs_hz,
                             uint32_t hop_samples) noexcept
        : fs_hz_(fs_hz), hop_(hop_samples)
    {
        const float dt = (fs_hz_ > 0 && hop_ > 0)
                       ? (static_cast<float>(hop_) / static_cast<float>(fs_hz_))
                       : 0.001f; // fallback 1 ms

        // EMA coefficients: y += a*(x - y), where a = 1 - exp(-dt/tau)
        a_fast_ = one_minus_exp(-dt / (p_.fast_tau_ms * 1e-3f));
        a_slow_ = one_minus_exp(-dt / (p_.slow_tau_ms * 1e-3f));

        // Refractory in frames
        min_frames_ = static_cast<uint32_t>(
            std::max(1.0f, (p_.min_interval_ms * 1e-3f) / dt)
        );
    }

    // Reset internal state
    void reset(float seed_level = 0.0f) noexcept {
        e_fast_ = e_slow_ = std::max(0.0f, seed_level);
        norm_lift_ = 0.0f;
        armed_ = true;
        since_last_ = min_frames_;
    }

    // --- Option A: feed a block of samples (mono) ---
    // Returns true if a beat was detected on this block.
    bool processBlock(const float* x, std::size_t n) noexcept {
        // Mean absolute value (very cheap loudness proxy)
        float acc = 0.0f;
        for (std::size_t i = 0; i < n; ++i) {
            const float s = std::fabs(x[i]);
            acc += s;
        }
        const float lvl = (n > 0) ? (acc / static_cast<float>(n)) : 0.0f;
        return processLevel(lvl);
    }

    // --- Option B: feed a precomputed level (e.g., your own RMS/peak/AGC) ---
    bool processLevel(float level) noexcept {
        // Optional clamp to mitigate pops/clip bursts in integer pipelines
        if (level > p_.level_clip) level = p_.level_clip;

        // Update EMAs
        e_fast_ += a_fast_ * (level - e_fast_);
        e_slow_ += a_slow_ * (level - e_slow_);

        // Scale-invariant onset measure
        norm_lift_ = (e_fast_ - e_slow_) / (e_slow_ + p_.denom_eps);
        if (norm_lift_ < 0.0f) norm_lift_ = 0.0f; // ignore “dips”

        // Refractory frames counter
        if (since_last_ < 0xFFFFFFFFu) ++since_last_;

        bool beat = false;

        if (armed_) {
            if (norm_lift_ >= p_.rise_threshold && since_last_ >= min_frames_) {
                beat = true;
                armed_ = false;
                since_last_ = 0;
            }
        } else {
            // Wait until energy returns near baseline before arming again
            if (norm_lift_ <= p_.fall_threshold) {
                armed_ = true;
            }
        }

        return beat;
    }

    // Telemetry/getters (optional)
    float fast()       const noexcept { return e_fast_; }
    float slow()       const noexcept { return e_slow_; }
    float norm_lift()  const noexcept { return norm_lift_; }
    Params params()    const noexcept { return p_; }

private:
    static float one_minus_exp(float x) noexcept {
        // For small |x|, 1 - e^x ≈ -x (better precision).
        // But here x <= 0, so 1-exp(x) ∈ [0,1). Use a tiny guard.
        if (x > -1e-4f) return -x;        // linearized
        const float e = std::exp(x);
        float a = 1.0f - e;
        if (a < 0.0f) a = 0.0f;
        if (a > 1.0f) a = 1.0f;
        return a;
    }

    // Config
    uint32_t fs_hz_{48000};
    uint32_t hop_{480};
    Params   p_{};
    float    a_fast_{0.0f}, a_slow_{0.0f};
    uint32_t min_frames_{1};

    // State
    float e_fast_{0.0f};
    float e_slow_{0.0f};
    float norm_lift_{0.0f};
    bool  armed_{true};
    uint32_t since_last_{0}; // frames since last beat
};
}
