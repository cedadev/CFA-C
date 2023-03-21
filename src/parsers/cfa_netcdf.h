#ifndef __CFA_NETCDF__
#define __CFA_NETCDF__

/*
Open an existing CFA file and parse it into the data-structures
*/
int parse_cfa_netcdf_file(const char *path, int *cfaidp);

/*
Create a netCDF CFA file, ready for data to be serialised into it
returns the netCDF file id in ncidp
*/
int create_cfa_netcdf_file(const char *path, int *ncidp);

/*
Create the initial CFA structures inside an already created CFA-netCDF file 
(or group) with the cfaid.  The netCDF id is contained in the 
AggregationContainer as x_id.
Further information can be added to the file (e.g. metadata for the variables)
by using the standard netCDF functions.  Use nc_inq_varid, nc_inq_dimid,
nc_inq_grpid, etc. to get the netCDF id of a variable, dimension or group
before adding to it.

create_cfa_netcdf_file() needs to be called before this

*/
int serialise_cfa_netcdf_file(int cfaid);

/* 
Close the cfa_netcdf file 
*/
int close_cfa_netcdf_file(int ncid);


#endif
