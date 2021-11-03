#include <stdlib.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"

/* 
create a AggregationVariable container, attach it to a AggregationContainer and one or more AggregatedDimension(s) and assign it to a cfa_var_id
*/
int
cfa_def_var(int cfa_id, const char *name, int ndims, int *cfa_dim_idsp, 
            int *cfa_var_idp)
{
    /* get the aggregation container */
    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    if (cfa_err)
        return cfa_err;
    /*
    if no variables have been defined previously, then the agg_cont->cfa_varp 
    pointer will be NULL
    */
    if (!(agg_cont->cfa_varp))
    {
        cfa_err = create_array(&(agg_cont->cfa_varp),
                    sizeof(AggregationVariable));
        if (cfa_err)
            return cfa_err;
    }
    /* Array is create so now allocate and return the array node */
    AggregationVariable *var_node = NULL;
    cfa_err = create_array_node(&(agg_cont->cfa_varp), (void**)(&var_node));
    if (cfa_err)
        return cfa_err;

    /* assign the name */
    var_node->name = (char*) cfa_malloc(sizeof(char) * strlen(name));
    if (!(var_node->name))
        return CFA_MEM_ERR;
    strcpy(var_node->name, name);

    /* assign the number of dimensions and copy the dimension array */
    var_node->cfa_ndim = ndims;
    var_node->cfa_dim_idp = (int*) cfa_malloc(sizeof(int) * ndims);
    if (!(var_node->cfa_dim_idp))
        return CFA_MEM_ERR;
    memcpy(var_node->cfa_dim_idp, cfa_dim_idsp, sizeof(int) * ndims);

    /* allocate the AggregationVariable identifier */
    int cfa_nvar = 0;
    cfa_err = get_array_length(&(agg_cont->cfa_varp), &cfa_nvar);
    if (cfa_err)
        return cfa_err;
    *cfa_var_idp = cfa_nvar - 1;

    return CFA_NOERR;
}