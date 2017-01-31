#ifndef __PS2_H
#define __PS2_H

#include "stdint.h"
#include "stdbool.h"
#include "idt.h"

#define PS2_TIMEOUT 50

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

/**
 * Command bytes definitions
 *
 * @see http://wiki.osdev.org/%228042%22_PS/2_Controller#Command_Register
 */
#define PS2_COMMAND_READ_BYTE_0 			0x20
#define PS2_COMMAND_WRITE_BYTE_0			0x60
#define PS2_COMMAND_DISABLE_PORT_B 			0xA7
#define PS2_COMMAND_ENABLE_PORT_B 			0xA8
#define PS2_COMMAND_TEST_PORT_B 			0xA9
#define PS2_COMMAND_TEST_CONTROLLER 		0xAA
#define PS2_COMMAND_TEST_PORT_A 			0xAB
#define PS2_COMMAND_DISABLE_PORT_A 			0xAD
#define PS2_COMMAND_ENABLE_PORT_A 			0xAE
#define PS2_COMMAND_READ_CONTROLLER_OUTPUT 	0xD0
#define PS2_COMMAND_WRITE_CONTROLLER_OUTPUT 0xD1
#define PS2_COMMAND_WRITE_PORT_A_OUTPUT 	0xD2
#define PS2_COMMAND_WRITE_PORT_B_OUTPUT 	0xD3
#define PS2_COMMAND_WRITE_PORT_B_INPUT 		0xD4

#define PS2_STATUS_OUTPUT_BUFFER			1
#define PS2_STATUS_INPUT_BUFFER				2
#define PS2_STATUS_SYSTEM_FLAG				4
#define PS2_STATUS_COMMAND_DATA				8
#define PS2_STATUS_TIMEOUT_ERROR			64
#define PS2_STATUS_PARITY_ERROR				128

#define PS2_DEV_COMMAND_IDENTIFY			0xF2
#define PS2_DEV_COMMAND_DISABLE_SCANNING	0xF5
#define PS2_DEV_COMMAND_RESET				0xFF

#define PS2_DEV_RESULT_SELFTEST_OK			0xAA
#define PS2_DEV_RESULT_ACK					0xFA

typedef uint16_t ps2_byte_t;
typedef ps2_byte_t ps2_command_t;
typedef ps2_byte_t ps2_port_t;

typedef void (*ps2_write_t)(ps2_port_t, ps2_byte_t);
typedef ps2_byte_t (*ps2_read_t)(ps2_port_t);

/**
 * PS/2 status register byte
 *
 * @see http://wiki.osdev.org/%228042%22_PS/2_Controller#Status_Register
 */
typedef struct ps2_status_register {
	uint32_t output_buffer_status : 1;
	uint32_t input_buffer_status : 1;
	uint32_t system_flag : 1;
	uint32_t command_data : 1;
	uint32_t unused : 2;
	uint32_t timeout_error : 1;
	uint32_t parity_error : 1;
} ps2_status_register_t;


/**
 * PS/2 controller configuration byte
 *
 * @see http://wiki.osdev.org/%228042%22_PS/2_Controller#PS.2F2_Controller_Configuration_Byte
 */
typedef struct ps2_config_byte {
	uint32_t porta_interrupt : 1;
	uint32_t portb_interrupt : 1;
	uint32_t system_flag : 1;
	uint32_t zero1 : 1;
	uint32_t porta_clock : 1;
	uint32_t portb_clock : 1;
	uint32_t porta_translation : 1;
	uint32_t zero2 : 1;
} ps2_config_byte_t;

typedef struct ps2_dev {
	uint32_t type; // @see http://wiki.osdev.org/%228042%22_PS/2_Controller#Detecting_PS.2F2_Device_Types
	uint8_t port; // Either 1 for port A or 2 for port B
	isr_t handler; // The method that should handle interrupts for this device
	ps2_write_t write;
	ps2_read_t read;
} ps2_dev_t;

void ps2_init();
void ps2_register_device_driver(ps2_dev_t device, uint8_t port, void *init(ps2_dev_t));

#endif
