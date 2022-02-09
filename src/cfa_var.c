#include <stdlib.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"

/* Start of the variables resizeable array in memory */
DynamicArray *cfa_vars = NULL;

extern int get_type_size(const cfa_type);

/* 
create a AggregationVariable container, attach it to a AggregationContainer and one or more AggregatedDimension(s) and assign it to a cfa_var_id
*/
int
cfa_def_var(int cfa_id, const char *name, const cfa_type vtype, 
            int *cfa_var_idp)
{
    /* get the aggregation container */
    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* if no variables have been defined previously, then the cfa_vars pointer
       will still be NULL : create the array */
    if (!cfa_vars)
    {
        cfa_err = create_array(&(cfa_vars), sizeof(AggregationVariable));
        CFA_CHECK(cfa_err);
    }

    /* Allocate and return the array node */
    AggregationVariable *var_node = NULL;
    cfa_err = create_array_node(&(cfa_vars), (void**)(&var_node));
    CFA_CHECK(cfa_err);

    /* assign the name */
    var_node->name = strdup(name);

    /* assign the type */
    var_node->cfa_dtype.type = vtype;
    var_node->cfa_dtype.size = get_type_size(vtype);

    /* allocate the AggregationVariable identifier */
    int cfa_nvar = 0;
    cfa_err = get_array_length(&(cfa_vars), &cfa_nvar);
    CFA_CHECK(cfa_err);

    /* allocate the AggregationInstructions struct */
    var_node->cfa_instructionsp = cfa_malloc(sizeof(AggregationInstructions));
    var_node->cfa_instructionsp->address = NULL;
    var_node->cfa_instructionsp->location = NULL;
    var_node->cfa_instructionsp->file = NULL;
    var_node->cfa_instructionsp->format = NULL;
    
    /* write back the cfa_var_id */
    *cfa_var_idp = cfa_nvar - 1;

    /* assign to the container */
    agg_cont->cfa_varids[agg_cont->n_vars++] = *cfa_var_idp;

    return CFA_NOERR;
}

/*
add the AggregatedDimensions to the AggregationVariable.
the dimensions have to be previously defined in the AggregationContainer
*/
int cfa_var_def_dims(const int cfa_id, const int cfa_var_id,
                     const int ndims, const int *cfa_dim_idsp)
{
    /* get the AggregationVariable from the ids */
    AggregationVariable *agg_var = NULL;
    int cfa_err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_CHECK(cfa_err);

    /* check that the dimension ids have been added to the container already.
       this basically means that the dim_id(s) passed in via the cfa_dim_idsp 
       pointer are greater than 0 and less than the number of dimensions */
    int n_cfa_dims = -1;
    cfa_err = cfa_inq_ndims(cfa_id, &n_cfa_dims);
    CFA_CHECK(cfa_err);

    for (int i=0; i<ndims; i++)
    {
        /* check the dimid is in range */
        if (cfa_dim_idsp[i] < 0 || cfa_dim_idsp[i] >= n_cfa_dims)
            return CFA_DIM_NOT_FOUND_ERR;
        /* check the dimension has been added to the AggregationContainer */
        AggregatedDimension* agg_dim;
        cfa_err = cfa_get_dim(cfa_id, cfa_dim_idsp[i], &agg_dim);
        CFA_CHECK(cfa_err);
    }

    /* assign the number of dimensions and copy the dimension array */
    agg_var->cfa_ndim = ndims;
    agg_var->cfa_dim_idp = (int*) cfa_malloc(sizeof(int) * ndims);
    if (!(agg_var->cfa_dim_idp))
        return CFA_MEM_ERR;
    memcpy(agg_var->cfa_dim_idp, cfa_dim_idsp, sizeof(int) * ndims);

    return CFA_NOERR;
}

/* add the AggregationInstructions from a string 
   the string follows the key: value pair format
   keys are: location:, address:, file:, format: (including the colon)
   multiple key: value pairs can be separated by a space
*/
int 
cfa_var_def_agg_instr(const int cfa_id, const int cfa_var_id,
                      const char* agg_instr_key, const char* agg_instr_val)
{
    /* get the CFA AggregationVariable */
    AggregationVariable *agg_var;
    int err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_CHECK(err);
    const char* LOCATION = "location";
    if (strstr(agg_instr_key, LOCATION))
        agg_var->cfa_instructionsp->location = strdup(agg_instr_val);
    /* file */
    const char *FILE = "file";
    if (strstr(agg_instr_key, FILE))
        agg_var->cfa_instructionsp->file = strdup(agg_instr_val);
    /* format */
    const char *FORMAT = "format";
    if (strstr(agg_instr_key, FORMAT))
        agg_var->cfa_instructionsp->format = strdup(agg_instr_val);
    /* address */
    const char *ADDRESS = "address";
    if (strstr(agg_instr_key, ADDRESS))
        agg_var->cfa_instructionsp->address = strdup(agg_instr_val);
    return CFA_NOERR;
}

/* add the fragment definitions.  There should be one number per dimension in
the fragments array.  This defines how many times that dimension is 
subdivided */
int 
cfa_var_def_frag(const int cfa_id, const int cfa_var_id, const int *fragments)
{
    /* get the CFA AggregationVariable */
    AggregationVariable *agg_var;
    int err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_CHECK(err);
}

/*
get the identifier of an AggregationVariable by name
*/
int
cfa_inq_var_id(const int cfa_id, const char* name, int *cfa_var_idp)
{
    /* get the AggregationContainer with cfa_id */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* search through the variables, looking for the matching name */

    AggregationVariable *cvar = NULL;
    for (int i=0; i<agg_cont->n_vars; i++)
    {
        /* variables that belong to a closed AggregationContainer have their
        name set to NULL */
        cfa_err = get_array_node(&(cfa_vars), 
                                 agg_cont->cfa_varids[i], 
                                 (void**)(&cvar));
        CFA_CHECK(cfa_err);

        if (!(cvar->name))
            continue;
        if (strcmp(cvar->name, name) == 0)
        {
            /* found, so assign and return */
            *cfa_var_idp = agg_cont->cfa_varids[i];
            return CFA_NOERR;
        }
    }
    return CFA_VAR_NOT_FOUND_ERR;
}

/*
get the number of AggregationVariables defined
*/
int 
cfa_inq_nvars(const int cfa_id, int *nvarp)
{
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);
    *nvarp = agg_cont->n_vars;

    return CFA_NOERR;
}

/* 
get the ids for the AggregationVariables in the AggregationContainer 
*/
int 
cfa_inq_var_ids(const int cfa_id, int **varids)
{
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    *varids = agg_cont->cfa_varids;
    return CFA_NOERR;
}

/*
get the AggregationVariable from a cfa_var_id
*/
int
cfa_get_var(const int cfa_id, const int cfa_var_id,
            AggregationVariable **agg_var)
{
    /*
    check that the array has been created yet 
    */
    if (!cfa_vars)
        return CFA_VAR_NOT_FOUND_ERR;

#ifdef _DEBUG
    /* check id is in range */
    int cfa_nvars = 0;
    int cfa_err_v = get_array_length(&(cfa_vars), &cfa_nvars);
    CFA_CHECK(cfa_err_v);
    if (cfa_nvars == 0)
        return CFA_VAR_NOT_FOUND_ERR;
    if (cfa_var_id < 0 || cfa_var_id >= cfa_nvars)
        return CFA_VAR_NOT_FOUND_ERR;
    /* check id belongs to AggregationContainer */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);
    int var_in_cont = 0;
    for (int i=0; i<agg_cont->n_vars; i++)
        if (agg_cont->cfa_varids[i] == cfa_var_id)
        {
            var_in_cont = 1;
            break;
        }
    if (!var_in_cont)
        return CFA_DIM_NOT_FOUND_ERR;
#endif
    /* 
    check that the name is not NULL.  On cfa_close, the name is set to NULL 
    */
    cfa_err = get_array_node(&(cfa_vars), cfa_var_id, (void**)(agg_var));
    CFA_CHECK(cfa_err);

    if (!(*agg_var)->name)
    {
        agg_var = NULL;
        return CFA_VAR_NOT_FOUND_ERR;
    }

    return CFA_NOERR;
}

/*
free the memory used by the CFA variables
*/
int
cfa_free_vars(const int cfa_id)
{
    /* get the AggregationContainer */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* loop over all the variables and free any associated memory */
    AggregationVariable *agg_var = NULL;
    for (int i=0; i<agg_cont->n_vars; i++)
    {
        cfa_err = get_array_node(&(cfa_vars), 
                                 agg_cont->cfa_varids[i], 
                                 (void**)(&agg_var));
        CFA_CHECK(cfa_err);
        if (agg_var->name)
        {
            cfa_free(agg_var->name, strlen(agg_var->name)+1);
            agg_var->name = NULL;
        }
        if (agg_var->cfa_dim_idp && agg_var->cfa_ndim > 0)
        {
            cfa_free(agg_var->cfa_dim_idp, sizeof(int) * agg_var->cfa_ndim);
            agg_var->cfa_dim_idp = NULL;
        }
        if (agg_var->cfa_instructionsp)
        {
            /* free the cfa instructions and their strings */
            if (agg_var->cfa_instructionsp->location)
                cfa_free(agg_var->cfa_instructionsp->location,
                         strlen(agg_var->cfa_instructionsp->location)+1);
            if (agg_var->cfa_instructionsp->address)
                cfa_free(agg_var->cfa_instructionsp->address,
                         strlen(agg_var->cfa_instructionsp->address)+1);
            if (agg_var->cfa_instructionsp->file)
                cfa_free(agg_var->cfa_instructionsp->file,
                         strlen(agg_var->cfa_instructionsp->file)+1);
            if (agg_var->cfa_instructionsp->format)
                cfa_free(agg_var->cfa_instructionsp->format,
                         strlen(agg_var->cfa_instructionsp->format)+1);
            cfa_free(agg_var->cfa_instructionsp, 
                     sizeof(AggregationInstructions));
            agg_var->cfa_instructionsp = NULL;
        }
    }
    /* check whether all cfa_vars are free (name=NULL) and free the DynamicArray
    holding all the AggregationVariables if they are */
    /* first check that cfa_vars != NULL - which it will be if it hasn't been
    created yet, or has been previously destroyed */
    if (cfa_vars)
    {
        int n_vars = 0;
        cfa_err = get_array_length(&cfa_vars, &n_vars);
        CFA_CHECK(cfa_err);
        int nfv = 0;
        AggregationVariable *cfa_node = NULL;
        
        for (int i=0; i<n_vars; i++)
        {
            cfa_err = get_array_node(&cfa_vars, i, (void**)(&(cfa_node)));
            CFA_CHECK(cfa_err);
            /* name == NULL indicates node has been freed */
            if (cfa_node->name)
                nfv += 1;
        }
        /* if number of non-free dimensions is 0 then free the array */
        if (nfv == 0)
        {
            cfa_err = free_array(&cfa_vars);
            CFA_CHECK(cfa_err);
            cfa_vars = NULL;
        }
    }

    return CFA_NOERR;
}