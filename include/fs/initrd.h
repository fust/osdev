#ifndef __INITRD_H
#define __INITRD_H

#include "fs/vfs.h"

typedef struct initrd_header
{
   uint32_t num_files;
} initrd_header_t;

typedef struct initrd_fs_header {
	uint32_t magic;
	uint8_t fname[32];
	uint32_t offset;
	uint32_t length;
} initrd_fs_header_t;

typedef struct initrd_inode {
	uint32_t inode_num;
	uint32_t offset;
	size_t length;
} initrd_inode_t;

void initrd_set_location(uint32_t * location);

uint32_t initrd_read(vfs_inode_t * inode, uint32_t length, uint32_t offset, uint8_t * buffer);
vfs_tnode_t * initrd_readdir(vfs_inode_t * inode);
vfs_inode_t * initrd_mount();

#endif
