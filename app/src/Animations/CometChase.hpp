//
// Created by bened on 09/12/2025.
//

#pragma once

// -------------------- CometChase --------------------
// Rotating bright head with fading tail; beat flips direction and boosts hue.
class CometChase final : public Animations::IAnimation
{
public:
    using LedChain = LedChain;
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
        set_count_(initial_count);
        reseed_positions_();
    }

    // ---- IAnimation ----
    void ProcessNextFrame(LedChain &leds) override
    {
        count++;
        if (count % 10 != 0)
        {
            return;
        }
        count = 0;
        // Tail fade; on a fresh beat we momentarily reduce the fade to “pop” the heads.
        const uint8_t fade_now = (boost_frames_ && tail_ > 8) ? static_cast<uint8_t>(tail_ - 8) : tail_;
        fade(leds, fade_now);

        // Move & draw each active comet
        const uint32_t period = (static_cast<uint32_t>(Constants::ChainLength) << 8); // Q8.8 ring length
        const uint32_t step = (static_cast<uint32_t>(speed_) << 8);

        for (uint8_t i = 0; i < active_; ++i)
        {
            auto& c = comets_[i];

            // Advance with wrap (Q8.8)
            if (dir_ > 0)
            {
                c.subpos += step;
                if (c.subpos >= period) c.subpos -= period;
            }
            else
            {
                c.subpos = (c.subpos >= step) ? (c.subpos - step) : (c.subpos + period - step);
            }

            const size_t idx = static_cast<size_t>(c.subpos >> 8);
            const uint8_t v_hd = boost_frames_ ? 255 : 220;

            stamp_head_(leds, idx, c.hue, v_hd);
        }

        if (boost_frames_ > 0) --boost_frames_;
        // slow hue drift for variety
        base_hue_ = static_cast<uint8_t>(base_hue_ + 1);
        update_hues_();
    }

    void ProcessNextBeat() override
    {
        // Flip direction + short “flash”
        dir_ = -dir_;
        boost_frames_ = 2;
        base_hue_ = static_cast<uint8_t>(base_hue_ + 8);
        update_hues_();
    }

private:
    struct Comet
    {
        uint32_t subpos = 0; // Q8.8 position on ring
        uint8_t hue = 0; // color
    };

    // Draw head (and tiny bloom) at idx
    static void add_at_(LedChain& s, size_t i, const led_rgb& c)
    {
        add_sat(s[wrap_index<Constants::ChainLength>(static_cast<int>(i))], c);
    }

    void stamp_head_(LedChain &s, size_t idx, uint8_t hue, uint8_t v) const
    {
        add_at_(s, idx, hsv(hue, 255, v));
        if (head_width_ >= 1)
        {
            const uint8_t v1 = static_cast<uint8_t>(v > 96 ? v - 96 : v / 2);
            add_at_(s, idx + 1, hsv(hue, 255, v1));
            add_at_(s, (idx == 0 ? Constants::ChainLength - 1 : idx - 1), hsv(hue, 255, v1));
        }
        if (head_width_ >= 2)
        {
            const uint8_t v2 = static_cast<uint8_t>(v1_scale_);
            add_at_(s, idx + 2, hsv(hue, 255, v2));
            add_at_(s, (idx + Constants::ChainLength - 2) % Constants::ChainLength, hsv(hue, 255, v2));
        }
    }

    void set_count_(uint8_t n) noexcept
    {
        active_ = (n == 0) ? 1 : (n > 5 ? 5 : n);
        update_hues_();
    }

    void reseed_positions_() noexcept
    {
        // Evenly distribute around the ring
        const uint32_t period = (static_cast<uint32_t>(Constants::ChainLength) << 8);
        for (uint8_t i = 0; i < active_; ++i)
        {
            const uint32_t step = (period * i) / (active_ ? active_ : 1);
            comets_[i].subpos = step;
        }
    }

    void update_hues_() noexcept
    {
        for (uint8_t i = 0; i < active_; ++i)
        {
            comets_[i].hue = static_cast<uint8_t>(base_hue_ + static_cast<uint8_t>(i * hue_stride_));
        }
    }

private:
    // Config/state
    uint8_t tail_ = 32;
    uint8_t speed_ = 1;
    uint8_t base_hue_ = 0;
    uint8_t hue_stride_ = 12;
    uint8_t head_width_ = 1; // 0..2
    uint8_t active_ = 3; // current number of comets
    int8_t dir_ = 1; // +1 / -1
    uint8_t boost_frames_ = 0; // small “flash” window
    uint8_t count = 0;

    // Pre-baked second ring halo intensity when head_width_==2
    static constexpr uint8_t v1_scale_ = 64;

    Comet comets_[5]{};
};
