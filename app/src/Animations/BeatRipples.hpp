//
// Created by bened on 07/12/2025.
//
#pragma once

// -------------------- BeatRipples --------------------
// On beat, spawn a ripple from a random origin traveling around the ring.
template <size_t LedCount>
class BeatRipples final : public Animations::IAnimation<LedCount> {
public:
    explicit BeatRipples(uint8_t fade = 24, uint8_t speed_px = 2, uint8_t max_ripples = 3) noexcept
        : fade_(fade), speed_(speed_px), max_active_(static_cast<uint8_t>(std::min<uint8_t>(max_ripples, kMaxRipples))) {}

    void ProcessNextFrame(typename Animations::IAnimation<LedCount>::LedChain &leds) override {
        LedUtil::fade(leds, fade_);

        for (uint8_t i = 0; i < kMaxRipples; ++i) {
            auto& r = rip_[i];
            if (!r.active) continue;

            r.radius = static_cast<uint16_t>(r.radius + speed_);
            if (r.radius >= kMaxRadius) { r.active = false; continue; }

            const uint8_t val = static_cast<uint8_t>(std::max<int>(40, 255 - (r.radius * 510 / LedCount)));
            const led_rgb c = LedUtil::hsv(r.hue, 255, val);

            const size_t p1 = wrap_index<LedCount>(static_cast<int>(r.origin) + static_cast<int>(r.radius));
            const size_t p2 = wrap_index<LedCount>(static_cast<int>(r.origin) - static_cast<int>(r.radius));

            LedUtil::add_sat(leds[p1], c);
            if (p2 != p1) LedUtil::add_sat(leds[p2], c);
        }
        hue_ = static_cast<uint8_t>(hue_ + 1);
    }

    void ProcessNextBeat() override {
        // pick slot
        uint8_t idx = 0xFF;
        for (uint8_t i = 0; i < kMaxRipples; ++i) {
            if (!rip_[i].active) { idx = i; break; }
        }
        if (idx == 0xFF) { idx = head_; head_ = static_cast<uint8_t>((head_ + 1) % max_active_); }

        rip_[idx].active = true;
        rip_[idx].radius = 0;
        rip_[idx].hue    = static_cast<uint8_t>(hue_ + 12);
        rip_[idx].origin = rng_.uniform<uint8_t>(static_cast<uint8_t>(LedCount));
    }

private:
    struct Ripple { bool active{false}; uint16_t radius{0}; uint8_t hue{0}; uint8_t origin{0}; };
    static constexpr uint16_t kMaxRadius = (LedCount > 1) ? static_cast<uint16_t>(LedCount / 2) : 0;
    static constexpr uint8_t  kMaxRipples = 4;

    uint8_t fade_{24}, speed_{2}, max_active_{3};
    uint8_t head_{0};
    uint8_t hue_{0};
    Ripple rip_[kMaxRipples]{};
    LedUtil::XorShift32 rng_{0xA5A5BEEFu};
};
