//
// Created by bened on 09/12/2025.
//

#pragma once

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class CometChase final : public IAnimation
{
public:
    explicit CometChase(uint8_t initial_count = 3,
                        uint8_t tail_fade = 10,
                        uint8_t speed_px_per_frame = 1,
                        uint8_t base_hue = 0,
                        uint8_t hue_stride = 12,
                        uint8_t head_width = 1) noexcept
        : tail_(tail_fade),
          speed_(speed_px_per_frame),
          base_hue_(base_hue),
          hue_stride_(hue_stride),
          head_width_(head_width)
    {
        SetCount(initial_count);
        ReseedPositions();
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        ++frame_count_;
        if (frame_count_ % 10 != 0)
        {
            return;
        }
        frame_count_ = 0;

        const uint8_t fade_now = (boost_frames_ > 0 && tail_ > 8) ? static_cast<uint8_t>(tail_ - 8) : tail_;
        LedUtil::fade(leds, fade_now);

        const uint32_t period = (static_cast<uint32_t>(Constants::ChainLength) << 8);
        const uint32_t step = (static_cast<uint32_t>(speed_) << 8);

        for (uint8_t i = 0; i < active_; ++i)
        {
            auto& comet = comets_[i];
            if (dir_ > 0)
            {
                comet.subpos += step;
                if (comet.subpos >= period)
                {
                    comet.subpos -= period;
                }
            }
            else
            {
                comet.subpos = (comet.subpos >= step) ? (comet.subpos - step) : (comet.subpos + period - step);
            }

            const size_t idx = static_cast<size_t>(comet.subpos >> 8);
            const uint8_t value = (boost_frames_ > 0) ? 255 : 220;
            StampHead(leds, idx, comet.hue, value);
        }

        if (boost_frames_ > 0)
        {
            --boost_frames_;
        }
        base_hue_ = static_cast<uint8_t>(base_hue_ + 1);
        UpdateHues();
    }

    void ProcessNextBeat() override
    {
        dir_ = -dir_;
        boost_frames_ = 2;
        base_hue_ = static_cast<uint8_t>(base_hue_ + 8);
        UpdateHues();
    }

private:
    struct Comet
    {
        uint32_t subpos = 0;
        uint8_t hue = 0;
    };

    static void AddAt(LedChain& strip, size_t i, const led_rgb& color)
    {
        LedUtil::add_sat(strip[LedUtil::wrap_index<Constants::ChainLength>(static_cast<int>(i))], color);
    }

    void StampHead(LedChain& strip, size_t idx, uint8_t hue, uint8_t value) const
    {
        AddAt(strip, idx, LedUtil::hsv(hue, 255, value));
        if (head_width_ >= 1)
        {
            const uint8_t value1 = static_cast<uint8_t>(value > 96 ? value - 96 : value / 2);
            AddAt(strip, idx + 1, LedUtil::hsv(hue, 255, value1));
            AddAt(strip, (idx == 0 ? Constants::ChainLength - 1 : idx - 1), LedUtil::hsv(hue, 255, value1));
        }
        if (head_width_ >= 2)
        {
            const uint8_t value2 = static_cast<uint8_t>(kV1Scale);
            AddAt(strip, idx + 2, LedUtil::hsv(hue, 255, value2));
            AddAt(strip, (idx + Constants::ChainLength - 2) % Constants::ChainLength,
                  LedUtil::hsv(hue, 255, value2));
        }
    }

    void SetCount(uint8_t n) noexcept
    {
        active_ = (n == 0) ? 1 : (n > 5 ? 5 : n);
        UpdateHues();
    }

    void ReseedPositions() noexcept
    {
        const uint32_t period = (static_cast<uint32_t>(Constants::ChainLength) << 8);
        for (uint8_t i = 0; i < active_; ++i)
        {
            const uint32_t step = (period * i) / (active_ == 0 ? 1 : active_);
            comets_[i].subpos = step;
        }
    }

    void UpdateHues() noexcept
    {
        for (uint8_t i = 0; i < active_; ++i)
        {
            comets_[i].hue = static_cast<uint8_t>(base_hue_ + static_cast<uint8_t>(i * hue_stride_));
        }
    }

    uint8_t tail_{32};
    uint8_t speed_{1};
    uint8_t base_hue_{0};
    uint8_t hue_stride_{12};
    uint8_t head_width_{1};
    uint8_t active_{3};
    int8_t dir_{1};
    uint8_t boost_frames_{0};
    uint8_t frame_count_{0};

    static constexpr uint8_t kV1Scale = 64;
    Comet comets_[5]{};
};
} // namespace Animations
