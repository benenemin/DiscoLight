//
// Created by bened on 10/10/2025.
//

#pragma once
#include <zephyr/kernel.h>
#include <utility>

#include "Threads/IWorker.hpp"

namespace Threads {

    template <typename TopicT, int STACK_SIZE, typename Handler>
    class SubscriberThread final : public IWorker {
        static_assert(STACK_SIZE > 0, "STACK_SIZE must be positive");
        using Msg = typename TopicT::message_type;

    public:
        SubscriberThread(TopicT& topic, Handler handler,
                         const k_timeout_t recv_timeout = K_FOREVER)
            : topic_(topic), handler_(std::move(handler)), recv_to_(recv_timeout)
        {
            stack_area_ = k_thread_stack_alloc(STACK_SIZE, 0);
        }

        ~SubscriberThread() override
        {
            k_thread_stack_free(stack_area_);
        }

        void start(int prio, const char* name) override {
            if (this->running()) return;
            set_running(true);
            k_thread_create(&thread_,
                            stack_area_,                        // <-- declared below
                            K_THREAD_STACK_SIZEOF(&stack_area_), // <-- size of that stack
                            &SubscriberThread::entry_trampoline,
                            this, nullptr, nullptr,
                            prio, 0, K_NO_WAIT);

#if defined(CONFIG_THREAD_NAME)
            if (name) { k_thread_name_set(&thread_, name); }
#endif
        }

        void stop() override {
            set_running(false);
            // Optional hard stop: k_thread_abort(&thread_);
        }

    private:
        static void entry_trampoline(void* p1, void*, void*) {
            static_cast<SubscriberThread*>(p1)->run();
        }

        void run() {
            Msg msg{};
            while (this->running()) {
                if (!topic_.subscribe(msg, recv_to_)) {
                    continue; // timed out, re-check running flag
                }
                handler_(msg);
            }
        }

        TopicT&     topic_;
        Handler     handler_;
        k_timeout_t recv_to_{K_FOREVER};
        k_thread_stack_t* stack_area_;
        k_thread thread_{};
    };
} // Threads
