#include <stdio.h>
#include <assert.h>

#include "cfa.h"

const char* test_file_path = "examples/test1.nc";
const char* dim_name = "latitude";
const int len = 16;

void 
test_cfa_def_dim(void)
{
    /* Test the cfa_def_dim method */
    int cfa_dim_id = -1;
    int cfa_id = -1;
    int cfa_err = 1;
    int cfa_dim_n = 0;

    /* first create the AggregationContainer */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* now create the dimension */
    cfa_err = cfa_def_dim(cfa_id, dim_name, len, &cfa_dim_id);
    assert(cfa_err == CFA_NOERR);
    /* check that the id is 1 less than the number of dimensions */
    cfa_err = cfa_inq_ndims(cfa_id, &cfa_dim_n);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_dim_id == cfa_dim_n-1);
    /* close / remove the AggregationContainer */
    cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

void 
test_cfa_inq_dim_id(void)
{
    int cfa_id = -1;
    int cfa_dim_id = -1;
    int cfa_err = 1;
    int cfa_dim_n = -1;

    /* try to get a dimension from a container that has not been created yet */
    cfa_err = cfa_inq_dim_id(cfa_id, dim_name, &cfa_dim_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
     /* create container */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* get a dimension without having created one */
    cfa_err = cfa_inq_dim_id(cfa_id, dim_name, &cfa_dim_id);
    assert(cfa_err == CFA_DIM_NOT_FOUND_ERR);
    /* create dimension */
    cfa_err = cfa_def_dim(cfa_id, dim_name, len, &cfa_dim_id);
    assert(cfa_err == CFA_NOERR);
    /* find an id, by name, that exists - this will be the length of the 
       dimension array - 1 */
    cfa_err = cfa_inq_dim_id(cfa_id, dim_name, &cfa_dim_id);
    cfa_err = cfa_inq_ndims(cfa_id, &cfa_dim_n);
    assert(cfa_dim_id == cfa_dim_n-1);
    /* find an id that doesn't exist */
    cfa_err = cfa_inq_dim_id(cfa_id, "bogus name", &cfa_dim_id);
    assert(cfa_err == CFA_DIM_NOT_FOUND_ERR);
    /* close the AggregationContainer then try to find the dim with the id */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_inq_dim_id(cfa_id, dim_name, &cfa_dim_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

void 
test_cfa_inq_ndims(void)
{
    int cfa_id = -1;
    int cfa_dim_id = -1;
    int cfa_err = 1;
    int cfa_dim_n = -1;

    /* get the number of dimensions before creating the container */
    cfa_err = cfa_inq_ndims(cfa_id, &cfa_dim_n);
    assert(cfa_err == CFA_NOT_FOUND_ERR);   
    /* create the container */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* get the number of dimensions before adding any */
    cfa_err = cfa_inq_ndims(cfa_id, &cfa_dim_n);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_dim_n == 0);
    /* create the dimension */
    cfa_err = cfa_def_dim(cfa_id, dim_name, len, &cfa_dim_id);
    assert(cfa_err == CFA_NOERR);
    /* get the number of dimensions after adding one */
    cfa_err = cfa_inq_ndims(cfa_id, &cfa_dim_n);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_dim_id == cfa_dim_n-1);
    /* close the container, check the memory */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

void 
test_cfa_get_dim(void)
{
    AggregatedDimension *cfa_dim = NULL;
    int cfa_err = 1;
    int cfa_id = -1;
    int cfa_dim_id = -1;

    /* try to get before creating the container */
    cfa_err = cfa_get_dim(cfa_id, cfa_dim_id, &cfa_dim);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* create the container */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* try to get a dimension before creating it */
    cfa_err = cfa_get_dim(cfa_id, 0, &cfa_dim);
    assert(cfa_err == CFA_DIM_NOT_FOUND_ERR);
    /* create the AggregatedDimension */
    cfa_err = cfa_def_dim(cfa_id, dim_name, len, &cfa_dim_id);
    assert(cfa_err == CFA_NOERR);
    /* get the newly created AggregatedDimension */
    cfa_err = cfa_get_dim(cfa_id, cfa_dim_id, &cfa_dim);
    assert(cfa_err == CFA_NOERR);
    /* get a none-existent AggregatedDimension in an existing cfa_id*/
    cfa_err = cfa_get_dim(cfa_id, cfa_dim_id+1, &cfa_dim);
    assert(cfa_err == CFA_DIM_NOT_FOUND_ERR);
    /* get a none-existent AggregatedDimension in a none-existing cfa_id*/
    cfa_err = cfa_get_dim(cfa_id+1, cfa_dim_id+1, &cfa_dim);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* close the AggregationContainer, then try to get the AggregatedDimension
    */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_get_dim(cfa_id, cfa_dim_id, &cfa_dim);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

int 
main(void)
{
    test_cfa_def_dim();
    test_cfa_inq_dim_id();
    test_cfa_inq_ndims();
    test_cfa_get_dim();
    return 0;
}