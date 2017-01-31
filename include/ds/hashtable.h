#ifndef __HASHTABLE_H
#define __HASHTABLE_H

#include "stdint.h"
#include "stdbool.h"
#include "stddef.h"

typedef struct hashtable_entry {
	char *key;
	bool writeable;
	void *value;
} hashtable_entry_t;

typedef struct hashtable {
	hashtable_entry_t *entries;
	uint32_t length;
	uint32_t size;
} hashtable_t;

hashtable_t *hashtable_create(size_t size);

int hashtable_insert(hashtable_t *table, char *key, bool writeable, void *value);
int hashtable_remove(hashtable_t *table, char *key);
hashtable_entry_t *hashtable_lookup(hashtable_t *table, char *key);
void *hashtable_lookup_value(hashtable_t *table, char *key);

typedef void (*hashtable_walker_t)(hashtable_t *table, hashtable_entry_t *entry);
void hashtable_walk(hashtable_t *table, hashtable_walker_t walker);

#endif
