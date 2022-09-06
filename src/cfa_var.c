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

extern void __free_str_via_pointer(char**);

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
    /* fragments set in cfa_var_def_frag_num */ 
    var_node->cfa_datap->cfa_fragmentsp = NULL; 
    /* locations set in cfa_var_def_frag_num */
    var_node->cfa_datap->cfa_fragmentsp = NULL;

    /* no fragments defined yet */
    var_node->cfa_frag_dim_idp[0] = -1;
    
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
    memcpy(agg_var->cfa_dim_idp, cfa_dim_idsp, sizeof(int) * ndims);

    return CFA_NOERR;
}

/* add the AggregationInstructions from a string 
*/
int 
cfa_var_def_agg_instr(const int cfa_id, const int cfa_var_id,
                      const char* instruction,
                      const char* value, const bool scalar)
{
    /* get the CFA AggregationVariable */
    AggregationVariable *agg_var;
    int err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_CHECK(err);
    if (strncmp(instruction, "location", 8) == 0)
    {
        agg_var->cfa_instructionsp->location = strdup(value);
        agg_var->cfa_instructionsp->location_scalar = scalar;
    }
    else if (strncmp(instruction, "file", 4) == 0)
    {
        agg_var->cfa_instructionsp->file = strdup(value);
    }
    else if (strncmp(instruction, "format", 6) == 0)
    {
        agg_var->cfa_instructionsp->format = strdup(value);
        agg_var->cfa_instructionsp->format_scalar = scalar;
    }
    else if (strncmp(instruction, "address", 7) == 0)
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

/*
create the Fragment Dimensions based on the fragment definitions
*/
int 
_create_fragment_dimensions(int cfa_id, AggregationVariable *agg_varp,
                            const int *fragments)
{
    int cfa_err = 0;
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
        the newly created FragmentDimension */
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
                           sizeof(Fragment) * n_total_frags);

    /* create all of the fragment nodes so we can just write to them */
    for (size_t f=0; f<n_total_frags; f++)
    {
        Fragment *cfrag = NULL;
        /* create the fragment node */
        cfa_err = create_array_node(&(agg_varp->cfa_datap->cfa_fragmentsp),
                                    (void**)(&cfrag));
        CFA_CHECK(cfa_err);
    }
    return CFA_NOERR;
}

/* 
add the fragment definitions.  There should be one number per dimension in
the fragments array.  This defines how many times that dimension is 
subdivided 
*/
int 
cfa_var_def_frag_num(const int cfa_id, const int cfa_var_id, 
                     const int *fragments)
{
    /* get the CFA AggregationVariable */
    AggregationVariable *agg_varp = NULL;
    int cfa_err = cfa_get_var(cfa_id, cfa_var_id, &agg_varp);
    CFA_CHECK(cfa_err);

    /* check if FragmentDimensions already defined */
    if (agg_varp->cfa_frag_dim_idp[0] != -1)
        return CFA_VAR_FRAGS_DEF;

    /* create the fragment dimensions */
    cfa_err = _create_fragment_dimensions(cfa_id, agg_varp, fragments);

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

    int cfa_err = CFA_NOERR;
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
    cfa_err = cfa_get(cfa_id, &agg_cont);
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
    CFA_CHECK(cfa_err);
    /* check FragDim array created */
    if (agg_var->cfa_frag_dim_idp[0] == -1)
        return CFA_VAR_FRAGS_UNDEF;
    /* check that dimn is less than the number of dimensions */
    if (dimn >= agg_var->cfa_ndim)
        return CFA_VAR_FRAG_DIM_NOT_FOUND;

    /* otherwise get from the FragmentDimension from the agg_var */
    cfa_err = get_array_node(&(cfa_frag_dims), 
                             agg_var->cfa_frag_dim_idp[dimn],
                             (void**)(frag_dim));
    CFA_CHECK(cfa_err);
    return CFA_NOERR;
}

/* Calculate the linear location in the Fragment Array of the Fragment indexed
at fraglocp */
int
_multidim_to_linear_index(const AggregationVariable *agg_var, 
                           const size_t *fraglocp, int *L)
{
    /* linear location is sum of the product of the index for the dimension and 
       the size of the less varying dimensions. i.e., for a 4 dimensional netCDF
       style file:
            L = t * len(z) * len(y) * len(x) +
                z * len(y) * len(x) + 
                y * len(x) + 
                x
    */
    int n_dims = agg_var->cfa_ndim;
    *L = 0;
    int cfa_err = 0;
    FragmentDimension *frag_dim;

    for (int v=0; v<n_dims; v++)
    {
        int D=1;
        for (int d=v+1; d<n_dims; d++)
        {
            cfa_err = get_array_node(&(cfa_frag_dims), 
                                    agg_var->cfa_frag_dim_idp[d],
                                    (void**)(&frag_dim));
            CFA_CHECK(cfa_err);
            D *= frag_dim->length;
        }
#ifdef _DEBUG
        if (fraglocp)
        {
            /* range check in DEBUG mode */
            cfa_err = get_array_node(&(cfa_frag_dims), 
                                    agg_var->cfa_frag_dim_idp[v],
                                    (void**)(&frag_dim));
            CFA_CHECK(cfa_err);
            if (fraglocp[v] >= (size_t)(frag_dim->length))
                return CFA_BOUNDS_ERR;
        }
#endif
        *L += D * fraglocp[v];
    }
    return CFA_NOERR;;
}

/* This is the reverse of the above - calculate a multi dimensional index from
a linear one */
int
_linear_index_to_multidim(const AggregationVariable *agg_var, 
                          int L, size_t *fraglocp)
{
    /* multidimensional index is the linear index, divided by the product of
    the other subsequent dimensions, then modded by the length of the dimensions
            t = (L / (len(z) * len(y) * len(x)) % len(t)
            z = (L / (len(y) * len(x)) % len(z)
            y = L / len(x) % len(y)
            x = L % len(x)
    */
    int n_dims = agg_var->cfa_ndim;
    int cfa_err = 0;
    FragmentDimension *frag_dim;

    for (int v=0; v<n_dims; v++)
    {
        int D=1;
        for (int d=v+1; d<n_dims; d++)
        {
            cfa_err = get_array_node(&(cfa_frag_dims), 
                                    agg_var->cfa_frag_dim_idp[d],
                                    (void**)(&frag_dim));
            CFA_CHECK(cfa_err);
            D *= frag_dim->length;
        }
        cfa_err = get_array_node(&(cfa_frag_dims), 
                                agg_var->cfa_frag_dim_idp[v],
                                (void**)(&frag_dim));
        CFA_CHECK(cfa_err);
        fraglocp[v] = (L / D) % frag_dim->length;
#ifdef _DEBUG
        if (fraglocp)
        {
            /* range check in DEBUG mode */
            cfa_err = get_array_node(&(cfa_frag_dims), 
                                    agg_var->cfa_frag_dim_idp[v],
                                    (void**)(&frag_dim));
            CFA_CHECK(cfa_err);
            if (fraglocp[v] >= (size_t)(frag_dim->length))
                return CFA_BOUNDS_ERR;
        }
#endif
    }
    return CFA_NOERR;;
}

int
_data_location_to_fragment_index(const AggregationVariable *agg_var, 
                                  const size_t *data_location,
                                  size_t *frag_index)
{
    /* calculate the fragment index from the data location and information in
    the aggregation variable */
    int cfa_err = 0;
    FragmentDimension *frag_dim = NULL;
    AggregatedDimension *agg_dim = NULL;
    for (int d=0; d<agg_var->cfa_ndim; d++)
    {
        /* get the FragmentDimension for this dimension of the variable */
        cfa_err = get_array_node(
            &cfa_frag_dims, agg_var->cfa_frag_dim_idp[d], (void**)(&frag_dim)
        );
        CFA_CHECK(cfa_err);
        /* get the Dimension */
        cfa_err = get_array_node(
            &cfa_dims, agg_var->cfa_dim_idp[d], (void**)(&agg_dim)
        );
        CFA_CHECK(cfa_err);
        frag_index[d] = (data_location[d] * frag_dim->length) /
                         agg_dim->length;
    }

    return CFA_NOERR;
}

int
_fragment_index_to_data_location(const AggregationVariable *agg_var, 
                                  const size_t *frag_index,
                                  size_t *data_location)
{
    /* this is the reverse of the above - get a data location from a fragment
    index */
    int cfa_err = 0;
    FragmentDimension *frag_dim;
    AggregatedDimension *agg_dim;

    for (int d=0; d<agg_var->cfa_ndim; d++)
    {
        cfa_err = get_array_node(
            &cfa_frag_dims, agg_var->cfa_frag_dim_idp[d], (void**)(&frag_dim)
        );
        CFA_CHECK(cfa_err);
        cfa_err = get_array_node(
            &cfa_dims, agg_var->cfa_dim_idp[d], (void**)(&agg_dim)
        );
        CFA_CHECK(cfa_err);    
        size_t frag_size = agg_dim->length / frag_dim->length;
        data_location[d<<1] = frag_index[d] * frag_size;
        data_location[(d<<1)+1] = data_location[d<<1] + frag_size;
    }

    return CFA_NOERR;
}

/* generalised method to get the linear index into a DynamicArray, when either
the frag_location or data_location is passed in */
int
_get_linear_index(AggregationVariable *agg_var,
                  const size_t *frag_location, const size_t *data_location,
                  int *L)
{
    int cfa_err = 0;
    size_t frag_location_calc[MAX_DIMS];
    if (frag_location)
    {
        cfa_err = _multidim_to_linear_index(agg_var, frag_location, L);
        CFA_CHECK(cfa_err);
    }
    else if (data_location)
    {
        /* calculate the fragment index if NULL passed in */
        cfa_err = _data_location_to_fragment_index(
                      agg_var, data_location, frag_location_calc
                    );
        CFA_CHECK(cfa_err);
        cfa_err = _multidim_to_linear_index(agg_var, frag_location_calc, L);
        CFA_CHECK(cfa_err);
    }
    else
    {
        /* either frag_location or data_location is required */
        return CFA_VAR_NO_FRAG_INDEX;
    }

    return CFA_NOERR;
}

/* write a single Fragment for a variable requires an
   external writing functions */
extern int cfa_netcdf_write1_frag(const int, const int, const int, 
                                  const Fragment*);

int 
cfa_var_put1_frag(const int cfa_id, const int cfa_var_id,
                  const size_t *frag_location, const size_t *data_location,
                  const char *file, const char *format, 
                  const char *address, const char *units)
{
    /* get the variable */
    AggregationVariable *agg_var = NULL;
    int cfa_err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_CHECK(cfa_err);
    /* check that the array has been created */
    if (!(agg_var->cfa_datap->cfa_fragmentsp))
        return(CFA_VAR_FRAGS_UNDEF);
    /* Calculate the linear position in the array.  This function will check 
    that the location is not out of bounds if _DEBUG is set*/
    int L = 0;
    cfa_err = _get_linear_index(agg_var, frag_location, data_location, &L);
    CFA_CHECK(cfa_err);

    /* get the fragment at the linear index */
    Fragment *frag;
    cfa_err = get_array_node(&(agg_var->cfa_datap->cfa_fragmentsp), L,
                             (void**)(&frag));
    CFA_CHECK(cfa_err);
    /* record the linear index, we might need it later for searching / slicing 
    */
    frag->linear_index = L;
    /* copy the data location - this is a pair of (loc,span) for each Dimension
    and points into the AggregatedData */
    if (data_location)
    {
        size_t size = (sizeof(size_t) << 1) * agg_var->cfa_ndim;
        if (frag->location == NULL)
            frag->location = cfa_malloc(size);
        memcpy(frag->location, data_location, size);
    }
    else if (frag_location)
    {
        /* calculate the data_location from the frag_location and NULL passed
        into the data_location */
        size_t size = (sizeof(size_t) << 1) * agg_var->cfa_ndim;
        if (frag->location == NULL)
            frag->location = cfa_malloc(size);
        cfa_err = _fragment_index_to_data_location(
                      agg_var, frag_location, frag->location
                    );
        CFA_CHECK(cfa_err);
    }
    else
    {
         /* either data_location or frag_location is required */
        return CFA_VAR_NO_FRAG_INDEX;
    }
    /* get the fragment at the linear position and write in the index details */
    cfa_err = get_array_node(&(agg_var->cfa_datap->cfa_fragmentsp), L,
                             (void**)(&frag));
    CFA_CHECK(cfa_err);

    /* create the memory for the fragment index */
    size_t size = sizeof(size_t) * agg_var->cfa_ndim;
    if (frag->index == NULL)
        frag->index = cfa_malloc(size);
    /* copy the frag location - this is a single index into each 
    FragmentDimension */
    if (frag_location)
        memcpy(frag->index, frag_location, size);
    else
    {
        size_t frag_location_calc[MAX_DIMS];
        cfa_err = _data_location_to_fragment_index(
                      agg_var, data_location, frag_location_calc
                    );
        memcpy(frag->index, frag_location_calc, size);
    }

    /* check if the values are NULL before copying - NULL means "missing value"
    */
    if (frag->file != NULL) cfa_free(frag->file, strlen(frag->file)+1);
    if (file != NULL) frag->file = strdup(file);
    else frag->file = NULL;

    if (frag->format != NULL) cfa_free(frag->format, strlen(frag->format)+1);
    if (format != NULL) frag->format = strdup(format);
    else frag->format = NULL;

    if (frag->address != NULL) cfa_free(frag->address, strlen(frag->address)+1);
    if (address != NULL) frag->address = strdup(address);
    else frag->address = NULL;

    if (frag->units != NULL) cfa_free(frag->units, strlen(frag->units)+1);
    if (units != NULL) frag->units = strdup(units);
    else frag->units = NULL;

    /* copy DataType from parent variable */
    frag->cfa_dtype = agg_var->cfa_dtype;

    /* is the data serialised?  if it is then we can write out the fragment */
    AggregationContainer *agg_cont = NULL;
    cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);
    if (agg_cont->serialised)
    {
        switch (agg_cont->format)
        {
            case CFA_NETCDF:
                cfa_err = cfa_netcdf_write1_frag(agg_cont->x_id, cfa_id, 
                                                 cfa_var_id, frag);
                CFA_CHECK(cfa_err);
                agg_cont->serialised = 1;
            break;
            case CFA_UNKNOWN:
            default:
                return CFA_UNKNOWN_FILE_FORMAT;
        }
    }

    return CFA_NOERR;
}

/* read a single Fragment for a variable requires an
   external read function */
extern int cfa_netcdf_read1_frag(const int, const int, const int, 
                                 const Fragment*);

int 
cfa_var_get1_frag(const int cfa_id, const int cfa_var_id,
                  const size_t *frag_location,
                  const size_t *data_location,
                  const Fragment **ret_frag)
{
    /* get the variable */
    AggregationVariable *agg_var = NULL;
    int cfa_err = cfa_get_var(cfa_id, cfa_var_id, &agg_var);
    CFA_CHECK(cfa_err);
    /* check that the array has been created */
    if (!(agg_var->cfa_datap->cfa_fragmentsp))
        return(CFA_VAR_FRAGS_UNDEF);
    
    /* Calculate the linear position in the array.  This function will check 
    that the location is not out of bounds if _DEBUG is set*/
    int L = 0;
    cfa_err = _get_linear_index(agg_var, frag_location, data_location, &L);
    CFA_CHECK(cfa_err);

    /* get the fragment at the linear index */
    Fragment *frag;
    cfa_err = get_array_node(&(agg_var->cfa_datap->cfa_fragmentsp), L,
                             (void**)(&frag));
    CFA_CHECK(cfa_err);
    frag->linear_index = L;

    /* if the fragment location is NULL then we have to fetch the fragment from
    the Parser */
    AggregationContainer *agg_cont = NULL;
    cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);
    if (frag->location == NULL)
    {
        switch (agg_cont->format)
        {
            case CFA_NETCDF:
                cfa_err = cfa_netcdf_read1_frag(agg_cont->x_id, cfa_id, 
                                                cfa_var_id, frag);
                CFA_CHECK(cfa_err);
                *ret_frag = frag;
            break;
            case CFA_UNKNOWN:
            default:
                return CFA_UNKNOWN_FILE_FORMAT;
        }
    }
    else
    {
        /* return the already read in fragment */
        *ret_frag = frag;
    }

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
        __free_str_via_pointer(&(agg_var->cfa_instructionsp->location));
        __free_str_via_pointer(&(agg_var->cfa_instructionsp->address));
        __free_str_via_pointer(&(agg_var->cfa_instructionsp->file));
        __free_str_via_pointer(&(agg_var->cfa_instructionsp->format));
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
                /* free location array */
                if (cfrag->location)
                {
                    cfa_free(cfrag->location, 
                             sizeof(size_t) * agg_var->cfa_ndim * 2);
                }
                /* free index array */
                if (cfrag->index)
                {
                    cfa_free(cfrag->index,
                             sizeof(size_t) * agg_var->cfa_ndim);
                }
                /* free file, format, address and units strings */
                __free_str_via_pointer(&(cfrag->file));
                __free_str_via_pointer(&(cfrag->format));
                __free_str_via_pointer(&(cfrag->address));
                __free_str_via_pointer(&(cfrag->units));
            }
            /* free the array */
            free_array(&(agg_var->cfa_datap->cfa_fragmentsp));
        }
        /* free the AggregatedData */
        cfa_free(agg_var->cfa_datap, sizeof(AggregatedData));
        agg_var->cfa_datap = NULL;
    }

    /* also free the FragmentDimensions, if defined */
    if (agg_var->cfa_frag_dim_idp[0] != - 1)
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
        if (agg_var->cfa_dim_idp[0] != -1)
            agg_var->cfa_dim_idp[0] = -1;
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
            if (cfa_vars)
            {
                cfa_err = free_array(&cfa_vars);
                CFA_CHECK(cfa_err);
                cfa_vars = NULL;
            }
            /* also free the FragmentDimension array */
            if (cfa_frag_dims)
            {
                cfa_err = free_array(&cfa_frag_dims);
                CFA_CHECK(cfa_err);
                cfa_frag_dims = NULL;
            }
        }
    }

    return CFA_NOERR;
}
