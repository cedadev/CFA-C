#include <stdlib.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"

/* 
create a AggregationVariable container, attach it to a AggregationContainer and one or more AggregatedDimension(s) and assign it to a cfa_var_id
*/
int
cfa_def_var(int cfa_id, const char *name, int ndims, int *cfa_dim_idsp, 
            int *cfa_var_idp)
{
    /* get the aggregation container */
    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;
    /*
    if no variables have been defined previously, then the agg_cont->cfa_varp 
    pointer will be NULL
    */
    if (!(agg_cont->cfa_varp))
    {
        cfa_err = create_array(&(agg_cont->cfa_varp),
                    sizeof(AggregationVariable));
        if (cfa_err)
            return cfa_err;
    }
    /* Array is create so now allocate and return the array node */
    AggregationVariable *var_node = NULL;
    cfa_err = create_array_node(&(agg_cont->cfa_varp), (void**)(&var_node));
    if (cfa_err)
        return cfa_err;

    /* assign the name */
    var_node->name = (char*) cfa_malloc(sizeof(char) * strlen(name));
    if (!(var_node->name))
        return CFA_MEM_ERR;
    strcpy(var_node->name, name);

    /* check that the dimension ids have been added to the container already.
       this basically means that the dim_id(s) passed in via the cfa_dim_idsp 
       pointer are greater than 0 and less than the number of dimensions */
    
    int n_cfa_dims = -1;
    cfa_err = cfa_inq_ndims(cfa_id, &n_cfa_dims);
    if (cfa_err)
        return cfa_err;
    for (int i=0; i<ndims; i++)
    {
        if (cfa_dim_idsp[i] < 0 || cfa_dim_idsp[i] >= n_cfa_dims)
            return CFA_DIM_NOT_FOUND_ERR;
    }

    /* assign the number of dimensions and copy the dimension array */
    var_node->cfa_ndim = ndims;
    var_node->cfa_dim_idp = (int*) cfa_malloc(sizeof(int) * ndims);
    if (!(var_node->cfa_dim_idp))
        return CFA_MEM_ERR;
    memcpy(var_node->cfa_dim_idp, cfa_dim_idsp, sizeof(int) * ndims);

    /* allocate the AggregationVariable identifier */
    int cfa_nvar = 0;
    cfa_err = get_array_length(&(agg_cont->cfa_varp), &cfa_nvar);
    if (cfa_err)
        return cfa_err;
    *cfa_var_idp = cfa_nvar - 1;

    return CFA_NOERR;
}

/*
get the identifier of an AggregationVariable by name
*/
int
cfa_inq_var_id(const int cfa_id, const char* name, int *cfa_var_idp)
{
    /* get the AggregationContainer with cfa_id */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;

    /* search through the variables, looking for the matching name */
    int cfa_nvar = 0;
    cfa_err = get_array_length(&(agg_cont->cfa_varp), &cfa_nvar);
    if (cfa_err)
    {
        if (cfa_err == CFA_MEM_ERR)
            return CFA_VAR_NOT_FOUND_ERR;
        else
            return cfa_err;
    }

    AggregationVariable *cvar = NULL;
    for (int i=0; i<cfa_nvar; i++)
    {
        /* variables that belong to a closed AggregationContainer have their
        name set to NULL */
        cfa_err = get_array_node(&(agg_cont->cfa_varp), i, (void**)(&cvar));
        if (cfa_err)
            return cfa_err;
        if (!(cvar->name))
            continue;
        if (strcmp(cvar->name, name) == 0)
        {
            /* found, so assign and return */
            *cfa_var_idp = i;
            return CFA_NOERR;
        }
    }
    return CFA_VAR_NOT_FOUND_ERR;
}

/*
get the number of AggregationVariables defined
*/
int 
cfa_inq_nvars(const int cfa_id, int *nvarp)
{
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;
    if (!(agg_cont->cfa_varp))
    {
        *nvarp = 0;
        return CFA_NOERR;
    }
    cfa_err = get_array_length(&(agg_cont->cfa_varp), nvarp);
    if (cfa_err)
        return cfa_err;
    return CFA_NOERR;
}

/*
get the AggregationVariable from a cfa_var_id
*/
int
cfa_get_var(const int cfa_id, const int cfa_var_id,
            AggregationVariable **agg_var)
{
#ifdef _DEBUG
    /* check id is in range */
    int cfa_nvars = 0;
    int cfa_err_v = cfa_inq_nvars(cfa_id, &cfa_nvars);
    if (cfa_err_v)
        return cfa_err_v;
    if (cfa_nvars == 0)
        return CFA_VAR_NOT_FOUND_ERR;
    if (cfa_var_id < 0 || cfa_var_id >= cfa_nvars)
        return CFA_VAR_NOT_FOUND_ERR;
#endif

    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;

    /* 
    check that the path is not NULL.  On cfa_close, the path is set to NULL 
    */
    cfa_err = get_array_node(&(agg_cont->cfa_varp), cfa_var_id, 
                             (void**)(agg_var));
    if (cfa_err)
        return cfa_err;

    if (!(*agg_var)->name)
        return CFA_VAR_NOT_FOUND_ERR;

    return CFA_NOERR;
}