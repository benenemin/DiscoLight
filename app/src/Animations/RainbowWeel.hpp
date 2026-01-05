//
// Created by bened on 07/12/2025.
//
#pragma once
// =====================================================
// 3) RainbowWheel — spinning gradient, sparkles on beat
// =====================================================
class RainbowWheel final : public Animations::IAnimation
{
public:
    explicit RainbowWheel(uint8_t sat = 255, uint8_t global = 180,
                          uint8_t spin = 1, uint8_t delta_per_led = 3)
        : sat_(sat), global_(global), spin_(spin), dper_(delta_per_led)
    {
    }

    void ProcessNextFrame(Animations::IAnimation::LedChain& leds) override
    {
        auto& s = leds;

        base_hue_ += spin_;

        for (size_t i = 0; i < Constants::ChainLength; ++i)
        {
            const uint8_t h = uint8_t(base_hue_ + uint8_t(i * dper_));
            led_rgb c = hsv(h, sat_, global_);
            // optional sparkle decay
            if (sparkle_frames_ && rng_.uniform<uint8_t>(16) == 0)
            {
                // add a white twinkle
                add_sat(c, {40, 40, 40});
            }
            s[i] = c;
        }

        if (sparkle_frames_ > 0) --sparkle_frames_;
    }

    void ProcessNextBeat() override
    {
        // short sparkle window
        sparkle_frames_ = 6;
        base_hue_ += 12;
    }

private:
    uint8_t sat_ = 255, global_ = 180;
    uint8_t spin_ = 1, dper_ = 3;
    uint8_t base_hue_ = 0;
    uint8_t sparkle_frames_ = 0;
    XorShift32 rng_{0xCAFEBABEu};
};

