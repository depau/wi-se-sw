//
// Created by hetii on 8/26/25.
//

#ifndef WI_SE_SW_UART_H
#define WI_SE_SW_UART_H

#define UART0    0
#define UART1    1

// Options for `config` argument of uart_init
#define UART_NB_BIT_MASK      0B00001100
#define UART_NB_BIT_5         0B00000000
#define UART_NB_BIT_6         0B00000100
#define UART_NB_BIT_7         0B00001000
#define UART_NB_BIT_8         0B00001100

#define UART_PARITY_MASK      0B00000011
#define UART_PARITY_NONE      0B00000000
#define UART_PARITY_EVEN      0B00000010
#define UART_PARITY_ODD       0B00000011

#define UART_NB_STOP_BIT_MASK 0B00110000
#define UART_NB_STOP_BIT_0    0B00000000
#define UART_NB_STOP_BIT_1    0B00010000
#define UART_NB_STOP_BIT_15   0B00100000
#define UART_NB_STOP_BIT_2    0B00110000

#endif // WI_SE_SW_UART_H
