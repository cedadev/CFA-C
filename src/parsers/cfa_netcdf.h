#ifndef __CFA_NETCDF__
#define __CFA_NETCDF__

/*
Parse an existing and open CFA file into the data-structures.  This will return
a cfa id that can be used in subsequent calls

call nc_open before this

*/
int parse_cfa_netcdf_file(const char* path, const int ncid, int *cfaidp);

/*
Create the initial CFA structures inside an already created CFA-netCDF file 
(or group) with the cfaid and write it to the netCDF file with the id ncid.
Further information can be added to the file (e.g. metadata for the variables)
by using the standard netCDF functions.  Use nc_inq_varid, nc_inq_dimid,
nc_inq_grpid, etc. to get the netCDF id of a variable, dimension or group
before adding to it.

call nc_open before this

*/
int serialise_cfa_netcdf_file(const int ncid, const int cfaid);



#endif
