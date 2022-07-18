#include <netcdf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cfa.h"
#include "parsers/cfa_netcdf.h"

const char* output_path = "examples/test/example2.nc";

int
example2(void)
{
    /* recreate example 1 from the documentation */
    int cfa_err = -1;
    int cfa_id = -1;
    int cfa_varid = -1;
    int cfa_dimids[4] = {-1, -1, -1, -1};
    int nc_id = -1;

    /* create the CFA parent container */
    cfa_err = cfa_create(output_path, CFA_NETCDF, &cfa_id);
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
    /* add the aggregation instructions */
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, 
                                    "location", "aggregation_location", 0);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, 
                                    "file", "aggregation_file", 0);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, 
                                    "format", "aggregation_format", 1);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, 
                                    "address", "aggregation_address", 0);
    CFA_ERR(cfa_err);
    /* add the fragmentation */
    const int frags[4] = {2, 1, 1, 1};
    cfa_err = cfa_var_def_frag_num(cfa_id, cfa_varid, frags);
    CFA_ERR(cfa_err);

    /* add the first Fragment */
    size_t frag_location[4] = {0, 0, 0, 0};
    size_t data_location[8] = {0,6, 0,1, 0,1, 0,1};
    cfa_err = cfa_var_put1_frag(cfa_id, cfa_varid, 
                                frag_location, data_location, 
                                "January-June.nc", "nc", "temp", NULL);
    CFA_ERR(cfa_err);

    /* add the second Fragment */
    frag_location[0] = 1;    // iterate to the next Fragment location
    data_location[0] = 6;    // iterate to the next location in the parent array
    cfa_err = cfa_var_put1_frag(cfa_id, cfa_varid,
                                frag_location, data_location,
                                NULL, NULL, "temp2", NULL);
    CFA_ERR(cfa_err);

    /* write out the initial structures, variables, etc */
    cfa_err = cfa_serialise(cfa_id);
    CFA_ERR(cfa_err);

    /* get the netCDF file ID stored internally in the CFA AggregationContainer
    */
    cfa_err = cfa_get_ext_file_id(cfa_id, &nc_id);
    CFA_ERR(cfa_err);

    /* once the CFA structure(s) have been serialised we can add the metadata
    to the netCDF variables that have been created during the serialisation */
    /* first get the netcdf var id for the temp variable */
    int nc_varid = -1;
    cfa_err = nc_inq_varid(nc_id, "temp", &nc_varid);
    CFA_ERR(cfa_err);

    /* add the metadata to the temp variable */
    const char* temp_standard_name = "air_temperature";
    cfa_err = nc_put_att_text(nc_id, nc_varid, "standard_name",
                              strlen(temp_standard_name), temp_standard_name);
    CFA_ERR(cfa_err);
    const char* temp_units = "K";
    cfa_err = nc_put_att_text(nc_id, nc_varid, "units",
                              strlen(temp_units), temp_units);
    CFA_ERR(cfa_err);
    const char* temp_cell_methods = "time: mean";
    cfa_err = nc_put_att_text(nc_id, nc_varid, "cell_methods",
                              strlen(temp_cell_methods), temp_cell_methods);
    CFA_ERR(cfa_err);

    /* add the metadata to the time dimension variable */
    int nc_timeid = -1;
    cfa_err = nc_inq_varid(nc_id, "time", &nc_timeid);
    CFA_ERR(cfa_err);
    const char* time_standard_name = "time";
    cfa_err = nc_put_att_text(nc_id, nc_timeid, "standard_name",
                              strlen(time_standard_name), time_standard_name);
    CFA_ERR(cfa_err);
    const char* time_units = "days since 2001-01-01";
    cfa_err = nc_put_att_text(nc_id, nc_timeid, "units",
                              strlen(time_units), time_units);
    CFA_ERR(cfa_err);

    /* add the metadata to the level dimension variable */
    int nc_lvlid = -1;
    cfa_err = nc_inq_varid(nc_id, "level", &nc_lvlid);
    CFA_ERR(cfa_err);
    const char* lvl_standard_name = "height_above_mean_sea_level";
    cfa_err = nc_put_att_text(nc_id, nc_lvlid, "standard_name",
                              strlen(lvl_standard_name), lvl_standard_name);
    CFA_ERR(cfa_err);
    const char* lvl_units = "m";
    cfa_err = nc_put_att_text(nc_id, nc_lvlid, "units",
                              strlen(lvl_units), lvl_units);
    CFA_ERR(cfa_err);

    /* add the metadata to the latitude dimension variable */
    int nc_latid = -1;
    cfa_err = nc_inq_varid(nc_id, "latitude", &nc_latid);
    CFA_ERR(cfa_err);
    const char* lat_standard_name = "latitude";
    cfa_err = nc_put_att_text(nc_id, nc_latid, "standard_name",
                              strlen(lat_standard_name), lat_standard_name);
    CFA_ERR(cfa_err);
    const char* lat_units = "degrees_north";
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
    const char* lon_units = "degrees_east";
    cfa_err = nc_put_att_text(nc_id, nc_lonid, "units",
                              strlen(lon_units), lon_units);
    CFA_ERR(cfa_err);

    /* modify the global conventions to add the CF-1.9 convention */
    char conventions[1024];
    cfa_err = nc_get_att_text(nc_id, NC_GLOBAL, CONVENTIONS, conventions);
    CFA_ERR(cfa_err);
    strcat(conventions, " CF-1.9");
    cfa_err = nc_put_att_text(nc_id, NC_GLOBAL, CONVENTIONS, 
                          strlen(conventions), conventions);
    CFA_ERR(cfa_err);

    /* create the temp2 variable */
    const int temp_dim_ids[3] = {nc_timeid, nc_latid, nc_lonid};
    int nc_temp2_var = -1;
    cfa_err = nc_def_var(nc_id, "temp2", NC_DOUBLE, 
                         3, temp_dim_ids, &nc_temp2_var);
    CFA_ERR(cfa_err);
    /* add metadata */
    cfa_err = nc_put_att_text(nc_id, nc_temp2_var, "units",
                              8, "degreesC");
    CFA_ERR(cfa_err);

    /* write the data for the time variable */
    const size_t start = 0;
    const size_t span = 12;
    const int time_vals[12] = 
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    cfa_err = nc_put_vara_int(nc_id, nc_timeid, &start, &span, time_vals);
    CFA_CHECK(cfa_err);

    /* write the data for the temp2 variable */
    const size_t temp_start[3] = {0, 0, 0};
    const size_t temp_span[3] = {1, 1, 6};
    const double temp2_vals[6] = {4.5, 3.0, 0.0, -2.6, -5.6, -10.2};
    cfa_err = nc_put_vara_double(nc_id, nc_temp2_var,
                                 temp_start, temp_span, 
                                 temp2_vals);
    CFA_CHECK(cfa_err);

    /* output info */
    cfa_err = cfa_info(cfa_id, 0);
    CFA_ERR(cfa_err);

    /* close the CFA file (now closes netCDF file as well) */
    cfa_err = cfa_close(cfa_id);
    CFA_ERR(cfa_err);

    /* check for memory leaks */
    cfa_err = cfa_memcheck();
    CFA_ERR(cfa_err);

    return CFA_NOERR;
}

int
main(void)
{
    example2();
    return 0;
}