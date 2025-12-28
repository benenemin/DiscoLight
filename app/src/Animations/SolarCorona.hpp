//
// Created by bened on 09/12/2025.
//

// -----------------------------------------------------------------------------
// SolarCorona
//   Visual idea:
//     * A warm base “disc” (amber/yellow) with slight rotating texture/grain.
//     * Random short “flares/spicules” radiating both directions around a seed.
//     * Beat -> strong brightness envelope + spawn a few hot flares.
// -----------------------------------------------------------------------------
template <size_t LedCount>
class SolarCorona final : public Animations::IAnimation<LedCount> {
public:

    // Tunables (all 0..255 domain where applicable)
    explicit SolarCorona(uint8_t base_hue        = 32,   // ~amber
                         uint8_t base_sat        = 240,  // saturated
                         uint8_t base_floor_v    = 70,   // dark floor
                         uint8_t pulse_decay     = 10,    // per-frame decay
                         uint8_t flicker_amount  = 4,    // random micro flicker
                         uint8_t flare_decay     = 14,   // per-frame flare life drop
                         uint8_t flare_max_radius= 4,    // LED steps from center
                         uint8_t flare_spawn_prob= 6     // 0..255 chance each frame
                         ) noexcept
        : hue_(base_hue),
          sat_(base_sat),
          floor_v_(base_floor_v),
          pulse_decay_(pulse_decay),
          flicker_(flicker_amount),
          flare_decay_(flare_decay),
          flare_rmax_(flare_max_radius),
          spawn_prob_(flare_spawn_prob)
    {
        // Seed a static texture (grain). Values in [160..255]
        for (size_t i = 0; i < LedCount; ++i) {
            const uint8_t r = static_cast<uint8_t>(160u + (rng_.next8() % 96u));
            grain_[i] = r;
        }
        // Pre-place a couple small flares so it’s alive from frame 1
        spawn_flare_(/*hot=*/false);
        spawn_flare_(/*hot=*/false);
    }

    // --- Animation tick ---
    void ProcessNextFrame(typename Animations::IAnimation<LedCount>::LedChain &leds) override
    {
        // 1) Base corona: warm hue + rotating grain + pulse envelope + micro-flicker
        const uint8_t core = static_cast<uint8_t>(floor_v_ + ((uint16_t(pulse_) * 170u) >> 8)); // map pulse→[floor..~240]
        const uint8_t phase = phase_; // local copy for consistent frame
        for (size_t i = 0; i < LedCount; ++i) {
            const uint8_t g = grain_[(i + phase) % LedCount];       // rotate the texture
            uint8_t v = LedUtil::scale8(core, g);                    // texture-modulated brightness
            if (flicker_) v = LedUtil::sadd8(v, rng_.uniform<uint8_t>(flicker_ + 1)); // tiny sparkle
            leds[i] = LedUtil::hsv(hue_, sat_, v);
        }

        // 2) Draw active flares (additive over base)
        for (auto& f : flares_) {
            if (!f.active) continue;
            // center gets hottest/whitest; edges fade quickly
            for (uint8_t d = 0; d <= f.radius; ++d) {
                const uint8_t fall = falloff_(f.radius, d);          // 0..255
                const uint8_t val  = LedUtil::scale8(f.life, fall);  // 0..255
                // Slight “white hot” core; edges stay colored
                const uint8_t hot_boost = (d <= 1) ? (val / 3) : 0;
                const led_rgb color_core = LedUtil::hsv(f.hue, sat_, val);
                led_rgb color = color_core;
                if (hot_boost) { // add white to center to look hotter
                    LedUtil::add_sat(color, led_rgb{hot_boost, hot_boost, hot_boost});
                }
                const size_t p1 = wrap_index<LedCount>(int(f.pos) + d);
                const size_t p2 = wrap_index<LedCount>(int(f.pos) - d);
                LedUtil::add_sat(leds[p1], color);
                if (p2 != p1) LedUtil::add_sat(leds[p2], color);
            }
            // decay life; retire if done
            f.life = (f.life > flare_decay_) ? static_cast<uint8_t>(f.life - flare_decay_) : 0;
            if (f.life == 0) f.active = 0;
        }

        // 3) Occasionally spawn a subtle background flare
        if ((rng_.next8() & 0xFF) < spawn_prob_) {
            spawn_flare_(/*hot=*/false);
        }

        // 4) Evolve base state
        if (pulse_ > pulse_decay_) pulse_ = static_cast<uint8_t>(pulse_ - pulse_decay_);
        else pulse_ = 0;
        hue_  = static_cast<uint8_t>(hue_ + 1);    // slow color drift
        phase_ = static_cast<uint8_t>(phase_ + 1); // rotate grain
    }

    // --- Beat hook ---
    void ProcessNextBeat() override
    {
        // Boost pulse envelope and hue; spawn 2–3 hot flares
        pulse_ = 255;
        hue_   = static_cast<uint8_t>(hue_ + 6);
        const uint8_t n = static_cast<uint8_t>(2 + (rng_.next8() % 2));
        for (uint8_t i = 0; i < n; ++i) spawn_flare_(/*hot=*/true);
    }

private:
    // One flare record
    struct Flare {
        uint8_t active{0};
        uint8_t pos{0};     // LED index
        uint8_t radius{0};  // in LEDs
        uint8_t life{0};    // brightness 0..255
        uint8_t hue{0};     // near the base hue
    };

    static constexpr uint8_t kMaxFlares = 8;

    // Quadratic-ish falloff (center 255 → edge 0)
    static uint8_t falloff_(uint8_t radius, uint8_t dist) noexcept {
        if (dist >= radius) return (radius ? 0u : 255u);
        const uint16_t t  = static_cast<uint16_t>(255u * dist / (radius ? radius : 1u));
        const uint16_t t2 = static_cast<uint16_t>((t * t + 127u) / 255u);
        const int val = 255 - static_cast<int>(t2);
        return static_cast<uint8_t>(val < 0 ? 0 : val);
    }

    void spawn_flare_(bool hot) noexcept {
        // Find a slot (free or the weakest one)
        uint8_t idx = 0xFF;
        uint8_t weakest = 255;
        for (uint8_t i = 0; i < kMaxFlares; ++i) {
            if (!flares_[i].active) { idx = i; break; }
            if (flares_[i].life < weakest) { weakest = flares_[i].life; idx = i; }
        }
        auto& f = flares_[idx];
        f.active = 1;
        f.pos    = rng_.uniform<uint8_t>(static_cast<uint8_t>(LedCount));
        f.radius = static_cast<uint8_t>(1 + (rng_.next8() % (std::max<uint8_t>(1, flare_rmax_))));
        f.life   = static_cast<uint8_t>( hot ? 255 : (180 + (rng_.next8() % 60)) );
        // Slight hue jitter around current base hue; hot flares are a bit whiter (smaller saturation effect comes from white boost)
        f.hue    = static_cast<uint8_t>(hue_ + (rng_.next8() % 8));
    }

    // ---- State ----
    uint8_t hue_{32};
    uint8_t sat_{240};
    uint8_t floor_v_{70};

    uint8_t pulse_{0};
    uint8_t pulse_decay_{6};
    uint8_t flicker_{4};

    uint8_t flare_decay_{14};
    uint8_t flare_rmax_{4};
    uint8_t spawn_prob_{6};  // 0..255

    uint8_t phase_{0};       // rotates the grain pattern
    std::array<uint8_t, LedCount> grain_{};   // static texture
    Flare flares_[kMaxFlares]{};
    LedUtil::XorShift32 rng_{0xD1E5F00Du};
};