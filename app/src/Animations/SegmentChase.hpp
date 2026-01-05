//
// Created by bened on 09/12/2025.
//

// -------------------- SegmentChase --------------------
// Divide ring into K segments; highlight one segment; beat advances.

class SegmentChase final : public Animations::IAnimation
{
public:
    explicit SegmentChase(uint8_t segments = 12, uint8_t tail_fade = 50) noexcept
        : segs_(segments ? segments : 1), tail_(tail_fade)
    {
    }

    void ProcessNextFrame(Animations::IAnimation::LedChain& leds) override
    {
        LedUtil::fade(leds, tail_);

        const size_t seg_len = Constants::ChainLength / segs_;
        const size_t start = (static_cast<size_t>(active_) * seg_len) % Constants::ChainLength;

        // Gradient within the active segment
        for (size_t i = 0; i < seg_len; ++i)
        {
            const size_t idx = wrap_index<Constants::ChainLength>(static_cast<int>(start + i));
            const uint8_t t = static_cast<uint8_t>((i * 255) / (seg_len ? seg_len : 1));
            const led_rgb c = hsv(hue_, 255, static_cast<uint8_t>(255 - t / 2));
            LedUtil::add_sat(leds[idx], c);
        }
        hue_ = static_cast<uint8_t>(hue_ + 1);
    }

    void ProcessNextBeat() override
    {
        active_ = static_cast<uint8_t>((active_ + 1) % segs_);
        hue_ = static_cast<uint8_t>(hue_ + 16);
    }

private:
    uint8_t segs_{12}, tail_{50};
    uint8_t active_{0};
    uint8_t hue_{0};
};
