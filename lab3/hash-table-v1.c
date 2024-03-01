#include "hash-table-base.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>

#include <errno.h>

struct list_entry {
    const char *key;
    uint32_t value;
    SLIST_ENTRY(list_entry) pointers;
};

SLIST_HEAD(list_head, list_entry);

struct hash_table_entry {
    struct list_head list_head;
};

struct hash_table_v1 {
    struct hash_table_entry entries[HASH_TABLE_CAPACITY];
    pthread_mutex_t mutex; // Single mutex for v1
};

void hash_table_v1_destroy(struct hash_table_v1 *hash_table);

struct hash_table_v1 *hash_table_v1_create() {
    struct hash_table_v1 *hash_table = calloc(1, sizeof(struct hash_table_v1));
    if (hash_table == NULL) {
        perror("Failed to allocate memory for hash table");
        exit(errno);
    }

    int mutex_init_result = pthread_mutex_init(&hash_table->mutex, NULL); // Initialize the single mutex
    if (mutex_init_result != 0) {
        fprintf(stderr, "Failed to initialize mutex: %s\n", strerror(mutex_init_result));
        hash_table_v1_destroy(hash_table); // Cleanup before exiting
        exit(mutex_init_result);
    }

    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        SLIST_INIT(&hash_table->entries[i].list_head);
    }
    return hash_table;
}

// Modify hash_table_v1_destroy to destroy the mutex
void hash_table_v1_destroy(struct hash_table_v1 *hash_table) {
    int mutex_destroy_result = pthread_mutex_destroy(&hash_table->mutex);
    if (mutex_destroy_result != 0) {
        fprintf(stderr, "Failed to destroy mutex: %s\n", strerror(mutex_destroy_result));
    }
    free(hash_table);
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
    int lock_result = pthread_mutex_lock(&hash_table->mutex);
    if (lock_result != 0) {
        fprintf(stderr, "Failed to lock mutex: %s\n", strerror(lock_result));
        exit(lock_result);
    }
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);
    int unlock_result = pthread_mutex_unlock(&hash_table->mutex);
    if (unlock_result != 0) {
        fprintf(stderr, "Failed to unlock mutex: %s\n", strerror(unlock_result));
        exit(unlock_result);
    }
    return list_entry != NULL;
}

// Modify hash_table_v1_add_entry to use the mutex for thread safety
void hash_table_v1_add_entry(struct hash_table_v1 *hash_table, const char *key, uint32_t value) {
    struct list_entry *new_entry = calloc(1, sizeof(struct list_entry));
    if (new_entry == NULL) {
        perror("Failed to allocate memory for new entry");
        exit(errno);
    }
    char *dup_key = strdup(key);
    if (dup_key == NULL) {
        free(new_entry);
        perror("Failed to duplicate key");
        exit(errno);
    }

    int lock_result = pthread_mutex_lock(&hash_table->mutex); // Lock the entire hash table
    if (lock_result != 0) {
        fprintf(stderr, "Failed to lock mutex: %s\n", strerror(lock_result));
        free(dup_key);
        free(new_entry);
        exit(lock_result);
    }
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);

    /* Update the value if it already exists */
    if (list_entry == NULL) {
        new_entry->key = dup_key;
        new_entry->value = value;
        SLIST_INSERT_HEAD(&hash_table_entry->list_head, new_entry, pointers);
    } else {
        list_entry->value = value;
        free(dup_key);
        free(new_entry);
    }
    int unlock_result = pthread_mutex_unlock(&hash_table->mutex); // Unlock the entire hash table
    if (unlock_result != 0) {
        fprintf(stderr, "Failed to unlock mutex: %s\n", strerror(unlock_result));
        exit(unlock_result);
    }
}

uint32_t hash_table_v1_get_value(struct hash_table_v1 *hash_table, const char *key) {
    int lock_result = pthread_mutex_lock(&hash_table->mutex);
    if (lock_result != 0) {
        fprintf(stderr, "Failed to lock mutex: %s\n", strerror(lock_result));
        exit(lock_result);
    }
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);
    int unlock_result = pthread_mutex_unlock(&hash_table->mutex);
    if (unlock_result != 0) {
        fprintf(stderr, "Failed to unlock mutex: %s\n", strerror(unlock_result));
        exit(unlock_result);
    }
    assert(list_entry != NULL);
    return list_entry->value;
}
