//
// Created by Pavan Kumar  Paluri  on 3/5/20.
//

#ifndef CODINGC_HASH_MAP_H
#define CODINGC_HASH_MAP_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Setting a size of 20 to accommodate 20 CPU-ID & Partition-ID assignments
// as a part of experimentation
#define SIZE 20
#define INT_MAX 2147483647

struct DataItem {
    int Key;
    int Value;
};

// Create Standard HashArray
struct DataItem* hashArray[SIZE];
struct DataItem *dummy;

// Generate a Hashcode given a Key
int hashCode_Gen(int key)
{
    return key%SIZE;
}

// Perform a search operation on Hashmap given a Key
// Return the hash struct containing desired key-val pair
struct DataItem *search_with_key(int key)
{
    // Get the hash Code
    int hash_Index = hashCode_Gen(key);

    // Traverse the hashArray until the very end
    while(hashArray[hash_Index] != NULL)
    {
        if(hashArray[hash_Index]->Key == key)
            return hashArray[hash_Index];
        // Else, go to next cell
        ++hash_Index;

        // Wrap around the table
        hash_Index %= SIZE;
    }

    // IF here, no such key-val pair found, So return NULL
    return NULL;
}

// Perform Insertion into the hash-map
void insert_key_val(int key, int Val)
{
    struct DataItem *new_entry = (struct DataItem*)malloc(sizeof(struct DataItem));
    new_entry->Key = key;
    new_entry->Value = Val;

    // Obtain the hash for this
    int hash_code = hashCode_Gen(key);

    // Traverse the array until we find an empty / Deleted Cell
    while(hashArray[hash_code] != NULL && hashArray[hash_code]->Key != -1)
    {
        ++hash_code;

        // Wrapping..
        hash_code %= SIZE;
    }
    // If here, hashArray[hash_code] is empty and ready to accept a new entry
    hashArray[hash_code] = new_entry;
}

// Perform deletion from hash-map given an entry
struct DataItem* delete_entry_hash_map(struct DataItem *item)
{
    // Get the corresponding key from the entry to be deleted
    int del_key = item->Key;

    // Get the hash of it
    int del_hash_code = hashCode_Gen(del_key);

    // Traverse the array until this corresponding key of that entry is found...
    while(hashArray[del_hash_code] != NULL)
    {
        if(hashArray[del_hash_code]->Key == del_key)
        {
            struct DataItem *tmp = hashArray[del_hash_code];
            hashArray[del_hash_code] = dummy;
            return tmp;
        }
        ++del_hash_code;

        del_hash_code %= SIZE;
    }
    return NULL;
}

void display()
{
    int i = 0;
    for(i=0; i<SIZE; i++) {
        if (hashArray[i] != NULL) {
            printf("(CPU_ID: %d, Partition_ID: %d)\n", hashArray[i]->Key,
                   hashArray[i]->Value);
        }
    }
}
#endif //CODINGC_HASH_MAP_H
