#ifndef __TREE_H
#define __TREE_H

#include "stdint.h"
#include "stddef.h"
#include "ds/list.h"

typedef struct tree_node {
	void *value;
	struct tree_node *parent;
	list_t *children;
} tree_node_t;

typedef struct tree {
	size_t nodes;
	tree_node_t *root;
} tree_t;

typedef uint8_t (*tree_node_comparator_t) (void *, void *);

tree_t *tree_create();
void tree_set_root(tree_t *tree, void *value);
void tree_node_insert_child_node(tree_t *tree, tree_node_t *parent, tree_node_t *node);
tree_node_t *tree_node_insert_child(tree_t *tree, tree_node_t *parent, void *value);
tree_node_t *tree_find(tree_t *tree, void *value, tree_node_comparator_t comparator);

#endif
