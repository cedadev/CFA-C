#include <netcdf.h>
#include <stdio.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"
#include "parsers/cfa_netcdf.h"

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
    char* convention = cfa_malloc(att_len*sizeof(char));
    err = nc_get_att(ncid, NC_GLOBAL, "Conventions", convention);
    CFA_CHECK(err);

    /* check that CFA_CONVENTION is a substring in convention */
    if (!strstr(convention, CFA_CONVENTION))
        return CFA_NOT_CFA_FILE;

    cfa_free(convention, att_len*sizeof(char));
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
    char attname[256];
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
get the aggregation instructions from an attribute string
*/
int
parse_aggregation_instructions(const int ncid, const int varid,
                               const int cfa_id, const int cfa_var_id)
{
    /* read the "aggregated_data" attribute - these are the aggregation 
    instructions */
    size_t att_len = 0;
    int err = nc_inq_attlen(ncid, varid, AGGREGATED_DATA, &att_len);
    CFA_CHECK(err);
    char *att_str = cfa_malloc(att_len*sizeof(char));
    err = nc_get_att_text(ncid, varid, AGGREGATED_DATA, att_str);
    CFA_CHECK(err);
    err = cfa_var_def_agg_instr(cfa_id, cfa_var_id, att_str);
    CFA_CHECK(err);

    cfa_free(att_str, att_len*sizeof(char));
    return CFA_NOERR;
}

/*
parse an individual variable
*/
int 
parse_netcdf_cfa_variable(const int ncid, const int ncvarid, const int cfa_id)
{
    /* get the name of the variable */
    char varname[256];
    int err = nc_inq_varname(ncid, ncvarid, varname);
    CFA_CHECK(err);
    /* define the variable */
    int cfa_var_id = -1;
    err = cfa_def_var(cfa_id, varname, &cfa_var_id);
    CFA_CHECK(err);
    /* get the aggregation instructions */
    err = parse_aggregation_instructions(ncid, ncvarid, cfa_id, cfa_var_id);
    CFA_CHECK(err);

    return CFA_NOERR;
}

/*
parse the variables from an already opened netCDF-CFA file or group and add to
the CFA container with cfa_id
*/
int 
parse_netcdf_cfa_variables(const int ncid, const int cfa_id)
{
    /* get the number of variables */
    int nvar = 0;
    int err = nc_inq_nvars(ncid, &nvar);
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
parse the groups from an already opened netCDF-CFA file and add to the CFA
container with cfa_id
*/
int 
parse_netcdf_cfa_groups(const int ncid, const int cfa_id)
{
    int ngrp = 0;
    /* get the number of groups in the netCDF file */
    int err = nc_inq_grps(ncid, &ngrp, NULL);
    CFA_CHECK(err);
    /* get the group ids */
    int *grp_idp = cfa_malloc(sizeof(int) * ngrp);
    err = nc_inq_grps(ncid, &ngrp, grp_idp);
    CFA_CHECK(err);

    /* loop over the groups */
    for (int g=0; g<ngrp; g++)
    {
        /* parse each variable in the group */
        err = parse_netcdf_cfa_variables(grp_idp[g], grp_idp[g]);
        CFA_CHECK(err);
    }

    cfa_free(grp_idp, sizeof(int) * ngrp);
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

    /* parse the groups */
    err = parse_netcdf_cfa_groups(ncid, *cfa_idp);
    CFA_CHECK(err);

    /* parse the variables in the root group */
    err = parse_netcdf_cfa_variables(ncid, *cfa_idp);
    CFA_CHECK(err);

    /* close the file */
    err = nc_close(ncid);
    CFA_CHECK(err);
    
    return CFA_NOERR;
}
