#include <netcdf.h>
#include <stdio.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"
#include "parsers/cfa_netcdf.h"

#define MAX_GROUPS 256
#define STR_LENGTH 256

#define AGGREGATED_DATA "aggregated_data"
#define AGGREGATED_DIMENSIONS "aggregated_dimensions"

/*
check this is a CFA-netCDF file
*/
int
is_netcdf_cfa_file(const int ncid)
{
    /* determine if this is a CFA file by searching for the string "CFA-0.6" 
       in the Conventions global attribute */
    size_t att_len;
    int err = nc_inq_att(ncid, NC_GLOBAL, "Conventions", NULL, &att_len);
    CFA_CHECK(err);

    /* allocate the memory and read in the Convention string */
    char *convention = cfa_malloc(att_len+1);
    err = nc_get_att(ncid, NC_GLOBAL, "Conventions", convention);
    CFA_CHECK(err);

    /* check that CFA_CONVENTION is a substring in convention */
    if (!strstr(convention, CFA_CONVENTION))
        return CFA_NOT_CFA_FILE;

    /* check that CFA_VERSION is a substring in convention */
    if (!strstr(convention, CFA_VERSION))
        return CFA_UNSUPPORTED_VERSION;

    cfa_free(convention, att_len+1);
    return CFA_NOERR;
}

/* 
check if a variable is a Data variable
*/
int
is_cfa_data_variable(const int ncid, const int ncvarid, int *iscfavarp)
{
    /* CFA Data variables have the attribute "aggregated_data" */
    int nvaratt = 0;
    int err = nc_inq_varnatts(ncid, ncvarid, &nvaratt);
    CFA_CHECK(err);
    char attname[STR_LENGTH];
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
is_cfa_data_group(const int ncid, int *iscfagrpp)
{
    /* a CFA Data group will have at least one CFA Data variable in it */
    int nvars = 0;
    int varidsp[NC_MAX_VARS];
    int err = nc_inq_varids(ncid, &nvars, varidsp);
    int ncfa_vars = 0;
    int iscfavar = 0;
    for (int v=0; v<nvars; v++)
    {
        err = is_cfa_data_variable(ncid, varidsp[v], &iscfavar);
        CFA_CHECK(err);
        ncfa_vars += iscfavar;
    }
    /* if more than one CFA Variable then iscfagrp = True */
    *iscfagrpp = (ncfa_vars > 0);
    return CFA_NOERR;
}

/*
get the aggregation instructions from an attribute string
*/
int
parse_aggregation_instructions(const int ncid, const int ncvarid,
                               const int cfa_id, const int cfa_var_id)
{
    /* read the "aggregated_data" attribute - these are the aggregation 
    instructions */
    size_t att_len = 0;
    int err = nc_inq_attlen(ncid, ncvarid, AGGREGATED_DATA, &att_len);
    CFA_CHECK(err);
    char *att_str = cfa_malloc(att_len+1);
    err = nc_get_att_text(ncid, ncvarid, AGGREGATED_DATA, att_str);
    CFA_CHECK(err);

    /* parse the "aggregated_data" attribute, getting each key:value pair */
    char *strp = strtok(att_str, " ");
    char key[STR_LENGTH];
    char value[STR_LENGTH];
    int n_agg_instr = 0;
    /* extract key: value pairs */
    while(strp != NULL)
    {
        strcpy(key, strp);
        strstrip(key);
        strp = strtok(NULL, " ");
        if (strp != NULL)
        {
            strcpy(value, strp);
            strstrip(value);
            strp = strtok(NULL, " ");
            /* define the aggregation instructions in the cfa var */
            err = cfa_var_def_agg_instr(cfa_id, cfa_var_id, key, value);
            CFA_CHECK(err);
            if (!err)
               n_agg_instr++;
        }
    }
    AggregationVariable *cfa_var = NULL;
    err = cfa_get_var(cfa_id, cfa_var_id, &cfa_var);
    CFA_CHECK(err);
    /* check that four AggregationInstructions have been added */
    if (n_agg_instr != 4)
        return CFA_AGG_DATA_ERR;

    cfa_free(att_str, att_len+1);
    return CFA_NOERR;
}

int
parse_aggregated_dimensions(const int ncid, const int ncvarid, 
                            const int cfa_id, const int cfa_var_id)
{
    /* read the "aggregated dimensions" attribute */
    size_t att_len = 0;
    int err = nc_inq_attlen(ncid, ncvarid, AGGREGATED_DIMENSIONS, &att_len);
    CFA_CHECK(err);
    char *att_str = cfa_malloc(att_len+1);
    err = nc_get_att_text(ncid, ncvarid, AGGREGATED_DIMENSIONS, att_str);
    CFA_CHECK(err);
    /* split on space */
    char *strp = strtok(att_str, " ");
    /* process the split string into dimname and either create or get the dimid
    */
    char dimname[STR_LENGTH];
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
            size_t ncdimlen = 0;
            err = nc_inq_dimid(ncid, dimname, &ncdimid);
            CFA_CHECK(err);
            err = nc_inq_dimlen(ncid, ncdimid, &ncdimlen);
            CFA_CHECK(err);
            /* create the AggregatedDimension */
            err = cfa_def_dim(cfa_id, dimname, ncdimlen, &cfa_dim_id);
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
    cfa_free(att_str, att_len+1);
    return CFA_NOERR;
}

/*
parse an individual variable
*/
int 
parse_netcdf_cfa_variable(const int ncid, const int ncvarid, const int cfa_id)
{
    /* get the name of the variable */
    char varname[NC_MAX_NAME];
    int err = nc_inq_varname(ncid, ncvarid, varname);
    CFA_CHECK(err);
    /* define the variable */
    int cfa_var_id = -1;
    err = cfa_def_var(cfa_id, varname, &cfa_var_id);
    CFA_CHECK(err);
    /* get the aggregation instructions */
    err = parse_aggregation_instructions(ncid, ncvarid, cfa_id, cfa_var_id);
    CFA_CHECK(err);
    /* get the aggregated dimensions */
    err = parse_aggregated_dimensions(ncid, ncvarid, cfa_id, cfa_var_id);
    CFA_CHECK(err);

    return CFA_NOERR;
}

/* 
entry point for the recursive parser - parse a single group. On first entry
this should be the root container (group)
*/
int
parse_netcdf_cfa_container(int ncid, int cfa_id)
{
    /* parse the groups in this group / root */
    /* get the number of groups in the netCDF file or group */
    int grp_idp[MAX_GROUPS];
    char grp_name[NC_MAX_NAME];
    int grp_name_len = 1;
    int ngrp = 0;
    int err = nc_inq_grps(ncid, &ngrp, grp_idp);
    CFA_CHECK(err);

    /* loop over the groups */
    for (int g=0; g<ngrp; g++)
    {
        /* check first if this is a CFA group - has CFA Variables defined in it 
        */
        int is_cfa_grp = 0;
        err = is_cfa_data_group(grp_idp[g], &is_cfa_grp);
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
        err = parse_netcdf_cfa_container(grp_idp[g], cfa_cont_id);
        CFA_CHECK(err);
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
        err = is_cfa_data_variable(ncid, v, &iscfavar);
        CFA_CHECK(err);
        if (iscfavar)
        {
            err = parse_netcdf_cfa_variable(ncid, v, cfa_id);
            CFA_CHECK(err);
        }
    }

    return CFA_NOERR; 
}

/*
load and parse a CFA-netCDF file
*/
int 
parse_netcdf_cfa_file(const char *path, int *cfa_idp)
{
    /* open the file first, using the netCDF library, in read only mode */
    int ncid = -1;
    int err = nc_open(path, NC_NOWRITE, &ncid);
    CFA_CHECK(err);
    /* check this is a valid CFA-netCDF file by checking the metadata */
    err = is_netcdf_cfa_file(ncid);
    CFA_CHECK(err);

    /* create the netCDF-CFA container */
    err = cfa_create(path, cfa_idp);
    CFA_CHECK(err);

    /* enter the recursive parser with the root group / container */
    err = parse_netcdf_cfa_container(ncid, *cfa_idp);
    CFA_CHECK(err);

    /* close the file */
    err = nc_close(ncid);
    CFA_CHECK(err);
    
    return CFA_NOERR;
}
