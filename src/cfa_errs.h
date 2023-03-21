#ifndef __CFA_ERRS_H__
#define __CFA_ERRS_H__

/* ERROR constants */
/* These start at -500 so we can pass standard netCDF errors back when parsing 
the file */
#define CFA_NOERR                     (0) /* No error */
#define CFA_MEM_ERR                (-500) /* Cannot create memory */
#define CFA_MEM_LEAK               (-501) /* Debugging check for memory leak */
#define CFA_BOUNDS_ERR             (-502) /* Bounds error in dynamic array */
#define CFA_EOS                    (-503) /* End of string */
#define CFA_NAT_ERR                (-504) /* Not a type */
#define CFA_NOT_FOUND_ERR          (-510) /* Cannot find CFA Container */
#define CFA_DIM_NOT_FOUND_ERR      (-520) /* Cannot find CFA Dimension */
#define CFA_VAR_NOT_FOUND_ERR      (-530) /* Cannot find CFA Variable */
#define CFA_VAR_NO_AGG_INSTR       (-531) /* Aggregation instructions missing */
#define CFA_VAR_FRAGS_DEF          (-532) /* Fragments already defined */
#define CFA_VAR_FRAGS_UNDEF        (-533) /* Fragments not defined yet */
#define CFA_VAR_FRAG_DIM_NOT_FOUND (-534) /* Fragment dimension not found */
#define CFA_VAR_NO_FRAG            (-535) /* Fragment not defined */
#define CFA_VAR_NO_FRAG_INDEX      (-536) /* either the frag_location or data_location not set */
#define CFA_VAR_FRAGDAT_NOT_FOUND  (-537) /* The FragmentDatum could not be found */
#define CFA_UNKNOWN_FILE_FORMAT    (-540) /* Unsupported CFA file format */
#define CFA_NOT_CFA_FILE           (-541) /* Not a CFA file - does not contain relevant metadata */
#define CFA_UNSUPPORTED_VERSION    (-542) /* Unsupported version of CFA-netCDF */
#define CFA_NO_FILE                (-543) /* Output / Input file not created */
#define CFA_AGG_DATA_ERR           (-550) /* Something went wrong parsing the
"aggregated_data" attribute */
#define CFA_AGG_DIM_ERR            (-551) /* Something went wrong parsing the "aggregated_dimensions" attribute */
#define CFA_AGG_NOT_DEFINED        (-552) /* aggregation instructions have not been defined */
#define CFA_AGG_NOT_RECOGNISED     (-553) /* unrecognised aggregation instruction*/

#endif
