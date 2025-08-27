//
// Created by hetii on 8/26/25.
//

#ifndef WI_SE_SW_WISE_XSCHEDULE_H
#define WI_SE_SW_WISE_XSCHEDULE_H

#ifdef ESP8266
    #include "Schedule.h"
#else

#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void schedule_function(std::function<void()> fn) {
    xTaskCreatePinnedToCore(
        [](void *param) {
            auto *f = static_cast<std::function<void()>*>(param);
            (*f)();
            delete f;
            vTaskDelete(NULL);
        },
        "sched_fn",
        4096,
        new std::function<void()>(fn),
        1,
        NULL,
        APP_CPU_NUM
    );
}

#endif

#endif // WI_SE_SW_WISE_XSCHEDULE_H
