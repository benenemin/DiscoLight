//
// Created by bened on 18/10/2025.
//

#pragma once
#include <functional>

namespace Utils
{
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
}
