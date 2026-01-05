//
// Created by bened on 30/12/2025.
//

#pragma once
#include "ModuleState.hpp"
#include "Core/EventTypes.hpp"

using Core::EventTypes::AppPublisher;
using Core::EventTypes::AppSubscriber;

namespace Modules
{
    class ModuleBase
    {
    protected:
        ModuleBase(AppPublisher& publisher, AppSubscriber& subscriber)
            : publisher_(publisher), subscriber_(subscriber)
        {
        }

        void Initialize()
        {
            state_ = Initialized;
        }

        void Start()
        {
            state_ = Running;
        }

        AppPublisher& publisher_;
        AppSubscriber& subscriber_;

        ModuleState state_ = Uninitialized;
    };
}
