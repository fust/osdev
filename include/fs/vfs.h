#ifndef __VFS_H
#define __VFS_H

#include <stdint.h>
#include "stddef.h"
#include "ds/list.h"
#include "ds/tree.h"
#include "dev/device.h"

#define VFS_TYPE_FILE 0x01
#define VFS_TYPE_DIRECTORY 0x02
#define VFS_TYPE_SYMLINK 0x03

struct vfs_inode;
struct vfs_tnode;

typedef uint32_t (* vfs_read_t)(struct vfs_inode * inode, uint32_t length, uint32_t offset, uint8_t * buffer);
typedef uint32_t (* vfs_write_t)(struct vfs_inode * inode, uint32_t length, uint32_t offset, uint8_t * buffer);
typedef struct vfs_tnode *(* vfs_readdir_t)(struct vfs_inode * inode);
typedef struct vfs_inode *(* vfs_finddir_t)(char **tokens, uint32_t flags, struct vfs_inode * inode);
typedef struct vfs_inode *(* vfs_mount_callback)();

typedef struct vfs_inode {
	uint32_t atime; // Last accessed time
	uint32_t mtime; // Last modified time
	uint32_t ctime; // Created time

	size_t size; // Filesize

	struct vfs_tnode * subdirs;	// List of subdirectory tnodes (only valid if flags = 0x02 (directory))
	struct vfs_tnode * subfiles; // List of subfile tnodes (only valid if flags = 0x02 (directory))

	uint32_t num_subdirs;	// Number of subdirectory tnodes (only valid if flags = 0x02 (directory))
	uint32_t num_subfiles;	// Number of subdirectory tnodes (only valid if flags = 0x02 (directory))

	uint32_t inode_num; // Concrete FS inode number

	uint32_t flags; // List of flags, like inode type (file, directory, symlink, etc)

	struct vfs_inode * ptr; // Reference (for symlinks)

	/* Inode operations */
	vfs_read_t read;
	vfs_write_t write;
	vfs_readdir_t readdir;
	vfs_finddir_t finddir;
} vfs_inode_t;

typedef struct vfs_tnode {
	char * name;
	vfs_inode_t * inode;
} vfs_tnode_t;

typedef struct filesystem {
	char * name;
	vfs_mount_callback * mount;
	struct filesystem * next;
} filesystem_t;

void register_filesystem(filesystem_t * fs);

void vfs_install();

void vfs_mount();

uint32_t vfs_read(vfs_inode_t * inode, uint32_t length, uint32_t offset, uint8_t * buffer);
uint32_t vfs_write(vfs_inode_t * inode, uint32_t length, uint32_t offset, uint8_t * buffer);
vfs_tnode_t * vfs_readdir(vfs_inode_t * inode);

vfs_inode_t * vfs_get_root();

vfs_inode_t * file_open(const char * fname, uint32_t flags);

#endif
