#include "hash-table-base.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define HASH_TABLE_CAPACITY 1024

struct entry {
    const char *key;
    uint32_t value;
    struct entry *next;
    pthread_mutex_t mutex; // Mutex for each entry for fine-grained locking
};

struct hash_table_v2 {
    struct entry *entries[HASH_TABLE_CAPACITY];
};

// Utility function for computing the hash of a key
static uint32_t hash(const char *key) {
    uint32_t hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash % HASH_TABLE_CAPACITY;
}

struct hash_table_v2 *hash_table_v2_create() {
    struct hash_table_v2 *table = malloc(sizeof(struct hash_table_v2));
    assert(table != NULL); // Ensure table allocation succeeded
    memset(table->entries, 0, sizeof(table->entries));
    return table;
}

void hash_table_v2_add(struct hash_table_v2 *table, const char *key, uint32_t value) {
    uint32_t index = hash(key);
    struct entry *new_entry = malloc(sizeof(struct entry));
    assert(new_entry != NULL); // Ensure entry allocation succeeded

    // Duplicate key for safe ownership
    new_entry->key = strdup(key);
    new_entry->value = value;
    pthread_mutex_init(&new_entry->mutex, NULL); // Initialize mutex for new entry

    // Lock only the necessary entry (or the first entry in the bucket for insertion)
    pthread_mutex_lock(&table->entries[index]->mutex);
    new_entry->next = table->entries[index];
    table->entries[index] = new_entry;
    pthread_mutex_unlock(&table->entries[index]->mutex);
}

bool hash_table_v2_contains(struct hash_table_v2 *table, const char *key) {
    uint32_t index = hash(key);
    struct entry *e = table->entries[index];

    while (e != NULL) {
        pthread_mutex_lock(&e->mutex); // Lock current entry
        if (strcmp(e->key, key) == 0) {
            pthread_mutex_unlock(&e->mutex); // Unlock before returning
            return true; // Found the key
        }
        pthread_mutex_unlock(&e->mutex); // Unlock before moving to next entry
        e = e->next;
    }
    return false; // Key not found
}

uint32_t hash_table_v2_get(struct hash_table_v2 *table, const char *key) {
    uint32_t index = hash(key);
    struct entry *e = table->entries[index];

    while (e != NULL) {
        pthread_mutex_lock(&e->mutex); // Lock current entry
        if (strcmp(e->key, key) == 0) {
            uint32_t value = e->value;
            pthread_mutex_unlock(&e->mutex); // Unlock before returning
            return value; // Return the found value
        }
        pthread_mutex_unlock(&e->mutex); // Unlock before moving to next entry
        e = e->next;
    }

    assert(0); // Key should exist
}

void hash_table_v2_destroy(struct hash_table_v2 *table) {
    for (int i = 0; i < HASH_TABLE_CAPACITY; i++) {
        struct entry *e = table->entries[i];
        while (e != NULL) {
            pthread_mutex_lock(&e->mutex); // Lock current entry
            struct entry *next = e->next;
            pthread_mutex_unlock(&e->mutex); // Unlock before freeing

            pthread_mutex_destroy(&e->mutex); // Destroy entry mutex
            free((void*)e->key); // Free duplicated key
            free(e);
            e = next;
        }
    }
    free(table);
}
