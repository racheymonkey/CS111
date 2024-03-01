#include "hash-table-base.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <errno.h>

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

// Initialize mutexes for each hash table entry
struct hash_table_v2 *hash_table_v2_create() {
    struct hash_table_v2 *hash_table = calloc(1, sizeof(struct hash_table_v2));
    if (hash_table == NULL) {
        perror("Failed to allocate memory for hash table");
        exit(errno);
    }

    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        SLIST_INIT(&hash_table->entries[i].list_head);
        int mutex_init_result = pthread_mutex_init(&hash_table->entries[i].mutex, NULL);
        if (mutex_init_result != 0) {
            fprintf(stderr, "Failed to initialize mutex: %s\n", strerror(mutex_init_result));
            // Properly handle error and clean-up
            for (size_t j = 0; j < i; j++) {
                pthread_mutex_destroy(&hash_table->entries[j].mutex);
            }
            free(hash_table);
            exit(mutex_init_result);
        }
    }
    return hash_table;
}

static struct hash_table_entry *get_hash_table_entry(struct hash_table_v2 *hash_table, const char *key) {
    assert(key != NULL);
    uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
    return &hash_table->entries[index];
}

static struct list_entry *get_list_entry(struct hash_table_v2 *hash_table, const char *key, struct list_head *list_head) {
    struct list_entry *entry = NULL;
    SLIST_FOREACH(entry, list_head, pointers) {
        if (strcmp(entry->key, key) == 0) {
            return entry;
        }
    }
    return NULL;
}

bool hash_table_v2_contains(struct hash_table_v2 *hash_table, const char *key) {
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_head *list_head = &hash_table_entry->list_head;
    struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
    return list_entry != NULL;
}

// Modify hash_table_v2_add_entry to use the mutex for each entry for thread safety
void hash_table_v2_add_entry(struct hash_table_v2 *hash_table, const char *key, uint32_t value) {
    uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
    struct hash_table_entry *entry = &hash_table->entries[index];
    
    int lock_result = pthread_mutex_lock(&entry->mutex);
    if (lock_result != 0) {
        fprintf(stderr, "Failed to lock mutex: %s\n", strerror(lock_result));
        exit(lock_result);
    }

    struct list_entry *list_entry = get_list_entry(hash_table, key, &entry->list_head);
    if (list_entry) {
        list_entry->value = value;
    } else {
        // Allocate a new list entry
        list_entry = malloc(sizeof(struct list_entry));
        if (!list_entry) {
            pthread_mutex_unlock(&entry->mutex);
            perror("Failed to allocate memory for new list entry");
            exit(errno);
        }
        char *dup_key = strdup(key);
        if (!dup_key) {
            free(list_entry);
            pthread_mutex_unlock(&entry->mutex);
            perror("Failed to duplicate key");
            exit(errno);
        }
        list_entry->key = dup_key;
        list_entry->value = value;
        SLIST_INSERT_HEAD(&entry->list_head, list_entry, pointers);
    }
    
    int unlock_result = pthread_mutex_unlock(&entry->mutex);
    if (unlock_result != 0) {
        fprintf(stderr, "Failed to unlock mutex: %s\n", strerror(unlock_result));
        exit(unlock_result);
    }
}

uint32_t hash_table_v2_get_value(struct hash_table_v2 *hash_table, const char *key) {
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_head *list_head = &hash_table_entry->list_head;
    struct list_entry *list_entry = get_list_entry(hash_table, key, list_head);
    assert(list_entry != NULL);
    return list_entry->value;
}

// Cleanup function to destroy mutexes and free hash table
void hash_table_v2_destroy(struct hash_table_v2 *hash_table) {
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        pthread_mutex_destroy(&hash_table->entries[i].mutex);
        struct list_head *list_head = &hash_table->entries[i].list_head;
        struct list_entry *entry;
        while (!SLIST_EMPTY(list_head)) {
            entry = SLIST_FIRST(list_head);
            SLIST_REMOVE_HEAD(list_head, pointers);
            free((void*)entry->key); // Free the duplicated key
            free(entry);
        }
    }
    free(hash_table);
}
