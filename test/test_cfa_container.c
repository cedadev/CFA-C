#include <stdio.h>
#include <assert.h>

#include "cfa.h"

const char* test_file_path = "examples/test1.nc";

void 
test_cfa_create(void)
{
    /* Test the cfa_create method */
    int cfa_id = -1;
    int cfa_err = 1;
    int n_conts = -1;
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_inq_n(&n_conts);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_id == n_conts-1);
    cfa_close(cfa_id);
}

void 
test_cfa_close()
{
    int cfa_err = 1;
    int cfa_id = 0;
    /* create */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* close */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    /* try to close again */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
}

void 
test_cfa_inq_id(void)
{
    int cfa_id = -1;
    int cfa_err = 1;
    int n_conts = -1;
    /* create */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_inq_n(&n_conts);
    assert(cfa_err == CFA_NOERR);
    /* find an id that exists */
    cfa_err = cfa_inq_id(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_id == n_conts-1); 
    /* find an id that doesn't exist */
    cfa_err = cfa_inq_id("bogus_path", &cfa_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* close the AggregationContainer then try to find the file with the id */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_inq_id(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
}

void 
test_cfa_get(void)
{
    AggregationContainer* cfa_cont = NULL;
    int cfa_err = 1;
    int cfa_id = -1;
    int n_conts = -1;
    /* create */
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_inq_n(&n_conts);
    assert(cfa_err == CFA_NOERR);
    assert(cfa_id = n_conts-1);
    /* get the newly created AggregationContainer */
    cfa_err = cfa_get(cfa_id, &cfa_cont);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_get(cfa_id+1, &cfa_cont);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
    /* close, then try to get the AggregationContainer */
    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_get(cfa_id, &cfa_cont);
    assert(cfa_err == CFA_NOT_FOUND_ERR);
}

void 
test_cfa_inq_n(void)
{
    int n_files = -1;
    int cfa_id = -1;
    int cfa_err = 1;
    int n_conts = -1;

    // get the current number of files 
    cfa_err = cfa_inq_n(&n_files);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_create(test_file_path, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_inq_n(&n_conts);
    assert(cfa_err == CFA_NOERR);
    assert(n_conts == n_files+1);

    cfa_err = cfa_close(cfa_id);
    assert(cfa_err == CFA_NOERR);

    /* this looks strange, but it's because closed files  are not deleted from
       the array for performance and ease-of-implementation reasons */
    cfa_err = cfa_inq_n(&n_conts);
    assert(n_conts == n_files+1); 
}

int 
main(void)
{
    /* run the unit tests */
    test_cfa_create();
    test_cfa_close();
    test_cfa_inq_id();
    test_cfa_get();
    test_cfa_inq_n();

    return 0;
}