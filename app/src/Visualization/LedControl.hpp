//
// Created by bened on 01/11/2025.
//

#pragma once

#include <zephyr/drivers/gpio.h>

#include "Utils/Logger.hpp"

namespace Visualization
{
class LedControl final
    {
    public:
        explicit LedControl(gpio_dt_spec* gpio, Utils::Logger& logger)
            : gpio_(gpio), active_(false), logger_(logger)
        {
            if (!gpio_is_ready_dt(gpio))
            {
                logger_.error("Gpio initialization failed.");
            }

            const auto ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT);
            if (ret != 0)
            {
                logger_.error("Gpio configuration failed: %d.", ret);
            }

            logger_.info("Signal LED initialized.");
        }

        void Set(const bool active)
        {
            if (active_ == active)
            {
                return;
            }

            const auto ret = gpio_pin_set_dt(gpio_, !active);
            if (ret != 0)
            {
                logger_.error("Gpio control failed: %d.", ret);
                return;
            }

            active_ = active;
            logger_.debug("Signal LED set to %s.", active_ ? "on" : "off");
        }

        void Toggle()
        {
            Set(!active_);
        }

    private:
        gpio_dt_spec* gpio_;
        bool active_;
        Utils::Logger& logger_;
    };
} // namespace Visualization
