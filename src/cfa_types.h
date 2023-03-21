/* Type constants - these match the netCDF ones, except for strings
   Currently we are only supporting numeric types
*/
#define CFA_NAT          (0)      /**< Not A Type */
#define CFA_BYTE         (1)      /**< signed 1 byte integer */
#define CFA_CHAR         (2)      /**< ISO/ASCII character */
#define CFA_SHORT        (3)      /**< signed 2 byte integer */
#define CFA_INT          (4)      /**< signed 4 byte integer */
#define CFA_LONG         (CFA_INT)
#define CFA_FLOAT        (5)      /**< single precision floating point number */
#define CFA_DOUBLE       (6)      /**< double precision floating point number */
#define CFA_UBYTE        (7)      /**< unsigned 1 byte int */
#define CFA_USHORT       (8)      /**< unsigned 2-byte int */
#define CFA_UINT         (9)      /**< unsigned 4-byte int */
#define CFA_INT64        (10)     /**< signed 8-byte int */
#define CFA_UINT64       (11)     /**< unsigned 8-byte int */
#define CFA_STRING       (12)     /**array of char, or however netCDF interprets it*/
