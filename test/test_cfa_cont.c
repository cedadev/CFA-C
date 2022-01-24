#include <stdio.h>
#include <assert.h>

#include "cfa.h"
#include "cfa_mem.h"

const char* test_file_path = "level 1";
extern DynamicArray *cfa_conts;

void
test_cfa_cont_create(void)
{
    int cfa_id = -1;
    int cfa_err = 1;
    int cfa_cont_id1 = -1;
    int cfa_cont_id2 = -1;
    int n_conts = 0;
    /* create a parent (file) container */
    cfa_err = cfa_create(test_file_path, &cfa_id);
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
}

int
main(void)
{
    /* run the unit tests */
    test_cfa_cont_create();
}