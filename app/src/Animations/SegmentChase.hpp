//
// Created by bened on 09/12/2025.
//

#pragma once

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class SegmentChase final : public IAnimation
{
public:
    explicit SegmentChase(uint8_t segments = 12, uint8_t tail_fade = 50) noexcept
        : segments_(segments == 0 ? 1 : segments), tail_(tail_fade)
    {
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        LedUtil::fade(leds, tail_);

        const size_t segment_length = Constants::ChainLength / segments_;
        const size_t start = (static_cast<size_t>(active_) * segment_length) % Constants::ChainLength;

        for (size_t i = 0; i < segment_length; ++i)
        {
            const size_t idx = LedUtil::wrap_index<Constants::ChainLength>(static_cast<int>(start + i));
            const uint8_t t = static_cast<uint8_t>((i * 255) / (segment_length == 0 ? 1 : segment_length));
            const led_rgb color = LedUtil::hsv(hue_, 255, static_cast<uint8_t>(255 - t / 2));
            LedUtil::add_sat(leds[idx], color);
        }
        hue_ = static_cast<uint8_t>(hue_ + 1);
    }

    void ProcessNextBeat() override
    {
        active_ = static_cast<uint8_t>((active_ + 1) % segments_);
        hue_ = static_cast<uint8_t>(hue_ + 16);
    }

private:
    uint8_t segments_{12};
    uint8_t tail_{50};
    uint8_t active_{0};
    uint8_t hue_{0};
};
} // namespace Animations
