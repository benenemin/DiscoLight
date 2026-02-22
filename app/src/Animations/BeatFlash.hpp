//
// Created by bened on 09/12/2025.
//

#pragma once

#include <algorithm>

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class BeatFlash final : public IAnimation
{
public:
    explicit BeatFlash(led_rgb color = {255, 255, 255},
                       uint8_t hold_frames = 2,
                       uint8_t decay_per_frame = 24) noexcept
        : color_(color), hold_(hold_frames), decay_(decay_per_frame)
    {
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        uint8_t value = 0;
        if (hold_cnt_ > 0)
        {
            value = 255;
        }
        else if (level_ > 0)
        {
            value = level_;
        }

        if (value > 0)
        {
            const led_rgb color = ScaleColor(color_, value);
            std::fill(leds.begin(), leds.end(), color);
        }
        else
        {
            std::fill(leds.begin(), leds.end(), led_rgb{0, 0, 0});
        }

        if (hold_cnt_ > 0)
        {
            --hold_cnt_;
        }
        else if (level_ > 0)
        {
            level_ = (level_ > decay_) ? static_cast<uint8_t>(level_ - decay_) : 0;
        }
    }

    void ProcessNextBeat() override
    {
        hold_cnt_ = hold_;
        level_ = 255;
    }

private:
    static led_rgb ScaleColor(const led_rgb& color, uint8_t value) noexcept
    {
        return {
            LedUtil::scale8(color.r, value),
            LedUtil::scale8(color.g, value),
            LedUtil::scale8(color.b, value),
        };
    }

    led_rgb color_{255, 160, 20};
    uint8_t hold_{2};
    uint8_t decay_{24};
    uint8_t hold_cnt_{0};
    uint8_t level_{0};
};
} // namespace Animations
