//
// Created by bened on 10/10/2025.
//
#pragma once


namespace Threads {
    // ---------------- Base worker (lifecycle) -----------------------------------
    class IWorker {

    public:
        virtual ~IWorker() = default;
        virtual void start(int prio, const char* name) = 0;
        virtual void stop() = 0;
        bool running() const { return running_; }

    protected:
        void set_running(const bool v) { running_ = v; }

    private:
        bool running_{false};
    };
} // Threads
