#include "ds/list.h"
#include "stdint.h"
#include "stddef.h"
#include "mem/kmalloc.h"
#include "debug.h"

list_t *list_create()
{
	list_t *list = (list_t *)kmalloc(sizeof(list_t));
	list->first = NULL;
	list->last = NULL;
	list->length = 0;

	return list;
}

int list_insert_start(list_t *list, list_item_t *item)
{
	ASSERT(!(item->next || item->prev), "Item already belongs to a list!");

	if (list->first) {
		item->next = list->first;
		list->first->prev = item;
		list->first = item;
		item->owner = list;
		list->length++;
	} else if (list->length == 0) { // Empty list
		list->first = list->last = item;
		item->owner = list;
	} else {
		return -1; // Error
	}

	return 0;
}

int list_insert_end(list_t *list, list_item_t *item)
{
	ASSERT(!(item->next || item->prev), "Item already belongs to a list!");

	if (list->last) {
		item->prev = list->last;
		list->last->next = item;
		list->last = item;
		item->owner = list;
		list->length++;
	} else if (list->length == 0) { // Empty list
		list->last = list->first = item;
		item->owner = list;
	} else {
		return -1; // Error
	}

	return 0;
}

int list_insert_after(list_t *list, list_item_t *item, list_item_t *after)
{
	ASSERT(!(item->next || item->prev), "Item already belongs to a list!");

	if (after->next) {
		after->next->prev = item;
		item->next = after->next;
	} else { // This is the last item in the list
		list->last = item;
	}

	item->prev = after;
	after->next = item;
	item->owner = list;

	list->length++;

	return 0;
}

int list_insert_before(list_t *list, list_item_t *item, list_item_t *before)
{
	ASSERT(!(item->next || item->prev), "Item already belongs to a list!");

	if (before->prev) {
		before->prev->next = item;
		item->prev = before->prev;
	} else { // This is the first item in the list
		list->first = item;
	}

	item->next = before;
	before->prev = item;
	item->owner = list;

	list->length++;

	return 0;
}

int list_remove(list_t *list, list_item_t *item)
{
	ASSERT((item->next || item->prev), "Item doesn't belong to a list!");

	if (item->prev && item->next) {
		item->prev->next = item->next;
		item->next->prev = item->prev;
	}

	if (!item->prev) {
		list->first = item->next;
	}

	if (!item->next) {
		list->last = item->prev;
	}

	item->owner = NULL;

	list->length--;

	return 0;
}

int list_append(list_t *list, void *value)
{
	list_item_t *item = (list_item_t *)kmalloc(sizeof(list_item_t));

	item->prev = NULL;
	item->next = NULL;
	item->owner = list;
	item->value = value;

	return list_insert_end(list, item);
}

list_item_t *list_find(list_t *list, list_item_comparator_t comparator, void *value)
{
	list_item_t *next = list->first;
	while (next) {
		if (comparator(next, value)) {
			return next;
		}
		next = next->next;
	}

	return NULL;
}
