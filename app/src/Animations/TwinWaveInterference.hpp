//
// Created by bened on 09/12/2025.
//

#pragma once

#include <algorithm>
#include <cmath>

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class TwinWaveInterference final : public IAnimation
{
public:
    explicit TwinWaveInterference(uint8_t hue_a = 0, uint8_t hue_b = 128,
                                  uint8_t speed = 1, uint8_t base_v = 40) noexcept
        : hue_a_(hue_a), hue_b_(hue_b), speed_(speed), base_v_(base_v)
    {
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        pha_ = static_cast<uint8_t>(pha_ + speed_);
        phb_ = static_cast<uint8_t>(phb_ - speed_);

        for (size_t i = 0; i < Constants::ChainLength; ++i)
        {
            const uint8_t ang = static_cast<uint8_t>((i * 256) / Constants::ChainLength);
            const uint8_t wa =
                255 - static_cast<uint8_t>(std::abs(static_cast<int>(ang + pha_) - 128) * 2);
            const uint8_t wb =
                255 - static_cast<uint8_t>(std::abs(static_cast<int>(ang + phb_) - 128) * 2);

            uint16_t value = base_v_ + (contrast_ * wa) / 255 + (contrast_ * wb) / 255;
            if (value > 255)
            {
                value = 255;
            }

            const led_rgb color_a = LedUtil::hsv(hue_a_, 255, static_cast<uint8_t>(value));
            const led_rgb color_b = LedUtil::hsv(hue_b_, 255, static_cast<uint8_t>(value / 2));
            led_rgb output = color_a;
            LedUtil::add_sat(output, color_b);
            leds[i] = output;
        }
    }

    void ProcessNextBeat() override
    {
        contrast_ = static_cast<uint8_t>(std::min<int>(255, contrast_ + 64));
        hue_a_ = static_cast<uint8_t>(hue_a_ + 8);
        hue_b_ = static_cast<uint8_t>(hue_b_ + 8);
    }

private:
    uint8_t hue_a_{0};
    uint8_t hue_b_{128};
    uint8_t speed_{1};
    uint8_t base_v_{40};
    uint8_t pha_{0};
    uint8_t phb_{0};
    uint8_t contrast_{80};
};
} // namespace Animations
