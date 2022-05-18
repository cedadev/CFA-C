#include <netcdf.h>
#include <stdio.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"
#include "parsers/cfa_netcdf.h"

#define MAX_GROUPS 256
#define STR_LENGTH 256

extern DynamicArray *cfa_frag_dims;

/*
check this is a CFA-netCDF file
*/
int
is_netcdf_cfa_file(const int ncid)
{
    /* determine if this is a CFA file by searching for the string "CFA-0.6" 
       in the Conventions global attribute */

    /* allocate the string and read in the Convention string */
    char convention[STR_LENGTH];
    int err = nc_get_att(ncid, NC_GLOBAL, CONVENTIONS, convention);
    if (err == NC_ENOTATT)
        return CFA_NOT_CFA_FILE;
    else
        CFA_CHECK(err);

    /* check that CFA_CONVENTION is a substring in convention */
    if (!strstr(convention, CFA_CONVENTION))
        return CFA_NOT_CFA_FILE;

    /* check that CFA_VERSION is a substring in convention */
    if (!strstr(convention, CFA_VERSION))
        return CFA_UNSUPPORTED_VERSION;
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
    char att_str[STR_LENGTH];
    int err = nc_get_att_text(ncid, ncvarid, AGGREGATED_DATA, att_str);
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
            /*err = cfa_var_def_agg_instr(cfa_id, cfa_var_id, key, value);
            CFA_CHECK(err);*/
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
    return CFA_NOERR;
}

int
parse_aggregated_dimensions(const int ncid, const int ncvarid, 
                            const int cfa_id, const int cfa_var_id)
{
    /* read the "aggregated dimensions" attribute */
    char att_str[STR_LENGTH];
    int err = nc_get_att_text(ncid, ncvarid, AGGREGATED_DIMENSIONS, att_str);
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
parse an individual variable
*/
int 
parse_netcdf_cfa_variable(const int ncid, const int ncvarid, const int cfa_id)
{
    /* get the name of the variable */
    char varname[NC_MAX_NAME];
    nc_type vtype = NC_NAT;      /* NC type and CFA type are analogous */
    int err = nc_inq_varname(ncid, ncvarid, varname);
    CFA_CHECK(err);
    err = nc_inq_vartype(ncid, ncvarid, &vtype);
    /* define the variable */
    int cfa_var_id = -1;
    err = cfa_def_var(cfa_id, varname, vtype, &cfa_var_id);
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
parse_cfa_netcdf_file(const char *path, int *cfa_idp)
{
    /* open the file first, using the netCDF library, in read only mode */
    int ncid = -1;
    int err = nc_open(path, NC_NOWRITE, &ncid);
    CFA_CHECK(err);
    /* check this is a valid CFA-netCDF file by checking the metadata */
    err = is_netcdf_cfa_file(ncid);
    CFA_CHECK(err);

    /* create the netCDF-CFA container */
    err = cfa_create(path, CFA_NETCDF, cfa_idp);
    CFA_CHECK(err);

    /* enter the recursive parser with the root group / container */
    err = parse_netcdf_cfa_container(ncid, *cfa_idp);
    CFA_CHECK(err);

    /* close the file */
    err = nc_close(ncid);
    CFA_CHECK(err);
    
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
    err = nc_def_var(nc_id, agg_dim->name, agg_dim->cfa_dtype.type, 1,
                     &nc_dim_id, &nc_dimvar_id);
    CFA_CHECK(err);
    return CFA_NOERR;
}


/*
get a group name from a string
*/
int
_get_grp_name_from_str(const char *in_str, char *out_grp_name)
{
    char str[STR_LENGTH];
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
get a netcdf group id from a string
*/
int
_get_nc_grp_id_from_str(const int nc_id, const char *in_str, const int create,
                        int *ret_grp_id)
{
    /* get the group id in a netCDF from a variable string that may contain
    various groups, e.g. /group1/group2/var1
    */
    char grpname[STR_LENGTH];
    int orig_str_len = strlen(in_str);
    int strpos = 0;
    int pgrp_id = nc_id;
    int cfa_err = CFA_NOERR;

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
                /* if the error is no group found then create it if requested */
                if (grp_err == NC_ENOGRP && create)
                {
                    grp_err = nc_def_grp(pgrp_id, grpname, &grp_id);
                    CFA_CHECK(grp_err);
                    /* any subsequent groups will be created as children of 
                    this group */
                    pgrp_id = grp_id;
                }
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
    int cfa_err = _get_nc_grp_id_from_str(nc_id, in_str, 0, ret_grp_id);
    CFA_CHECK(cfa_err);
    /* get the varname */
    char var_name[STR_LENGTH];
    cfa_err = _get_var_name_from_str(in_str, var_name);
    CFA_CHECK(cfa_err);
    /* get the netCDF var id */
    cfa_err = nc_inq_varid(*ret_grp_id, var_name, ret_var_id);
    CFA_CHECK(cfa_err);
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
                              int *ret_grp_id)
{
    return _get_nc_grp_id_from_str(nc_id, agg_inst_var, 1, ret_grp_id);
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
get the type of the Aggregation Instruction variable
*/
int
_get_agg_inst_type(const char* agg_inst_name, nc_type *agg_inst_type)
{
    if (strcmp("location", agg_inst_name) == 0)
    {
        *agg_inst_type = NC_INT;
        return CFA_NOERR;
    }
    if (strcmp("file", agg_inst_name) == 0)
    {
        *agg_inst_type = NC_STRING;
        return CFA_NOERR;
    }
    if (strcmp("address", agg_inst_name) == 0)
    {
        *agg_inst_type = NC_STRING;
        return CFA_NOERR;
    }
    if (strcmp("format", agg_inst_name) == 0)
    {
        *agg_inst_type = NC_STRING;
        return CFA_NOERR;
    }
    return CFA_AGG_NOT_RECOGNISED;
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
    static int nc_fragdimids[MAX_DIMS];
    FragmentDimension *frag_dim = NULL;

    nc_type nc_dtype = -1;
    /* get the fragment variable name, without the group prefix */
    err = _get_agg_inst_type(agg_inst_name, &nc_dtype);
    CFA_CHECK(err);

    /* check whether Aggregation Instruction is a scaler */
    int scaler = 0;
    if (strcmp(agg_inst_name, "format") == 0 && 
        agg_var->cfa_instructionsp->format_scaler != 0)
        scaler = 1;
    if (strcmp(agg_inst_name, "location") == 0 &&
        agg_var->cfa_instructionsp->location_scaler != 0)
        scaler = 1;

    /* get the actual name of the variable, outside of the group */
    char agg_var_name[STR_LENGTH];
    err = _get_var_name_from_str(agg_inst_var, agg_var_name);
    CFA_CHECK(err);

    if (scaler)
    {
        /* create the variable */
        err = nc_def_var(nc_id, agg_var_name, nc_dtype, 0, NULL, frag_varid);
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
        err = nc_def_var(nc_id, agg_var_name, nc_dtype, 2, nc_loc_dim_ids,
                         frag_varid);
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
        err = nc_def_var(nc_id, agg_var_name, nc_dtype, agg_var->cfa_ndim,
                         nc_fragdimids, frag_varid);
        CFA_CHECK(err);
    }

    return CFA_NOERR;
}

/*
write out a single fragment - this can be called from either 
_serialise_cfa_fragments_netcdf below, as part of the serialisation, or
cfa_var_put1_frag if the serialisation has already taken place
*/
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
    CFA_ERR(cfa_err);

    int grp_id = -1;
    int var_id = -1;

    /* write out one element in the file variable */
    /* get the netCDF group and variable id from the long name with groups
    compounded in it */
    if (frag->file)
    {
        cfa_err = _get_nc_grp_var_ids_from_str(
            nc_id, 
            agg_var->cfa_instructionsp->file,
            &grp_id, &var_id
        );
        CFA_CHECK(cfa_err);
        cfa_err = nc_put_var1_string(grp_id, var_id, frag->index,
                                     (const char**)(&(frag->file)));
        CFA_CHECK(cfa_err);
    }

    /* write out one element in the format variable */
    if (frag->format)
    {
        cfa_err = _get_nc_grp_var_ids_from_str(
            nc_id, 
            agg_var->cfa_instructionsp->format,
            &grp_id, &var_id
        );
        CFA_CHECK(cfa_err);
        /* format can be a scalar */
        if (agg_var->cfa_instructionsp->format_scaler)
            cfa_err = nc_put_var1_string(grp_id, var_id, frag->index,
                                         (const char**)(&(frag->format)));
        else
            cfa_err = nc_put_var1_string(grp_id, var_id, frag->index,
                                         (const char**)(&(frag->format)));
        CFA_CHECK(cfa_err);
    }

    /* write out one element in the address variable */
    if (frag->address)
    {
        cfa_err = _get_nc_grp_var_ids_from_str(nc_id, 
                    agg_var->cfa_instructionsp->address,
                    &grp_id, &var_id);
        CFA_CHECK(cfa_err);
        /* write out the data from the fragment */
        cfa_err = nc_put_var1_string(grp_id, var_id, frag->index, 
                                     (const char**)(&(frag->address)));
        CFA_CHECK(cfa_err);        
    }
    return CFA_NOERR;
}

/*
write out the location variable - this is a special case as it contains the
spans of the fragments in a 2-D array
*/

int
_write_cfa_fragloc_netcdf(const int loc_grpid, 
                          const int cfa_id, const int cfa_varid,
                          const char *fragloc_varname)
{
    /* get the AggregationVariable */
    AggregationVariable *agg_var = NULL;
    int err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(err);    
    /* get the variable id for the fragloc_varname */
    int nc_varid = -1;
    err = nc_inq_varid(loc_grpid, fragloc_varname, &nc_varid);
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
        /* load up the frag_span */
        int c_len = 0;
        int c_index = 0;
        while (c_len < agg_dim->length)
        {
            frag_span[c_index] = span;
            c_index ++;
            c_len += span;
        }
        /* write the frag_span */
        size_t c_pos[2] = {(size_t)(d), 0};
        size_t c_span[2] = {1, (size_t)(c_index)};
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
    AggregationInstructions *agg_inst = agg_var->cfa_instructionsp;

    /****** LOCATION ******/
    int loc_grpid = -1;
    err = _serialise_cfa_fraggrp_netcdf(nc_id, agg_inst->location, &loc_grpid);
    CFA_CHECK(err);
    err = _serialise_cfa_fragdims_netcdf(loc_grpid, cfa_id, cfa_varid, 0);
    CFA_CHECK(err);
    int loc_varid = -1;
    err = _serialise_cfa_fragvar_netcdf(loc_grpid, cfa_id, cfa_varid, 
                                        "location",
                                        agg_inst->location, &loc_varid);
    CFA_CHECK(err);
    err = _write_cfa_fragloc_netcdf(loc_grpid, cfa_id, cfa_varid, "location");
    CFA_CHECK(err);

    /****** FILE ******/
    int file_grpid = -1;
    err = _serialise_cfa_fraggrp_netcdf(nc_id, agg_inst->file, &file_grpid);
    CFA_CHECK(err);
    err = _serialise_cfa_fragdims_netcdf(file_grpid, cfa_id, cfa_varid, 0);
    CFA_CHECK(err);
    int file_varid = -1;
    err = _serialise_cfa_fragvar_netcdf(file_grpid, cfa_id, cfa_varid, 
                                        "file",
                                        agg_inst->file, &file_varid);
    CFA_CHECK(err);

    /****** FORMAT ******/
    int fmt_grpid = -1;
    err = _serialise_cfa_fraggrp_netcdf(nc_id, agg_inst->format, &fmt_grpid);
    CFA_CHECK(err);
    err = _serialise_cfa_fragdims_netcdf(fmt_grpid, cfa_id, cfa_varid, 0);
    CFA_CHECK(err);
    int format_varid = -1;
    err = _serialise_cfa_fragvar_netcdf(fmt_grpid, cfa_id, cfa_varid, 
                                        "format",
                                        agg_inst->format, &format_varid);
    CFA_CHECK(err);

    /****** ADDRESS ******/
    int add_grpid = -1;
    err = _serialise_cfa_fraggrp_netcdf(nc_id, agg_inst->address, &add_grpid);
    CFA_CHECK(err);
    err = _serialise_cfa_fragdims_netcdf(add_grpid, cfa_id, cfa_varid, 0);
    CFA_CHECK(err);
    int add_varid = -1;
    err = _serialise_cfa_fragvar_netcdf(add_grpid, cfa_id, cfa_varid, 
                                        "address",
                                        agg_inst->address, &add_varid);
    CFA_CHECK(err);

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
    char agg_dim_names[1024] = "";
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
                          strlen(agg_dim_names), agg_dim_names);
    CFA_CHECK(err);
    /* write the AggregationInstructions as an attribute */
    char agg_instr[2048] = "";
    /* location */
    if (agg_var->cfa_instructionsp->location)
    {
        strcat(agg_instr, "location: ");
        strcat(agg_instr, agg_var->cfa_instructionsp->location);
    }
    else
        return CFA_AGG_NOT_DEFINED;
    /* file */
    if (agg_var->cfa_instructionsp->file)
    {
        strcat(agg_instr, " file: ");
        strcat(agg_instr, agg_var->cfa_instructionsp->file);
    }
    else
        return CFA_AGG_NOT_DEFINED;
    /* format */
    if (agg_var->cfa_instructionsp->format)
    {
        strcat(agg_instr, " format: ");
        strcat(agg_instr, agg_var->cfa_instructionsp->format);
    }
    else
        return CFA_AGG_NOT_DEFINED;
    /* address */
    if (agg_var->cfa_instructionsp->address)
    {
        strcat(agg_instr, " address: ");
        strcat(agg_instr, agg_var->cfa_instructionsp->address);
    }
    else
        return CFA_AGG_NOT_DEFINED;
    /* write the netCDF attribute */
    err = nc_put_att_text(nc_id, nc_varid, AGGREGATED_DATA,
                          strlen(agg_instr), agg_instr);
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
    /* create the fragment variables */
    err = _serialise_cfa_fragments_netcdf(nc_id, cfa_id, cfa_varid);
    CFA_CHECK(err);
    /* add the aggregation instructions */
    err = _serialise_cfa_aggregation_instructions(nc_id, nc_varid,
                                                  cfa_id, cfa_varid);
    CFA_CHECK(err);
  
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
        /* recursively serialise the container */
        _serialise_cfa_container_netcdf(nc_grpid, cont_ids[g]);
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
create the netCDF file 
*/
int 
create_cfa_netcdf_file(const char *path, int *ncidp)
{
    /* create the netCDF file to save the CFA info into */
    int cfa_err = nc_create(path, NC_NETCDF4|NC_CLOBBER, ncidp);
    CFA_ERR(cfa_err);
    return CFA_NOERR;
}

/*
write cfa structures into existing netCDF file
*/
int
serialise_cfa_netcdf_file(const int cfa_id)
{
    /* get the container to get the external id (x_id) */
    AggregationContainer *agg_cont = NULL;
    int err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(err);
    /* check the file has been created */
    if (agg_cont->x_id == -1)
        return CFA_NO_FILE;

    /* enter the recursive writer with the root group / container */
    err = _serialise_cfa_container_netcdf(agg_cont->x_id, cfa_id);
    CFA_CHECK(err);

    /* add the conventions global metadata */
    char conventions[256] = "";
    /* CFA-n.m */
    strcat(conventions, CFA_CONVENTION);
    strcat(conventions, CFA_VERSION);
    /* write the netCDF attribute */
    err = nc_put_att_text(agg_cont->x_id, NC_GLOBAL, CONVENTIONS,
                          strlen(conventions), conventions);
    CFA_CHECK(err);

    return CFA_NOERR;
}