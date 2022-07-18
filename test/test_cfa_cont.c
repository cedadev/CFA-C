#include <stdio.h>
#include <assert.h>

#include "cfa.h"
#include "cfa_mem.h"

const char* test_file_path = "level 1";
const char* cont_name = "container";
extern DynamicArray *cfa_conts;

void
test_cfa_def_cont(void)
{
    int cfa_id = -1;
    int cfa_err = 1;
    int cfa_cont_id1 = -1;
    int cfa_cont_id2 = -1;
    int n_conts = 0;
    /* create a parent (file) container */
    cfa_err = cfa_create(test_file_path, CFA_NETCDF, &cfa_id);
    assert(cfa_err == CFA_NOERR);

    /* create a container in the parent (file) container */
    cfa_err = cfa_def_cont(cfa_id, "level 2", &cfa_cont_id1);
    assert(cfa_err == CFA_NOERR);
    /* check that the id is the length of the cfa_conts array (-1)*/
    cfa_err = get_array_length(&cfa_conts, &n_conts);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_cont_id1 == n_conts-1);

    /* now create a container in that container (a third level) */
    cfa_err = cfa_def_cont(cfa_cont_id1, "level 3", &cfa_cont_id2);
    assert(cfa_err == CFA_NOERR);
    /* check that the id is the length of the cfa_conts array (-1)*/
    cfa_err = get_array_length(&cfa_conts, &n_conts);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_cont_id2 == n_conts-1);

    /* close the AggregationContainer */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
    printf("Completed test_cfa_cont_create\n");
}

void 
test_cfa_inq_cont_id(void)
{
    int cfa_id = -1;
    int cfa_cont_id = -1;
    int cfa_err = 1;
    int cfa_cont_n = -1;

    /* try to get a container that has not been created yet */
    cfa_err = cfa_inq_cont_id(cfa_id, cont_name, &cfa_cont_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
     /* create container */
    cfa_err = cfa_create(test_file_path, CFA_NETCDF, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* get a container without having created one */
    cfa_err = cfa_inq_cont_id(cfa_id, cont_name, &cfa_cont_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* create container */
    /* create a container in the parent (file) container */
    cfa_err = cfa_def_cont(cfa_id, cont_name, &cfa_cont_id);
    assert(cfa_err == CFA_NOERR);
    /* find an id, by name, that exists - this will be the length of the 
       container array - 1 */
    cfa_err = cfa_inq_cont_id(cfa_id, cont_name, &cfa_cont_id);
    cfa_err = get_array_length(&cfa_conts, &cfa_cont_n);
    assert(cfa_cont_id == cfa_cont_n-1);
    /* find an id that doesn't exist */
    cfa_err = cfa_inq_cont_id(cfa_id, "bogus name", &cfa_cont_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* close the AggregationContainer then try to find the container with the 
    id */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_inq_cont_id(cfa_id, cont_name, &cfa_cont_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
    printf("Completed test_cfa_inq_cont_id\n");
}

void 
test_cfa_inq_nconts(void)
{
    int cfa_id = -1;
    int cfa_cont_id = -1;
    int cfa_err = 1;
    int cfa_cont_n = -1;

    /* get the number of containers before creating the container */
    cfa_err = cfa_inq_nconts(cfa_id, &cfa_cont_n);
    assert(cfa_err == CFA_NOT_FOUND_ERR);   
    /* create the container */
    cfa_err = cfa_create(test_file_path, CFA_NETCDF, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* get the number of containers before adding any */
    cfa_err = cfa_inq_nconts(cfa_id, &cfa_cont_n);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_cont_n == 0);
    /* create the container */
    cfa_err = cfa_def_cont(cfa_id, cont_name, &cfa_cont_id);
    assert(cfa_err == CFA_NOERR);
    /* get the number of containers after adding one */
    cfa_err = get_array_length(&cfa_conts, &cfa_cont_n);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_cont_id == cfa_cont_n-1);
    /* close the container, check the memory */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
    printf("Completed test_cfa_inq_nconts\n");
}

void 
test_cfa_get_cont(void)
{
    AggregationContainer *cfa_cont = NULL;
    int cfa_err = 1;
    int cfa_id = -1;
    int cfa_cont_id = -1;

    /* try to get a container before creating the parent container */
    cfa_err = cfa_get_cont(cfa_id, cfa_cont_id, &cfa_cont);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* create the container */
    cfa_err = cfa_create(test_file_path, CFA_NETCDF, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* try to get a container before creating it (id 0 is used by parent 
    container */
    cfa_err = cfa_get_cont(cfa_id, 1, &cfa_cont);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* create the container */
    cfa_err = cfa_def_cont(cfa_id, cont_name, &cfa_cont_id);
    assert(cfa_err == CFA_NOERR);
    /* get the newly created AggregationContainer */
    cfa_err = cfa_get_cont(cfa_id, cfa_cont_id, &cfa_cont);
    assert(cfa_err == CFA_NOERR);
    /* get a none-existent AggregationContainer in an existing cfa_id */
    cfa_err = cfa_get_cont(cfa_id, cfa_cont_id+1, &cfa_cont);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* get a none-existent AggregationContainer in a none-existing cfa_id*/
    cfa_err = cfa_get_cont(cfa_id+1, cfa_cont_id+1, &cfa_cont);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* close the parent AggregationContainer, then try to get the 
    child AggregationContainer */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_get_cont(cfa_id, cfa_cont_id, &cfa_cont);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
    printf("Completed test_cfa_get_cont\n");
}

int
main(void)
{
    /* run the unit tests */
    test_cfa_def_cont();
    test_cfa_inq_cont_id();
    test_cfa_inq_nconts();
    test_cfa_get_cont();
}