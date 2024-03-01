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

// Initialize the mutex for v1
struct hash_table_v1 *hash_table_v1_create() {
    struct hash_table_v1 *hash_table = calloc(1, sizeof(struct hash_table_v1));
    assert(hash_table != NULL);
    int err;
    if ((err = pthread_mutex_init(&hash_table->mutex, NULL)) != 0) {
        fprintf(stderr, "Failed to initialize mutex: %s\n", strerror(err));
        free(hash_table);
        exit(err);
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

bool hash_table_v1_contains(struct hash_table_v1 *hash_table, const char *key) {
    int err;
    if ((err = pthread_mutex_lock(&hash_table->mutex)) != 0) {
        fprintf(stderr, "Failed to lock mutex: %s\n", strerror(err));
        exit(err);
    }
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);
    if ((err = pthread_mutex_unlock(&hash_table->mutex)) != 0) {
        fprintf(stderr, "Failed to unlock mutex: %s\n", strerror(err));
        exit(err);
    }
    return list_entry != NULL;
}

void hash_table_v1_add_entry(struct hash_table_v1 *hash_table, const char *key, uint32_t value) {
    struct list_entry *new_entry = calloc(1, sizeof(struct list_entry));
    if (new_entry == NULL) {
        exit(EXIT_FAILURE);
    }
    char *dup_key = strdup(key);
    if (dup_key == NULL) {
        free(new_entry);
        exit(EXIT_FAILURE);
    }

    int err;
    if ((err = pthread_mutex_lock(&hash_table->mutex)) != 0) {
        fprintf(stderr, "Failed to lock mutex: %s\n", strerror(err));
        free(dup_key);
        free(new_entry);
        exit(err);
    }

    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);

    if (list_entry == NULL) {
        new_entry->key = dup_key;
        new_entry->value = value;
        SLIST_INSERT_HEAD(&hash_table_entry->list_head, new_entry, pointers);
    } else {
        list_entry->value = value;
        free(dup_key);
        free(new_entry);
    }

    if ((err = pthread_mutex_unlock(&hash_table->mutex)) != 0) {
        fprintf(stderr, "Failed to unlock mutex: %s\n", strerror(err));
        exit(err);
    }
}

uint32_t hash_table_v1_get_value(struct hash_table_v1 *hash_table, const char *key) {
    int err;
    if ((err = pthread_mutex_lock(&hash_table->mutex)) != 0) {
        fprintf(stderr, "Failed to lock mutex: %s\n", strerror(err));
        exit(err);
    }
    struct hash_table_entry *hash_table_entry = get_hash_table_entry(hash_table, key);
    struct list_entry *list_entry = get_list_entry(hash_table, key, &hash_table_entry->list_head);
    if ((err = pthread_mutex_unlock(&hash_table->mutex)) != 0) {
        fprintf(stderr, "Failed to unlock mutex: %s\n", strerror(err));
        exit(err);
    }
    assert(list_entry != NULL);
    return list_entry->value;
}

void hash_table_v1_destroy(struct hash_table_v1 *hash_table) {
    int err;
    if ((err = pthread_mutex_destroy(&hash_table->mutex)) != 0) {
        fprintf(stderr, "Failed to destroy mutex: %s\n", strerror(err));
        // Proceed with cleanup even if mutex destroy fails
    }
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        struct list_head *list_head = &hash_table->entries[i].list_head;
        struct list_entry *entry;
        while (!SLIST_EMPTY(list_head)) {
            entry = SLIST_FIRST(list_head);
            SLIST_REMOVE_HEAD(list_head, pointers);
            free((void*)entry->key);
            free(entry);
        }
    }
    free(hash_table);
}
