#ifndef DEFINES_H_
#define DEFINES_H_

#define SYSTEM_CORE_CLOCK 48000000
#define APB_CLOCK SYSTEM_CORE_CLOCK

#define FUNCONF_UART_PRINTF_BAUD 31250
#define UART_PINS_TXPD6_RXPD5
#define UART_MODE_RX
#define CH32V003_UART_IMPLEMENTATION

#define CH32V003_SPI_SPEED_HZ 24000000
#define CH32V003_SPI_DIRECTION_2LINE_TXRX
#define CH32V003_SPI_CLK_MODE_POL0_PHA0
#define CH32V003_SPI_NSS_SOFTWARE_ANY_MANUAL
#define CH32V003_SPI_IMPLEMENTATION

#endif
