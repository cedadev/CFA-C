#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#include "cfa.h"
#include "cfa_mem.h"

#ifndef _DEBUG
#define _DEBUG
#endif

void
test_cfa_malloc(void)
{
    int *intmem = cfa_malloc(16 * sizeof(int));
    assert(intmem != NULL);
    cfa_free(intmem, 16 * sizeof(int));
    int mcheck = cfa_memcheck();
    assert(mcheck == CFA_NOERR);
}

void
test_cfa_realloc(void)
{
    /* test allocating then reallocating memory */
    int *intmem = cfa_malloc(16 * sizeof(int));
    assert(intmem != NULL);
    int *newmem = cfa_realloc(intmem, 16 * sizeof(int), 32 * sizeof(int));
    assert(newmem != NULL);
    intmem = newmem;
    cfa_free(intmem, 32 * sizeof(int));
    int mcheck = cfa_memcheck();
    assert(mcheck == CFA_NOERR);
}

void
test_cfa_free(void)
{
    /* just run the test_cfa_malloc test again as this also invokes free */
    test_cfa_malloc();
}

void
test_cfa_memcheck(void)
{
    /* test two cases : 1. memory not freed, 2. memory freed */
    int *intmem = cfa_malloc(16 * sizeof(int));
    assert(intmem != NULL);
    int mcheck = cfa_memcheck();
    assert(mcheck == CFA_MEM_LEAK);
    cfa_free(intmem, 16 * sizeof(int));
    mcheck = cfa_memcheck();
    assert(mcheck == CFA_NOERR);
}

void
test_create_array(void)
{
    DynamicArray* darray;
    int cfa_err = create_array(&darray, sizeof(int));
    assert(cfa_err == CFA_NOERR);
    free_array(&darray);
    int mcheck = cfa_memcheck();
    assert(mcheck == CFA_NOERR);
}

void
test_create_array_node(void)
{
    DynamicArray* darray;
    int cfa_err = create_array(&darray, sizeof(int));
    assert(cfa_err == CFA_NOERR);
    int* node_ptr;
    cfa_err = create_array_node(&darray, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);
    free_array(&darray);
    int mcheck = cfa_memcheck();
    assert(mcheck == CFA_NOERR);
}

void
test_get_array_node(void)
{
    DynamicArray* darray;
    int cfa_err = create_array(&darray, sizeof(int));
    assert(cfa_err == CFA_NOERR);
    /* test getting a node on an empty array */
    int* node_ptr;
    cfa_err = get_array_node(&darray, 0, (void**)(&node_ptr));
    assert(cfa_err == CFA_BOUNDS_ERR);
    /* test getting a node after it has been created */
    cfa_err = create_array_node(&darray, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);
    cfa_err = get_array_node(&darray, 0, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);
    /* assign a value to recover later */
    *node_ptr = 808;
    /* create another node and assign a different value*/
    cfa_err = create_array_node(&darray, (void**)(&node_ptr));
    *node_ptr = 909;
    assert(cfa_err == CFA_NOERR);
    /* get the value of the previously created node and check that it matches */
    int* node_ptr_2;
    cfa_err = get_array_node(&darray, 0, (void**)(&node_ptr_2));
    assert(cfa_err == CFA_NOERR);   
    assert(*node_ptr_2 == 808);
    /* now get a node out of bounds */
    cfa_err = get_array_node(&darray, 10, (void**)(&node_ptr));
    assert(cfa_err == CFA_BOUNDS_ERR);
    free_array(&darray);
    int mcheck = cfa_memcheck();
    assert(mcheck == CFA_NOERR);
}

void 
test_free_array(void)
{
    DynamicArray* darray0;
    DynamicArray* darray1;
    DynamicArray* darray2;

    /* correct allocate / free */
    int cfa_err = create_array(&darray0, sizeof(int));
    assert(cfa_err == CFA_NOERR);
    free_array(&darray0);
    int mcheck = cfa_memcheck();
    assert(mcheck == CFA_NOERR);

    /* no free */
    cfa_err = create_array(&darray0, sizeof(int));
    assert(cfa_err == CFA_NOERR);
    mcheck = cfa_memcheck();
    assert(mcheck == CFA_MEM_LEAK);
    free_array(&darray0);

    /* create lots and free at the end */
    int* node_ptr;
    cfa_err = create_array(&darray0, sizeof(int));
    assert(cfa_err == CFA_NOERR);
    cfa_err = create_array_node(&darray0, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);

    cfa_err = create_array(&darray1, sizeof(int));
    assert(cfa_err == CFA_NOERR);
    cfa_err = create_array_node(&darray1, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);
    cfa_err = create_array_node(&darray1, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);    

    cfa_err = create_array(&darray2, sizeof(int));
    assert(cfa_err == CFA_NOERR);
    cfa_err = create_array_node(&darray2, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);
    cfa_err = create_array_node(&darray2, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);
    cfa_err = create_array_node(&darray2, (void**)(&node_ptr));
    assert(cfa_err == CFA_NOERR);

    free_array(&darray0);
    free_array(&darray1);
    free_array(&darray2);

    mcheck = cfa_memcheck();
    assert(mcheck == CFA_NOERR);    
}

int
main(void)
{
    test_cfa_malloc();
    test_cfa_realloc();
    test_cfa_free();
    test_cfa_memcheck();

    test_create_array();
    test_create_array_node();
    test_get_array_node();
    test_free_array();

    return 0;
}