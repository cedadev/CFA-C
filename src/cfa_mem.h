#ifndef __CFA_MEM__
#define __CFA_MEM__

typedef struct DynamicArray_t DynamicArray;

/* cfa memory functions to keep track of memory allocations and detect leaks */
void*  cfa_malloc(const size_t size);
void*  cfa_realloc(void *ptr, const size_t old_size, const size_t new_size);
void   cfa_free(void *ptr, const size_t size);
int    cfa_memcheck(void);

/* dynamic array functions */
int create_array(DynamicArray **array, size_t typesize);
int create_array_node(DynamicArray **array, void **ptr);
int get_array_node(DynamicArray **array, int node, void **ptr);
int free_array(DynamicArray **array);
int get_array_length(DynamicArray **array, int* n_nodes);

int allocate_array(void **ptr, int csize, int typesize);

#endif