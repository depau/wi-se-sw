//
// Created by depau on 1/27/21.
//

#ifndef WI_SE_SW_DEBUG_H
#define WI_SE_SW_DEBUG_H

#include "config.h"

#if ENABLE_DEBUG == 1
#define debugf(...) UART_DEBUG.printf(__VA_ARGS__)
#else
#define debugf(...) do {} while(0)
#endif

#if ENABLE_BENCHMARK == 1
#define BENCH if (1)
#else
#define BENCH if (0)
#endif

#endif //WI_SE_SW_DEBUG_H
