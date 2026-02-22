//
// Created by bened on 07/12/2025.
//

#pragma once

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class RainbowWheel final : public IAnimation
{
public:
    explicit RainbowWheel(uint8_t sat = 255, uint8_t global = 180,
                          uint8_t spin = 1, uint8_t delta_per_led = 3)
        : sat_(sat), global_(global), spin_(spin), delta_per_led_(delta_per_led)
    {
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        base_hue_ += spin_;

        for (size_t i = 0; i < Constants::ChainLength; ++i)
        {
            const uint8_t hue = static_cast<uint8_t>(base_hue_ + static_cast<uint8_t>(i * delta_per_led_));
            led_rgb color = LedUtil::hsv(hue, sat_, global_);
            if (sparkle_frames_ > 0 && rng_.uniform<uint8_t>(16) == 0)
            {
                LedUtil::add_sat(color, {40, 40, 40});
            }
            leds[i] = color;
        }

        if (sparkle_frames_ > 0)
        {
            --sparkle_frames_;
        }
    }

    void ProcessNextBeat() override
    {
        sparkle_frames_ = 6;
        base_hue_ += 12;
    }

private:
    uint8_t sat_{255};
    uint8_t global_{180};
    uint8_t spin_{1};
    uint8_t delta_per_led_{3};
    uint8_t base_hue_{0};
    uint8_t sparkle_frames_{0};
    LedUtil::XorShift32 rng_{0xCAFEBABEu};
};
} // namespace Animations
