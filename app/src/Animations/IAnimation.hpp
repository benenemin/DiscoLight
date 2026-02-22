//
// Created by bened on 11/11/2025.
//

#pragma once

#include <array>

#include <zephyr/drivers/led_strip.h>

#include "Constants.hpp"

namespace Animations
{
class IAnimation
{
    public:
    virtual ~IAnimation() = default;

    using LedChain = std::array<led_rgb, Constants::ChainLength>;
    virtual void ProcessNextFrame(LedChain& leds) = 0;
    virtual void ProcessNextBeat() = 0;
};
} // namespace Animations
