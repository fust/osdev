#ifndef __TREE_H
#define __TREE_H

#include "ds/list.h"
#include <stdint.h>

typedef struct tree_node {
	void * value;
	list_t * children;
	struct tree_node * parent;
} tree_node_t;

typedef struct tree {
	size_t nodes;
	tree_node_t * root;
} tree_t;

typedef uint8_t (*tree_comparator_t)(void *, void *);

tree_t * tree_create();
void tree_set_root(tree_t * tree, void * value);
tree_node_t * tree_node_create(void * value);
void tree_node_insert_child_node(tree_t * tree, tree_node_t * parent, tree_node_t * node);
tree_node_t * tree_node_insert_child(tree_t * tree, tree_node_t * parent, void * value);

tree_node_t * tree_node_find(tree_node_t * node, void * search, tree_comparator_t comparator);
tree_node_t * tree_find(tree_t * tree, void * search, tree_comparator_t comparator);

#endif
