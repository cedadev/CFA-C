#include <stdlib.h>
#include <string.h>

#include "cfa.h"

/* Start of the resizeable array in memory */
static AggregationContainer *cfa_conts = NULL;
static int cfa_nfiles = 0;

/* 
create a CFA AggregationContainer and assign it to cfaid 
*/
int
cfa_create(const char *path, int *cfa_idp)
{
    if (!cfa_conts)
    {
        /* create the first item in the array if it doesn't exist */
        cfa_conts = (AggregationContainer*) malloc(
                sizeof(AggregationContainer)
            );
        if (!cfa_conts)
            return CFA_MEM_ERR;
    }
    else
    {
        /* reallocate the memory so that it can contain one more 
           AggregationContainer
        */
        AggregationContainer* tmp_mem = (AggregationContainer*) realloc(
            cfa_conts, (cfa_nfiles+1) * sizeof(AggregationContainer)
        );
        if (!tmp_mem)
            return CFA_MEM_ERR;
        cfa_conts = tmp_mem;
    }

    /* add the path */
    cfa_conts[cfa_nfiles].path = (char*) malloc(sizeof(char) * strlen(path));
    if (!(cfa_conts[cfa_nfiles].path))
        return CFA_MEM_ERR; 
    strcpy(cfa_conts[cfa_nfiles].path, path);

    /* set var pointer to NULL */
    cfa_conts[cfa_nfiles].cfa_varp = NULL;
    cfa_conts[cfa_nfiles].cfa_nvar = 0;

    /* set dim pointer to NULL */
    cfa_conts[cfa_nfiles].cfa_dimp = NULL;
    cfa_conts[cfa_nfiles].cfa_ndim = 0;

    /* set container ponter to NULL */
    cfa_conts[cfa_nfiles].cfa_containerp = NULL;
    cfa_conts[cfa_nfiles].cfa_ncontainer = 0;


    /* allocate then iterate the identifier */
    *cfa_idp = cfa_nfiles;
    cfa_nfiles++;

    return CFA_NOERR;
}

/*
get the identifier of a CFA AggregationContainer by the path name
*/
int
cfa_inq_id(const char *path, int *cfa_idp)
{
    /* search through the array looking for the matching path */
    for (int i=0; i<cfa_nfiles; i++)
    {
        /* closed AggregationContainers have their path set to NULL */
        if (!cfa_conts[i].path)
            continue;
        if (strcmp(cfa_conts[i].path, path) == 0)
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
cfa_inq_n(void)
{
    return cfa_nfiles;
}

/*
get an instance of an AggregationContainer struct, with the id of cfa_id
*/
int
cfa_get(const int cfa_id, AggregationContainer **agg_file)
{
    /* check id is in range */
    if (cfa_id < 0 || cfa_id >= cfa_nfiles)
        return CFA_NOT_FOUND_ERR;

    /* check that the path is not NULL.  On cfa_close, the path is set to NULL 
    */
    if (!cfa_conts[cfa_id].path)
        return CFA_NOT_FOUND_ERR;

    /* assign return value */
    *agg_file = &(cfa_conts[cfa_id]);
    return CFA_NOERR;
}

/* close a CFA AggregationContainer container */
int cfa_close(const int cfa_id)
{
    AggregationContainer *agg_file = NULL;
    int cfa_err = 1;

    /* check id is in range */
    if (cfa_id < 0 || cfa_id >= cfa_nfiles)
        return CFA_NOT_FOUND_ERR;

    /* get the aggregation file struct */
    cfa_err = cfa_get(cfa_id, &agg_file);
    /* check that it is valid */
    if (cfa_err)
        return cfa_err;

    if (agg_file)
    {
        /* free memory of dynamically allocated components */
        if (agg_file->path)
        {
            /* free the path */
            free(agg_file->path);
            agg_file->path = NULL;
        }
        /* free the AggregatedDimension */
        if (agg_file->cfa_dimp)
        {
            for (int i=0; i<agg_file->cfa_ndim; i++)
            {
                /* get a pointer to the individual AggregatedDimension */
                AggregatedDimension *agg_dim = &(agg_file->cfa_dimp[i]);
                if(agg_dim->name)
                {
                    free(agg_dim->name);
                    agg_dim->name = NULL;
                }
            }

            /* free the AggregatedDimension array and set to NULL */
            free(agg_file->cfa_dimp);
            agg_file->cfa_dimp = NULL;
            agg_file->cfa_ndim = 0;
        }
    }
    
    return CFA_NOERR;
}