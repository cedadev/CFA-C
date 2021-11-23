#include <stdio.h>
#include <assert.h>

#include "cfa.h"

const char* test_file_path = "examples/test1.nc";
const char* var_name = "tas";

int
create_variable(int cfa_file_id)
{
    int cfa_lat_id = -1;
    int cfa_lon_id = -1;
    int cfa_t_id = -1;
    int cfa_err = 1;
    int cfa_var_id = -1;

    /* now create the dimensions */
    cfa_err = cfa_def_dim(cfa_file_id, "latitude", 16, &cfa_lat_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_def_dim(cfa_file_id, "longitude", 16, &cfa_lon_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_def_dim(cfa_file_id, "time", 32, &cfa_t_id);
    assert(cfa_err == CFA_NOERR);
    /* now create the variable */
    int dim_ids[3] = {cfa_t_id, cfa_lat_id, cfa_lon_id};
    cfa_err = cfa_def_var(cfa_file_id, "tas", 3, dim_ids, &cfa_var_id);
    assert(cfa_err == CFA_NOERR);
    return cfa_var_id;
}

void 
test_cfa_def_var(void)
{
    /* Test the cfa_def_var method */
    int cfa_file_id = -1;
    int cfa_err = 1;
    int cfa_tas_id = -1;

    /* first create the AggregationContainer */
    cfa_err = cfa_create(test_file_path, &cfa_file_id);
    assert(cfa_err == CFA_NOERR);
    /* attempt to create a variable before creating any dimensions */
    int dim_ids_1[1] = {0};
    cfa_err = cfa_def_var(cfa_file_id, var_name, 1, dim_ids_1, &cfa_tas_id);
    assert(cfa_err == CFA_DIM_NOT_FOUND_ERR);
    /* create the variable */
    cfa_tas_id = create_variable(cfa_file_id);
    /* close the AggregationContainer */
    cfa_err = cfa_close(cfa_file_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

void 
test_cfa_inq_var_id(void)
{
    int cfa_id = -1;
    int cfa_var_id = -1;
    int cfa_err = 1;
    int cfa_var_n = -1;

    /* try to get a variable from a container that has not been created yet */
    cfa_err = cfa_inq_var_id(cfa_id, var_name, &cfa_var_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
     /* create container */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* get a variable id without having created one */
    cfa_err = cfa_inq_var_id(cfa_id, var_name, &cfa_var_id);
    assert(cfa_err == CFA_VAR_NOT_FOUND_ERR);
    /* create the variable */
    cfa_var_id = create_variable(cfa_id);
    /* find an id, by name, that exists - this will be the length of the
       variable array -1 */
    cfa_err = cfa_inq_var_id(cfa_id, var_name, &cfa_var_id);
    cfa_err = cfa_inq_nvars(cfa_id, &cfa_var_n);
    assert(cfa_var_id == cfa_var_n-1);
    /* find an id that doesn't exist */
    cfa_err = cfa_inq_var_id(cfa_id, "bogus name", &cfa_var_id);
    assert(cfa_err == CFA_VAR_NOT_FOUND_ERR);
    /* close the AggregationContainer then try to find the var with the id */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_inq_var_id(cfa_id, var_name, &cfa_var_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

void 
test_cfa_inq_nvars(void)
{
    int cfa_id = -1;
    int cfa_var_id = -1;
    int cfa_err = 1;
    int cfa_var_n = -1;

    /* get the number of variables before creating the container */
    cfa_err = cfa_inq_nvars(cfa_id, &cfa_var_n);
    assert(cfa_err == CFA_NOT_FOUND_ERR);   
    /* create the container */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* get the number of variables before adding any */
    cfa_err = cfa_inq_nvars(cfa_id, &cfa_var_n);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_var_n == 0);
    /* create the variable */
    cfa_var_id = create_variable(cfa_id);
    /* get the number of variables after adding one */
    cfa_err = cfa_inq_nvars(cfa_id, &cfa_var_n);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_var_id == cfa_var_n-1);
    /* close the container, check the memory */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

void 
test_cfa_get_var(void)
{
    AggregationVariable *cfa_var = NULL;
    int cfa_err = 1;
    int cfa_id = -1;
    int cfa_var_id = -1;

    /* try to get before creating the container */
    cfa_err = cfa_get_var(cfa_id, cfa_var_id, &cfa_var);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* create the container */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* try to get a variable before creating it */
    cfa_err = cfa_get_var(cfa_id, 0, &cfa_var);
    assert(cfa_err == CFA_VAR_NOT_FOUND_ERR);
    /* create the AggregationVariable */
    cfa_var_id = create_variable(cfa_id);
    /* get the newly created AggregationVariable */
    cfa_err = cfa_get_var(cfa_id, cfa_var_id, &cfa_var);
    assert(cfa_err == CFA_NOERR);
    /* get a none-existent AggregationVariable in an existing cfa_id*/
    cfa_err = cfa_get_var(cfa_id, cfa_var_id+1, &cfa_var);
    assert(cfa_err == CFA_VAR_NOT_FOUND_ERR);
    /* get a none-existent AggregationVariable in a none-existing cfa_id*/
    cfa_err = cfa_get_var(cfa_id+1, cfa_var_id+1, &cfa_var);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* close the AggregationContainer, then try to get the AggregationVariable
    */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_get_var(cfa_id, cfa_var_id, &cfa_var);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

int
main(void)
{
    test_cfa_def_var();
    test_cfa_inq_var_id();
    test_cfa_inq_nvars();
}