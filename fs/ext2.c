#include "fs/ext2.h"
#include "fs/ata.h"
#include "mem/kmalloc.h"
#include "fs/vfs.h"
#include "debug.h"

typedef struct ext2_fs {
	ext2_superblock_t * sb;
	uint32_t bs; // Blocksize
	uint32_t bpg; // Blocks per group
	uint32_t ipg; // Inodes per group
	uint32_t num_block_groups;
	block_group_desc_t * bgdt;
} ext2_fs_t;

ext2_fs_t * fsroot;

int read_block(uint32_t blockno, uint8_t * buffer) {
	if (!blockno) {
		return 1;
	}

	debug("[EXT2]: Reading block 0x%x\n", blockno);
	unsigned char err = ide_read_sectors(0, 1, blockno, buffer);
	ide_print_error(0, err);
	debug("[EXT2]: Read block 0x%x\n", blockno);

	return 0;
}

ext2_superblock_t * ext2_read_superblock() {
	ext2_superblock_t *sb = (ext2_superblock_t *)kmalloc(sizeof(ext2_superblock_t));

	unsigned char err = ide_read_sectors(0, 2, 2, (uint8_t *) sb);
	if (err)
		return 0;

	if (sb->magic != EXT2_MAGIC) {
		return 0;
	}

	fsroot->sb = sb;
	fsroot->bs = 1024 << sb->log_block_size;
	fsroot->num_block_groups = sb->blocks_count / sb->blocks_per_group;
	fsroot->ipg = sb->inodes_per_group;
	fsroot->bpg = sb->blocks_per_group;

	debug("[EXT2]: Initialized FS. BS: %d IPG: %d BPG: %d\n", fsroot->bs, fsroot->ipg, fsroot->bpg);

	int err2 = read_block(2, fsroot->bgdt);
	if (err2)
		return 0;

	return sb;
}

ext2_inode_t * ext2_read_inode(uint32_t inode_num) {
	debug("[EXT2]: Read inode #%d\n", inode_num);

	uint32_t block_group = (inode_num - 1) / fsroot->ipg;
	block_group_desc_t * block_descriptor_table = &fsroot->bgdt[block_group];
	uint32_t t = block_descriptor_table->block_of_inode_table;
	debug("\t[EXT2]: Block's inode table is in block #%d\n", t);
	uint32_t index = (inode_num - 1) % fsroot->ipg;
	uint32_t block = (inode_num * fsroot->sb->inode_size) / fsroot->bs;

	debug("[EXT2]: Inode #%d is located block %d\n", inode_num, block);

	ext2_inode_t * res = (ext2_inode_t *) kmalloc(sizeof(ext2_inode_t));

	read_block((t + index) * fsroot->bs, res);

	debug("[EXT2]: Inode's block address: 0x%x\n", res->block_address);

	return res;
}

uint32_t ext2_read(vfs_inode_t * inode, uint32_t length, uint32_t offset, uint8_t * buffer) {
	return 0;
}

vfs_tnode_t * ext2_readdir(vfs_inode_t * inode) {
	vfs_tnode_t * node = (vfs_tnode_t *) kmalloc(sizeof(vfs_tnode_t));

	return node;
}

vfs_inode_t * ext2_finddir(char **tokens, uint32_t flags, vfs_inode_t *node) {
	return node;
}

vfs_inode_t * ext2_mount() {
	vfs_inode_t * root = (vfs_inode_t *) kmalloc(sizeof(vfs_inode_t));

	ext2_superblock_t * sb = ext2_read_superblock();

	ext2_inode_t * root_inode = ext2_read_inode(2);

	root->inode_num = 0;
	root->flags = VFS_TYPE_DIRECTORY;
	root->read = ext2_read;
	root->readdir = ext2_readdir;
	root->finddir = ext2_finddir;

	return root;
}

filesystem_t * ext2_register() {
	filesystem_t * ext2 = (filesystem_t *) kmalloc(sizeof(filesystem_t));
	ext2->name = strdup("EXT2");
	ext2->mount = &ext2_mount;
	register_filesystem(ext2);

	return ext2;
}
