//
// Created by bened on 07/12/2025.
//

#pragma once

#include <functional>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "Utils/Logger.hpp"

namespace UtilsButton
{
  enum class ButtonState : uint8_t
  {
    Pressed,
    ReleasedShort,
    ReleasedLong
  };

  class Button
  {
  public:
    using NotifyButtonEvt = std::function<void(ButtonState evt)>;

    explicit Button(gpio_dt_spec& buttonDevice)
      : button_device_(buttonDevice)
    {
      this->irq_cb_wrap.self = this;
      this->debounce_work_wrap.self = this;
    }

    void AttachLogger(Utils::Logger& logger)
    {
      logger_ = &logger;
    }

    int Initialize(const NotifyButtonEvt& notify)
    {
      if (logger_)
      {
        logger_->info("Initializing button driver.");
      }

      if (!notify)
      {
        if (logger_)
        {
          logger_->error("Button notify callback is not set.");
        }
        return -EINVAL;
      }

      this->notify_ = notify;

      if (!device_is_ready(this->button_device_.port))
      {
        if (logger_)
        {
          logger_->error("Button gpio device is not ready.");
        }
        return -EIO;
      }

      auto err = gpio_pin_configure_dt(&this->button_device_, GPIO_INPUT);
      if (err)
      {
        if (logger_)
        {
          logger_->error("Button gpio input configuration failed: %d", err);
        }
        return err;
      }

      err = gpio_pin_interrupt_configure_dt(&this->button_device_, GPIO_INT_EDGE_BOTH);
      if (err)
      {
        if (logger_)
        {
          logger_->error("Button gpio interrupt configuration failed: %d", err);
        }
        return err;
      }

      k_work_init_delayable(&this->debounce_work_wrap.work, &Button::DebounceWorkTrampoline_);
      gpio_init_callback(&this->irq_cb_wrap.irq_cb_, GpioIsrTrampoline, BIT(this->button_device_.pin));
      err = gpio_add_callback(this->button_device_.port, &this->irq_cb_wrap.irq_cb_);
      if (err)
      {
        if (logger_)
        {
          logger_->error("Button gpio callback registration failed: %d", err);
        }
        return err;
      }

      if (logger_)
      {
        logger_->info("Button driver initialized.");
      }
      return 0;
    }

  private:
    void OnCooldownExpired()
    {
      const int v = gpio_pin_get_dt(&button_device_);
      if (v < 0)
      {
        if (logger_)
        {
          logger_->error("Button gpio read failed: %d", v);
        }
        return;
      }

      if (!notify_)
      {
        if (logger_)
        {
          logger_->warning("Button event dropped because notify callback is missing.");
        }
        return;
      }

      const bool lvl = v != 0;
      if (lvl == last_level_)
      {
        return;
      }

      last_level_ = lvl;

      if (lvl)
      {
        timeLastPressed = k_uptime_get();
        if (logger_)
        {
          logger_->info("Button pressed.");
        }
        notify_(ButtonState::Pressed);
        return;
      }

      const auto press_duration_ms = static_cast<int>(k_uptime_delta(&timeLastPressed));

      //short button press
      if (press_duration_ms <= 2000)
      {
        if (logger_)
        {
          logger_->info("Button short release after %d ms.", press_duration_ms);
        }
        notify_(ButtonState::ReleasedShort);
        return;
      }

      //long button press
      if (logger_)
      {
        logger_->info("Button long release after %d ms.", press_duration_ms);
      }
      notify_(ButtonState::ReleasedLong);
    }


    // Runs in ISR context on each expiry; schedule work to run in thread context.
    static void GpioIsrTrampoline(const device* port, gpio_callback* cb, gpio_port_pins_t pins)
    {
      const auto* wrap = CONTAINER_OF(cb, IrqWrap, irq_cb_);
      auto* self = static_cast<Button*>(wrap->self);
      if (!self)
      {
        return;
      }
      // Re-arm or schedule debounce; collapsing rapid bounces
      k_work_reschedule(&self->debounce_work_wrap.work, K_MSEC(30));
    }

    // ---- Debounce handler (thread context) ----
    static void DebounceWorkTrampoline_(k_work* work)
    {
      auto* dwork = CONTAINER_OF(work, k_work_delayable, work);
      const auto* wrap = CONTAINER_OF(dwork, WorkWrap, work);
      auto* self = static_cast<Button*>(wrap->self);
      if (!self)
      {
        return;
      }
      self->OnCooldownExpired();
    }

    gpio_dt_spec& button_device_;

    struct IrqWrap
    {
      gpio_callback irq_cb_;
      void* self;
    };

    IrqWrap irq_cb_wrap{};

    struct WorkWrap
    {
      k_work_delayable work;
      void* self{};
    };

    WorkWrap debounce_work_wrap{};
    NotifyButtonEvt notify_{nullptr};
    Utils::Logger* logger_{nullptr};
    bool last_level_{false};
    int64_t timeLastPressed{0};
  };
};
