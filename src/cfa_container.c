#include <stdlib.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"

/* Start of the resizeable array in memory */
static DynamicArray *cfa_conts = NULL;

/* 
create a CFA AggregationContainer and assign it to cfa_idp
*/
int
cfa_create(const char *path, int *cfa_idp)
{
    /* create the array if not already created */
    int cfa_err;
    if (!cfa_conts)
    {
        cfa_err = create_array(&cfa_conts, sizeof(AggregationContainer));
        if (cfa_err) 
            return cfa_err;
    }

    /* create the node in the array */
    AggregationContainer *cfa_node = NULL;
    cfa_err = create_array_node(&cfa_conts, (void**)(&cfa_node));
    if (cfa_err) 
        return cfa_err;

    /* add the path */
    cfa_node->path = (char*) cfa_malloc(sizeof(char) * strlen(path));
    if (!(cfa_node->path)) 
        return CFA_MEM_ERR; 
    strcpy(cfa_node->path, path);

    /* set var pointer to NULL */
    cfa_node->cfa_varp = NULL;

    /* set dim pointer to NULL */
    cfa_node->cfa_dimp = NULL;

    /* set container ponter to NULL */
    cfa_node->cfa_containerp = NULL;

    /* get the identifier as the last node of the array*/
    int cfa_ncont = 0;
    cfa_err = get_array_length(&cfa_conts, &cfa_ncont);
    if (cfa_err)
        return cfa_err;
    *cfa_idp = cfa_ncont - 1;

    return CFA_NOERR;
}

/*
get the identifier of a CFA AggregationContainer by the path name
*/
int
cfa_inq_id(const char *path, int *cfa_idp)
{
    AggregationContainer *cfa_node = NULL;
    /* search through the array looking for the matching path */
    int cfa_nfiles = 0;
    int cfa_err = get_array_length(&cfa_conts, &cfa_nfiles);
    if (cfa_err)
        return cfa_err;
    for (int i=0; i<cfa_nfiles; i++)
    {
        cfa_err = get_array_node(&cfa_conts, i, (void**)(&cfa_node));
        if (cfa_err)
            return cfa_err;        
        /* closed AggregationContainers have their path set to NULL */
        if (!(cfa_node->path))
            continue;
        if (strcmp(cfa_node->path, path) == 0)
        {
            /* found, so assign and return */
            *cfa_idp = i;
            return CFA_NOERR;
        }
    }
    /* not found, return error */
    return CFA_NOT_FOUND_ERR;
}

/*
get the number of cfa files.  this includes closed files, but there are checks
to ensure closed files are not used in the other functions
*/
int 
cfa_inq_n(int *ncfa)
{
    int cfa_err = get_array_length(&cfa_conts, ncfa);
    if (cfa_err)
        return cfa_err;
    return CFA_NOERR;
}

/*
get an instance of an AggregationContainer struct, with the id of cfa_id
*/
int
cfa_get(const int cfa_id, AggregationContainer **agg_cont)
{
#ifdef _DEBUG
    /* check id is in range */
    int cfa_nfiles = 0;
    int cfa_err_d = cfa_inq_n(&cfa_nfiles);
    if (cfa_err_d)
        return cfa_err_d;
    if (cfa_id < 0 || cfa_id >= cfa_nfiles)
        return CFA_NOT_FOUND_ERR;
#endif
    /* assign return value */
    int cfa_err = get_array_node(&cfa_conts, cfa_id, (void**)(agg_cont));
    if (cfa_err)
        return cfa_err;
    /* 
    check that the path is not NULL.  On cfa_close, the path is set to NULL 
    */
    if (!(*agg_cont)->path)
        return CFA_NOT_FOUND_ERR;
    return CFA_NOERR;
}

/* close a CFA AggregationContainer container */
int cfa_close(const int cfa_id)
{
#ifdef _DEBUG
    /* check id is in range */
    int cfa_nfiles = 0;
    int cfa_err_d = cfa_inq_n(&cfa_nfiles);
    if (cfa_err_d)
        return cfa_err_d;
    if (cfa_id < 0 || cfa_id >= cfa_nfiles)
        return CFA_NOT_FOUND_ERR;
#endif

    /* get the aggregation container struct */
    AggregationContainer *cfa_node = NULL;

    int cfa_err = cfa_get(cfa_id, &(cfa_node));
    /* check that it is valid */
    if (cfa_err)
        return cfa_err;

    if (cfa_node)
    {
        /* free memory of dynamically allocated components */
        if (cfa_node->path)
        {
            /* free the path */
            free(cfa_node->path);
            cfa_node->path = NULL;
        }
        // /* free the AggregatedDimension */
        // if (cfa_node->cfa_dimp)
        // {
        //     for (int i=0; i<cfa_node->cfa_ndim; i++)
        //     {
        //         /* get a pointer to the individual AggregatedDimension */
        //         AggregatedDimension *agg_dim = &(cfa_node->cfa_dimp[i]);
        //         if(agg_dim->name)
        //         {
        //             free(agg_dim->name);
        //             agg_dim->name = NULL;
        //         }
        //     }

        //     /* free the AggregatedDimension array and set to NULL */
        //     free(cfa_node->cfa_dimp);
        //     cfa_node->cfa_dimp = NULL;
        //     cfa_node->cfa_ndim = 0;
        // }
    }
    
    return CFA_NOERR;
}