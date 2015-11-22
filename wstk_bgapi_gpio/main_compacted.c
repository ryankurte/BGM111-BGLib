/**
 * wstk_bgapi_gpio project - main.c
 *
 * Copyright © 2015 Bluegiga Technologies Inc. http://www.bluegiga.com/support
 * This source code is licensed under Bluegiga Source Code License.
 */

/**
 * This an example application that demonstrates Bluetooth Smart peripheral
 * connectivity using BGLib C function definitions. It resets the module, then
 * starts advertising as a connectable peripheral. When a connection is opened,
 * a 50ms repeating soft timer is started to begin polling the logic state of
 * the PF7 pin, which is connected in an active-low configuration to the "PB1"
 * pushbutton on the main WSTK board. The timer is stopped once the connection
 * is closed, at which point connectable advertisements are resumed.
 *
 * This demo uses an advertised device name of "BGM111 GPIO Demo", so you
 * should look for this in any BLE scanning apps or software tools that you use
 * for testing.
 *
 * Most of the functionality in BGAPI uses a request-response-event pattern
 * where the module responds to each command with a response, indicating that
 * it has processed the command. Events which occur asynchonously or with non-
 * predictable timing may be sent from the module to the host at any time. For
 * more information, please see the WSTK BGAPI GPIO Demo Application Note.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "../include/gecko_bglib.h"
#include "uart.h"

/**
 * Named attribute handles from "gatt_db.h" from bgbuild.exe firmware compile process
 */
#define gattdb_device_name                      3
#define gattdb_gpio_control                    11

BGLIB_DEFINE();

/**
 * Configurable parameters that can be modified to match the test setup.
 */

/** The default serial port to use for BGAPI communication. */
uint8_t* default_uart_port = "COM1";

/** The default baud rate to use. */
uint32_t default_baud_rate = 115200;

/** The serial port to use for BGAPI communication. */
static uint8_t* uart_port = NULL;

/** The baud rate to use. */
static uint32_t baud_rate = 0;

/** Usage string */
#define USAGE "Usage: %s [serial_port [baud_rate]]\n\n"

/** Connection state variable. (0=disconnected, 1=connected) */
uint8_t connection_state = 0;

/** Connection handle variable. */
uint8_t connection_handle = 0;

/** Subscription state variable. (0=not subscribed to indications, 1=subscribed to indications) */
uint8_t subscription_state = 0;

/** Button press state variable. (0=not pressed, 1=pressed) */
uint8_t button_state = 0;

/**
 * Function called when a message needs to be written to the serial port.
 * @param msg_len Length of the message.
 * @param msg_data Message data, including the header.
 * @param data_len Optional variable data length.
 * @param data Optional variable data.
 */
static void on_message_send(uint16 msg_len, uint8* msg_data)
{
    /** Variable for storing function return values. */
    int ret;

#ifdef _DEBUG
	printf("on_message_send()\n");
#endif /* DEBUG */

    ret = uart_tx(msg_len, msg_data);
    if (ret < 0)
    {
        printf("on_message_send() - failed to write to serial port %s, ret: %d, errno: %d\n", uart_port, ret, errno);
        exit(EXIT_FAILURE);
    }
}

void print_address(bd_addr address)
{
    for (int i = 5; i >= 0; i--)
    {
        printf("%02x", address.addr[i]);
        if (i > 0)
            printf(":");
    }
    
}

int hw_init(int argc, char* argv[])
{
    char tmpbaud[10];

    /**
    * Handle the command-line arguments.
    */
    baud_rate = default_baud_rate;
    
	switch (argc)
    {
    case 3:
        baud_rate = atoi(argv[2]);
        /** Falls through on purpose. */
    case 2:
        uart_port = argv[1];
        /** Falls through on purpose. */
    default:
        break;
    }

    if (!uart_port)
    { /*no uart port given, ask from user*/
        printf("Serial port to use (e.g. COM11): ");
        if (scanf_s("%s", tmpbaud, sizeof(tmpbaud)) == 1)
        {
            uart_port = tmpbaud;
        }
    }
    if (!uart_port || !baud_rate)
    {
        printf(USAGE, argv[0]);
        exit(EXIT_FAILURE);
    }

    /**
    * Initialise the serial port.
    */
    return uart_open(uart_port, baud_rate);
}

/**
 * The main program.
 */
int main(int argc, char* argv[])
{
    struct gecko_cmd_packet *evt, *rsp;
    
	/**
    * Initialize BGLIB with our output function for sending messages.
    */

    BGLIB_INITIALIZE(on_message_send, uart_rx);
    
    if (hw_init(argc, argv) < 0)
    {
        printf("Hardware initialization failure, check serial port and baud rate values\n");
        exit(EXIT_FAILURE);
    }

	/**
	* Display welcome message.
	*/
	printf("\nBlue Gecko WSTK GPIO BLE Peripheral Application\n");
	printf("-----------------------------------------------\n\n");

	// trigger reset manually with an API command instead
	printf("--> Resetting device\n\tgecko_cmd_system_reset(0)\n");
	rsp = (struct gecko_cmd_packet *)gecko_cmd_system_reset(0);
	printf("\n--- No reponse expected, should go directly to 'system_boot' event\n");
	printf("--- If this does not occur, please reset the module to trigger it\n\n");

	// ========================================================================================
    // This infinite loop is similar to the BGScript interpreter environment, and each "case"
	// represents one of the event handlers that you would define using BGScript code. You can
	// restructure this code to use your own function calls instead of placing all of the event
	// handler logic right inside each "case" block.
	// ========================================================================================
	while (1)
    {
		// blocking wait for event API packet
		evt = gecko_wait_event();

		// if a non-blocking implementation is needed, use gecko_peek_event() instead
		// (will return NULL instead of packet struct pointer if no event is ready)

        switch (BGLIB_MSG_ID(evt -> header))
        {
		// SYSTEM BOOT (power-on/reset)
        case gecko_evt_system_boot_id:
			// set device name (will appear in active scans and readable GATT characteristic)
			rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_write_attribute_value(gattdb_device_name, 0, 16, "BGM111 GPIO Demo");

			// start advertisements after boot/reset
			rsp = (struct gecko_cmd_packet *)gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
            
			break;

		// LE CONNECTION OPENED (remote device connected)
		case gecko_evt_le_connection_opened_id:
			// update connection state var
			connection_state = 1;
			connection_handle = evt->data.evt_le_connection_opened.connection;

			// start soft timer for GPIO polling
			rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_set_soft_timer(205, 0, 0);

			break;

		// LE CONNECTION CLOSED (remote device disconnected)
		case gecko_evt_le_connection_closed_id:
			// update status vars
			connection_state = 0;
			subscription_state = 0;
			
			// stop soft timer for GPIO polling
			rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_set_soft_timer(0, 0, 0);
				
			// restart advertisements after disconnection
			rsp = (struct gecko_cmd_packet *)gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);

			break;

        // GATT SERVER CHARACTERISTIC STATUS (remote GATT client changed subscription status)
		case gecko_evt_gatt_server_characteristic_status_id:
			// make sure this is on the correct characteristic
			if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_gpio_control)
			{
				// make sure this update corresponds to a change in configuration status
				if (evt->data.evt_gatt_server_characteristic_status.status_flags == gatt_server_client_config)
				{
					// client characteristic configuration status changed for GPIO control
					if (evt->data.evt_gatt_server_characteristic_status.client_config_flags & gatt_indication)
					{
						// update status vars
						subscription_state = 1;

						// read PF7 GPIO state to detect button press status immediately
						//printf("--> Reading PF7 state\n\thardware_read_gpio(5, 0x0080)\n");
						rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_read_gpio(5, 0x0080);

						// TODO: bugfix in BGLib - this response packet doesn't map correctly
						uint8_t *b = (uint8_t *)rsp;
						uint16_t manual_gpio_data = b[2] | (b[3] << 8);

						//button_state = (rsp->data.rsp_hardware_read_gpio.data & 0x0080) ? 0 : 1; // logic high = not pressed, logic low = pressed
						button_state = (manual_gpio_data & 0x0080) != 0 ? 0 : 1; // logic high = not pressed, logic low = pressed

						// push current status to client so they have it available
						rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_characteristic_notification(
							evt->data.evt_gatt_server_characteristic_status.connection,
							evt->data.evt_gatt_server_characteristic_status.characteristic,
							1,
							&button_state);
					}
					else
					{
						// update status vars
						subscription_state = 0;
					}
				}
			}

			break;

		// GATT SERVER USER READ REQUEST (remote GATT client is reading a value from a user-type characteristic)
		case gecko_evt_gatt_server_user_read_request_id:
			// make sure this is on the correct characteristic
			if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_gpio_control)
			{
				// read PF7 GPIO state to detect button press status
				rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_read_gpio(5, 0x0080);

				// TODO: bugfix in BGLib - this response packet doesn't map correctly
				uint8_t *b = (uint8_t *)rsp;
				uint16_t manual_gpio_data = b[2] | (b[3] << 8);

				//button_state = (rsp->data.rsp_hardware_read_gpio.data & 0x0080) ? 0 : 1;
				button_state = (manual_gpio_data & 0x0080) != 0 ? 0 : 1;

				// send back "success" response packet with value manually (GATT structure has `type="user"` set)
				rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_read_response(
					evt->data.evt_gatt_server_user_read_request.connection,
					evt->data.evt_gatt_server_user_read_request.characteristic,
					0x00, /* SUCCESS */
					0x01, /* length */
					&button_state);
			}
			else
			{
				// send 0x81 error response for invalid characteristic (shouldn't be able to happen, but let's be safe)

				// send back "error" response packet manually (GATT structure has `type="user"` set)
				rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_write_response(
					evt->data.evt_gatt_server_attribute_value.connection,
					evt->data.evt_gatt_server_attribute_value.attribute,
					0x81 /* CUSTOM ERROR (0x80-0xFF are user-defined) */);
			}

			break;

		// GATT SERVER USER WRITE REQUEST (remote GATT client wrote a new value to a user-type characteristic)
		case gecko_evt_gatt_server_user_write_request_id:
			// make sure this is on the correct characteristic
			if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_gpio_control)
			{
				// set LED0 (PF6) off/on based on zero/non-zero status of written byte
				if (evt->data.evt_gatt_server_user_write_request.value.len == 1)
				{
					// valid byte string, so process it
					if (evt->data.evt_gatt_server_user_write_request.value.data[0])
					{
						// non-zero byte written, turn LED0 on
						rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_write_gpio(5, 0x40, 0x00);
					}
					else
					{
						// zero byte written, turn LED0 off
						rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_write_gpio(5, 0x40, 0x40);
					}

					// send back "success" response packet manually (GATT structure has `type="user"` set)
					rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_write_response(
						evt->data.evt_gatt_server_user_write_request.connection,
						evt->data.evt_gatt_server_user_write_request.characteristic,
						0x00 /* SUCCESS */);
				}
				else
				{
					// invalid byte string, so don't process change anything with LEDs and send error response 0x80

					// send back "error" response packet manually (GATT structure has `type="user"` set)
					rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_write_response(
						evt->data.evt_gatt_server_attribute_value.connection,
						evt->data.evt_gatt_server_attribute_value.attribute,
						0x80 /* CUSTOM ERROR (0x80-0xFF are user-defined) */);
				}
			}
			else
			{
				// send 0x81 error response for invalid characteristic (shouldn't be able to happen, but let's be safe)

				// send back "error" response packet manually (GATT structure has `type="user"` set)
				rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_write_response(
					evt->data.evt_gatt_server_attribute_value.connection,
					evt->data.evt_gatt_server_attribute_value.attribute,
					0x81 /* CUSTOM ERROR (0x80-0xFF are user-defined) */);
			}

			break;

		case gecko_evt_hardware_soft_timer_id:
			// skip all of this if we're not connected but there are any residual timer
			// events still occurring (may be some API processing gap here)
			if (!connection_state)
				break;

			// make sure this is the correct timer (we scheduled handle 0 above)
			if (evt->data.evt_hardware_soft_timer.handle == 0)
			{
				// read PF7 GPIO state to detect button press status
				//printf("--> Reading PF7 state\n\thardware_read_gpio(5, 0x0080)\n");
				rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_read_gpio(5, 0x0080);

				// TODO: bugfix in BGLib - this response packet doesn't map correctly
				uint8_t *b = (uint8_t *)rsp;
				uint16_t manual_gpio_data = b[2] | (b[3] << 8);

				// compare with last known status
				//if (button_state != ((rsp->data.rsp_hardware_read_gpio.data & 0x0080) ? 0 : 1))
				if (button_state != ((manual_gpio_data & 0x0080) != 0 ? 0 : 1))
				{
					// update button_state
					//button_state = (rsp->data.rsp_hardware_read_gpio.data & 0x0080) ? 0 : 1; // logic high = not pressed, logic low = pressed
					button_state = (manual_gpio_data & 0x0080) != 0 ? 0 : 1; // logic high = not pressed, logic low = pressed

					// check to see if the client is subscribed to indications
					if (subscription_state)
					{
						// push updated status to client
						rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_characteristic_notification(
							connection_handle,
							gattdb_gpio_control,
							1,
							&button_state);
					}
				}
			}

			break;

		}
    }

    return 0;
}
