#include "ds/hashtable.h"
#include "mem/kmalloc.h"
#include "mem/liballoc/liballoc.h"
#include "string.h"
#include "stdint.h"
#include "stddef.h"

hashtable_t *hashtable_create(size_t size)
{
	hashtable_t * table = (hashtable_t *)kmalloc(sizeof(hashtable_t));

	table->entries = lcalloc(size, sizeof(hashtable_entry_t *));
	table->size = size;
	table->length = 0;

	return table;
}

int hashtable_insert(hashtable_t *table, char *key, bool writeable, void *value)
{
	if (table->length == table->size) {
		return 1;
	}

	table->entries[table->length].key = key;
	table->entries[table->length].writeable = writeable;
	table->entries[table->length].value = value;

	table->length++;

	return 0;
}

int hashtable_remove(hashtable_t *table, char *key)
{
	hashtable_entry_t *entry = hashtable_lookup(table, key);
	if (!entry) {
		return -1;
	}

	kfree(entry);

	return 0;
}

hashtable_entry_t *hashtable_lookup(hashtable_t *table, char *key)
{
	debug("HASHMAP: Scanning %d entries.\n", table->length);
	for (uint32_t i = 0; i < table->length; i++) {
		debug("HASHMAP: Searching index %d: ", i);
		if (table->entries[i].key && strcmp(table->entries[i].key, key) == 0) {
			debug("Found key %s.\n", key);
			return &table->entries[i];
		} else {
			debug("HASHMAP: NOT found.\n");
		}
	}

	return (void *)0;
}

void *hashtable_lookup_value(hashtable_t *table, char *key)
{
	hashtable_entry_t *entry = hashtable_lookup(table, key);
	if (entry) {
		return entry->value;
	}

	return (void *)0;
}

void hashtable_walk(hashtable_t *table, hashtable_walker_t walker)
{
	for (uint32_t i = 0; i < table->length; i++) {
		walker(table, &table->entries[i]);
	}
}
