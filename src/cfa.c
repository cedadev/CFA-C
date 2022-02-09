#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cfa.h"
#include "cfa_mem.h"
#include "parsers/cfa_netcdf.h"

/* Start of the containers resizeable array in memory
   These containers can contain CFA Files or Groups */
DynamicArray *cfa_conts = NULL;

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

    /* add the path, name to NULL */
    cfa_node->path = strdup(path);
    cfa_node->name = NULL;

    /* set number of vars to 0 */
    cfa_node->n_vars = 0;

    /* set number of dims to 0 */
    cfa_node->n_dims = 0;

    /* set number of containers to 0 */
    cfa_node->n_conts = 0;

    /* get the identifier as the last node of the array */
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
    check that the path or name is not NULL.  On cfa_close, the path or name is
    set to NULL 
    */
    if ((*agg_cont)->path || (*agg_cont)->name)
        return CFA_NOERR;
    return CFA_NOT_FOUND_ERR;
}

extern int cfa_free_cont(const int);

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
        /* free containers */
        cfa_err = cfa_free_cont(cfa_id);
        CFA_CHECK(cfa_err);
   }
    return CFA_NOERR;
}

/* return the size of a type */
int get_type_size(const cfa_type type)
{
    switch (type)
    {
        case CFA_NAT:
            return 0;
        case CFA_BYTE:
            return 1;
        case CFA_CHAR:
            return 1;
        case CFA_SHORT:
            return 2;
        case CFA_INT:
            return 4;
        case CFA_FLOAT:
            return 4;
        case CFA_DOUBLE:
            return 8;
        case CFA_UBYTE:
            return 1;
        case CFA_USHORT:
            return 2;
        case CFA_UINT:
            return 4;
        case CFA_INT64:
            return 8;
        case CFA_UINT64:
            return 8;
    }
}

/* return the type name */
const char* get_type_name(const cfa_type type)
{
    switch (type)
    {
        case CFA_NAT:
            return "nat";
        case CFA_BYTE:
            return "byte";
        case CFA_CHAR:
            return "char";
        case CFA_SHORT:
            return "short int";
        case CFA_INT:
            return "int";
        case CFA_FLOAT:
            return "float";
        case CFA_DOUBLE:
            return "double";
        case CFA_UBYTE:
            return "unsigned byte";
        case CFA_USHORT:
            return "unsigned short int";
        case CFA_UINT:
            return "unsigned int";
        case CFA_INT64:
            return "long int";
        case CFA_UINT64:
            return "unsigned long int";
    }    
}