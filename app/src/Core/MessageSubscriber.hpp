//
// Created by bened on 28/12/2025.
//
#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "MessagePublisher.hpp"
#include "ThreadWorker.hpp"
#include "Utils/Detail.hpp"

extern "C" {
#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
}

namespace zbus_cpp
{
    template <typename... MsgTs>
    class MessageSubscriber final
    {
    public:
        using Observer = zbus_observer;

        constexpr explicit MessageSubscriber(Utils::ThreadWorker& threadWorker,
                                             const Observer* subscriber,
                                             Topic<MsgTs>... topics)
            : thread_worker_(threadWorker), subscriber_{subscriber}, topics_{topics...}
        {
        }

        MessageSubscriber(const MessageSubscriber&) = delete;
        MessageSubscriber& operator=(const MessageSubscriber&) = delete;

        void AttachLogger(Utils::Logger& logger)
        {
            logger_ = &logger;
        }

        void Initialize(int prio)
        {
            prio_ = prio;
            if (logger_)
            {
                logger_->info("Message subscriber initialized with thread priority %d.", prio_);
            }
        }

        void Start()
        {
            if (running_)
            {
                if (logger_)
                {
                    logger_->warning("Message subscriber already running.");
                }
                return;
            }
            running_ = true;
            if (logger_)
            {
                logger_->info("Starting message subscriber worker.");
            }

            thread_worker_.Start([this]
            {
                while (true)
                {
                    const int rc = DispatchOnce(K_FOREVER);
                    if (rc != 0 && rc != -ENOENT)
                    {
                        if (logger_)
                        {
                            logger_->warning("Message dispatch returned: %d", rc);
                        }
                    }
                }
            }, prio_);
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

            // Must be callable as void(MsgT&)
            static_assert(std::is_invocable_r_v<void, F, MsgT&>,
                          "Handler must be callable as: void(MsgT&)");

            auto& slot = std::get<Slot<MsgT>>(slots_);
            slot.callback = std::function<void(MsgT&)>(std::forward<F>(f));
            slot.used = static_cast<bool>(slot.callback);
            if (slot.used && logger_)
            {
                logger_->info("Registered subscriber callback for message size %d bytes.",
                              static_cast<int>(sizeof(MsgT)));
            }
            return slot.used ? 0 : -EINVAL;
        }

        template <typename MsgT>
        void Unsubscribe() noexcept
        {
            static_assert(is_configured_<MsgT>(),
                          "MsgT not configured in this MessageSubscriber");
            std::get<Slot<MsgT>>(slots_) = Slot<MsgT>{};
            if (logger_)
            {
                logger_->info("Unregistered subscriber callback for message size %d bytes.",
                              static_cast<int>(sizeof(MsgT)));
            }
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

        Utils::ThreadWorker& thread_worker_;
        bool running_ = false;
        int prio_ = 0;
        Utils::Logger* logger_{nullptr};

        const Observer* subscriber_{nullptr};

        std::tuple<zbus_cpp::Topic<MsgTs>...> topics_{};
        std::tuple<Slot<MsgTs>...> slots_{};

        static constexpr std::size_t kMaxMsgSize = Detail::max_sizeof<MsgTs...>();
        std::array<std::byte, kMaxMsgSize> rx_buf_{};
    };
} // namespace zbus_cpp
