#ifndef __PCI_H
#define __PCI_H

#include <stdint.h>
#include "stddef.h"

// PCI Header offsets
#define PCI_VID			0x00
#define PCI_PID			0x02
#define PCI_CMD			0x04
#define PCI_ST			0x06
#define PCI_REV			0x08
#define PCI_PROGIF		0x09
#define PCI_SUBCLASS	0x0A
#define PCI_BASECLASS	0x0B
#define PCI_CLS			0x0C
#define PCI_LT			0x0D
#define PCI_HEADERTYPE	0x0E
#define PCI_BIST		0x0F
#define PCI_BAR0		0x10
#define	PCI_BAR1		0x14
#define PCI_BAR2		0x18
#define PCI_BAR3		0x1C
#define PCI_BAR4		0x20
#define PCI_BAR5		0x24
#define PCI_CIS			0x28
#define PCI_SVID		0x2C
#define PCI_SID			0x2E
#define PCI_ERA			0x30
#define PCI_CAPP		0x34
#define PCI_IL			0x3C
#define PCI_IP			0x3D
#define PCI_MG			0x3E
#define PCI_ML			0x3F

#define PCI_SECONDARY_BUS	0x19

#define PCI_PORT_ADDRESS	0xCF8
#define PCI_PORT_DATA		0xCFC

#define PCI_VENID_NONE	0xFFFF

#define PCI_BARS_MAX = 5;

typedef struct pci_bar {
	uint8_t flag : 1; // If 0 it's a bar_mbar_t, if 1 it's a bar_iobar_t.
} pci_bar_t;

typedef struct bar_mbar {
	pci_bar_t;
	uint8_t type : 1;
	uint8_t prefetchable : 1;
	uint32_t address : 27; // 16-byte aligned
} pci_bar_mbar_t;

typedef struct pci_bar_iobar {
	pci_bar_t;
	uint8_t reserved : 1;
	uint32_t address : 30; // 4-byte aligned
} pci_bar_iobar_t;

typedef struct pci_dev {
	uint16_t vid; // Vendor ID
	uint16_t pid; // Product ID
	uint16_t command;
	uint16_t status;
	uint8_t revision_id;
	uint8_t progif;
	uint8_t subclass;
	uint8_t baseclass;
	uint8_t cls; // Cache Line Size
	uint8_t lt; // Latency Timer
	uint8_t ht; // Header Type
	uint8_t bist; // Built-in self test
	pci_bar_t * bar0;
	pci_bar_t * bar1;
	pci_bar_t * bar2;
	pci_bar_t * bar3;
	pci_bar_t * bar4;
	pci_bar_t * bar5;
	uint32_t * cis; // Cardbus CIS pointer
	uint16_t svid; // Subsystem Vendor ID
	uint16_t sid; // Subsystem ID
	uint32_t era; // Expansion ROM address
	uint8_t * capabilities;
	uint32_t reserved1 : 24;
	uint32_t reserved2;
	uint8_t interrupt_line;
	uint8_t interrupt_pin;
	uint8_t min_grant;
	uint8_t max_latency;
	uint8_t bus;
	uint8_t dev;
	uint8_t func;
} pci_dev_t;

typedef uint8_t (*pci_matcher_t)(pci_dev_t *, void * extra);

uint32_t pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t len);

void pci_config_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset, uint8_t len, uint32_t data);

pci_bar_t * pci_read_bar(uint8_t bus, uint8_t dev, uint8_t func, uint8_t barnum);

void pci_search(pci_matcher_t matcher, uint8_t baseClass, uint8_t subClass, void * extra);

void pci_enumerate_busses();

#endif
