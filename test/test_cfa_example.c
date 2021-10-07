#include <stdio.h>
#include <assert.h>

#include "cfa.h"

#define CFA_ERR(cfa_err) if(cfa_err) {printf("CFA error: %i\n", cfa_err); return cfa_err;}

int main(void)
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
    cfa_err = cfa_create("test_path", &cfa_id_2);
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

    AggregationContainer *agg_cont = NULL;
    cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_ERR(cfa_err);
    for(int i=0; i<agg_cont->cfa_ndim; i++)
    {
        printf("%i:%i\n", i, agg_cont->cfa_dim_idp[i]);
        AggregatedDimension* agg_d = NULL;
        cfa_err = cfa_get_dim(agg_cont->cfa_dim_idp[i], &agg_d);
        printf("%s\n", agg_d->name);
    }


    return 0;
}