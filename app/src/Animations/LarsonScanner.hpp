//
// Created by bened on 07/12/2025.
//

#pragma once

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class LarsonScanner final : public IAnimation
{
public:
    explicit LarsonScanner(uint8_t hue = 0, uint8_t tail_fade = 32, uint8_t speed = 1)
        : hue_(hue), tail_(tail_fade), speed_(speed)
    {
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        LedUtil::fade(leds, tail_);

        subpos_ += (dir_ > 0) ? (speed_ << 8) : static_cast<uint16_t>(0 - (speed_ << 8));
        int pos = static_cast<int>(subpos_ >> 8);

        if (pos <= 0)
        {
            pos = static_cast<int>(Constants::ChainLength) - pos;
            dir_ = 1;
            subpos_ = static_cast<uint16_t>(Constants::ChainLength - pos);
        }
        else if (pos >= static_cast<int>(Constants::ChainLength - 1))
        {
            pos %= static_cast<int>(Constants::ChainLength);
            dir_ = 1;
            subpos_ = static_cast<uint16_t>((pos << 8) % Constants::ChainLength);
        }

        leds[static_cast<size_t>(pos)] = LedUtil::hsv(hue_, 255, 255);
        hue_ += 1;
    }

    void ProcessNextBeat() override
    {
        dir_ = -dir_;
        if (tail_ > 8)
        {
            tail_ -= 8;
        }
    }

private:
    uint8_t hue_{0};
    uint8_t tail_{32};
    uint8_t speed_{1};
    int8_t dir_{1};
    uint16_t subpos_{0};
};
} // namespace Animations
