This directory (CFA-C/examples) contains the examples from the
[CFA 0.6 document](https://github.com/NCAS-CMS/cfa-conventions/blob/master/source/cfa.md),
in CDL format.

They can be converted to valid netCDF4 by running the command:

    ncgen -4 example?.cdl

There is also a Makefile.  Individual examples can be converted using:

    make example?.nc

Or, to make all the examples:

    make
