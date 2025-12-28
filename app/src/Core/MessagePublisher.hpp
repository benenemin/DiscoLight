//
// Created by bened on 28/12/2025.
//

#pragma once

#include <tuple>
#include <type_traits>

extern "C" {
#include <zephyr/zbus/zbus.h>
#include <zephyr/kernel.h>
}

namespace zbus_cpp
{
    /**
     * Typed wrapper around a zbus channel pointer.
     * Binding MsgT at compile time makes Publish<MsgT>() type-safe.
     */
    template <typename MsgT>
    struct Topic final
    {
        static_assert(std::is_trivially_copyable_v<MsgT>,
                      "zbus messages must be trivially copyable");

        const zbus_channel* chan{nullptr};

        constexpr explicit Topic(const zbus_channel* c) : chan(c)
        {
        }
    };

    template <typename... MsgTs>
    class MessagePublisher final
    {
    public:
        constexpr explicit MessagePublisher(Topic<MsgTs>... topics) : topics_{topics...}
        {
        }

        MessagePublisher(const MessagePublisher&) = delete;
        MessagePublisher& operator=(const MessagePublisher&) = delete;

        /**
         * Publish a message of type MsgT to the channel that was provided as Topic<MsgT>.
         *
         * Usage:
         *   publisher.Publish<BeatEvent>(beatEvent);
         */
        template <typename MsgT>
        int Publish(const MsgT& msg, const k_timeout_t timeout = K_NO_WAIT) const noexcept
        {
            static_assert(IsConfigured_<MsgT>(),
                          "MsgT not configured in this MessagePublisher. "
                          "Pass Topic<MsgT> to the constructor.");

            static_assert(std::is_trivially_copyable_v<MsgT>,
                          "MsgT must be trivially copyable");

            const auto& topic = std::get<Topic<MsgT>>(topics_);
            if (topic.chan == nullptr)
            {
                return -EINVAL;
            }

            return zbus_chan_pub(topic.chan, &msg, timeout);
        }

    private:
        template <typename MsgT>
        static constexpr bool IsConfigured_() noexcept
        {
            return (std::is_same_v<MsgT, MsgTs> || ...);
        }

        std::tuple<Topic<MsgTs>...> topics_;
    };
} // namespace zbus_cpp
