/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "App/Application.hpp"

LOG_MODULE_REGISTER(App, CONFIG_APP_LOG_LEVEL);

int main()
{
    LOG_INF("App started.");

    static App::Application app;
    LOG_INF("Initializing application...");
    app.Initialize();
    LOG_INF("Starting application...");
    app.Start();

    LOG_INF("Application running...");
    k_sleep(K_FOREVER);
    return 0;
}
