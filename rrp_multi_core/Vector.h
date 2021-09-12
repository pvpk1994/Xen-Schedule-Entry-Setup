//
// Created by Pavan Kumar  Paluri  on 3/5/20.
//

#ifndef CODINGC_VECTOR_H
#define CODINGC_VECTOR_H
#include <stdio.h>
#include <stdlib.h>
#define VECTOR_INIT_CAPACITY 4

//#define PRINT_ENABLE

// #defines for ease of usage
#define VECTOR_INIT(_vec) Vector _vec; vector_init(&_vec)
#define VECTOR_ADD(_vec, _item) vector_add(&_vec, (void*)_item)
#define VECTOR_SET(_vec, _id, _item) vector_set(&_vec, _id, (void*)_item)
#define VECTOR_GET(_vec, _type, _id) (_type) vector_get(&_vec, _id)
#define VECTOR_ERASE(_vec, _id) vector_erase(&_vec, _id)
#define VECTOR_FILL_UP(_vec) vector_fill_up(&_vec)
#define VECTOR_FREE(_vec) vector_free(&_vec)

typedef struct vector {
    void **items;
    int capacity;
    int total;
}Vector;

// Function Prototypes ...
void vector_init(Vector*);
int vector_fill_up(Vector*);
// Visibility of vector_resize only limited to this header file
static void vector_resize(Vector*, int);
void vector_add(Vector*, void*);
void vector_set(Vector*, int, void*);
void *vector_get(Vector*, int);
void vector_erase(Vector*, int);
void vector_free(Vector*);

/* ******* VECTOR BUILDERS ********** */
void vector_init(Vector *v)
{
    // Init params of struct vector
    v->capacity = VECTOR_INIT_CAPACITY;
    v->total = 0;
    v->items = malloc(sizeof(void*) * v->capacity);
}

int vector_fill_up(Vector* v)
{
    return v->total;
}

static void vector_resize(Vector* v, int capacity)
{
#ifdef PRINT_ENABLE
    printf("Vector Resizing: %d to %d\n", v->capacity, capacity);
#endif // PRINT_ENABLE
    void **items = realloc(v->items, sizeof(void*) * capacity);
    // If valid: assign it
    if(items)
    {
        v->items = items;
        v->capacity = capacity;
    }
}

void vector_add(Vector* v, void *item)
{
    // If before addition, reaches Init_capacity, issue resizing
    if(v->capacity == v->total)
        vector_resize(v, v->capacity * 2);
    // Else: Add item to the vector list - Simulate a push_back
    v->items[v->total++] = item;
}

void vector_set(Vector* v, int index, void* item)
{
    // Check index range before re-setting the item param
    if(index >= 0 && index < v->total)
        v->items[index] = item;
    // Else: Debugging, print error
    else {
#ifdef PRINT_ENABLE
        printf("Index out of Range!\n");
#endif // PRINT_ENABLE
    }
}

void* vector_get(Vector* v, int index)
{
    if(index >=0 && index < v->total)
        return v->items[index];
    return NULL;
}

void vector_erase(Vector *v, int index)
{
    // Error Checks
    if(index < 0 || index >= v->total)
        return;
    // If here, valid
    v->items[index] = NULL;

    // Shift everything one unit to the left
    for(int i=0; i< v->total -1 ; i++)
    {
        v->items[i] = v->items[i+1];
        v->items[i+1] = NULL;
    }

    // fill_up count decrements by 1 unit
    v->total--;

    // Adjust the capacity param as well
    if(v->total > 0 && v->total == v->capacity/4)
        // Issue vector resizing..
        vector_resize(v, v->capacity/2);
}

// This function mimics vector clear() operation in C++.
void vector_free(Vector *v)
{
  // free(v->items);

  for(int index = v->total-1; index >=0; index--)
      vector_erase(v, index);

}


#endif //CODINGC_VECTOR_H
