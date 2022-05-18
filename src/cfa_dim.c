#include <stdlib.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"

extern int get_type_size(const cfa_type);

/* Start of the dimensions resizeable array in memory */
DynamicArray *cfa_dims = NULL;

extern void __free_str_via_pointer(char**);

/*
create an AggregatedDimension, attach it to a cfa_id
*/
int 
cfa_def_dim(const int cfa_id, const char *name, const int len, 
            const cfa_type dtype, int *cfa_dim_idp)
{
    /* get the AggregationContainer */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* 
    if no dimensions have been defined previously, then the cfa_dims pointer 
    will still be NULL
    */
    if (!(cfa_dims))
    {
        cfa_err = create_array(&(cfa_dims), sizeof(AggregatedDimension));
        CFA_CHECK(cfa_err);
    }
    /* array is created so now create and return the array node */
    AggregatedDimension *dim_node = NULL;
    cfa_err = create_array_node(&(cfa_dims), (void**)(&dim_node));
    CFA_CHECK(cfa_err);

    /* copy the length and name to the dimension */
    dim_node->length = len;
    dim_node->name = strdup(name);

    /* assign the type */
    dim_node->cfa_dtype.type = dtype;
    dim_node->cfa_dtype.size = get_type_size(dtype);

    /* get the length of the AggregatedDimension array - the id is len-1 */
    int cfa_ndim = 0;
    cfa_err = get_array_length(&(cfa_dims), &cfa_ndim);
    CFA_CHECK(cfa_err);

    /* write back the cfa_dim_id */
    *cfa_dim_idp = cfa_ndim - 1;

    /* assign to the container */
    agg_cont->cfa_dimids[agg_cont->n_dims++] = *cfa_dim_idp;

    return CFA_NOERR;
}

/*
get the identifier of an AggregatedDimension by name
*/
int
cfa_inq_dim_id(const int cfa_id, const char* name, int *cfa_dim_idp)
{
    /* get the AggregationContainer */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* search through the dimensions, looking for the matching name */
    AggregatedDimension *cdim = NULL;
    for (int i=0; i<agg_cont->n_dims; i++)
    {
        /* dimensions that belong to a closed AggregationContainer have their
        name set to NULL */
        cfa_err = get_array_node(&(cfa_dims), 
                                 agg_cont->cfa_dimids[i], 
                                 (void**)(&cdim));
        CFA_CHECK(cfa_err);

        if (!(cdim->name))
            continue;
        if (strcmp(cdim->name, name) == 0)
        {
            /* found, so assign and return */
            *cfa_dim_idp = agg_cont->cfa_dimids[i];
            return CFA_NOERR;
        }
    }
    return CFA_DIM_NOT_FOUND_ERR;
}

/*
get the number of AggregatedDimensions defined
*/
int
cfa_inq_ndims(const int cfa_id, int *ndimp)
{
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    *ndimp = agg_cont->n_dims;
    return CFA_NOERR;
}

/* 
get the ids for the AggregatedDimensions in the AggregationContainer 
*/
int 
cfa_inq_dim_ids(const int cfa_id, int **dimids)
{
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    *dimids = agg_cont->cfa_dimids;
    return CFA_NOERR;
}

/* 
get the AggregatedDimension from a cfa_dim_id
*/
int 
cfa_get_dim(const int cfa_id, const int cfa_dim_id, 
            AggregatedDimension **agg_dim)
{
    /*
    check that the array has been created yet 
    */
    if (!cfa_dims)
        return CFA_DIM_NOT_FOUND_ERR;

#ifdef _DEBUG
    /* check id is in range */
    int cfa_ndims = 0;
    int cfa_err_d = get_array_length(&(cfa_dims), &cfa_ndims);
    CFA_CHECK(cfa_err_d);
    if (cfa_ndims == 0)
        return CFA_DIM_NOT_FOUND_ERR;

    if (cfa_dim_id < 0 || cfa_dim_id >= cfa_ndims)
        return CFA_DIM_NOT_FOUND_ERR;
    /* check id belongs to AggregationContainer */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);
    int dim_in_cont = 0;
    for (int i=0; i<agg_cont->n_dims; i++)
        if (agg_cont->cfa_dimids[i] == cfa_dim_id)
        {
            dim_in_cont = 1;
            break;
        }
    if (!dim_in_cont)
        return CFA_DIM_NOT_FOUND_ERR;
#endif

    /* 
    check that the path is not NULL.  On cfa_close, the path is set to NULL 
    */
    cfa_err = get_array_node(&(cfa_dims), cfa_dim_id, (void**)(agg_dim));
    CFA_CHECK(cfa_err);

    if (!(*agg_dim)->name)
    {
        agg_dim = NULL;
        return CFA_DIM_NOT_FOUND_ERR;
    }

    return CFA_NOERR;
}

/*
free the memory used by the CFA dimensions
*/
int
cfa_free_dims(const int cfa_id)
{
    /* get the AggregationContainer */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* loop over the dimensions, freeing memory as we go */
    AggregatedDimension *agg_dim = NULL;
    for (int i=0; i<agg_cont->n_dims; i++)
    {
        cfa_err = get_array_node(&(cfa_dims), 
                                 agg_cont->cfa_dimids[i], 
                                 (void**)(&agg_dim));
        CFA_CHECK(cfa_err);
        __free_str_via_pointer(&(agg_dim->name));
    }
    /* check whether all cfa_dims are free (name=NULL) and free the DynamicArray
    holding all the AggregatedDimensions if they are */
    /* first check that cfa_dims != NULL */
    if (cfa_dims)
    {
        int n_dims = 0;
        cfa_err = get_array_length(&cfa_dims, &n_dims);
        CFA_CHECK(cfa_err);
        int nfd = 0;
        AggregatedDimension *cfa_node = NULL;
        
        for (int i=0; i<n_dims; i++)
        {
            cfa_err = get_array_node(&cfa_dims, i, (void**)(&(cfa_node)));
            CFA_CHECK(cfa_err);
            /* name == NULL indicates node has been freed */
            if (cfa_node->name)
                nfd += 1;
        }
        /* if number of non-free dimensions is 0 then free the array 
        and also free the FragmentDimension array */
        if (nfd == 0)
        {
            cfa_err = free_array(&cfa_dims);
            CFA_CHECK(cfa_err);
            cfa_dims = NULL;
        }
    }

    return CFA_NOERR;
}
