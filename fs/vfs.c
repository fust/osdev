#include "fs/vfs.h"
#include <stdint.h>
#include "stddef.h"
#include "ds/tree.h"
#include "mem/kmalloc.h"

vfs_inode_t vfs_root;
tree_t* vfs_tree;

void vfs_install()
{
	vfs_tree = tree_create();
}

void vfs_mount(filesystem_t* fs)
{
	vfs_mount_callback callback = fs->mount;
	if (!callback) {
		debug("Invalid filesystem specification: %s", fs->name);
		return;
	}

	// Currently only support mounting the root FS
	//tree_node_t * root_node = vfs_tree->root;

	vfs_inode_t * root = callback();
	tree_set_root(vfs_tree, root);
}

void register_filesystem(filesystem_t * fs)
{
	debug("VFS: Registered filesystem type '%s'\n", fs->name);
}

uint32_t vfs_read(vfs_inode_t * inode, uint32_t length, uint32_t offset, uint8_t * buffer)
{
	if (!inode) {
		return 0;
	}

	if (inode->read) {
		return inode->read(inode, length, offset, buffer);
	} else {
		return 0;
	}
}

uint32_t vfs_write(vfs_inode_t * inode, uint32_t length, uint32_t offset, uint8_t * buffer)
{
	if (!inode) {
		return -1;
	}

	if (inode->write) {
		return inode->write(inode, length, offset, buffer);
	} else {
		return 0;
	}
}

vfs_tnode_t * vfs_readdir(vfs_inode_t * inode)
{
	if (!inode) {
		return -1;
	}

	if ((inode->flags & VFS_TYPE_DIRECTORY) && inode->readdir) {
		return inode->readdir(inode);
	} else {
		return 0;
	}
}

vfs_inode_t * vfs_get_root()
{
	return (vfs_inode_t *) vfs_tree->root->value;
}

vfs_inode_t * vfs_finddir(char **tokens, uint32_t flags, vfs_inode_t * inode)
{
	if (!inode) {
		return -1;
	}

	if ((inode->flags & VFS_TYPE_DIRECTORY) && inode->readdir) {
		return inode->finddir(tokens, flags, inode);
	} else {
		return 0;
	}
}

uint8_t file_find_comparator(tree_node_t *value, char *search) {
	return (value->value == search);
}

vfs_inode_t * file_find_recursive(char **tokens, uint32_t idx, uint32_t flags, tree_node_t *curnode)
{
	if (0 == tokens[0]) {
		debug("\t[VFS]: Tokenstring is empty, returning root.\n");
		return curnode->value;
	}

	if (idx == sizeof(tokens)) {
		debug("\t[VFS]: Reached end of tokenstring. returning NULL.\n");
		return NULL;
	}

	tree_node_t *res = tree_node_find(curnode, tokens[idx], &file_find_comparator);

	if (!res) {
		debug("\t[VFS]: Could not find node, try to request it from the FS driver.\n");
		if (idx < sizeof(tokens) - 1) {
			vfs_inode_t *node = vfs_finddir(tokens, flags, curnode->value);
			if (!node) {
				return NULL;
			}
			debug("\t[VFS]: Got node from FS driver: #%d.\n", node->inode_num);
			return node;
		} else {
			debug("\t[VFS]: Returning current node.\n");
			return curnode;
		}
	}

	if (idx < sizeof(tokens) - 1) {
		debug("\t[VFS]: Recursing to level %d.\n", idx++);
		return file_find_recursive(tokens, idx++, flags, res);
	}

	debug("\t[VFS]: Found a node, returning from level %d.\n", idx++);
	return res->value;
}

vfs_inode_t * file_open(const char * fname, uint32_t flags)
{
	if (strlen(fname) == 1 && fname == '/') {
		return vfs_tree->root->value;
	}

	char *tmp = strdup(fname);
	char *t = strtok(tmp, "/");
	uint32_t indices = 0;

	while(t) {
		indices++;
		t = strtok(0, "/");
	}
	kfree(t);
	kfree(tmp);

	char *tmp2 = strdup(fname);
	char *token = strtok(tmp2, "/");
	char **tokens = (char**) kmalloc(sizeof(char) * indices);
	uint32_t i = 0;

	while (token) {
		tokens[i] = token;
		token = strtok(0, "/");
		i++;
	}

	return file_find_recursive(tokens, 0, flags, vfs_tree->root);
}
