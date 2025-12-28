//
// Created by bened on 28/12/2025.
//
#pragma once

#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <functional>   // <-- important

extern "C" {
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
}

namespace detail
{
    template <typename... Ts>
    constexpr std::size_t max_sizeof() noexcept {
        std::size_t m = 1U;
        ((m = (m < sizeof(Ts)) ? sizeof(Ts) : m), ...);
        return m;
    }
}

namespace zbus_cpp
{
    // If you already have a ThreadWorker elsewhere, prefer not redefining it here.
    // Keeping your structure as-is.

    class ThreadWorker
    {
    public:
        using Worker = std::function<void()>;

        explicit ThreadWorker(k_thread_stack_t& stack, size_t stack_size)
            : stack(stack), stack_size(stack_size)
        {
        }

        ~ThreadWorker() = default;

        void Start(const Worker& worker, const int prio)
        {
            this->worker = worker;
            thread_tid = k_thread_create(&thread_data, &this->stack,
                                         this->stack_size, thread_entry_point,
                                         this, nullptr, nullptr, prio, 0, K_NO_WAIT);
        }

    private:
        k_thread_stack_t& stack;
        size_t stack_size;
        Worker worker;
        k_thread thread_data{};
        k_tid_t thread_tid{};

        static void thread_entry_point(void* arg1, void*, void*)
        {
            auto self = static_cast<ThreadWorker*>(arg1);
            if (!self || !self->worker)
            {
                return;
            }
            self->worker();
        }
    };

    template <typename... MsgTs>
    class MessageSubscriber final
    {
    public:
        using Observer = zbus_observer;

        constexpr explicit MessageSubscriber(ThreadWorker& threadWorker,
                                             const Observer* subscriber,
                                             Topic<MsgTs>... topics)
            : thread_worker_(threadWorker), subscriber_{subscriber}, topics_{topics...}
        {
        }

        MessageSubscriber(const MessageSubscriber&) = delete;
        MessageSubscriber& operator=(const MessageSubscriber&) = delete;

        void Initialize(int prio)
        {
            this->prio = prio;
        }

        void Start()
        {
            if (running) return;
            running = true;

            thread_worker_.Start([this]
            {
                while (true)
                {
                    (void)DispatchOnce(K_FOREVER);
                }
            }, prio);
        }

        /**
         * Subscribe with any callable:
         *   - lambda:  [](const MsgT& m) { ... }
         *   - functor: struct H { void operator()(const MsgT&) { ... } };
         *   - std::function<void(const MsgT&)>
         */
        template <typename MsgT, typename F>
        int Subscribe(F&& f) noexcept
        {
            static_assert(is_configured_<MsgT>(),
                          "MsgT not configured in this MessageSubscriber");

            // Must be callable as void(const MsgT&)
            static_assert(std::is_invocable_r_v<void, F, MsgT&>,
                          "Handler must be callable as: void(const MsgT&)");

            auto& slot = std::get<Slot<MsgT>>(slots_);
            slot.callback = std::function<void(MsgT&)>(std::forward<F>(f));
            slot.used = static_cast<bool>(slot.callback);
            return slot.used ? 0 : -EINVAL;
        }

        template <typename MsgT>
        void Unsubscribe() noexcept
        {
            static_assert(is_configured_<MsgT>(),
                          "MsgT not configured in this MessageSubscriber");
            std::get<Slot<MsgT>>(slots_) = Slot<MsgT>{};
        }

        int DispatchOnce(k_timeout_t timeout = K_FOREVER) noexcept
        {
            if (subscriber_ == nullptr)
            {
                return -EINVAL;
            }

            const zbus_channel* chan = nullptr;

            const int rc = zbus_sub_wait(subscriber_, &chan, timeout);
            if (rc != 0)
            {
                return rc;
            }
            if (chan == nullptr)
            {
                return -EIO;
            }

            const std::size_t msg_size = zbus_chan_msg_size(chan);
            if (msg_size > rx_buf_.size())
            {
                return -ENOBUFS;
            }

            const int read_rc = zbus_chan_read(chan, rx_buf_.data(), K_MSEC(250));
            if (read_rc != 0)
            {
                return read_rc;
            }

            bool handled = false;
            (try_dispatch_<MsgTs>(chan, msg_size, handled), ...);
            return handled ? 0 : -ENOENT;
        }

    private:
        template <typename MsgT>
        struct Slot
        {
            std::function<void(MsgT&)> callback{};
            bool used{false};
        };

        template <typename MsgT>
        static constexpr bool is_configured_() noexcept
        {
            return (std::is_same_v<MsgT, MsgTs> || ...);
        }

        template <typename MsgT>
        void try_dispatch_(const zbus_channel* chan, std::size_t msg_size, bool& handled) noexcept
        {
            if (handled) return;

            const auto& topic = std::get<zbus_cpp::Topic<MsgT>>(topics_);
            if (topic.chan == nullptr || topic.chan != chan)
            {
                return;
            }

            auto& slot = std::get<Slot<MsgT>>(slots_);
            if (!slot.used || !slot.callback)
            {
                return;
            }

            if (msg_size != sizeof(MsgT))
            {
                return; // type/channel mismatch guard
            }

            auto& m = *reinterpret_cast<MsgT*>(rx_buf_.data());
            slot.callback(m);
            handled = true;
        }

        ThreadWorker& thread_worker_;
        bool running = false;
        int prio = 0;

        const Observer* subscriber_{nullptr};

        std::tuple<zbus_cpp::Topic<MsgTs>...> topics_{};
        std::tuple<Slot<MsgTs>...> slots_{};

        static constexpr std::size_t kMaxMsgSize = detail::max_sizeof<MsgTs...>();
        std::array<std::byte, kMaxMsgSize> rx_buf_{};
    };
}

