//
// Created by bened on 09/12/2025.
//

// -------------------- 6) TwinWaveInterference --------------------
// Two sinusoidal brightness waves counter-rotate; beat increases contrast.

class TwinWaveInterference final : public Animations::IAnimation {
public:
    explicit TwinWaveInterference(uint8_t hue_a = 0, uint8_t hue_b = 128,
                                  uint8_t speed = 1, uint8_t base_v = 40) noexcept
        : hue_a_(hue_a), hue_b_(hue_b), speed_(speed), base_v_(base_v) {}

    void ProcessNextFrame(Animations::IAnimation::LedChain &leds) override {
        // phase increment (Q0.8 for smoothness)
        pha_ = static_cast<uint8_t>(pha_ + speed_);
        phb_ = static_cast<uint8_t>(phb_ - speed_);

        for (size_t i = 0; i < Constants::ChainLength; ++i) {
            // map i to angle 0..255
            const uint8_t ang = static_cast<uint8_t>((i * 256) / Constants::ChainLength);
            // cosine-ish waves via triangle blend
            const uint8_t wa = 255 - static_cast<uint8_t>(std::abs(int(ang + pha_) - 128) * 2);
            const uint8_t wb = 255 - static_cast<uint8_t>(std::abs(int(ang + phb_) - 128) * 2);

            uint16_t v = base_v_ + (contrast_ * wa) / 255 + (contrast_ * wb) / 255;
            if (v > 255) v = 255;

            const led_rgb ca = hsv(hue_a_, 255, static_cast<uint8_t>(v));
            const led_rgb cb = hsv(hue_b_, 255, static_cast<uint8_t>(v / 2));
            led_rgb out = ca;
            add_sat(out, cb);
            leds[i] = out;
        }
    }

    void ProcessNextBeat() override {
        contrast_ = static_cast<uint8_t>(std::min<int>(255, contrast_ + 64));
        hue_a_ = static_cast<uint8_t>(hue_a_ + 8);
        hue_b_ = static_cast<uint8_t>(hue_b_ + 8);
        // gentle decay will bring contrast back down over frames
        if (decay_counter_ == 0) decay_counter_ = 1;
    }

private:
    // apply a small decay each frame via lazy trick in ProcessNextFrame wrapper
    void decay_contrast_() noexcept {
        if (contrast_ > 2) contrast_ -= 2;
        else contrast_ = 0;
    }

    // Hook decay each call; wrap ProcessNextFrame via CRTP-like local trick
    struct DecayCaller {
        TwinWaveInterference* self;
        explicit DecayCaller(TwinWaveInterference* s) : self(s) {}
        ~DecayCaller() { self->decay_contrast_(); if (self->decay_counter_) --self->decay_counter_; }
    };

    uint8_t hue_a_{0}, hue_b_{128};
    uint8_t speed_{1}, base_v_{40};
    uint8_t pha_{0}, phb_{0};
    uint8_t contrast_{80};
    uint8_t decay_counter_{0};
};