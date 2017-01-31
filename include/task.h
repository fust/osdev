#ifndef __TASK_H
#define __TASK_H

#include "mem/paging.h"
#include "stdint.h"

page_directory_t *clone_directory(page_directory_t * src);

page_table_t * clone_table(page_table_t * src, uint32_t * physAddr);

extern void copy_page_physical(uint32_t, uint32_t);

#endif
