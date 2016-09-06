#ifndef __BOCHS_H
#define __BOCHS_H

#include <io.h>

#define BochsConsolePrintChar(c) outb(0xe9, c)

#endif
