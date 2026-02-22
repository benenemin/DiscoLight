//
// Created by bened on 07/12/2025.
//

#pragma once

#include <algorithm>

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class BeatRipples final : public IAnimation
{
public:
    explicit BeatRipples(uint8_t fade = 24, uint8_t speed_px = 2, uint8_t max_ripples = 3) noexcept
        : fade_(fade),
          speed_(speed_px),
          max_active_(static_cast<uint8_t>(std::min<uint8_t>(max_ripples, kMaxRipples)))
    {
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        LedUtil::fade(leds, fade_);

        for (uint8_t i = 0; i < kMaxRipples; ++i)
        {
            auto& ripple = ripples_[i];
            if (!ripple.active)
            {
                continue;
            }

            ripple.radius = static_cast<uint16_t>(ripple.radius + speed_);
            if (ripple.radius >= kMaxRadius)
            {
                ripple.active = false;
                continue;
            }

            const uint8_t value =
                static_cast<uint8_t>(std::max<int>(40, 255 - (ripple.radius * 510 / Constants::ChainLength)));
            const led_rgb color = LedUtil::hsv(ripple.hue, 255, value);

            const size_t p1 = LedUtil::wrap_index<Constants::ChainLength>(
                static_cast<int>(ripple.origin) + static_cast<int>(ripple.radius));
            const size_t p2 = LedUtil::wrap_index<Constants::ChainLength>(
                static_cast<int>(ripple.origin) - static_cast<int>(ripple.radius));

            LedUtil::add_sat(leds[p1], color);
            if (p2 != p1)
            {
                LedUtil::add_sat(leds[p2], color);
            }
        }
        hue_ = static_cast<uint8_t>(hue_ + 1);
    }

    void ProcessNextBeat() override
    {
        uint8_t idx = 0xFF;
        for (uint8_t i = 0; i < kMaxRipples; ++i)
        {
            if (!ripples_[i].active)
            {
                idx = i;
                break;
            }
        }

        if (idx == 0xFF)
        {
            idx = head_;
            head_ = static_cast<uint8_t>((head_ + 1) % max_active_);
        }

        ripples_[idx].active = true;
        ripples_[idx].radius = 0;
        ripples_[idx].hue = static_cast<uint8_t>(hue_ + 12);
        ripples_[idx].origin = rng_.uniform<uint8_t>(Constants::ChainLength);
    }

private:
    struct Ripple
    {
        bool active{false};
        uint16_t radius{0};
        uint8_t hue{0};
        uint8_t origin{0};
    };

    static constexpr uint16_t kMaxRadius =
        (Constants::ChainLength > 1) ? static_cast<uint16_t>(Constants::ChainLength / 2) : 0;
    static constexpr uint8_t kMaxRipples = 4;

    uint8_t fade_{24};
    uint8_t speed_{2};
    uint8_t max_active_{3};
    uint8_t head_{0};
    uint8_t hue_{0};
    Ripple ripples_[kMaxRipples]{};
    LedUtil::XorShift32 rng_{0xA5A5BEEFu};
};
} // namespace Animations
