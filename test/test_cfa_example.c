#include <stdio.h>
#include <assert.h>

#include "cfa.h"

int
main(void)
{
    int cfa_err = 1;
    int cfa_id = -1;
    int cfa_id_2 = -2;
    int cfa_lat_id = -1;
    int cfa_lon_id = -1;
    int cfa_time_id = -1;

    /* Container */
    cfa_err = cfa_create("test_path", &cfa_id);
    CFA_ERR(cfa_err);
    cfa_err = cfa_create("test_path2", &cfa_id_2);
    CFA_ERR(cfa_err);

    /* Dimensions */
    cfa_err = cfa_def_dim(cfa_id, "longitude", 360, &cfa_lon_id);
    CFA_ERR(cfa_err);
    cfa_err = cfa_def_dim(cfa_id_2, "lapitude", 180, &cfa_lat_id);
    CFA_ERR(cfa_err);    
    cfa_err = cfa_def_dim(cfa_id_2, "latipude", 180, &cfa_lat_id);
    CFA_ERR(cfa_err);    
    cfa_err = cfa_def_dim(cfa_id, "latitude", 180, &cfa_lat_id);
    CFA_ERR(cfa_err);
    cfa_err = cfa_def_dim(cfa_id, "time", 365, &cfa_time_id);
    CFA_ERR(cfa_err);

    /* get the number of dimensions and the dimension ids */
 
    int ndims = 0;
    cfa_err = cfa_inq_ndims(cfa_id, &ndims);
    CFA_ERR(cfa_err);
    int *dimids = NULL;
    cfa_err = cfa_inq_dim_ids(cfa_id, &dimids);
    CFA_ERR(cfa_err);

    AggregatedDimension *agg_dim = NULL;
    for(int i=0; i<ndims; i++)
    {
        cfa_err = cfa_get_dim(cfa_id, dimids[i], &agg_dim);
        CFA_ERR(cfa_err)
        printf("%i: %i : ", i, agg_dim->len);
        printf("%s\n", agg_dim->name);
    }
    cfa_err = cfa_close(cfa_id);
    CFA_ERR(cfa_err);
    cfa_err = cfa_close(cfa_id_2);
    CFA_ERR(cfa_err);
    cfa_err = cfa_memcheck();
    CFA_ERR(cfa_err);

    return 0;
}