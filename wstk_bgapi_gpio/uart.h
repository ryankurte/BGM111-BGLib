/**
 * uart.h
 *
 * Copyright © 2013 Bluegiga Technologies Inc. <support@bluegiga.com>
 * This source code is licensed under Bluegiga Source Code License.
 */

#ifndef UART_H
#define UART_H

/**
 * Open the serial port.
 *
 * @param port Serial port to use.
 * @param baudrate Baud rate to be use.
 * @return 0 on success, -1 on failure.
 */
int uart_open(unsigned char* port, uint32_t baudrate);

/**
 * Close the serial port.
 */
void uart_close();

/**
 * Read data from serial port. The function will block until
 * the desired amount has been read or an error occurs.
 *
 * @param data_length The amount of bytes to read.
 * @param data Buffer used for storing the data.
 * @return The amount of bytes read or -1 on failure.
 */
int uart_rx(uint16_t data_length, uint8_t* data);

/**
 * Write data to serial port. The function will block until
 * the desired amount has been written or an error occurs.
 *
 * @param data_length The amount of bytes to write.
 * @param data Data to write.
 * @return The amount of bytes written or -1 on failure.
 */
int uart_tx(uint16_t data_length, uint8_t* data);

/**
 * Peek uart if there is data to be received
 *
 * @return nonzero if there is data in uart
 */
int uart_peek(void);
#endif /* UART_H */
