#include <netcdf.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "cfa.h"
#include "parsers/cfa_netcdf.h"

const char* output_path = "examples/test/example3.nc";

int
example3(void)
{
    /* recreate example 3 from the documentation */
    int cfa_err = -1;
    int cfa_id = -1;
    int cfa_varid = -1;
    int cfa_dimids[4] = {-1, -1, -1, -1};
    int nc_id = -1;

    /* create the CFA parent container */
    cfa_err = cfa_create(output_path, &cfa_id);
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
                                    "/aggregation/location", 0);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, "file",
                                    "/aggregation/file", 0);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, "format",
                                    "/aggregation/format", 1);
    CFA_ERR(cfa_err);
    cfa_err = cfa_var_def_agg_instr(cfa_id, cfa_varid, "address",
                                    "/aggregation/address", 0);
    CFA_ERR(cfa_err);
    /* add the fragmentation */
    const int frags[4] = {2, 1, 1, 1};
    cfa_err = cfa_var_def_frag_size(cfa_id, cfa_varid, frags);
    CFA_ERR(cfa_err);

    /* output info */
    cfa_err = cfa_info(cfa_id, 0);
    CFA_ERR(cfa_err);

    /* create the netCDF file to save the CFA info into */
    cfa_err = nc_create(output_path, NC_NETCDF4|NC_CLOBBER, &nc_id);
    CFA_ERR(cfa_err);

    /* add the first Fragment */
    Fragment frag;
    frag.location = cfa_malloc(sizeof(int)*4);
    frag.location[1] = 0;
    frag.location[2] = 0;
    frag.location[3] = 0;

    cfa_err = cfa_var_put1_frag(cfa_id, cfa_varid, &frag);
    CFA_ERR(cfa_err);

    /* write out the initial structures, variables, etc */
    cfa_err = serialise_cfa_netcdf(nc_id, cfa_id);
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

    /*  close the netCDF file */
    cfa_err = nc_close(nc_id);
    CFA_ERR(cfa_err);

    /* close the CFA file */
    cfa_err = cfa_close(cfa_id);
    CFA_ERR(cfa_err);

    cfa_free(frag.location, sizeof(int)*4);

    /* check for memory leaks */
    cfa_err = cfa_memcheck();
    CFA_ERR(cfa_err);

    return CFA_NOERR;
}

int
main(void)
{
    example3();
    return 0;
}