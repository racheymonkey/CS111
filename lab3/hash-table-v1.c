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
    pthread_mutex_t mutex; // Mutex for thread safety
};

struct hash_table_v1 {
    struct hash_table_entry entries[HASH_TABLE_CAPACITY];
};

// Forward declarations for utility functions
static uint32_t bernstein_hash(const char *key);

struct hash_table_v1 *hash_table_v1_create() {
    struct hash_table_v1 *hash_table = calloc(1, sizeof(struct hash_table_v1));
    assert(hash_table != NULL);
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        struct hash_table_entry *entry = &hash_table->entries[i];
        SLIST_INIT(&entry->list_head);
        pthread_mutex_init(&entry->mutex, NULL); // Initialize the mutex
    }
    return hash_table;
}

static struct hash_table_entry *get_hash_table_entry(struct hash_table_v1 *hash_table, const char *key) {
    assert(key != NULL);
    uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
    return &hash_table->entries[index];
}

static struct list_entry *get_list_entry(struct hash_table_v1 *hash_table, const char *key, struct list_head *list_head) {
    struct list_entry *entry = NULL;
    SLIST_FOREACH(entry, list_head, pointers) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
    }
    return NULL;
}

bool hash_table_v1_contains(struct hash_table_v1 *hash_table, const char *key) {
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    pthread_mutex_lock(&hash_table_entry->mutex); // Lock
    bool found = get_list_entry(hash_table, key, &hash_table_entry->list_head) != NULL;
    pthread_mutex_unlock(&hash_table_entry->mutex); // Unlock
    return found;
}

void hash_table_v1_add_entry(struct hash_table_v1 *hash_table, const char *key, uint32_t value) {
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    pthread_mutex_lock(&hash_table_entry->mutex); // Lock

    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);
    if (list_entry != NULL) {
        list_entry->value = value;
    } else {
        list_entry = calloc(1, sizeof(struct list_entry));
        assert(list_entry != NULL);
        list_entry->key = strdup(key); // Duplicate key
        list_entry->value = value;
        SLIST_INSERT_HEAD(&hash_table_entry->list_head, list_entry, pointers);
    }

    pthread_mutex_unlock(&hash_table_entry->mutex); // Unlock
}

uint32_t hash_table_v1_get_value(struct hash_table_v1 *hash_table, const char *key) {
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    pthread_mutex_lock(&hash_table_entry->mutex); // Lock

    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);
    assert(list_entry != NULL); // Key must exist
    uint32_t value = list_entry->value;

    pthread_mutex_unlock(&hash_table_entry->mutex); // Unlock
    return value;
}

void hash_table_v1_destroy(struct hash_table_v1 *hash_table) {
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        struct hash_table_entry *entry = &hash_table->entries[i];
        pthread_mutex_lock(&entry->mutex); // Lock

        struct list_entry *list_entry;
        while (!SLIST_EMPTY(&entry->list_head)) {
            list_entry = SLIST_FIRST(&entry->list_head);
            SLIST_REMOVE_HEAD(&entry->list_head, pointers);
            free((void*)list_entry->key); // Free the duplicated key
            free(list_entry);
        }

        pthread_mutex_unlock(&entry->mutex); // Unlock before destroying mutex
        pthread_mutex_destroy(&entry->mutex);
    }
    free(hash_table);
}

// Utility function: Bernstein hash function for strings
static uint32_t bernstein_hash(const char *key) {
    uint32_t hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash;
}
