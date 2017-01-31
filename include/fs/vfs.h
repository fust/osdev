#ifndef __VFS_H
#define __VFS_H

#include "stdint.h"
#include "ds/hashtable.h"

typedef struct vfs_node vfs_node_t;
typedef struct vfs_dirent vfs_dirent_t;
typedef struct vfs_dir vfs_dir_t;

typedef uint32_t (*vfs_read_t)(vfs_node_t *, uint32_t, uint32_t, uint8_t *);
typedef uint32_t (*vfs_write_t)(vfs_node_t *, uint32_t, uint32_t, uint8_t *);
typedef vfs_dirent_t* (*vfs_read_dir_t)(vfs_dir_t *, uint32_t);
typedef vfs_dirent_t* (*vfs_write_dir_t)(vfs_dir_t *, uint32_t);
typedef vfs_dirent_t* (*vfs_find_dir_t)(vfs_dir_t *, char *);

#define VFS_MASK_FILE 0x1
#define VFS_MASK_DIR 0x2
#define VFS_MASK_DEVICE 0x3
#define VFS_MASK_SYMLINK 0x4

typedef struct vfs_node {
	uint32_t mask;
	uint32_t flags;
	uint32_t length;
	uint32_t inode;
	vfs_read_t read;
	vfs_write_t write;
	vfs_read_dir_t read_dir;
	vfs_write_dir_t write_dir;
	vfs_find_dir_t find_dir;
	struct vfs_node *ptr;	// Used in symlinks
	void * device; // Used to mount pipes, etc.
} vfs_node_t;

typedef struct vfs_dirent {
	vfs_node_t node;
	char fname[128];
	struct vfs_dirent *next;
} vfs_dirent_t;

typedef struct vfs_dir {
	vfs_node_t node;
	char fname[128];
	vfs_dirent_t *entries;
	vfs_dirent_t *last_entry;
} vfs_dir_t;

typedef struct vfs_entry {
	char * name;
	vfs_node_t *node;
} vfs_entry_t;

uint32_t vfs_read (vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

uint32_t vfs_write (vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

vfs_dirent_t *vfs_read_dir (vfs_dir_t *dir, uint32_t inode);

vfs_dirent_t *vfs_find_dir (vfs_dir_t *dir, char *fname);

void vfs_install();

void *vfs_mount(char *path, vfs_node_t *node);

vfs_node_t *kopen(char *filename);

hashtable_t *vfs_get_mount_table();

void debug_print_vfs_tree(void);

#endif
