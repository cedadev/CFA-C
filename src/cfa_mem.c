#include <stdlib.h>
#include <stdio.h>

#include "cfa.h"
#include "cfa_mem.h"

/* 
resizeable array functions
*/

/* resizeable array structure */
typedef struct DynamicArray_t
{
    void* array;
    int size;
    int used;
    size_t typesize;
} DynamicArray;

/*
default dynamic array size
*/
const int DARRAY_SIZE=16;

/*
static count of used memory
*/
static size_t cfa_used_mem=0;

/*
functions to allocate memory and add to the cfa_used_mem
*/

void*
cfa_malloc(const size_t size)
{
    /* allocate memory and keep a record */
    void* ptr = malloc(size);

#ifdef _DEBUG
    if (ptr)
        cfa_used_mem += size;
#endif
    return ptr;
}

void*
cfa_realloc(void *ptr, const size_t old_size, const size_t new_size)
{
    void* tmp_mem = (void*) realloc(ptr, new_size);
#ifdef _DEBUG
    if (tmp_mem)
        /* realloc adds the difference between the current size and the 
        requested (re)size */
        cfa_used_mem += new_size - old_size;
#endif
    return tmp_mem;
}

void
cfa_free(void *ptr, const size_t size)
{
    free(ptr);
#ifdef _DEBUG
    ptr = NULL;
    cfa_used_mem -= size;
#endif
}

int
cfa_memcheck(void)
{
    /* 
    check for a memory leak.  call this at clean-up to check that cfa_used_mem=0
    */
    if(cfa_used_mem != 0)
        return CFA_MEM_LEAK;
    return CFA_NOERR;
}

/*
create the array, with a max size (before realloc) of DARRAY_SIZE
*/
int
create_array(DynamicArray **array, size_t typesize)
{
    *array = cfa_malloc(sizeof(DynamicArray));

    if (!(*array))
        return CFA_MEM_ERR;
    (*array)->size = DARRAY_SIZE;
    (*array)->used = 0;
    (*array)->typesize = typesize;
    (*array)->array = cfa_malloc((*array)->size * (*array)->typesize);

    if (!((*array)->array))
        return CFA_MEM_ERR;

    return CFA_NOERR;
}

/*
create and get a pointer to an array element - if this is out of bounds then 
create some more array elements
*/
int
create_array_node(DynamicArray **array, void** ptr)
{
    if (!(*array))
        return CFA_MEM_ERR;
    /* resize array if run out of elements */
    if ((*array)->used >= (*array)->size)
    {
        /* reallocate / resize the memory */
        int new_size = ((*array)->size + DARRAY_SIZE) * (*array)->typesize;
        int old_size = (*array)->used * (*array)->typesize;
        void* tmp_mem = cfa_realloc((*array)->array, old_size, new_size);
        if (!tmp_mem) 
            return CFA_MEM_ERR;
        (*array)->array = tmp_mem;
        (*array)->size  += DARRAY_SIZE;
    }
    /* get the last used element in the array and return it */
    *ptr = (*array)->array + ((*array)->used) * (*array)->typesize;
    /* increment number of used array elements */
    (*array)->used += 1;
    return CFA_NOERR;
}

int get_array_node(DynamicArray **array, int node, void** ptr)
{
#ifdef _DEBUG
    /* bounds check in debug mode */
    if (node >= (*array)->used)
        return CFA_BOUNDS_ERR;
#endif
    if (!(*array))
        return CFA_MEM_ERR;
    /* to get the node we have to cast the array->array to char* and add the
    node * array->typesize */
    *ptr = (*array)->array + node * (*array)->typesize;
    return CFA_NOERR;
}

int get_array_length(DynamicArray **array, int* n_nodes)
{
    /* get the length of the array = the number of used nodes */
    if (!(*array))
        return CFA_MEM_ERR;
    if (!(*array)->array)
        return CFA_MEM_ERR;

    *n_nodes = (*array)->used;

    return CFA_NOERR;
}

int free_array(DynamicArray **array)
{
    /* free the array */
    if (!(*array))
        return CFA_MEM_ERR;
    if (!(*array)->array)
        return CFA_MEM_ERR;
    cfa_free((*array)->array, (*array)->size * (*array)->typesize);
    cfa_free(*array, sizeof(DynamicArray));

    /* set pointer to NULL to indicate it has been freed*/
    *array = NULL;
    return CFA_NOERR;
}

/*
allocate memory in a resizeable array
*/
int
allocate_array(void** ptr, int csize, int typesize)
{
    if (!(*ptr))
    {
        *ptr = (void*) malloc(typesize); /* allocate 1 */
        if (!(*ptr))
            return CFA_MEM_ERR;
    }
    else
    {
        void* tmp_mem = (void*) realloc(*ptr, (csize+1)*typesize);
        if (!tmp_mem)
            return CFA_MEM_ERR;
        *ptr = tmp_mem;
    }
    return CFA_NOERR;
}
