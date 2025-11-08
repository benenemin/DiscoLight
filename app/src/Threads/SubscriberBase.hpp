//
// Created by bened on 18/10/2025.
//
#pragma once

#include "ThreadWorker.hpp"

using namespace Core::EventTypes;
using namespace Core;

namespace Threads
{
  template<typename TEvent>
  class SubscriberBase
  {
    static_assert(std::is_base_of_v<BaseEvent, TEvent>, "TEvent must derive from BaseEvent.");

  public:
    virtual ~SubscriberBase() = default;

    virtual void Initialize(int prio)
    {
      this->prio = prio;
    }

    // Start the subscriber thread with chosen priority and optional name
    virtual void Start()
    {
      if (running) return;
      running = true;
      threadWorker.Start([this]{Poll();}, prio);
    }

  protected:
    SubscriberBase(MessageBus &message_bus, ThreadWorker &threadWorker)
      : message_bus(message_bus), threadWorker(threadWorker)
    {
    }

    virtual void Notify(TEvent &event) = 0;

    MessageBus &message_bus;

  private:
    void Poll()
    {
      while (running) {
        TEvent event{};
        if (message_bus.Subscribe(event, K_NO_WAIT))
        {
          Notify(event);
        }
        else
        {
          k_sleep(K_USEC(100)); // back off a bit when no data
        }
      }
    }

    bool running = false;
    int prio = 0;

    ThreadWorker &threadWorker;
  };
};
