#include <stdlib.h>
#include <string.h>

#include "cfa.h"

/*
create an AggregatedDimension, attach it to a cfa_id
*/
int 
cfa_def_dim(const int cfa_id, const char *name, const int len, int *cfa_dim_idp)
{
    /* get the AggregationContainer */
    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;
    
    /* check whether the AggregationContainer has Dimensions already allocated*/
    if (!(agg_cont->cfa_dimp))
    {
        /* create the first AggregatedDimension if none have been created so far
        */
        agg_cont->cfa_dimp = (AggregatedDimension*) malloc(
            sizeof(AggregatedDimension)
        );
        if (!(agg_cont->cfa_dimp))
            return CFA_MEM_ERR;
    }
    else
    {
        /* resize the array to hold one more AggregatedDimension */
        AggregatedDimension* tmp_mem = (AggregatedDimension*) realloc(
            agg_cont->cfa_dimp, 
            (agg_cont->cfa_ndim+1) * sizeof(AggregatedDimension)
        );
        if (!tmp_mem)
            return CFA_MEM_ERR;
        agg_cont->cfa_dimp = tmp_mem;
    }
    /* get a pointer to the newly create dimension so we can assign items */
    AggregatedDimension* new_dimp = &(agg_cont->cfa_dimp[agg_cont->cfa_ndim]);
    /* assign the name and length */
    new_dimp->name = (char*) malloc(sizeof(char) * strlen(name));
    if (!(new_dimp->name))
        return CFA_MEM_ERR;
    strcpy(new_dimp->name, name);
    new_dimp->len = len;

    /* assign to the AggregationContainer */
    agg_cont->cfa_ndim++;

    /* allocate then iterate the AggregatedDimension identifier */
    *cfa_dim_idp = agg_cont->cfa_ndim-1;
    return CFA_NOERR;
}

/*
get the identifier of an AggregatedDimension
*/
int
cfa_inq_dim_id(const int cfa_id, const char* name, int *cfa_dim_idp)
{
    /* get the AggregationContainer */
    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;

    /* search through the dimensions, looking for the matching name */
    for (int i=0; i<agg_cont->cfa_ndim; i++)
    {
        /* dimensions that belong to a closed AggregationContainer have their
        name set to NULL */
        if (!(agg_cont->cfa_dimp[i].name))
            continue;
        if (strcmp(agg_cont->cfa_dimp[i].name, name) == 0)
        {
            /* found, so assign and return */
            *cfa_dim_idp = i;
            return CFA_NOERR;
        }
    }
    return CFA_DIM_NOT_FOUND_ERR;
}

/*
get the number of AggregatedDimensions defined
*/
int
cfa_inq_ndims(const int cfa_id)
{
    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;
    return agg_cont->cfa_ndim;
}

/* 
get the AggregatedDimension from a cfa_dim_id
*/
int cfa_get_dim(const int cfa_id, const int cfa_dim_id, 
                AggregatedDimension **agg_dim)
{
    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;

    /* check id is in range */
    if (cfa_dim_id < 0 || cfa_dim_id >= agg_cont->cfa_ndim)
        return CFA_DIM_NOT_FOUND_ERR;

    /* check that the path is not NULL.  On cfa_close, the path is set to NULL 
    */
    if (!(agg_cont->cfa_dimp[cfa_dim_id].name))
        return CFA_DIM_NOT_FOUND_ERR;

    /* assign return value */
    *agg_dim = &(agg_cont->cfa_dimp[cfa_dim_id]);
    return CFA_NOERR;
}