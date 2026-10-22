#pragma once
#if defined(MESHIT_NRF_T114)
#define NRF_BOARD_NAME "Heltec T114"
#define MESHIT_BOARD_POWER 21
#define NRF_LED 35
#define NRF_SCK 19
#define NRF_MISO 23
#define NRF_MOSI 22
#define NRF_CS 24
#define NRF_IRQ 20
#define NRF_BUSY 17
#define NRF_RESET 25
#define NRF_UART_RX 37
#define NRF_UART_TX 39
#else
#define NRF_BOARD_NAME "RAK4631"
#define MESHIT_BOARD_POWER 37
#define NRF_LED 35
#define NRF_SCK 43
#define NRF_MISO 45
#define NRF_MOSI 44
#define NRF_CS 42
#define NRF_IRQ 47
#define NRF_BUSY 46
#define NRF_RESET 38
// Unused UART pins; no GPS initialization or traffic.
#define NRF_UART_RX 15
#define NRF_UART_TX 16

#endif
