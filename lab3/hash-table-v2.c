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
    pthread_mutex_t mutex; // Mutex for each entry in v2
};

struct hash_table_v2 {
    struct hash_table_entry entries[HASH_TABLE_CAPACITY];
};

// Initialize mutexes for each hash table entry
struct hash_table_v2 *hash_table_v2_create() {
    struct hash_table_v2 *hash_table = calloc(1, sizeof(struct hash_table_v2));
    if (hash_table == NULL) {
        fprintf(stderr, "Failed to allocate memory for hash table\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        SLIST_INIT(&hash_table->entries[i].list_head);
        int ret = pthread_mutex_init(&hash_table->entries[i].mutex, NULL);
        if (ret != 0) {
            // Destroy previously initialized mutexes in case of error
            for (size_t j = 0; j < i; ++j) {
                pthread_mutex_destroy(&hash_table->entries[j].mutex);
            }
            free(hash_table);
            fprintf(stderr, "Error initializing mutex: %d\n", ret);
            exit(ret);
        }
    }
    return hash_table;
}

void hash_table_v2_destroy(struct hash_table_v2 *hash_table) {
    for (size_t i = 0; i < HASH_TABLE_CAPACITY; ++i) {
        pthread_mutex_destroy(&hash_table->entries[i].mutex);
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

static struct hash_table_entry *get_hash_table_entry(struct hash_table_v2 *hash_table, const char *key) {
    assert(key != NULL);
    uint32_t index = bernstein_hash(key) % HASH_TABLE_CAPACITY;
    return &hash_table->entries[index];
}

static struct list_entry *find_list_entry(struct hash_table_entry *entry, const char *key) {
    struct list_entry *le = NULL;
    SLIST_FOREACH(le, &entry->list_head, pointers) {
        if (strcmp(le->key, key) == 0) {
            return le;
        }
    }
    return NULL;
}

void hash_table_v2_add_entry(struct hash_table_v2 *hash_table, const char *key, uint32_t value) {
    struct hash_table_entry *entry = get_hash_table_entry(hash_table, key);

    int lock_ret = pthread_mutex_lock(&entry->mutex);
    if (lock_ret != 0) {
        fprintf(stderr, "Error locking mutex: %d\n", lock_ret);
        hash_table_v2_destroy(hash_table);
        exit(lock_ret);
    }

    struct list_entry *list_entry = find_list_entry(entry, key);
    if (list_entry == NULL) {
        list_entry = malloc(sizeof(struct list_entry));
        if (list_entry == NULL) {
            fprintf(stderr, "Failed to allocate memory for new list entry\n");
            pthread_mutex_unlock(&entry->mutex); // Assuming this call succeeds, given context
            hash_table_v2_destroy(hash_table);
            exit(EXIT_FAILURE);
        }
        char *dup_key = strdup(key);
        if (dup_key == NULL) {
            free(list_entry);
            fprintf(stderr, "Failed to duplicate key\n");
            pthread_mutex_unlock(&entry->mutex); // Assuming this call succeeds, given context
            hash_table_v2_destroy(hash_table);
            exit(EXIT_FAILURE);
        }
        list_entry->key = dup_key;
        list_entry->value = value;
        SLIST_INSERT_HEAD(&entry->list_head, list_entry, pointers);
    } else {
        list_entry->value = value;
    }

    lock_ret = pthread_mutex_unlock(&entry->mutex);
    if (lock_ret != 0) {
        fprintf(stderr, "Error unlocking mutex: %d\n", lock_ret);
        hash_table_v2_destroy(hash_table);
        exit(lock_ret);
    }
}

bool hash_table_v2_contains(struct hash_table_v2 *hash_table, const char *key) {
    struct hash_table_entry *entry = get_hash_table_entry(hash_table, key);
    int lock_ret = pthread_mutex_lock(&entry->mutex);
    if (lock_ret != 0) {
        fprintf(stderr, "Error locking mutex: %d\n", lock_ret);
        hash_table_v2_destroy(hash_table);
        exit(lock_ret);
    }

    bool exists = find_list_entry(entry, key) != NULL;

    lock_ret = pthread_mutex_unlock(&entry->mutex);
    if (lock_ret != 0) {
        fprintf(stderr, "Error unlocking mutex: %d\n", lock_ret);
        hash_table_v2_destroy(hash_table);
        exit(lock_ret);
    }

    return exists;
}

uint32_t hash_table_v2_get_value(struct hash_table_v2 *hash_table, const char *key) {
    struct hash_table_entry *entry = get_hash_table_entry(hash_table, key);
    int lock_ret = pthread_mutex_lock(&entry->mutex);
    if (lock_ret != 0) {
        fprintf(stderr, "Error locking mutex: %d\n", lock_ret);
        hash_table_v2_destroy(hash_table);
        exit(lock_ret);
    }

    struct list_entry *list_entry = find_list_entry(entry, key);
    assert(list_entry != NULL); // In production, handle this more gracefully

    uint32_t value = list_entry->value;

    lock_ret = pthread_mutex_unlock(&entry->mutex);
    if (lock_ret != 0) {
        fprintf(stderr, "Error unlocking mutex: %d\n", lock_ret);
        hash_table_v2_destroy(hash_table);
        exit(lock_ret);
    }

    return value;
}
