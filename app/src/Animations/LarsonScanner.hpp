//
// Created by bened on 07/12/2025.
//
#pragma once

#include "Utils/LedUtils.hpp"
#include "zephyr/drivers/led_strip.h"

using namespace LedUtil;

// =====================================================
// 2) LarsonScanner — bouncing dot with fading tail
//    Beat: reverse direction + brief boost
// =====================================================
template <size_t LedCount>
class LarsonScanner final : public Animations::IAnimation<LedCount> {
public:
    explicit LarsonScanner(uint8_t hue = 0, uint8_t tail_fade = 32, uint8_t speed = 1)
        : hue_(hue), tail_(tail_fade), speed_(speed) {}

    void ProcessNextFrame(typename Animations::IAnimation<LedCount>::LedChain &leds) override {
        auto& s = leds;

        fade(s, tail_);

        // move position (fixed-point 8.8 for smoothness)
        subpos_ += (dir_ > 0) ? (speed_ << 8) : uint16_t(0 - (speed_ << 8));
        int pos = int(subpos_ >> 8);

        if (pos <= 0)
        {
            pos = LedCount - pos;
            dir_ = 1;
            subpos_ = LedCount - pos;
        }
        else if (pos >= int(LedCount - 1))
        {
            pos = pos % LedCount;
            dir_ = 1;
            subpos_ = uint16_t(pos << 8) % LedCount;
        }

        // head brightness stronger, tail is already faded in buffer
        const led_rgb head = hsv(hue_, 255, 255);
        s[static_cast<size_t>(pos)] = head;

        // subtle hue drift
        hue_ += 1;
    }

    void ProcessNextBeat() override {
        dir_ = -dir_;                // bounce on beat
        // brief brightness boost via lower tail fade (one frame effect)
        if (tail_ > 8) tail_ -= 8;
    }

private:
    uint8_t  hue_  = 0;
    uint8_t  tail_ = 32;        // fade per frame (higher = shorter tail)
    uint8_t  speed_ = 1;        // pixels per frame (integer)
    int8_t   dir_  = 1;         // +1 / -1
    uint16_t subpos_ = 0;       // 8.8 fixed-point position
};