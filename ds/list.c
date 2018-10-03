#include "ds/list.h"
#include <stdint.h>
#include "stddef.h"
#include "mem/kmalloc.h"
#include "debug.h"

list_t *list_create()
{
	list_t *list = (list_t *)kcalloc(sizeof(list_t));

	list->length = 0;
	list->head = NULL;
	list->tail = NULL;

	return list;
}

void list_append(list_t * list, node_t * node)
{
	if (list->tail->next != NULL) {
		list->tail->next = node;
	}
	list->tail = node;
	list->length++;
}

node_t * list_insert(list_t * list, void * value)
{
	node_t * node = (node_t *) kcalloc(sizeof(node_t));
	node->value = value;

	list_append(list, node);

	return node;
}

void list_delete(list_t * list, node_t * node)
{
	foreach (item, list) {
		if (node == item) {
			if (node->prev != NULL) {
				node->prev->next = node->next;
			}
			if (node->next != NULL) {
				node->next->prev = node->prev;
			}
			list->length--;
		}
	}
}

int list_index_of(list_t * list, void * value)
{
	int i = 0;
	foreach (item, list) {
		if (item->value == value) {
			return i;
		}
		i++;
	}
	return -1;
}

void list_append_before(list_t * list, node_t * before, node_t * node)
{
	node->next = before;
	node->prev = before->prev;

	if (node->prev != NULL) {
		before->prev->next = node;
	}
	before->prev = node;

	list->length++;
}

node_t * list_insert_before(list_t * list, node_t * before, void * value)
{
	node_t * node = (node_t *) kcalloc(sizeof(node_t));
	node->value = value;

	list_append_before(list, before, node);

	return node;
}

void list_append_after(list_t * list, node_t * after, node_t * node)
{
	node->prev = after;
	node->next = after->next;

	if (node->next != NULL) {
		after->next->prev = node;
	}
	after->next = node;

	list->length++;
}

node_t * list_insert_after(list_t * list, node_t * after, void * value)
{
	node_t * node = (node_t *) kcalloc(sizeof(node_t));
	node->value = value;

	list_append_after(list, after, node);

	return node;
}

node_t * list_get(list_t * list, int index)
{
	if (index > list->length - 1) {
		debug("[LIST]: Error: Index out of bounds!");
		return NULL;
	}

	int i = 0;
	foreach (item, list) {
		if (i == index) {
			return item;
		}
	}

	return NULL;
}
