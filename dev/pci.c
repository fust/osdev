#include "dev/pci.h"
#include "dev/pcihdr.h"
#include "stdint.h"
#include "io.h"
#include "debug.h"

void checkBus(uint8_t bus);
void checkDevice(uint8_t bus, uint8_t device);
void checkFunction(uint8_t bus, uint8_t device, uint8_t function);
void printDeviceDetails(uint16_t vendorID, uint16_t deviceID, uint8_t bus, uint8_t device, uint8_t function);

uint16_t pciConfigReadWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
	uint32_t address;
	uint32_t lbus = (uint32_t) bus;
	uint32_t lslot = (uint32_t) slot;
	uint32_t lfunc = (uint32_t) func;
	uint16_t tmp = 0;

	address = 0x80000000 | ((lbus & 0xFF) << 16) | ((lslot & 0x1F) << 11) | ((lfunc & 0x7) << 8) | (offset & 0xFC);
	//address = (uint32_t) ((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xfc) | ((uint32_t) 0x80000000));

	/* write out the address */
	outl(0xCF8, address);
	/* read in the data */
	/* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
	uint32_t rep = inl(0xCFC);
	tmp = (uint16_t) ((rep >> ((offset & 2) * 8)) & 0xffff);
	return (tmp);
}

uint16_t getDeviceID(uint8_t bus, uint8_t slot, uint8_t func)
{
	return pciConfigReadWord(bus, slot, func, 2);
}

uint16_t getVendorID(uint8_t bus, uint8_t slot, uint8_t func)
{
	return pciConfigReadWord(bus, slot, func, 0);
}

uint8_t getHeaderType(uint8_t bus, uint8_t slot, uint8_t func)
{
	uint16_t res = pciConfigReadWord(bus, slot, func, 0x0E);

	return (uint8_t) (res & 0xF);
}

uint8_t getBaseClass(uint8_t bus, uint8_t slot, uint8_t function)
{
	uint16_t res = pciConfigReadWord(bus, slot, function, 0xA);

	return (uint8_t) ((res >> 8) & 0xFF);
}

uint8_t getSubClass(uint8_t bus, uint8_t slot, uint8_t function)
{
	uint16_t res = pciConfigReadWord(bus, slot, function, 0xA);

	return (uint8_t) (res & 0xFF);
}

uint8_t getSecondaryBus(uint8_t bus, uint8_t slot, uint8_t function)
{
	uint16_t res = pciConfigReadWord(bus, slot, function , 0x18);

	return (uint8_t) ((res >> 8) & 0xFF);
}

void checkFunction(uint8_t bus, uint8_t device, uint8_t function)
{
	uint8_t baseClass;
	uint8_t subClass;
	uint8_t secondaryBus;

	baseClass = getBaseClass(bus, device, function);
	subClass = getSubClass(bus, device, function);
	if ((baseClass == 0x06) && (subClass == 0x04)) {
		secondaryBus = getSecondaryBus(bus, device, function);
		checkBus(secondaryBus);
	}
}

void checkDevice(uint8_t bus, uint8_t device)
{
	uint8_t function = 0;

	uint16_t vendorID = getVendorID(bus, device, function);
	if (vendorID == 0xFFFF)
		return;        // Device doesn't exist

	checkFunction(bus, device, function);
	uint8_t headerType = getHeaderType(bus, device, function);
	if ((headerType & 0x80) != 0) {
		/* It is a multi-function device, so check remaining functions */
		for (function = 1; function < 8; function++) {
			if (getVendorID(bus, device, function) != 0xFFFF) {
				checkFunction(bus, device, function);
			}
		}
	}
	uint16_t deviceID = getDeviceID(bus, device, function);
	printDeviceDetails(vendorID, deviceID, bus, device, function);
}

void checkBus(uint8_t bus)
{
	uint8_t device;

	for (device = 0; device < 32; device++) {
		checkDevice(bus, device);
	}
}

void listPCIBus()
{
	uint8_t function;
	uint8_t bus;

	uint8_t headerType = getHeaderType(0, 0, 0);
	if ((headerType & 0x80) == 0) {
		/* Single PCI host controller */
		checkBus(0);
	} else {
		/* Multiple PCI host controllers */
		for (function = 0; function < 8; function++) {
			if (getVendorID(0, 0, function) != 0xFFFF)
				break;
			bus = function;
			checkBus(bus);
		}
	}
}

void printDeviceDetails(uint16_t vendorID, uint16_t deviceID, uint8_t bus, uint8_t device, uint8_t function)
{
	kprintf("Found PCI device: (0x%x:0x%x:%d) 0x%x:0x%x (", bus, device, function, vendorID, deviceID);

	for (uint32_t i = 0; i < PCI_VENTABLE_LEN; i++) {
		if (PciVenTable[i].VenId == vendorID) {
			kprintf("%s ", PciVenTable[i].VenFull);
		}
	}

	for (uint32_t i = 0; i < PCI_DEVTABLE_LEN; i++) {
		if (PciDevTable[i].DevId == deviceID && PciDevTable[i].VenId == vendorID) {
			kprintf("%s", PciDevTable[i].ChipDesc);
		}
	}

	kprintf(")\n");
}
