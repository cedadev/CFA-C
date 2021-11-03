#include <stdio.h>
#include <assert.h>

#include "cfa.h"

const char* test_file_path = "examples/test1.nc";

void 
test_cfa_def_var(void)
{
    /* Test the cfa_def_dim method */
    int cfa_lat_id = -1;
    int cfa_lon_id = -1;
    int cfa_t_id = -1;
    int cfa_file_id = -1;
    int cfa_err = 1;
    int cfa_tas_id = -1;

    /* first create the AggregationContainer */
    cfa_err = cfa_create(test_file_path, &cfa_file_id);
    assert(cfa_err == CFA_NOERR);

    /* now create the dimensions */
    cfa_err = cfa_def_dim(cfa_file_id, "latitude", 16, &cfa_lat_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_def_dim(cfa_file_id, "longitude", 16, &cfa_lon_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = cfa_def_dim(cfa_file_id, "time", 32, &cfa_t_id);
    assert(cfa_err == CFA_NOERR);

    /* now create the variable */
    int dim_ids[3] = {cfa_t_id, cfa_lat_id, cfa_lon_id};
    cfa_err = cfa_def_var(cfa_file_id, "tas", 3, dim_ids, &cfa_tas_id);

    /* close the AggregationContainer */
    cfa_err = cfa_close(cfa_file_id);
    assert(cfa_err == CFA_NOERR);
}

int
main(void)
{
    test_cfa_def_var();
}