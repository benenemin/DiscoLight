//
// Created by bened on 01/11/2025.
//

#pragma once

namespace Visualization
{
class LedControl
{
public:
    explicit LedControl(gpio_dt_spec *gpio, Logger& logger)
        :gpio(gpio), active(false), logger_(logger)
    {
        if (!gpio_is_ready_dt(gpio))
        {
            this->logger_.error("Gpio initialization failed.");
        }

        const auto ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT);
        if (ret != 0)
        {
            this->logger_.error("Gpio configuration failed: %d.", ret);
        }
    }

    void Set(const bool active)
    {
        const auto ret = gpio_pin_set_dt(gpio, !active);
        if (ret != 0)
        {
            this->logger_.error("Gpio control failed: %d.", ret);
        }

        this->active = active;
    }

    void Toggle()
    {
        this->active = !this->active;
        Set(active);
    }

private:
    gpio_dt_spec *gpio;
    bool active;
    Logger& logger_;
};
}
