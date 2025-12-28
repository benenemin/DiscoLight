//
// Created by bened on 11/11/2025.
//
#pragma once
#include "zephyr/drivers/led_strip.h"

namespace LedUtil
{
    // Saturating add
    static uint8_t sadd8(const uint8_t a, const uint8_t b)
    {
        const uint16_t s = static_cast<uint16_t>(a) + static_cast<uint16_t>(b);
        return static_cast<uint8_t>(s > 255 ? 255 : s);
    }

    // Scale x by y/255 (rounded)
    static uint8_t scale8(const uint8_t x, const uint8_t y)
    {
        return static_cast<uint8_t>((static_cast<uint16_t>(x) * static_cast<uint16_t>(y) + 128) / 255);
    }

    // Fast HSV->RGB (0..255 for h,s,v), adapted from the classic 6-sector method
    static led_rgb hsv(const uint8_t h, const uint8_t s, const uint8_t v)
    {
        if (s == 0) return {v, v, v};
        const uint8_t region = h / 43; // 0..5
        const uint8_t rem = (h % 43) * 6; // 0..258
        const uint8_t p = scale8(v, 255 - s);
        const uint8_t q = scale8(v, 255 - scale8(s, rem));
        const uint8_t t = scale8(v, 255 - scale8(s, 255 - rem));

        switch (region)
        {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        default: return {v, p, q};
        }
    }

    // Fade in-place by factor (keep ≈(255-factor)/255 brightness)
    template <size_t N>
    static void fade(std::array<led_rgb, N>& strip, const uint8_t fade_by)
    {
        const uint8_t keep = 255 - fade_by;
        for (auto& px : strip)
        {
            px.r = scale8(px.r, keep);
            px.g = scale8(px.g, keep);
            px.b = scale8(px.b, keep);
        }
    }

    // Add color with saturation
    static void add_sat(led_rgb& dst, const led_rgb& src)
    {
        dst.r = sadd8(dst.r, src.r);
        dst.g = sadd8(dst.g, src.g);
        dst.b = sadd8(dst.b, src.b);
    }

    // Tiny PRNG (xorshift32)
    struct XorShift32
    {
        uint32_t s = 0xA3C59AC3u;

        uint32_t next()
        {
            uint32_t x = s;
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            s = x;
            return x;
        }

        uint8_t next8() { return static_cast<uint8_t>(next() >> 24); }
        uint16_t next16() { return static_cast<uint16_t>(next() >> 16); }

        template <typename T>
        T uniform(T max_exclusive)
        {
            return T(next() % (max_exclusive ? max_exclusive : 1));
        }
    };

    template <size_t N>
    static size_t wrap_index(int i) noexcept {
        const int m = static_cast<int>(N);
        int r = i % m;
        return static_cast<size_t>(r < 0 ? r + m : r);
    }
}
