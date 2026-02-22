//
// Created by bened on 11/11/2025.
//

#pragma once

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class BeatPulse final : public IAnimation
{
public:
    explicit BeatPulse(uint8_t hue0 = 0, uint8_t sat = 255, uint8_t decay = 5, uint8_t hue_step = 1)
        : hue_(hue0), sat_(sat), decay_(decay), hue_step_(hue_step)
    {
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        if (level_ > decay_)
        {
            level_ -= decay_;
        }
        else
        {
            level_ = 0;
        }

        const led_rgb color = LedUtil::hsv(hue_, sat_, level_);
        for (auto& px : leds)
        {
            px = color;
        }
        hue_ += hue_step_;
    }

    void ProcessNextBeat() override
    {
        level_ = 255;
        hue_ += 8;
    }

private:
    uint8_t hue_{0};
    uint8_t sat_{255};
    uint8_t level_{0};
    uint8_t decay_{8};
    uint8_t hue_step_{1};
};
} // namespace Animations
