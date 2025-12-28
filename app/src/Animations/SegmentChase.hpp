//
// Created by bened on 09/12/2025.
//

// -------------------- SegmentChase --------------------
// Divide ring into K segments; highlight one segment; beat advances.
template <size_t LedCount>
class SegmentChase final : public Animations::IAnimation<LedCount> {
public:
    explicit SegmentChase(uint8_t segments = 12, uint8_t tail_fade = 50) noexcept
        : segs_(segments ? segments : 1), tail_(tail_fade) {}

    void ProcessNextFrame(typename Animations::IAnimation<LedCount>::LedChain &leds) override {
        LedUtil::fade(leds, tail_);

        const size_t seg_len = static_cast<size_t>(LedCount / segs_);
        const size_t start = (static_cast<size_t>(active_) * seg_len) % LedCount;

        // Gradient within the active segment
        for (size_t i = 0; i < seg_len; ++i) {
            const size_t idx = wrap_index<LedCount>(static_cast<int>(start + i));
            const uint8_t t = static_cast<uint8_t>((i * 255) / (seg_len ? seg_len : 1));
            const led_rgb c = LedUtil::hsv(hue_, 255, static_cast<uint8_t>(255 - t/2));
            LedUtil::add_sat(leds[idx], c);
        }
        hue_ = static_cast<uint8_t>(hue_ + 1);
    }

    void ProcessNextBeat() override {
        active_ = static_cast<uint8_t>((active_ + 1) % segs_);
        hue_ = static_cast<uint8_t>(hue_ + 16);
    }

private:
    uint8_t segs_{12}, tail_{50};
    uint8_t active_{0};
    uint8_t hue_{0};
};
