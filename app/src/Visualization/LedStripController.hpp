//
// Created by bened on 02/11/2025.
//

#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>

namespace Visualization
{
    template<size_t LedCount>
class LedStripController
{
public:
    LedStripController(const device *ledStrip, Logger &logger)
    : logger_(logger), led_strip_(ledStrip)
    {

    }

    void Initialize() const
    {
        const auto ret = device_is_ready(led_strip_);
        if (ret != 0)
        {
            this->logger_.error("Led strip initialization failed: %d.", ret);
        }
    }

    void Clear()
    {
        for (auto led : leds_)
        {
            SetLedColor(led, 0, 0, 0);
        }
        UpdateLedStrip();
    }

    void FlashColor(const uint8_t red, const uint8_t green, const uint8_t blue)
    {
        for (auto led : leds_)
        {
            SetLedColor(led, red, green, blue);
        }
        UpdateLedStrip();
    }


private:

        void UpdateLedStrip()
        {
            auto ret = led_strip_update_rgb(this->led_strip_, leds_.data(), LedCount);
            if (ret != 0)
            {
                this->logger_.error("Led strip update failed: %d.", ret);
            }
        }

    static void SetLedColor(led_rgb &led, const uint8_t red, const uint8_t green, const uint8_t blue)
        {
            led.r = red;
            led.g = green;
            led.b = blue;
        }

    Logger& logger_;
    const device* led_strip_;
    array<led_rgb, LedCount> leds_{};
};
}
