#include <netcdf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cfa.h"

const char* example3_path = "examples/test/example3.nc";

int
example3_save(void)
{
    printf("Example 3 test save\n");
    /* recreate example 3 from the documentation */
    int cfa_err = -1;
    int cfa_id = -1;
    int cfa_varid = -1;
    int cfa_dimids[4] = {-1, -1, -1, -1};
    int nc_id = -1;

    /* create the CFA parent container */
    cfa_err = cfa_create(example3_path, CFA_NETCDF, &cfa_id);
    CFA_ERR(cfa_err);

    /* define the CFA dimensions */
    /* time */
    cfa_err = cfa_def_dim(cfa_id, "time", 12, CFA_DOUBLE, cfa_dimids);
    CFA_ERR(cfa_err);
    /* level */
    cfa_err = cfa_def_dim(cfa_id, "level", 1, CFA_DOUBLE, cfa_dimids+1);
    CFA_ERR(cfa_err);
    /* latitude */
    cfa_err = cfa_def_dim(cfa_id, "latitude", 73, CFA_DOUBLE, cfa_dimids+2);
    CFA_ERR(cfa_err);
    /* longitude */
    cfa_err = cfa_def_dim(cfa_id, "longitude", 144, CFA_DOUBLE, cfa_dimids+3);
    CFA_ERR(cfa_err);

    /* create variable */
    cfa_err = cfa_def_var(cfa_id, "temp", CFA_DOUBLE, &cfa_varid);
    CFA_ERR(cfa_err);
    /* add dimensions */
    cfa_err = cfa_var_def_dims(cfa_id, cfa_varid, 4, cfa_dimids);
    CFA_ERR(cfa_err);
    /* add the aggregation instructions 
       these are part of a group for example 3 */
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, "location",
                                    "/aggregation/location", false, CFA_INT);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, "file",
                                    "/aggregation/file", false, CFA_STRING);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, "format",
                                    "/aggregation/format", true, CFA_STRING);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, "address",
                                    "/aggregation/address", false, CFA_STRING);
    CFA_ERR(cfa_err);
    /* add the fragmentation */
    const int frags_nums[4] = {2, 1, 1, 1};
    cfa_err = cfa_var_def_frag_num(cfa_id, cfa_varid, frags_nums);
    CFA_ERR(cfa_err);

    /* output info */
    cfa_err = cfa_info(cfa_id, 0);
    CFA_ERR(cfa_err);

    /* add the first Fragment */
    size_t frag_location[4];
    frag_location[0] = 0; frag_location[1] = 0; 
    frag_location[2] = 0; frag_location[3] = 0;
    cfa_err = cfa_var_put1_frag_string(cfa_id, cfa_varid, frag_location, NULL,
                                       "address", "temp1");
    CFA_ERR(cfa_err);

    /* add the second Fragment */
    frag_location[0] = 1; frag_location[1] = 0; 
    frag_location[2] = 0; frag_location[3] = 0;
    cfa_err = cfa_var_put1_frag_string(cfa_id, cfa_varid, frag_location, NULL,
                                       "address", "temp2");
    CFA_ERR(cfa_err);

    /* write out the initial structures, variables, etc 
       now have to open a netCDF file first */
    cfa_err = nc_create(example3_path, NC_NETCDF4|NC_CLOBBER, &nc_id);
    CFA_CHECK(cfa_err);

    cfa_err = cfa_serialise(cfa_id, nc_id);
    CFA_ERR(cfa_err);

    /* once the CFA structure(s) have been serialised we can add the metadata
    to the netCDF variables that have been created during the serialisation */
    /* get the netCDF file id from the CFA File Container */
    cfa_err = cfa_get_ext_file_id(cfa_id, &nc_id);
    CFA_ERR(cfa_err);

    /* get the netCDF var id for the temp variable */
    int nc_varid = -1;
    cfa_err = nc_inq_varid(nc_id, "temp", &nc_varid);
    CFA_ERR(cfa_err);

    /* add the metadata to the temp variable */
    const char *temp_standard_name = "air_temperature";
    cfa_err = nc_put_att_text(nc_id, nc_varid, "standard_name",
                              strlen(temp_standard_name), temp_standard_name);
    CFA_ERR(cfa_err);
    const char *temp_units = "K";
    cfa_err = nc_put_att_text(nc_id, nc_varid, "units",
                              strlen(temp_units), temp_units);
    CFA_ERR(cfa_err);
    const char *temp_cell_methods = "time: mean";
    cfa_err = nc_put_att_text(nc_id, nc_varid, "cell_methods",
                              strlen(temp_cell_methods), temp_cell_methods);
    CFA_ERR(cfa_err);

    /* add the metadata to the time dimension variable */
    int nc_timeid = -1;
    cfa_err = nc_inq_varid(nc_id, "time", &nc_timeid);
    CFA_ERR(cfa_err);
    const char *time_standard_name = "time";
    cfa_err = nc_put_att_text(nc_id, nc_timeid, "standard_name",
                              strlen(time_standard_name), time_standard_name);
    CFA_ERR(cfa_err);
    const char *time_units = "days since 2001-01-01";
    cfa_err = nc_put_att_text(nc_id, nc_timeid, "units",
                              strlen(time_units), time_units);
    CFA_ERR(cfa_err);

    /* add the metadata to the level dimension variable */
    int nc_lvlid = -1;
    cfa_err = nc_inq_varid(nc_id, "level", &nc_lvlid);
    CFA_ERR(cfa_err);
    const char *lvl_standard_name = "height_above_mean_sea_level";
    cfa_err = nc_put_att_text(nc_id, nc_lvlid, "standard_name",
                              strlen(lvl_standard_name), lvl_standard_name);
    CFA_ERR(cfa_err);
    const char *lvl_units = "m";
    cfa_err = nc_put_att_text(nc_id, nc_lvlid, "units",
                              strlen(lvl_units), lvl_units);
    CFA_ERR(cfa_err);

    /* add the metadata to the latitude dimension variable */
    int nc_latid = -1;
    cfa_err = nc_inq_varid(nc_id, "latitude", &nc_latid);
    CFA_ERR(cfa_err);
    const char *lat_standard_name = "latitude";
    cfa_err = nc_put_att_text(nc_id, nc_latid, "standard_name",
                              strlen(lat_standard_name), lat_standard_name);
    CFA_ERR(cfa_err);
    const char *lat_units = "degrees_north";
    cfa_err = nc_put_att_text(nc_id, nc_latid, "units",
                              strlen(lat_units), lat_units);
    CFA_ERR(cfa_err);

    /* add the metadata to the longitude dimension variable */
    int nc_lonid = -1;
    cfa_err = nc_inq_varid(nc_id, "longitude", &nc_lonid);
    CFA_ERR(cfa_err);
    const char* lon_standard_name = "longitude";
    cfa_err = nc_put_att_text(nc_id, nc_lonid, "standard_name",
                              strlen(lon_standard_name), lon_standard_name);
    CFA_ERR(cfa_err);
    const char *lon_units = "degrees_east";
    cfa_err = nc_put_att_text(nc_id, nc_lonid, "units",
                              strlen(lon_units), lon_units);
    CFA_ERR(cfa_err);

    /* modify the global conventions to add the CF-1.9 convention */
    char conventions[1024] = "";
    cfa_err = nc_get_att_text(nc_id, NC_GLOBAL, CONVENTIONS, conventions);
    CFA_ERR(cfa_err);
    strcat(conventions, " CF-1.9");
    cfa_err = nc_put_att_text(nc_id, NC_GLOBAL, CONVENTIONS, 
                          strlen(conventions), conventions);
    CFA_ERR(cfa_err);

    /* create the temp1 and temp2 variables inside the aggregation group */
    int nc_agg_grp = -1;
    cfa_err = nc_inq_grp_ncid(nc_id, "aggregation", &nc_agg_grp);
    CFA_ERR(cfa_err);
    const int temp_dim_ids[3] = {nc_timeid, nc_latid, nc_lonid};
    int nc_temp1_var = -1;
    cfa_err = nc_def_var(nc_agg_grp, "temp1", NC_DOUBLE, 
                         3, temp_dim_ids, &nc_temp1_var);
    CFA_ERR(cfa_err);
    /* add metadata */
    cfa_err = nc_put_att_text(nc_agg_grp, nc_temp1_var, "units",
                              6, "Kelvin");
    CFA_ERR(cfa_err);
    int nc_temp2_var = -1;
    cfa_err = nc_def_var(nc_agg_grp, "temp2", NC_DOUBLE, 
                         3, temp_dim_ids, &nc_temp2_var);
    CFA_ERR(cfa_err);
    /* add metadata */
    cfa_err = nc_put_att_text(nc_agg_grp, nc_temp2_var, "units",
                              8, "degreesC");
    CFA_ERR(cfa_err);

    /* write the data for the time variable */
    const size_t start = 0;
    const size_t span = 12;
    const int time_vals[12] = 
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    cfa_err = nc_put_vara_int(nc_id, nc_timeid, &start, &span, time_vals);
    CFA_CHECK(cfa_err);

    /* write the data for the temp1 and temp2 variables */
    const double temp1_vals[6] = {270.3, 272.5, 274.1, 278.5, 280.3, 283.6};
    const size_t temp_start[3] = {0, 0, 0};
    const size_t temp_span[3] = {1, 1, 6};
    cfa_err = nc_put_vara_double(nc_agg_grp, nc_temp1_var, 
                                 temp_start, temp_span,
                                 temp1_vals);
    CFA_CHECK(cfa_err);
    const double temp2_vals[6] = {4.5, 3.0, 0.0, -2.6, -5.6, -10.2};
    cfa_err = nc_put_vara_double(nc_agg_grp, nc_temp2_var,
                                 temp_start, temp_span, 
                                 temp2_vals);
    CFA_CHECK(cfa_err);

    /* close the CFA file (closes netCDF file as well) */
    cfa_err = cfa_close(cfa_id);
    CFA_ERR(cfa_err);

    /* check for memory leaks */
    cfa_err = cfa_memcheck();
    CFA_ERR(cfa_err);

    /* close the netCDF file */
    cfa_err = nc_close(nc_id);
    CFA_ERR(cfa_err);

    return CFA_NOERR;
}

int
example3_load(void)
{
    int cfa_err = -1;
    int cfa_id = -1;
    int nc_id = -1;
    printf("Example 3 test load\n");

    /* open the netCDF file */
    cfa_err = nc_open(example3_path, NC_NOWRITE, &nc_id);
    CFA_ERR(cfa_err);

    /* load and parse */
    cfa_err = cfa_load(example3_path, nc_id, CFA_NETCDF, &cfa_id);
    CFA_ERR(cfa_err);

    /* get the "temp" variable id */
    int cfa_var_id = -1;
    cfa_err = cfa_inq_var_id(cfa_id, "temp", &cfa_var_id);
    CFA_ERR(cfa_err);

    /* get the first fragment */
    size_t frag_location[4];
    frag_location[0] = 0; frag_location[1] = 0; 
    frag_location[2] = 0; frag_location[3] = 0;
    char* address = NULL;
    cfa_err = cfa_var_get1_frag(cfa_id, cfa_var_id, frag_location, NULL, 
                                "address", (void**)(&address));
    CFA_ERR(cfa_err);
    printf("%s %s \n", "address", address);
    /* get the second Fragment */
    frag_location[0] = 1; frag_location[1] = 0; 
    frag_location[2] = 0; frag_location[3] = 0;
    cfa_err = cfa_var_get1_frag(cfa_id, cfa_var_id, frag_location, NULL,
                                "address", (void**)(&address));
    CFA_ERR(cfa_err);
    printf("%s %s \n", "address", address);
    /* output info */
    cfa_err = cfa_info(cfa_id, 0);
    CFA_ERR(cfa_err);

    /* close the CFA file to free memory */
    cfa_err = cfa_close(cfa_id);
    CFA_ERR(cfa_err);

    /* check for memory leaks */
    cfa_err = cfa_memcheck();
    CFA_ERR(cfa_err);

    /* close the netCDF file */
    cfa_err = nc_close(nc_id);
    CFA_ERR(cfa_err);

    return CFA_NOERR;
}

int
main(int argc, char **argv)
{
    /* Argument passed in: S - test save, L - test load */
    if (argc != 2)
    {
        printf("Wrong number of arguments\n");
        return 1;
    }
    if (strcmp(argv[1], "S") == 0)
        example3_save();
    else if (strcmp(argv[1], "L") == 0)
        example3_load();
    else
    {
        printf("Unknown argument\n");
        return 1;
    }
    return 0;
}