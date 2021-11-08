#ifndef __CFA_H__
#define __CFA_H__

#include <stdio.h>

#include "cfa_mem.h"

#define EXTERNL extern

/* ERROR constants */
#define CFA_NOERR 0                   /* No error */
#define CFA_MEM_ERR -1                /* Cannot create memory */
#define CFA_MEM_LEAK -2               /* Debugging check for memory leak */
#define CFA_BOUNDS_ERR -3             /* Bounds error in dynamic array */
#define CFA_NOT_FOUND_ERR -10         /* Cannot find CFA Container */
#define CFA_DIM_NOT_FOUND_ERR -20     /* Cannot find CFA Dimension */
#define CFA_VAR_NOT_FOUND_ERR -30     /* Cannot find CFA Variable */

/* CFA-structs */
/* DataType struct */
typedef struct {

} DataType;

/* FragmentDimension */
typedef struct {

} FragmentDimension;

/* AggregationInstructions */
typedef struct {

} AggregationInstructions;

/* AggregatedData */
typedef struct {
    char *units;
} AggregatedData;

/* AggregatedDimension */
typedef struct {
    char *name;
    int len;
} AggregatedDimension;

/* AggregationVariable */
typedef struct {
    char *name;
    int cfa_ndim;
    int *cfa_dim_idp;
} AggregationVariable;

/* AggregationContainer */
typedef struct AggregationContainer AggregationContainer;
struct AggregationContainer {
    /* vars*/
    DynamicArray *cfa_varp;
    /* dims */
    DynamicArray *cfa_dimp;
    /* containers (for groups) */
    DynamicArray *cfa_containerp;

    /* file info */
    char* path;
    /* name (if a group) */
    char* name;
}; 


/* create a AggregationContainer and assign it to cfaid */
EXTERNL int cfa_create(const char *path, int *cfa_idp);

/* get the id of a AggregationContainer */
EXTERNL int cfa_inq_id(const char *path, int *cfa_idp);

/* get the number of AggregationContainers */
EXTERNL int cfa_inq_n(int *ncfa);

/* get the AggregationContainer from a cfa_id */
EXTERNL int cfa_get(const int cfa_id, AggregationContainer **agg_cont);

/* close a AggregationContainer */
EXTERNL int cfa_close(const int cfa_id);

/* create a AggregatedDimension, attach it to a cfa_id */
EXTERNL int cfa_def_dim(const int cfa_id, const char *name, 
                        const int len, int *cfa_dim_idp);

/* Get the identifier of an AggregatedDimension */
EXTERNL int cfa_inq_dim_id(const int cfa_id, const char* name, 
                           int *cfa_dim_idp);

/* return the number of AggregatedDimensions that have been defined */
EXTERNL int cfa_inq_ndims(const int cfa_id, int *ndimp);

/* get the AggregatedDimension from a cfa_dim_id */
EXTERNL int cfa_get_dim(const int cfa_id, const int cfa_dim_id, 
                        AggregatedDimension **agg_dim);

/* create an AggregationVariable container, attach it to a cfa_id and one 
or more cfa_dim_ids and assign it to a cfavarid */
EXTERNL int cfa_def_var(const int cfa_id, const char *name, const int ndims, 
                        int *cfa_dim_idsp, int *cfa_var_idp);

/* get the identifier of an AggregationVariable by name */
EXTERNL int cfa_inq_var_id(const int cfa_id, const char* name, 
                           int *cfa_dim_idp);

/* get the number of AggregationVariables defined */
EXTERNL int cfa_inq_nvars(const int cfa_id, int *nvarp);

/* get the AggregationVariable from a cfa_var_id */
EXTERNL int cfa_get_var(const int cfa_id, const int cfa_var_id,
                        AggregationVariable **agg_var);

#define CFA_ERR(cfa_err) if(cfa_err) {printf("CFA error: %i\n", cfa_err); return cfa_err;}
#endif