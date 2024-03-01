#include "hash-table-base.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <pthread.h>

struct list_entry {
	const char *key;
	uint32_t value;
	SLIST_ENTRY(list_entry) pointers;
};

SLIST_HEAD(list_head, list_entry);

struct hash_table_entry {
    struct list_head list_head;
    pthread_mutex_t mutex; // Mutex for each entry for v2
};

struct hash_table_v2 {
	struct hash_table_entry entries[HASH_TABLE_CAPACITY];
};

// Modify hash_table_v2_create to initialize mutexes for each entry
struct hash_table_v2 *hash_table_v2_create() {
    struct hash_table_v2 *hash_table = calloc(1, sizeof(struct hash_table_v2));
    assert(hash_table != NULL);
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        struct hash_table_entry *entry = &hash_table->entries[i];
        SLIST_INIT(&entry->list_head);
        if (pthread_mutex_init(&entry->mutex, NULL) != 0) {
            free(hash_table); // Assuming cleanup is not needed for previously initialized mutexes
            exit(EXIT_FAILURE);
        }
    }
    return hash_table;
}

static struct hash_table_entry *get_hash_table_entry(struct hash_table_v2 *hash_table,
                                                     const char *key)
{
	assert(key != NULL);
	uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
	struct hash_table_entry *entry = &hash_table->entries[index];
	return entry;
}

static struct list_entry *get_list_entry(struct hash_table_v2 *hash_table,
                                         const char *key,
                                         struct list_head *list_head)
{
	assert(key != NULL);

	struct list_entry *entry = NULL;
	
	SLIST_FOREACH(entry, list_head, pointers) {
	  if (strcmp(entry->key, key) == 0) {
	    return entry;
	  }
	}
	return NULL;
}

bool hash_table_v2_contains(struct hash_table_v2 *hash_table,
                            const char *key)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
	return list_entry != NULL;
}

// Modify hash_table_v2_add_entry to use the mutex for each entry for thread safety
void hash_table_v2_add_entry(struct hash_table_v2 *hash_table, const char *key, uint32_t value) {
    uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
    struct hash_table_entry *entry = &hash_table->entries[index];
    
    if (pthread_mutex_lock(&entry->mutex) != 0) {
        exit(EXIT_FAILURE);
    }

    struct list_entry *list_entry = get_list_entry(hash_table, key, &entry->list_head);
    if (list_entry != NULL) {
        list_entry->value = value;
    } else {
        list_entry = calloc(1, sizeof(struct list_entry));
        if (list_entry == NULL) {
            pthread_mutex_unlock(&entry->mutex); // Attempt to unlock before exit, even though we're about to exit
            exit(EXIT_FAILURE);
        }
        list_entry->key = strdup(key); // Ensure key duplication for ownership
        list_entry->value = value;
        SLIST_INSERT_HEAD(&entry->list_head, list_entry, pointers);
    }
    
    if (pthread_mutex_unlock(&entry->mutex) != 0) {
        exit(EXIT_FAILURE);
    }
}

uint32_t hash_table_v2_get_value(struct hash_table_v2 *hash_table,
                                 const char *key)
{
	struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
	struct list_head *list_head = &hash_table_entry->list_head;
	struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
	assert(list_entry != NULL);
	return list_entry->value;
}

// Modify hash_table_v2_destroy to destroy the mutex for each entry
void hash_table_v2_destroy(struct hash_table_v2 *hash_table) {
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        struct hash_table_entry *entry = &hash_table->entries[i];

		struct list_head *list_head = &entry->list_head;
		struct list_entry *list_entry = NULL;
		while (!SLIST_EMPTY(list_head)) {
			list_entry = SLIST_FIRST(list_head);
			SLIST_REMOVE_HEAD(list_head, pointers);
			free(list_entry);
		}
        pthread_mutex_destroy(&entry->mutex); // Destroy the mutex for each entry
    }
    free(hash_table);
}
