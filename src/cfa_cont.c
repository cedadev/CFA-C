#include <stdlib.h>
#include <string.h>

#include "cfa.h"
#include "cfa_mem.h"

/* 
AggregationContainers will be added to the existing cfa_conts defined in cfa.c 
*/
extern DynamicArray *cfa_conts;

extern void __free_str_via_pointer(char**);

/* 
create an AggregationContainer within another AggregationContainer 
*/
int 
cfa_def_cont(const int cfa_id, const char* name, int *cfa_cont_idp)
{
    /* check that the cfa_conts has been created */
    if (!cfa_conts)
        return CFA_NOT_FOUND_ERR;

    /* get the aggregation container that we are going to create a new 
    container in */
    AggregationContainer* agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* Allocate and return the array node (AggregationContainer) */
    AggregationContainer *cont_node = NULL;
    cfa_err = create_array_node(&cfa_conts, (void**)(&cont_node));
    CFA_CHECK(cfa_err);

    /* assign the name, path to NULL */
    cont_node->name = strdup(name),
    cont_node->path = NULL;

    /* set number of vars, dims and containers to 0 */
    cont_node->n_vars = 0;
    cont_node->n_dims = 0;
    cont_node->n_conts = 0;

    /* set the serialised to false and external id to -1*/
    cont_node->serialised = 0;
    cont_node->x_id = -1;

    /* get the identifier as the last node of the container array */
    int cfa_ncont = 0;
    cfa_err = get_array_length(&cfa_conts, &cfa_ncont);
    CFA_CHECK(cfa_err);
    *cfa_cont_idp = cfa_ncont-1;

    /* also assign to the parent container */
    agg_cont->cfa_contids[agg_cont->n_conts++] = cfa_ncont-1;

    return CFA_NOERR;
}

/* 
get the identifier of an AggregationContainer within another 
AggregationContainer, using the name 
*/
int 
cfa_inq_cont_id(const int cfa_id, const char *name, int *cfa_cont_idp)
{
    /* get the parent AggregationContainer */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* search through the containers, looking for the matching name */
    AggregationContainer *ccont = NULL;
    for (int i=0; i<agg_cont->n_conts; i++)
    {
        cfa_err = get_array_node(&cfa_conts, 
                                 agg_cont->cfa_contids[i],
                                 (void**)(&ccont));
        CFA_CHECK(cfa_err);

        if (!(ccont->name))
            continue;
        if (strcmp(ccont->name, name) == 0)
        {
            *cfa_cont_idp = agg_cont->cfa_contids[i];
            return CFA_NOERR;
        }
    }
    return CFA_NOT_FOUND_ERR;
}

/* 
return the number AggregationContainers inside another AggregationContainer
*/
int 
cfa_inq_nconts(const int cfa_id, int *ncontp)
{
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    *ncontp = agg_cont->n_conts;
    return CFA_NOERR;
}

/* 
get the ids for the AggregationContainers in the AggregationContainer
*/
int
cfa_inq_cont_ids(const int cfa_id, int **contids)
{
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);
    *contids = agg_cont->cfa_contids;
    return CFA_NOERR;
}

/* 
get the AggregationContainer from a cfa_cont_id 
*/
int 
cfa_get_cont(const int cfa_id, const int cfa_cont_id, 
             AggregationContainer **agg_cont)
{
    /* can just alias this to cfa_get */
    return cfa_get(cfa_cont_id, agg_cont);
}

/*
free the memory used by the AggregationContainers and the AggregatedVariables 
and AggregatedDimensions they contain
*/
extern int cfa_free_vars(const int);
extern int cfa_free_dims(const int);

int
cfa_free_cont(const int cfa_id)
{
    /* get the aggregation container struct */
    AggregationContainer *agg_cont = NULL;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);

    /* free the variables */
    cfa_err = cfa_free_vars(cfa_id);
    CFA_CHECK(cfa_err);
    /* free the dimensions */
    cfa_err = cfa_free_dims(cfa_id);
    CFA_CHECK(cfa_err);

    /* free the sub-containers (groups) - recursive call */
    for (int g=0; g<agg_cont->n_conts; g++)
    {
        cfa_err = cfa_free_cont(agg_cont->cfa_contids[g]);
        CFA_CHECK(cfa_err);    
    }

    /* free the path and the name */
    __free_str_via_pointer(&(agg_cont->path));
    __free_str_via_pointer(&(agg_cont->name));
    
    /* get the number of none freed array nodes */
    int n_conts = 0;
    cfa_err = get_array_length(&cfa_conts, &n_conts);
    CFA_CHECK(cfa_err);
    int nfc = 0;
    for (int i=0; i<n_conts; i++)
    {
        cfa_err = get_array_node(&cfa_conts, i, (void**)(&(agg_cont)));
        CFA_CHECK(cfa_err);
        /* path == NULL and name == NULL indicates node has been freed */
        if (agg_cont->path || agg_cont->name)
            nfc += 1;
    }
    /* if number of non-free containers is 0 then free the array */
    if (nfc == 0)
    {
        if (cfa_conts)
        {
            cfa_err = free_array(&cfa_conts);
            cfa_conts = NULL;
        }
    }
    CFA_CHECK(cfa_err);
    return CFA_NOERR;
}