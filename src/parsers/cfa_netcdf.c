#include <netcdf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"
#include "parsers/cfa_netcdf.h"

#define MAX_GROUPS 256
#define STR_LENGTH 256

extern DynamicArray *cfa_frag_dims;
extern int _has_standard_agg_instr(const int, const int);

/*
check this is a CFA-netCDF file
*/
int
_is_cfa_netcdf_file(const int ncid)
{
    /* determine if this is a CFA file by searching for the string "CFA-0.6" 
       in the Conventions global attribute */

    /* allocate the string and read in the Convention string */
    char convention[STR_LENGTH] = "";
    int err = nc_get_att(ncid, NC_GLOBAL, CONVENTIONS, convention);
    if (err == NC_ENOTATT)
        return CFA_NOT_CFA_FILE;
    else
        CFA_CHECK(err);

    /* check that CFA_CONVENTION is a substring in convention */
    if (!strstr(convention, CFA_CONVENTION))
        return CFA_NOT_CFA_FILE;

    /* check that CFA_VERSION major and minor versions are substrings in 
    convention */
    char version[4];
    sprintf(version, "%i.%i", CFA_MAJOR_VERSION, CFA_MINOR_VERSION);
    if (!strstr(convention, version))
        return CFA_UNSUPPORTED_VERSION;
    return CFA_NOERR;
}

/* 
check if a variable is a Data variable
*/
int
_is_cfa_data_variable(const int ncid, const int ncvarid, int *iscfavarp)
{
    /* CFA Data variables have the attribute "aggregated_data" */
    int nvaratt = 0;
    int err = nc_inq_varnatts(ncid, ncvarid, &nvaratt);
    CFA_CHECK(err);
    char attname[STR_LENGTH] = "";
    /* needs both "aggregated_data" and "aggregated_dimensions" attributes */
    int cfa_att_count = 0;
    for (int a=0; a<nvaratt; a++)
    {
        err = nc_inq_attname(ncid, ncvarid, a, attname);
        CFA_CHECK(err);
        if (strstr(attname, AGGREGATED_DATA))
            cfa_att_count += 1;
        if (strstr(attname, AGGREGATED_DIMENSIONS))
            cfa_att_count += 1;
    }
    *iscfavarp = (cfa_att_count > 1);
    return CFA_NOERR;
}

/*
check if a group has a CFA Data variable in it
*/
int
_is_cfa_data_group(const int ncid, int *iscfagrpp)
{
    /* a CFA Data group will have at least one CFA Data variable in it */
    int nvars = 0;
    int varidsp[NC_MAX_VARS];
    int err = nc_inq_varids(ncid, &nvars, varidsp);
    int ncfa_vars = 0;
    int iscfavar = 0;
    for (int v=0; v<nvars; v++)
    {
        err = _is_cfa_data_variable(ncid, varidsp[v], &iscfavar);
        CFA_CHECK(err);
        ncfa_vars += iscfavar;
    }
    /* if more than one CFA Variable then iscfagrp = True */
    *iscfagrpp = (ncfa_vars > 0);
    return CFA_NOERR;
}

/*
get a group name from a string
*/
int
_get_grp_name_from_str(const char *in_str, char *out_grp_name)
{
    char str[STR_LENGTH] = "";
    strcpy(str, in_str);
    char *strp = strtok(str, "/");
    if (strp != NULL)
    {
        strcpy(out_grp_name, strp);
        /* needs another / to still be a group, rather than a variable */
        if (strtok(NULL, "/") != NULL)
            return CFA_NOERR;
    }
    return CFA_EOS;
}

/*
get a variable name from a string
*/
int
_get_var_name_from_str(const char *in_str, char *out_var_name)
{
    char str[STR_LENGTH];
    strcpy(str, in_str);
    char *strp = strtok(str, "/");
    while (strp != NULL)
    {
        strcpy(out_var_name, strp);
        strp = strtok(NULL, "/");
    }
    return CFA_NOERR;
}

/* 
get the root group from any group id
*/
int 
_get_root_grp_id(const int nc_id, int *root_grp_id)
{
    int cfa_err = CFA_NOERR;
    int c_grp = -1;
    *root_grp_id = nc_id;
    while (cfa_err == CFA_NOERR)
    {
        cfa_err = nc_inq_grp_parent(*root_grp_id, &c_grp);
        if (cfa_err == CFA_NOERR)
            *root_grp_id = c_grp;
    }
    return CFA_NOERR;
}

/*
get a netcdf group id from a string
*/
int
_get_nc_grp_id_from_str(const int nc_id, const char *in_str, int *ret_grp_id)
{
    /* get the group id in a netCDF from a variable string that may contain
    various groups, e.g. /group1/group2/var1
    */
    char grpname[STR_LENGTH] = "";
    int orig_str_len = strlen(in_str)+1;
    int strpos = 0;
    int pgrp_id = -1;
    int cfa_err = CFA_NOERR;

    /* get the root group to start the search from */
    cfa_err = _get_root_grp_id(nc_id, &pgrp_id);
    CFA_CHECK(cfa_err);

    while(strpos < orig_str_len)
    {
        /* get the first group name */
        cfa_err = _get_grp_name_from_str(in_str + strpos, grpname);
        /* if the token has been found then use the preceeding characters to
        create the group - check if it exists first */
        if(cfa_err != CFA_EOS)
        {
            int grp_id = -1;
            int grp_err = nc_inq_grp_ncid(pgrp_id, grpname, &grp_id);
            if (grp_err == NC_NOERR)
            {
                /* if no error then the group has been found and we need to use
                it as the parent group */
                pgrp_id = grp_id;
            }
            else
            {
                *ret_grp_id = pgrp_id;
                return NC_ENOGRP;
            }
        }
        strpos += strlen(grpname) + 1;
    }
    /* assign the return group id to the parent group (which is the same as the
    grp_id at this point) */
    *ret_grp_id = pgrp_id;
    return CFA_NOERR;
}

int
_get_nc_grp_var_ids_from_str(const int nc_id, const char *in_str,
                             int *ret_grp_id, int *ret_var_id)
{
    /* get the netCDF ids of a group and a variable from a string that has
    groups and variables compounded in it, e.g.:
    /group1/group2/variable */
    /* get the group id from the function above, do not create the group */
    int cfa_err = _get_nc_grp_id_from_str(nc_id, in_str, ret_grp_id);
    CFA_CHECK(cfa_err);
    /* get the varname */
    char var_name[STR_LENGTH] = "";
    cfa_err = _get_var_name_from_str(in_str, var_name);
    CFA_CHECK(cfa_err);
    /* get the netCDF var id */
    cfa_err = nc_inq_varid(*ret_grp_id, var_name, ret_var_id);
    CFA_CHECK(cfa_err);
    return CFA_NOERR;
}

/*
get a key and value pair from the att_str, returning the next position in the
string.  The att_str is space delimited and the key is identified by a colon,
e.g.:
"key: value key: value key: value \0"
*/
char*
_get_key_and_value_from_att_str(const char* att_str, char* key, char* value)
{
    /* use sscanf to search for 2 strings, the key and value */
    char* c_str_ptr = (char*)(att_str);
     /* ignore any whitespace */
    while (*c_str_ptr == ' ' || *c_str_ptr == '\n')
        c_str_ptr++;

    int n_strs = sscanf(c_str_ptr, "%s %s", key, value);
    /* if not 2 strings then return */
    if (n_strs < 2)
        return NULL;
    c_str_ptr += strlen(key) + strlen(value) + 2;
    /* remove the colon from key by adding an extra string terminator */
    key[strlen(key)-1] = '\0';

    return c_str_ptr;
}

/*
get the size of a variable - mostly to check whether it is a scalar or not
*/
int
_get_var_size(int nc_grpid, int nc_varid, size_t* var_size)
{
    /* create a product of all the dimension lengths */
    /* get the number of dimensions */
    int ndims = -1;
    int err = nc_inq_varndims(nc_grpid, nc_varid, &ndims);
    CFA_CHECK(err);

    /* get all the dimension ids */
    int dimids[MAX_DIMS];
    err = nc_inq_vardimid(nc_grpid, nc_varid, dimids);
    CFA_CHECK(err);

    /* size will be a product of all the dimension lengths */
    size_t dimlen = -1;
    *var_size = 1;
    for (int d=0; d<ndims; d++)
    {
        err = nc_inq_dimlen(nc_grpid, dimids[d], &dimlen);
        CFA_CHECK(err);
        *var_size *= dimlen;
    }
    return CFA_NOERR;
}

int
_parse_cfa_aggregated_dimensions(const int ncid, const int ncvarid, 
                                 const int cfa_id, const int cfa_var_id)
{
    /* read the "aggregated dimensions" attribute */
    char att_str[STR_LENGTH] = "";

    int err = nc_get_att_text(ncid, ncvarid, AGGREGATED_DIMENSIONS, att_str);
    CFA_CHECK(err);
    /* split on space */
    char *strp = strtok(att_str, " ");
    /* process the split string into dimname and either create or get the dimid
    */
    char dimname[STR_LENGTH] = "";
    int ndims = 0;
    int dimids[NC_MAX_DIMS];
    while(strp != NULL)
    {
        strcpy(dimname, strp);
        /* see if the CFA AggregatedDimension has already been added */
        int cfa_dim_id = 0;
        err = cfa_inq_dim_id(cfa_id, dimname, &cfa_dim_id);
        if (err == CFA_DIM_NOT_FOUND_ERR)   /* this is the desirable outcome */
        {
            /* get the length of the dimension from the netCDF file */
            int ncdimid = 0;
            int ncdimvarid = 0;
            size_t ncdimlen = 0;
            nc_type dtype = NC_NAT;
            err = nc_inq_dimid(ncid, dimname, &ncdimid);
            CFA_CHECK(err);
            err = nc_inq_dimlen(ncid, ncdimid, &ncdimlen);
            CFA_CHECK(err);
            /* get the Dimension type from the Dimension variable */
            err = nc_inq_varid(ncid, dimname, &ncdimvarid);
            CFA_CHECK(err);
            err = nc_inq_vartype(ncid, ncdimvarid, &dtype);
            CFA_CHECK(err);
            /* create the AggregatedDimension */
            err = cfa_def_dim(cfa_id, dimname, ncdimlen, dtype, &cfa_dim_id);
            CFA_CHECK(err);
        }
        else if (err != CFA_NOERR)
            CFA_CHECK(err);
        dimids[ndims] = cfa_dim_id;
        ndims++;
        strp = strtok(NULL, " ");
    }
    /* we now have an array of AggregatedDimensions in dimids, add these to 
    the AggregationVariable */
    err = cfa_var_def_dims(cfa_id, cfa_var_id, ndims, dimids);
    CFA_CHECK(err);
    return CFA_NOERR;
}

/*
parse the FragmentDimensions from the netCDF file and call
_create_fragment_dimensions - this will create the Fragment array ready to read
Fragment definitions into when cfa_var_get1_frag(...) is called (which in turn
calls _read_cfa_fragloc_netcdf(...)
*/

extern int _create_fragment_dimensions(int, AggregationVariable*, const int*);

int
_parse_cfa_fragment_dimensions(const int ncid,
                               const int cfa_id, const int cfa_var_id)
{
    int cfa_err = -1;
    /* get the CFA variable */
    AggregationVariable *cfa_var = NULL;
    cfa_err = cfa_get_var(cfa_id, cfa_var_id, &cfa_var);
    CFA_CHECK(cfa_err);
    /* we can get the number of fragments along each dimension from the 
    file variable, or from the address variable as these are never scalar */
    if (cfa_var->n_instr == 0)
        return CFA_VAR_NO_AGG_INSTR;

    /* get the file AggregationInstruction */
    AggregationInstruction *pinst;
    cfa_err = cfa_var_get_agg_instr(cfa_id, cfa_var_id, "file", &pinst);
    CFA_CHECK(cfa_err);
    int file_frag_grpid = -1;
    int file_frag_varid = -1;
    cfa_err = _get_nc_grp_var_ids_from_str(
        ncid, pinst->value, &file_frag_grpid, &file_frag_varid
    );
    CFA_CHECK(cfa_err);

    /* get the number of dims */
    int ndims = -1;
    int err = nc_inq_varndims(file_frag_grpid, file_frag_varid, &ndims);
    CFA_CHECK(err);

    /* get the dim ids from the file variable */
    int dimids[MAX_DIMS];
    int dimlen[MAX_DIMS];
    cfa_err = nc_inq_vardimid(file_frag_grpid, file_frag_varid, dimids);
    CFA_CHECK(cfa_err);

    /* get the dim lens from the file variable */
    for (int d=0; d<ndims; d++)
    {
        size_t dl = -1;
        err = nc_inq_dimlen(file_frag_grpid, dimids[d], &dl);
        dimlen[d] = (int)(dl);
        CFA_CHECK(err);
    }

    /* finally define the fragments */
    cfa_err = _create_fragment_dimensions(cfa_id, cfa_var, dimlen);
    CFA_CHECK(err);

    return CFA_NOERR;
}

/*
get the aggregation instructions from an attribute string
*/
int
_parse_cfa_aggregation_instructions(const int ncid, const int ncvarid,
                                    const int cfa_id, const int cfa_var_id)
{
    /* read the "aggregated_data" attribute - these are the aggregation 
    instructions */
    char att_str[STR_LENGTH] = "";
    int err = nc_get_att_text(ncid, ncvarid, AGGREGATED_DATA, att_str);
    CFA_CHECK(err);

    /* variables for determining if a scalar or not */
    int nc_agg_grpid = -1;
    int nc_agg_varid = -1;
    size_t var_size = 0;
    bool scalar = false;

    /* parse the "aggregated_data" attribute, getting each key:value pair */
    char key[STR_LENGTH] = "";
    char value[STR_LENGTH] = "";

    /* extract the initial key: value pairs */
    char* c_ptr = _get_key_and_value_from_att_str(att_str, key, value);;
    while(c_ptr != NULL)
    {
        /* determine whether this is a scalar variable or not
        for all variables in v0.6.2 */
        err = _get_nc_grp_var_ids_from_str(
            ncid, value, &nc_agg_grpid, &nc_agg_varid
        );
        CFA_CHECK(err);
        err = _get_var_size(nc_agg_grpid, nc_agg_varid, &var_size);
        CFA_CHECK(err);
        if (var_size == 1)
            scalar = true;
        else
            scalar = false;
        /* get the type of the aggregation instruction */
        nc_type vtype = NC_NAT;
        err = nc_inq_vartype(nc_agg_grpid, nc_agg_varid, &vtype);
        CFA_CHECK(err);
        /* define the aggregation instructions in the cfa var */
        err = cfa_var_def_agg_instr(
            cfa_id, cfa_var_id, key, value, scalar, vtype
        );
        CFA_CHECK(err);
        /* get the next key and value */
        c_ptr = _get_key_and_value_from_att_str(c_ptr, key, value);
    }
    /* check the variable exists */
    AggregationVariable *cfa_var = NULL;
    err = cfa_get_var(cfa_id, cfa_var_id, &cfa_var);
    CFA_CHECK(err);
    /* check that four AggregationInstructions have been added */
    if (!_has_standard_agg_instr(cfa_id, cfa_var_id))
        return CFA_AGG_NOT_DEFINED;

    return CFA_NOERR;
}

/*
parse an individual variable
*/
int 
_parse_cfa_variable_netcdf(const int ncid, const int ncvarid, const int cfa_id)
{
    /* get the name of the variable */
    char varname[NC_MAX_NAME] = "";
    nc_type vtype = NC_NAT;      /* NC type and CFA type are analogous */
    int err = nc_inq_varname(ncid, ncvarid, varname);
    CFA_CHECK(err);
    err = nc_inq_vartype(ncid, ncvarid, &vtype);
    /* define the variable */
    int cfa_var_id = -1;
    err = cfa_def_var(cfa_id, varname, vtype, &cfa_var_id);
    CFA_CHECK(err);

    /* get the aggregation instructions */
    err = _parse_cfa_aggregation_instructions(ncid, ncvarid, cfa_id, cfa_var_id);
    CFA_CHECK(err);
    /* get the aggregated dimensions */
    err = _parse_cfa_aggregated_dimensions(ncid, ncvarid, cfa_id, cfa_var_id);
    CFA_CHECK(err);
    /* parse the fragment dimensions */
    err = _parse_cfa_fragment_dimensions(ncid, cfa_id, cfa_var_id);
    CFA_CHECK(err);

    return CFA_NOERR;
}

/* 
entry point for the recursive parser - parse a single group. On first entry
this should be the root container (group)
*/
int
_parse_cfa_container_netcdf(int ncid, int cfa_id)
{
    /* parse the groups in this group / root */
    /* get the number of groups in the netCDF file or group */
    int grp_idp[MAX_GROUPS];
    char grp_name[NC_MAX_NAME] = "";
    size_t grp_name_len = 1;
    int ngrp = 0;
    int err = nc_inq_grps(ncid, &ngrp, grp_idp);
    CFA_CHECK(err);

    /* loop over the groups */
    for (int g=0; g<ngrp; g++)
    {
        /* check first if this is a CFA group - has CFA Variables defined in it 
        */
        int is_cfa_grp = 0;
        err = _is_cfa_data_group(grp_idp[g], &is_cfa_grp);
        CFA_CHECK(err);
        if (!is_cfa_grp)
            continue;

        /* get the netCDF group name */
        err = nc_inq_grpname_full(grp_idp[g], &grp_name_len, grp_name);
        /* create the sub-group as part of this parent group and then 
        recursively parse it by calling this function with the sub-group id */
        int cfa_cont_id = -1;
        err = cfa_def_cont(cfa_id, grp_name, &cfa_cont_id);
        CFA_CHECK(err);
        err = _parse_cfa_container_netcdf(grp_idp[g], cfa_cont_id);
        CFA_CHECK(err);
        /* assign the external id */
        AggregationContainer *agg_cont = NULL;
        err = cfa_get(cfa_cont_id, &agg_cont);
        CFA_CHECK(err);
        agg_cont->x_id = grp_idp[g];
    }

    /* parse the variables in the group */
    /* get the number of (netcdf) variables */
    int nvar = 0;
    err = nc_inq_nvars(ncid, &nvar);
    CFA_CHECK(err);

    /* loop over each netCDF variable and check whether it is a CFA-variable */
    for (int v=0; v<nvar; v++)
    {
        int iscfavar = 0;
        err = _is_cfa_data_variable(ncid, v, &iscfavar);
        CFA_CHECK(err);
        if (iscfavar)
        {
            err = _parse_cfa_variable_netcdf(ncid, v, cfa_id);
            CFA_CHECK(err);
        }
    }

    return CFA_NOERR; 
}

int _read_scalar_location(const int nc_id, const int nc_var_id,
                          Fragment *frag)
{
    /* just one element for scalar */
    size_t size = 1;
    if (frag->location == NULL) 
        frag->location = cfa_malloc(size);

    /* just one data point if scalar */
    size_t scale_var[1] = {0};
    int frag_loc = -1;
    int cfa_err = nc_get_var1_int(nc_id, nc_var_id, scale_var, &frag_loc);
    *(frag->location) = (size_t)(frag_loc);
    CFA_CHECK(cfa_err);
    return CFA_NOERR;
}

#ifndef FAST_INDEX
int _read_indexed_location(int frag_grp_id, int frag_var_id,
                           AggregationVariable* agg_var,
                           Fragment* frag)
{
    /* read the fragment locations from an index into the Fragment array */
    /* this is the "slow" method, which allows fragments to have different 
       spans in their location variable */
    size_t span_size = (sizeof(size_t) << 1) * agg_var->cfa_ndim;
    if (frag->location == NULL)
        frag->location = cfa_malloc(span_size);

    /* start and count - these are 2D arrays, for i and j coordinates 
       start : i = dimension number
               j = 0
       count : i = 1 
               j = index of fragment in this dimension + 1
    */
    size_t startp[2] = {0,0};
    size_t countp[2] = {1,0};
    /* get max index to create the memory to read the spans into */
    size_t max_index = 0;
    for (int d=0; d<(agg_var->cfa_ndim); d++)
    {
        if (frag->index[d] > max_index)
            max_index = frag->index[d];
    }
    /* allocate the memory to store the max span length */
    int* spans = cfa_malloc(sizeof(int) * (max_index+1));

    for (int d=0; d<(agg_var->cfa_ndim); d++)
    {
        startp[0] = d;
        countp[1] = frag->index[d] + 1;
        int cfa_err = nc_get_vara_int(frag_grp_id, frag_var_id, 
                                      startp, countp, spans);
        CFA_CHECK(cfa_err);
        /* the start location is the spans upto the index */
        int i = d<<1;
        frag->location[i] = 0;
        for (size_t s=0; s<frag->index[d]; s++)
            frag->location[i] += spans[s];
        /* the end location is the start location plus the span at the index */
        frag->location[i+1] = frag->location[i] + spans[frag->index[d]];
    }
    cfa_free(spans, sizeof(int) * (max_index+1));
    
    return CFA_NOERR;
}
#else
int _read_indexed_location(int cfa_id, int cfa_var_id,
                           AggregationVariable* agg_var,
                           Fragment* frag)
{
    /* read the fragment locations from an index into the Fragment array */
    /* this is the "fast" method, which just uses the index multiplied by
    the cfa dimension length divided by the number of fragments */
    size_t span_size = (sizeof(size_t) << 1) * agg_var->cfa_ndim;
    if (frag->location == NULL)
        frag->location = cfa_malloc(span_size);

    AggregatedDimension *agg_dim = NULL;
    FragmentDimension *frag_dim = NULL;
    int err = CFA_NOERR;
    for (int d=0; d<(agg_var->cfa_ndim); d++)
    {
        /* get the span by dividing the dimension length by the fragment 
        dimension length */
        err = cfa_get_dim(cfa_id, agg_var->cfa_dim_idp[d], &agg_dim);
        CFA_CHECK(err);
        err = cfa_var_get_frag_dim(cfa_id, cfa_var_id, d, &frag_dim);
        CFA_CHECK(err);
        int span = agg_dim->length / frag_dim->length;
        /* calculate the locations */
        int i = d << 1;
        /* first location */
        frag->location[i] = frag->index[d] * span;

        /* special case for odd agg_dim->length */
        if (agg_dim->length % 2 != 0 && 
            frag->index[d] == (size_t)(frag_dim->length-1) &&
            frag_dim->length !=1)
            span ++;
        /* last location */
        frag->location[i+1] = frag->location[i] + span;
    }
    return CFA_NOERR;
}
#endif

extern int _linear_index_to_multidim(const AggregationVariable*, int, size_t*);
extern int _cfa_var_assign_datum_to_frag(AggregationVariable *, Fragment *,
                                         const char*, const void*, int);
int 
cfa_netcdf_read1_frag(const int nc_id, 
                      const int cfa_id, const int cfa_var_id, 
                      Fragment *frag)
{
    /* get the variable pointer */
    AggregationVariable *agg_var = NULL;
    int err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_CHECK(err);

    /* create the array to get the fragment indices in and convert the
    linear index to a multidimension index */
    size_t frag_index[MAX_DIMS];
    int cfa_err = _linear_index_to_multidim(agg_var, frag->linear_index,
                                            frag_index);
    CFA_CHECK(cfa_err);

    /* create the memory for the fragment index */
    size_t size = sizeof(size_t) * agg_var->cfa_ndim;
    if (frag->index == NULL)
        frag->index = cfa_malloc(size);
    /* copy the frag location - this is a single index into each 
    FragmentDimension */
    memcpy(frag->index, frag_index, size);

    /* now parse all the fragment information from the netCDF file to the 
    Fragment Structure */
    int frag_var_id;    /* variable and group ids for the variable containing */
    int frag_grp_id;    /* the fragment info */
    /* Read location */

    /* AggInst pointer to be rewritten every loop */
    AggregationInstruction *agg_inst;

    void *data = cfa_malloc(1024);
    for (int i=0; i<agg_var->n_instr; i++)
    {
        agg_inst = &(agg_var->cfa_instr[i]);
        cfa_err = _get_nc_grp_var_ids_from_str(
            nc_id, agg_inst->value, &frag_grp_id, &frag_var_id
        );
        CFA_CHECK(cfa_err);

        /* read the location */
        if (strcmp(agg_inst->term, "location") == 0)
        {
            /* is it scalar? */
            if (agg_inst->scalar)
            {
                cfa_err = _read_scalar_location(frag_grp_id, frag_var_id, frag);
                CFA_CHECK(cfa_err);
            }
            else
            {
#ifndef FAST_INDEX
                cfa_err = _read_indexed_location(frag_grp_id, frag_var_id, 
                                                 agg_var, frag);
#else
                cfa_err = _read_indexed_location(cfa_id, cfa_var_id, 
                                                 agg_var, frag);
#endif
                CFA_CHECK(cfa_err);
            }
        }
        else
        {
            /* Read the FragmentDatum value (of any type) from the fragment in 
            the netCDF file */
            int length = 1;

            /* Use the specific type netCDF functions to read the data */
            switch (agg_inst->type.type)
            {
                case CFA_NAT:
                    return CFA_NAT_ERR;
                case CFA_BYTE:
                    cfa_err = nc_get_var1_schar(
                        frag_grp_id, frag_var_id, frag->index, 
                        (signed char*)data
                    );
                    break;
                case CFA_CHAR:
                    cfa_err = nc_get_var1_uchar(
                        frag_grp_id, frag_var_id, frag->index, 
                        (unsigned char*)data
                    );
                    break;
                case CFA_SHORT:
                    cfa_err = nc_get_var1_short(
                        frag_grp_id, frag_var_id, frag->index, 
                        (short int*)data
                    );
                    break;
                case CFA_INT:   /* Also CFA_LONG */
                    cfa_err = nc_get_var1_int(
                        frag_grp_id, frag_var_id, frag->index, 
                        (int*)data
                    );
                    break;
                case CFA_FLOAT:
                    cfa_err = nc_get_var1_float(
                        frag_grp_id, frag_var_id, frag->index, (float*)data
                    );
                    break;
                case CFA_DOUBLE:
                    cfa_err = nc_get_var1_double(
                        frag_grp_id, frag_var_id, frag->index, (double*)data
                    );
                    break;
                case CFA_UBYTE:
                    cfa_err = nc_get_var1_ubyte(
                        frag_grp_id, frag_var_id, frag->index, 
                        (unsigned char*)data
                    );
                    break;
                case CFA_USHORT:
                    cfa_err = nc_get_var1_ushort(
                        frag_grp_id, frag_var_id, frag->index, 
                        (unsigned short*)data
                    );
                    break;
                case CFA_UINT:
                    cfa_err = nc_get_var1_uint(
                        frag_grp_id, frag_var_id, frag->index, 
                        (unsigned int*)data
                    );
                    break;
                case CFA_INT64:
                    cfa_err = nc_get_var1_longlong(
                        frag_grp_id, frag_var_id, frag->index, 
                        (long long*)data
                    );
                    break;
                case CFA_UINT64:
                    cfa_err = nc_get_var1_ulonglong(
                        frag_grp_id, frag_var_id, frag->index, 
                        (unsigned long long*)data
                    );
                    break;
                case CFA_STRING:
                    cfa_err = nc_get_var1_string(
                        frag_grp_id, frag_var_id, frag->index, 
                        (char**)&data
                    );
                    length = strlen(data) + 1;
                    break;
            };
            CFA_CHECK(cfa_err);
            /* get the length if a string, otherwise length is 1 as above */
            cfa_err = _cfa_var_assign_datum_to_frag(
                agg_var, frag, agg_inst->term, data, length
            );
            CFA_CHECK(cfa_err);
            /* clean up memory allocated by netCDF library */
        }
    }
    cfa_free(data, 1024);
    return CFA_NOERR;
}

/*
load and parse a CFA-netCDF file
*/
int 
parse_cfa_netcdf_file(const char* path, const int ncid, int *cfaidp)
{
    /* check this is a valid CFA-netCDF file by checking the metadata */
    int err = _is_cfa_netcdf_file(ncid);
    CFA_CHECK(err);

    /* create the netCDF-CFA container */
    err = cfa_create(path, CFA_NETCDF, cfaidp);
    CFA_CHECK(err);

    /* enter the recursive parser with the root group / container */
    err = _parse_cfa_container_netcdf(ncid, *cfaidp);
    CFA_CHECK(err);
    
    /* get the root aggregation container and add the external id to it */
    AggregationContainer *agg_cont = NULL;
    err = cfa_get(*cfaidp, &agg_cont);
    CFA_CHECK(err);
    agg_cont->x_id = ncid;

    return CFA_NOERR;
}

/*
write a CFA AggregatedDimension out as a netCDF dimension and variable
*/
int
_serialise_cfa_dimension_netcdf(const int nc_id,
                                const int cfa_id, const int cfa_dimid)
{
    /* get the AggregatedDimension */
    AggregatedDimension *agg_dim = NULL;
    int err = cfa_get_dim(cfa_id, cfa_dimid, &agg_dim);
    CFA_CHECK(err);
    /* 
    create the netCDF dimension with the same size as the AggregatedDimension 
    */
    int nc_dim_id = -1;
    err = nc_def_dim(nc_id, agg_dim->name, agg_dim->length, &nc_dim_id);
    CFA_CHECK(err);
    /*
    create the corresponding dimension variable - we need the data type
    */
    int nc_dimvar_id = -1;
    err = nc_def_var(nc_id, agg_dim->name, agg_dim->type.type, 1,
                     &nc_dim_id, &nc_dimvar_id);
    CFA_CHECK(err);
    return CFA_NOERR;
}

/*
create any aggregation groups needed.  This has to be able to create any 
subgroups in the string, for example:
/aggregation/location
/aggregation/local/location
/group1/group2/group3/aggregation/location
*/
int
_serialise_cfa_fraggrp_netcdf(const int nc_id, const char* agg_inst_var,
                              int* ret_grp_id)
{
    int pgrp_id = nc_id;
    int cfa_err = _get_nc_grp_id_from_str(nc_id, agg_inst_var, &pgrp_id);
    if (cfa_err == NC_ENOGRP)
    {
        /* get the last grpname from agg_inst_var */
        char grpname[STR_LENGTH] = "";
        char pgrpname[STR_LENGTH] = ""; 
        int orig_str_len = strlen(agg_inst_var);
        int strpos = 0;
        cfa_err = -1;
        while(strpos < orig_str_len)
        {
            cfa_err = _get_grp_name_from_str(agg_inst_var + strpos, grpname);
            if (cfa_err != CFA_EOS)
                strcpy(pgrpname, grpname);
            strpos += strlen(grpname) + 1;
        }
        /* define the group */
        cfa_err = nc_def_grp(pgrp_id, pgrpname, ret_grp_id);
        CFA_CHECK(cfa_err); 
    }
    else
        *ret_grp_id = pgrp_id;
    return CFA_NOERR;
}

/*
get the maximum length of the fragment dimensions
*/
int
_get_max_frag_dim_len(AggregationVariable *agg_varp, int *max_frag_dim_len)
{
    *max_frag_dim_len = 0;
    FragmentDimension *frag_dim = NULL;
    int cfa_err;
    for (int d=0; d<agg_varp->cfa_ndim; d++)
    {
        cfa_err = get_array_node(&(cfa_frag_dims), 
                                 agg_varp->cfa_frag_dim_idp[d],
                                 (void**)(&frag_dim));
        CFA_CHECK(cfa_err);
        if (frag_dim->length > *max_frag_dim_len)
            *max_frag_dim_len = frag_dim->length;
    }
    return CFA_NOERR;
}

/*
create any aggregation dimensions needed.  The dimensions will be created in
the group or in the parent group if the create_in_parent flag is set to 1.
Only one set of dimensions will be created in each group, or in the parent if
the flag is set.  Therefore a search is needed to see if the dimension already
exists.
*/
int
_serialise_cfa_fragdims_netcdf(const int nc_id, 
                               const int cfa_id, const int cfa_varid,
                               const int create_in_parent)
{
    /* get the variable */
    AggregationVariable *agg_var = NULL;
    int err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(err);

    /* if the create_in_parent flag is set then get the nc_id of the parent */
    int pnc_id = nc_id;
    if (create_in_parent > 0)
    {
        err = nc_inq_grp_parent(nc_id, &pnc_id);
        CFA_CHECK(err);
    }
    /* define the Dimensions for the file, format and address aggregation 
       definition variables */
    for (int fd=0; fd<agg_var->cfa_ndim; fd++)
    {
        FragmentDimension *frag_dim;
        err = cfa_var_get_frag_dim(cfa_id, cfa_varid, fd, &frag_dim);
        CFA_CHECK(err);
        /* check if a dimension with the frag_dim name has been created yet */
        int nc_dimid = -1;
        err = nc_inq_dimid(pnc_id, frag_dim->name, &nc_dimid);
        /* we want the error to be NC_ENODIM (no dimension found) */
        if (err == NC_EBADDIM)
        {
            err = nc_def_dim(pnc_id, frag_dim->name, frag_dim->length,
                             &nc_dimid);
            CFA_CHECK(err);
        }
    }
    /* define the Dimensions for the location: i and j 
       i is number of dimensions
       j is the maximum length of any of the dimensions
    */
    int nc_iid = -1;
    err = nc_inq_dimid(pnc_id, "i", &nc_iid);
    if (err == NC_EBADDIM)
    {
        err = nc_def_dim(pnc_id, "i", agg_var->cfa_ndim, &nc_iid);
        CFA_CHECK(err);
    }
    int nc_jid = -1;
    err = nc_inq_dimid(pnc_id, "j", &nc_jid);
    if (err == NC_EBADDIM)
    {
        int max_dim_len = 0;
        err = _get_max_frag_dim_len(agg_var, &max_dim_len);
        CFA_CHECK(err);
        err = nc_def_dim(pnc_id, "j", max_dim_len, &nc_jid);
        CFA_CHECK(err);
    }
    return CFA_NOERR;
}

/* 
write out a netCDF variable to store the information for one part of a 
fragment, i.e. for the location, file, format or address 
*/
int
_serialise_cfa_fragvar_netcdf(const int nc_id, 
                              const int cfa_id, const int cfa_varid, 
                              const char *agg_inst_name, 
                              const char *agg_inst_var, 
                              int *frag_varid)
{
    AggregationVariable *agg_var = NULL;
    int err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(err);

    /* get the netCDF dimension ids for the FragmentDimensions */
    int nc_fragdimids[MAX_DIMS];
    FragmentDimension *frag_dim = NULL;

    AggregationInstruction *pinst;
    err = cfa_var_get_agg_instr(cfa_id, cfa_varid, agg_inst_name, &pinst);
    CFA_CHECK(err);

    /* get the actual name of the variable, outside of the group */
    char agg_var_name[STR_LENGTH] = "";
    err = _get_var_name_from_str(agg_inst_var, agg_var_name);
    CFA_CHECK(err);

    if (pinst->scalar)
    {
        /* create the variable */
        err = nc_def_var(nc_id, agg_var_name, pinst->type.type, 0, 
                         NULL, frag_varid);
        CFA_CHECK(err);
        return CFA_NOERR;
    }
    /* Location is different, as it has two scalar dimensions:
        i = length of the number of dimensions
        j = always maximum length of a dimension
    */
    if (strcmp(agg_inst_name, "location") == 0)
    {
        /* check if i and j dimension has been created yet */
        int nc_loc_dim_ids[2];
        err = nc_inq_dimid(nc_id, "i", &(nc_loc_dim_ids[0]));
        CFA_CHECK(err);
        err = nc_inq_dimid(nc_id, "j", &(nc_loc_dim_ids[1]));
        CFA_CHECK(err);
        /* create the location variable */
        err = nc_def_var(nc_id, agg_var_name, pinst->type.type, 2, 
                         nc_loc_dim_ids, frag_varid);
        CFA_CHECK(err);
    }
    else
    {
        for (int fd=0; fd<agg_var->cfa_ndim; fd++)
        {
            err = cfa_var_get_frag_dim(cfa_id, cfa_varid, fd, &frag_dim);
            CFA_CHECK(err);
            /* check if a dimension with the frag_dim name has been created yet 
            */
            err = nc_inq_dimid(nc_id, frag_dim->name, &(nc_fragdimids[fd]));
            CFA_CHECK(err);
        }
        /* create the variable */
        err = nc_def_var(nc_id, agg_var_name, pinst->type.type, 
                         agg_var->cfa_ndim, nc_fragdimids, frag_varid);
        CFA_CHECK(err);
    }

    return CFA_NOERR;
}

/*
write out a single fragment - this can be called from either 
_serialise_cfa_fragments_netcdf below, as part of the serialisation, or
cfa_var_put1_frag if the serialisation has already taken place
*/
extern int _cfa_var_get_frag_datum(const Fragment*, const char*,
                                   const FragmentDatum**);
int
cfa_netcdf_write1_frag(const int nc_id, 
                       const int cfa_id, const int cfa_varid,
                       const Fragment *frag)
{
    /* check whether the input Fragment is defined */
    if (!(frag->location))
        return CFA_VAR_NO_FRAG;

    /* get the cfa variable */
    AggregationVariable *agg_var;
    int cfa_err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(cfa_err);

    int grp_id = -1;
    int var_id = -1;

    /* write out one element in the file variable */
    /* get the netCDF group and variable id from the long name with groups
    compounded in it */
    AggregationInstruction *agg_inst;
    const FragmentDatum *frag_dat;
    for (int i=0; i<agg_var->n_instr; i++)
    {
        agg_inst = &(agg_var->cfa_instr[i]);
        /* get the FragmentDatum from the Fragment using the term from the
           AggregationInstruction */
        cfa_err = _cfa_var_get_frag_datum(frag, agg_inst->term, &frag_dat);
        if (cfa_err == CFA_NOERR)
        {
            cfa_err = _get_nc_grp_var_ids_from_str(
                nc_id, agg_inst->value, &grp_id, &var_id
            );
            CFA_CHECK(cfa_err);
            /* Use the specific type netCDF functions to write the data */
            switch (agg_inst->type.type)
            {
                case CFA_NAT:
                    return CFA_NAT_ERR;
                case CFA_BYTE:
                    cfa_err = nc_put_var1_schar(
                        grp_id, var_id, frag->index, 
                        (signed char*)(frag_dat->data)
                    );
                    break;
                case CFA_CHAR:
                    cfa_err = nc_put_var1_uchar(
                        grp_id, var_id, frag->index,
                        (unsigned char*)(frag_dat->data)
                    );
                    break;
                case CFA_SHORT:
                    cfa_err = nc_put_var1_short(
                        grp_id, var_id, frag->index, 
                        (short int*)(frag_dat->data)
                    );
                    break;
                case CFA_INT:   /* Also CFA_LONG */
                    cfa_err = nc_put_var1_int(
                        grp_id, var_id, frag->index, 
                        (int*)(frag_dat->data)
                    );
                    break;
                case CFA_FLOAT:
                    cfa_err = nc_put_var1_float(
                        grp_id, var_id, frag->index, 
                        (float*)(frag_dat->data)
                    );
                    break;
                case CFA_DOUBLE:
                    cfa_err = nc_put_var1_double(
                        grp_id, var_id, frag->index, 
                        (double*)(frag_dat->data)
                    );
                    break;
                case CFA_UBYTE:
                    cfa_err = nc_put_var1_ubyte(
                        grp_id, var_id, frag->index, 
                        (unsigned char*)(frag_dat->data)
                    );
                    break;
                case CFA_USHORT:
                    cfa_err = nc_put_var1_ushort(
                        grp_id, var_id, frag->index, 
                        (unsigned short*)(frag_dat->data)
                    );
                    break;
                case CFA_UINT:
                    cfa_err = nc_put_var1_uint(
                        grp_id, var_id, frag->index, 
                        (unsigned int*)(frag_dat->data)
                    );
                    break;
                case CFA_INT64:
                    cfa_err = nc_put_var1_longlong(
                        grp_id, var_id, frag->index, 
                        (long long*)(frag_dat->data)
                    );
                    break;
                case CFA_UINT64:
                    cfa_err = nc_put_var1_ulonglong(
                        grp_id, var_id, frag->index, 
                        (unsigned long long*)(frag_dat->data)
                    );
                    break;
                case CFA_STRING:
                    cfa_err = nc_put_var1_string(
                        grp_id, var_id, frag->index, 
                        (const char**)(&(frag_dat->data))
                    );
                    break;
            };
            CFA_CHECK(cfa_err);
        }
    }
    return CFA_NOERR;
}

/*
write out the location variable - this is a special case as it contains the
spans of the fragments in a 2-D array
*/
int
_write_cfa_fragloc_netcdf(const int loc_grpid, 
                          const int cfa_id, const int cfa_varid)
{
    /* get the AggregationVariable */
    AggregationVariable *agg_var = NULL;
    int err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(err);

    /* get the "location" AggregationInstruction */
    AggregationInstruction *pinst;
    err = cfa_var_get_agg_instr(cfa_id, cfa_varid, "location", &pinst);
    CFA_CHECK(err);
    /* get the actual name of the variable, outside of the group */
    char agg_var_name[STR_LENGTH] = "";
    err = _get_var_name_from_str(pinst->value, agg_var_name);
    CFA_CHECK(err);

    /* get the variable id for the aggregation instruction variable name */
    int nc_varid = -1;
    
    err = nc_inq_varid(loc_grpid, agg_var_name, &nc_varid);
    CFA_CHECK(err);

    /* loop over each dimension */
    AggregatedDimension *agg_dim = NULL;
    FragmentDimension *frag_dim = NULL;
    /* get the maximum size of the j dimensions */
    int max_frag_dim_len = -1;
    err = _get_max_frag_dim_len(agg_var, &max_frag_dim_len);
    CFA_CHECK(err);
    /* create an array containing the spans */
    int frag_span[max_frag_dim_len];

    for (int d=0; d<agg_var->cfa_ndim; d++)
    {
        /* get the span by dividing the dimension length by the fragment 
        dimension length */
        err = cfa_get_dim(cfa_id, agg_var->cfa_dim_idp[d], &agg_dim);
        CFA_CHECK(err);
        err = cfa_var_get_frag_dim(cfa_id, cfa_varid, d, &frag_dim);
        CFA_CHECK(err);
        int span = agg_dim->length / frag_dim->length;
        /* write out each frag span into the array */
        for (int f=0; f<frag_dim->length; f++)
        {
            frag_span[f] = span;
            /* special case for odd agg_dim->length */
            if (agg_dim->length % 2 != 0 && 
                f == frag_dim->length-1 &&
                frag_dim->length !=1)
                frag_span[f] ++;
        }
        /* write the frag_span */
        size_t c_pos[2] = {(size_t)(d), 0};
        size_t c_span[2] = {1, (size_t)(frag_dim->length)};
        err = nc_put_vara_int(loc_grpid, nc_varid, 
                              c_pos, c_span, frag_span);
        CFA_CHECK(err);
    }

    return CFA_NOERR;
}

/*
write out the CFA Fragments - these are variables in the netCDF file, that may
be contained within a group
*/
int
_serialise_cfa_fragments_netcdf(const int nc_id,
                                const int cfa_id, const int cfa_varid)
{
    /* This function creates the variables mentioned in the aggregation 
    instructions */
    AggregationVariable *agg_var = NULL;
    int err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(err);

    for (int i=0; i<agg_var->n_instr; i++)
    {
        AggregationInstruction *pinst = &(agg_var->cfa_instr[i]);
        /****** LOCATION ******/
        if (strcmp(pinst->term, "location") == 0)
        {
            int loc_grpid = -1;    
            err = _serialise_cfa_fraggrp_netcdf(
                nc_id, pinst->value, &loc_grpid
            );
            CFA_CHECK(err);
            err = _serialise_cfa_fragdims_netcdf(
                loc_grpid, cfa_id, cfa_varid, 0
            );
            CFA_CHECK(err);
            int loc_varid = -1;
            err = _serialise_cfa_fragvar_netcdf(
                loc_grpid, cfa_id, cfa_varid, pinst->term, pinst->value, 
                &loc_varid
            );
            CFA_CHECK(err);
            err = _write_cfa_fragloc_netcdf(loc_grpid, cfa_id, cfa_varid);
            CFA_CHECK(err);
        }
        else
        {
            int grpid = -1;
            err = _serialise_cfa_fraggrp_netcdf(nc_id, pinst->value, &grpid);
            CFA_CHECK(err);
            err = _serialise_cfa_fragdims_netcdf(grpid, cfa_id, cfa_varid, 0);
            CFA_CHECK(err);
            int varid = -1;
            err = _serialise_cfa_fragvar_netcdf(
                grpid, cfa_id, cfa_varid, pinst->term, pinst->value, &varid
            );
            CFA_CHECK(err);            
        }
    }

    /* See if any Fragments have been added yet and write them out if they have
    */
    int nfrags = 0;
    err = get_array_length(&(agg_var->cfa_datap->cfa_fragmentsp), &nfrags);
    CFA_CHECK(err);
    Fragment *frag;
    for (int f=0; f<nfrags; f++)
    {
        err = get_array_node(&(agg_var->cfa_datap->cfa_fragmentsp),
                             f, (void**)(&frag));
        CFA_CHECK(err);
        if (frag->location)
        {
            err = cfa_netcdf_write1_frag(nc_id, cfa_id, cfa_varid, frag);
            CFA_CHECK(err);
        }
    }

    return CFA_NOERR;
}

/*
write out the AggregationInstructions and AggregationDimensions for a netCDF
variable - these are attributes (metadata)
*/

int
_serialise_cfa_aggregation_instructions(int nc_id, int nc_varid,
                                        int cfa_id, int cfa_varid)
{
    AggregationVariable *agg_var = NULL;
    int err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(err);

    /* add the AggregatedDimensions as an attribute */
    AggregatedDimension *agg_dim = NULL;
    char agg_dim_names[STR_LENGTH*4] = "";
    for (int d=0; d<agg_var->cfa_ndim; d++)
    {
        err = cfa_get_dim(cfa_id, agg_var->cfa_dim_idp[d], &agg_dim);
        CFA_CHECK(err);
        strcat(agg_dim_names, agg_dim->name);
        strcat(agg_dim_names, " ");
    }
    /* remove the final space */
    agg_dim_names[strlen(agg_dim_names)-1] = '\0';
    /* write the AggregationDimension attribute */
    err = nc_put_att_text(nc_id, nc_varid, AGGREGATED_DIMENSIONS,
                            strlen(agg_dim_names)+1, agg_dim_names);
    CFA_CHECK(err);
    /* check there are all the standard AggregationInstructions */
    if (!_has_standard_agg_instr(cfa_id, cfa_varid))
        return CFA_AGG_NOT_DEFINED;
    /* write the AggregationInstructions as an attribute */
    char agg_instr[STR_LENGTH*8] = "";
    for (int i=0; i<agg_var->n_instr; i++)
    {
        AggregationInstruction *pinstr = &(agg_var->cfa_instr[i]);
        strcat(agg_instr, pinstr->term);
        strcat(agg_instr, ": ");
        strcat(agg_instr, pinstr->value);
        strcat(agg_instr, " ");
    }
    /* remove the final space and set to string terminator */
    agg_instr[strlen(agg_instr)-1] = '\0';
    /* write the netCDF attribute */
    err = nc_put_att_text(nc_id, nc_varid, AGGREGATED_DATA,
                          strlen(agg_instr)+1, agg_instr);
    CFA_CHECK(err);
    return CFA_NOERR;
}
/*
write a CFA AggregationVariables out as a netCDF variable
*/
int
_serialise_cfa_variable_netcdf(const int nc_id, 
                              const int cfa_id, const int cfa_varid)
{
    /* get the AggregationVariable */
    AggregationVariable *agg_var = NULL;
    int err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(err);

    /* create the variable with no dimensions, as per the CFA spec */
    int nc_varid = -1;
    err = nc_def_var(nc_id, agg_var->name, agg_var->cfa_dtype.type, 0, NULL,
                     &nc_varid);
    CFA_CHECK(err);
    /* create the fragment variables if there are any */
    if (agg_var->cfa_datap && agg_var->cfa_datap->cfa_fragmentsp)
    {
        err = _serialise_cfa_fragments_netcdf(nc_id, cfa_id, cfa_varid);
        CFA_CHECK(err);
    }
    /* add the aggregation instructions, if they exist */
    if (agg_var->n_instr > 0)
    {
        err = _serialise_cfa_aggregation_instructions(nc_id, nc_varid,
                                                      cfa_id, cfa_varid);
        CFA_CHECK(err);
    }
  
    return CFA_NOERR;
}

/*
write a CFA container out as a netCDF group
*/
int
_serialise_cfa_container_netcdf(const int nc_id, const int cfa_id)
{
    /* serialise the containers */
    int nconts = 0;
    int *cont_ids;
    /* get the number of containers and their ids */
    int err = cfa_inq_nconts(cfa_id, &nconts);
    CFA_CHECK(err);
    err = cfa_inq_cont_ids(cfa_id, &cont_ids);
    CFA_CHECK(err);

    /* pointer to container required inside loop */
    AggregationContainer *agg_cont = NULL;

    for (int g=0; g<nconts; g++)
    {
        /* get the CFA container */
        err = cfa_get_cont(cfa_id, cont_ids[g], &agg_cont);
        CFA_CHECK(err);
        /* create a netCDF group for this container */
        int nc_grpid = -1;
        err = nc_def_grp(nc_id, agg_cont->name, &nc_grpid);
        CFA_CHECK(err);
        /* recursively serialise the container */
        err = _serialise_cfa_container_netcdf(nc_grpid, cont_ids[g]);
        CFA_CHECK(err);
    }
    /* get the parent container */
    err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(err);
    /* serialise the dimensions in this container */
    for (int d=0; d<agg_cont->n_dims; d++)
    {
        err = _serialise_cfa_dimension_netcdf(nc_id, cfa_id, 
                                             agg_cont->cfa_dimids[d]);
        CFA_CHECK(err);
    }

    /* serialise the variables in this container */
    for (int v=0; v<agg_cont->n_vars; v++)
    {
        err = _serialise_cfa_variable_netcdf(nc_id, cfa_id, 
                                            agg_cont->cfa_varids[v]);
        CFA_CHECK(err);
    }

    return CFA_NOERR;
}

/*
write cfa structures into existing netCDF file
*/
int
serialise_cfa_netcdf_file(const int ncid, const int cfa_id)
{
    /* get the container to get the external id (x_id) */
    AggregationContainer *agg_cont = NULL;
    int err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(err);
    /* check the file has been created - if not then create it */
    if (agg_cont->x_id == -1)
    {
        switch (agg_cont->format)
        {
            case CFA_NETCDF:
                agg_cont->x_id = ncid;
                break;
            default:
                return CFA_UNKNOWN_FILE_FORMAT;
        }
    }

    /* enter the recursive writer with the root group / container */
    err = _serialise_cfa_container_netcdf(agg_cont->x_id, cfa_id);
    CFA_CHECK(err);

    /* add the conventions global metadata */
    char conventions[STR_LENGTH] = "";
    /* CFA-n.m */
    strcat(conventions, CFA_CONVENTION);
    char version[6];
    sprintf(version, "%i.%i.%i", 
            CFA_MAJOR_VERSION, CFA_MINOR_VERSION, CFA_REVISION
        );
    strcat(conventions, version);
    /* write the netCDF attribute */
    err = nc_put_att_text(agg_cont->x_id, NC_GLOBAL, CONVENTIONS,
                          strlen(conventions)+1, conventions);
    CFA_CHECK(err);

    return CFA_NOERR;
}