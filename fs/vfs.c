#include "fs/vfs.h"
#include "stdint.h"

uint32_t vfs_read (vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
	if (node->read) {
		return node->read(node, offset, size, buffer);
	}

	return size;
}

uint32_t vfs_write (vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
	if (node->write) {
		return node->write(node, offset, size, buffer);
	}

	return size;
}

vfs_dirent_t *vfs_read_dir (vfs_dir_t *dir, uint32_t inode)
{
	if (dir->node.read_dir != 0) {
		//debug("\tVFS: read for directory %s is at 0x%x\n", dir->fname, dir->node.read_dir);
		return dir->node.read_dir(dir, inode);
	}
	return 0;
}
