#include "cfa.h"
#include "parsers/cfa_netcdf.h"
#include <netcdf.h>

/*
load and parse a netCDF-C file
*/
int 
parse_netcdf_file(const char* path)
{
    int ncid = -1;
    int nc_err = nc_open(path, NC_NOWRITE, &ncid);
    if (nc_err)
        return nc_err;
    return CFA_NOERR;
}
