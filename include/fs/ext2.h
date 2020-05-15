#ifndef __EXT2_H
#define __EXT2_H

#include <stdint.h>
#include "fs/vfs.h"

#define EXT2_MAGIC 0xEF53

struct ext2_superblock {
	uint32_t inodes_count;
	uint32_t blocks_count;
	uint32_t r_blocks_count;
	uint32_t free_blocks_count;
	uint32_t free_inodes_count;
	uint32_t first_data_block;
	uint32_t log_block_size;
	uint32_t log_frag_size;
	uint32_t blocks_per_group;
	uint32_t frags_per_group;
	uint32_t inodes_per_group;
	uint32_t mtime;
	uint32_t wtime;

	uint16_t mnt_count;
	uint16_t max_mnt_count;
	uint16_t magic;
	uint16_t state;
	uint16_t errors;
	uint16_t minor_rev_level;

	uint32_t lastcheck;
	uint32_t checkinterval;
	uint32_t creator_os;
	uint32_t rev_level;

	uint16_t def_resuid;
	uint16_t def_resgid;

	/* EXT2_DYNAMIC_REV */
	uint32_t first_ino;
	uint16_t inode_size;
	uint16_t block_group_nr;
	uint32_t feature_compat;
	uint32_t feature_incompat;
	uint32_t feature_ro_compat;

	uint8_t uuid[16];
	uint8_t volume_name[16];

	uint8_t last_mounted[64];

	uint32_t algo_bitmap;

	/* Performance Hints */
	uint8_t prealloc_blocks;
	uint8_t prealloc_dir_blocks;
	uint16_t _padding;

	/* Journaling Support */
	uint8_t journal_uuid[16];
	uint32_t journal_inum;
	uint32_t jounral_dev;
	uint32_t last_orphan;

	/* Directory Indexing Support */
	uint32_t hash_seed[4];
	uint8_t def_hash_version;
	uint16_t _padding_a;
	uint8_t _padding_b;

	/* Other Options */
	uint32_t default_mount_options;
	uint32_t first_meta_bg;
	uint8_t _unused[760];

}__attribute__ ((packed));

typedef struct {
	uint32_t block_of_block_usage_bitmap;
	uint32_t block_of_inode_usage_bitmap;
	uint32_t block_of_inode_table;
	uint16_t num_of_unalloc_block;
	uint16_t num_of_unalloc_inode;
	uint16_t num_of_dirs;
	uint8_t unused[14];
} __attribute__((packed)) block_group_desc_t;

typedef struct {
	uint16_t type_perm;
	uint16_t uid;
	uint32_t size_low;
	uint32_t t_accessed;
	uint32_t t_created;
	uint32_t t_modified;
	uint32_t t_deleted;
	uint16_t gid;
	uint16_t hardlinks;
	uint32_t num_sects_used;
	uint32_t flags;
	uint32_t reserved1;
	uint32_t dbp0;
	uint32_t dbp1;
	uint32_t dbp2;
	uint32_t dbp3;
	uint32_t dbp4;
	uint32_t dbp5;
	uint32_t dbp6;
	uint32_t dbp7;
	uint32_t dbp8;
	uint32_t dbp9;
	uint32_t dbp10;
	uint32_t dbp11;
	uint32_t sibp;
	uint32_t dibp;
	uint32_t tibp;
	uint32_t generation;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t block_address;
	uint32_t reserved4;
	uint32_t reserved5;
	uint32_t reserved6;
} __attribute__((packed)) ext2_inode_t;

typedef struct {
	uint32_t inode;
	uint16_t size;
	uint8_t name_length;
	uint8_t type;
	unsigned char *name[128];
} __attribute__((packed)) ext2_d;

typedef struct ext2_superblock ext2_superblock_t;

filesystem_t * ext2_register();

#endif
