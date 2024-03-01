#include "hash-table-base.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/queue.h>

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

// Function prototype for hash_table_v1_destroy
void hash_table_v1_destroy(struct hash_table_v1 *hash_table);

// Initialize the mutex for v1
struct hash_table_v1 *hash_table_v1_create() {
    struct hash_table_v1 *hash_table = calloc(1, sizeof(struct hash_table_v1));
    if (hash_table == NULL) {
        exit(EXIT_FAILURE); // Use a standard error code for memory allocation failure
    }
    int ret = pthread_mutex_init(&hash_table->mutex, NULL);
    if (ret != 0) {
        // Cleanup and exit if mutex init fails
        free(hash_table);
        exit(ret); // Exit with the error code returned by pthread_mutex_init
    }
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        SLIST_INIT(&hash_table->entries[i].list_head);
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

void hash_table_v1_add_entry(struct hash_table_v1 *hash_table, const char *key, uint32_t value) {
    struct list_entry *new_entry = calloc(1, sizeof(struct list_entry));
    if (new_entry == NULL) {
        fprintf(stderr, "Failed to allocate memory for new entry\n");
        exit(EXIT_FAILURE); // Use a standard error code for memory allocation failure
    }
    char *dup_key = strdup(key);
    if (dup_key == NULL) {
        free(new_entry);
        fprintf(stderr, "Failed to duplicate key\n");
        exit(EXIT_FAILURE); // Use a standard error code for memory allocation failure
    }

    int lock_ret = pthread_mutex_lock(&hash_table->mutex);
    if (lock_ret != 0) {
        free(dup_key);
        free(new_entry);
        hash_table_v1_destroy(hash_table);
        exit(lock_ret); // Exit with the error code returned by pthread_mutex_lock
    }
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);

    if (list_entry == NULL) {
        new_entry->key = dup_key; // Assign the duplicated key to the new entry
        new_entry->value = value;
        SLIST_INSERT_HEAD(&hash_table_entry->list_head, new_entry, pointers);
    } else {
        // If the entry exists, update the value and free the newly allocated memory
        list_entry->value = value;
        free(dup_key);
        free(new_entry);
    }
    int unlock_ret = pthread_mutex_unlock(&hash_table->mutex);
    if (unlock_ret != 0) {
        hash_table_v1_destroy(hash_table);
        exit(unlock_ret); // Exit with the error code returned by pthread_mutex_unlock
    }
}

void hash_table_v1_destroy(struct hash_table_v1 *hash_table) {
    // Attempt to destroy the mutex and check for errors
    int destroy_ret = pthread_mutex_destroy(&hash_table->mutex);
    if (destroy_ret != 0) {
        // Log the error if mutex destroy fails
        fprintf(stderr, "Failed to destroy mutex with error code: %d\n", destroy_ret);
        // Consider your options here: exiting is harsh in a destructor.
        // It's often better to log the failure and continue with cleanup.
    }

    // Proceed to cleanup the hash table's contents
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        struct list_head *list_head = &hash_table->entries[i].list_head;
        struct list_entry *entry;
        struct list_entry *temp_entry;

        SLIST_FOREACH_SAFE(entry, list_head, pointers, temp_entry) {
            SLIST_REMOVE(list_head, entry, list_entry, pointers);
            free((void*)entry->key); // Cast to void* to match free's signature
            free(entry);
        }
    }

    // Finally, free the hash table structure itself
    free(hash_table);
}
