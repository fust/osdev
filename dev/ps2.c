#include "dev/ps2.h"
#include "io.h"
#include "stdint.h"
#include "stdbool.h"
#include "debug.h"
#include "timer.h"
#include "idt.h"
#include "cpu.h"
#include "stdio.h"

void ps2_write(ps2_port_t port, ps2_byte_t data);
ps2_byte_t ps2_read(ps2_port_t port);
bool ps2_wait(uint8_t flag);
void ps2_wait_and_poll(uint8_t flag, ps2_port_t port_to_poll);
void detect_device_type(ps2_dev_t *device);

void port_1_handler(registers_t regs);
void port_2_handler(registers_t regs);

bool has_port_a = 1;
bool has_port_b = 1;
bool has_dev_a = 0;
bool has_dev_b = 0;

ps2_dev_t ps2_devices[2];

void ps2_init()
{
	debug("PS/2: Initializing\n");

	// First we disable the devices
	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_DISABLE_PORT_A);

	// Clear the output buffer
	ps2_wait_and_poll(PS2_STATUS_OUTPUT_BUFFER, PS2_DATA_PORT);

	debug("PS/2: Disabled port A\n");
	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_DISABLE_PORT_B);

	// Clear the output buffer
	ps2_wait_and_poll(PS2_STATUS_OUTPUT_BUFFER, PS2_DATA_PORT);

	debug("PS/2: Disabled port B\n");

	// Clear the output buffer
	ps2_wait_and_poll(PS2_STATUS_OUTPUT_BUFFER, PS2_DATA_PORT);

	// Read the configuration byte
	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_READ_BYTE_0);
	ps2_byte_t config = ps2_read(PS2_DATA_PORT);
	debug("PS/2: Config byte is 0x%x\n", config);

	// Check if we have a second port
	if ((config | 0x20) == 0) {
		has_port_b = 0;
		debug("PS/2: No second port found.\n");
	} else {
		debug("PS/2: Dual-port controller found.\n");
	}

	// Update configuration byte and write
	config &= 0x74;
	debug("PS/2: Config byte is 0x%x\n", config);
	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_WRITE_BYTE_0); // Announce config write
	ps2_write(PS2_DATA_PORT, (ps2_byte_t) config); // Write

	debug("PS/2: Wrote configuration.\n");

	// Test the controller
	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_TEST_CONTROLLER);
	ps2_byte_t controller_result = ps2_read(PS2_DATA_PORT);

	// Check the result, should be 0x55 for success
	if (controller_result != (ps2_byte_t) 0x55) {
		has_port_a = 0;
		has_port_b = 0;

		debug("PS/2: Controller test failed! (0x%x)\n", controller_result);
		return;
	} else {
		debug("PS/2: Controller test OK.\n");
	}

	// Test port A
	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_TEST_PORT_A);
	ps2_byte_t porta_result = ps2_read(PS2_DATA_PORT);

	// Check the result, should be 0x00 for success
	if (porta_result != (ps2_byte_t) 0x00) {
		has_port_a = 0;

		debug("PS/2: Port 1 test failed (0x%x). Disabling.\n", porta_result);
	}

	// If we have a port B
	if (has_port_b) {
		// Test port B
		ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_TEST_PORT_B);
		ps2_byte_t portb_result = ps2_read(PS2_DATA_PORT);

		// Check the result, should be 0x00 for success
		if (portb_result != (ps2_byte_t) 0x00) {
			has_port_b = 0;

			debug("PS/2: Port 2 test failed (0x%x). Disabling.\n", portb_result);
		}
	}

	// Validate we still have working ports
	if (!has_port_a && !has_port_b) {
		debug("PS/2: No working ports found!\n");
		return;
	}
	// Read the configuration byte
	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_READ_BYTE_0);
	ps2_byte_t config2 = ps2_read(PS2_DATA_PORT);
	debug("PS/2: Config byte is 0x%x\n", config2);

	if (has_port_a) {
		debug("PS/2: Enabling port A\n");
		ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_ENABLE_PORT_A); // Enable port A
		config |= 0x1; // Enable port A interrupts
	}

	if (has_port_b) {
		debug("PS/2: Enabling port B\n");
		ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_ENABLE_PORT_B); // Enable port B
		config |= 0x2; // Enable port B interrupts
	}

	debug("PS/2: Write config: 0x%x\n", config);

	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_WRITE_BYTE_0); // Announce config write
	ps2_write(PS2_DATA_PORT, config); // Write

	// Reset (0xFF) the devices. They should respond with 0xAA (self-test passed) and then 0xFA (ACK)
	if (has_port_a) {
		debug("PS/2: Polling port A\n");
		ps2_write(PS2_DATA_PORT, 0xFF);
		ps2_byte_t status1 = ps2_read(PS2_DATA_PORT);
		ps2_byte_t status2 = ps2_read(PS2_DATA_PORT);
		if (status1 == PS2_DEV_RESULT_ACK || status2 == PS2_DEV_RESULT_SELFTEST_OK) {
			has_dev_a = 1;
			ps2_devices[0].port = 1;
			debug("PS/2: Found a device on port A.\n");
			detect_device_type(&ps2_devices[0]);
		} else {
			debug("PS/2: Nothing found on port A. (0x%x, 0x%x)\n", status1, status2);
			ps2_byte_t st = ps2_read(PS2_STATUS_PORT);
			debug("PS/2: Status is 0x%x\n", st);
		}
	}

	if (has_port_b) {
		debug("PS/2: Polling port B\n");
		ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_WRITE_PORT_B_INPUT);
		ps2_write(PS2_DATA_PORT, 0xFF);
		ps2_byte_t status1 = ps2_read(PS2_DATA_PORT);
		ps2_byte_t status2 = ps2_read(PS2_DATA_PORT);
		if (status1 == PS2_DEV_RESULT_ACK || status2 == PS2_DEV_RESULT_SELFTEST_OK) {
			has_dev_b = 1;
			ps2_devices[1].port = 2;
			debug("PS/2: Found a device on port B.\n");
			detect_device_type(&ps2_devices[1]);
		} else {
			debug("PS/2: Nothing found on port B. (0x%x, 0x%x)\n", status1, status2);
			ps2_byte_t st = ps2_read(PS2_STATUS_PORT);
			debug("PS/2: Status is 0x%x\n", st);
		}
	}

	kprintf("PS/2: Enabling interrupts and handlers.\n");
	register_interrupt_handler(IRQ1, &port_1_handler);
	register_interrupt_handler(IRQ12, &port_2_handler);

	// Read the configuration byte
	ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_READ_BYTE_0);
	ps2_byte_t config3 = ps2_read(PS2_DATA_PORT);
	debug("PS/2: Config byte is 0x%x\n", config3);
}

void detect_device_type(ps2_dev_t *device)
{
	if (device->port == 0) {
		return;
	}

	debug("PS/2: Detecting type for device %d\n", device->port);
	ps2_byte_t res1, res2, res3;
	if (device->port == 2) {
		ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_WRITE_PORT_B_INPUT);
	}
	ps2_write(PS2_DATA_PORT, PS2_DEV_COMMAND_DISABLE_SCANNING);
	res1 = ps2_read(PS2_DATA_PORT);

	if (!res1 == PS2_DEV_RESULT_ACK) {
		debug("PS/2: Device type detection failed.\n");
		return;
	}
	res1 = 0;

	if (device->port == 2) {
		ps2_write(PS2_COMMAND_PORT, PS2_COMMAND_WRITE_PORT_B_INPUT);
	}
	ps2_write(PS2_DATA_PORT, PS2_DEV_COMMAND_IDENTIFY);
	res1 = ps2_read(PS2_DATA_PORT);
	res2 = ps2_read(PS2_DATA_PORT);
	res3 = ps2_read(PS2_DATA_PORT);

	if (!res1 == PS2_DEV_RESULT_ACK) {
		debug("PS/2: Device type detection failed.\n");
		return;
	}

	debug("PS/2: IDENTIFY Result: 0x%x 0x%x\n", res2, res3);
}

void ps2_write(ps2_port_t port, ps2_byte_t data)
{
	ps2_wait(PS2_STATUS_INPUT_BUFFER);
	outb((uint16_t)port, (uint16_t)data);
}

/**
 * Reads from the given port.
 * Returns the read data on OK, 0 on failure or timeout.
 */
ps2_byte_t ps2_read(ps2_port_t port)
{
	if (port == PS2_DATA_PORT) {
		if (!ps2_wait(!PS2_STATUS_OUTPUT_BUFFER)) {
			return (ps2_byte_t) 0;
		}
	}
	return (ps2_byte_t)inb((uint16_t)port);
}

/*
 * Waits for data to become available.
 * Returns 1 on data OK, 0 on timeout
 */
bool ps2_wait(uint8_t flag)
{
	uint32_t start = get_timer_ticks();
	while ((inb(PS2_STATUS_PORT) & flag) && (get_timer_ticks() - start < PS2_TIMEOUT));
	if (get_timer_ticks() - start >= PS2_TIMEOUT) {
		debug("\tPS/2: Timeout. Status is 0x%x (0x%x)\n", inb(PS2_STATUS_PORT), inb(PS2_STATUS_PORT) & flag);
		return 0;
	}
//	debug("\tPS/2: Wait is over. Status is 0x%x\n", inb(PS2_STATUS_PORT));
	return 1;
}

void ps2_wait_and_poll(uint8_t flag, ps2_port_t port_to_poll)
{
	uint32_t start = get_timer_ticks();
	while (!(inb(PS2_STATUS_PORT) & flag) && (get_timer_ticks() - start < PS2_TIMEOUT)) {
		inb(port_to_poll);
	}
}

// Interrupt handlers
void port_1_handler(registers_t regs)
{
//	kprintf("PS/2: Port 1 interrupt\n");
	if (ps2_devices[0].handler) {
		ps2_devices[0].handler(regs);
	}
}

void port_2_handler(registers_t regs)
{
//	kprintf("PS/2: Port 2 interrupt\n");
	if (ps2_devices[1].handler) {
		ps2_devices[1].handler(regs);
	}
}

void ps2_register_device_driver(ps2_dev_t device, uint8_t port, void *init(ps2_dev_t))
{
	if (ps2_devices[port].handler != 0) {
		return;
	}

	ps2_devices[port].handler = device.handler;
	ps2_devices[port].read = &ps2_read;
	ps2_devices[port].write = &ps2_write;

	if (init) {
		init(ps2_devices[port]);
	}
}
