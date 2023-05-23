CFA-C
=====

This repository contains a C library reference implementation of the
[CFA-Conventions v0.6.](https://github.com/NCAS-CMS/cfa-conventions/blob/master/source/cfa.md)

From that website, the CFA conventions are described as:

---

The CFA (Climate and Forecast Aggregation) conventions describe how a **netCDF** 
file can be used to describe a dataset distributed across multiple other data 
files. A CFA-compliant aggregation can be described in netCDF in such way that
the describing file does not contain the data of selected variables 
("aggregation variables"), rather it contains variables with special attributes 
that provide instructions on how to create the aggregated variable data as an 
aggregation of data from other sources, each of which may be self-describing 
datasets in their own right.

---

This library provides methods to read and write CFA files in C.  A number of 
examples are provided.

Installation
------------

1.  CFA-C is compiled using a C compiler that is compatible with C99.  It is built using **make** via a provided *Makefile*.

2.  CFA-C requires the netCDF library to be installed.  See [here](https://docs.unidata.ucar.edu/netcdf-c/current/winbin.html) for installation instructions.  Depending on the platform, an easier way of installing the netCDF libraries may be available.

    2a.  On Apple Mac computers running MacOS, [Homebrew](https://brew.sh) can be used with the command: `brew install netcdf`

    2b.  On RedHat, CentOS or Rocky Linux, the in-built package manager can be used with the command (as *root* or *sudo*): `yum install netcdf`

3.  The CFA-C library can then be built:

    3a.  Navigate to the top `CFA-C` directory, where the `Makefile` file is contained.

    3b.  Use the command `make`.

    3c.  This will have built the files `lib/libcfa.so` and `lib/libcfa.so.dSYM`.

4.  There are a number of examples in the directory `test/examples`.  

    4a.  To build an example use the command `make <example_name>`.

    4b.  For example, to build `test/examples/example1a.c` use the command `make example1a`.

    4c.  The example will have been built at `build/examples/example1a`.  To run it use the command `build/examples/example1a S` (to **S**ave/create the file) and
    `build/examples/example1b L` to load the file.

    4d.  The output file for `example1a` will be written to `examples/test/example1a.nc`.  The `ncdump` command, installed with the netCDF library, can be used to examine the contents of the file: `ncdump -h examples/test/example1a.nc`.

5.  The examples match those in the [CFA-Conventions v0.6](https://github.com/NCAS-CMS/cfa-conventions/blob/master/source/cfa.md) document.

6.  Studying the examples in conjunction with the CFA-Conventions document will reveal how to write your own CFA files using CFA-C.

---

CFA-C was developed at the [Centre for Environmental Data Analysis](https://www.ceda.ac.uk) in collaboration with [NCAS-CMS](https://cms.ncas.ac.uk).
It has received support from the ESiWACE2 project. The project ESiWACE2 has received 
funding from the European Union's Horizon 2020 research and innovation programme
under grant agreement No 823988. 

CFA-C is Open-Source software with a BSD-2 Clause License.  The license can be
read in the repository.

--- 