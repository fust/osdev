#include "ds/tree.h"
#include <stdint.h>
#include "stddef.h"
#include "mem/kmalloc.h"
#include "ds/list.h"

tree_t * tree_create()
{
	tree_t * tree = (tree_t *)kcalloc(sizeof(tree_t));
	tree->nodes = 0;
	tree->root = NULL;

	return tree;
}

void tree_set_root(tree_t * tree, void * value)
{
	tree_node_t * node = tree_node_create(value);

	tree->nodes = 1;
	tree->root = node;
}

tree_node_t * tree_node_create(void * value)
{
	tree_node_t * node = (tree_node_t *)kcalloc(sizeof(tree_node_t));
	node->value = value;
	node->children = list_create();
	node->parent = NULL;

	return node;
}

void tree_node_insert_child_node(tree_t * tree, tree_node_t * parent, tree_node_t * node)
{
	list_insert(parent->children, node);
	node->parent = parent;
	tree->nodes++;
}

tree_node_t * tree_node_insert_child(tree_t * tree, tree_node_t * parent, void * value)
{
	tree_node_t * node = tree_node_create(value);
	tree_node_insert_child_node(tree, parent, node);

	return node;
}

tree_node_t * tree_node_find(tree_node_t * node, void * search, tree_comparator_t comparator)
{
	if (comparator(node->value, search)) {
		return node;
	}

	tree_node_t * retval;
	foreach (child, node->children) {
		retval = tree_node_find((tree_node_t *)child->value, search, comparator);
		if (retval) {
			return retval;
		}
	}

	return NULL;
}

tree_node_t * tree_find(tree_t * tree, void * search, tree_comparator_t comparator)
{
	return tree_node_find(tree->root, search, comparator);
}
