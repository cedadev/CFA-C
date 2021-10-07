#ifndef __CFA_H__
#define __CFA_H__

#define EXTERNL extern

/* ERROR constants */
#define CFA_NOERR 0                   /* No error */
#define CFA_MEM_ERR -1                /* Cannot create memory */
#define CFA_NOT_FOUND_ERR -10         /* Cannot find CFA Container */
#define CFA_DIM_NOT_FOUND_ERR -20     /* Cannot find CFA Dimension */

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
    int *cfa_dim_idp;
} AggregationVariable;

/* AggregationContainer */
typedef struct AggregationContainer AggregationContainer;
struct AggregationContainer {
    /* vars*/
    AggregationVariable *cfa_varp;
    int  cfa_nvar;
    /* dims */
    AggregatedDimension *cfa_dimp;
    int  cfa_ndim;
    /* containers (for groups) */
    AggregationContainer *cfa_containerp;
    int  cfa_ncontainer;

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
EXTERNL int cfa_inq_n(void);

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
EXTERNL int cfa_inq_ndims(const int cfa_id);

/* get the AggregatedDimension from a cfa_dim_id */
EXTERNL int cfa_get_dim(const int cfa_id, const int cfa_dim_id, 
                        AggregatedDimension **agg_dim);

/* create a AggregationVariable container, attach it to a cfa_id and one 
or more cfa_dim_ids and assign it to a cfavarid */
EXTERNL int cfa_def_var(int cfa_id, const char *name, int ndims, 
                        int *cfa_dim_idsp, int *cfa_var_idp);

#endif