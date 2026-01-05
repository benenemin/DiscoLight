//
// Created by bened on 11/11/2025.
//

#pragma once

#include "Utils/LedUtils.hpp"
#include "zephyr/drivers/led_strip.h"

using namespace LedUtil;

class BeatPulse final : public Animations::IAnimation
{
public:
    // color in HSV; hue animates slowly, v is controlled by pulse level
    explicit BeatPulse(uint8_t hue0 = 0, uint8_t sat = 255,
                       uint8_t decay = 5, uint8_t hue_step = 1)
        : hue_(hue0), sat_(sat), decay_(decay), hue_step_(hue_step)
    {
    }

    typedef array<led_rgb, Constants::ChainLength> ledChain;
    void ProcessNextFrame(typename Animations::IAnimation::LedChain &leds) override
    {
        // decay pulse level
        if (level_ > decay_) level_ -= decay_;
        else level_ = 0;

        const uint8_t v = level_; // 0..255
        const led_rgb c = hsv(hue_, sat_, v);

        for (auto& px : leds)
        {
            // overwrite (or use add_sat for additive flare)
            px = c;
        }

        // slow hue drift
        hue_ += hue_step_;
    }

    void ProcessNextBeat() override
    {
        // kick pulse; clamp to 255
        level_ = 255;
        hue_ += 8; // small hue jump per beat
    }

private:
    uint8_t hue_ = 0, sat_ = 255;
    uint8_t level_ = 0; // brightness level (0..255)
    uint8_t decay_ = 8; // per-frame decay
    uint8_t hue_step_ = 1; // per-frame hue drift
};
