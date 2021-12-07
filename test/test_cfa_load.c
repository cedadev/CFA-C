#include <stdio.h>
#include <assert.h>

#include "cfa.h"

/* note that the examples/Makefile has to be run before running this test */
const char* test_file_path = "examples/example7.nc";

int
output(const int cfa_id)
{
    AggregationContainer *agg_cont;
    int err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(err);
    printf("CFA Aggregation Container\n");
    printf("=================================\n");
    printf("Path: %s\n", agg_cont->path);
    printf("---------------------------------\n");
    printf("Dimensions: \n");
    int cfa_ndims;
    err = cfa_inq_ndims(cfa_id, &cfa_ndims);
    CFA_CHECK(err);
    AggregatedDimension *agg_dim;
    for (int d=0; d<cfa_ndims; d++)
    {
        err = cfa_get_dim(cfa_id, d, &agg_dim);
        CFA_CHECK(err);
        printf("\t%-20s %4i\n", agg_dim->name, agg_dim->len);
    }
    printf("---------------------------------\n");
    printf("Variables: \n");
    int cfa_nvars;
    err = cfa_inq_nvars(cfa_id, &cfa_nvars);
    CFA_CHECK(err);
    AggregationVariable *agg_var;
    for (int v=0; v<cfa_nvars; v++)
    {
        err = cfa_get_var(cfa_id, v, &agg_var);
        CFA_CHECK(err);
        printf("\t%-23s (\n", agg_var->name);
        /* print the attached dimensions */
        for (int d=0; d<agg_var->cfa_ndim; d++)
        {
            err = cfa_get_dim(cfa_id, agg_var->cfa_dim_idp[d], &agg_dim);
            CFA_CHECK(err);
            printf("\t\t%-16s\n", agg_dim->name);
        }
        printf("\t)\n");
        /* print the aggregation instructions */
        printf("\tFile    : %s\n", agg_var->cfa_instructionsp->file);
        printf("\tFormat  : %s\n", agg_var->cfa_instructionsp->format);
        printf("\tAddress : %s\n", agg_var->cfa_instructionsp->address);
        printf("\tLocation: %s\n", agg_var->cfa_instructionsp->location);
        printf("        -------------------------\n");
   }

    return CFA_NOERR;
}

void 
test_cfa_load(void)
{
    /* Test the cfa_load method */
    int cfa_id = -1;
    int cfa_err = 1;
    int n_conts = -1;
    cfa_err = cfa_load(test_file_path, CFA_NETCDF, &cfa_id);
    assert(cfa_err == CFA_NOERR);
    cfa_err = output(cfa_id);
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