//
// Created by bened on 18/10/2025.
//

#pragma once
#include <functional>
#include <zephyr/kernel.h>

#include "Utils/Logger.hpp"

namespace Utils
{
    class ThreadWorker final
    {
    public:
        using Worker = std::function<void()>;

        explicit ThreadWorker(k_thread_stack_t& stack, size_t stack_size)
            : stack_(stack), stack_size_(stack_size)
        {
        }

        ~ThreadWorker() = default;

        void AttachLogger(Logger& logger)
        {
            logger_ = &logger;
        }

        void Start(const Worker& worker, const int prio)
        {
            if (thread_tid_ != nullptr)
            {
                if (logger_)
                {
                    logger_->warning("Thread worker already started.");
                }
                return;
            }

            worker_ = worker;
            if (logger_)
            {
                logger_->info("Starting worker thread (prio=%d, stack=%d bytes).",
                              prio, static_cast<int>(stack_size_));
            }
            thread_tid_ = k_thread_create(&thread_data_, &stack_,
                                          stack_size_, thread_entry_point,
                                          this, nullptr, nullptr, prio, 0, K_NO_WAIT);
            if (thread_tid_ == nullptr && logger_)
            {
                logger_->error("Failed to create worker thread.");
            }
        }

    private:
        k_thread_stack_t& stack_;
        size_t stack_size_;
        Worker worker_;
        k_thread thread_data_{};
        k_tid_t thread_tid_{};
        Logger* logger_{nullptr};

        static void thread_entry_point(void* arg1, void*, void*)
        {
            auto self = static_cast<ThreadWorker*>(arg1);
            if (!self || !self->worker_)
            {
                return;
            }
            if (self->logger_)
            {
                self->logger_->info("Worker thread entry started.");
            }
            self->worker_();
        }
    };
} // namespace Utils
