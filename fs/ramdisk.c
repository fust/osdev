#include "fs/ramdisk.h"
#include "stdint.h"
#include "fs/vfs.h"
#include "mem/kmalloc.h"
#include "debug.h"
#include "string.h"

uint32_t RAMDISK_SIZE = 0x500000; // 5MB

vfs_dir_t *ramdisk_root;
ramdisk_t *fs_int;

void ramdisk_fill();

uintptr_t *calculate_offset(ramdisk_t *, ramdisk_inode_t *);

vfs_dir_t *ramdisk_init()
{
	// Allocate the internal storage area
	fs_int = (ramdisk_t *)kmalloc(get_ramdisk_size());
	fs_int->disk_start = (uintptr_t *)((uint32_t) (fs_int) + (sizeof(ramdisk_inode_t) * RAMDISK_MAX_INODES));
	ramdisk_fill();

	// Allocate the VFS struct for the root directory
	ramdisk_root = (vfs_dir_t *)kmalloc(sizeof(vfs_dir_t));

	strcpy(ramdisk_root->fname, "/");

	ramdisk_root->node.flags = 0x775;
	ramdisk_root->node.length = 0;
	ramdisk_root->node.mask = VFS_MASK_DIR;
	ramdisk_root->node.inode = 1;
	ramdisk_root->node.read_dir = &ramdisk_read_dir;

	vfs_dirent_t *dev = (vfs_dirent_t *)kmalloc(sizeof(vfs_dirent_t));

	strcpy(dev->fname, "dev");

	dev->node.flags = 0x775;
	dev->node.length = 0;
	dev->node.mask = VFS_MASK_DIR;
	dev->node.inode = 1;
	dev->node.read_dir = &ramdisk_read_dir;

	ramdisk_root->entries = dev;
	ramdisk_root->last_entry = dev;

	vfs_dirent_t *kbd = (vfs_dirent_t *)kmalloc(sizeof(vfs_dirent_t));

	strcpy(kbd->fname, "kbd");

	kbd->node.flags = 0x775;
	kbd->node.length = fs_int->inodes[2].length;
	kbd->node.mask = VFS_MASK_FILE;
	kbd->node.inode = 2;
	kbd->node.read = &ramdisk_read;
	kbd->node.write = &ramdisk_write;

	ramdisk_root->last_entry->next = kbd;
	ramdisk_root->last_entry = kbd;

	return ramdisk_root;
}

uint32_t get_ramdisk_size()
{
	return RAMDISK_SIZE;
}

uint32_t ramdisk_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
	ramdisk_inode_t inode = fs_int->inodes[node->inode];
	uintptr_t *fs_offset = calculate_offset(fs_int, &inode);

	if (offset > node->length) {
		return 0;
	}

	if (offset + size > node->length) {
		size = node->length - offset;
	}

	memcpy(buffer, (uint8_t *)((uint32_t)(fs_offset) + offset), (size_t) size);

	return size;
}

uint32_t ramdisk_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
	ramdisk_inode_t inode = fs_int->inodes[node->inode];
	uintptr_t *fs_offset = calculate_offset(fs_int, &inode);

	if (offset > node->length) {
		return 0;
	}

	if (offset + size > node->length) {
		size = node->length - offset;
	}

	memcpy((uint8_t *)((uint32_t)(fs_offset) + offset), buffer, (size_t) size);

	return size;
}


vfs_dirent_t *ramdisk_read_dir (vfs_dir_t *dir, uint32_t inode)
{
	if (!dir->entries) {
		return 0;
	}

	vfs_dirent_t *dirent = dir->entries;
	int c = 0;
	while (dirent) {
		if (dirent->node.inode == inode) {
			return dirent;
		}
		c++;
		dirent = dirent->next;
	}

	return 0;
}

void ramdisk_fill()
{
	ramdisk_inode_t root;
	strcpy(root.name, "/");
	fs_int->inodes[0] = root;

	ramdisk_inode_t dev;
	strcpy(dev.name, "dev");
	fs_int->inodes[1] = dev;

	ramdisk_inode_t kbd;
	strcpy(kbd.name, "kbd");
	kbd.offset = 0;
	kbd.length = sizeof(uint32_t) * 32; // Reserve 32 bytes for the keyboard buffer
	fs_int->inodes[2] = kbd;

	uintptr_t *kbd_offset = calculate_offset(fs_int, &kbd);

	strcpy((char *)kbd_offset, "KEYBOARDBUFFER");
}

uintptr_t *calculate_offset(ramdisk_t *disk, ramdisk_inode_t *node)
{
	return (uintptr_t *)((uint32_t)(disk->disk_start) + sizeof(ramdisk_t) + node->offset);
}
