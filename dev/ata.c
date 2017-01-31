#include "dev/ata.h"
#include "stdint.h"
#include "stdbool.h"
#include "io.h"
#include "idt.h"
#include "cpu.h"
#include "fs/vfs.h"
#include "string.h"
#include "debug.h"

static char ata_current_drive_letter = 'a';

typedef struct ata_device {
	int32_t io_base;
	int32_t control;
	bool is_slave;
	bool is_atapi;
	ata_identify_t ident_result;
} ata_device_t;

static ata_device_t ata_primary_master   = {.io_base = 0x1F0, .control = 0x3F6, .is_slave = false};
static ata_device_t ata_primary_slave    = {.io_base = 0x1F0, .control = 0x3F6, .is_slave = true};
static ata_device_t ata_secondary_master = {.io_base = 0x170, .control = 0x376, .is_slave = false};
static ata_device_t ata_secondary_slave  = {.io_base = 0x170, .control = 0x376, .is_slave = true};

void ata_delay_io(ata_device_t *dev)
{
	// 400nS delay
	inb(dev->io_base + ATA_REG_ALTSTATUS);
	inb(dev->io_base + ATA_REG_ALTSTATUS);
	inb(dev->io_base + ATA_REG_ALTSTATUS);
	inb(dev->io_base + ATA_REG_ALTSTATUS);
}

int32_t ata_delay_status(ata_device_t *dev, int32_t timeout)
{
	int32_t status;
	if (timeout > 0) {
		int32_t i = 0;
		while (((status = inb(dev->io_base + ATA_REG_STATUS)) & ATA_SR_BSY) && (i < timeout)) i++;
	} else {
		while ((status = inb(dev->io_base + ATA_REG_STATUS)) & ATA_SR_BSY);
	}

	return status;
}

// Resets BOTH drives on the bus!
void ata_reset_soft(ata_device_t *dev)
{
	outb(dev->control, 0x04); // Reset
	ata_delay_io(dev);
	outb(dev->control, 0x00); // Clear reset bit
}

void init_ata_device(ata_device_t *dev)
{
	debug("Initializing ATA device: %d\n", dev->io_base);

	outb(dev->io_base + 1, 1);
	outb(dev->control, 0);

	outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | dev->is_slave << 4); // Select drive
	ata_delay_io(dev);

	outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY); // Send IDENTIFY command
	ata_delay_io(dev);

	int32_t status = inb(dev->io_base + ATA_REG_COMMAND);
	debug("ATA device status: 0x%x\n", status);

	ata_delay_io(dev);
	ata_delay_status(dev, -1);

	uint16_t *buffer = (uint16_t *) &dev->ident_result;

	for (int32_t i = 0; i < 256; i++) {
		buffer[i] = ins(dev->io_base);
	}

	uint8_t *idptr = (uint8_t *) &dev->ident_result.model;
	for (int32_t i = 0; i < 39; i+=2) {
		uint8_t tmp = idptr[i+1];
		idptr[i+1] = idptr[i];
		idptr[i] = tmp;
	}

	dev->is_atapi = false;

	debug("Device Name:  %s ", dev->ident_result.model);
	debug("Sectors (48): %d ", (uint32_t)dev->ident_result.sectors_48);
	debug("Sectors (24): %d\n", dev->ident_result.sectors_28);

	outb(dev->io_base + ATA_REG_CONTROL, 0x02);
}

vfs_node_t *create_ata_dev(ata_device_t *dev)
{
	vfs_node_t *node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
	memset(node, 0, sizeof(vfs_node_t));

	node->device = dev;
	node->mask = VFS_MASK_DEVICE;
//	node->read = ata_read;
//	node->write = ata_write;

	return node;
}

void ata_detect(ata_device_t *dev)
{
	ata_reset_soft(dev);
	ata_delay_io(dev);

	outb(dev->io_base + ATA_REG_HDDEVSEL, 0xA0 | dev->is_slave << 4); // Select drive
	ata_delay_io(dev);
	ata_delay_status(dev, 5000);

	uint32_t cyl_low = inb(dev->io_base + ATA_REG_LBA1);
	uint32_t cyl_high = inb(dev->io_base + ATA_REG_LBA2);

	if (cyl_low == 0xFF && cyl_high == 0xFF) {
		return;
	}
	if ((cyl_low == 0x00 && cyl_high == 0x00) || (cyl_low == 0x3C && cyl_high == 0xC3)) {
		/* PATA or emulated SATA */
		char *name = (char *)kmalloc(sizeof(char) * 32);
		name = strdup("/dev/sd");

		name[7] = ata_current_drive_letter;
		name[8] = '\0';

		debug("Creating device %s\n", name);

		vfs_node_t *node = create_ata_dev(dev);
		vfs_mount(name, node);

		kfree(name);

		ata_current_drive_letter++;

		init_ata_device(dev);
	} else if ((cyl_low == 0x14 && cyl_high == 0xEB) || (cyl_low == 0x69 && cyl_high == 0x96)) {
		/* ATAPI */
		debug("Found an ATAPI device. Not supported!\n");
	}
}


static void ata_primary_irq(registers_t regs)
{
}

static void ata_secondary_irq(registers_t regs)
{
}

void ata_init()
{
	register_interrupt_handler(IRQ14, ata_primary_irq);
	register_interrupt_handler(IRQ15, ata_secondary_irq);

	ata_detect(&ata_primary_master);
	ata_detect(&ata_primary_slave);
	ata_detect(&ata_secondary_master);
	ata_detect(&ata_secondary_slave);
}
