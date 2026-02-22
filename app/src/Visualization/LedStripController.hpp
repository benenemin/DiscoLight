//
// Created by bened on 02/11/2025.
//

#pragma once

#include <array>

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>

#include "Constants.hpp"
#include "Utils/Logger.hpp"

namespace Visualization
{
    class LedStripController final
    {
    public:
        using LedChain = std::array<led_rgb, Constants::ChainLength>;

        LedStripController(const device* ledStrip, Utils::Logger& logger)
            : logger_(logger), led_strip_(ledStrip)
        {
        }

        void Initialize() const
        {
            if (led_strip_ == nullptr)
            {
                this->logger_.error("Led strip device pointer is null.");
                return;
            }

            const auto ret = device_is_ready(led_strip_);
            if (ret != 0)
            {
                this->logger_.error("Led strip initialization failed: %d.", ret);
                return;
            }

            this->logger_.info("Led strip initialized.");
        }

        LedChain* GetLeds()
        {
            return &leds_;
        }

        void Clear()
        {
            logger_.info("Clearing LED strip.");
            for (auto& led : leds_)
            {
                SetLedColor(led, 0, 0, 0);
            }
            UpdateLedStrip();
        }

        void FlashColor(const uint8_t red, const uint8_t green, const uint8_t blue)
        {
            logger_.debug("Flashing LED strip with RGB(%d, %d, %d).", red, green, blue);
            for (size_t i = 0; i < leds_.size(); ++i)
            {
                SetLedColor(leds_[i], red, green, blue);
            }
            UpdateLedStrip();
        }

        void UpdateLedStrip()
        {
            auto ret = led_strip_update_rgb(this->led_strip_, leds_.data(), Constants::ChainLength);
            if (ret != 0)
            {
                this->logger_.error("Led strip update failed: %d.", ret);
            }
        }

    private:
        static void SetLedColor(led_rgb& led, const uint8_t red, const uint8_t green, const uint8_t blue)
        {
            led.r = red;
            led.g = green;
            led.b = blue;
        }

        Utils::Logger& logger_;
        const device* led_strip_;
        LedChain leds_{};
    };
} // namespace Visualization
