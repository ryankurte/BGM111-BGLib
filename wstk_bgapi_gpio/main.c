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
			// VERBOSE PACKET OUTPUT
			printf("<-- Received event:\n\tgecko_evt_system_boot(%d, %d, %d, %d, %d, %d)\n",
				evt->data.evt_system_boot.major,
				evt->data.evt_system_boot.minor,
				evt->data.evt_system_boot.patch,
				evt->data.evt_system_boot.build,
				evt->data.evt_system_boot.bootloader,
				evt->data.evt_system_boot.hw
				);

			// set device name (will appear in active scans and readable GATT characteristic)
			printf("--> Setting device name:\n\tgecko_cmd_gatt_server_write_attribute_value(%d, 0, 16, \"BGM111 GPIO Demo\")\n",
				gattdb_device_name);
			rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_write_attribute_value(gattdb_device_name, 0, 16, "BGM111 GPIO Demo");
			printf("<-- Received response:\n\tgecko_rsp_gatt_server_write_attribute_value(0x%04X)\n",
				rsp->data.rsp_gatt_server_write_attribute_value.result);

			// start advertisements after boot/reset
			printf("--> Starting advertisements:\n\tgecko_cmd_le_gap_set_mode(2, 2)\n");
			rsp = (struct gecko_cmd_packet *)gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
			printf("<-- Received response:\n\tgecko_rsp_gap_set_mode(0x%04X)\n",
				rsp->data.rsp_le_gap_set_mode.result);
			printf("\n--- AWAITING CONNECTION FROM BLE MASTER\n\n");
			break;

		// LE CONNECTION OPENED (remote device connected)
		case gecko_evt_le_connection_opened_id:
			// VERBOSE PACKET OUTPUT
			printf("<-- Received event:\n\tgecko_evt_le_connection_opened(%02X:%02X:%02X:%02X:%02X:%02X, %d, %d, %d, 0x%02X)\n",
				evt->data.evt_le_connection_opened.address.addr[5], // <-- address is little-endian
				evt->data.evt_le_connection_opened.address.addr[4],
				evt->data.evt_le_connection_opened.address.addr[3],
				evt->data.evt_le_connection_opened.address.addr[2],
				evt->data.evt_le_connection_opened.address.addr[1],
				evt->data.evt_le_connection_opened.address.addr[0],
				evt->data.evt_le_connection_opened.address_type,
				evt->data.evt_le_connection_opened.master,
				evt->data.evt_le_connection_opened.connection,
				evt->data.evt_le_connection_opened.bonding
				);

			// update connection state var
			connection_state = 1;
			connection_handle = evt->data.evt_le_connection_opened.connection;

			// start soft timer for GPIO polling
			printf("--> Starting 50ms repeating timer for button status polling\n\thardware_set_soft_timer(205, 0, 0)\n");
			rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_set_soft_timer(205, 0, 0);
			printf("<-- Received response:\n\tgecko_rsp_hardware_set_soft_timer(0x%04X)\n",
				rsp->data.rsp_hardware_set_soft_timer.result);

			break;

		// LE CONNECTION CLOSED (remote device disconnected)
		case gecko_evt_le_connection_closed_id:
			// VERBOSE PACKET OUTPUT
			printf("<-- Received event:\n\tgecko_evt_le_connection_closed(%d, 0x%04X)\n",
				evt->data.evt_le_connection_closed.connection,
				evt->data.evt_le_connection_closed.reason
				);

			// update status vars
			connection_state = 0;
			subscription_state = 0;
			
			// stop soft timer for GPIO polling
			printf("--> Ending 50ms repeating timer for button status polling\n\thardware_set_soft_timer(0, 0, 0)\n");
			rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_set_soft_timer(0, 0, 0);
			printf("<-- Received response:\n\tgecko_rsp_hardware_set_soft_timer(0x%04X)\n",
				rsp->data.rsp_hardware_set_soft_timer.result);
				
			// restart advertisements after disconnection
			printf("--> Restarting advertisements\n\tgecko_cmd_le_gap_set_mode(0x02, 0x02)\n");
			rsp = (struct gecko_cmd_packet *)gecko_cmd_le_gap_set_mode(le_gap_general_discoverable, le_gap_undirected_connectable);
			printf("<-- Received response:\n\tgecko_rsp_gap_set_mode(0x%04X)\n",
				rsp -> data.rsp_le_gap_set_mode.result);
			printf("\n--- AWAITING CONNECTION FROM BLE MASTER\n\n");

			break;

		// GATT SERVER CHARACTERISTIC STATUS (remote GATT client changed subscription status)
		case gecko_evt_gatt_server_characteristic_status_id:
			// VERBOSE PACKET OUTPUT
			printf("<-- Received event:\n\tgecko_evt_gatt_server_characteristic_status(%d, %d, 0x%02X, 0x%04X)\n",
				evt->data.evt_gatt_server_characteristic_status.connection,
				evt->data.evt_gatt_server_characteristic_status.characteristic,
				evt->data.evt_gatt_server_characteristic_status.status_flags,
				evt->data.evt_gatt_server_characteristic_status.client_config_flags
				);

			// make sure this is on the correct characteristic
			if (evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_gpio_control)
			{
				// make sure this update corresponds to a change in configuration status
				if (evt->data.evt_gatt_server_characteristic_status.status_flags == gatt_server_client_config)
				{
					// client characteristic configuration status changed for GPIO control
					if (evt->data.evt_gatt_server_characteristic_status.client_config_flags & gatt_indication)
					{
						printf("\n--- INDICATIONS ENABLED ON GPIO CHARACTERISTIC\n");
						printf("\n--- Button1 press will now push value to client\n\n");

						// update status vars
						subscription_state = 1;

						// read PF7 GPIO state to detect button press status immediately
						//printf("--> Reading PF7 state\n\thardware_read_gpio(5, 0x0080)\n");
						rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_read_gpio(5, 0x0080);

						// TODO: bugfix in BGLib - this response packet doesn't map correctly
						uint8_t *b = (uint8_t *)rsp;
						uint16_t manual_gpio_data = b[2] | (b[3] << 8);

						/*
						printf("<-- Received response:\n\tgecko_rsp_hardware_read_gpio(0x%04X, 0x%04X)\n",
							rsp->data.rsp_hardware_read_gpio.result,
							manual_gpio_data);
							//rsp->data.rsp_hardware_read_gpio.data);
							//*/

						//button_state = (rsp->data.rsp_hardware_read_gpio.data & 0x0080) ? 0 : 1; // logic high = not pressed, logic low = pressed
						button_state = (manual_gpio_data & 0x0080) != 0 ? 0 : 1; // logic high = not pressed, logic low = pressed

						// push current status to client so they have it available
						printf("--> Pushing current PF7 state to client immediately\n\tgecko_cmd_gatt_server_send_characteristic_notification(%d, %d, 1, [ 0x%02X ])\n",
							evt->data.evt_gatt_server_characteristic_status.connection,
							evt->data.evt_gatt_server_characteristic_status.characteristic,
							button_state
							);
						rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_characteristic_notification(
							evt->data.evt_gatt_server_characteristic_status.connection,
							evt->data.evt_gatt_server_characteristic_status.characteristic,
							1,
							&button_state);
						printf("<-- Received response:\n\tgatt_server_send_characteristic_notification(0x%04X)\n",
							rsp->data.rsp_gatt_server_send_characteristic_notification.result);
					}
					else
					{
						// update status vars
						subscription_state = 0;

						printf("\n--- INDICATIONS DISABLED ON GPIO CHARACTERISTIC\n");
						printf("\n--- Button1 press will no longer push value to client\n\n");
					}
				}
			}
			else
			{
				// how did this happen? the GATT structure only has one indication-enabled characteristic
				printf("\n--- STATUS UPDATED ON UNEXPECTED CHARACTERISTIC\n");
				printf("--- (not a problem, just...very strange)\n\n");
			}

			break;

		// GATT SERVER USER READ REQUEST (remote GATT client is reading a value from a user-type characteristic)
		case gecko_evt_gatt_server_user_read_request_id:
			// VERBOSE PACKET OUTPUT
			printf("<-- Received event:\n\tgecko_evt_gatt_server_user_read_request(%d, %d, %d, %d)\n",
				evt->data.evt_gatt_server_user_read_request.connection,
				evt->data.evt_gatt_server_user_read_request.characteristic,
				evt->data.evt_gatt_server_user_read_request.att_opcode,
				evt->data.evt_gatt_server_user_read_request.offset
				);

			// make sure this is on the correct characteristic
			if (evt->data.evt_gatt_server_user_read_request.characteristic == gattdb_gpio_control)
			{
				// read PF7 GPIO state to detect button press status
				//printf("--> Reading PF7 state\n\thardware_read_gpio(5, 0x0080)\n");
				rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_read_gpio(5, 0x0080);

				// TODO: bugfix in BGLib - this response packet doesn't map correctly
				uint8_t *b = (uint8_t *)rsp;
				uint16_t manual_gpio_data = b[2] | (b[3] << 8);

				/*
				printf("<-- Received response:\n\tgecko_rsp_hardware_read_gpio(0x%04X, 0x%04X)\n",
					rsp->data.rsp_hardware_read_gpio.result,
					manual_gpio_data);
					//rsp->data.rsp_hardware_read_gpio.data);
					//*/
				//button_state = (rsp->data.rsp_hardware_read_gpio.data & 0x0080) ? 0 : 1;
				button_state = (manual_gpio_data & 0x0080) != 0 ? 0 : 1;

				// send back "success" response packet with value manually (GATT structure has `type="user"` set)
				printf("--> Sending success response for read request\n\tgecko_cmd_gatt_server_send_user_read_response(%d, %d, 0x00, 1, [ 0x%02X ])\n",
					evt->data.evt_gatt_server_user_read_request.connection,
					evt->data.evt_gatt_server_user_read_request.characteristic,
					button_state);
				rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_read_response(
					evt->data.evt_gatt_server_user_read_request.connection,
					evt->data.evt_gatt_server_user_read_request.characteristic,
					0x00, /* SUCCESS */
					0x01, /* length */
					&button_state);
				printf("<-- Received response:\n\tgecko_rsp_gatt_server_send_user_read_response(0x%04X)\n",
					rsp->data.rsp_gatt_server_send_user_write_response.result);
			}
			else
			{
				// send 0x81 error response for invalid characteristic (shouldn't be able to happen, but let's be safe)

				// send back "error" response packet manually (GATT structure has `type="user"` set)
				printf("--> Sending error response for write operation\n\tgecko_cmd_gatt_server_send_user_write_response(%d, %d, 0x81)\n",
					evt->data.evt_gatt_server_attribute_value.connection,
					evt->data.evt_gatt_server_attribute_value.attribute);
				rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_write_response(
					evt->data.evt_gatt_server_attribute_value.connection,
					evt->data.evt_gatt_server_attribute_value.attribute,
					0x81 /* CUSTOM ERROR (0x80-0xFF are user-defined) */);
				printf("<-- Received response:\n\tgecko_rsp_gatt_server_send_user_write_response(0x%04X)\n",
					rsp->data.rsp_gatt_server_send_user_write_response.result);
			}

			break;

		// GATT SERVER USER WRITE REQUEST (remote GATT client wrote a new value to a user-type characteristic)
		case gecko_evt_gatt_server_user_write_request_id:
			// VERBOSE PACKET OUTPUT
			printf("<-- Received event:\n\tgecko_evt_gatt_server_user_write_request(%d, %d, %d, %d, [ ",
				evt->data.evt_gatt_server_user_write_request.connection,
				evt->data.evt_gatt_server_user_write_request.characteristic,
				evt->data.evt_gatt_server_user_write_request.att_opcode,
				evt->data.evt_gatt_server_user_write_request.offset
				);
			for (int i = 0; i < evt->data.evt_gatt_server_user_write_request.value.len; i++)
				printf("%02X ", evt->data.evt_gatt_server_user_write_request.value.data[i]);
			printf("])\n");

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
						printf("--> Turning on LED0 (PF6 low)\n\tgecko_cmd_hardware_write_gpio(0x05, 0x0040, 0x0000)\n");
						rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_write_gpio(5, 0x40, 0x00);
						printf("<-- Received response:\n\tgecko_rsp_hardware_write_gpio(0x%04X)\n",
							rsp->data.rsp_hardware_write_gpio.result);
					}
					else
					{
						// zero byte written, turn LED0 off
						printf("--> Turning off LED0 (PF6 high)\n\tgecko_cmd_hardware_write_gpio(0x05, 0x0040, 0x0040)\n");
						rsp = (struct gecko_cmd_packet *)gecko_cmd_hardware_write_gpio(5, 0x40, 0x40);
						printf("<-- Received response:\n\tgecko_rsp_hardware_write_gpio(0x%04X)\n",
							rsp->data.rsp_hardware_write_gpio.result);
					}

					// send back "success" response packet manually (GATT structure has `type="user"` set)
					printf("--> Sending success response for write operation\n\tgecko_cmd_gatt_server_send_user_write_response(%d, %d, 0x00)\n",
						evt->data.evt_gatt_server_user_write_request.connection,
						evt->data.evt_gatt_server_user_write_request.characteristic);
					rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_write_response(
						evt->data.evt_gatt_server_user_write_request.connection,
						evt->data.evt_gatt_server_user_write_request.characteristic,
						0x00 /* SUCCESS */);
					printf("<-- Received response:\n\tgecko_rsp_gatt_server_send_user_write_response(0x%04X)\n",
						rsp->data.rsp_gatt_server_send_user_write_response.result);
				}
				else
				{
					// invalid byte string, so don't process change anything with LEDs and send error response 0x80

					// send back "error" response packet manually (GATT structure has `type="user"` set)
					printf("--> Sending error response for write operation\n\tgecko_cmd_gatt_server_send_user_write_response(%d, %d, 0x80)\n",
						evt->data.evt_gatt_server_attribute_value.connection,
						evt->data.evt_gatt_server_attribute_value.attribute);
					rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_write_response(
						evt->data.evt_gatt_server_attribute_value.connection,
						evt->data.evt_gatt_server_attribute_value.attribute,
						0x80 /* CUSTOM ERROR (0x80-0xFF are user-defined) */);
					printf("<-- Received response:\n\tgecko_rsp_gatt_server_send_user_write_response(0x%04X)\n",
						rsp->data.rsp_gatt_server_send_user_write_response.result);
				}
			}
			else
			{
				// send 0x81 error response for invalid characteristic (shouldn't be able to happen, but let's be safe)

				// send back "error" response packet manually (GATT structure has `type="user"` set)
				printf("--> Sending error response for write operation\n\tgecko_cmd_gatt_server_send_user_write_response(%d, %d, 0x81)\n",
					evt->data.evt_gatt_server_attribute_value.connection,
					evt->data.evt_gatt_server_attribute_value.attribute);
				rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_user_write_response(
					evt->data.evt_gatt_server_attribute_value.connection,
					evt->data.evt_gatt_server_attribute_value.attribute,
					0x81 /* CUSTOM ERROR (0x80-0xFF are user-defined) */);
				printf("<-- Received response:\n\tgecko_rsp_gatt_server_send_user_write_response(0x%04X)\n",
					rsp->data.rsp_gatt_server_send_user_write_response.result);
			}

			break;

		case gecko_evt_hardware_soft_timer_id:
			// VERBOSE PACKET OUTPUT
			// NOTE: this is suppresed because it would generate a LOT of useless content
			/*
			printf("<-- Received event:\n\tgecko_evt_hardware_soft_timer(%d)\n",
				evt->data.evt_hardware_soft_timer.handle
				);
			*/

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

				/*
				printf("<-- Received response:\n\tgecko_rsp_hardware_read_gpio(0x%04X, 0x%04X)\n",
					rsp->data.rsp_hardware_read_gpio.result,
					manual_gpio_data);
					//rsp->data.rsp_hardware_read_gpio.data);
					//*/

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
						printf("--> Pushing current PF7 state to client immediately\n\tgecko_cmd_gatt_server_send_characteristic_notification(%d, %d, 1, [ 0x%02X ])\n",
							connection_handle,
							gattdb_gpio_control,
							button_state
							);
						rsp = (struct gecko_cmd_packet *)gecko_cmd_gatt_server_send_characteristic_notification(
							connection_handle,
							gattdb_gpio_control,
							1,
							&button_state);
						printf("<-- Received response:\n\tgatt_server_send_characteristic_notification(0x%04X)\n",
							rsp->data.rsp_hardware_read_gpio.result);
					}
				}
			}

			break;

		}
    }

    return 0;
}
