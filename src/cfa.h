#ifndef __CFA_H__
#define __CFA_H__

#include <stdio.h>

#include "cfa_mem.h"

/* CFA metadata identifier string */
#define CFA_CONVENTION ("CFA-0.6")

/* ERROR constants */
/* These start at -500 so we can pass standard netCDF errors back when parsing 
the file */
#define CFA_NOERR                    (0) /* No error */
#define CFA_MEM_ERR               (-500) /* Cannot create memory */
#define CFA_MEM_LEAK              (-501) /* Debugging check for memory leak */
#define CFA_BOUNDS_ERR            (-502) /* Bounds error in dynamic array */
#define CFA_NOT_FOUND_ERR         (-510) /* Cannot find CFA Container */
#define CFA_DIM_NOT_FOUND_ERR     (-520) /* Cannot find CFA Dimension */
#define CFA_VAR_NOT_FOUND_ERR     (-530) /* Cannot find CFA Variable */
#define CFA_UNKNOWN_FILE_FORMAT   (-540) /* Unsupported CFA file format */
#define CFA_NOT_CFA_FILE          (-541) /* Not a CFA file - does not contain */
                                         /* relevant metadata */
#define CFA_AGG_DATA_ERR          (-542) /* Something went wrong parsing the */
                                         /* "aggregated_data" attribute */
#define CFA_AGG_DIM_ERR           (-542) /* Something went wrong parsing the */
                                         /* "aggregated_dimensions" attribute */
                                         


/* CFA-structs */
/* DataType struct */
typedef struct {
    char *type;
    size_t size;
} DataType;

/* FragmentDimension */
typedef struct {
    char *name;
    int length;
    /* which AggregatedDimension is this a subdomain of? */
    struct AggregatedDimension* cfa_dimp;
} FragmentDimension;

/* Fragment */
typedef struct {
    int  *location;
    char *file;
    char *format;
    char *address;
    char *units;
    /* <FragmentDimension> */
    DynamicArray *cfa_frag_dimsp;
    DataType* cfa_dtype;
} Fragment;

/* AggregationInstructions */
typedef struct {
    char *location;
    char *file;
    char *format;
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
    int len;
    FragmentDimension *cfa_frag_dimp;
} AggregatedDimension;

/* AggregationVariable */
typedef struct {
    char *name;
    /* dim ids <int> */
    int cfa_ndim;
    int *cfa_dim_idp;
    DataType *cfa_dtype;
    AggregatedData *cfa_datap;
    AggregationInstructions *cfa_instructionsp;
} AggregationVariable;

/* AggregationContainer */
typedef struct AggregationContainer AggregationContainer;
struct AggregationContainer {
    /* var ids <AggregationVariable> */
    DynamicArray *cfa_varp;
    /* dims <AggregatedDimension> */
    DynamicArray *cfa_dimp;
    /* containers <AggregationContainer> (for groups) */
    DynamicArray *cfa_containerp;

    /* file info */
    char* path;
    /* name (if a group) */
    char* name;
}; 

/* File formats */
typedef enum {
    CFA_UNKNOWN=-1,
    CFA_NETCDF=0
} CFAFileFormat;

/* create a AggregationContainer and assign it to cfaid */
extern int cfa_create(const char *path, int *cfa_idp);

/* load a CFA-netCDF file into an AggegrationContainer and assign it a cfaid */
extern int cfa_load(const char *path, CFAFileFormat format, int *cfa_idp);

/* get the id of a AggregationContainer */
extern int cfa_inq_id(const char *path, int *cfa_idp);

/* get the number of AggregationContainers */
extern int cfa_inq_n(int *ncfa);

/* get the AggregationContainer from a cfa_id */
extern int cfa_get(const int cfa_id, AggregationContainer **agg_cont);

/* close a AggregationContainer */
extern int cfa_close(const int cfa_id);

/* create a AggregatedDimension, attach it to a cfa_id */
extern int cfa_def_dim(const int cfa_id, const char *name, 
                        const int len, int *cfa_dim_idp);

/* Get the identifier of an AggregatedDimension */
extern int cfa_inq_dim_id(const int cfa_id, const char* name, 
                           int *cfa_dim_idp);

/* return the number of AggregatedDimensions that have been defined */
extern int cfa_inq_ndims(const int cfa_id, int *ndimp);

/* get the AggregatedDimension from a cfa_dim_id */
extern int cfa_get_dim(const int cfa_id, const int cfa_dim_id, 
                        AggregatedDimension **agg_dim);

/* create an AggregationVariable container, attach it to a cfa_id and one 
or more cfa_dim_ids and assign it to a cfavarid */
extern int cfa_def_var(const int cfa_id, const char *name, int *cfa_var_idp);

/* add the AggregatedDimension ids to to the variable */
extern int cfa_var_def_dims(const int cfa_id, const int cfa_var_id,
                            const int ndims, int *cfa_dim_idsp);

/* add the AggregationInstructions from a string 
   the string follows the key: value pair format
   keys are: location:, address:, file:, format: (including the colon)
   multiple key: value pairs can be separated by a space
*/
extern int cfa_var_def_agg_instr(const int cfa_id, const int cfa_var_id,
                                 const char* agg_instr);

/* get the identifier of an AggregationVariable by name */
extern int cfa_inq_var_id(const int cfa_id, const char* name, 
                           int *cfa_dim_idp);

/* get the number of AggregationVariables defined */
extern int cfa_inq_nvars(const int cfa_id, int *nvarp);

/* get the AggregationVariable from a cfa_var_id */
extern int cfa_get_var(const int cfa_id, const int cfa_var_id,
                        AggregationVariable **agg_var);

#define CFA_ERR(cfa_err) if(cfa_err) {printf("CFA error: %i\n", cfa_err); return cfa_err;}
#define CFA_CHECK(cfa_err) if(cfa_err) {return cfa_err;}

#endif
