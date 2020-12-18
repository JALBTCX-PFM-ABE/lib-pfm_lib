
/*********************************************************************************************

    This is public domain software that was developed by IVS under a Cooperative Research
    And Development Agreement (CRADA) with the Naval Oceanographic Office (NAVOCEANO) and
    was contributed to the Pure File Magic (PFM) Application Programming Interface (API)
    library.  It was later modified by SAIC under a separate CRADA with NAVOCEANO.

*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! are being used by Doxygen to document the
    software.  Dashes in these comment blocks are used to create bullet lists.  The lack of
    blank lines after a block of dash preceeded comments means that the next block of dash
    preceeded comments is a new, indented bullet list.  I've tried to keep the Doxygen
    formatting to a minimum but there are some other items (like <br> and <pre>) that need
    to be left alone.  If you see a comment that starts with / * ! and there is something
    that looks a bit weird it is probably due to some arcane Doxygen syntax.  Be very
    careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



/*! <pre>
 *****************************************************************************
 ***   Interactive Visualizations Systems Inc.                             ***
 *****************************************************************************
 * LIBRARY        : part of pfm_lib                    )                     *
 * AUTHOR(S)      : Graeme Sweet (modified by J. Parker - SAIC)              *
 * FILE           : hyp.h (cloned from pcube.h)                              *
 * DATE CREATED   : Jan 13th, 2004 (modified on May 2006)                    *
 * PURPOSE        :                                                          *
 *  This library provides an interface to the hypothesis file of a PFM.      *
 *  A list of generated hypotheses is stored in a directory with extension   *
 *  ".hyp".                                                                  *
 *                                                                           *
 *****************************************************************************
 </pre>*/
#ifndef HYP_H
#define HYP_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "pfm.h"
#include "pfm_nvtypes.h"
#include "sys/param.h"

/*****************************************************************************
 ***
 *** Hypothesis Defines and Typedefs
 ***
 *****************************************************************************/

/*! Typedefs for progress callback */
typedef void (*HYP_PROGRESS_CALLBACK) (int state, int percent);

/*! Slash strings for unix and windows */
#ifdef NVWIN3X     /* Windows Operating System */
#define HYP_SLASH '\\'
#define HYP_SLASH_STR "\\"
#else            /* Unix operating system */
#define HYP_SLASH '/'
#define HYP_SLASH_STR "/"
#endif


/* Defines for the endian when writing binary data */
/* Updated to automatically detect whether the code is little/big endian. QPS.
 * Inspired by a web page by Jan Wolter on Byte Order in Unix. */
#ifdef BYTE_ORDER
/* Mac uses a no underscore, some one underscore, Linux two underscores. */
# if BYTE_ORDER == LITTLE_ENDIAN
#  define HYP_LITENDIAN
# else
#  if BYTE_ORDER == BIG_ENDIAN
#   define HYP_BIGENDIAN
#  else
#   error Error: unknown byte order!
#  endif
# endif
#elif defined(_BYTE_ORDER)
/* Some systems use a single underscore. */
# if _BYTE_ORDER == _LITTLE_ENDIAN
#  define HYP_LITENDIAN
# else
#  if _BYTE_ORDER == _BIG_ENDIAN
#   define HYP_BIGENDIAN
#  else
#   error Error: unknown byte order!
#  endif
# endif
#elif defined(__BYTE_ORDER)
/* Linux */
# if __BYTE_ORDER == __LITTLE_ENDIAN
#  define HYP_LITENDIAN
# else
#  if __BYTE_ORDER == __BIG_ENDIAN
#   define HYP_BIGENDIAN
#  else
#   error Error: unknown byte order!
#  endif
# endif
#elif  _WIN32
/* Windows is special. */
# define HYP_LITENDIAN
#else
# error The Endianness is undefined... fix me!
#endif

  /*
#ifdef HYP_LITENDIAN
#pragma message("Found little endian machine.")
#else
#pragma message("Found big endian machine.")
#endif
  */

/* Endian file types when writing binary data */
#define HYP_FTYPE_BIGENDIAN 0
#define HYP_FTYPE_LITENDIAN 1

/* Constants used for the library */
#define HYP_VERSION                 4.00f
#define HYP_ASCII_HEADER_SIZE       512
#define HYP_BINARY_HEADER_SIZE      512
#define HYP_NODE_RECORD_SIZE        16
#define HYP_HYPOTHESIS_RECORD_SIZE  24
#define HYP_DELETED_POINTER_POS    (HYP_ASCII_HEADER_SIZE + 4)
#define HYP_FILE_SIZE_POS          (HYP_ASCII_HEADER_SIZE + 4 + 8)
#define HYP_POINTER_POS            (HYP_HYPOTHESIS_RECORD_SIZE - 8)
#define HYP_CONF_99PC               2.56f /*!< Scale for 99% CI on Unit Normal */
#define HYP_CONF_95PC               1.96f /*!< Scale for 95% CI on Unit Normal */
#define HYP_CONF_90PC               1.69f /*!< Scale for 90% CI on Unit Normal */
#define HYP_CONF_68PC               1.00f /*!< Scale for 68% CI on Unit Normal */

#ifndef __CUBE_PRIVATE_H__
typedef enum {
        CUBE_PRIOR = 0,
        CUBE_LHOOD,
        CUBE_POSTERIOR,
        CUBE_PREDSURF,
        CUBE_UNKN
} CubeExtractor;

#ifndef __STDTYPES_H__
#ifndef False
typedef enum {
        False = 0,
        True
} Bool;
#endif
#endif

  /*!  Combined Bathymetric Uncertainty Estimator (CUBE).  */

typedef struct _cube_param {
        Bool                   init;           /*!< System mapsheet initialisation marker */
        float                  no_data_value;  /*!< Value used to indicate 'no data' (typ. FLT_MAX) */
        CubeExtractor          mthd;           /*!< Method used to extract information from sheet */
        double                 null_depth;     /*!< Depth to initialise estimates */
        double                 null_variance;  /*!< Variance for initialisation */
        double                 dist_exp;       /*!< Exponent on distance for variance scale */
        double                 inv_dist_exp;   /*!< 1.0/dist_exp for efficiency */
        double                 dist_scale;     /*!< Normalisation coefficient for distance (m) */
        double                 var_scale;      /*!< Variance scale dilution factor */
        double                 iho_fixed;      /*!< Fixed portion of IHO error budget (m^2) */
        double                 iho_pcent;      /*!< Variable portion of IHO error budget (unitless) */
        uint32_t               median_length;  /*!< Length of median pre-filter sort queue */
        float                  quotient_limit; /*!< Outlier quotient upper allowable limit */
        float                  discount;       /*!< Discount factor for evolution noise variance */
        float                  est_offset;     /*!< Threshold for significant offset from current
                                                    estimate to warrant an intervention */
        float                  bayes_fac_t;    /*!< Bayes factor threshold for either a single
                                                    estimate, or the worst-case recent sequence to
                                                    warrant an intervention */
        uint32_t               runlength_t;    /*!< Run-length threshold for worst-case recent
                                                    sequence to indicate a drift failure and hence
                                                    to warrant an intervention */
        float                  min_context;    /*!< Minimum context search range for hypothesis
                                                    disambiguation algorithm */
        float                  max_context;    /*!< Maximum context search range */
        float                  sd2conf_scale;  /*!< Scale from Std. Dev. to CI */
        float                  blunder_min;    /*!< Minimum depth difference from pred. depth to
                                                    consider a blunder (m) */
        float                  blunder_pcent;  /*!< Percentage of predicted depth to be considered
                                                    a blunder, if more than the minimum (0<p<1, typ. 0.25). */
        float                  blunder_scalar; /*!< Scale on initialisation surface std. dev. at a node
                                                    to allow before considering deep spikes to be blunders. */
        float                  capture_dist_scale;     /*!< Scale on predicted or estimated depth for how far out
                                                            to accept data.  (unitless; typically 0.05 for
                                                            hydrography but can be greater for geological mapping
                                                            in flat areas with sparse data) */
        float                  horiz_error_scale;        /* !< Added for v4.0 hyp file. Default is 2.95. QPS.
                                                            A value of 2.95 represents the 95% C.I.
                                                            This value is used during the 'scattering'
                                                            stage to scale the horizontal error of
                                                            each sounding when used in the radius of
                                                            influence computation. */

        int32_t                use_fixed_capture_radius; /* <! v4.0 hyp file.  Set to zero to disable.  QPS. */
        float                  fixed_capture_radius;     /* <! v4.0 hyp file.  Used if use_fixed_capture_radius is non-zero. QPS.
                                                               Only accept data with this radius. 0<=>infinity. */

} CubeParam /* , *Cube */;

typedef struct _queue {
        float          depth;
        float          var;
        float          avg_tpe;
} Queue;

typedef struct _depth_hypothesis
{
  double               cur_estimate;     /*!< Current depth mean estimate */
  double               cur_variance;     /*!< Current depth variance estimate */
  double               pred_estimate;    /*!< Current depth next-state mean prediction */
  double               pred_variance;    /*!< Current depth next-state variance pred. */
  double               cum_bayes_fac;    /*!< Cumulative Bayes factor for node monitoring */
  uint16_t             seq_length;       /*!< Worst-case sequence length for monitoring */
  uint16_t             hypothesis_num;   /*!< Index term for debuging */
  uint32_t             num_samples;      /*!< Number of samples incorporated into this node */
  float                var_estimate;     /*!< Running estimate of variance of inputs (m^2) */
  float                avg_tpe;          /*!< Average TPE as variance */
  float                shoal;            /*!< Shoalest of depth in hypothesis */
  struct _depth_hypothesis *next;        /*!< Singly Linked List */
} Hypothesis;

typedef struct _queue *pQueue;
typedef struct _depth_hypothesis *pHypothesis;

typedef struct _cube_node {
        pQueue          queue;          /*!< Queued points in pre-filter */
        uint32_t        n_queued;       /*!< Number of elements in queue structure */
        pHypothesis     depth;          /*!< Depth hypotheses currently being tracked */
        pHypothesis     nominated;      /*!< A nominated hypothesis from the user */
        float           pred_depth;     /*!< Predicted depth, or NaN for 'no update', or
                                             INVALID_DATA for 'no information available' */
        float           pred_var;       /*!< Variance of predicted depth, only valid if the
                                             predicted depth is (as above), meter^2 */
        Bool            write;          /*!< Debug write out */
} CNode /*, *CubeNode */;
#endif


/*****************************************************************************
 ***
 *** Hypothesis Structures
 ***
 *****************************************************************************/

/*! Structure for a hypothesis - each node may have many */
typedef struct _PHypothesis
{
    float       z;          /*!< Height associated with the hypothesis (m) */
    float       var;        /*!< Current variance for the node */
    int32_t     n_snds;     /*!< Number of soundings associated with the hypothesis */

    float       ci;         /*!< Confidence interval I associated with hypothesis (m) - calculated from var
                                 --- *which* CI depends on the parameter file, by default, 95% */
    float       avg_tpe;    /*!< Average vertical TPE for soundings in hypothesis */
    float       shoal;      /*!< Shoalest depth in hypothesis */
}
PHypothesis;

/*! Structure for each node in the sheet - each one correcponds to a bin */
typedef struct _HypNode
{
    /* Variables stored in the file */
    float         best_hypo;        /*!< Best hypothesis choosen by the algorithm */
    float         best_unct;        /*!< Uncertainty of best hypothesis */
    CNode        *node;             /*!< Pointer to a CubeNode structure - used in cube_node.c */
    BIN_RECORD    bin;              /*!< Store the PFM bin information associated with this node */

    /* Predicted surface - calculated when algorithm is PRED_SURF */
    float         pred_depth;       /*!< Predicted depth - calculate to be median of the depths */
    float         pred_var;         /*!< Predicted variance */

    /* Final array of hypotheses */
    int32_t       n_hypos;          /*!< Number of hypotheses in array */
    PHypothesis  *hypos;            /*!< Array of all hypotheses for the node */
}
HypNode;

/*! Main Hypothesis structure */
typedef struct _HypHead
{
    /* Main variables for the Hypothesis file */
    float       version;            /*!< Version of hyp file. */
    int32_t     pfmhnd;             /*!< Handle for the pfm file this hyp is associated with. */
    int32_t     ncols;              /*!< Number of rows and columns - need to make sure this lines */
    int32_t     nrows;              /*!<   up with the pfm. */
    int32_t     filehnd;            /*!< The file handle used for reading/writing. */
    uint8_t     readonly;           /*!< If true, file is opened in read-only mode. */
    uint8_t     endian;             /*!< 1 for little endian, 0 for big */
    int64_t     deleted_ptr;        /*!< Pointer to first deleted record on disk - records organized in a chain */
    int64_t     file_size;          /*!< Size of the file in bytes - make sure whole file exists */
    int32_t     warning;            /*!< Non-zero if a warning has occured */

    /* The following are indices to attributes stored within the PFM file. */
    int32_t     vert_error_attr;    /*!< Vertical error attribute. */
    int32_t     horiz_error_attr;   /*!< Horizontal error attribute. */
    int32_t     num_hypos_attr;     /*!< Number of hypotheses attribute. */
    int32_t     hypo_strength_attr; /*!< Hypothesis strength attribute. */
    int32_t     uncertainty_attr;   /*!< Hypothesis uncertainty attribute. */
    int32_t     custom_hypo_flag;   /*!< Flag for marking a sounding as a custom hypothesis */

    /* User controllable parameters for the cube algorithm */
    /*  use hyp_get_parameters and hyp_set_parameters to modify */
    int32_t     hypo_resolve_algo;    /*!< Which hypothesis resolution algorithm to use - see HYP_RESOLVE_ */
    float       node_capture_percent; /*!< Only soundings less than this percent distance of the depth can
                                           away from a node center can contribute to the node. */

    /* Variables from the pfm */
    BIN_HEADER  bin_header;         /*!< The bin header from the PFM */

    /*! Parameters for running the cube - from cube.c */
    CubeParam  *cube_param;
}
HypHead;

/*****************************************************************************
 ***
 *** Hypothesis Public API functions
 ***
 *****************************************************************************/

/*! Returns a pointer to a Cube Node at the given row and column position. The caller
 * is responsible for freeing the object using 'hyp_free_node'.
 */
HypNode*   hyp_get_node( HypHead *p, int32_t x, int32_t y );

/*! Frees a node returned by hyp_get_node.
 */
void         hyp_free_node( HypNode *node );

/*! Sets a progress callback used when creating or recubeing an area.
 */
void         hyp_register_progress_callback( HYP_PROGRESS_CALLBACK progressCB );

/*! Returns a string describing the last error.
 */
char*        hyp_get_last_error();

int32_t      hyp_init_hypothesis (int hnd, HypHead *hyp, int nw, BIN_HEADER *bin_header);

HypHead*     hyp_open_file( char *list_file_path, int32_t pfmhnd );

HypHead*     hyp_create_file( char *list_file_path, int32_t pfmhnd,
                              int32_t horizErrorAttr, int32_t vertErrorAttr,
                              int32_t numHyposAttr, int32_t hypoStrengthAttr,
                              int32_t uncertaintyAttr, int32_t customHypoFlag );

void         hyp_close_file( HypHead *p );

int32_t      hyp_write_header( HypHead *p );
int32_t      hyp_read_header( HypHead *p );
int32_t      hyp_read_node( HypHead *p, int32_t x, int32_t y, HypNode *node );
int32_t      hyp_write_node( HypHead *p, int32_t x, int32_t y, HypNode *node );

#ifdef  __cplusplus
}
#endif


#endif
