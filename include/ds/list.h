#ifndef __LIST_H
#define __LIST_H

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

typedef struct list_item {
	void *value;
	struct list_item *prev;
	struct list_item *next;
	void *owner;
} list_item_t;

typedef struct list {
	size_t length;
	list_item_t *first;
	list_item_t *last;
} list_t;

list_t *list_create();

int list_insert_start(list_t *list, list_item_t *item);
int list_insert_end(list_t *list, list_item_t *item);
int list_insert_after(list_t *list, list_item_t *item, list_item_t *after);
int list_insert_before(list_t *list, list_item_t *item, list_item_t *before);
int list_remove(list_t *list, list_item_t *item);
int list_append(list_t *list, void *value);

typedef bool (*list_item_comparator_t)(list_item_t*, void *);

list_item_t *list_find(list_t *list, list_item_comparator_t comparator, void *value);

#endif
