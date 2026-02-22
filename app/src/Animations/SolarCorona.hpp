//
// Created by bened on 09/12/2025.
//

#pragma once

#include <algorithm>
#include <array>

#include "Animations/IAnimation.hpp"
#include "Utils/LedUtils.hpp"

namespace Animations
{
class SolarCorona final : public IAnimation
{
public:
    explicit SolarCorona(uint8_t base_hue = 32,
                         uint8_t base_sat = 240,
                         uint8_t base_floor_v = 70,
                         uint8_t pulse_decay = 10,
                         uint8_t flicker_amount = 4,
                         uint8_t flare_decay = 14,
                         uint8_t flare_max_radius = 4,
                         uint8_t flare_spawn_prob = 6) noexcept
        : hue_(base_hue),
          sat_(base_sat),
          floor_v_(base_floor_v),
          pulse_decay_(pulse_decay),
          flicker_(flicker_amount),
          flare_decay_(flare_decay),
          flare_rmax_(flare_max_radius),
          spawn_prob_(flare_spawn_prob)
    {
        for (size_t i = 0; i < Constants::ChainLength; ++i)
        {
            grain_[i] = static_cast<uint8_t>(160u + (rng_.next8() % 96u));
        }
        SpawnFlare(false);
        SpawnFlare(false);
    }

    void ProcessNextFrame(LedChain& leds) override
    {
        const uint8_t core = static_cast<uint8_t>(floor_v_ + ((uint16_t(pulse_) * 170u) >> 8));
        const uint8_t phase = phase_;

        for (size_t i = 0; i < Constants::ChainLength; ++i)
        {
            const uint8_t grain = grain_[(i + phase) % Constants::ChainLength];
            uint8_t value = LedUtil::scale8(core, grain);
            if (flicker_ != 0)
            {
                value = LedUtil::sadd8(value, rng_.uniform<uint8_t>(flicker_ + 1));
            }
            leds[i] = LedUtil::hsv(hue_, sat_, value);
        }

        for (auto& flare : flares_)
        {
            if (!flare.active)
            {
                continue;
            }

            for (uint8_t dist = 0; dist <= flare.radius; ++dist)
            {
                const uint8_t fall = Falloff(flare.radius, dist);
                const uint8_t value = LedUtil::scale8(flare.life, fall);
                const uint8_t hot_boost = (dist <= 1) ? (value / 3) : 0;

                led_rgb color = LedUtil::hsv(flare.hue, sat_, value);
                if (hot_boost != 0)
                {
                    LedUtil::add_sat(color, led_rgb{hot_boost, hot_boost, hot_boost});
                }

                const size_t p1 = LedUtil::wrap_index<Constants::ChainLength>(int(flare.pos) + dist);
                const size_t p2 = LedUtil::wrap_index<Constants::ChainLength>(int(flare.pos) - dist);
                LedUtil::add_sat(leds[p1], color);
                if (p1 != p2)
                {
                    LedUtil::add_sat(leds[p2], color);
                }
            }

            flare.life =
                (flare.life > flare_decay_) ? static_cast<uint8_t>(flare.life - flare_decay_) : 0;
            if (flare.life == 0)
            {
                flare.active = 0;
            }
        }

        if (rng_.next8() < spawn_prob_)
        {
            SpawnFlare(false);
        }

        pulse_ = (pulse_ > pulse_decay_) ? static_cast<uint8_t>(pulse_ - pulse_decay_) : 0;
        hue_ = static_cast<uint8_t>(hue_ + 1);
        phase_ = static_cast<uint8_t>(phase_ + 1);
    }

    void ProcessNextBeat() override
    {
        pulse_ = 255;
        hue_ = static_cast<uint8_t>(hue_ + 6);
        const uint8_t count = static_cast<uint8_t>(2 + (rng_.next8() % 2));
        for (uint8_t i = 0; i < count; ++i)
        {
            SpawnFlare(true);
        }
    }

private:
    struct Flare
    {
        uint8_t active{0};
        uint8_t pos{0};
        uint8_t radius{0};
        uint8_t life{0};
        uint8_t hue{0};
    };

    static constexpr uint8_t kMaxFlares = 8;

    static uint8_t Falloff(uint8_t radius, uint8_t dist) noexcept
    {
        if (dist >= radius)
        {
            return radius != 0 ? 0u : 255u;
        }

        const uint16_t t = static_cast<uint16_t>(255u * dist / (radius == 0 ? 1u : radius));
        const uint16_t t2 = static_cast<uint16_t>((t * t + 127u) / 255u);
        const int value = 255 - static_cast<int>(t2);
        return static_cast<uint8_t>(value < 0 ? 0 : value);
    }

    void SpawnFlare(bool hot) noexcept
    {
        uint8_t idx = 0xFF;
        uint8_t weakest = 255;
        for (uint8_t i = 0; i < kMaxFlares; ++i)
        {
            if (!flares_[i].active)
            {
                idx = i;
                break;
            }
            if (flares_[i].life < weakest)
            {
                weakest = flares_[i].life;
                idx = i;
            }
        }

        auto& flare = flares_[idx];
        flare.active = 1;
        flare.pos = rng_.uniform<uint8_t>(Constants::ChainLength);
        flare.radius =
            static_cast<uint8_t>(1 + (rng_.next8() % (std::max<uint8_t>(1, flare_rmax_))));
        flare.life = static_cast<uint8_t>(hot ? 255 : (180 + (rng_.next8() % 60)));
        flare.hue = static_cast<uint8_t>(hue_ + (rng_.next8() % 8));
    }

    uint8_t hue_{32};
    uint8_t sat_{240};
    uint8_t floor_v_{70};
    uint8_t pulse_{0};
    uint8_t pulse_decay_{6};
    uint8_t flicker_{4};
    uint8_t flare_decay_{14};
    uint8_t flare_rmax_{4};
    uint8_t spawn_prob_{6};
    uint8_t phase_{0};

    std::array<uint8_t, Constants::ChainLength> grain_{};
    Flare flares_[kMaxFlares]{};
    LedUtil::XorShift32 rng_{0xD1E5F00Du};
};
} // namespace Animations
