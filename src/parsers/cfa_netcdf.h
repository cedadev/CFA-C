#ifndef __CFA_NETCDF__
#define __CFA_NETCDF__

/*
Open an existing CFA file and parse it into the data-structures
*/
int parse_netcdf_cfa_file(const char *path, int *cfaidp);

/*
Create the initial CFA structures inside an already created netCDF file 
(or group) with the ncid
Further information can be added to the file (e.g. metadata for the variables)
by using the standard netCDF functions.  Use nc_inq_varid, nc_inq_dimid,
nc_inq_grpid, etc. to get the netCDF id of a variable, dimension or group
before adding to it.
*/ 
int serialise_cfa_netcdf(int ncid, int cfaid);

#endif