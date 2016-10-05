#ifndef __STDDEF_H
#define __STDDEF_H
#include "stdint.h"

#define _HAVE_SIZE_T
typedef uint16_t size_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif
