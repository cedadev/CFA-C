#include <stdio.h>
#include <string.h>

#include "cfa.h"

extern const char* get_type_name(const cfa_type);

/*
Output the information for an AggregationVariable
*/
int cfa_info_var(const int cfa_id, const int cfa_varid, 
                 const int level, const int indent)
{
    AggregationVariable *agg_var;
    AggregatedDimension *agg_dim;
    int cfa_err = cfa_get_var(cfa_id, cfa_varid, &agg_var);
    CFA_CHECK(cfa_err);

    const char* typename = get_type_name(agg_var->cfa_dtype.type);
    printf("%*s%-18s%-24s: (", indent, "", typename, agg_var->name);
    /* print the attached dimensions */
    for (int d=0; d<agg_var->cfa_ndim; d++)
    {
        cfa_err = cfa_get_dim(cfa_id, agg_var->cfa_dim_idp[d], &agg_dim);
        CFA_CHECK(cfa_err);
        printf("%s", agg_dim->name);
        if (d<agg_var->cfa_ndim-1)
            printf(", ");
    }
    printf(")\n");
    /* print the aggregation instructions */
    for (int i=0; i<agg_var->n_instr; i++)
    {
        AggregationInstruction *pinst = &(agg_var->cfa_instr[i]);
        printf("%*s%-18s%-24s: %s\n", indent, "", "", 
               pinst->term, pinst->value);
    }
    return CFA_NOERR;
}

/*
Output the information for an AggregatedDimension
*/
int cfa_info_dim(const int cfa_id, const int cfa_dimid, 
                 const int level, const int indent)
{
    AggregatedDimension *agg_dim;
    int cfa_err = cfa_get_dim(cfa_id, cfa_dimid, &agg_dim);
    CFA_CHECK(cfa_err);
    const char* typename = get_type_name(agg_dim->type.type);
    printf("%*s%-18s%-24s: (%4i)\n", indent, "", typename, 
           agg_dim->name, agg_dim->length);
    return CFA_NOERR;
}

/*
Output the information for an AggregationContainer - either a file or a group
for netCDF defined Aggregations
*/
int cfa_info_cont(const int cfa_id, const int level, const int indent)
{
    AggregationContainer *agg_cont;
    int cfa_err = cfa_get(cfa_id, &agg_cont);
    CFA_CHECK(cfa_err);
    if(agg_cont->path)
        printf("%*sCFA: %s\n", indent, "", agg_cont->path);
    if(agg_cont->name)
        printf("%*s%s\n", indent, "", agg_cont->name);
    printf(
"%*s========================================================================\n", 
    indent, "");

    /* print the dimensions - if there are any */
    if (agg_cont->n_dims)
    {
        printf("%*sDimensions: \n", indent, "");
        for (int d=0; d<agg_cont->n_dims; d++)
        {
            cfa_err = cfa_info_dim(cfa_id, agg_cont->cfa_dimids[d], 
                               level, indent+4);
            CFA_CHECK(cfa_err);
        }
        printf(
"%*s------------------------------------------------------------------------\n", 
        indent, "");
    }
    
    /* print the variables - if there are any */
    if (agg_cont->n_vars)
    {
        printf("%*sVariables: \n", indent, "");
        for (int v=0; v<agg_cont->n_vars; v++)
        {
            cfa_err = cfa_info_var(cfa_id, agg_cont->cfa_varids[v], 
                                   level, indent+4);
            CFA_CHECK(cfa_err);
        }
        printf(
"%*s------------------------------------------------------------------------\n", 
        indent, "");
    }

    /* print the containers - if there are any */
    if (agg_cont->n_conts)
    {
        printf("%*sContainers: \n", indent, "");
        for (int c=0; c<agg_cont->n_conts; c++)
        {
            cfa_err = cfa_info_cont(agg_cont->cfa_contids[c],
                                    level, indent+4);
            CFA_CHECK(cfa_err);
        }
        printf(
"%*s------------------------------------------------------------------------\n", 
        indent, "");
    }
    return CFA_NOERR;
}

int
cfa_info(const int cfa_id, const int level)
{
    int cfa_err = cfa_info_cont(cfa_id, level, 0);
    return cfa_err;
}