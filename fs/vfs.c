#include "fs/vfs.h"
#include "stdint.h"
#include "ds/tree.h"
#include "ds/list.h"
#include "ds/hashtable.h"
#include "string.h"
#include "mem/kmalloc.h"
#include "debug.h"

#define MOUNT_MAX_NODES 128

tree_t *vfs_tree;
hashtable_t *mount_table;

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

vfs_dirent_t *vfs_find_dir (vfs_dir_t *dir, char *fname)
{
	if (dir->node.find_dir != 0) {
		return dir->node.find_dir(dir, fname);
	}
	return 0;
}

void vfs_install()
{
	vfs_tree = tree_create();

	vfs_entry_t *root = (vfs_entry_t *)kmalloc(sizeof(vfs_entry_t));

	root->name = strdup("/");
	root->node = NULL;

	tree_set_root(vfs_tree, root);

	mount_table = hashtable_create(MOUNT_MAX_NODES);
}

void *vfs_mount(char *path, vfs_node_t *node)
{
	if (!vfs_tree) {
		debug("Tried to mount while VFS not initialized.\n");
		return NULL;
	}

	if (!path || !path[0] == '/') {
		debug("Mount paths must be absolute!\n");
		return NULL;
	}

	tree_node_t *ret = NULL;

	char *p = strdup(path);
	char *i = p;

	int path_length = strlen(p);

	while (i < p + path_length) {
		if (*i == '/') {
			*i = '\0';
		}
		i++;
	}

	p[path_length] = '\0';
	i = p + 1;

	tree_node_t *root = vfs_tree->root;

	if (*i == '\0') {
		// Mount root.
		vfs_entry_t *root_node = (vfs_entry_t *)root->value;
		if (root_node->node) {
			debug("%s is already mounted. Unmount first.\n", path);
			return NULL;
		}

		root_node->node = node;
		ret = root;

		// Make sure we know where everything's mounted
		hashtable_insert(mount_table, "root", 0, strdup(path));
	} else {
		tree_node_t *cur_node = root;
		char *pos = i;

		while (1) {
			if (pos >= p + path_length) {
				break;
			}

			int found = 0;

			for (list_item_t *i = cur_node->children->first; i != NULL; i = i->next) {
				tree_node_t *child = (tree_node_t *)i->value;
				vfs_entry_t *entry = (vfs_entry_t *)child->value;
				if (!strcmp(entry->name, pos)) {
					found = 1;
					cur_node = child;
					ret = cur_node;
					break;
				}
			}

			if (!found) {
				debug("Didn't find %s, Creating it.\n", path);
				vfs_entry_t *entry = (vfs_entry_t *)kmalloc(sizeof(vfs_entry_t));
				entry->name = strdup(pos);
				entry->node = NULL;
				cur_node = tree_node_insert_child(vfs_tree, cur_node, entry);
			}

			pos = pos + strlen(pos) + 1;
		}

		vfs_entry_t *entry = (vfs_entry_t *)cur_node->value;
		if (entry->node) {
			debug("%s is already mounted. Unmount first.\n", path);
			return NULL;
		}
		entry->node = node;
		ret = cur_node;

		// Make sure we know where everything's mounted
		hashtable_insert(mount_table, strdup(entry->name), 0, strdup(path));
	}

	debug("Mounted %s.\n", path);

	kfree(p);

	return ret;
}

vfs_node_t *kopen(char *filename)
{
	if (!filename || !filename[0] == '/') {
		debug("Kopen needs absolute paths (for now)!\n");
		return NULL;
	}

	tree_node_t *ret = NULL;

	char *p = strdup(filename);
	char *i = p;

	int path_length = strlen(p);

	while (i < p + path_length) {
		if (*i == '/') {
			*i = '\0';
		}
		i++;
	}

	p[path_length] = '\0';
	i = p + 1;

	tree_node_t *root = vfs_tree->root;

	if (*i == '\0') {
		return root;
	} else {
		tree_node_t *cur_node = root;
		char *pos = i;

		while (1) {
			if (pos >= p + path_length) {
				break;
			}

			int found = 0;

			for (list_item_t *i = cur_node->children->first; i != NULL; i = i->next) {
				tree_node_t *child = (tree_node_t *)i->value;
				vfs_entry_t *entry = (vfs_entry_t *)child->value;
				if (!strcmp(entry->name, pos)) {
					found = 1;
					cur_node = child;
					ret = entry->node;
					break;
				}
			}

			if (!found) {
				ret = NULL;
				break;
			}

			pos = pos + strlen(pos) + 1;
		}
	}

	kfree(p);

	return ret;
}

hashtable_t *vfs_get_mount_table()
{
	return mount_table;
}

void debug_print_vfs_tree_node(tree_node_t * node, size_t height)
{
	/* End recursion on a blank entry */
	if (!node) return;
	/* Indent output */
	for (uint32_t i = 0; i < height; ++i) {
		debug("\t");
	}
	/* Get the current process */
	vfs_entry_t * fnode = (struct vfs_entry *)node->value;
	/* Print the process name */
	if (fnode->node) {
		debug("%s → 0x%x", fnode->name, fnode->node);
	} else {
		debug("%s → (empty)", fnode->name);
	}
	/* Linefeed */
	debug("\n");

	for (list_item_t *i = node->children->first; i != NULL; i = i->next) {
		/* Recursively print the children */
		debug_print_vfs_tree_node(i->value, height + 1);
	}
}

void debug_print_vfs_tree(void)
{
	debug_print_vfs_tree_node(vfs_tree->root, 0);
}
