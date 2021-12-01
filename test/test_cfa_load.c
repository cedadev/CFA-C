#include <stdio.h>
#include <assert.h>

#include "cfa.h"

/* note that the examples/Makefile has to be run before running this test */
const char* test_file_path = "examples/example6.nc";

void 
test_cfa_load(void)
{
    /* Test the cfa_load method */
    int cfa_id = -1;
    int cfa_err = 1;
    int n_conts = -1;
    cfa_err = cfa_load(test_file_path, CFA_NETCDF, &cfa_id);
    assert(cfa_err == CFA_NOERR);

    cfa_close(cfa_id);
    cfa_err = cfa_memcheck();
    assert(cfa_err == CFA_NOERR);
}

int 
main(void)
{
    /* run the unit tests */
    printf("%s\n", test_file_path);
    test_cfa_load();

    return 0;
}