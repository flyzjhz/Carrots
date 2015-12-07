//
//  hashmap.h
//  
//
//  Created by SongJian on 15/12/7.
//
//

#ifndef hashmap_h
#define hashmap_h

#include <stdio.h>

/* Structure of one hash table entry. */
typedef struct hashmap_entry_s hashmap_entry_t;

struct hashmap_entry_s
{
    char                *key;           /* lookup key */
    uint32_t            key_len;
    uint32_t            hash_code;      /* hash code */
    char                *value;         /* associated value */
    hashmap_entry_t     *next;          /* colliding entry */
    hashmap_entry_t     *prev;          /* colliding entry */
};

/* Structure of one hash table. */
typedef struct hashmap_t
{
    uint32_t            size;           /* length of entries array */
    uint16_t            used;           /* number of entries in table */
    uint8_t             idx;            /* primes id */
    hashmap_entry_t     **data;         /* entries array, auto-resized */
    
}hashmap_t;


hashmap_t *hashmap_create(int);

hashmap_entry_t *hashmap_insert(hashmap_t *, char *, uint32_t , void *);

void *hashmap_find(hashmap_t *table, char *key, int key_len);

void hashmap_free(hashmap_t *table);

int hashmap_delete(hashmap_t *table, char *key, int key_len);

#endif /* hashmap_h */
