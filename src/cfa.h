#ifndef __CFA_H__
#define __CFA_H__

#include <stdio.h>
#include <stdbool.h>

#include "cfa_mem.h"
#include "cfa_errs.h"

/* Type constants - these match the netCDF ones, except for strings
   Currently we are only supporting numeric types
*/
#define CFA_NAT          (0)      /**< Not A Type */
#define CFA_BYTE         (1)      /**< signed 1 byte integer */
#define CFA_CHAR         (2)      /**< ISO/ASCII character */
#define CFA_SHORT        (3)      /**< signed 2 byte integer */
#define CFA_INT          (4)      /**< signed 4 byte integer */
#define CFA_LONG         (CFA_INT)
#define CFA_FLOAT        (5)      /**< single precision floating point number */
#define CFA_DOUBLE       (6)      /**< double precision floating point number */
#define CFA_UBYTE        (7)      /**< unsigned 1 byte int */
#define CFA_USHORT       (8)      /**< unsigned 2-byte int */
#define CFA_UINT         (9)      /**< unsigned 4-byte int */
#define CFA_INT64        (10)     /**< signed 8-byte int */
#define CFA_UINT64       (11)     /**< unsigned 8-byte int */

/* CFA metadata identifier string */
#define CFA_CONVENTION ("CFA-")
#define CFA_VERSION    ("0.6")
#define CONVENTIONS    ("Conventions")
#define AGGREGATED_DIMENSIONS ("aggregated_dimensions")
#define AGGREGATED_DATA ("aggregated_data")

/* Fixed size of arrays */
#define MAX_VARS 256
#define MAX_DIMS 256
#define MAX_CONTS 256

/* Fast / naive indexing */
#define FAST_INDEX

/* cfa_type is just an int */
typedef int cfa_type;

/* CFA-structs */
/* DataType struct */
typedef struct {
    cfa_type type;   /* see the constants above for the definitions of types */
    size_t   size;
} DataType;

/* FragmentDimension */
typedef struct {
    char *name;
    int length;
    /* which AggregatedDimension is this a subdomain of? This is the ID in the
    DynamicArray */
    int cfa_dim_id;
} FragmentDimension;

/* Fragment */
typedef struct {
    size_t *location;
    size_t *index;    
    char *file;
    char *format;
    char *address;
    char *units;
    /* Datatype of Fragment - inherited from Variable */
    DataType cfa_dtype;
    /* helper variable - linear index into 1D arrays */
    int linear_index;
} Fragment;

/* AggregationInstructions */
typedef struct {
    char *location;
    bool location_scalar;
    char *file;
    char *format;
    bool format_scalar;
    char *address;
} AggregationInstructions;

/* AggregatedData */
typedef struct {
    char *units;
    DynamicArray *cfa_fragmentsp;
} AggregatedData;

/* AggregatedDimension */
typedef struct {
    char *name;
    int length;
    DataType cfa_dtype;
} AggregatedDimension;

/* AggregationVariable */
typedef struct {
    char *name;
    /* dim ids <int> */
    int cfa_ndim;
    int cfa_dim_idp[MAX_DIMS];
    /* fragment dimension ids */
    int cfa_frag_dim_idp[MAX_DIMS];
    DataType cfa_dtype;
    AggregatedData *cfa_datap;
    AggregationInstructions *cfa_instructionsp;
} AggregationVariable;

/* File formats */
typedef enum {
    CFA_UNKNOWN=-1,
    CFA_NETCDF=0
} CFAFileFormat;

/* AggregationContainer */
typedef struct AggregationContainer AggregationContainer;
struct AggregationContainer {
    /* var ids <AggregationVariable> */
    int cfa_varids[MAX_VARS];
    int n_vars;
    /* dims <AggregatedDimension> */
    int cfa_dimids[MAX_DIMS];
    int n_dims;
    /* containers <AggregationContainer> (for groups) */
    int cfa_contids[MAX_CONTS];
    int n_conts;

    /* file info */
    char* path;
    CFAFileFormat format;
    int x_id;           /* external file id (e.g. netCDF id) */
    int serialised;     /* has the file been serialised yet? */
    /* name (if a group) */
    char* name;
}; 


/* create a AggregationContainer and assign it to cfaid */
extern int cfa_create(const char *path, CFAFileFormat format, int *cfa_idp);

/* load a CFA-netCDF file into an AggegrationContainer and assign it a cfaid */
extern int cfa_load(const char *path, CFAFileFormat format, int *cfa_idp);

/* get the external file id */
extern int cfa_get_ext_file_id(const int cfa_id, int *x_id);

/* serialise (save) the AggregationContainer into a CFA file.  The format will
depend on the CFAFileFormat argument passed into the cfa_create method */
extern int cfa_serialise(const int cfa_id);

/* get the id of a AggregationContainer */
extern int cfa_inq_id(const char *path, int *cfa_idp);

/* get the number of AggregationContainers */
extern int cfa_inq_n(int *ncfa);

/* get the AggregationContainer from a cfa_id */
extern int cfa_get(const int cfa_id, AggregationContainer **agg_cont);

/* close a AggregationContainer */
extern int cfa_close(const int cfa_id);

/* create an AggregationContainer within another AggregationContainer */
extern int cfa_def_cont(const int cfa_id, const char* name, int *cfa_cont_idp);

/* get the identifier of an AggregationContainer within another 
AggregationContainer, using the name */
extern int cfa_inq_cont_id(const int cfa_id, const char *name, 
                           int *cfa_cont_idp);

/* return the number AggregationContainers inside another AggregationContainer*/
extern int cfa_inq_nconts(const int cfa_id, int *ncontp);

/* get the ids for the AggregationContainers in the AggregationContainer */
extern int cfa_inq_cont_ids(const int cfa_id, int **contids);

/* get the AggregationContainer from a cfa_cont_id */
extern int cfa_get_cont(const int cfa_id, const int cfa_cont_id,
                        AggregationContainer **agg_cont);

/* create a AggregatedDimension, attach it to a cfa_id */
extern int cfa_def_dim(const int cfa_id, const char *name, const int len,
                       const cfa_type dtype, int *cfa_dim_idp);

/* Get the identifier of an AggregatedDimension */
extern int cfa_inq_dim_id(const int cfa_id, const char* name, 
                          int *cfa_dim_idp);

/* return the number of AggregatedDimensions that have been defined */
extern int cfa_inq_ndims(const int cfa_id, int *ndimp);

/* get the ids for the AggregatedDimensions in the AggregationContainer */
extern int cfa_inq_dim_ids(const int cfa_id, int **dimids);

/* get the AggregatedDimension from a cfa_dim_id */
extern int cfa_get_dim(const int cfa_id, const int cfa_dim_id, 
                       AggregatedDimension **agg_dim);

/* create an AggregationVariable container, attach it to a cfa_id and one 
or more cfa_dim_ids and assign it to a cfavarid */
extern int cfa_def_var(const int cfa_id, const char *name, 
                       const cfa_type vtype, int *cfa_var_idp);

/* add the AggregatedDimension ids to to the variable */
extern int cfa_var_def_dims(const int cfa_id, const int cfa_var_id,
                            const int ndims, const int *cfa_dim_idsp);

/* add the AggregationInstructions from a string 
   location or format may be scalar */
extern int cfa_var_def_agg_instr(const int cfa_id, const int cfa_var_id,
                                 const char *instruction,
                                 const char *value, const bool scalar_location);

/* get the identifier of an AggregationVariable by name */
extern int cfa_inq_var_id(const int cfa_id, const char *name, 
                          int *cfa_dim_idp);

/* get the number of AggregationVariables defined */
extern int cfa_inq_nvars(const int cfa_id, int *nvarp);

/* get the ids for the AggregationVariables in the AggregationContainer */
extern int cfa_inq_var_ids(const int cfa_id, int **varids);

/* get the AggregationVariable from a cfa_var_id */
extern int cfa_get_var(const int cfa_id, const int cfa_var_id,
                       AggregationVariable **agg_var);

/* add the fragment definitions.  There should be one number per dimension in
the fragments array.  This defines how many times that dimension is subdivided
*/
extern int cfa_var_def_frag_num(const int cfa_id, const int cfa_var_id,
                                const int *fragments);
                                
/* get a FragmentDimension */
extern int cfa_var_get_frag_dim(const int cfa_id, const int cfa_var_id,
                                const int dimn, FragmentDimension **frag_dim);

/* write a single Fragment for a variable, DataType is inherited from parent
   variable */
extern int cfa_var_put1_frag(const int cfa_id, const int cfa_var_id,
                             const size_t *frag_location, 
                             const size_t *data_location,
                             const char *file, const char *format, 
                             const char *address, const char *units);

/* get a single Fragment for a variable */
extern int cfa_var_get1_frag(const int cfa_id, const int cfa_var_id,
                             const size_t *frag_location,
                             const size_t *data_location,
                             const Fragment **ret_frag);

/* info / output command - output the structure of a container, including the
dimensions, variables and any sub-containers
  level dictates how much info is output
*/
extern int cfa_info(const int cfa_id, const int level);

#define CFA_ERR(cfa_err) if(cfa_err) {if (cfa_err > -500) printf("NC error: %i\n", cfa_err); else printf("CFA error %i\n", cfa_err); return cfa_err;}
#define CFA_CHECK(cfa_err) if(cfa_err) {return cfa_err;}

#endif
