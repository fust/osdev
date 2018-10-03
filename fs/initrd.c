#include "fs/initrd.h"
#include "fs/vfs.h"
#include "debug.h"

static uint8_t const FILE_INODE_OFFSET = 2;

uint32_t * initrd_location;
vfs_inode_t * fsroot;
initrd_header_t * header;
initrd_fs_header_t * file_headers;

void initrd_set_location(uint32_t * location)
{
	initrd_location = location;
}

uint32_t initrd_read(vfs_inode_t * inode, uint32_t length, uint32_t offset, uint8_t * buffer)
{
	if (!inode->inode_num) {
		return -1;
	}

	initrd_fs_header_t header = file_headers[inode->inode_num - FILE_INODE_OFFSET];
	if (length > header.length) {
		length = header.length;
	}
	memcpy(buffer, header.offset, length);

	return length;
}

vfs_tnode_t * initrd_readdir(vfs_inode_t * inode)
{
	vfs_tnode_t * tnode = (vfs_tnode_t *) kmalloc(sizeof(vfs_tnode_t));
	debug("[INITRD]: Try to read inode #%d.\n", inode->inode_num);
	if (inode->inode_num == 0) {
		tnode->name = strdup("[root]");
		tnode->inode = (initrd_inode_t *) initrd_location;
		debug("[INITRD]: Returning root inode.\n");
		return tnode;
	} else if (inode->inode_num == 1) {
		tnode->name = strdup("dev");
		tnode->inode = 1;

		return tnode;
	} else {
		tnode->name = strdup(fsroot->subfiles[1 + inode->inode_num].name);
		tnode->inode = fsroot->subfiles[1 + inode->inode_num].inode;

		return tnode;
	}
}

vfs_inode_t * initrd_finddir(char **tokens, uint32_t flags, vfs_inode_t *node)
{
	debug("[INITRD]: Request to find directory, start recursive search from inode #%d.\n", node->inode_num);
	for (uint32_t i = 0; i < sizeof(tokens); i++) {
		if (!tokens[i])
			break;
		debug("\t[INITRD]: Searching for token '%s'.\n", tokens[i]);

		for (uint32_t j = 0; j < fsroot->num_subfiles; j++) {
			if (strcmp(fsroot->subfiles[j].name, tokens[i]) == 0) {
				debug("\t[INITRD]: Found requested tnode, returning.\n");
				return fsroot->subfiles[j].inode;
			}
		}
	}
	return NULL;
}

vfs_inode_t * initrd_create()
{
	header = (initrd_header_t *) initrd_location;
	file_headers = (initrd_fs_header_t *) ((uint32_t)initrd_location + sizeof(initrd_header_t));

	debug("INITRD: Header located at 0x%x, file headers start at 0x%x\n", header, file_headers);

	vfs_inode_t * root = (vfs_inode_t *) kmalloc(sizeof(vfs_inode_t));
	debug("INITRD: Root is to be located at 0x%x\n", root);
	root->inode_num = 0;
	root->flags = VFS_TYPE_DIRECTORY;
	root->read = initrd_read;
	root->readdir = initrd_readdir;
	root->finddir = initrd_finddir;

	fsroot = root;

	vfs_inode_t * dev_node = (vfs_inode_t *) kmalloc(sizeof(vfs_inode_t));
	dev_node->inode_num = 1;
	dev_node->flags = VFS_TYPE_DIRECTORY;
	dev_node->read = initrd_read;
	dev_node->readdir = initrd_read;
	dev_node->finddir = initrd_finddir;

	uint32_t num_root_nodes = header->num_files;
	vfs_inode_t *root_nodes = (vfs_inode_t *)kmalloc(sizeof(vfs_inode_t) * num_root_nodes);
    vfs_tnode_t *subfiles = (vfs_tnode_t *)kmalloc(sizeof(vfs_tnode_t) * num_root_nodes);

	for (uint32_t i = 0; i < header->num_files; i++) {
		debug("[INITRD]: Processing header at 0x%x\n", &file_headers[i]);
		if (file_headers[i].magic != 0xDEADBEEF) {
			debug("[INITRD]: Invalid file header at index %d\n", i);
			continue;
		}
		file_headers[i].offset += (uint32_t) initrd_location;
		root_nodes[i].inode_num = FILE_INODE_OFFSET + i;
		root_nodes[i].flags = VFS_TYPE_FILE;
		root_nodes[i].size = file_headers[i].length;
		root_nodes[i].read = initrd_read;

		subfiles[i].inode = &root_nodes[i];
		subfiles[i].name = strdup(&file_headers[i].fname);

		debug("[INTIRD]: Found file %s with magic 0x%x\n", subfiles[i].name, file_headers[i].magic);
	}

	root->subfiles = subfiles;
	root->num_subfiles = num_root_nodes;

	return fsroot;
/*
	initrd_inode_t * node = (initrd_inode_t *)initrd_location;
	debug("INITRD: located at 0x%x\n", node);
	vfs_inode_t * root = (vfs_inode_t *) kmalloc(sizeof(vfs_inode_t));
	debug("INITRD: Root is to be located at 0x%x\n", root);
	root->inode_num = node->inode_num;
	root->flags = VFS_TYPE_DIRECTORY;
	root->read = initrd_read;
	root->readdir = initrd_readdir;

	fsroot = root;

	vfs_inode_t * file1 = (vfs_inode_t *) kmalloc(sizeof(vfs_inode_t));
	file1->inode_num = 1;
	file1->flags = VFS_TYPE_FILE;
	file1->read = initrd_read;

	vfs_tnode_t * file1t = (vfs_tnode_t *) kmalloc(sizeof(vfs_tnode_t));
	file1t->inode = file1;
	file1t->name = strdup("About this OS.txt");

	root->subfiles = file1;
	root->num_subfiles = 1;

	return fsroot;*/
}

vfs_inode_t * initrd_mount()
{
	debug("INITRD: Mounting filesystem\n");
	return initrd_create();
}
