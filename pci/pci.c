#include "pci/pci.h"
#include "pci/pci_list.h"
#include "mem/kmalloc.h"
#include "string.h"
#include "debug.h"
#include "io.h"

uint32_t pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t len)
{
	uint32_t address = (uint32_t) (((uint32_t) bus << 16) | ((uint32_t) dev << 11) | ((uint32_t) func << 8)
			| (offset & 0xFC) | ((uint32_t) 0x80000000));

	outl(PCI_PORT_ADDRESS, address);

	switch (len) {
		case 4:
			return inl(PCI_PORT_DATA);
		case 2:
			return ins(PCI_PORT_DATA + (offset & 2));
		case 1:
			return inb(PCI_PORT_DATA + (offset & 3));
	}

	return 0;
}

void pci_config_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t len, uint32_t data)
{
	uint32_t address = (uint32_t) (((uint32_t) bus << 16) | ((uint32_t) dev << 11) | ((uint32_t) func << 8)
				| (offset & 0xFC) | ((uint32_t) 0x80000000));

	switch (len) {
		case 4:
			outl(PCI_PORT_DATA, data);
			break;
		case 2:
			outs(PCI_PORT_DATA, data);
			break;
		case 1:
			outb(PCI_PORT_DATA, data);
			break;
	}

	outl(PCI_PORT_DATA, data);
}

uint16_t pci_read_vid(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint16_t) pci_config_read(bus, dev, func, PCI_VID, 2);
}

uint16_t pci_read_pid(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint16_t) pci_config_read(bus, dev, func, PCI_PID, 2);
}

uint8_t pci_read_headertype(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint8_t) pci_config_read(bus, dev, func, PCI_HEADERTYPE, 1);
}

uint8_t pci_read_base_class(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint8_t) pci_config_read(bus, dev, func, PCI_BASECLASS, 1);
}

uint8_t pci_read_sub_class(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint8_t) pci_config_read(bus, dev, func, PCI_SUBCLASS, 1);
}

uint8_t pci_read_progif(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint8_t) pci_config_read(bus, dev, func, PCI_PROGIF, 1);
}

uint8_t pci_read_secondary_bus(uint8_t bus, uint8_t dev, uint8_t func)
{
	return (uint8_t) pci_config_read(bus, dev, func, PCI_SECONDARY_BUS, 1);
}

pci_bar_t * pci_read_bar(uint8_t bus, uint8_t dev, uint8_t func, uint8_t barnum)
{
	switch (barnum) {
		case 0:
			barnum = PCI_BAR0;
			break;
		case 1:
			barnum = PCI_BAR1;
			break;
		case 2:
			barnum = PCI_BAR2;
			break;
		case 3:
			barnum = PCI_BAR3;
			break;
		case 4:
			barnum = PCI_BAR4;
			break;
		case 5:
			barnum = PCI_BAR5;
			break;
	}
debug("[PCI]: Reading %d::%d::%d offset 0x%x: ", bus, dev, func, barnum);
	uint32_t b = (uint32_t) pci_config_read(bus, dev, func, barnum, 4);
debug("0x%x \n", b);

	if (b == 0x0) {
		return 0;
	}

	if (!b) {
		return NULL;
	}

	pci_bar_mbar_t * bar;
	if (b & 0x01) {
		debug("[PCI]: Allocating I/O BAR\n");
		// I/O BAR
		bar = (pci_bar_iobar_t *) kmalloc(sizeof(pci_bar_iobar_t));
		memcpy(bar, b, sizeof(pci_bar_iobar_t));
	} else {
		debug("[PCI]: Allocating Memory-mapping BAR\n");
		// Memory-mapping BAR
		bar = (pci_bar_iobar_t *) kmalloc(sizeof(pci_bar_mbar_t));
		memcpy(bar, b, sizeof(pci_bar_mbar_t));
	}

	return (pci_bar_t *) bar;
}

void pci_search_bus(pci_matcher_t matcher, uint8_t baseClass, uint8_t subClass, void * extra, uint8_t bus);

void pci_search_hit(pci_matcher_t matcher, uint8_t baseClass, uint8_t subClass, void * extra, pci_dev_t *pcidev)
{
	pcidev->baseclass = baseClass;
	pcidev->subclass = subClass;

	uint8_t result = matcher(pcidev, extra);

	if (result) {
		kprintf("[PCI] Driver accepted match 0x%x:0x%x.\n", pcidev->vid, pcidev->pid);
	} else {
		kprintf("[PCI] Driver discarded match 0x%x:0x%x.\n", pcidev->vid, pcidev->pid);
	}
}

void pci_search_function(pci_matcher_t matcher, uint8_t baseClass, uint8_t subClass, void * extra, uint8_t bus, uint8_t dev, uint8_t func)
{
	uint8_t bc;
	uint8_t sc;
	uint8_t secondaryBus;

	bc = pci_read_base_class(bus, dev, func);
	sc = pci_read_sub_class(bus, dev, func);

	if (bc == baseClass && sc == subClass) {
		pci_dev_t * pcidev = (pci_dev_t*) kmalloc(sizeof(pci_dev_t));
		pcidev->vid = pci_read_vid(bus, dev, func);
		pcidev->pid = pci_read_pid(bus, dev, func);
		pcidev->ht = pci_read_headertype(bus, dev, func);
		pcidev->progif = pci_read_progif(bus, dev, func);
		pcidev->bus = bus;
		pcidev->dev = dev;
		pcidev->func = func;
		pcidev->bar0 = pci_read_bar(bus, dev, func, 0);
		pcidev->bar1 = pci_read_bar(bus, dev, func, 1);
		pcidev->bar2 = pci_read_bar(bus, dev, func, 2);
		pcidev->bar3 = pci_read_bar(bus, dev, func, 3);
		pcidev->bar4 = pci_read_bar(bus, dev, func, 4);
		pcidev->bar5 = pci_read_bar(bus, dev, func, 5);

		pci_search_hit(matcher, baseClass, subClass, extra, pcidev);
	}

	if (bc == 0x06 && sc == 0x04) {
		secondaryBus = pci_read_secondary_bus(bus, dev, func);
		pci_search_bus(matcher, baseClass, subClass, extra, secondaryBus);
	}
}

void pci_search_device(pci_matcher_t matcher, uint8_t baseClass, uint8_t subClass, void * extra, uint8_t bus, uint8_t dev)
{
	uint8_t func = 0;

	if (pci_read_vid(bus, dev, func) == PCI_VENID_NONE) return;
	pci_search_function(matcher, baseClass, subClass, extra, bus, dev, func);
	uint8_t headertype = pci_read_headertype(bus, dev, func);
	if ((headertype & 0x80) != 0) {
		for (func = 1; func < 8; func++) {
			if (pci_read_vid(bus, dev, func) != PCI_VENID_NONE) {
				pci_search_function(matcher, baseClass, subClass, extra, bus, dev, func);
			}
		}
	}
}

void pci_search_bus(pci_matcher_t matcher, uint8_t baseClass, uint8_t subClass, void * extra, uint8_t bus)
{
	uint8_t dev;

	for (dev = 0; dev < 32; dev++) {
		pci_search_device(matcher, baseClass, subClass, extra, bus, dev);
	}
}

void pci_search(pci_matcher_t matcher, uint8_t baseClass, uint8_t subClass, void * extra)
{
	uint8_t func;
	uint8_t bus;

	uint8_t headertype = pci_read_headertype(0, 0, 0);
	if ((headertype & 0x80) == 0) {
		// Single PCI host controller
		pci_search_bus(matcher, baseClass, subClass, extra, 0);
	} else {
		for (func = 0; func < 8; func++) {
			if (pci_read_vid(0, 0, func) != PCI_VENID_NONE) {
				bus = func;
				pci_search_bus(matcher, baseClass, subClass, extra, bus);
			}
		}
	}
}




















// LEGACY


pci_dev_t *devices[64]; // @TODO: This really shouldn't be statically allocated but probably use a list_t.
uint32_t num_devs = 0;

uint8_t pci_read_config_8(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
	uint32_t address;
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldevice = (uint32_t) device;
	uint32_t lfunc = (uint32_t) func;
	uint8_t tmp = 0;

	/* create configuration address as per Figure 1 */
	address = (uint32_t) ((lbus << 16) | (ldevice << 11) | (lfunc << 8)
			| (offset & 0xfc) | ((uint32_t) 0x80000000));

	/* write out the address */
	outl(PCI_PORT_ADDRESS, address);
	/* read in the data */
	tmp = (uint8_t) inb(PCI_PORT_DATA + (offset & 3));

	return (tmp);
}

uint16_t pci_read_config_16(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
	uint32_t address;
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldevice = (uint32_t) device;
	uint32_t lfunc = (uint32_t) func;
	uint16_t tmp = 0;

	/* create configuration address as per Figure 1 */
	address = (uint32_t) ((lbus << 16) | (ldevice << 11) | (lfunc << 8)
			| (offset & 0xfc) | ((uint32_t) 0x80000000));

	/* write out the address */
	outl(PCI_PORT_ADDRESS, address);
	/* read in the data */
	tmp = (uint16_t) ins(PCI_PORT_DATA + (offset & 2));

	return (tmp);
}

uint32_t pci_read_config_32(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
	uint32_t address;
	uint32_t lbus = (uint32_t) bus;
	uint32_t ldevice = (uint32_t) device;
	uint32_t lfunc = (uint32_t) func;
	uint32_t tmp = 0;

	/* create configuration address as per Figure 1 */
	address = (uint32_t) ((lbus << 16) | (ldevice << 11) | (lfunc << 8)
			| (offset & 0xfc) | ((uint32_t) 0x80000000));

	/* write out the address */
	outl(PCI_PORT_ADDRESS, address);
	/* read in the data */
	tmp = (uint32_t) inl(PCI_PORT_DATA);

	return (tmp);
}

void pci_print_vendor(uint8_t vid, uint8_t pid) {
	uint32_t i;

	for (i = 0; i < PCI_VENTABLE_LEN; i++) {
		if (PciVenTable[i].VenId == vid) {
			debug(" %s ", PciVenTable[i].VenFull);
			kprintf(" %s ", PciVenTable[i].VenFull);
			break ;
		}
	}
	for (i = 0; i < PCI_DEVTABLE_LEN; i++) {
		if ((PciDevTable[i].VenId == vid) &&
		   (PciDevTable[i].DevId == pid)) {
			debug("%s (%s) ", PciDevTable[i].Chip, PciDevTable[i].ChipDesc);
			kprintf("%s (%s) ", PciDevTable[i].Chip, PciDevTable[i].ChipDesc);
			break ;
		}
	}
}

uint16_t pci_read_vendor(uint8_t bus, uint8_t device, uint8_t func) {
	uint16_t vendor, dev;
	/* try and read the first configuration register. Since there are no */
	/* vendors that == 0xFFFF, it must be a non-existent device. */
	if ((vendor = pci_read_config_16(bus, device, func, PCI_VID)) != PCI_VENID_NONE) {
		dev = pci_read_config_16(bus, device, func, PCI_PID);
		debug("[PCI]: Got vendor 0x%x device 0x%x.", vendor, dev);
		kprintf("[PCI]: Got vendor 0x%x device 0x%x.", vendor, dev);
		pci_print_vendor(vendor, dev);
		debug("\n");
		kprintf("\n");
	}
	return (vendor);
}

void pci_enumerate_function(uint8_t bus, uint8_t device, uint8_t func) {
	uint8_t baseClass;
	uint8_t subClass;
	uint8_t secondaryBus;

	baseClass = pci_read_config_8(bus, device, func, PCI_BASECLASS);
	subClass = pci_read_config_8(bus, device, func, PCI_SUBCLASS);
	if( (baseClass == 0x06) && (subClass == 0x04) ) {
//		debug("\t[PCI]: Got PCI-to-PCI bridge. bus: %d; device: %d; func: %d\n", bus, device, func);
//		kprintf("\t[PCI]: Got PCI-to-PCI bridge. bus: %d; device: %d; func: %d\n", bus, device, func);
		//secondaryBus = getSecondaryBus(bus, device, func);
		//checkBus(secondaryBus);
	} else {
		uint8_t progIf = pci_read_config_8(bus, device, func, PCI_PROGIF);
//		debug("\t[PCI]: Got Base: 0x%x Sub: 0x%x progIf: 0x%x. bus: %d; device: %d; func: %d;\n", baseClass, subClass, progIf, bus, device, func);
//		kprintf("\t[PCI]: Got Base: 0x%x Sub: 0x%x progIf: 0x%x. bus: %d; device: %d; func: %d;\n", baseClass, subClass, progIf, bus, device, func);
		pci_dev_t *dev = (pci_dev_t *) kmalloc(sizeof(pci_dev_t));
		dev->baseclass = baseClass;
		dev->subclass = subClass;
		dev->progif = progIf;
		devices[++num_devs] = dev;
	}
}

void pci_enumerate_device(uint8_t bus, uint8_t device) {
	uint8_t function = 0;

	uint16_t vendor = pci_read_vendor(bus, device, function);
	if(vendor == PCI_VENID_NONE) return;        // Device doesn't exist
	pci_enumerate_function(bus, device, function);
	uint16_t headerType = pci_read_config_16(bus, device, function, PCI_HEADERTYPE);
	if( (headerType & 0x80) != 0) {
		debug("[PCI]: Multi-functional device detected. Enumerating all functions\n");
		kprintf("[PCI]: Multi-functional device detected. Enumerating all functions\n");
		/* It is a multi-function device, so check remaining functions */
		for(function = 1; function < 8; function++) {
			if(pci_read_vendor(bus, device, function) != PCI_VENID_NONE) {
				pci_enumerate_function(bus, device, function);
			}
		}
	}
}

void pci_enumerate_busses() {
	uint16_t bus;
	uint8_t device;

	for(bus = 0; bus < 256; bus++) {
		for(device = 0; device < 32; device++) {
			pci_enumerate_device(bus, device);
		}
	}
}

