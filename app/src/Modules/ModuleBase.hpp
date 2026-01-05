//
// Created by bened on 30/12/2025.
//

#pragma once
namespace Modules
{
    class ModuleBase
    {
        public:
        IModule();
        virtual ~IModule();

        virtual void Initialize();
    };
};
