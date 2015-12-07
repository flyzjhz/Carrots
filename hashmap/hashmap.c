//
//  hashmap.c
//  
//
//  Created by SongJian on 15/12/7.
//
//

#include <syslog.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>

#include "hashmap.h"


static uint32_t primes[] =
{
    3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89,
    107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
    1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419,
    10103, 12143, 14591, 17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523,
    108631, 130363, 156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897,
    1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369
};



/* hashmap_hash - hash a string
 * hashcode_index -- RShash
 * hashcode_create -- BKDRhash
 * */

inline static uint32_t hashcode_index(char *key, int len, uint32_t size)
{
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    char *str = key;
    
    while (len-- > 0)
    {
        hash = hash * a + (*str++);
        a *= b;
    }
    
    return (uint32_t )(hash & 0x7FFFFFFF) % size;
}


inline static uint32_t hashcode_create(char *key, int len)
{
    uint32_t seed = 131;
    uint32_t hash = 0;
    char *str = key;
    
    while (len-- > 0)
        hash = hash * seed + (*str++);
    
    return (hash & 0x7FFFFFFF);
}


/* hashmap_link - insert element into map */

void hashmap_link(hashmap_t *map, hashmap_entry_t *elm)
{
    hashmap_entry_t **etr = map->data + hashcode_index((elm)->key, (elm)->key_len , map->size);
    if (((elm)->next = *etr) != NULL)
        (*etr)->prev = elm;
    *etr = elm;
    map->used++;
}

/* hashmap_size - allocate and initialize hash map */
static int hashmap_size(hashmap_t *map, unsigned size)
{
    hashmap_entry_t **h;
    
    map->data = h = (hashmap_entry_t **) calloc(size, sizeof(hashmap_entry_t *));
    if(!h)
    {
        return -1;
    }
    map->size = size;
    map->used = 0;
    
    while (size-- > 0)
        *h++ = 0;
    
    return 0;
}




/* hashmap_create - create initial hash map */
hashmap_t *hashmap_create(int size)
{
    hashmap_t *map = (hashmap_t *)calloc(1, sizeof(hashmap_t));
    if (map == NULL) {
        printf("calloc fail\n");
        return NULL;
    }
    
    uint8_t idx = 0;
    
    for(; idx < sizeof(primes) / sizeof(uint32_t); idx++)
    {
        if(size < primes[idx])
        {
            break;
        }
    }
    
    if(idx >= sizeof(primes) /  sizeof(uint32_t))
        return NULL;
    
    map->idx = idx;
    
    return map;
}

/* hashmap_grow - extend existing map */
static int hashmap_grow(hashmap_t *map)
{
    hashmap_entry_t *ht;
    hashmap_entry_t *next;
    unsigned old_size = map->size;
    hashmap_entry_t **h = map->data;
    hashmap_entry_t **old_entries = h;
    
    if(hashmap_size(map, primes[++map->idx]) == -1)
    {
        return -1;
    }
    
    while (old_size-- > 0)
    {
        for (ht = *h++; ht; ht = next)
        {
            next = ht->next;
            hashmap_link(map, ht);
        }
    }
    
    if (old_entries) {
        free((char *) old_entries);
        old_entries = NULL;
    }
    
    return 0;
}


/* hashmap_insert - enter (key, value) pair */
hashmap_entry_t *hashmap_insert(hashmap_t *map, char *key, uint32_t key_len, void *value)
{
    hashmap_entry_t *ht = NULL;
    
    if ((map->used >= map->size) && hashmap_grow(map)==-1)
    {
        printf("hashmap_grow error:%m");
        return NULL;
    }
    
    ht = (hashmap_entry_t *)calloc(1, sizeof(hashmap_entry_t) + key_len + 1);
    if(!ht)
    {
        printf("malloc hashmap entry error:%m");
        return NULL;
    }
    
    // don't need free key, it just free ht.
    ht->key = (char *)(ht + 1);
    memcpy(ht->key, key, key_len);
    ht->key_len = key_len;
    ht->value = value;
    ht->hash_code = hashcode_create(key, key_len);
    hashmap_link(map, ht);
    
    //printf("key:%s, val:%s, klen:%d\n", key, value, key_len);
    
    return (ht);
}


/* hashmap_find - lookup value */
void *hashmap_find(hashmap_t *map, char *key, int key_len)
{
    hashmap_entry_t *ht;
    uint32_t idx = hashcode_index(key, key_len , map->size);
    uint32_t hc = hashcode_create(key, key_len);
    
    if (map)
    {
        for (ht = map->data[idx]; ht; ht = ht->next)
        {
            if (key_len == ht->key_len && hc == ht->hash_code
                && (memcmp(key, ht->key, ht->key_len) == 0))
                return (ht->value);
        }
    }
    
    return NULL;
}


/* hashmap_locate - lookup entry */
hashmap_entry_t *hashmap_locate(hashmap_t *map, char *key, int key_len)
{
    hashmap_entry_t *ht;
    uint32_t idx = hashcode_index(key, key_len, map->size);
    uint32_t hc = hashcode_create(key, key_len);
    
    if (map)
    {
        for (ht = map->data[idx]; ht; ht = ht->next)
        {
            if (key_len == ht->key_len && hc == ht->hash_code && (memcmp(key, ht->key, ht->key_len) == 0))
                return (ht);
        }
    }
    
    return (0);
}


/* hashmap_delete - delete one entry */
int hashmap_delete(hashmap_t *map, char *key, int key_len)
{
    if (map != NULL) {
        hashmap_entry_t **h = map->data + hashcode_index(key, key_len, map->size);
        hashmap_entry_t *ht = *h;
        uint32_t hc = hashcode_create(key, key_len);
        
        for (; ht; ht = ht->next)
        {
            if (key_len == ht->key_len
                && hc == ht->hash_code
                && !memcmp(key, ht->key, ht->key_len))
            {
                if (ht->next)
                    ht->next->prev = ht->prev;
                
                if (ht->prev)
                    ht->prev->next = ht->next;
                else
                    *h = ht->next;
                
                map->used--;
             
                // free ht item
                if (ht->value != NULL) {
                    free(ht->value);
                    ht->value = NULL;
                }
                
                if (ht != NULL) {
                    free(ht);
                    ht = NULL;
                }
                
                
                return 0;
            }
        }
    }
    
    return 1;
}


/* hashmap_free - destroy hash map */
void hashmap_free(hashmap_t *map)
{
    if (map != NULL)
    {
        unsigned i = map->size;
        hashmap_entry_t *ht;
        hashmap_entry_t *next;
        hashmap_entry_t **h = map->data;
        
        while (i-- > 0)
        {
            for (ht = *h++; ht; ht = next)
            {
                next = ht->next;
                
                if (ht->value) {
                    free(ht->value);
                    ht->value = NULL;
                }
                if (ht) {
                    free(ht);
                    ht = NULL;
                }
            }
        }
        if (map->data) {
            free(map->data);
            map->data = NULL;
        }
        free(map);
        map = NULL;
    }
}



/* hashmap_walk - iterate over hash map */
void hashmap_walk(hashmap_t *map, void (*action) (hashmap_entry_t *, char *), char *ptr)
{
    if (map)
    {
        unsigned i = map->size;
        hashmap_entry_t **h = map->data;
        hashmap_entry_t *ht;
        
        while (i-- > 0)
            for (ht = *h++; ht; ht = ht->next)
                (*action) (ht, ptr);
    }
}


/* hashmap_list - list all map members */
hashmap_entry_t **hashmap_list(hashmap_t *map)
{
    hashmap_entry_t **list;
    hashmap_entry_t *member;
    int     count = 0;
    int     i;
    
    if (map != 0)
    {
        list = (hashmap_entry_t **) calloc((map->used + 1), sizeof(*list));
        if(!list)
        {
            printf("calloc error:%m");
            return NULL;
        }
        
        for (i = 0; i < map->size; i++)
            for (member = map->data[i]; member != 0; member = member->next)
                list[count++] = member;
    }
    else
    {
        list = (hashmap_entry_t **) calloc(1, sizeof(*list));
        if(!list)
        {
            printf("calloc error:%m");
            return NULL;
        }
        
    }
    list[count] = 0;
    
    return (list);
}




// ----------

#ifdef HASHMAP_TEST

void hash_test_func(hashmap_t *hmp)
{
    //char *key = NULL;
    char *val = NULL;
    char key[1024] = {0};
    char buf[1024] = {0};
    int j=0;
    int i=0;
    
    for(; i< 9; i++)
    {
        j = 0xffffff - i;
        
        sprintf(key, "key_%x", j);
        
        //key = strdup(buf);
        
        sprintf(buf, "val_%x", j);
        
        val = strdup(buf);
        
        hashmap_insert(hmp, key, strlen(key), val);
        
        //printf("insert key:%s, val:%s\n", key, val);
    }
    
    
    for(i=0; i< 9; i++)
    {
        j = 0xffffff - i;
        sprintf(buf, "key_%x", j);
        
        printf("find key:%s, val:%s\n", buf, hashmap_find(hmp, buf, strlen(buf)));
        
    }
    
    
}

int main(int argc, char *argv[])
{
    hashmap_t *map = NULL;
    
    map = hashmap_create(102400);
    if(map == NULL)
    {
        printf("create hashmap error:%m");
        return -1;
    }
    
    
    hash_test_func(map);
    
    
    hashmap_free(map);
    
    return 0;
}

#endif

