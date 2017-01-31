#ifndef __RAMDISK_H
#define __RAMDISK_H

#include "fs/vfs.h"
#include "stdint.h"

#define RAMDISK_MAX_INODES 128  // Let's just limit it to this for now. Should be enough for the foreseeable future.

typedef struct ramdisk_inode {
	char name[128];
	uint32_t offset; // Offset into the disk
	uint32_t length; // Length of the file (0 for directories/symlinks/etc.)
} ramdisk_inode_t;

typedef struct ramdisk {
	ramdisk_inode_t inodes[RAMDISK_MAX_INODES];
	uintptr_t *disk_start; // Inode offsets are calculated from this base address.
} ramdisk_t;

vfs_dir_t *ramdisk_init();

uint32_t get_ramdisk_size();

uint32_t ramdisk_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

uint32_t ramdisk_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

vfs_dirent_t *ramdisk_read_dir (vfs_dir_t *dir, uint32_t inode);

vfs_dirent_t *ramdisk_find_dir (vfs_dir_t *dir, char *fname);

#endif
