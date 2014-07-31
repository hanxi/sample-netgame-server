#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"

#define HASH_SIZE 4
#define GLOBALNAME_LENGTH 32

struct keyvalue {
    struct keyvalue * next;
    char *key;
    uint32_t hash;
    uint32_t value;
};

struct hashmap {
    int hash_size;
    struct keyvalue **node;
};

static uint32_t
hash_string(const char * s) {
    int len = strlen(s);
    uint32_t h = 0;
    assert(s);
    int i = 0;
    while (i++<len) {
        h = 17*h+*s++;
    }
    return h;
}

static uint32_t 
hash_search(struct hashmap * hash, const char * key) {
    uint32_t h = hash_string(key);
    struct keyvalue * node = hash->node[h % hash->hash_size];
    while (node) {
        if (node->hash == h && strncmp(node->key, key, GLOBALNAME_LENGTH) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return -1;
}

static struct keyvalue *
hash_insert(struct hashmap * hash, const char * key, uint32_t value) {
    uint32_t h = hash_string(key);
    struct keyvalue ** pkv = &hash->node[h % hash->hash_size];
    struct keyvalue * node = MALLOC(sizeof(*node));
    node->key = MALLOC(GLOBALNAME_LENGTH);
    memcpy(node->key, key, GLOBALNAME_LENGTH);
    node->next = *pkv;
    node->hash = h;
    node->value = value;
    *pkv = node;

    return node;
}

static uint32_t
hash_remove(struct hashmap * hash, const char * key) {
    uint32_t h = hash_string(key);
    struct keyvalue * ptr = hash->node[h % hash->hash_size];
    uint32_t value = -1;
    if (ptr->key==NULL) {
        return -1;
    }
    if (ptr->hash == h && strncmp(ptr->key,key, GLOBALNAME_LENGTH) == 0) {
        hash->node[h % hash->hash_size] = ptr->next;
		goto _clear;
    }
	while (ptr->next) {
		if (ptr->next->hash == h && strncmp(ptr->next->key, key, GLOBALNAME_LENGTH) == 0) {
            struct keyvalue * node = ptr->next;
			ptr->next = node->next;
            ptr = node;
            goto _clear;
		}
		ptr = ptr->next;
	}
	return -1;
_clear:
    value = ptr->value;
    FREE(ptr->key);
    FREE(ptr);
	return value;
}

static struct hashmap *
hash_new(int hash_size) {
    struct hashmap * h = MALLOC(sizeof(struct hashmap));
    memset(h,0,sizeof(*h));
	h->node = MALLOC(hash_size * sizeof(struct keyvalue *));
	memset(h->node, 0, hash_size * sizeof(struct keyvalue *));
    h->hash_size = hash_size;
    return h;
}

static void
hash_delete(struct hashmap *hash) {
    int i;
    for (i=0;i<hash->hash_size;i++) {
        struct keyvalue * node = hash->node[i];
        while (node) {
            struct keyvalue * next = node->next;
            FREE(node->key);
            FREE(node);
            node = next;
        }
    }
    FREE(hash);
}

