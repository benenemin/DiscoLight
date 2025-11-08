//
// Created by bened on 18/10/2025.
//

#pragma once

#include <zephyr/kernel.h>
#include <functional>


namespace Utils
{
    class PeriodicTimer {
    public:
        using Callback = std::function<void()>;

        PeriodicTimer() = default;

        // Call once before start(); safe to call multiple times.
        void init(Callback cb) {
            cb_ = std::move(cb);

            work_wrap_.self = this;
            k_work_init(&work_wrap_.work, &PeriodicTimer::work_trampoline);

            k_timer_init(&timer_,
                         &PeriodicTimer::expiry_trampoline,
                         /*stop handler*/ nullptr);
            k_timer_user_data_set(&timer_, this);
        }

        // Start periodic timer: first fire after 'period', then every 'period'.
        void start(const int period_us) {
            this->periodUs_ = period_us;
            k_timer_start(&timer_, K_NO_WAIT, K_USEC(period_us));
        }

        // Stop the timer (idempotent).
        void stop() {
            k_timer_stop(&timer_);
        }

        int periodUs_;

    private:
        // Runs in ISR context on each expiry; schedule work to run in thread context.
        static void expiry_trampoline(k_timer* t)
        {
            auto* self = static_cast<PeriodicTimer*>(k_timer_user_data_get(t));
            if (!self || !self->cb_)
            {
                return;
            }
            k_work_submit(&self->work_wrap_.work);
        }

        // Runs in system workqueue thread.
        static void work_trampoline(k_work* w) {
            const auto* wrap = CONTAINER_OF(w, WorkWrap, work);         // standard-layout OK
            const auto* self = static_cast<PeriodicTimer*>(wrap->self); // our instance
            if (!self || !self->cb_)
            {
                return;
            }
            self->cb_();
        }

        struct WorkWrap {
            k_work work;     // must be first for predictable layout, but not required
            void*  self{};   // back-pointer to PeriodicTimer
        };

        k_timer  timer_{};
        WorkWrap  work_wrap_{};
        Callback cb_{nullptr};
        void*    ctx_{nullptr};
    };

};
