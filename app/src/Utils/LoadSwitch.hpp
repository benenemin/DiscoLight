//
// Created by bened on 30/12/2025.
//
#pragma once

namespace Utils
{
    class LoadSwitch
    {
    public:
        explicit LoadSwitch(gpio_dt_spec* loadSwitchSpec, Logger& logger)
            : load_switch_spec_(loadSwitchSpec), logger_(logger)
        {
            if (!gpio_is_ready_dt(load_switch_spec_))
            {
                this->logger_.error("Gpio initialization failed.");
            }

            const auto ret = gpio_pin_configure_dt(load_switch_spec_, GPIO_OUTPUT);
            if (ret != 0)
            {
                this->logger_.error("Gpio configuration failed: %d.", ret);
            }
        }

        void Open()
        {
            if (!closed)
            {
                return;
            }

            const auto ret = gpio_pin_set_dt(load_switch_spec_, 0);
            if (ret != 0)
            {
                this->logger_.error("Gpio control failed: %d.", ret);
            }
            closed = false;
        }

        void Close()
        {
            if (closed)
            {
                return;
            }
            const auto ret = gpio_pin_set_dt(load_switch_spec_, 1);
            if (ret != 0)
            {
                this->logger_.error("Gpio control failed: %d.", ret);
            }
            closed = true;
        }

    private:
        gpio_dt_spec* load_switch_spec_;
        Logger& logger_;

        bool closed = false;
    };
};
