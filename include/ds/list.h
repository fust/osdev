#ifndef __LIST_H
#define __LIST_H

#include <stdint.h>
#include "stddef.h"

typedef struct node {
	struct node *next;
	struct node *prev;
	void *value;
} __attribute__((packed)) node_t;

typedef struct list {
	node_t *head;
	node_t *tail;
	size_t length;
} __attribute__((packed)) list_t;

list_t *list_create();
void list_append(list_t * list, node_t * node);
node_t * list_insert(list_t * list, void * value);
void list_delete(list_t * list, node_t * node);
int list_index_of(list_t * list, void * value);

void list_append_before(list_t * list, node_t * before, node_t * node);
node_t * list_insert_before(list_t * list, node_t * before, void * value);
void list_append_after(list_t * list, node_t * after, node_t * node);
node_t * list_insert_after(list_t * list, node_t * after, void * value);

node_t * list_get(list_t * list, int index);

#define foreach(i, list) for (node_t * i = (list)->head; i != NULL; i = i->next)

#endif
