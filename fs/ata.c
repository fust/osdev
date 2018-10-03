#include "fs/ata.h"
#include <stdint.h>
#include "pci/pci.h"

#include "debug.h"

pci_dev_t *atadev;

uint8_t ata_pci_hit(pci_dev_t * dev, void * extra)
{
	if (dev->vid == 0x8086 && (dev->pid == 0x7010 || dev->pid == 0x7111)) {
		*((uint32_t*)extra) = dev;
		return 1;
	}
	return 0;
}

void ata_pci_native_init()
{
	pci_bar_mbar_t * bar0 = pci_read_bar(atadev->bus, atadev->dev, atadev->func, 0);
	if (NULL == bar0) {
		debug("[ATA]: No BAR0!\n");
		return;
	}
	debug("[ATA]: BAR0: 0x%x\n", bar0->address);
}

void ata_init()
{
	debug("[ATA]: Start scanning the PCI bus for a valid ATA device.\n");
	pci_search(&ata_pci_hit, ATA_PCIDEV_BASECLASS, ATA_PCIDEV_SUBCLASS, &atadev);

	kprintf("[ATA]: atadev is set to 0x%x\n", atadev);

	if (!atadev) {
		debug("[ATA]: Detection failed. Driver is aborting.\n");
		return;
	}

	// Check PCI mode compatibility
	if (atadev->progif & 0x80) {
		// ISA mode with busmastering, cannot enable PCI native mode
		debug("[ATA]: Running in ISA mode with busmastering, no PCI native mode availabel.\n");
	}
	if (atadev->progif & 0x8A) {
		// ISA mode with busmastering, enable PCI native mode
		debug("[ATA]: Trying to enable PCI native mode.\n");
		atadev->progif &= 0x5;
		pci_config_write(atadev->bus, atadev->dev, atadev->func, PCI_PROGIF, atadev->progif, 1);
		atadev->progif = pci_config_read(atadev->bus, atadev->dev, atadev->func, PCI_PROGIF, 1);
		if (!atadev->progif & 0x8F) {
			debug("[ATA]: Could not enable PCI native mode. Aborting.\n");
			return;
		} else {
			debug("[ATA]: PCI native mode enabled.\n");
			ata_pci_native_init();
		}
	} else if (atadev->progif & 0x85 || atadev->progif & 0x8F) {
		debug("[ATA]: PCI native mode already enabled, initializing bus mastering.\n");
		ata_pci_native_init();
		// PCI native mode with busmastering
	} else {
		debug("[ATA]: Error: No bus mastering supported. Aborting.\n");
		return;
	}
}
