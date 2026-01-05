//
// Created by bened on 09/12/2025.
//

#include "IAnimation.hpp"
// If Additive == false: overwrite the strip with the flash color while active, else black.
// If Additive == true : add flash color on top of existing content (preserves what’s underneath).
class BeatFlash final : public Animations::IAnimation
{
public:
    // color: flash color (RGB)vv
    // hold_frames: number of frames to keep at full brightness after a beat
    // decay_per_frame: how much brightness to subtract each frame after hold (0..255). Larger = faster fade.
    explicit BeatFlash(led_rgb color = {255, 255, 255},
                       uint8_t hold_frames = 2,
                       uint8_t decay_per_frame = 24) noexcept
        : color_(color),
          hold_(hold_frames),
          decay_(decay_per_frame)
    {
    }

    // ---- IAnimation ----
    void ProcessNextFrame(typename Animations::IAnimation::LedChain& leds) override
    {
        uint8_t v = 0;

        if (hold_cnt_ > 0)
        {
            v = 255;
        }
        else if (level_ > 0)
        {
            v = level_;
        }


        if (v)
        {
            const led_rgb c = scale_color_(color_, v);
            std::fill(leds.begin(), leds.end(), c);
        }
        else
        {
            // fully off between flashes
            std::fill(leds.begin(), leds.end(), led_rgb{0, 0, 0});
        }


        // Envelope update (run AFTER drawing so a fresh beat shows at full V first)
        if (hold_cnt_ > 0)
        {
            --hold_cnt_;
        }
        else if (level_ > 0)
        {
            level_ = (level_ > decay_) ? static_cast<uint8_t>(level_ - decay_) : 0;
        }
    }

    void ProcessNextBeat() override
    {
        hold_cnt_ = hold_;
        level_ = 255; // reset to full brightness
    }

private:
    static led_rgb scale_color_(const led_rgb& c, uint8_t v) noexcept
    {
        return {
            scale8(c.r, v),
            scale8(c.g, v),
            scale8(c.b, v)
        };
    }

    // State
    led_rgb color_{255, 160, 20};
    uint8_t hold_{2};
    uint8_t decay_{24};
    uint8_t hold_cnt_{0};
    uint8_t level_{0}; // 0..255 (current brightness outside hold)
};

