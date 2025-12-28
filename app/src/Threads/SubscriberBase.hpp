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
      threadWorker.Start([this](const zbus_observer *sub){Poll(sub);}, prio);
    }

  protected:
    SubscriberBase(const zbus_channel *message_bus, ThreadWorker &threadWorker)
      : message_bus(message_bus), threadWorker(threadWorker)
    {
    }

    virtual void Notify(TEvent &event) = 0;

    const zbus_channel *message_bus;

  private:
    void Poll(const zbus_observer* subscriber)
    {
      const zbus_channel *chan;
      TEvent event;

      while (!zbus_sub_wait(subscriber, &chan, K_FOREVER))
      {
        if (message_bus != chan)
        {
           continue;
        }
        zbus_chan_read(chan, &event, K_MSEC(250));
        Notify(event);
      }
    }

    bool running = false;
    int prio = 0;

    ThreadWorker &threadWorker;
  };
};
