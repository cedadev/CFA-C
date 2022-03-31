#include <stdlib.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"

/* Start of the variables resizeable array in memory */
DynamicArray *cfa_vars = NULL;

/* 
  Fragment dimension resizeable array in memory 
  Defined here as they are only used by the AggregatedData in the 
  AggregatedVariable struct.
*/
DynamicArray *cfa_frag_dims = NULL;
extern DynamicArray *cfa_dims;

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

    /* allocate the AggregatedData struct */
    var_node->cfa_datap = cfa_malloc(sizeof(AggregatedData));
    /* set units and fragments to NULL for now */
    var_node->cfa_datap->units = NULL;
    /* fragments set in cfa_var_def_frag_size */ 
    var_node->cfa_datap->cfa_fragmentsp = NULL; 

    /* no fragments defined yet */
    var_node->cfa_frag_dim_idp = NULL;
    
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
*/
int 
cfa_var_def_agg_instr(const int cfa_id, const int cfa_var_id,
                      const char* instruction,
                      const char* value, const int scalar_location)
{
    /* get the CFA AggregationVariable */
    AggregationVariable *agg_var;
    int err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_CHECK(err);
    if (strcmp(instruction, "location") == 0)
    {
        agg_var->cfa_instructionsp->location = strdup(value);
        agg_var->cfa_instructionsp->location_scaler = scalar_location;
    }
    else if (strcmp(instruction, "file") == 0)
    {
        agg_var->cfa_instructionsp->file = strdup(value);
    }
    else if (strcmp(instruction, "format") == 0)
    {
        agg_var->cfa_instructionsp->format = strdup(value);
        agg_var->cfa_instructionsp->format_scaler = scalar_location;
    }
    else if (strcmp(instruction, "address") == 0)
    {
        agg_var->cfa_instructionsp->address = strdup(value);
    }
    else
        return CFA_AGG_NOT_RECOGNISED;
    return CFA_NOERR;
}

int
_frag_dim_name_exists(const char* frag_name)
{
    /* see if this frag_name is already in the fragments */
    FragmentDimension *exist_frag_dim = NULL;
    int nfragdims = 0;
    int cfa_err = get_array_length(&cfa_frag_dims, &nfragdims);
    for (int f=0; f<nfragdims; f++)
    {
        cfa_err = get_array_node(&(cfa_frag_dims), f, 
                                 (void**)(&exist_frag_dim));
        CFA_CHECK(cfa_err);
        /* first pass through will have one frag_dim->name as NULL, as this has
        just been created and is the FragmentDimension we are trying to create a
        name for */
        if(exist_frag_dim->name)
            if (strcmp(exist_frag_dim->name, frag_name) == 0)
                return 1;
    }
    return 0;
}

char*
_create_frag_dim_name(const char* var_name)
{
    /* Fragment dimension names have to be unique, so this function creates a 
    FragmentDimension name and ensures that it doesn't already exist */
    char* frag_name = cfa_malloc(strlen(var_name)+3);
    strcpy(frag_name, "f_");
    strcat(frag_name, var_name);
    int suffix = 1;
    char* new_frag_name = cfa_malloc(strlen(frag_name)+7);
    strcpy(new_frag_name, frag_name);
    char suffix_array[4];
    while (_frag_dim_name_exists(new_frag_name))
    {
        /* copy, create suffix, add suffix */
        strcpy(new_frag_name, frag_name);
        sprintf(suffix_array, "_%i", suffix);
        strcat(new_frag_name, suffix_array);
        suffix += 1;
    }
    cfa_free(frag_name, strlen(frag_name)+1);

    return new_frag_name;
}

/* add the fragment definitions.  There should be one number per dimension in
the fragments array.  This defines how many times that dimension is 
subdivided */
int 
cfa_var_def_frag_size(const int cfa_id, const int cfa_var_id, 
                      const int *fragments)
{
    /* get the CFA AggregationVariable */
    AggregationVariable *agg_varp = NULL;
    int cfa_err = cfa_get_var(cfa_id, cfa_var_id, &agg_varp);
    CFA_CHECK(cfa_err);

    /* check if FragmentDimensions already defined */
    if (agg_varp->cfa_frag_dim_idp)
        return CFA_VAR_FRAGS_DEF;

    /* create the FragmentDimension DynamicArray if not already created */
    if (!cfa_frag_dims)
    {
        cfa_err = create_array(&(cfa_frag_dims), sizeof(FragmentDimension));
        CFA_CHECK(cfa_err);
    }

    /* create a FragmentDimension for each AggregatedDimension */
    FragmentDimension *frag_dimp = NULL;
    AggregatedDimension *agg_dimp = NULL;
    int cfa_nfragdim = 0;

    /* create the array to hold the FragmentDimension ids */
    agg_varp->cfa_frag_dim_idp = cfa_malloc(sizeof(int) * agg_varp->cfa_ndim);

    /* keep a track of the total number of Fragments defined */
    size_t n_total_frags = 1;

    /* loop over the AggregatedDimensions that this AggregationVariable is
    defined over and create a FragmentDimension for each one with length from
    *fragments parameter
    */
    for (int d=0; d<agg_varp->cfa_ndim; d++)
    {
        /* get the AggregatedDimension and check */
        cfa_err = cfa_get_dim(cfa_id, agg_varp->cfa_dim_idp[d], &agg_dimp);
        CFA_CHECK(cfa_err);
        /* create the array node for the fragment */
        cfa_err = create_array_node(&(cfa_frag_dims), (void**)(&frag_dimp));
        CFA_CHECK(cfa_err);
        /* write the length into the FragmentDimension */
        frag_dimp->length = fragments[d];
        /* get the name, this is the same as the dimension name with a 
        f_ prefix and a number suffix.  The number is required as different
        variables could define different fragment patterns for the same 
        Dimensions */
        frag_dimp->name = _create_frag_dim_name(agg_dimp->name);
        frag_dimp->cfa_dim_id = agg_varp->cfa_dim_idp[d];

        /* set the FragmentDimension reference in the AggregatedDimension to 
        the newly create FragmentDimension */
        cfa_err = get_array_length(&(cfa_frag_dims), &cfa_nfragdim);
        CFA_CHECK(cfa_err);
        agg_varp->cfa_frag_dim_idp[d] = cfa_nfragdim - 1;

        /* product of the fragment dimension lengths = total number of fragments
        */
        n_total_frags *= frag_dimp->length;
    }
    /* create the Fragments DynamicArray if not already created*/
    if (agg_varp->cfa_datap->cfa_fragmentsp)
        return CFA_VAR_FRAGS_DEF;

    cfa_err = create_array(&(agg_varp->cfa_datap->cfa_fragmentsp), 
                           sizeof(Fragment)*n_total_frags);

    /* set the location to NULL so we can check if the Fragment has been
    assigned yet */
    for (size_t f=0; f<n_total_frags; f++)
    {
        Fragment *cfrag = NULL;
        /* create the fragment node */
        cfa_err = create_array_node(&(agg_varp->cfa_datap->cfa_fragmentsp),
                                    (void**)(&cfrag));
        CFA_CHECK(cfa_err);
        cfrag->location = NULL;
    }

    return CFA_NOERR;
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


/* get a FragmentDimension */
int 
cfa_var_get_frag_dim(const int cfa_id, const int cfa_var_id,
                     const int dimn, FragmentDimension **frag_dim)
{
    /* check FragmentDimensions created */
    if (!cfa_frag_dims)
        return CFA_VAR_FRAGS_UNDEF;
    /* get the variable */
    AggregationVariable *agg_var = NULL;
    int cfa_err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_ERR(cfa_err);
    /* check FragDim array created */
    if (!(agg_var->cfa_frag_dim_idp))
        return CFA_VAR_FRAGS_UNDEF;
    /* check that dimn is less than the number of dimensions */
    if (dimn >= agg_var->cfa_ndim)
        return CFA_VAR_FRAG_DIM_NOT_FOUND;

    /* otherwise get from the agg_var */
    cfa_err = get_array_node(&(cfa_frag_dims), 
                             agg_var->cfa_frag_dim_idp[dimn],
                             (void**)(frag_dim));
    CFA_CHECK(cfa_err);
    return CFA_NOERR;
}

/*
free the memory used by the CFA variables
*/
int
_cfa_free_agg_instructions(AggregationVariable *agg_var)
{
    if (!agg_var)
        return CFA_NOERR;
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
    return CFA_NOERR;
}

int
_cfa_free_fragments(AggregationVariable *agg_var)
{
    int cfa_err = CFA_NOERR;

    /* free the Fragments, if defined */
    if (agg_var->cfa_datap)
    {
        /* free units */
        if (agg_var->cfa_datap->units)
        {
            cfa_free(agg_var->cfa_datap->units, 
                     strlen(agg_var->cfa_datap->units)+1);
            agg_var->cfa_datap->units = NULL;
        }
        /* free Fragment definitions */
        Fragment *cfrag = NULL;
        if (agg_var->cfa_datap->cfa_fragmentsp)
        {
            int nfrags = 0;
            cfa_err = get_array_length(&(agg_var->cfa_datap->cfa_fragmentsp),
                                       &nfrags);
            CFA_CHECK(cfa_err);
            for (int f=0; f<nfrags; f++)
            {
                cfa_err = get_array_node(&(agg_var->cfa_datap->cfa_fragmentsp),
                                         f, (void**)(&cfrag));
                CFA_CHECK(cfa_err);
                /* Fragment is defined if cfrag->location != NULL */
                /* free location */
                if (!(cfrag->location))
                    continue;
                else
                {
                    cfa_free(cfrag->location, sizeof(int) * agg_var->cfa_ndim);
                    cfrag->location = NULL;
                }
                /* free file */
                if (cfrag->file)
                {
                    cfa_free(cfrag->file, strlen(cfrag->file)+1);
                    cfrag->file = NULL;
                }
                /* free format */
                if (cfrag->format)
                {
                    cfa_free(cfrag->format, strlen(cfrag->format)+1);
                    cfrag->format = NULL;
                }
                /* free address */
                if (cfrag->address)
                {
                    cfa_free(cfrag->address, strlen(cfrag->address)+1);
                    cfrag->address = NULL;
                }
                /* free units */
                if (cfrag->units)
                {
                    cfa_free(cfrag->units, strlen(cfrag->units)+1);
                    cfrag->units = NULL;
                }
                /* free datatype */
                if (cfrag->cfa_dtype)
                {
                    cfa_free(cfrag->cfa_dtype, sizeof(cfrag->cfa_dtype));
                    cfrag->cfa_dtype = NULL;
                }
            }
            /* free the array */
            free_array(&(agg_var->cfa_datap->cfa_fragmentsp));
        }
        /* free the AggregatedData */
        cfa_free(agg_var->cfa_datap, sizeof(AggregatedData));
        agg_var->cfa_datap = NULL;
    }

    /* also free the FragmentDimensions, if defined */
    if (agg_var->cfa_frag_dim_idp)
    {
        FragmentDimension *frag_dim = NULL;
        for (int d=0; d<agg_var->cfa_ndim; d++)
        {
            cfa_err = get_array_node(&(cfa_frag_dims),
                                    agg_var->cfa_frag_dim_idp[d],
                                    (void**)(&frag_dim));
            CFA_CHECK(cfa_err);
            if (frag_dim->name)
            {
                cfa_free(frag_dim->name, strlen(frag_dim->name)+7);
                frag_dim->name = NULL;
            }
        }
        /* free the array containing the ids of the FragmentDimensions*/
        cfa_free(agg_var->cfa_frag_dim_idp, sizeof(int) * agg_var->cfa_ndim);
    }
    return cfa_err;
}

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
        cfa_err = _cfa_free_agg_instructions(agg_var);
        CFA_CHECK(cfa_err);
        cfa_err = _cfa_free_fragments(agg_var);
        CFA_CHECK(cfa_err);
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
            /* also free the FragmentDimension array */
            cfa_err = free_array(&cfa_frag_dims);
            CFA_CHECK(cfa_err);
            cfa_frag_dims = NULL;
        }
    }

    return CFA_NOERR;
}
