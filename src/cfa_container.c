#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cfa.h"
#include "cfa_mem.h"
#include "parsers/cfa_netcdf.h"

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
        CFA_CHECK(cfa_err);
    }

    /* create the node in the array */
    AggregationContainer *cfa_node = NULL;
    cfa_err = create_array_node(&cfa_conts, (void**)(&cfa_node));
    CFA_CHECK(cfa_err);


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
    CFA_CHECK(cfa_err);
    *cfa_idp = cfa_ncont - 1;

    return CFA_NOERR;
}

/*
load a file from disk.  We may support different formats of CFA-netCDF files in 
the future, or different file formats to hold the CFA info.
*/

int 
cfa_load(const char *path, CFAFileFormat format, int *cfa_idp)
{
    int cfa_err = CFA_NOERR;
    switch (format)
    {
        case CFA_UNKNOWN:
            return CFA_UNKNOWN_FILE_FORMAT;
        break;
        case CFA_NETCDF:
            cfa_err = parse_netcdf_cfa_file(path, cfa_idp);
            CFA_CHECK(cfa_err);
        break;
        default:
            return CFA_UNKNOWN_FILE_FORMAT;
    }
    return cfa_err;
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
    int cfa_err = CFA_NOERR;
    if (cfa_conts)
    {
        cfa_err = get_array_length(&cfa_conts, &cfa_nfiles);
        CFA_CHECK(cfa_err);
    }
    for (int i=0; i<cfa_nfiles; i++)
    {
        cfa_err = get_array_node(&cfa_conts, i, (void**)(&cfa_node));
        CFA_CHECK(cfa_err);     
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
    /* first check for cfa_conts=NULL, this is 0 */
    int cfa_err = CFA_NOERR;
    if (!cfa_conts)
        *ncfa = 0;
    else
        cfa_err = get_array_length(&cfa_conts, ncfa);
    return cfa_err;
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
    CFA_CHECK(cfa_err);
    /* 
    check that the path is not NULL.  On cfa_close, the path is set to NULL 
    */
    if (!(*agg_cont)->path)
        return CFA_NOT_FOUND_ERR;
    return CFA_NOERR;
}

/*
free the memory used by the Aggregation containers
*/
int
cfa_free_containers(void)
{
    /* get the aggregation container struct */
    AggregationContainer *cfa_node = NULL;
    /* get the number of none freed array nodes */
    int n_conts = 0;
    int cfa_err = get_array_length(&cfa_conts, &n_conts);
    CFA_CHECK(cfa_err);
    int nfc = 0;
    for (int i=0; i<n_conts; i++)
    {
        cfa_err = get_array_node(&cfa_conts, i, (void**)(&(cfa_node)));
        CFA_CHECK(cfa_err);
        /* path of NULL indicates node has been freed */
        if (cfa_node->path)
            nfc += 1;
    }
    /* if number of non-free containers is 0 then free the array */
    if (nfc == 0)
    {
        cfa_err = free_array(&cfa_conts);
        cfa_conts = NULL;
    }
    CFA_CHECK(cfa_err);
    return CFA_NOERR;
}

extern int cfa_free_vars(const int);
extern int cfa_free_dims(const int);
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
    CFA_CHECK(cfa_err);
    if (cfa_node)
    {
        /* free the variables and the dimensions */
        cfa_err = cfa_free_vars(cfa_id);
        CFA_CHECK(cfa_err);
        cfa_err = cfa_free_dims(cfa_id);
        CFA_CHECK(cfa_err);
        /* free memory of dynamically allocated components */
        if (cfa_node->path)
        {
            /* free the path */
            cfa_free(cfa_node->path, strlen(cfa_node->path));
            cfa_node->path = NULL;
        }
    }
    /* check and free the overall container if empty */
    cfa_err = cfa_free_containers();
    CFA_CHECK(cfa_err);
    return CFA_NOERR;
}