#include "ds/tree.h"
#include "ds/list.h"
#include "stdint.h"
#include "mem/kmalloc.h"

tree_t *tree_create()
{
	tree_t * tree = (tree_t *)kmalloc(sizeof(tree));

	tree->nodes = 0;
	tree->root = NULL;

	return tree;
}

tree_node_t *tree_node_create(void *value)
{
	tree_node_t * node = (tree_node_t *)kmalloc(sizeof(tree_node_t));
	if (!node) {
		return NULL;
	}

	node->children = list_create();
	node->parent = NULL;
	node->value = value;

	return node;
}

void tree_set_root(tree_t *tree, void *value)
{
	tree_node_t *node = tree_node_create(value);

	tree->root = node;
	tree->nodes = 1;
}

void tree_node_insert_child_node(tree_t *tree, tree_node_t *parent, tree_node_t *node)
{
	list_append(parent->children, node);
	node->parent = parent;
	tree->nodes++;
}

tree_node_t *tree_node_insert_child(tree_t *tree, tree_node_t *parent, void *value)
{
	tree_node_t *node = tree_node_create(value);
	tree_node_insert_child_node(tree, parent, node);
	return node;
}

tree_node_t *tree_node_find(tree_node_t *node, void *value, tree_node_comparator_t comparator)
{
	if (comparator(node->value, value)) {
		return node;
	}

	tree_node_t *found;
	for (list_item_t * i = node->children->first; i != NULL; i = i->next) {
		found = tree_node_find((tree_node_t *)i->value, value, comparator);
		if (found) {
			return found;
		}
	}

	return 0;
}

tree_node_t *tree_find(tree_t *tree, void *value, tree_node_comparator_t comparator)
{
	return tree_node_find(tree->root, value, comparator);
}
