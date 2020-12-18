
/*********************************************************************************************

    This is public domain software that was developed by the U.S. Naval Oceanographic Office.

    This is a work of the US Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the US Government.

    Neither the United States Government nor any employees of the United States Government,
    makes any warranty, express or implied, without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.

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




/************************* Fun disclaimer ***********************************

    NOTE: No warranties, either express or implied, are hereby given. All
    software is supplied as is, without guarantee.  The user assumes all
    responsibility for damages resulting from the use of these features,
    including, but not limited to, frustration, disgust, system abends, disk
    head-crashes, general malfeasance, floods, fires, shark attack, nerve
    gas, locust infestation, cyclones, hurricanes, tsunamis, local
    electromagnetic disruptions, hydraulic brake system failure, invasion,
    hashing collisions, normal wear and tear of friction surfaces, cosmic
    radiation, inadvertent destruction of sensitive electronic components,
    windstorms, the Riders of Nazgul, infuriated chickens, malfunctioning
    mechanical or electrical sexual devices, premature activation of the
    distant early warning system, peasant uprisings, halitosis, artillery
    bombardment, explosions, cave-ins, and/or frogs falling from the sky.

****************************************************************************/

#define  __PFM_IO__

#include <stdio.h>
#include <stdlib.h>
/*#include <inttypes.h>*/
#include <fcntl.h>

#ifdef NVWIN3X
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "pfm_nvtypes.h"
#include "pfm.h"
#include "pfm_header.h"
#include "pfm_version.h"
#include "pfm_extras.h"

#include "pfmP.h"
#include "large_io.h"


#if (defined _WIN32) && (defined _MSC_VER)
#undef fseeko64
#undef ftello64
#undef fopen64
#define fseeko64(x, y, z) _fseeki64((x), (y), (z))
#define ftello64(x)       _ftelli64((x))
#define fopen64(x, y)      fopen((x), (y))
#elif defined(__APPLE__)
#define fseeko64(x, y, z) fseek((x), (y), (z))
#define ftello64(x)       ftell((x))
#define fopen64(x, y)     fopen((x), (y))
#define ftruncate64(x, y) ftruncate((x), (y))
#endif

/***************************************************************************\
 *
 *  Include the bit_pack functions so they can be statically inlined.
 *
\***************************************************************************/

#include "bit_pack.c"



#define BIN_HEADER_SIZE         131072                   /*!<  Default BIN file header size.  WARNING - Never make this smaller!  */
#define MAX_SUB_PATHS           30                       /*!<  Maximum number of substitute file paths allowed in the .pfm_cfg file  */


#define PFM_A0                  6378137.0                /*!<  Semi-major axis  */
#define PFM_B0                  6356752.314245           /*!<  Semi-minor axis  */

#ifndef NINT
#define NINT(a)                 ((a)<0.0 ? (int32_t) ((a) - 0.5) : (int32_t) ((a) + 0.5))  /*!<  Nearest integer macro  */
#endif


/*!  If MAX_PFM_FILES changes from 150, please revisit this.  */

static int32_t                  pfm_hnd[MAX_PFM_FILES] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
							  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
							  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
							  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
							  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
							  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
							  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int32_t                  bin_file_hnd[MAX_PFM_FILES];
static int32_t                  ndx_file_hnd[MAX_PFM_FILES];
static FILE                     *list_file_fp[MAX_PFM_FILES];
static FILE                     *line_file_fp[MAX_PFM_FILES];

static int64_t                  previous_bin_address[MAX_PFM_FILES];
static int64_t                  previous_depth_block[MAX_PFM_FILES];
static int32_t                  depth_record_pos[MAX_PFM_FILES];
static BIN_HEADER               bin_header[MAX_PFM_FILES];
static char                     bin_header_block[MAX_PFM_FILES][BIN_HEADER_SIZE];
static uint8_t                  compute_average[MAX_PFM_FILES];
static uint8_t                  use_chain_pointer[MAX_PFM_FILES];
static BIN_RECORD               bin_record[MAX_PFM_FILES];
static int64_t                  bin_record_address[MAX_PFM_FILES];
static uint8_t                  *bin_record_data[MAX_PFM_FILES];
static int64_t                  bin_record_head_pointer[MAX_PFM_FILES];
static int64_t                  bin_record_tail_pointer[MAX_PFM_FILES];
static uint8_t                  bin_record_modified[MAX_PFM_FILES];
static int64_t                  depth_record_address[MAX_PFM_FILES];
static uint8_t                  *depth_record_data[MAX_PFM_FILES];
static uint8_t                  *zero_depth_record_data[MAX_PFM_FILES];
static uint8_t                  depth_record_modified[MAX_PFM_FILES];
static int64_t                  continuation_pointer[MAX_PFM_FILES];
static int16_t                  list_file_ver[MAX_PFM_FILES];
static uint32_t                 list_file_count[MAX_PFM_FILES];
static uint32_t                 list_file_index[MAX_PFM_FILES][PFM_MAX_FILES];
static uint16_t                 list_file_seq[MAX_PFM_FILES][PFM_MAX_FILES];
static char                     list_file_del_flag[MAX_PFM_FILES][PFM_MAX_FILES];

static uint32_t                 line_file_count[MAX_PFM_FILES];
static uint32_t                 line_file_index[MAX_PFM_FILES][PFM_MAX_FILES];


static uint8_t                  list_dir[MAX_PFM_FILES];
static char                     list_path[MAX_PFM_FILES][512];
static char                     substitute_path[MAX_PFM_FILES][MAX_SUB_PATHS][3][512];
static int16_t                  substitute_cnt[MAX_PFM_FILES];
static uint8_t                  screwup[MAX_PFM_FILES];
static uint8_t                  no_check = NVFalse;


static FILE                     *cov_file_fp[MAX_PFM_FILES];


/*  These are used in read_cov_map_index.  */

static int32_t                  read_cov_map_index_prev_row[MAX_PFM_FILES];
static int32_t                  read_cov_map_index_prev_num_cols[MAX_PFM_FILES];
static uint8_t                  *read_cov_map_index_row[MAX_PFM_FILES];
static BIN_RECORD               *read_cov_map_index_bin_row[MAX_PFM_FILES];


/*  This is used in read_line_file.  */

static char                     read_line_file_string[MAX_PFM_FILES][512];


/*  pfm_geo_distance data.  */

static double                   *geo_distance[MAX_PFM_FILES];
static double                   *geo_post[MAX_PFM_FILES];
static uint8_t                  geo_dist_init[MAX_PFM_FILES];



/*  PFM error status            */

static char                     pfm_err_str[2048];


static BIN_RECORD_OFFSETS       bin_off[MAX_PFM_FILES];



/*!  Depth record size and offsets.  */

typedef struct
{
  uint32_t                      record_size;             /*!<  Record size in bytes  */
  uint32_t                      single_point_bits;       /*!<  Bits per single sounding record.  */
  uint32_t                      file_number_pos;         /*!<  Bit position within a depth record buffer of the file number  */
  uint32_t                      line_number_pos;         /*!<  Bit position within a depth record buffer of the line number  */
  uint32_t                      ping_number_pos;         /*!<  Bit position within a depth record buffer of the ping/record number  */
  uint32_t                      beam_number_pos;         /*!<  Bit position within a depth record buffer of the beam/subrecord number  */
  uint32_t                      depth_pos;               /*!<  Bit position within a depth record buffer of the depth  */
  uint32_t                      x_offset_pos;            /*!<  Bit position within a depth record buffer of the X offset  */
  uint32_t                      y_offset_pos;            /*!<  Bit position within a depth record buffer of the Y offset  */
  uint32_t                      validity_pos;            /*!<  Bit position within a depth record buffer of the validity  */
  uint32_t                      attr_pos[NUM_ATTR];      /*!<  Bit position within a depth record buffer of the NDX attributes  */
  uint32_t                      horizontal_error_pos;    /*!<  Bit position within a depth record buffer of the horizontal uncertainty  */
  uint32_t                      vertical_error_pos;      /*!<  Bit position within a depth record buffer of the vertical uncertainty  */
  uint32_t                      continuation_pointer_pos;/*!<  Bit position within a depth record buffer of the continuation pointer  */
} DEPTH_RECORD_OFFSETS;

static DEPTH_RECORD_OFFSETS     dep_off[MAX_PFM_FILES];


/*  Set up for multithreading.  */

#define PFM_MUTEXES             MAX_PFM_FILES / 2        /*!<  Maximum number of mutexes for multithreading  */

#ifdef NVWIN3X
static HANDLE                   pfm_write_lock[PFM_MUTEXES];
#else
static pthread_mutex_t          pfm_write_lock[PFM_MUTEXES];
#endif
static uint8_t                  threading = NVFalse;
static int32_t                  mutex[MAX_PFM_FILES];    /*!<  Per handle mutex number  */
static int32_t                  num_mutexes = 0;


/*!  Structures containing all of the data in the bin file headers.  */

static BIN_HEADER_DATA          hd[MAX_PFM_FILES];

static float                    x_offset_scale[MAX_PFM_FILES];
static float                    y_offset_scale[MAX_PFM_FILES];
static uint32_t                 count_size[MAX_PFM_FILES];



void pfm_newgp (double, double, double, double, double *, double *);
void pfm_invgp (double, double, double, double, double, double, double *, double *);


static  PFM_PROGRESS_CALLBACK  pfm_progress_callback = NULL;


/***************************************************************************/
/*!

  - Module Name:        pfm_register_progress_callback

  - Programmer(s):      Graeme Sweet

  - Date Written:       September 2002

  - Purpose:            This function is used to register a callback function
                        to keep track of the percentage complete when
                        initializing the PFM bin file and when initializing
                        the coverage map.  If you do not call this function
                        to set your handler the percentages will be printed
                        to stderr.

  - Arguments:
                        - progressCB      =   The function that will handle the
                                              percentage information

  - Return Value:
                        - 0               =   success
                        - -1              =   failure, see errno for reason

  - Caveats:            The callback function that you specify must have two
                        arguments.  Both arguments are int32_t values.  The
                        function should look like this:
                        <br>
                        <br>
                        <pre>
                        static void progressCB (int32_t state, int32_t percent)
                          {
                            Do something with the state and percentage
                          }
                        </pre>
                        <br>
                        <br>
                        The <b>state</b> value is 1 to indicate that the API
                        is initializing the bin file, 2 to indicate that it
                        is initializing the coverage map, or 3 to indicate
                        that it is making a checkpoint file.  The
                        <b>percentage</b> value is the percentage complete.

****************************************************************************/

void pfm_register_progress_callback (PFM_PROGRESS_CALLBACK progressCB)
{
  pfm_progress_callback = progressCB;
}


/***************************************************************************/
/*!

  - Module Name:        pfm_substitute

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 2003

  - Purpose:            Substitutes for paths that are specified in the
                        .pfm_cfg file as [SUBSTITUTE PATH] arguments.

  - Caveats:            The [SUBSTITUTE PATH] statement will contain two or
                        three path names separated by commas.  The first
                        field is a Windows folder name such as X:\\data1 or
                        even just X:.  The second and third fields will be
                        UNIX directory names (the third field is not
                        required).  These fields are used to allow
                        transparent file access on heterogeneous networks
                        (those containing both UNIX and Windows systems).
                        When a PFM structure is opened the .pfm_cfg file is
                        read in.  The file may be located, in order, in the
                        current working directory/folder, the user's home
                        directory, or anywhere in the PATH (first come
                        first served).  [SUBSTITUTE PATH] statements are
                        read at that time.  If there are any
                        [SUBSTITUTE PATH] statements then all file names
                        subsequently read from the list file (e.g. bin,
                        index, mosaic, target, input) will be compared
                        against these statements.  If there is a match then
                        the paths will be replaced with the matching path
                        from the statement.  A normal [SUBSTITUTE PATH]
                        statement might look like this:
                        <br>
                        <br>
                        [SUBSTITUTE PATH] = X:,/net/alh-pogo1/data3/datasets,/data3/datasets
                        <br>
                        <br>
                        On UNIX systems, the first of the two UNIX fields
                        will have precedence over the second.  For this
                        reason it is best to place a networked path name
                        (name containing /net or /.automount) first since
                        it will usually contain the entirety of the
                        non-networked path name.  In addition, the
                        networked name will work on all systems, even, in
                        this case, alh-pogo1.  Obviously, if you place them
                        the other way 'round, you could end up with some
                        very interesting path names ;-)  Make sure that you
                        don't have conflicting substitute paths in the
                        .pfm_cfg file.  The first match will take
                        precedence.

  - Arguments:
                        - hnd             =   PFM handle
                        - path            =   path to be mangled

  - Return Value:
                        - void

****************************************************************************/

void pfm_substitute (int32_t hnd, char *path)
{
  char           string[1024];
  int16_t        i, j;

#ifndef UNIX
  int16_t        k;
  uint8_t        hit;
#endif


  for (i = 0 ; i < substitute_cnt[hnd] ; i++)
    {


#ifdef UNIX

      /*  Check for the Windows path (for example X:)  */

      if (strstr (path, substitute_path[hnd][i][0]))
        {
          /*  Always use the first UNIX path (which should be the networked path).  */

          strcpy (string, substitute_path[hnd][i][1]);

          j = strlen (substitute_path[hnd][i][0]);
          strcat (string, &path[j]);
          strcpy (path, string);

          for (j = 0 ; j < strlen (path) ; j++)
            {
              if (path[j] == '\\') path[j] = '/';
            }

          break;
        }


#else


      /*  Set the hit flag so we can break out if we do the substitution.  */

      hit = NVFalse;


      for (k = 1 ; k < 3 ; k++)
        {
          /*  Make sure that we had two UNIX substitutes, otherwise it will substitute for a blank string (this only works when k is 2).  */

          if (substitute_path[hnd][i][k][0] && strstr (path, substitute_path[hnd][i][k]))
            {
              strcpy (string, substitute_path[hnd][i][0]);

              j = strlen (substitute_path[hnd][i][k]);
              strcat (string, &path[j]);
              strcpy (path, string);

              for (j = 0 ; j < strlen (path) ; j++)
                {
                  if (path[j] == '/') path[j] = '\\';
                }

              hit = NVTrue;

              break;
            }
        }


      /*  Break out if we did a substitution.  */

      if (hit) break;


#endif
    }
}


/***************************************************************************/
/*!

  - Module Name:        get_version

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Returns the software version string.

  - Arguments:
                        - version         =   Version information

  - Return Value:
                        - void

****************************************************************************/

void get_version (char *version)
{
  strcpy (version, VERSION);
}



/***************************************************************************/
/*!

  - Module Name:        pfm_get_version

  - Programmer(s):      Jan C. Depner

  - Date Written:       January 2012

  - Purpose:            Returns the software major and minor version numbers.

  - Arguments:
                        - major         =   Major version number
                        - minor         =   Minor version number

  - Return Value:
                        - void

****************************************************************************/

void pfm_get_version (int32_t *major, int32_t *minor)
{
  double temp;

  sscanf (strstr (VERSION, "library V"), "library V%lf", &temp);
  *major = (int32_t) temp;
  *minor = NINT (fmod (temp, 1.0));
}



/***************************************************************************/
/*!

  - Module Name:        pfm_thread_init

  - Programmer(s):      Jan C. Depner

  - Date Written:       January 04, 2012

  - Purpose:            This function sets a flag (threading) that indicates that
                        mutexes may be required for append operations (i.e. calls
                        to add_depth_record or add_cached_depth_record).  This
                        only has an effect if a single PFM is opened with multiple
                        handles and an append operation is done.  The mutexes will
                        be created if there are identical PFM file names opened.
                        Even then, the mutex will only be exercised if an append
                        operation is attempted.

  - Arguments:          NONE

  - Return Value:
                        - SUCCESS
                        - MUTEX_UNAVAILABLE_ERROR
                        - MUTEX_RELEASE_ERROR

  - Caveats:            Don't ever call this function if you aren't multithreading
                        your application.  Also, note that there are only
                        MAX_PFM_FILES / 2 possible mutexes.  This is because it
                        would make no sense to have a mutex for each of
                        MAX_PFM_FILES.  You'll run out of possible open PFM
                        files before you'll run out of mutexes.

****************************************************************************/

int32_t pfm_thread_init ()
{
  int32_t i, j;

  for (i = 0 ; i < PFM_MUTEXES ; i++)
    {
      mutex[i] = -1;

#ifdef NVWIN3X
      pfm_write_lock[i] = NULL;
#endif
    }

  threading = NVTrue;


  /*  Since we're multithreading the application check to see if we have single PFMs opened with multiple handles.
      In order to multithread your application you should have already opened your PFM(s) with multiple handles.  */

  for (i = 0 ; i < MAX_PFM_FILES ; i++)
    {

      /*  Check for opened PFM files.  */

      if (pfm_hnd[i] != -1)
        {

          /*  Now beat all of the other opened PFMs against this one to look for a match.  */

          for (j = 0 ; j < MAX_PFM_FILES ; j++)
            {

              /*  If there is no file for this handle or we already have a mutex, skip this.  */

              if (pfm_hnd[j] != -1 && mutex[j] == -1)
                {

                  /*  If the file is the same as the one we're looking for... */

                  if (!strcmp (list_path[j], list_path[i]))
                    {

                      /*  If the file already had a mutex assigned (there's already more than one handle for this file),
                          set the current mutex to the previous mutex.  */

                      if (mutex[i] != -1)
                        {
                          mutex[j] = mutex[i];
                        }


                      /*  We have a match but no mutexes have been created so we need to create a single mutex for
                          both handles.  */

                      else
                        {
#ifdef NVWIN3X
                          if ((pfm_write_lock[num_mutexes] = CreateMutex (NULL, TRUE, NULL)) == NULL)
                            {
                              sprintf (pfm_err_str, "Unable to create mutex for threading");
                              return (pfm_error = MUTEX_UNAVAILABLE_ERROR);
                            }

                          if (!ReleaseMutex (pfm_write_lock[num_mutexes]))
                            {
                              CloseHandle (pfm_write_lock[num_mutexes]);
                              pfm_write_lock[num_mutexes] = NULL;
                              sprintf (pfm_err_str, "Unable to release mutex for threading");
                              return (pfm_error = MUTEX_RELEASE_ERROR);
                            }
#else
                          if (pthread_mutex_init (&pfm_write_lock[num_mutexes], NULL))
                            {
                              sprintf (pfm_err_str, "Unable to create mutex for threading");
                              return (pfm_error = MUTEX_UNAVAILABLE_ERROR);
                            }
#endif
                          mutex[i] = mutex[j] = num_mutexes;
                          num_mutexes++;
                        }
                    }
                }
            }
        }
    }


  /*  Issue a warning if no single PFM has been opened multiple times.  */

  if (!num_mutexes)
    {
      sprintf (pfm_err_str, "No single PFM opened multiple times.  Mutexes not needed.");
      return (pfm_error = MUTEX_NOT_NEEDED_ERROR);
    }


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        pfm_thread_destroy

  - Programmer(s):      Jan C. Depner

  - Date Written:       January 04, 2012

  - Purpose:            This function destroys the mutexes created in
                        open_pfm_file and unsets the boolean that was
                        set in pfm_thread_init.

  - Arguments:          NONE

  - Return Value:
                        - SUCCESS
                        - MUTEX_RELEASE_ERROR

****************************************************************************/

int32_t pfm_thread_destroy ()
{
  int32_t i;

  for (i = 0 ; i < num_mutexes ; i++)
    {
#ifdef NVWIN3X
      if (!CloseHandle (pfm_write_lock[i]))
        {
          sprintf (pfm_err_str, "Unable to release mutex for threading");
          return (pfm_error = MUTEX_RELEASE_ERROR);
        }

      pfm_write_lock[i] = NULL;
#else
      if (pthread_mutex_destroy (&pfm_write_lock[i]))
        {
          sprintf (pfm_err_str, "Unable to release mutex for threading");
          return (pfm_error = MUTEX_RELEASE_ERROR);
        }
#endif
    }

  threading = NVFalse;

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        open_cov_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2015

  - Purpose:            Opens already existing coverage file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin_path        =   Pathname of bin file

  - Return Value:
                        - SUCCESS
                        - OPEN_COV_FILE_OPEN_ERROR
                        - OPEN_COV_FILE_READ_VERSION_ERROR
                        - OPEN_COV_FILE_NEWER_VERSION_ERROR

****************************************************************************/

static int32_t open_cov_file (int32_t hnd, char *bin_path)
{
  char                file[1024], cov_path[1024];
  int16_t             soft_v, cov_file_ver;
  float               temp;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Create the coverage file path name from the bin file name.  This is hard-wired to be the same as the bin file name with a .cvg extension.  */

  strcpy (cov_path, bin_path);
  sprintf (&cov_path[strlen (cov_path) - 4], ".cvg");

  if ((cov_file_fp[hnd] = fopen (cov_path, "rb+")) == NULL)
    {
      if ((cov_file_fp[hnd] = fopen (cov_path, "rb")) == NULL)
        {
          sprintf (pfm_err_str, "Error opening %s", cov_path);
          return (pfm_error = OPEN_COV_FILE_OPEN_ERROR);
        }
      fprintf (stderr, "Coverage file %s is opened read-only.\n", cov_path);
    }


  /*  Read the PFM Software version number from the first line of the coverage file.  If it is corrupted, error out.  */

  if (pfm_ngets (file, 512, cov_file_fp[hnd]) == NULL)
    {
      sprintf (pfm_err_str, "Error reading version number from coverage file %s", cov_path);
      return (pfm_error = OPEN_COV_FILE_READ_VERSION_ERROR);
    }

  if (strstr (file, "Software - PFM I/O library V") == NULL)
    {
      sprintf (pfm_err_str, "Version string in coverage file %s in error", cov_path);
      return (pfm_error = OPEN_COV_FILE_READ_VERSION_ERROR);
    }

  sscanf (strstr (file, "library V"), "library V%f", &temp);
  cov_file_ver = temp * 10.0 + 0.05;

  sscanf (strstr (VERSION, "library V"), "library V%f", &temp);
  soft_v = temp * 10.0 + 0.05;


  /*  Check for the file version newer than the software version.  */

  if (cov_file_ver > soft_v)
    {
      sprintf (pfm_err_str, "File %s version %.1f newer than software version %.1f", cov_path, (float) cov_file_ver / 10.0,
               (float) soft_v / 10.0);
      return (pfm_error = OPEN_COV_FILE_NEWER_VERSION_ERROR);
    }


  pfm_error = SUCCESS;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error);
}


/***************************************************************************/
/*!

  - Module Name:        create_cov_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2015

  - Purpose:            Creates and opens a new coverage file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin_path        =   Pathname of bin file

  - Return Value:
                        - SUCCESS
                        - CREATE_COV_FILE_FILE_EXISTS
                        - CREATE_COV_FILE_OPEN_ERROR

****************************************************************************/

static int32_t create_cov_file (int32_t hnd, char *bin_path)
{
  int32_t             status, size, i, percent = 0, old_percent = -1;
  uint8_t             *cov, *hblock;
  char                cov_path[1024];


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Create the coverage file path name from the bin file name.  This is hard-wired to be the same as the bin file name with a .cvg extension.  */

  strcpy (cov_path, bin_path);
  sprintf (&cov_path[strlen (cov_path) - 4], ".cvg");


  /*  If the coverage file exists, error out.  */

  if ((cov_file_fp[hnd] = fopen64 (cov_path, "rb")) != NULL)
    {
      sprintf (pfm_err_str, "Trying to create coverage file %s, already exists", cov_path);
      fclose (cov_file_fp[hnd]);
      return (pfm_error = CREATE_COV_FILE_FILE_EXISTS);
    }


  if ((cov_file_fp[hnd] = fopen64 (cov_path, "wb")) == NULL)
    {
      sprintf (pfm_err_str, "Unable to open coverage file %s in create_cov_file", cov_path);
      return (pfm_error = CREATE_COV_FILE_OPEN_ERROR);
    }


  /*  Write the PFM Software version to the first line of the coverage file.  */

  fprintf (cov_file_fp[hnd], "%s\n", VERSION);


  /*  Write the end of header sentinel to the coverage file.  */

  fprintf (cov_file_fp[hnd], "[END OF HEADER]\n");


  /*  Zero out the rest of the header.  */

  size = COV_HEADER_SIZE - (int32_t) ftello64 (cov_file_fp[hnd]);
  hblock = (uint8_t *) calloc (size, 1);
  fwrite (hblock, size, 1, cov_file_fp[hnd]);
  free (hblock);


  /*  Zero out the coverage map data area.  */

  cov = (uint8_t *) calloc (bin_header[hnd].bin_width, 1);

  for (i = 0 ; i < bin_header[hnd].bin_height ; i++)
    {
      fwrite (cov, bin_header[hnd].bin_width, 1, cov_file_fp[hnd]);

      percent = ((float) i / bin_header[hnd].bin_height) * 100.0;
      if (percent != old_percent)
        {
          /*  Call the callback if it is registered.  */

          if (pfm_progress_callback)
            {
              (*pfm_progress_callback) (2, percent);
            }
          else
            {
              fprintf (stderr, "Initializing the coverage map : %03d%% processed\r", percent);
            }

          old_percent = percent;
        }
    }

  free (cov);


  if (!pfm_progress_callback) fprintf (stderr, "Initializing the coverage map : 100%% processed\n\n");


  fclose (cov_file_fp[hnd]);


  status = open_cov_file (hnd, bin_path);
  if (status) return (pfm_error = status);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        open_list_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Opens already existing file list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - path            =   Pathname of list file
                        - bin_path        =   Pathname of bin file
                        - index_path      =   Pathname base (no extension) of
                                              index files
                        - image_path      =   Pathname of image file
                        - target_path     =   Pathname of target file

  - Return Value:
                        - SUCCESS
                        - OPEN_LIST_FILE_OPEN_ERROR
                        - OPEN_LIST_FILE_READ_VERSION_ERROR
                        - OPEN_LIST_FILE_NEWER_VERSION_ERROR
                        - OPEN_LIST_FILE_READ_BIN_ERROR
                        - OPEN_LIST_FILE_READ_INDEX_ERROR
                        - OPEN_LIST_FILE_READ_IMAGE_ERROR
                        - OPEN_LIST_FILE_READ_TARGET_ERROR
                        - OPEN_LIST_FILE_CORRUPTED_FILE_ERROR
                        - CHECK_INPUT_FILE_OPEN_ERROR
                        - CHECK_INPUT_FILE_WRITE_ERROR

****************************************************************************/

static int32_t open_list_file (int32_t hnd, char *path, char *bin_path, char *index_path, char *image_path, char *target_path)
{
  char                file[512], string[1024];
  FILE                *fp;
  int16_t             soft_v;
  float               temp;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  screwup[hnd] = NVFalse;


  if ((list_file_fp[hnd] = fopen (path, "rb+")) == NULL)
    {
      if ((list_file_fp[hnd] = fopen (path, "rb")) == NULL)
        {
          sprintf (pfm_err_str, "Error opening %s", path);
          return (pfm_error = OPEN_LIST_FILE_OPEN_ERROR);
        }
      fprintf (stderr, "List file %s is opened read-only.\n", path);
    }

  list_file_count[hnd] = 0;


  /*  Read the PFM Software version number from the first line of the list file.  If it is corrupted, error out.  */

  if (pfm_ngets (file, 512, list_file_fp[hnd]) == NULL)
    {
      sprintf (pfm_err_str, "Error reading version number from list file %s", path);
      return (pfm_error = OPEN_LIST_FILE_READ_VERSION_ERROR);
    }

  if (strstr (file, "Software - PFM I/O library V") == NULL)
    {
      sprintf (pfm_err_str, "Version string in list file %s in error", path);
      return (pfm_error = OPEN_LIST_FILE_READ_VERSION_ERROR);
    }

  sscanf (strstr (file, "library V"), "library V%f", &temp);
  list_file_ver[hnd] = temp * 10.0 + 0.05;


  sscanf (strstr (VERSION, "library V"), "library V%f", &temp);
  soft_v = temp * 10.0 +0.05;


  /*  Check for the file version newer than the software version.  */

  if (list_file_ver[hnd] > soft_v)
    {
      sprintf (pfm_err_str, "File %s version %.1f newer than software version %.1f", path, (float) list_file_ver[hnd] / 10.0,
               (float) soft_v / 10.0);
      return (pfm_error = OPEN_LIST_FILE_NEWER_VERSION_ERROR);
    }


  /*  Read the bin file path. */

  if (pfm_ngets (string, 512, list_file_fp[hnd]) == NULL)
    {
      sprintf (pfm_err_str, "Error reading bin file name from list file %s", path);
      return (pfm_error = OPEN_LIST_FILE_READ_BIN_ERROR);
    }


  /*  If we've input a directory instead of a list file path we don't want to read the bin path from the file because we
      might have moved the entire directory.  */

  if (!list_dir[hnd]) strcpy (bin_path, string);


  /*  Check for substitute paths if we have defined any.  These are defined in the .pfm_cfg file (see cfg_in and pfm_substitute).  An example would be:

      [SUBSTITUTE PATH]=N:,/net/alh-pogo1/data1,/data1

      On UNIX, any path that starts with N: will have /net/alh-pogo1/data1 substituted
      the first UNIX path has precedence).  On Windows, any path with /net/alh-pogo1/data1
      or /data1 will have N: substituted.
  */

  if (substitute_cnt[hnd]) pfm_substitute (hnd, bin_path);


  /*  Read the index file path. */

  if (pfm_ngets (string, 512, list_file_fp[hnd]) == NULL)
    {
      sprintf (pfm_err_str, "Error reading index file name from list file %s", path);
      return (pfm_error = OPEN_LIST_FILE_READ_INDEX_ERROR);
    }


  /*  If we've input a directory instead of a list file path we don't want to read the index path from the file because we
      might have moved the entire directory.  */

  if (!list_dir[hnd]) strcpy (index_path, string);


  if (substitute_cnt[hnd]) pfm_substitute (hnd, index_path);


  /*  Read the image file path. */

  if (pfm_ngets (image_path, 512, list_file_fp[hnd]) == NULL)
    {
      sprintf (pfm_err_str, "Error reading mosaic file name from list file %s", path);
      return (pfm_error = OPEN_LIST_FILE_READ_IMAGE_ERROR);
    }


  if (substitute_cnt[hnd]) pfm_substitute (hnd, image_path);


  /*  Read the target file path. */

  if (pfm_ngets (target_path, 512, list_file_fp[hnd]) == NULL)
    {
      sprintf (pfm_err_str, "Error reading target file name from list file %s", path);
      return (pfm_error = OPEN_LIST_FILE_READ_TARGET_ERROR);
    }


  if (substitute_cnt[hnd]) pfm_substitute (hnd, target_path);


  pfm_error = SUCCESS;


  /*  Read the input file names and save pointers to them in the list file.  Leave the file open for further additions or reads. */

  list_file_index[hnd][list_file_count[hnd]] = ftell (list_file_fp[hnd]);
  while (pfm_ngets (string, sizeof (file), list_file_fp[hnd]) != NULL)
    {
      /*  Check for pre 6.0 major file version (no longer supported).  */

      if (list_file_ver[hnd] < 60)
        {
          sprintf (pfm_err_str, "Versions prior to 6.0 are no longer supported.\nThe file %s is version %.1f", path,
                   (float) list_file_ver[hnd] / 10.0);

          return (pfm_error = PFM_UNSUPPORTED_VERSION_ERROR);
        }
      else
        {
          sscanf (string, "%1c %hd %s", &list_file_del_flag[hnd][list_file_count[hnd]], &list_file_seq[hnd][list_file_count[hnd]], file);


          /*  Fix my PFM/ACME screwup in 3.1  */

          if (string[1] != ' ')
            {
              screwup[hnd] = NVTrue;
              strcpy (file, string);
            }
          else
            {
              if (list_file_seq[hnd][list_file_count[hnd]] != list_file_count[hnd])
                {
                  sprintf (pfm_err_str,
                           "Mismatch between file count %d and file sequence number %d in PFM list file.\nPFM list file %s has been corrupted.\n",
                           list_file_count[hnd], list_file_seq[hnd][list_file_count[hnd]], path);
                  return (pfm_error = OPEN_LIST_FILE_CORRUPTED_FILE_ERROR);
                }
            }
        }


      list_file_count[hnd]++;
      list_file_index[hnd][list_file_count[hnd]] = ftell (list_file_fp[hnd]);


      if (substitute_cnt[hnd]) pfm_substitute (hnd, file);


      /*  Try to open the input file.  */

      if ((fp = fopen (file, "r+")) == NULL)
        {
          pfm_error = CHECK_INPUT_FILE_WRITE_ERROR;
          if ((fp = fopen (file, "r")) == NULL)
            {
              pfm_error = CHECK_INPUT_FILE_OPEN_ERROR;
            }
        }
      else
        {
          fclose (fp);
        }
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error);
}


/***************************************************************************/
/*!

  - Module Name:        create_list_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Creates and opens a new list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - list_path       =   Pathname of list file
                        - bin_path        =   Pathname of bin file
                        - index_path      =   Pathname of index file
                        - image_path      =   Pathname of image file
                        - target_path     =   Pathname of target file

  - Return Value:
                        - SUCCESS
                        - CREATE_LIST_FILE_FILE_EXISTS
                        - CREATE_LIST_FILE_OPEN_ERROR
                        - OPEN_LIST_FILE_OPEN_ERROR
                        - OPEN_LIST_FILE_READ_VERSION_ERROR
                        - OPEN_LIST_FILE_READ_BIN_ERROR
                        - OPEN_LIST_FILE_READ_INDEX_ERROR
                        - OPEN_LIST_FILE_READ_IMAGE_ERROR
                        - OPEN_LIST_FILE_READ_TARGET_ERROR
                        - OPEN_LIST_FILE_CORRUPTED_FILE_ERROR

****************************************************************************/

static int32_t create_list_file (int32_t hnd, char *list_path, char *bin_path, char *index_path, char *image_path, char *target_path)
{
  int32_t             status;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  If the list file exists, error out.  */

  if ((list_file_fp[hnd] = fopen (list_path, "r")) != NULL)
    {
      sprintf (pfm_err_str, "Trying to create list file %s, already exists", list_path);
      fclose (list_file_fp[hnd]);
      return (pfm_error = CREATE_LIST_FILE_FILE_EXISTS);
    }


  if ((list_file_fp[hnd] = fopen (list_path, "w")) == NULL)
    {
      sprintf (pfm_err_str, "Unable to open list file %s in create_list_file", list_path);
      return (pfm_error = CREATE_LIST_FILE_OPEN_ERROR);
    }


  /*  Write the PFM Software version to the first line of the list file.  */

  fprintf (list_file_fp[hnd], "%s\n", VERSION);


  /*  Write the pre-defined file paths out to the list file.  */

  fprintf (list_file_fp[hnd], "%s\n", bin_path);
  fprintf (list_file_fp[hnd], "%s\n", index_path);
  fprintf (list_file_fp[hnd], "%s\n", image_path);
  fprintf (list_file_fp[hnd], "%s\n", target_path);


  fclose (list_file_fp[hnd]);


  status = open_list_file (hnd, list_path, bin_path, index_path, image_path, target_path);

  if (status) return (pfm_error = status);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}


/***************************************************************************/
/*!

  - Module Name:        set_offsets

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Sets the bin and depth record sizes and offsets.

  - Arguments:
                        - hnd  =  PFM file handle

  - Return Value:
                        - void

****************************************************************************/

static int32_t set_offsets (int32_t hnd)
{
  int32_t             i, num_bits;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Set the bin record size and offsets.    */

  bin_off[hnd].num_soundings_pos = 0;
  bin_off[hnd].std_pos = hd[hnd].count_bits;
  bin_off[hnd].avg_filtered_depth_pos = bin_off[hnd].std_pos + hd[hnd].std_bits;
  bin_off[hnd].min_filtered_depth_pos = bin_off[hnd].avg_filtered_depth_pos + hd[hnd].depth_bits;
  bin_off[hnd].max_filtered_depth_pos = bin_off[hnd].min_filtered_depth_pos + hd[hnd].depth_bits;
  bin_off[hnd].avg_depth_pos = bin_off[hnd].max_filtered_depth_pos + hd[hnd].depth_bits;
  bin_off[hnd].min_depth_pos = bin_off[hnd].avg_depth_pos + hd[hnd].depth_bits;
  bin_off[hnd].max_depth_pos = bin_off[hnd].min_depth_pos + hd[hnd].depth_bits;


  /*  Pre v7.0 version dependency  */

  if (list_file_ver[hnd] < 70)
    {
      bin_off[hnd].attr_pos[0] = bin_off[hnd].max_depth_pos + hd[hnd].depth_bits;
    }
  else
    {
      bin_off[hnd].num_valid_pos = bin_off[hnd].max_depth_pos + hd[hnd].depth_bits;
      bin_off[hnd].attr_pos[0] = bin_off[hnd].num_valid_pos + hd[hnd].count_bits;
    }


  for (i = 1 ; i < NUM_ATTR ; i++) bin_off[hnd].attr_pos[i] = bin_off[hnd].attr_pos[i - 1] + hd[hnd].bin_attr_bits[i - 1];

  bin_off[hnd].validity_pos = bin_off[hnd].attr_pos[NUM_ATTR - 1] + hd[hnd].bin_attr_bits[NUM_ATTR - 1];
  bin_off[hnd].head_pointer_pos = bin_off[hnd].validity_pos + hd[hnd].validity_bits;

  bin_off[hnd].tail_pointer_pos = bin_off[hnd].head_pointer_pos + hd[hnd].record_pointer_bits;

  num_bits = bin_off[hnd].tail_pointer_pos + hd[hnd].record_pointer_bits;

  bin_off[hnd].record_size = num_bits / 8;


  /*  Set the size to the next larger whole byte if it wasn't an even multiple of 8.   */

  if (num_bits % 8) bin_off[hnd].record_size++;

  /*  Allocate the memory for bin record I/O. */

  if (bin_record_data[hnd] == NULL)
    {
      if ((bin_record_data[hnd] = (uint8_t *) malloc (bin_off[hnd].record_size)) == NULL)
        {
          sprintf (pfm_err_str, "Unable to allocate memory for bin record");
          return (pfm_error = SET_OFFSETS_BIN_MALLOC_ERROR);
        }
    }


  /*  Set the depth record size and offsets.  */

  dep_off[hnd].file_number_pos = 0;
  dep_off[hnd].line_number_pos = hd[hnd].file_number_bits;
  dep_off[hnd].ping_number_pos = dep_off[hnd].line_number_pos + hd[hnd].line_number_bits;
  dep_off[hnd].beam_number_pos = dep_off[hnd].ping_number_pos + hd[hnd].ping_number_bits;
  dep_off[hnd].depth_pos = dep_off[hnd].beam_number_pos + hd[hnd].beam_number_bits;
  dep_off[hnd].x_offset_pos = dep_off[hnd].depth_pos + hd[hnd].depth_bits;
  dep_off[hnd].y_offset_pos = dep_off[hnd].x_offset_pos + hd[hnd].offset_bits;


  dep_off[hnd].attr_pos[0] = dep_off[hnd].y_offset_pos + hd[hnd].offset_bits;

  for (i = 1 ; i < NUM_ATTR ; i++) dep_off[hnd].attr_pos[i] = dep_off[hnd].attr_pos[i - 1] + hd[hnd].ndx_attr_bits[i - 1];

  dep_off[hnd].validity_pos = dep_off[hnd].attr_pos[NUM_ATTR - 1] + hd[hnd].ndx_attr_bits[NUM_ATTR - 1];

  dep_off[hnd].horizontal_error_pos = dep_off[hnd].validity_pos + hd[hnd].validity_bits;
  dep_off[hnd].vertical_error_pos = dep_off[hnd].horizontal_error_pos + hd[hnd].horizontal_error_bits;


  /*  Bits per single sounding record.  */

  dep_off[hnd].single_point_bits = dep_off[hnd].vertical_error_pos + hd[hnd].vertical_error_bits;


  dep_off[hnd].continuation_pointer_pos = dep_off[hnd].single_point_bits * hd[hnd].record_length;

  num_bits = dep_off[hnd].continuation_pointer_pos + hd[hnd].record_pointer_bits;

  dep_off[hnd].record_size = num_bits / 8;


  /*  Set the size to the next larger whole byte if it wasn't an even multiple of 8.   */

  if (num_bits % 8) dep_off[hnd].record_size++;


  /*  Compute the largest number of soundings per bin.  */

  count_size[hnd] = NINT (pow (2.0, (double) hd[hnd].count_bits));


  /*  Allocate the memory for depth record I/O.   */

  if (depth_record_data[hnd] == NULL)
    {
      if ((depth_record_data[hnd] = (uint8_t *) calloc (1, dep_off[hnd].record_size)) == NULL)
        {
          sprintf (pfm_err_str, "Unable to allocate memory for depth record");
          return (pfm_error = SET_OFFSETS_DEPTH_MALLOC_ERROR);
        }
    }


  /*  Allocate the memory for the zero'ed out depth record for use as an end of file placeholder.  This will be written after we get the
      continuation pointer (at the end of file) so that we can use a mutex to make sure we don't give two records the same continuation pointer
      if we're multithreading.   */

  if (zero_depth_record_data[hnd] == NULL)
    {
      if ((zero_depth_record_data[hnd] = (uint8_t *) calloc (1, dep_off[hnd].record_size)) == NULL)
        {
          sprintf (pfm_err_str, "Unable to allocate memory for zero'ed depth record");
          return (pfm_error = SET_OFFSETS_DEPTH_MALLOC_ERROR);
        }
    }

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}


/***************************************************************************/
/*!

  - Module Name:        get_string

  - Programmer(s):      Jan C. Depner

  - Date Written:       December 1994

  - Purpose:            Parses the input string for the equals sign and
                        returns everything to the right.

  - Arguments:
                        - *in     =   Input string
                        - *out    =   Output string

  - Return Value:
                        - void

\***************************************************************************/

static void get_string (char *in, char *out)
{
  int32_t             i, start, length;
  char                *ptr;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif


  start = 0;
  length = 0;


  ptr = strchr (in, '=') + 1;


  /*  Search for first non-blank character.   */

  for (i = 0 ; i < strlen (ptr) ; i++)
    {
      if (ptr[i] != ' ')
        {
          start = i;
          break;
        }
    }


  /*  Search for last non-blank character.    */

  for (i = strlen (ptr) ; i >= 0 ; i--)
    {
      if (ptr[i] != ' ' && ptr[i] != 0)
        {
          length = (i + 1) - start;
          break;
        }
    }

  strncpy (out, &ptr[start], length);
  out[length] = 0;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif
}


/***************************************************************************/
/*!

  - Module Name:        read_bin_header

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Reads the bin file header.

  - Arguments:
                        - hnd             =   PFM file handle
                        - head            =   BIN_HEADER structure

  - Return Value:
                        - SUCCESS

****************************************************************************/

int32_t read_bin_header (int32_t hnd, BIN_HEADER *head)
{
  int32_t             i, key_count, offset, index, temp, status;
  char                varin[1024], info[1024];
  double              az;
  NV_F64_COORD2       central;
  NV_I32_COORD2       coord;
  uint32_t            utemp;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  /*  Compute the number of KEY phrases.  To understand (yeah, right) how the KEY phrases work and are set up take a look at pfm_header.h.
      Hopefully pfm_header.h makes it easy to add fields to the PFM header ;-)  */

  key_count = sizeof (keys) / sizeof (KEY);


  /*  Zero out the header structure (defined in pfm_header.h).  */

  memset (&header_data.head, 0, sizeof (BIN_HEADER));


  /*  Zero out the polygon point count.    */

  header_data.head.polygon_count = 0;


  for (i = 0 ; i < NUM_ATTR ; i++)
    {
      header_data.bin_attr_bits[i] = 0;
      header_data.ndx_attr_bits[i] = 0;
    }


  /*  This is needed for pre-6.0 files that didn't have the header size in the header.  */

  header_data.bin_header_size = 16384;


  /*  Read the header.  We read the default size even though it may be a smaller one from an earlier version.  It really doesn't
      matter since we're going to fseek to the end of the actual header when we're done parsing it.  */

  lfseek (bin_file_hnd[hnd], 0, SEEK_SET);

  if (!(i = lfread (bin_header_block[hnd], BIN_HEADER_SIZE, 1, bin_file_hnd[hnd])))
    {
      sprintf (pfm_err_str, "Error reading header record");
      return (pfm_error = OPEN_BIN_OPEN_ERROR);
    }

  index = 0;

  while (bin_header_block[hnd][index] != 0)
    {
      for (i = 0 ; i < 1024 ; i++)
        {
          if (bin_header_block[hnd][index] == '\n') break;
          varin[i] = bin_header_block[hnd][index];
          index++;
        }

      varin[i] = 0;


      /*  Put everything to the right of the equals sign in 'info'.   */

      if (strchr (varin, '=') != NULL) strcpy (info, (strchr (varin, '=') + 1));

      for (i = 0 ; i < key_count ; i++)
        {
          /*  Set the array offset for repeating fields.  */

          if (keys[i].count == NULL)
            {
              offset = 0;
            }
          else
            {
              offset = *keys[i].count;
            }


          /*  Check input for matching strings and load values if found.  */

          if (strstr (varin, keys[i].keyphrase) != NULL)
            {
              if (!strcmp (keys[i].datatype, "string"))
                {
                  get_string (varin, keys[i].address.c);
                }
              else if (!strcmp (keys[i].datatype, "i16"))
                {
                  sscanf (info, "%hd", keys[i].address.i16 + offset);
                }
              else if (!strcmp (keys[i].datatype, "i32"))
                {
                  sscanf (info, "%d", keys[i].address.i32 + offset);
                }
              else if (!strcmp (keys[i].datatype, "uc"))
                {
                  sscanf (info, "%u", &utemp);
                  *(keys[i].address.uc + offset) = utemp;
                }
              else if (!strcmp (keys[i].datatype, "ui16"))
                {
                  sscanf (info, "%hd", keys[i].address.ui16 + offset);
                }
              else if (!strcmp (keys[i].datatype, "ui32"))
                {
                  sscanf (info, "%d", keys[i].address.ui32 + offset);
                }
              else if (!strcmp (keys[i].datatype, "f32"))
                {
                  sscanf (info, "%f", keys[i].address.f32 + offset);
                }
              else if (!strcmp (keys[i].datatype, "f64"))
                {
                  sscanf (info, "%lf", (keys[i].address.f64 + offset));
                }
              else if (!strncmp (keys[i].datatype, "f64p", 4))
                {
                  sscanf (info, "%lf", (keys[i].address.f64 + offset));
                }
              else if (!strcmp (keys[i].datatype, "f64c2"))
                {
                  sscanf (info, "%lf,%lf", &central.y, &central.x);
                  *(keys[i].address.f64c2 + offset) = central;
                }
              else if (!strcmp (keys[i].datatype, "i32c2"))
                {
                  sscanf (info, "%d,%d", &coord.x, &coord.y);
                  *(keys[i].address.i32c2 + offset) = coord;
                }
              else if (!strcmp (keys[i].datatype, "b"))
                {
                  sscanf (info, "%d", &temp);
                  *(keys[i].address.b + offset) = temp;
                }
              else if (!strcmp (keys[i].datatype, "i64"))
                {
                  sscanf (info, "%"PRId64, (keys[i].address.i64 + offset));
                }


              /*  Increment the repeating field counter.  */

              if (keys[i].count != NULL) (*keys[i].count)++;
            }
        }


      /*  Skip the carriage return.  */

      index++;
    }


  /* if bin_attr_offset and bin_attr_max were not found, set them to min_bin_attr and max_bin_attr. */

  for (i = 0 ; i < header_data.head.num_bin_attr ; i++)
    {
      if ((header_data.head.bin_attr_scale[i] > 0.000001) && (header_data.bin_attr_offset[i] < 0.000001) && (header_data.bin_attr_max[i] < 0.000001))
        {
          /*  Pre 6.3 version dependency.  */
          /*  This is the fix for the broken bin attributes.  */

          if (list_file_ver[hnd] < 63)
            {
              header_data.bin_attr_offset[i] = header_data.head.min_bin_attr[i];
            }
          else
            {
              header_data.bin_attr_offset[i] = -header_data.head.min_bin_attr[i];
            }

          header_data.bin_attr_max[i] = header_data.head.max_bin_attr[i];
        }
    }


  /*  Set anything over header_data.head.num_bin_attr to 0.0.  */

  for (i = header_data.head.num_bin_attr ; i < NUM_ATTR ; i++)
    {
      header_data.head.min_bin_attr[i] = 999999.0;
      header_data.head.max_bin_attr[i] = -999999.0;
      header_data.head.bin_attr_scale[i] = 0.0;
      header_data.bin_attr_offset[i] = 0.0;
      header_data.bin_attr_max[i] = 0.0;
    }


  if (!(strlen (header_data.head.user_flag_name[0]))) strcpy (header_data.head.user_flag_name[0], "PFM_USER_01");
  if (!(strlen (header_data.head.user_flag_name[1]))) strcpy (header_data.head.user_flag_name[1], "PFM_USER_02");
  if (!(strlen (header_data.head.user_flag_name[2]))) strcpy (header_data.head.user_flag_name[2], "PFM_USER_03");
  if (!(strlen (header_data.head.user_flag_name[3]))) strcpy (header_data.head.user_flag_name[3], "PFM_USER_04");
  if (!(strlen (header_data.head.user_flag_name[4]))) strcpy (header_data.head.user_flag_name[4], "PFM_USER_05");
  if (!(strlen (header_data.head.user_flag_name[5]))) strcpy (header_data.head.user_flag_name[5], "PFM_USER_06");
  if (!(strlen (header_data.head.user_flag_name[6]))) strcpy (header_data.head.user_flag_name[6], "PFM_USER_07");
  if (!(strlen (header_data.head.user_flag_name[7]))) strcpy (header_data.head.user_flag_name[7], "PFM_USER_08");
  if (!(strlen (header_data.head.user_flag_name[8]))) strcpy (header_data.head.user_flag_name[8], "PFM_USER_09");
  if (!(strlen (header_data.head.user_flag_name[9]))) strcpy (header_data.head.user_flag_name[9], "PFM_USER_10");


  /*  Check to see if we want to compute averages here.  */

  if (!(strlen (header_data.head.average_filt_name))) strcpy (header_data.head.average_filt_name, "Average Filtered Depth");


  /*  Supporting "Average Edited" as a standard name for this field in addition to "Average Filtered Depth".  */

  if (!strcmp (header_data.head.average_filt_name, "Average Filtered Depth") || !strcmp (header_data.head.average_filt_name, "Average Edited"))
    {
      compute_average[hnd] = NVTrue;
    }
  else
    {
      compute_average[hnd] = NVFalse;
    }
  if (!(strlen (header_data.head.average_name))) strcpy (header_data.head.average_name, "Average Depth");


  /*  Move the data from the temp BIN_HEADER_DATA structure.  */

  hd[hnd] = header_data;
  *head = hd[hnd].head;


  central.y = head->mbr.min_y + (head->mbr.max_y - head->mbr.min_y) * 0.5;
  central.x = head->mbr.min_x + (head->mbr.max_x - head->mbr.min_x) * 0.5;


  /*  Compute the offset scales.  These are based on the number of bits used for the offset values.  */

  x_offset_scale[hnd] = (double) (NINT (pow (2.0, (double) hd[hnd].offset_bits)) - 1);
  y_offset_scale[hnd] = (double) (NINT (pow (2.0, (double) hd[hnd].offset_bits)) - 1);


  /*  Compute the bin size in meters (lat/lon only).   */

  if (!head->proj_data.projection) pfm_invgp (PFM_A0, PFM_B0, central.y, central.x, central.y + head->y_bin_size_degrees, central.x, &head->bin_size_xy, &az);


  bin_header[hnd] = *head;


  /*  Set the max file, line, ping, and beam numbers based on the number of bits used to store these fields.  */

  hd[hnd].head.max_input_files = head->max_input_files = NINT (pow (2.0, (double) hd[hnd].file_number_bits)) - 1;
  hd[hnd].head.max_input_lines = head->max_input_lines = NINT (pow (2.0, (double) hd[hnd].line_number_bits)) - 1;
  hd[hnd].head.max_input_pings = head->max_input_pings = NINT (pow (2.0, (double) hd[hnd].ping_number_bits)) - 1;
  hd[hnd].head.max_input_beams = head->max_input_beams = NINT (pow (2.0, (double) hd[hnd].beam_number_bits)) - 1;


  /*  Set the bin record size and offsets.    */

  status = set_offsets (hnd);
  if (status) return (pfm_error = status);


  if (hd[hnd].horizontal_error_bits) hd[hnd].horizontal_error_null = pow (2.0, (double) hd[hnd].horizontal_error_bits) - 1.0;
  if (hd[hnd].vertical_error_bits) hd[hnd].vertical_error_null = pow (2.0, (double) hd[hnd].vertical_error_bits) - 1.0;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        write_bin_header

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes the bin header.

  - Arguments:
                        - hnd             =   PFM file handle
                        - head            =   BIN_HEADER structure
                        - init            =   Initialize file flag

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_HEADER_EXCEEDED_MAX_POLY

****************************************************************************/

int32_t write_bin_header (int32_t hnd, BIN_HEADER *head, uint8_t init)
{
  int32_t             i, j, percent, old_percent, key_count, loop, status, tmp;
  int64_t             init_block_size;
  uint8_t             *init_block;
  char                *ptr, format[10];
  uint8_t             *cov;
  NV_F64_COORD2       xy, central;
  NV_I32_COORD2       coord;
  float               temp;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Make sure that the pad data is zeroed so that future versions won't pick up garbage by mistake.  */

  memset (head->pad, 0, sizeof (head->pad));


  /*  Check to see if we need to initialize the header.  */

  if (init)
    {
      if (head->polygon_count > MAX_POLY)
        {
          sprintf (pfm_err_str, "Exceeded max polygon point count %d", MAX_POLY);
          return (pfm_error = WRITE_BIN_HEADER_EXCEEDED_MAX_POLY);
        }


      /*  Compute the central x and y based on the input area size (which will not be the final area size).    */

      central.y = head->mbr.min_y + (head->mbr.max_y - head->mbr.min_y) * 0.5;
      central.x = head->mbr.min_x + (head->mbr.max_x - head->mbr.min_x) * 0.5;


      /*  If the data is to be stored as lat/lon...  */

      if (!head->proj_data.projection)
        {
          /*  If we already set the bin sizes in degrees, use them to define the bin sizes, otherwise we'll compute the
              bin sizes from the size in meters at the center of the area.  */

          if (head->bin_size_xy != 0.0)
            {
              /*  Convert from meters.    */

              pfm_newgp (central.y, central.x, 90.0, head->bin_size_xy, &xy.y, &xy.x);


              /* Danny Neville: Add a check if the longitude is in the form 0 to 360 */

              if (central.x > 180) xy.x = xy.x + 360;


              head->x_bin_size_degrees = xy.x - central.x;
              pfm_newgp (central.y, central.x, 0.0, head->bin_size_xy, &xy.y, &xy.x);
              head->y_bin_size_degrees = xy.y - central.y;
            }


          /*  Compute the new north lat and east lon based on the bin sizes and the original north lat and east lon.    */

          head->bin_height = ((head->mbr.max_y - head->mbr.min_y) / head->y_bin_size_degrees) + 1;
          head->mbr.max_y = head->mbr.min_y + (head->bin_height * head->y_bin_size_degrees);


          head->bin_width = ((head->mbr.max_x - head->mbr.min_x) / head->x_bin_size_degrees) + 1;
          head->mbr.max_x = head->mbr.min_x + (head->bin_width * head->x_bin_size_degrees);
        }
      else
        {
          head->bin_width = ((head->mbr.max_x - head->mbr.min_x) / head->bin_size_xy) + 1;
          head->mbr.max_x = head->mbr.min_x + (head->bin_width * head->bin_size_xy);

          head->bin_height = ((head->mbr.max_y - head->mbr.min_y) / head->bin_size_xy) + 1;
          head->mbr.max_y = head->mbr.min_y + (head->bin_height * head->bin_size_xy);
        }


      /* Added safety check - DN */

      if ( head->bin_width <= 0 || head->bin_height <= 0 )
        {
          sprintf (pfm_err_str, "Bin width or Bin height is zero or negative" );
          return (pfm_error = WRITE_BIN_HEADER_NEGATIVE_BIN_DIMENSION);
        }

      hd[hnd].head.num_bin_attr = 0;
      for (i = 0 ; i < NUM_ATTR ; i++)
        {
          if (hd[hnd].bin_attr_bits[i]) hd[hnd].head.num_bin_attr++;
        }

      hd[hnd].head.num_ndx_attr = 0;
      for (i = 0 ; i < NUM_ATTR ; i++)
        {
          if (hd[hnd].ndx_attr_bits[i]) hd[hnd].head.num_ndx_attr++;
        }


      /*  Set the default header size since this is a new bin file.  */

      hd[hnd].bin_header_size = BIN_HEADER_SIZE;


      /*  Now seek to that position.  */

      lfseek (bin_file_hnd[hnd], hd[hnd].bin_header_size, SEEK_SET);
    }


  /*  If no attribute data is available, set the name.  */

  for (i = 0 ; i < NUM_ATTR ; i++)
    {
      if (hd[hnd].bin_attr_bits[i] == 0) strcpy (hd[hnd].head.bin_attr_name[i], "UNAVAILABLE");
      if (hd[hnd].ndx_attr_bits[i] == 0) strcpy (hd[hnd].head.ndx_attr_name[i], "UNAVAILABLE");
    }


  /*  Check the average surface name to see if we will be recomputing the average surface.  */

  if (!(strlen (head->average_filt_name))) strcpy (head->average_filt_name, "Average Filtered Depth");


  /*  Now supporting "Average Edited" in addition to "Average Filtered Depth".  */

  if ((!strcmp (head->average_filt_name, "Average Filtered Depth")) || (!strcmp (head->average_filt_name, "Average Edited")))
    {
      compute_average[hnd] = NVTrue;
    }
  else
    {
      compute_average[hnd] = NVFalse;
    }
  if (!(strlen (head->average_name))) strcpy (head->average_name, "Average Depth");


  /*  Move the data into the temp BIN_HEADER_DATA structure.  */

  header_data = hd[hnd];
  header_data.head = *head;


  bin_header[hnd] = *head;


  /*  Check to see if we need to initialize the file.  */

  if (init)
    {
      /*  Compute the bin record size to the nearest byte.    */

      status = set_offsets (hnd);
      if (status) return (pfm_error = status);


      /*  Zero out the record.  */

      memset (bin_record_data[hnd], 0, bin_off[hnd].record_size);


      /*  Compute the offset scales.  These are based on the number of bits used for the offset values.  */

      x_offset_scale[hnd] = (double) (NINT (pow (2.0, (double) hd[hnd].offset_bits)) - 1);
      y_offset_scale[hnd] = (double) (NINT (pow (2.0, (double) hd[hnd].offset_bits)) - 1);


      /*  Initialize the remainder of the file.    */

      percent = 0;
      old_percent = -1;
      if (!pfm_progress_callback) fprintf (stderr, "\n");


      /*  New initialization code.  JCD  */

      init_block_size = bin_header[hnd].bin_width * bin_off[hnd].record_size;
      init_block = (uint8_t *) calloc (1, init_block_size);
      if (init_block == NULL)
        {
          perror ("Allocating init block in write_bin_header");
          exit (-1);
        }

      for (i = 0 ; i < bin_header[hnd].bin_height ; i++)
        {
          lfwrite (init_block, init_block_size, 1, bin_file_hnd[hnd]);

          percent = ((float) i / bin_header[hnd].bin_height) * 100.0;
          if (percent != old_percent)
            {
              /*  GS: now calls a callback if it is registered.  */

              if (pfm_progress_callback)
                {
                  (*pfm_progress_callback) (1, percent);
                }
              else
                {
                  fprintf (stderr, "Initializing the bin file : %03d%% processed\r", percent);
                }
              old_percent = percent;
            }
        }

      free (init_block);


      /*  This is the old initialization code.  The above code is much faster.

      for (i = 0 ; i < bin_header[hnd].bin_height ; i++)
        {
          for (j = 0 ; j < bin_header[hnd].bin_width ; j++)
            {
              lfwrite (bin_record_data[hnd], bin_off[hnd].record_size, 1, bin_file_hnd[hnd]);
            }

          percent = ((float) i / bin_header[hnd].bin_height) * 100.0;
          if (percent != old_percent)
          {
              //  GS: now calls a callback if it is registered.  //

              if (pfm_progress_callback)
                {
                  (*pfm_progress_callback) (1, percent);
                }
              else
                {
                  fprintf (stderr, "Initializing the bin file : %03d%% processed\r", percent);
                }
              old_percent = percent;
          }
      }

      */


      if (!pfm_progress_callback) fprintf (stderr, "Initializing the bin file : 100%% processed\n\n");


      /*  Pre v7.0 version dependency  */

      if (list_file_ver[hnd] < 70)
        {
          /*  Compute the location of the coverage map.  */


          /*
           *   Variable casts to clear up a corruption problem with very large PFM areas.
           *   Fri Aug 13 04:34:01 2004 -- Webb McDonald (SAIC)
           */

          hd[hnd].coverage_map_address = (int64_t) ((int64_t) hd[hnd].bin_header_size +
                                                    (int64_t) (bin_header[hnd].bin_height * bin_header[hnd].bin_width) *
                                                    (int64_t) bin_off[hnd].record_size);

          header_data.coverage_map_address = hd[hnd].coverage_map_address;

          lfseek (bin_file_hnd[hnd], hd[hnd].coverage_map_address, SEEK_SET);


          /*  Zero out the coverage map area.  */

          cov = (uint8_t *) calloc (bin_header[hnd].bin_width, 1);

          for (i = 0 ; i < bin_header[hnd].bin_height ; i++)
            {
              lfwrite (cov, bin_header[hnd].bin_width, 1, bin_file_hnd[hnd]);

              percent = ((float) i / bin_header[hnd].bin_height) * 100.0;
              if (percent != old_percent)
                {
                  /*  GS: now calls a callback if it is registered.  */

                  if (pfm_progress_callback)
                    {
                      (*pfm_progress_callback) (2, percent);
                    }
                  else
                    {
                      fprintf (stderr, "Initializing the coverage map : %03d%% processed\r", percent);
                    }

                  old_percent = percent;
                }
            }

          free (cov);

          if (!pfm_progress_callback) fprintf (stderr, "Initializing the coverage map : 100%% processed\n\n");
        }
      else
        {
          /*  This is hard-wired to zero to denote an external coverage map (v7 and greater).  */

          header_data.coverage_map_address = 0;
        }


      /*  Set the version so we can check the header output values for version dependency.  This is a new file so the version
          is not set yet.  */

      sscanf (strstr (VERSION, "library V"), "library V%f", &temp);
      list_file_ver[hnd] = temp * 10.0 + 0.05;
    }


  /*  Compute the number of KEY phrases.  To understand (yeah, right) how the KEY phrases work and are set up take a look at pfm_header.h.
      Hopefully pfm_header.h makes it easy to add fields to the PFM header ;-)  */

  key_count = sizeof (keys) / sizeof (KEY);


  ptr = bin_header_block[hnd];
  memset (bin_header_block[hnd], 0, hd[hnd].bin_header_size);


  /*  Write each entry.    */

  for (i = 0 ; i < key_count ; i++)
    {
      /*  Check for version dependencies.  We don't want to write old style header info to a new file.  Since format changes are only done
          as major version releases we can check the major (integer) portion for version dependency.  */

      if (!keys[i].version_dependency || (list_file_ver[hnd] <= keys[i].version_dependency))
        {
          /*  This is a little hard to understand.  If this key phrase is for a repeating field we want to repeat *keys[i].count
              times (see pfm_header.h).  If this is not a repeating field we want just one iteration.  If it is a repeating field but
              no values have been stored, we want to loop once so that there is a dummy in the output file for the user to see.    */

          if (keys[i].count == NULL)
            {
              loop = 1;
            }
          else
            {
              if (*keys[i].count == 0)
                {
                  loop = 1;
                }
              else
                {
                  loop = *keys[i].count;
                }
            }

          for (j = 0 ; j < loop ; j++)
            {
              if (!strcmp (keys[i].datatype, "string"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%s", (keys[i].address.c + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "i16"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%d", *(keys[i].address.i16 + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "i32"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%d", *(keys[i].address.i32 + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "uc"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%d", *(keys[i].address.uc + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "ui16"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%d", *(keys[i].address.ui16 + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "ui32"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%d", *(keys[i].address.ui32 + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "f32"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%f", *(keys[i].address.f32 + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "f64"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%f", *(keys[i].address.f64 + j));
                  ptr += strlen (ptr);
                }


              /*  f64p class of datatypes with variable precision.  */

              else if (strstr (keys[i].datatype, "f64p"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sscanf ((strstr (keys[i].datatype, "f64p") + 4), "%d", &tmp);
                  sprintf (format, "%%.%df", tmp);
                  sprintf (ptr, format, *(keys[i].address.f64 + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "f64c2"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  central = *(keys[i].address.f64c2 + j);
                  sprintf (ptr, "%.9f,%.9f", central.y, central.x);
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "i32c2"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  coord = *(keys[i].address.i32c2 + j);
                  sprintf (ptr, "%d,%d", coord.x, coord.y);
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "b"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%d", *(keys[i].address.b + j));
                  ptr += strlen (ptr);
                }
              else if (!strcmp (keys[i].datatype, "i64"))
                {
                  sprintf (ptr, "%s = ", keys[i].keyphrase);
                  ptr += strlen (ptr);
                  sprintf (ptr, "%"PRId64, *(keys[i].address.i64 + j));
                  ptr += strlen (ptr);
                }


              /*  Add the newline.    */

              sprintf (ptr, "\n");
              ptr += strlen (ptr);
            }
        }
    }


  /*  Write the header to the file.  */

  lfseek (bin_file_hnd[hnd], 0, SEEK_SET);

  lfwrite (bin_header_block[hnd], hd[hnd].bin_header_size, 1, bin_file_hnd[hnd]);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        write_bin_buffer_only

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a single bin buffer to the bin file.  It does
                        not update the coverage map.  This is used from
                        pfm_cached_io.c during loads.  This function is only
                        used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - address         =   Address of record

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_BUFFER_WRITE_ERROR

****************************************************************************/

static int32_t write_bin_buffer_only (int32_t hnd, int64_t address)
{
#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  lfseek (bin_file_hnd[hnd], address, SEEK_SET);

  if (!(lfwrite (bin_record_data[hnd], bin_off[hnd].record_size, 1, bin_file_hnd[hnd])))
    {
      sprintf (pfm_err_str, "Error writing to bin file");
      return (pfm_error = WRITE_BIN_BUFFER_WRITE_ERROR);
    }


  /*  Reset the modified flag.  */

  bin_record_modified[hnd] = NVFalse;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        write_bin_buffer

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a single bin buffer to the bin file and updates
                        the coverage map.  This function is only used internal
                        to the library.
                        Contrary to popular belief, this function is not
                        deprecated.  It just isn't used during loading from
                        pfm_cached_io.c.  We still need to update the
                        coverage map during edit sessions because we may not
                        be doing a recompute after a modification to a bin
                        record.

  - Arguments:
                        - hnd             =   PFM file handle
                        - address         =   Address of record

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_BUFFER_WRITE_ERROR

****************************************************************************/

static int32_t write_bin_buffer (int32_t hnd, int64_t address)
{
#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  lfseek (bin_file_hnd[hnd], address, SEEK_SET);

  if (!(lfwrite (bin_record_data[hnd], bin_off[hnd].record_size, 1, bin_file_hnd[hnd])))
    {
      sprintf (pfm_err_str, "Error writing to bin file");
      return (pfm_error = WRITE_BIN_BUFFER_WRITE_ERROR);
    }


  update_cov_map (hnd, address);


  /*  Reset the modified flag.  */

  bin_record_modified[hnd] = NVFalse;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        update_cov_map

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Updates the coverage map with bit flags from the
                        bin records.  This function is only used internal
                        to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - address         =   Address of record

  - Return Value:
                        - SUCCESS

****************************************************************************/

static int32_t update_cov_map (int32_t hnd, int64_t address)
{
  int32_t             df, cf, vf;
  uint32_t            val;
  int64_t             temp;
  uint8_t             cov;
  NV_I32_COORD2       coord;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  val = pfm_bit_unpack (bin_record_data[hnd], bin_off[hnd].validity_pos, hd[hnd].validity_bits);

  df = val & PFM_DATA;
  cf = val & PFM_CHECKED;
  vf = val & PFM_VERIFIED;


  /*  Compute the x and y coordinates.  */

  /*
   *   Variable casts to clear up a corruption problem with very large
   *   PFM areas.
   *   Fri Aug 13 04:34:01 2004 -- Webb McDonald (SAIC)
   */

  temp = (int64_t) (((int64_t) (address - hd[hnd].bin_header_size)) / (int64_t) bin_off[hnd].record_size);
  coord.y = temp / (int64_t) bin_header[hnd].bin_width;
  coord.x = temp % (int64_t) bin_header[hnd].bin_width;


  /* first read previous covmap byte */

  read_cov_map_index (hnd, coord, &cov);


  /* map new flags in the covmap */

  if (df)
    {
      cov |= COV_DATA;
      cov |= COV_SURVEYED;
    }
  else
    {
      cov &= (~COV_DATA);
    }

  if (cf)
    {
      cov |= COV_CHECKED;
    }
  else
    {
      cov &= (~COV_CHECKED);
    }

  if (vf)
    {
      cov |= COV_VERIFIED;
    }
  else
    {
      cov &= (~COV_VERIFIED);
    }

  write_cov_map_index (hnd, coord, cov);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        open_bin

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Opens/creates the bin file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - path            =   Pathname of file
                        - head            =   BIN_HEADER structure

  - Return Value:
                        - SUCCESS
                        - OPEN_BIN_OPEN_ERROR
                        - OPEN_BIN_CORRUPT_HEADER_ERROR
                        - WRITE_BIN_HEADER_EXCEEDED_MAX_POLY

****************************************************************************/

static int32_t open_bin (int32_t hnd, char *path, BIN_HEADER *head)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if ((bin_file_hnd[hnd] = lfopen (path, "rb+")) < 0)
    {
      /*  Added this section of code to try to open read-only if we get a permission denied error.  This can happen if the file exists but
          can't be written.  If it can't be read it will still fail.  JCD 06/22/11  */

      if (errno == EACCES)
        {
          if ((bin_file_hnd[hnd] = lfopen (path, "rb")) < 0)
            {
              sprintf (pfm_err_str, "Unable to open bin file %s", path);
              return (pfm_error = OPEN_BIN_OPEN_ERROR);
            }
          fprintf (stderr, "Bin file is opened read-only.\n");

          strcpy (head->version, " ");
          pfm_error = read_bin_header (hnd, head);

          if (pfm_error) return (pfm_error);

          if (strstr (head->version, "Software - PFM I/O library") == NULL)
            {
              sprintf (pfm_err_str, "%s bin file header corrupt.\n\n", path);
              return (pfm_error = OPEN_BIN_CORRUPT_HEADER_ERROR);
            }
        }
      else
        {
          /*  If read error was not an EEXIST, Try to open for write.  */

          if (errno == EEXIST || (bin_file_hnd[hnd] = lfopen (path, "wb+")) < 0)
            {
              sprintf (pfm_err_str, "Unable to open bin file %s", path);
              return (pfm_error = OPEN_BIN_OPEN_ERROR);
            }


          /*  Jeff's fix for different values when creating and appending.  */

          pfm_error = write_bin_header (hnd, head, NVTrue);
          if (pfm_error) return (pfm_error);


          pfm_error = read_bin_header (hnd, head);
          if (pfm_error) return (pfm_error);


          /*  V7 and greater version dependency  */

          if (list_file_ver[hnd] >= 70)
            {
              /*  Create the empty coverage file.  */

              return (create_cov_file (hnd, path));
            }
        }
    }
  else
    {
      strcpy (head->version, " ");
      pfm_error = read_bin_header (hnd, head);

      if (pfm_error) return (pfm_error);

      if (strstr (head->version, "Software - PFM I/O library") == NULL)
        {
          sprintf (pfm_err_str, "%s bin file header corrupt.\n\n", path);
          return (pfm_error = OPEN_BIN_CORRUPT_HEADER_ERROR);
        }


      /*  V7 and greater version dependency  */

      if (list_file_ver[hnd] >= 70)
        {
          pfm_error = open_cov_file (hnd, path);
          if (pfm_error) return (pfm_error);
        }
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        close_bin

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Closes the bin file and the associated coverage file
                        for v7 and greater.

  - Arguments:
                        - hnd         =   PFM file handle

  - Return Value:
                        - void

****************************************************************************/

static void close_bin (int32_t hnd)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if (bin_record_modified[hnd])
    {
      write_bin_buffer (hnd, bin_record_address[hnd]);
      bin_record_modified[hnd] = NVFalse;
    }

  lfclose (bin_file_hnd[hnd]);


  /*  V7 version dependency  */

  if (list_file_ver[hnd] >= 70) fclose (cov_file_fp[hnd]);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif
}


/***************************************************************************/
/*!

  - Module Name:        write_depth_buffer

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a single 'physical' depth buffer to the
                        index file.  This function is only used internal to
                        the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - address         =   Address of record

  - Return Value:
                        - SUCCESS
                        - WRITE_DEPTH_BUFFER_WRITE_ERROR

****************************************************************************/

static int32_t write_depth_buffer (int32_t hnd, int64_t address)
{
#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  lfseek (ndx_file_hnd[hnd], address, SEEK_SET);

  if (!(lfwrite (depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd])))
    {
      sprintf (pfm_err_str, "Unable to write to index file");
      return (pfm_error = WRITE_DEPTH_BUFFER_WRITE_ERROR);
    }


  /*  Reset the modified flag.  */

  depth_record_modified[hnd] = NVFalse;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        open_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Opens the index file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - path            =   Pathname of file

  - Return Value:
                        - SUCCESS
                        - OPEN_INDEX_OPEN_ERROR

****************************************************************************/

static int32_t open_index (int32_t hnd, char *path)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if ((ndx_file_hnd[hnd] = lfopen (path, "rb+")) < 0)
    {
      /*  Added this section of code to try to open read-only if we get a permission denied error.  This can happen if the file exists but
          can't be written.  If it can't be read it will still fail.  JCD 06/22/11  */

      if (errno == EACCES)
        {
          if ((ndx_file_hnd[hnd] = lfopen (path, "rb")) < 0)
            {
              sprintf (pfm_err_str, "Unable to open index file %s", path);
              return (pfm_error = OPEN_INDEX_OPEN_ERROR);
            }
          fprintf (stderr, "Index file is opened read-only.\n");
        }
      else
        {
          if ((ndx_file_hnd[hnd] = lfopen (path, "wb+")) < 0)
            {
              sprintf (pfm_err_str, "Unable to open index file %s", path);
              return (pfm_error = OPEN_INDEX_OPEN_ERROR);
            }
        }
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        close_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Closes the index file.

  - Arguments:
                        - hnd          =   PFM file handle

  - Return Value:
                        - void

\***************************************************************************/

static void close_index (int32_t hnd)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if (depth_record_modified[hnd])
    {
      write_depth_buffer (hnd, depth_record_address[hnd]);
      depth_record_modified[hnd] = NVFalse;
    }

  lfclose (ndx_file_hnd[hnd]);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        cfg_in

  - Programmer(s):      Jan C. Depner

  - Date Written:       September 1999

  - Purpose:            Get the PFM config information from the .pfm_cfg
                        file.  This may be (in order) in the current
                        working directory, the user's home directory, or
                        the user's path.  The default .pfm_cfg file might
                        look like this:
                        <br>
                        <br>
                        [STD BITS]=32
                        [STD SCALE]=100000.0
                        [SUBSTITUTE PATH]=N:,/net/alh-pogo1/data1,/data1
                        [SUBSTITUTE PATH]=O:,/net/alh-pogo1/data2,/data2
                        <br>
                        <br>

  - Arguments:
                        - head           =   BIN file header structure
                        - hnd            =   PFM file handle

  - Return Value:
                        - void

  - Caveats:            This function is now mostly used for setting SUBSTITUTE PATHs
                        in a mixed Windows/Linux/UNIX environment.  RECORD_LENGTH,
                        RECORD_POINTER_BITS, and OFFSET_BITS can no longer be replaced
                        using the .pfm_cfg file.  To change those values and the
                        count bits, file bits, line bits, record (ping) bits, and
                        subrecord (beam) bits use the settings in the BIN_HEADER
                        record when creating a PFM file.

****************************************************************************/

static void cfg_in (BIN_HEADER *head, int32_t hnd)
{
  char        varin[1024], info[1024], string[1024];
  FILE        *fp;
  int32_t     i;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  substitute_cnt[hnd] = 0;


  /*  Set the defaults for the bin header data.  */

  hd[hnd].record_length = PFM_RECORD_LENGTH;
  hd[hnd].count_bits = PFM_COUNT_BITS;
  hd[hnd].std_bits = PFM_STD_BITS;
  hd[hnd].std_scale = PFM_STD_SCALE;
  hd[hnd].record_pointer_bits = PFM_RECORD_POINTER_BITS;
  hd[hnd].file_number_bits = PFM_FILE_BITS;
  hd[hnd].line_number_bits = PFM_LINE_BITS;
  hd[hnd].ping_number_bits = PFM_PING_BITS;
  hd[hnd].beam_number_bits = PFM_BEAM_BITS;
  hd[hnd].offset_bits = PFM_OFFSET_BITS;
  hd[hnd].validity_bits = PFM_VALIDITY_BITS;


  /*  Check to see if the user application has passed different numbers for input files, lines, pings, beams, record length,
      bin count, record pointer bits, and/or offset bits.  If so, we need to modify the default number of bits for these fields.  */

  if (head->record_length) hd[hnd].record_length = head->record_length;

  if (head->max_bin_soundings) hd[hnd].count_bits = NINT (log10 ((double) ((head->max_bin_soundings))) / log10 (2.0) + 0.5);

  if (head->max_record_pointer) hd[hnd].record_pointer_bits = NINT (log10 ((double) ((head->max_record_pointer))) / log10 (2.0) + 0.5);

  if (head->max_input_files) hd[hnd].file_number_bits = NINT (log10 ((double) ((head->max_input_files))) / log10 (2.0) + 0.5);

  if (head->max_input_lines) hd[hnd].line_number_bits = NINT (log10 ((double) ((head->max_input_lines))) / log10 (2.0) + 0.5);

  if (head->max_input_pings) hd[hnd].ping_number_bits = NINT (log10 ((double) ((head->max_input_pings))) / log10 (2.0) + 0.5);

  if (head->max_input_beams) hd[hnd].beam_number_bits = NINT (log10 ((double) ((head->max_input_beams))) / log10 (2.0) + 0.5);

  if (head->offset_bits) hd[hnd].offset_bits = head->offset_bits;


  hd[hnd].horizontal_error_bits = 0;
  hd[hnd].vertical_error_bits = 0;

  for ( i = 0 ; i < NUM_ATTR ; i++)
    {
      hd[hnd].bin_attr_bits[i] = 0;
      hd[hnd].ndx_attr_bits[i] = 0;
    }


  /*  If the startup file was found...    */

  if ((fp = pfm_find_startup (".pfm_cfg")) != NULL)
    {
      /*  Read each entry.    */

      while (pfm_ngets (varin, sizeof (varin), fp) != NULL)
        {
          /*  Ignore comments.  */

          if (varin[0] != '#')
            {
              /*  Put everything to the right of the equals sign in 'info'.   */

              if (strchr (varin, '=') != NULL)
                {
                  strcpy (info, (strchr (varin, '=') + 1));


                  /*  Check input for matching strings and load values if found.  */

                  if (strstr (varin, "[STD BITS]") != NULL) sscanf (info, "%d", &hd[hnd].std_bits);
                  if (strstr (varin, "[STD SCALE]") != NULL) sscanf (info, "%f", &hd[hnd].std_scale);
                  if (strstr (varin, "[VALIDITY BITS]") != NULL) sscanf (info, "%d", &hd[hnd].validity_bits);
                  if (strstr (varin, "[SUBSTITUTE PATH]") != NULL && substitute_cnt[hnd] < MAX_SUB_PATHS)
                    {
                      get_string (varin, string);


                      /*  Throw out malformed substitute paths.  */

                      if (strchr (string, ','))
                        {
                          /*  Check for more than 1 UNIX substitute path.  */

                          if (strchr (string, ',') == strrchr (string, ','))
                            {
                              strcpy (substitute_path[hnd][substitute_cnt[hnd]][0], strtok (string, ","));
                              strcpy (substitute_path[hnd][substitute_cnt[hnd]][1], strtok (NULL, ","));
                              substitute_path[hnd][substitute_cnt[hnd]][2][0] = 0;
                            }
                          else
                            {
                              strcpy (substitute_path[hnd][substitute_cnt[hnd]][0], strtok (string, ","));
                              strcpy (substitute_path[hnd][substitute_cnt[hnd]][1], strtok (NULL, ","));
                              strcpy (substitute_path[hnd][substitute_cnt[hnd]][2], strtok (NULL, ","));
                            }

                          substitute_cnt[hnd]++;
                        }
                      else
                        {
                          fprintf (stderr, "\n\nMalformed substitute path entry in .pfm_cfg\n");
                          fprintf (stderr, "%s\n", varin);
                        }
                    }
                  else
                    {
                      /*  Give a message if they go over MAX_SUB_PATHS.  */

                      if (substitute_cnt[hnd] >= MAX_SUB_PATHS)
                        {
                          fprintf (stderr, "\n\nToo many substitute paths in .pfm_cfg\n");
                          fprintf (stderr, "Only %d allowed, all others will be ignored.\n\n", MAX_SUB_PATHS);
                        }
                    }
                }
              else
                {
                  fprintf (stderr, "\n\nMalformed entry in .pfm_cfg\n");
                  fprintf (stderr, "%s\n", varin);
                }
            }
        }

      fclose (fp);
    }

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        open_line_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 2001

  - Purpose:            Opens already existing line list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - path            =   Pathname of line file

  - Return Value:
                        - SUCCESS
                        - OPEN_LINE_FILE_OPEN_ERROR

****************************************************************************/

static int32_t open_line_file (int32_t hnd, char *path)
{
  char                line[512], string[512];


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if ((line_file_fp[hnd] = fopen (path, "r+")) == NULL)
    {
      if ((line_file_fp[hnd] = fopen (path, "r")) == NULL)
        {
          sprintf (pfm_err_str, "Error opening %s", path);
          return (pfm_error = OPEN_LINE_FILE_OPEN_ERROR);
        }
      fprintf (stderr, "Line file is opened read-only.\n");
    }

  line_file_count[hnd] = 0;


  pfm_error = SUCCESS;


  /*  Read the input line names and save pointers to them in the line file.  Leave the file open for further additions or reads. */

  line_file_index[hnd][line_file_count[hnd]] = ftell (line_file_fp[hnd]);
  while (pfm_ngets (string, sizeof (line), line_file_fp[hnd]) != NULL)
    {
      sscanf (string, "%s", line);

      line_file_count[hnd]++;
      line_file_index[hnd][line_file_count[hnd]] = ftell (line_file_fp[hnd]);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error);
}



/***************************************************************************/
/*!

  - Module Name:        create_line_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Creates and opens a new line file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - line_path       =   Pathname of line file

  - Return Value:
                        - SUCCESS
                        - CREATE_LINE_FILE_FILE_EXISTS
                        - CREATE_LINE_FILE_OPEN_ERROR
                        - OPEN_LINE_FILE_OPEN_ERROR

****************************************************************************/

static int32_t create_line_file (int32_t hnd, char *line_path)
{
  int32_t             status;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  If the line file exists, error out.  */

  if ((line_file_fp[hnd] = fopen (line_path, "r")) != NULL)
    {
      sprintf (pfm_err_str, "Trying to create line file %s, already exists", line_path);
      fclose (line_file_fp[hnd]);
      return (pfm_error = CREATE_LINE_FILE_FILE_EXISTS);
    }


  if ((line_file_fp[hnd] = fopen (line_path, "w")) == NULL)
    {
      sprintf (pfm_err_str, "Unable to open line file %s in create_line_file", line_path);
      return (pfm_error = CREATE_LINE_FILE_OPEN_ERROR);
    }


  fclose (line_file_fp[hnd]);


  status = open_line_file (hnd, line_path);

  if (status) return (pfm_error = status);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        checkpoint_pfm_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       September 2001

  - Purpose:            Saves a checkpoint file for recovery if a program
                        bombs.  This is usually used for loads where you
                        are appending to a preexisting file.  The format of
                        the checkpoint file is:                            <br><br>
                        8 bits              - completion flag (1 = closed) <br>
                        32 bits             - current lines in list file   <br>
                        32 bits             - current lines in line file   <br>
                        64 bits             - current size of index file   <br><br>
                        Current list file stored as ASCII                  <br>
                        Current line file stored as ASCII                  <br>
                        Current bin_header stored as ASCII                 <br><br>
                        bin_width x bin_height records (unsigned char):
                            - COUNT BITS          = number of soundings in bin
                            - RECORD POINTER BITS = tail pointer

  - Arguments:
                        - hnd             =   PFM file handle

  - Return Value:
                        - void

****************************************************************************/

static void checkpoint_pfm_file (int32_t hnd)
{
  FILE           *chk_fp;
  int32_t        i, j, list_size, line_size, num_bytes, percent, old_percent;
  char           chk_file[512], string[1024];
  uint8_t        complete, *buffer;
  NV_I32_COORD2  coord;
  int64_t        index_size;
  BIN_RECORD     bin_rec;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  sprintf (chk_file, "%s.chk", list_path[hnd]);

  if ((chk_fp = fopen64 (chk_file, "wb+")) == NULL)
    {
      fprintf (stderr,"\n\nUnable to open checkpoint file.\n");
      perror (chk_file);
      return;
    }


  num_bytes  = (hd[hnd].record_pointer_bits + hd[hnd].count_bits) / 8;


  /*  Set the size to the next larger whole byte if it wasn't an even multiple of 8.   */

  if (num_bytes % 8) num_bytes++;


  buffer = (uint8_t *) malloc (hd[hnd].head.bin_width * num_bytes);
  if (buffer == NULL)
    {
      fprintf (stderr,"\n\nUnable to allocate checkpoint memory.\n");
      fclose (chk_fp);
      return;
    }


  if (!pfm_progress_callback) fprintf (stderr, "\nCreating checkpoint file :          \r");


  /*  Write the completion flag to show that the checkpoint file is not complete.  */

  complete = 0;
  fwrite (&complete, 1, 1, chk_fp);


  /*  Write a placeholder for the list file size.  */

  list_size = 0;
  fwrite (&list_size, 4, 1, chk_fp);


  /*  Write a placeholder for the line file size.  */

  line_size = 0;
  fwrite (&line_size, 4, 1, chk_fp);


  /*  Write the index file size.  */

  lfseek (ndx_file_hnd[hnd], 0, SEEK_END);
  index_size = lftell (ndx_file_hnd[hnd]);
  lfseek (ndx_file_hnd[hnd], 0, SEEK_SET);
  pfm_double_bit_pack (buffer, 0, 64, index_size);
  fwrite (buffer, 8, 1, chk_fp);


  /*  Write the list file contents.  */

  list_size = 0;
  fseek (list_file_fp[hnd], 0, SEEK_SET);
  while (fgets (string, sizeof (string), list_file_fp[hnd]))
    {
      fprintf (chk_fp, "%s", string);
      list_size++;
    }


  /*  Write the line file contents.  */

  line_size = 0;
  fseek (line_file_fp[hnd], 0, SEEK_SET);
  while (fgets (string, sizeof (string), line_file_fp[hnd]))
    {
      fprintf (chk_fp, "%s", string);
      line_size++;
    }


  /*  Write the list and line counts.  */

  fseeko64 (chk_fp, 1, SEEK_SET);
  pfm_bit_pack (buffer, 0, 32, list_size);
  fwrite (buffer, 4, 1, chk_fp);
  pfm_bit_pack (buffer, 0, 32, line_size);
  fwrite (buffer, 4, 1, chk_fp);
  fseeko64 (chk_fp, 0, SEEK_END);


  /*  Write the bin header.  */

  fwrite (bin_header_block[hnd], hd[hnd].bin_header_size, 1, chk_fp);


  /*  Write the number of soundings and the tail pointer.  */

  old_percent = -1;
  for (i = 0 ; i < hd[hnd].head.bin_height ; i++)
    {
      coord.y = i;

      for (j = 0 ; j < hd[hnd].head.bin_width ; j++)
        {
          coord.x = j;

          read_bin_record_index (hnd, coord, &bin_rec);

          pfm_bit_pack (buffer, 0, hd[hnd].count_bits, bin_rec.num_soundings);
          pfm_double_bit_pack (buffer, hd[hnd].count_bits, hd[hnd].record_pointer_bits, bin_record_tail_pointer[hnd]);

          fwrite (buffer, num_bytes, 1, chk_fp);
        }


      percent = ((float) i / (float) hd[hnd].head.bin_height) * 100.0;
      if (percent != old_percent)
        {
          old_percent = percent;

          /*  GS: now calls a callback if it is registered.  */

          if (pfm_progress_callback)
            {
              (*pfm_progress_callback) (3, percent);
            }
          else
            {
              fprintf (stderr, "Creating checkpoint file : %03d%%     \r", percent);
            }
        }
    }

  if (!pfm_progress_callback) fprintf (stderr, "                                           \r");


  free (buffer);


  fseeko64 (chk_fp, 0, SEEK_SET);
  complete = 1;
  fwrite (&complete, 1, 1, chk_fp);

  fclose (chk_fp);


  return;
}



/***************************************************************************/
/*!

  - Module Name:        recover_pfm_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       September 2001

  - Purpose:            Recovers from a checkpoint file if it is complete.

  - Arguments:
                        - path            =   List file path

  - Return Value:
                        - 0 on success
                        - -1 on failure

  - Caveats:            If you have a checkpoint file from a previous version
                        of the PFM API this won't work.  Why would you have
                        an old checkpoint file anyway???

\***************************************************************************/

static int32_t recover_pfm_file (char *path)
{
  FILE           *chk_fp, *list_fp, *line_fp;
  int32_t        i, j, list_size, line_size, num_bytes, percent, old_percent, hnd, ndx_hnd, bin_hnd;
  char           chk_file[512], line_path[512], bin_path[512], ndx_path[512], string[1024], hdr[BIN_HEADER_SIZE];
  uint8_t        complete, *buffer = NULL, buff[9];
  NV_I32_COORD2  coord;
  int64_t        ndx_size;
  BIN_RECORD     bin_rec;
  PFM_OPEN_ARGS  open_args;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif


  sprintf (chk_file, "%s.chk", path);

  if ((chk_fp = fopen (chk_file, "rb")) == NULL)
    {
      fprintf (stderr,"\n\nUnable to open checkpoint file.\n");
      perror (chk_file);
      return (-1);
    }


  if (!fread (&complete, 1, 1, chk_fp))
    {
      sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      fclose (chk_fp);
      return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
    }


  fprintf (stderr, "\nRecovering from checkpoint file :           \r");


  if (!fread (buff, 4, 1, chk_fp))
    {
      sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      fclose (chk_fp);
      return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
    }
  list_size = pfm_bit_unpack (buff, 0, 32);

  if (!fread (buff, 4, 1, chk_fp))
    {
      sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      fclose (chk_fp);
      return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
    }
  line_size = pfm_bit_unpack (buff, 0, 32);


  /*  The ndx size (stored in buff) will be unpacked below the list file read since we need to know the PFM version.  */

  if (!fread (buff, 8, 1, chk_fp))
    {
      sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      fclose (chk_fp);
      return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
    }


  if ((list_fp = fopen (path, "w")) == NULL)
    {
      fclose (chk_fp);
      return (-1);
    }

  for (i = 0 ; i < list_size ; i++)
    {
      if (fgets (string, sizeof (string), chk_fp) == NULL)
	{
	  sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
          fclose (list_fp);
          fclose (chk_fp);
	  return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
	}
      fprintf (list_fp, "%s", string);


      /*  Bin path.  */

      if (i == 1)
        {
          string[strlen (string) - 1] = 0;
          sscanf (string, "%s", bin_path);
        }


      /*  Ndx path.  */

      if (i == 2)
        {
          string[strlen (string) - 1] = 0;
          sscanf (string, "%s", ndx_path);
        }
    }

  fclose (list_fp);


  /*  Create the line file path name from the list file name.  This is hard-wired to be the list file name with a .lin extension.  Check for
      post 4.5 .ctl extension on list file.  */

  if (!strcmp (&path[strlen (path) - 4], ".ctl"))
    {
      strcpy (line_path, path);
      sprintf (&line_path[strlen (line_path) - 4], ".lin");
    }
  else
    {
      sprintf (line_path, "%s.lin", path);
    }

  if ((line_fp = fopen (line_path, "w")) == NULL)
    {
      fclose (chk_fp);
      return (-1);
    }

  for (i = 0 ; i < line_size ; i++)
    {
      if (fgets (string, sizeof (string), chk_fp) == NULL)
	{
	  sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
          fclose (line_fp);
          fclose (chk_fp);
	  return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
	}
      fprintf (line_fp, "%s", string);
    }

  fclose (line_fp);


  /*  Read the saved bin header.  */

  if (!fread (hdr, BIN_HEADER_SIZE, 1, chk_fp))
    {
      sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      fclose (chk_fp);
      return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
    }


  /*  Unpack the ndx size now that we know the PFM version and who created the file.  */

  ndx_size = pfm_double_bit_unpack (buff, 0, 64);


  /*  Write the saved bin header to the bin file.  */

  if ((bin_hnd = lfopen (bin_path, "rb+")) < 0)
    {
      fclose (chk_fp);
      return (-1);
    }

  lfwrite (hdr, BIN_HEADER_SIZE, 1, bin_hnd);

  lfclose (bin_hnd);


  /*  Truncate the ndx file to the previous size.  */

  if ((ndx_hnd = lfopen (ndx_path, "rb+")) < 0)
    {
      fclose (chk_fp);
      return (-1);
    }

  if (lftruncate (ndx_hnd, ndx_size))
    {
      fclose (chk_fp);
      return (-1);
    }

  lfclose (ndx_hnd);


  /*  Open the files as a PFM structure.  */

  no_check = NVTrue;
  strcpy (open_args.list_path, path);
  open_args.checkpoint = 0;
  hnd = open_pfm_file (&open_args);


  /*  Number of bytes needed to store the number of soundings and tail pointer.  */

  num_bytes  = (hd[hnd].record_pointer_bits + hd[hnd].count_bits) / 8;


  /*  Set the size to the next larger whole byte if it wasn't an even multiple of 8.   */

  if (num_bytes % 8) num_bytes++;


  buffer = (uint8_t *) malloc (hd[hnd].head.bin_width * num_bytes);
  if (buffer == NULL)
    {
      fprintf (stderr,"\n\nUnable to allocate checkpoint memory.\n");
      fclose (chk_fp);
      return (-1);
    }


  /*  Read the number of soundings and the tail pointer.  */

  old_percent = -1;
  for (i = 0 ; i < hd[hnd].head.bin_height ; i++)
    {
      coord.y = i;

      for (j = 0 ; j < hd[hnd].head.bin_width ; j++)
        {
          if (!fread (buffer, num_bytes, 1, chk_fp))
	    {
	      sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
	      return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
	    }

          coord.x = j;

          read_bin_record_index (hnd, coord, &bin_rec);

          bin_rec.num_soundings = pfm_bit_unpack (buffer, 0, hd[hnd].count_bits);
          bin_record_tail_pointer[hnd] = pfm_double_bit_unpack (buffer, hd[hnd].count_bits, hd[hnd].record_pointer_bits);

          write_bin_record_index (hnd, &bin_rec);
        }


      percent = ((float) i / (float) hd[hnd].head.bin_height) * 100.0;
      if (percent != old_percent)
        {
          old_percent = percent;
          fprintf (stderr, "Recovering from checkpoint file : %03d%%      \r", percent);
        }
    }

  fprintf (stderr, "                                                \r");


  free (buffer);

  fclose (chk_fp);

  remove (chk_file);

  close_pfm_file (hnd);


  return (0);
}



/***************************************************************************/
/*!

  - Module Name:        set_pfm_data_types

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2011

  - Purpose:            Sets the available data type descriptions.  I moved
                        this from it's previous location (in open_pfm_file)
                        so that we could get the descriptions without having
                        to open a PFM file.

  - Arguments:
                        - void

  - Return Value:
                        - void

****************************************************************************/

void set_pfm_data_types ()
{
  static uint8_t once = NVFalse;


  /*  Only do this one time.  */

  if (once) return;
  once = NVTrue;


  /* MP: I don't really like this trick of assigning static storage to an array of static points. This should be allocated and
     assigned at runtime. - Then again I'm too lazy to change it right now :-)  */

  PFM_data_type[PFM_UNDEFINED_DATA]     = "Undefined";
  PFM_data_type[PFM_CHRTR_DATA]         = "NAVOCEANO CHRTR format";
  PFM_data_type[PFM_GSF_DATA]           = "Generic Sensor Format";
  PFM_data_type[PFM_SHOALS_OUT_DATA]    = "Optech SHOALS .out format";
  PFM_data_type[PFM_CHARTS_HOF_DATA]    = "CHARTS Hydrographic Output Format";
  PFM_data_type[PFM_NAVO_ASCII_DATA]    = "NAVOCEANO ASCII XYZ format";
  PFM_data_type[PFM_HTF_DATA]           = "Royal Australian Navy HTF";
  PFM_data_type[PFM_WLF_DATA]           = "Waveform LIDAR Format";
  PFM_data_type[PFM_DTM_DATA]           = "IVS DTM data format";
  PFM_data_type[PFM_HDCS_DATA]          = "Caris HDCS data format";
  PFM_data_type[PFM_ASCXYZ_DATA]        = "Ascii XYZ data format";
  PFM_data_type[PFM_CNCBIN_DATA]        = "C&C Binary XYZ data format";
  PFM_data_type[PFM_STBBIN_DATA]        = "STB Binary XYZ data format";
  PFM_data_type[PFM_XYZBIN_DATA]        = "IVS XYZ Binary data format";
  PFM_data_type[PFM_OMG_DATA]           = "OMG Merged data format";
  PFM_data_type[PFM_CNCTRACE_DATA]      = "C&C Trace data format";
  PFM_data_type[PFM_NEPTUNE_DATA]       = "Simrad Neptune data format";
  PFM_data_type[PFM_SHOALS_1K_DATA]     = "Shoals 1K(HOF) data format";
  PFM_data_type[PFM_SHOALS_ABH_DATA]    = "Shoals Airborne data format";
  PFM_data_type[PFM_SURF_DATA]          = "Altas SURF data format";
  PFM_data_type[PFM_SMF_DATA]           = "French Carribes format";
  PFM_data_type[PFM_VISE_DATA]          = "Danish FAU data format";
  PFM_data_type[PFM_PFM_DATA]           = "NAVOCEANO PFM data format";
  PFM_data_type[PFM_MIF_DATA]           = "MapInfo MIF format";
  PFM_data_type[PFM_SHOALS_TOF_DATA]    = "Shoals TOF data format";
  PFM_data_type[PFM_UNISIPS_DEPTH_DATA] = "UNISIPS depth data format";
  PFM_data_type[PFM_HYD93B_DATA]        = "Hydro93 Binary data format";
  PFM_data_type[PFM_LADS_DATA]          = "Lads Lidar data format";
  PFM_data_type[PFM_HS2_DATA]           = "Hypack data format";
  PFM_data_type[PFM_9COLLIDAR]          = "9 Column Ascii Lidar data format";
  PFM_data_type[PFM_FGE_DATA]           = "Danish Geographic FAU data format";
  PFM_data_type[PFM_PIVOT_DATA]         = "SHOM Pivot data format";
  PFM_data_type[PFM_MBSYSTEM_DATA]      = "MBSystem data format";
  PFM_data_type[PFM_LAS_DATA]           = "LAS data format";
  PFM_data_type[PFM_BDI_DATA]           = "Swedish Binary DIS format";
  PFM_data_type[PFM_NAVO_LLZ_DATA]      = "NAVO lat/lon/depth data format";
  PFM_data_type[PFM_LADSDB_DATA]        = "Lads Database Link format";
  PFM_data_type[PFM_DTED_DATA]          = "NGA DTED format";
  PFM_data_type[PFM_HAWKEYE_HYDRO_DATA] = "Hawkeye CSS Generic Binary Output Format (hydro)";
  PFM_data_type[PFM_HAWKEYE_TOPO_DATA]  = "Hawkeye CSS Generic Binary Output Format (topo)";
  PFM_data_type[PFM_BAG_DATA]           = "Bathymetric Attributed Grid format";
  PFM_data_type[PFM_CZMIL_DATA]         = "Coastal Zone Mapping and Imaging LIDAR Format";
}



/***************************************************************************/
/*!

  - Module Name:        open_pfm_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Opens all of the associated PFM files.

  - Restrictions:
                        <br>
                        <br>
                        - This function will try to open and append to an
                          existing PFM list file or create a new PFM file.
                          Unless you really know what you are doing, don't
                          use this function to open an existing PFM file.
                          USE open_existing_pfm_file instead!  In most cases
                          you only want to use this function to create a new
                          PFM file.  When creating a new PFM file, <b>all</b>
                          of the arguments in the PFM_OPEN_ARGS structure
                          need to be set (do <i>memset (&open_args, 0,
                          sizeof (PFM_OPEN_ARGS))</i> to zero out the
                          PFM_OPEN_ARGS and the PFM_OPEN_ARGS.BIN_HEADER
                          structures).  You also need to set the following
                          members of the PFM_OPEN_ARGS.head structure :
                          <br>
                          <br>
                            - bin_size_xy or (x_bin_size_degrees and y_bin_size_degrees while setting bin_size_xy to 0.0)
                            - polygon
                            - polygon_count
                            - proj_data.projection (and other proj data)
                            - num_bin_attr
                            - bin_attr_name[0 to num_bin_attr - 1]
                            - bin_attr_offset[0 to num_bin_attr - 1]
                            - bin_attr_max[0 to num_bin_attr - 1]
                            - bin_attr_scale[0 to num_bin_attr - 1]
                            - num_ndx_attr
                            - ndx_attr_name[0 to num_ndx_attr - 1]
                            - min_ndx_attr[0 to num_ndx_attr - 1]
                            - max_ndx_attr[0 to num_ndx_attr - 1]
                            - ndx_attr_scale[0 to num_ndx_attr - 1]
                            - user_flag_name[PFM_USER_FLAGS]
                            - horizontal_error_scale
                            - max_horizontal_error
                            - vertical_error_scale
                            - max_vertical_error
                            <br>
                            <br>
                        - You may optionally set the following members of the
                          PFM_OPEN_ARGS.head structure to change the associated
                          field sizes in the PFM:
                          <br>
                          <br>
                            - record_length (set to 0 for default)
                            - max_bin_soundings (set to 0 for default)
                            - max_max_record_pointer (set to 0 for default)
                            - max_input_files (set to 0 for default)
                            - max_input_lines (set to 0 for default)
                            - max_input_pings (set to 0 for default)
                            - max_input_beams (set to 0 for default)
                            - offset_bits (set to 0 for default)
                            <br>
                            <br>
                        - If bin_size_xy is 0.0 then we will use the
                          bin_sizes in degrees to define the final bin size
                          and the area dimensions.  This is useful for
                          defining PFMs for areas where you want to use
                          matching latitudinal and longitudinal bin sizes.
                          Pay special attention to checkpoint.  This can be
                          used to save/recover your file on an aborted load.
                          See pfm.h for better descriptions of these values.
                        <br>
                        <br>
                        - IMPORTANT NOTE : For post 4.7 PFM structures the
                          file name in open_args->list_file will actually be
                          the PFM handle file name.  The actual PFM data
                          directory and structure file names will be derived
                          from the handle file name.  If the handle file name
                          is fred.pfm then a directory called fred.pfm.data
                          will exist (or be created) that contains the files
                          fred.pfm.ctl, fred.pfm.bin, fred.pfm.ndx,
                          fred.pfm.lin, and the rest of the PFM structure
                          files.  Pre 5.0 structures will use the actual list
                          file name here.

  - Arguments:
                        - open_args       =   see pfm.h

  - Return Value:
                        - PFM file handle or -1 on error (error status stored
                          in pfm_error, can be printed with pfm_error_exit)
                        - Possible error status :
                            - OPEN_HANDLE_FILE_CREATE_ERROR
                            - CREATE_PFM_DATA_DIRECTORY_ERROR
                            - CREATE_LIST_FILE_FILE_EXISTS
                            - CREATE_LIST_FILE_OPEN_ERROR
                            - OPEN_LIST_FILE_OPEN_ERROR
                            - OPEN_LIST_FILE_READ_VERSION_ERROR
                            - OPEN_LIST_FILE_READ_BIN_ERROR
                            - OPEN_LIST_FILE_READ_INDEX_ERROR
                            - OPEN_LIST_FILE_READ_IMAGE_ERROR
                            - OPEN_LIST_FILE_CORRUPTED_FILE_ERROR
                            - OPEN_BIN_OPEN_ERROR
                            - OPEN_BIN_HEADER_CORRUPT_ERROR
                            - WRITE_BIN_HEADER_EXCEEDED_MAX_POLY
                            - OPEN_INDEX_OPEN_ERROR
                            - CHECK_INPUT_FILE_OPEN_ERROR
                            - CHECK_INPUT_FILE_WRITE_ERROR
                            - CHECKPOINT_FILE_EXISTS_ERROR
                            - CHECKPOINT_FILE_UNRECOVERABLE_ERROR

  - Warning:            This function is not thread safe.

****************************************************************************/

int32_t open_pfm_file (PFM_OPEN_ARGS *open_args)
{
  int32_t             hnd = -1, i, status, line_file_open = -1;
  float               temp;
  FILE                *temp_fp, *chk_fp, *handle_fp;
  char                line_path[512], chk_file[512], string[2048], data_dir_name[2048];
  uint8_t             complete;
  uint8_t             new, directory;
  char                dir_list_path[512];
  time_t              systemtime;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif


  /*  Set the PFM data type descriptions.  */

  set_pfm_data_types ();


  /*  As of version 4.6 we put all of the PFM "files" into a single directory with a .pfm file associated with it.  The naming convention
      calls for a .pfm file associated with the directory.  The .pfm file will not be the list file but will contain a version string.  This will
      keep things simple for people who are used to opening a single .pfm list file to open the structure.  As an example of the naming convention,
      if you have a file called fred.pfm you will have a directory called fred.pfm.data.  */

  strcpy (dir_list_path, open_args->list_path);
  directory = NVFalse;


  if ((handle_fp = fopen (open_args->list_path, "r")) != NULL)
    {
      if (fgets (string, sizeof (string), handle_fp) == NULL)
	{
	  sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
	  return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
	}

      if (strstr (string, "PFM Handle File"))
        {
          sprintf (string, "%s.data%1c%s", open_args->list_path, SEPARATOR, pfm_basename (open_args->list_path));

          strcpy (open_args->bin_path, string);
          strcat (open_args->bin_path, ".bin");

          strcpy (open_args->index_path, string);
          strcat (open_args->index_path, ".ndx");

          sprintf (dir_list_path, "%s.ctl", string);
          strcpy (open_args->ctl_path, dir_list_path );

          directory = NVTrue;
        }
      fclose (handle_fp);
    }
  else
    {
      if (errno == ENOENT)
        {
          if ((handle_fp = fopen (open_args->list_path, "w")) == NULL)
            {
              sprintf (pfm_err_str, "Unable to create the PFM handle file %s, %s", open_args->list_path, strerror (errno));
              return (pfm_error = OPEN_HANDLE_FILE_CREATE_ERROR);
            }

          sprintf (string, "%s.data%1c%s", open_args->list_path, SEPARATOR, pfm_basename (open_args->list_path));

          sprintf (data_dir_name, "%s.data", open_args->list_path);

#ifdef NVWIN3X
 #if defined (__MINGW64__) || defined (__MINGW32__)
          if (mkdir (data_dir_name))
  #else
          if (_mkdir (data_dir_name))
  #endif
#else
          if (mkdir (data_dir_name, 00777))
#endif
            {
              sprintf (pfm_err_str, "Unable to create the PFM data directory %s, %s", data_dir_name, strerror (errno));
              fclose (handle_fp);
              return (pfm_error = CREATE_PFM_DATA_DIRECTORY_ERROR);
            }

          fprintf (handle_fp, "PFM Handle File - %s\n", VERSION);
          fprintf (handle_fp, "#\n");
          fprintf (handle_fp, "################################################################################\n");
          fprintf (handle_fp, "#\n");
          fprintf (handle_fp, "# This is a PFM handle file.  If you were expecting to see the PFM list file\n");
          fprintf (handle_fp, "# you need to look for a directory called:\n#\n#\t%s\n#\n", data_dir_name);
          fprintf (handle_fp, "# In that directory should be a file called:\n#\n#\t%s.ctl\n#\n", string);
          fprintf (handle_fp, "# That is the PFM list (or control) file.\n");
          fprintf (handle_fp, "#\n");
          fprintf (handle_fp, "################################################################################\n");
          fprintf (handle_fp, "#\n");

          fclose (handle_fp);

          strcpy (open_args->bin_path, string);
          strcat (open_args->bin_path, ".bin");

          strcpy (open_args->index_path, string);
          strcat (open_args->index_path, ".ndx");

          sprintf (dir_list_path, "%s.ctl", string);
          strcpy( open_args->ctl_path, dir_list_path );

          directory = NVTrue;


          /*  If bin_attr_offset and bin_attr_max were not found, set them to min_bin_attr and max_bin_attr.

              The bin_attr_max and bin_attr_offset values are holdovers from when bin and ndx attributes were
              related.  Now that they are separate we want to use max_bin_attr and min_bin_attr from the header
              structure.  Unfortunately the old values are still used throughout the code so we have to make
              sure that we can use either the originals or the new ones.  I think Danny Neville or Graeme
              Sweet took care of this when opening an existing file (the header strings were different
              between old and new) but we need to do it here when opening a new file as well.  JCD  */

          for (i = 0 ; i < open_args->head.num_bin_attr ; i++)
            {
              if ((open_args->head.bin_attr_scale[i] > 0.000001) && (open_args->bin_attr_offset[i] < 0.000001) && (open_args->bin_attr_max[i] < 0.000001))
                {
		  open_args->bin_attr_offset[i] = -open_args->head.min_bin_attr[i];
                  open_args->bin_attr_max[i] = open_args->head.max_bin_attr[i];
                }
            }


          /*  Since we are creating a new file we want to set the version and creation date.  */

          strcpy (open_args->head.version, VERSION);

          systemtime = time (&systemtime);
          strcpy (open_args->head.date, asctime (localtime (&systemtime)));

          open_args->head.date[strlen (open_args->head.date) - 1] = 0;
        }
    }


  /*  Look for a checkpoint file.  If present, and the checkpoint variable is set to 2, recover.  Otherwise kick out unless we have set
      no_check (in the process of recovering).  Also, if checkpoint is set to 3 this is a special case where we are appending to
      the same PFM using separate handles (usually separate threads).  */

  if (!no_check && open_args->checkpoint != 3)
    {
      sprintf (chk_file, "%s.chk", dir_list_path);

      if ((chk_fp = fopen (chk_file, "rb")) != NULL)
        {
          if (!fread (&complete, 1, 1, chk_fp))
	    {
	      sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
	      return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
	    }

          if (complete)
            {
              if (open_args->checkpoint == 2)
                {
                  fclose (chk_fp);
                  if (recover_pfm_file (dir_list_path))
                    {
                      sprintf (pfm_err_str, "Unable to recover checkpoint file for %s.  Bummer!\n", open_args->list_path);
                      remove (chk_file);

                      return (pfm_error = CHECKPOINT_FILE_UNRECOVERABLE_ERROR);
                    }
                }
              else
                {
                  sprintf (pfm_err_str,
                           "\nCheckpoint file exists.\nPrevious load or append operation aborted.\nPlease run pfm_recompute %s to rebuild the file.\n",
                           open_args->list_path);

                  return (pfm_error = CHECKPOINT_FILE_EXISTS_ERROR);
                }
            }
          else
            {
              remove (chk_file);
            }
        }
    }


  new = NVFalse;
  pfm_error = 0;

  for (i = 0 ; i < MAX_PFM_FILES ; i++)
    {
      if (pfm_hnd[i] == -1)
        {
          pfm_hnd[i] = i;
          hnd = i;
          list_dir[hnd] = directory;
          bin_record_data[hnd] = NULL;
          bin_record_modified[hnd] = NVFalse;
          use_chain_pointer[hnd] = NVTrue;
          depth_record_data[hnd] = NULL;
          zero_depth_record_data[hnd] = NULL;
          depth_record_modified[hnd] = NVFalse;
          x_offset_scale[hnd] = 0.0;
          y_offset_scale[hnd] = 0.0;
          geo_dist_init[hnd] = NVFalse;
          geo_distance[hnd] = NULL;
          geo_post[hnd] = NULL;
          read_cov_map_index_prev_row[hnd] = -1;
          read_cov_map_index_prev_num_cols[hnd] = 0;
          read_cov_map_index_row[hnd] = NULL;
          read_cov_map_index_bin_row[hnd] = NULL;
          mutex[hnd] = 0;
          break;
        }
    }


  /*  Set the previous address, coord, pos for use later.  */

  previous_bin_address[hnd] = -1;
  previous_depth_block[hnd] = -1;
  depth_record_pos[hnd] = 0;


  /*  Set the bit field sizes from the .pfm_cfg file or default.  */

  cfg_in (&open_args->head, hnd);


  /*  Set the depth bit field sizes based on the input max, scale, and offset.  */

  hd[hnd].depth_offset = open_args->offset;
  hd[hnd].depth_scale = open_args->scale;


  /*  Additions from Jeff Parker (SAIC).  */

  hd[hnd].head.proj_data.projection = open_args->head.proj_data.projection;
  hd[hnd].head.proj_data.zone = open_args->head.proj_data.zone;
  hd[hnd].head.proj_data.hemisphere = open_args->head.proj_data.hemisphere;


  /*  Fix from Danny Neville (IVS).  */

  for (i = 0 ; i < NUM_ATTR ; i++)
    {
      hd[hnd].head.bin_attr_scale[i] = open_args->head.bin_attr_scale[i];
      hd[hnd].head.bin_attr_null[i]  = open_args->head.bin_attr_null[i];
      hd[hnd].bin_attr_max[i]        = open_args->bin_attr_max[i];
      hd[hnd].bin_attr_offset[i]     = open_args->bin_attr_offset[i];
      hd[hnd].head.ndx_attr_scale[i] = open_args->head.ndx_attr_scale[i];
      hd[hnd].head.max_ndx_attr[i]   = open_args->head.max_ndx_attr[i];
      hd[hnd].head.min_ndx_attr[i]   = open_args->head.min_ndx_attr[i];
    }


  /*********************************************************************************************\

      The "_bits" values in the following section will only be used if we're creating a new PFM
      structure.  Don't get wrapped around the axle if you are opening a preexisting PFM and these
      numbers are wildly wrong.  They get recomputed in read_bin_header if this isn't a new
      structure.  Obviously, I've been wrapped around the axle by this a number of times or I
      wouldn't have added this comment.   JCD ;-)

  \*********************************************************************************************/


  /*  No scale value set for errors means no error values will be stored.  */

  if (open_args->head.horizontal_error_scale > 0.00001)
    {
      hd[hnd].head.horizontal_error_scale = open_args->head.horizontal_error_scale;
      hd[hnd].head.max_horizontal_error = open_args->head.max_horizontal_error;

      temp = open_args->head.max_horizontal_error + 1.0;
      hd[hnd].horizontal_error_bits = NINT (log10 ((double) ((temp) * open_args->head.horizontal_error_scale)) / log10 (2.0) + 0.5);
    }

  if (open_args->head.vertical_error_scale > 0.00001)
    {
      hd[hnd].head.vertical_error_scale = open_args->head.vertical_error_scale;
      hd[hnd].head.max_vertical_error = open_args->head.max_vertical_error;

      temp = open_args->head.max_vertical_error + 1.0;
      hd[hnd].vertical_error_bits = NINT (log10 ((double) ((temp) * open_args->head.vertical_error_scale)) / log10 (2.0) + 0.5);
    }


  /*  Adding 1 to max_depth gives us a NULL value to be stored in the bin if all of the filtered data is deleted.  Using this value plus
      the offset we can compute the max number of bits needed to store a depth.  */

  temp = open_args->max_depth + open_args->offset + 1.0;
  hd[hnd].depth_bits = NINT (log10 ((double) ((temp) * open_args->scale)) / log10 (2.0) + 0.5);

  open_args->head.null_depth = open_args->max_depth + 1.0;
  hd[hnd].head.null_depth = open_args->head.null_depth;


  /*  Set the attribute bit field sizes based on the input min, max, and scale.  */

  for (i = 0 ; i < NUM_ATTR ; i++)
    {
      temp = (open_args->bin_attr_max[i] + open_args->bin_attr_offset[i]) + 1.0;

      if (open_args->bin_attr_max[i] != fabs(open_args->bin_attr_offset[i]))
        {
          hd[hnd].bin_attr_bits[i] = NINT (log10 ((double) ((temp) * open_args->head.bin_attr_scale[i])) / log10 (2.0) + 0.5);
        }
      else
        {
          hd[hnd].bin_attr_bits[i] = 0;
        }

      temp = (open_args->head.max_ndx_attr[i] - open_args->head.min_ndx_attr[i]) + 1.0;
      if (open_args->head.max_ndx_attr[i] != open_args->head.min_ndx_attr[i])
        {
          hd[hnd].ndx_attr_bits[i] = NINT (log10 ((double) ((temp) * open_args->head.ndx_attr_scale[i])) / log10 (2.0) + 0.5);
        }
      else
        {
          hd[hnd].ndx_attr_bits[i] = 0;
        }
      if (open_args->head.bin_attr_scale[i] > 0.00001)
        {
          open_args->head.bin_attr_null[i] = open_args->bin_attr_max[i] + 1.0;
        }
      else
        {
          open_args->head.bin_attr_null[i] = 0.0;
        }
    }

  /*********************************************************************************************/


  /*  Set the coverage map address.  */

  hd[hnd].coverage_map_address = (int64_t) 0;


  /*  Create the line file path name from the list file name.  This is hard-wired to be the list file name with a .lin extension.  Check for
      post 4.5 .ctl extension on list file.  */

  if (!strcmp (&dir_list_path[strlen (dir_list_path) - 4], ".ctl"))
    {
      strcpy (line_path, dir_list_path);
      sprintf (&line_path[strlen (line_path) - 4], ".lin");
    }
  else
    {
      sprintf (line_path, "%s.lin", dir_list_path);
    }


  /*  Check to see if the file exists.  */

  if ((temp_fp = fopen (dir_list_path, "r")) == NULL)
    {
      status = create_list_file (hnd, dir_list_path, open_args->bin_path, open_args->index_path, open_args->image_path, open_args->target_path);

      if (status)
        {
          pfm_error = status;
          return (-1);
        }


      line_file_open = create_line_file (hnd, line_path);

      new = NVTrue;
    }
  else
    {
      fclose (temp_fp);

      status = open_list_file (hnd, dir_list_path, open_args->bin_path, open_args->index_path, open_args->image_path, open_args->target_path);

      if (status)
        {
          pfm_error = status;
          if (pfm_error != CHECK_INPUT_FILE_OPEN_ERROR && pfm_error != CHECK_INPUT_FILE_WRITE_ERROR) return (-1);
        }


      line_file_open = open_line_file (hnd, line_path);
    }


  status = open_bin (hnd, open_args->bin_path, &open_args->head);

  if (status)
    {
      pfm_error = status;
      pfm_hnd[hnd] = -1;
      if (!line_file_open) fclose (line_file_fp[hnd]);
      fclose (list_file_fp[hnd]);
      return (-1);
    }


  /*  Make sure we set max_depth, offset, and scale to pass back to the caller.  This is used for appending to a PFM file or subsetting.  */

  open_args->offset = hd[hnd].depth_offset;
  open_args->max_depth = hd[hnd].head.null_depth - 1.0;
  open_args->scale = hd[hnd].depth_scale;


  /*  Also have to pass back the bin attribute information.  */

  for (i = 0 ; i < NUM_ATTR ; i++)
    {
      open_args->bin_attr_offset[i] = hd[hnd].bin_attr_offset[i];
      open_args->bin_attr_max[i] = hd[hnd].bin_attr_max[i];
    }


  /*  Open/create the index file.  */

  status = open_index (hnd, open_args->index_path);

  if (status)
    {
      pfm_error = status;
      pfm_hnd[hnd] = -1;
      if (!line_file_open) fclose (line_file_fp[hnd]);
      fclose (list_file_fp[hnd]);
      return (-1);
    }


  strcpy (list_path[hnd], dir_list_path);


  /*  Set the null values for horizontal and vertical error based on the number of bits used to store them.  This maximizes use of the error
      field bits.  */

  if (hd[hnd].horizontal_error_bits) hd[hnd].horizontal_error_null = pow (2.0, (double) hd[hnd].horizontal_error_bits) - 1.0;
  if (hd[hnd].vertical_error_bits) hd[hnd].vertical_error_null = pow (2.0, (double) hd[hnd].vertical_error_bits) - 1.0;


  /*  Set up the checkpoint file if this is not a new PFM structure and the checkpoint flag is set.  */

  if (!new && open_args->checkpoint == 1 && open_args->checkpoint != 3) checkpoint_pfm_file (hnd);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif

  return (hnd);
}



/***************************************************************************/
/*!

  - Module Name:        open_existing_pfm_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       February 2005

  - Purpose:            Opens all of the associated files.

  - Restrictions:       This function will try to open an existing PFM
                        directory or list file.  If the specified directory
                        or list file does not exist an error will be
                        returned.  This funtion should be used in most
                        cases.  Use open_pfm_file for loaders only.  Make
                        sure that you set open_args.checkpoint to 0.  All
                        of the other parts of the open_args structure
                        shouldn't matter.

  - Arguments:
                        - open_args       =   see pfm.h

  - Return Value:
                        - PFM file handle or -1 on error (error status stored
                          in pfm_error, can be printed with pfm_error_exit)
                        - Possible error status :                             *
                            - OPEN_EXISTING_PFM_ENOENT_ERROR
                            - OPEN_EXISTING_PFM_LIST FILE_ERROR
                            - OPEN_LIST_FILE_OPEN_ERROR
                            - OPEN_LIST_FILE_READ_VERSION_ERROR
                            - OPEN_LIST_FILE_READ_BIN_ERROR
                            - OPEN_LIST_FILE_READ_INDEX_ERROR
                            - OPEN_LIST_FILE_READ_IMAGE_ERROR
                            - OPEN_LIST_FILE_CORRUPTED_FILE_ERROR
                            - OPEN_BIN_OPEN_ERROR
                            - OPEN_BIN_HEADER_CORRUPT_ERROR
                            - WRITE_BIN_HEADER_EXCEEDED_MAX_POLY
                            - OPEN_INDEX_OPEN_ERROR
                            - CHECK_INPUT_FILE_OPEN_ERROR
                            - CHECK_INPUT_FILE_WRITE_ERROR

  - Warning:            This function is not thread safe.

****************************************************************************/

int32_t open_existing_pfm_file (PFM_OPEN_ARGS *open_args)
{
  FILE                *fp, *handle_fp;
  char                dir_list_path[2048], file[512], string[2048];


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif


  /*  As of version 4.6 we put all of the PFM "files" into a single directory with a .pfm file associated with it.  The naming convention
      calls for a .pfm file associated with the directory.  The .pfm file will not be the list file but will contain a version string.  This will
      keep things simple for people who are used to opening a single .pfm list file to open the structure.  As an example of the naming convention,
      if you have a file called fred.pfm you will have a directory called fred.pfm.data.  */

  strcpy (dir_list_path, open_args->list_path);


  if ((handle_fp = fopen (open_args->list_path, "r")) == NULL)
    {
      sprintf (pfm_err_str, "%s does not exist", open_args->list_path);
      return (pfm_error = OPEN_EXISTING_PFM_ENOENT_ERROR);
    }

  if (fgets (string, sizeof (string), handle_fp) == NULL)
    {
      sprintf (pfm_err_str, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      fclose (handle_fp);
      return (pfm_error = GCC_IGNORE_RETURN_VALUE_WARNING_ERROR);
    }

  if (strstr (string, "PFM Handle File")) sprintf (dir_list_path, "%s.data%1c%s.ctl", open_args->list_path, SEPARATOR, pfm_basename (open_args->list_path));

  fclose (handle_fp);


  if ((fp = fopen (dir_list_path, "r")) == NULL)
    {
      sprintf (pfm_err_str, "Unable to open %s, %s", open_args->list_path, strerror (errno));
      return (pfm_error = OPEN_EXISTING_PFM_LIST_FILE_OPEN_ERROR);
    }


  /*  Read the PFM Software version number from the first line of the list file.  If it is corrupted, error out.  */

  if (pfm_ngets (file, sizeof (file), fp) == NULL)
    {
      sprintf (pfm_err_str, "Error reading version number from list file %s", dir_list_path);
      return (pfm_error = OPEN_LIST_FILE_READ_VERSION_ERROR);
    }

  if (strstr (file, "Software - PFM I/O library V") == NULL)
    {
      sprintf (pfm_err_str, "Version string in list file %s in error", dir_list_path);
      return (pfm_error = OPEN_LIST_FILE_READ_VERSION_ERROR);
    }

  fclose (fp);


  open_args->max_depth = 0.0;
  open_args->offset = 0.0;
  open_args->scale = 0.0;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif


  return (open_pfm_file (open_args));
}



/***************************************************************************/
/*!

  - Module Name:        close_pfm_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Closes all associated files.  Also, flushes
                        buffers.

  - Arguments:
                        - hnd           =   PFM file handle

  - Return Value:
                        - void

  - Warning:            This function is not thread safe.

****************************************************************************/

void close_pfm_file (int32_t hnd)
{
  char            chk_file[512];


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Free any pfm_geo_distance memory that was allocated.  */

  if (geo_distance[hnd] != NULL) free (geo_distance[hnd]);
  if (geo_post[hnd] != NULL) free (geo_post[hnd]);
  geo_dist_init[hnd] = NVFalse;


  if (list_file_fp[hnd] != (FILE *) NULL)
    {
      fclose (list_file_fp[hnd]);
      list_file_fp[hnd] = (FILE *) NULL;
    }

  if (line_file_fp[hnd] != (FILE *) NULL)
    {
      fclose (line_file_fp[hnd]);
      line_file_fp[hnd] = (FILE *) NULL;
    }

  close_bin (hnd);

  close_index (hnd);


  /*  Remove the checkpoint file.  */

  sprintf (chk_file, "%s.chk", list_path[hnd]);
  remove (chk_file);


  /*  Clear the handle so it can be reused.  */

  pfm_hnd[hnd] = -1;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        read_list_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Reads a file name from the file list file.  If the
                        file is marked as deleted, a - is placed in the
                        first character of the string (an * is returned as
                        the first character of the filename if this is the
                        case).

  - Arguments:
                        - hnd             =   PFM file handle
                        - file_number     =   Number of the entry in the file
                        - path            =   Pathname of file
                        - type            =   Data type (defined in pfm.h)

  - Return Value:
                        - SUCCESS
                        - READ_LIST_FILE_READ_NAME_ERROR

****************************************************************************/

int32_t read_list_file (int32_t hnd, int16_t file_number, char *path, int16_t *type)
{
  char             string[512], dummy_char;
  int16_t          dummy_int;
  int32_t          nskip, slen, curpos;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  fseek (list_file_fp[hnd], list_file_index[hnd][file_number], SEEK_SET);


  if ((pfm_ngets (string, 512, list_file_fp[hnd])) == NULL)
    {
      sprintf (pfm_err_str, "Error reading list file");
      return (pfm_error = READ_LIST_FILE_READ_NAME_ERROR);
    }


  sscanf (string, "%1c %hd %hd %s", &dummy_char, &dummy_int, type, path);


  /*  IVS fix for whitespace in file names.  */

  nskip  = 2;                   /* Number of parameters to skip! */
  slen   = strlen (string);     /* Length of the string          */
  curpos = 2;                   /* Start after the 1st read!     */

  while (nskip > 0 && curpos < slen)
    {
      /* Skip the non-whitespace */

      while (curpos < slen && string[curpos] != ' ' && string[curpos] != '\t') curpos++;


      /* Skip the white-space    */

      while (curpos < slen && (string[curpos] == ' ' || string[curpos] == '\t')) curpos++;

      nskip--;


      /* Stop processing if we hit the end of the string */

      if (curpos >= slen) nskip = 0;
    }

  strcpy (path, &(string[curpos]));


  /*  Remove non-printing end of line.  */

  if (path[strlen (path) - 1] == '\r' || path[strlen (path) - 1] == '\n') path[strlen (path) - 1] = '\0';

  if (dummy_char == '-') path[0] = '*';


  /*  Perform path substitution if needed.  */

  if (substitute_cnt[hnd]) pfm_substitute (hnd, path);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        write_list_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a file name to the end of the file list
                        file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - path            =   Pathname of file
                        - type            =   Data type (defined in pfm.h)

  - Return Value:
                        - int16_t         =   File number in the file list

****************************************************************************/

int16_t write_list_file (int32_t hnd, char *path, int16_t type)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  fseek (list_file_fp[hnd], 0, SEEK_END);


  fprintf (list_file_fp[hnd], "+ %05d %02d %s\n", list_file_count[hnd], type, path);


  list_file_index[hnd][list_file_count[hnd]] = ftell (list_file_fp[hnd]);
  list_file_count[hnd]++;

  fflush (list_file_fp[hnd]);

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (list_file_count[hnd] - 1);
}



/***************************************************************************/
/*!

  - Module Name:        delete_list_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2000

  - Purpose:            Marks a file name as deleted (- in first column) in
                        the list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - file_number     =   File number in the file list

  - Return Value:
                        - void

****************************************************************************/

void delete_list_file (int32_t hnd, int32_t file_number)
{
  fseek (list_file_fp[hnd], list_file_index[hnd][file_number], SEEK_SET);

  fwrite ("-", 1, 1, list_file_fp[hnd]);
}



/***************************************************************************/
/*!

  - Module Name:        restore_list_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2000

  - Purpose:            Unmarks a file name as deleted (+ in first column)
                        in the list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - file_number     =   File number in the file list

  - Return Value:
                        - void

****************************************************************************/

void restore_list_file (int32_t hnd, int32_t file_number)
{
  fseek (list_file_fp[hnd], list_file_index[hnd][file_number], SEEK_SET);


  fwrite ("+", 1, 1, list_file_fp[hnd]);
}



/***************************************************************************/
/*!

  - Module Name:        update_target_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2000

  - Purpose:            Changes the target (feature) file name in the PFM
                        list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - lst             =   PFM list file name (NOT USED)
                        - path            =   Pathname of target file

  - Return Value:
                        - void

****************************************************************************/

void update_target_file (int32_t hnd, char *lst, char *path)
{
  int32_t             i;
  char                string[512], backup[512], dummy[4][512];
  FILE                *fp;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  First, create the backup name.  */

  sprintf (backup, "%s.bak", list_path[hnd]);


  /*  Remove the backup file if it already exists.  */

  remove (backup);


  /*  Open the backup file.  */

  if ((fp = fopen (backup, "w+")) == NULL) return;


  /*  Write the backup file.  */

  fseek (list_file_fp[hnd], 0, SEEK_SET);
  while (fgets (string, sizeof (string), list_file_fp[hnd]) != NULL) fprintf (fp, "%s", string);


  /*  Close the list file.  */

  fclose (list_file_fp[hnd]);


  /*  Remove the list file.  */

  remove (list_path[hnd]);


  /*  Open the new list file.  */

  if ((list_file_fp[hnd] = fopen (list_path[hnd], "w+")) == NULL)
    {
      fprintf (stderr, "Unrecoverable error - we can't re-open the list file %s!\n", list_path[hnd]);
      fprintf (stderr, "Please rename the backup file %s to %s to correct the problem!\n", backup, list_path[hnd]);
      exit (-1);
    }


  /*  Rewind the backup file and read/write the first 4 records.  */

  fseek (fp, 0, SEEK_SET);
  for (i = 0 ; i < 4 ; i++)
    {
      if (fgets (string, sizeof (string), fp) == NULL)
	{
	  fprintf (stderr, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
	  exit (-1);
	}
      fprintf (list_file_fp[hnd], "%s", string);
    }


  /*  Write the new target file name.  */

  if (fgets (string, sizeof (string), fp) == NULL)
    {
      fprintf (stderr, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      exit (-1);
    }
  fprintf (list_file_fp[hnd], "%s\n", path);


  /*  Copy the rest of the records.  */

  while (fgets (string, sizeof (string), fp) != NULL)
    {
      fprintf (list_file_fp[hnd], "%s", string);
    }


  /*  Close the backup file and the list file.  */

  fclose (fp);
  fclose (list_file_fp[hnd]);


  /*  Finally, re-open the new list file.  */

  open_list_file (hnd, list_path[hnd], dummy[0], dummy[1], dummy[2], dummy[3]);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        update_mosaic_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       March 2003, cloned from update_target_file by
                        J. Parker.

  - Purpose:            Changes the mosaic file name in the PFM list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - lst             =   PFM list file name (NOT USED)
                        - path            =   Pathname of mosaic file

  - Return Value:
                        - void

****************************************************************************/

void update_mosaic_file (int32_t hnd, char *lst, char *path)
{
  int32_t             i;
  char                string[512], backup[512], dummy[4][512];
  FILE                *fp;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  First, create the backup name.  */

  sprintf (backup, "%s.bak", list_path[hnd]);


  /*  Remove the backup file if it already exists.  */

  remove (backup);


  /*  Open the backup file.  */

  if ((fp = fopen (backup, "w+")) == NULL) return;


  /*  Write the backup file.  */

  fseek (list_file_fp[hnd], 0, SEEK_SET);
  while (fgets (string, sizeof (string), list_file_fp[hnd]) != NULL) fprintf (fp, "%s", string);


  /*  Close the list file.  */

  fclose (list_file_fp[hnd]);


  /*  Remove the list file.  */

  remove (list_path[hnd]);


  /*  Open the new list file.  */

  if ((list_file_fp[hnd] = fopen (list_path[hnd], "w+")) == NULL)
    {
      fprintf (stderr, "Unrecoverable error - we can't re-open the list file %s!\n", list_path[hnd]);
      fprintf (stderr, "Please rename the backup file %s to %s to correct the problem!\n", backup, list_path[hnd]);
      exit (-1);
    }


  /*  Rewind the backup file and read/write the first 3 records.  */

  fseek (fp, 0, SEEK_SET);
  for (i = 0 ; i < 3 ; i++)
    {
      if (fgets (string, sizeof (string), fp) == NULL)
	{
	  fprintf (stderr, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
	  exit (-1);
	}
      fprintf (list_file_fp[hnd], "%s", string);
    }


  /*  Write the new mosaic file name.  */

  if (fgets (string, sizeof (string), fp) == NULL)
    {
      fprintf (stderr, "Bad return from file %s function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      exit (-1);
    }
  fprintf (list_file_fp[hnd], "%s\n", path);


  /*  Copy the rest of the records.  */

  while (fgets (string, sizeof (string), fp) != NULL)
    {
      fprintf (list_file_fp[hnd], "%s", string);
    }


  /*  Close the backup file and the list file.  */

  fclose (fp);
  fclose (list_file_fp[hnd]);


  /*  Finally, re-open the new list file.  */

  open_list_file (hnd, list_path[hnd], dummy[0], dummy[1], dummy[2], dummy[3]);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        get_target_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2000

  - Purpose:            Reads the target file name from the PFM list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - lst             =   PFM list file name (NOT USED)
                        - path            =   Pathname of target file

  - Return Value:
                        - void

****************************************************************************/

void get_target_file (int32_t hnd, char *lst, char *path)
{
  char                dummy[3][512];


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Close the list file and reopen it.  */

  fclose (list_file_fp[hnd]);
  open_list_file (hnd, list_path[hnd], dummy[0], dummy[1], dummy[2], path);


  /*  Perform path substitution if needed.  */

  if (substitute_cnt[hnd]) pfm_substitute (hnd, path);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        get_mosaic_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2000

  - Purpose:            Reads the mosaic file name from the PFM list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - lst             =   PFM list file name (NOT USED)
                        - path            =   Pathname of mosaic file

  - Return Value:
                        - void

****************************************************************************/

void get_mosaic_file (int32_t hnd, char *lst, char *path)
{
  char                dummy[3][512];


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Close the list file and reopen it.  */

  fclose (list_file_fp[hnd]);
  open_list_file (hnd, list_path[hnd], dummy[0], dummy[1], path, dummy[2]);


  /*  Perform path substitution if needed.  */

  if (substitute_cnt[hnd]) pfm_substitute (hnd, path);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        get_next_list_file_number

  - Programmer(s):      Jan C. Depner

  - Date Written:       September 1999

  - Purpose:            Returns the next available input file number in the
                        list file.

  - Arguments:
                        - hnd             =   PFM file handle

  - Return Value:
                        - int16_t         =   File number in the file list

****************************************************************************/

int16_t get_next_list_file_number (int32_t hnd)
{
  return (list_file_count[hnd]);
}



/***************************************************************************/
/*!

  - Module Name:        get_next_line_number

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 2001

  - Purpose:            Returns the next available input line number in the
                        line file.

  - Arguments:
                        - hnd             =   PFM file handle

  - Return Value:
                        - int16_t         =   File number in the file list

****************************************************************************/

int16_t get_next_line_number (int32_t hnd)
{
  return (line_file_count[hnd]);
}



/***************************************************************************/
/*!

  - Module Name:        unpack_bin_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       December 2000

  - Purpose:            Unpacks a bin record from a given unsigned char
                        array.

  - Arguments:
                        - hnd             =   PFM file handle
                        - buffer          =   address of array
                        - bin             =   address of returned bin record

  - Return Value:
                        - void

****************************************************************************/

static void unpack_bin_record (int32_t hnd, uint8_t *buffer, BIN_RECORD *bin)
{
  int32_t      i;


  bin->num_soundings = pfm_bit_unpack (buffer, bin_off[hnd].num_soundings_pos, hd[hnd].count_bits);

  bin->standard_dev = (float) (pfm_bit_unpack (buffer, bin_off[hnd].std_pos, hd[hnd].std_bits)) / hd[hnd].std_scale;

  bin->avg_filtered_depth = (float) (pfm_bit_unpack (buffer, bin_off[hnd].avg_filtered_depth_pos, hd[hnd].depth_bits)) /
    hd[hnd].depth_scale - hd[hnd].depth_offset;

  bin->min_filtered_depth = (float) (pfm_bit_unpack (buffer, bin_off[hnd].min_filtered_depth_pos, hd[hnd].depth_bits)) /
    hd[hnd].depth_scale - hd[hnd].depth_offset;

  bin->max_filtered_depth = (float) (pfm_bit_unpack (buffer, bin_off[hnd].max_filtered_depth_pos, hd[hnd].depth_bits)) /
    hd[hnd].depth_scale - hd[hnd].depth_offset;

  bin->avg_depth = (float) (pfm_bit_unpack (buffer, bin_off[hnd].avg_depth_pos, hd[hnd].depth_bits)) /
    hd[hnd].depth_scale - hd[hnd].depth_offset;

  bin->min_depth = (float) (pfm_bit_unpack (buffer, bin_off[hnd].min_depth_pos, hd[hnd].depth_bits)) /
    hd[hnd].depth_scale - hd[hnd].depth_offset;

  bin->max_depth = (float) (pfm_bit_unpack (buffer, bin_off[hnd].max_depth_pos, hd[hnd].depth_bits)) /
    hd[hnd].depth_scale - hd[hnd].depth_offset;


  /*  V7.0 and later version dependency  */

  if (list_file_ver[hnd] >= 70) bin->num_valid = pfm_bit_unpack (buffer, bin_off[hnd].num_valid_pos, hd[hnd].count_bits);


  for (i = 0 ; i < hd[hnd].head.num_bin_attr ; i++)
    {
      bin->attr[i] = (float) ((double) (pfm_bit_unpack (buffer, bin_off[hnd].attr_pos[i], hd[hnd].bin_attr_bits[i])) /
                              (double) hd[hnd].head.bin_attr_scale[i] - (double) hd[hnd].bin_attr_offset[i]);
    }

  bin->validity = pfm_bit_unpack (buffer, bin_off[hnd].validity_pos, hd[hnd].validity_bits);

  bin->depth_chain.head = bin_record_head_pointer[hnd] = pfm_double_bit_unpack (buffer, bin_off[hnd].head_pointer_pos,
                                                                             hd[hnd].record_pointer_bits);

  bin->depth_chain.tail = bin_record_tail_pointer[hnd] = pfm_double_bit_unpack (buffer, bin_off[hnd].tail_pointer_pos,
                                                                             hd[hnd].record_pointer_bits);


  /*  Compute the position of the center of the bin.  */

  if (!hd[hnd].head.proj_data.projection)
    {
      bin->xy.y = bin_header[hnd].mbr.min_y + bin->coord.y * bin_header[hnd].y_bin_size_degrees + bin_header[hnd].y_bin_size_degrees * 0.5;
      bin->xy.x = bin_header[hnd].mbr.min_x + bin->coord.x * bin_header[hnd].x_bin_size_degrees + bin_header[hnd].x_bin_size_degrees * 0.5;
    }
  else
    {
      bin->xy.y = bin_header[hnd].mbr.min_y + bin->coord.y * bin_header[hnd].bin_size_xy + bin_header[hnd].bin_size_xy * 0.5;
      bin->xy.x = bin_header[hnd].mbr.min_x + bin->coord.x * bin_header[hnd].bin_size_xy + bin_header[hnd].bin_size_xy * 0.5;
    }
}



/***************************************************************************/
/*!

  - Module Name:        read_bin_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Reads a bin record from the bin/index file.  This
                        function is only used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   BIN_RECORD structure
                        - address         =   Address of record

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

static int32_t read_bin_record (int32_t hnd, BIN_RECORD *bin, int64_t address)
{
#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  If we need to read a different record...  */

  if (address != previous_bin_address[hnd])
    {
      /*  Check to see if the bin buffer has been modified.  If so, write it out.  */

      if (bin_record_modified[hnd]) write_bin_buffer (hnd, bin_record_address[hnd]);


      /*  Save the address of the record read.  */

      bin_record_address[hnd] = address;


      /*  Read the record.  */

      lfseek (bin_file_hnd[hnd], address, SEEK_SET);

      if (!(lfread (bin_record_data[hnd], bin_off[hnd].record_size, 1, bin_file_hnd[hnd])))
        {
          sprintf (pfm_err_str, "Error reading from bin file");
          return (pfm_error = READ_BIN_RECORD_DATA_READ_ERROR);
        }
    }


  /*  If the address has changed (read a new record), or the buffer was modified, unpack the record from the buffer.  */

  if (address != previous_bin_address[hnd] || bin_record_modified[hnd])
    {
      unpack_bin_record (hnd, bin_record_data[hnd], bin);
    }


  /*  If you didn't unpack it put the last one you read into the BIN_RECORD that was passed in.  */

  else
    {
      *bin = bin_record[hnd];
    }


  /*  Save the previous address for the bin_record.  */

  previous_bin_address[hnd] = address;


  /*  Save the bin_record contents.  */

  bin_record[hnd] = *bin;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_bin_record_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Reads a bin record from the bin/index file at index
                        coord.

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   X and Y index values
                        - bin             =   BIN_RECORD structure

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

int32_t read_bin_record_index (int32_t hnd, NV_I32_COORD2 coord, BIN_RECORD *bin)
{
  int64_t             address;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  address = ((int64_t) coord.y * (int64_t) bin_header[hnd].bin_width + coord.x) * (int64_t) bin_off[hnd].record_size + hd[hnd].bin_header_size;


  /*  Save the x and y indices.   */

  bin->coord.x = coord.x;
  bin->coord.y = coord.y;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = read_bin_record (hnd, bin, address));
}



/***************************************************************************/
/*!

  - Module Name:        read_bin_row

  - Programmer(s):      Jan C. Depner

  - Date Written:       May 1999

  - Purpose:            Reads a row of bin records from the bin/index file
                        of length "length", at row "row", and column
                        "column".  This data is placed into BIN_RECORD
                        array "a".

  - Arguments:
                        - hnd             =   PFM file handle
                        - length          =   number of columns to read
                        - row             =   row
                        - column          =   starting column
                        - a               =   array of bin records

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

int32_t read_bin_row (int32_t hnd, int32_t length, int32_t row, int32_t column, BIN_RECORD a[])
{
  int32_t             i, j, size, position;
  uint8_t             *buffer;
  int64_t             address;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if (bin_record_modified[hnd])
    {
      write_bin_buffer (hnd, bin_record_address[hnd]);
      bin_record_modified[hnd] = NVFalse;
    }


  if (depth_record_modified[hnd])
    {
      write_depth_buffer (hnd, depth_record_address[hnd]);
      depth_record_modified[hnd] = NVFalse;
    }


  address = ((int64_t) row * (int64_t) bin_header[hnd].bin_width + column) * (int64_t) bin_off[hnd].record_size + hd[hnd].bin_header_size;

  size = length * bin_off[hnd].record_size;


  buffer = (uint8_t *) malloc (size);

  if (buffer == NULL)
    {
      sprintf (pfm_err_str, "Allocating memory in read_bin_row");
      return (pfm_error = READ_BIN_RECORD_DATA_READ_ERROR);
    }


  /*  Read the row.  */

  lfseek (bin_file_hnd[hnd], address, SEEK_SET);

  lfread (buffer, size, 1, bin_file_hnd[hnd]);

  for (i = column, j = 0 ; i < column + length ; i++, j++)
    {
      a[j].coord.x = i;
      a[j].coord.y = row;

      position = j * bin_off[hnd].record_size;

      unpack_bin_record (hnd, &buffer[position], &a[j]);
    }


  /* GS: Make sure system knows the last bin that loaded into memory. */

  previous_bin_address[hnd] = address + (int64_t) bin_off[hnd].record_size * (length - 1);


  free (buffer);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_bin_record_validity_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       September 1999

  - Purpose:            Reads a bin record from the bin/index file at index
                        coord but only unpacks and returns the validity.

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   X and Y index values
                        - validity        =   what else?

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

int32_t read_bin_record_validity_index (int32_t hnd, NV_I32_COORD2 coord, uint32_t *validity)
{
  int64_t             address;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  address = ((int64_t) coord.y * (int64_t) bin_header[hnd].bin_width + coord.x) * (int64_t) bin_off[hnd].record_size + hd[hnd].bin_header_size;


  /*  Read the record.  */

  lfseek (bin_file_hnd[hnd], address, SEEK_SET);

  if (!(lfread (bin_record_data[hnd], bin_off[hnd].record_size, 1, bin_file_hnd[hnd])))
    {
      sprintf (pfm_err_str, "Error reading bin file flags");
      return (pfm_error = READ_BIN_RECORD_DATA_READ_ERROR);
    }


  *validity = pfm_bit_unpack (bin_record_data[hnd], bin_off[hnd].validity_pos, hd[hnd].validity_bits);


  /*  Reset the previous bin record address to force a bin read.  */

  previous_bin_address[hnd] = -1;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_cov_map_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 2003

  - Purpose:            Reads a coverage map record from the end of the bin
                        file (at index coord).

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   X and Y index values
                        - cov             =   cov map flags (see pfm.h)

  - Return Value:
                        - SUCCESS

****************************************************************************/

int32_t read_cov_map_index (int32_t hnd, NV_I32_COORD2 coord, uint8_t *cov)
{
  int64_t             address;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /* if the row has changed. */

  if (coord.y != read_cov_map_index_prev_row[hnd])
    {
      /* if the number of columns in the row has changed. */

      if (bin_header[hnd].bin_width != read_cov_map_index_prev_num_cols[hnd])
        {
          if (read_cov_map_index_row[hnd]) free (read_cov_map_index_row[hnd]);
          read_cov_map_index_row[hnd] = (uint8_t *) malloc (bin_header[hnd].bin_width * sizeof (uint8_t));
          read_cov_map_index_prev_num_cols[hnd] = bin_header[hnd].bin_width;
        }
      else
        {
          /* else just set the buffer to 0. */

          if (read_cov_map_index_row[hnd] != NULL) memset (read_cov_map_index_row[hnd], 0, bin_header[hnd].bin_width);
        }


      /*  Pre v7 version dependency  */

      if (list_file_ver[hnd] < 70)
        {
          /* Seems to be needed in some cases to prevent corruption of very large PFMs */
          /* Fri Aug 13 04:35:15 2004 -- Webb McDonald (SAIC) */

          address =  (int64_t) hd[hnd].coverage_map_address + (int64_t) coord.y * (int64_t) bin_header[hnd].bin_width;


          /*  Read the row.  */

          lfseek (bin_file_hnd[hnd], address, SEEK_SET);

          lfread (read_cov_map_index_row[hnd], 1, bin_header[hnd].bin_width, bin_file_hnd[hnd]);
        }
      else
        {
          address = (int64_t) coord.y * (int64_t) bin_header[hnd].bin_width + (int64_t) COV_HEADER_SIZE;


          /*  Read the row.  */

          fseeko64 (cov_file_fp[hnd], address, SEEK_SET);

          if (!fread (read_cov_map_index_row[hnd], 1, bin_header[hnd].bin_width, cov_file_fp[hnd]))
            {
              sprintf (pfm_err_str, "Reading coverage file row in read_cov_map_index");
              return (pfm_error = READ_COV_MAP_INDEX_READ_ERROR);
            }
        }
    }


  *cov = read_cov_map_index_row[hnd][coord.x];


  /*  Set the previous address.  */

  read_cov_map_index_prev_row[hnd] = coord.y;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/****************************************************************************/
/*!

  - Module Name:        write_cov_map_index

  - Programmer(s):      Webb McDonald

  - Date Written:       May 2006

  - Purpose:            Writes cov flags into the coverage map record
                        at index coord.

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   X and Y index values
                        - cov             =   cov map flags (see pfm.h)

  - Return Value:
                        - SUCCESS

*****************************************************************************/

int32_t write_cov_map_index (int32_t hnd, NV_I32_COORD2 coord, uint8_t cov)
{
  int64_t             address;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Pre v7 version dependency  */

  if (list_file_ver[hnd] < 70)
    {
      address =  (int64_t) hd[hnd].coverage_map_address + (int64_t) coord.y * (int64_t) bin_header[hnd].bin_width + (int64_t) coord.x;


      /* Write the cell. */

      lfseek (bin_file_hnd[hnd], address, SEEK_SET);

      lfwrite (&cov, 1, 1, bin_file_hnd[hnd]);
    }
  else
    {
      address = (int64_t) coord.y * (int64_t) bin_header[hnd].bin_width + (int64_t) coord.x + (int64_t) COV_HEADER_SIZE;


      /*  Write the cell.  */

      fseeko64 (cov_file_fp[hnd], address, SEEK_SET);

      fwrite (&cov, 1, 1, cov_file_fp[hnd]);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_coverage_map_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 2000

  - Purpose:            Reads a coverage map record from the end of the bin
                        file (at index coord).

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   X and Y index values
                        - data            =   DATA flag
                        - checked         =   CHECKED flag

  - Return Value:
                        - SUCCESS

  - NOTE:               This function is deprecated, please use
                        read_cov_map_index.

****************************************************************************/

int32_t read_coverage_map_index (int32_t hnd, NV_I32_COORD2 coord, uint8_t *data, uint8_t *checked)
{
  uint8_t             cov;


  read_cov_map_index (hnd, coord, &cov);

  *data = NVFalse;
  *checked = NVFalse;
  if (cov & COV_DATA) *data = NVTrue;
  if (cov & COV_CHECKED) *checked = NVTrue;

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        get_data_extents

  - Programmer(s):      J. Parker

  - Date Written:       August 2007

  - Purpose:            Reads a coverage map record from the end of the bin
                        file (at index coord) and determine min/max extents.

  - Arguments:
                        - hnd             =   PFM file handle
                        - min_coord       =   X and Y index values
                        - max_coord       =   X and Y index values

  - Return Value:
                        - SUCCESS

****************************************************************************/

int32_t get_data_extents (int32_t hnd, NV_I32_COORD2 *min_coord, NV_I32_COORD2 *max_coord)
{
  int64_t       address;
  uint8_t      *row = NULL;
  int32_t       i, j;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  /*  Pre v7 version dependency  */

  if (list_file_ver[hnd] < 70)
    {
      address =  (int64_t) hd[hnd].coverage_map_address;
      lfseek (bin_file_hnd[hnd], address, SEEK_SET);
    }
  else
    {
      fseek (cov_file_fp[hnd], COV_HEADER_SIZE, SEEK_SET);
    }

  min_coord->x = 1000000000;
  min_coord->y = 1000000000;
  max_coord->x = 0;
  max_coord->y = 0;

  row = (uint8_t *) calloc (bin_header[hnd].bin_width, sizeof (uint8_t));

  for (i = 0; i < bin_header[hnd].bin_height; i++)
    {
      /*  Pre v7 version dependency  */

      if (list_file_ver[hnd] < 70)
        {
          lfread (row, 1, bin_header[hnd].bin_width, bin_file_hnd[hnd]);
        }
      else
        {
          if (!fread (row, 1, bin_header[hnd].bin_width, cov_file_fp[hnd]))
            {
              free (row);
              sprintf (pfm_err_str, "Reading coverage file row in get_data_extents");
              return (pfm_error = READ_COV_MAP_INDEX_READ_ERROR);
            }
        }

      for (j = 0; j < bin_header[hnd].bin_width; j++)
        {
          if (row[j] & COV_DATA)
            {
              if (min_coord->x > j)  min_coord->x = j;
              if (min_coord->y > i)  min_coord->y = i;
              if (max_coord->x < j)  max_coord->x = j;
              if (max_coord->y < i)  max_coord->y = i;
            }
        }  /* for j */
      memset (row, 0, bin_header[hnd].bin_width * sizeof (uint8_t));
    }  /* for i */


  free (row);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        compute_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Computes a bin index based on an xy pair.

  - Arguments:
                        - xy              =   Position
                        - coord           =   Returned x and y indices
                        - bin             =   BIN_HEADER (passed by value,
                                              a bad idea)

  - Return Value:
                        - void

****************************************************************************/

void compute_index (NV_F64_COORD2 xy, NV_I32_COORD2 *coord, BIN_HEADER bin)
{
  compute_index_ptr (xy, coord, &bin);
}

/***************************************************************************/
/*!

  - Module Name:        compute_index_ptr

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Computes a bin index based on an xy pair.

  - Arguments:
                        - xy              =   Position
                        - coord           =   Returned x and y indices
                        - bin             =   BIN_HEADER (passed as a pointer for speed)

  - Return Value:
                        - void

****************************************************************************/

void compute_index_ptr (NV_F64_COORD2 xy, NV_I32_COORD2 *coord, BIN_HEADER *bin)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif

  if (bin->proj_data.projection)
    {
      coord->x = (xy.x - bin->mbr.min_x) / bin->bin_size_xy;
      coord->y = (xy.y - bin->mbr.min_y) / bin->bin_size_xy;
    }
  else
    {
      coord->x = (int32_t)((double)(xy.x - bin->mbr.min_x) / (double)bin->x_bin_size_degrees);
      coord->y = (int32_t)((double)(xy.y - bin->mbr.min_y) / (double)bin->y_bin_size_degrees);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        compute_center_xy

  - Programmer(s):      J. Parker

  - Date Written:       August 2007

  - Purpose:            Computes the center of the cell at coord "coord".

  - Arguments:
                        - *xy             =   Returned position
                        - coord           =   Cell coordinates
                        - bin             =   PFM bin header

  - Return Value:
                        - void

****************************************************************************/

void compute_center_xy (NV_F64_COORD2 *xy, NV_I32_COORD2 coord, BIN_HEADER *bin)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif

  if (bin->proj_data.projection)
    {
      xy->x = (coord.x * bin->bin_size_xy) + bin->mbr.min_x + (0.5 * bin->bin_size_xy);
      xy->y = (coord.y * bin->bin_size_xy) + bin->mbr.min_y + (0.5 * bin->bin_size_xy);
    }
  else
    {
      xy->x = (coord.x * (double)bin->x_bin_size_degrees) + bin->mbr.min_x + (0.5 * (double)bin->x_bin_size_degrees);
      xy->y = (coord.y * (double)bin->y_bin_size_degrees) + bin->mbr.min_y + (0.5 * (double)bin->y_bin_size_degrees);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__); fflush (stderr);
#endif
}



/***************************************************************************/
/*!

  - Module Name:        read_bin_record_xy

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Reads a bin record from the bin/index file at the
                        position x,y

  - Arguments:
                        - hnd             =   PFM file handle
                        - xy              =   x, y position
                        - bin             =   BIN_RECORD structure

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

int32_t read_bin_record_xy (int32_t hnd, NV_F64_COORD2 xy, BIN_RECORD *bin)
{
  NV_I32_COORD2       coord;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  compute_index_ptr (xy, &coord, &bin_header[hnd]);


  /*  Save the x and y indices.   */

  bin->coord.x = coord.x;
  bin->coord.y = coord.y;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = read_bin_record_index (hnd, bin->coord, bin));
}



/***************************************************************************/
/*!

  - Module Name:        write_bin_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a bin record to the bin/index file.  This
                        function is only used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   BIN_RECORD structure
                        - address         =   Address of the record

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

static int32_t write_bin_record (int32_t hnd, BIN_RECORD *bin, int64_t address)
{
  BIN_RECORD          tmpBin;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if (bin_record_modified[hnd])
    {
      write_bin_buffer (hnd, bin_record_address[hnd]);
      bin_record_modified[hnd] = NVFalse;
    }


  /*  GS: Save the address of the record to write.  */


  bin_record_address[hnd] = address;


  /*  We can use the chain pointers passed to the caller instead of having to reread the bin record as long as we haven't done an
      add_depth_record since the file was opened.  This, of course, assumes that the caller hasn't messed with the chain pointers.
      DON'T DO THAT!!!!!!!!!!  'NUFF SAID?  */

  if (use_chain_pointer[hnd])
    {
      bin_record_head_pointer[hnd] = bin->depth_chain.head;
      bin_record_tail_pointer[hnd] = bin->depth_chain.tail;
    }
  else
    {

      /*  If the record has changed since the last read, read the record to get the data record pointers.  If it hasn't changed we still
          have the pointers.  */

      if (bin_record_address[hnd] != address)
        {
          if (read_bin_record(hnd, &tmpBin, address))
            {
              sprintf (pfm_err_str, "Error reading bin record in write_bin_record");
              return (pfm_error = WRITE_BIN_RECORD_DATA_READ_ERROR);
            }


          /*  Set the bin_record_address since we just read a new buffer.  */

          bin_record_address[hnd] = address;

          bin_record_head_pointer[hnd] = pfm_double_bit_unpack (bin_record_data[hnd], bin_off[hnd].head_pointer_pos, hd[hnd].record_pointer_bits);
          bin_record_tail_pointer[hnd] = pfm_double_bit_unpack (bin_record_data[hnd], bin_off[hnd].tail_pointer_pos, hd[hnd].record_pointer_bits);
        }
      else
        {
          /*  Making sure we have the chain head and tail pointers set for pack_bin_record.  */

          bin->depth_chain.head = bin_record_head_pointer[hnd];
          bin->depth_chain.tail = bin_record_tail_pointer[hnd];
        }
    }

  pack_bin_record (hnd, bin_record_data[hnd], bin, &bin_off[hnd], &hd[hnd], list_file_ver[hnd], &bin->depth_chain);


  /*  Mark the data buffer as modified.  Should be in pack_bin_record. */

  bin_record_modified[hnd] = NVTrue;


  /* GS: Write a modified bin to disk */


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        pack_bin_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a bin record to the bin/index file.  This
                        function is only used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin_data        =   bin_data structure
                        - bin             =   BIN_RECORD structure
                        - offsets         =   bin record bit offsets
                        - hd              =   header data
                        - chain           =   Depth record chain

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

static int32_t pack_bin_record (int32_t hnd, uint8_t *bin_data, BIN_RECORD *bin, BIN_RECORD_OFFSETS *offsets, BIN_HEADER_DATA *hdPtr,
                                 int16_t list_file_ver_idx, CHAIN *depth_chain )
{
  int32_t             i;


  pfm_bit_pack (bin_data, offsets->num_soundings_pos, hdPtr->count_bits, bin->num_soundings);

  pfm_bit_pack (bin_data, offsets->std_pos, hdPtr->std_bits, NINT (bin->standard_dev * hdPtr->std_scale));

  pfm_bit_pack (bin_data, offsets->avg_filtered_depth_pos, hdPtr->depth_bits, NINT ((bin->avg_filtered_depth + hdPtr->depth_offset) * hdPtr->depth_scale));

  pfm_bit_pack (bin_data, offsets->min_filtered_depth_pos, hdPtr->depth_bits, NINT ((bin->min_filtered_depth + hdPtr->depth_offset) * hdPtr->depth_scale));

  pfm_bit_pack (bin_data, offsets->max_filtered_depth_pos, hdPtr->depth_bits, NINT ((bin->max_filtered_depth + hdPtr->depth_offset) * hdPtr->depth_scale));

  pfm_bit_pack (bin_data, offsets->avg_depth_pos, hdPtr->depth_bits, NINT ((bin->avg_depth + hdPtr->depth_offset) * hdPtr->depth_scale));

  pfm_bit_pack (bin_data, offsets->min_depth_pos, hdPtr->depth_bits, NINT ((bin->min_depth + hdPtr->depth_offset) * hdPtr->depth_scale));

  pfm_bit_pack (bin_data, offsets->max_depth_pos, hdPtr->depth_bits, NINT ((bin->max_depth + hdPtr->depth_offset) * hdPtr->depth_scale));


  /*  V7.0 and later version dependency  */

  if (list_file_ver[hnd] >= 70) pfm_bit_pack (bin_data, offsets->num_valid_pos, hdPtr->count_bits, bin->num_valid);


  for (i = 0 ; i < hdPtr->head.num_bin_attr ; i++)
    {
      pfm_bit_pack (bin_data, offsets->attr_pos[i], hdPtr->bin_attr_bits[i],
                    NINT (((double) bin->attr[i] + (double) hdPtr->bin_attr_offset[i]) * (double) hdPtr->head.bin_attr_scale[i]));
    }

  pfm_bit_pack (bin_data, offsets->validity_pos, hdPtr->validity_bits, bin->validity);


  pfm_double_bit_pack (bin_data, offsets->head_pointer_pos, hdPtr->record_pointer_bits, depth_chain->head);

  pfm_double_bit_pack (bin_data, offsets->tail_pointer_pos, hdPtr->record_pointer_bits, depth_chain->tail);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        write_bin_record_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a bin record to the bin/index file at
                        coord.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   BIN_RECORD structure

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

int32_t write_bin_record_index (int32_t hnd, BIN_RECORD *bin)
{
  int64_t             address;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  address = ((int64_t) bin->coord.y * (int64_t) bin_header[hnd].bin_width + bin->coord.x) * (int64_t) bin_off[hnd].record_size + hd[hnd].bin_header_size;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = write_bin_record (hnd, bin, address));
}



/***************************************************************************/
/*!

  - Module Name:        write_bin_record_xy

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a bin record to the bin/index file at
                        position xy.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   BIN_RECORD structure

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

int32_t write_bin_record_xy (int32_t hnd, BIN_RECORD *bin)
{
  int64_t             address;
  NV_I32_COORD2       coord;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  compute_index_ptr (bin->xy, &coord, &bin_header[hnd]);


  address = ((int64_t) coord.y * (int64_t) bin_header[hnd].bin_width + coord.x) * (int64_t) bin_off[hnd].record_size + hd[hnd].bin_header_size;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = write_bin_record (hnd, bin, address));
}



/***************************************************************************/
/*!

  - Module Name:        write_bin_record_validity_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a bin record with only the flags modified to
                        the bin file at bin->coord.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   BIN_RECORD structure
                        - mask            =   set the bits that you wish to
                                              write, for example:
                                              setting mask to PFM_DATA &
                                              PFM_CHECKED will set the
                                              PFM_DATA and PFM_CHECKED bits
                                              to whatever is stored in the
                                              passed bin record.  No change
                                              will be done to the other
                                              validity bits.

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_RECORD_VALIDITY_INDEX_ERROR

****************************************************************************/

int32_t write_bin_record_validity_index (int32_t hnd, BIN_RECORD *bin, uint32_t mask)
{
  int64_t             address;
  uint32_t            val;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  val = bin->validity;


  address = ((int64_t) bin->coord.y * (int64_t) bin_header[hnd].bin_width + bin->coord.x) * (int64_t) bin_off[hnd].record_size + hd[hnd].bin_header_size;


  if (read_bin_record(hnd, bin, address))
    {
      sprintf (pfm_err_str, "Error reading bin record in write_bin_record_validity_index");
      return (pfm_error = WRITE_BIN_RECORD_VALIDITY_INDEX_ERROR);
    }


  /*  Set the proper bits.  */

  bin->validity = (val & mask) | (bin->validity & ~mask);


  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].validity_pos, hd[hnd].validity_bits, bin->validity);


  /*  Mark the data buffer as modified.  */

  bin_record_modified[hnd] = NVTrue;


  /*  Set the previous bin record address to force a read.  */

  previous_bin_address[hnd] = -1;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_depth_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Reads a sounding record from the index/data file.
                        This function is only used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure
                        - count           =   Returns record number of the
                                              sounding, no more soundings
                                              when 0

  - Return Value:
                        - SUCCESS
                        - READ_DEPTH_RECORD_READ_ERROR
                        - READ_DEPTH_RECORD_CONTINUATION_ERROR

****************************************************************************/

static int32_t read_depth_record (int32_t hnd, DEPTH_RECORD *depth, int32_t *count)
{
  float               x_offset, y_offset;
  int32_t             i;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Increment the buffer position.  */

  depth_record_pos[hnd] += dep_off[hnd].single_point_bits;


  /*  Check for head of chain.  */

  if (bin_record[hnd].num_soundings == *count || depth_record_modified[hnd])
    {
      /*  See if we need to flush the depth buffer.  */

      if (depth_record_modified[hnd]) write_depth_buffer (hnd, depth_record_address[hnd]);

      lfseek (ndx_file_hnd[hnd], bin_record_head_pointer[hnd], SEEK_SET);

      if (!lfread (depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]))
        {
          sprintf (pfm_err_str, "Error reading depth record");
          return (pfm_error = READ_DEPTH_RECORD_READ_ERROR);
        }


      /*  Set the position within the 'physical' record buffer.  */

      depth_record_pos[hnd] = 0;


      /*  Save the depth record address read.  */

      depth_record_address[hnd] = bin_record_head_pointer[hnd];


      /* Graeme Sweet (IVS): bug fix */

      previous_depth_block[hnd] = depth_record_address[hnd];
    }
  else
    {
      /*  See if we need to move to the next record in the data chain.  */

      if (!((bin_record[hnd].num_soundings - *count) % hd[hnd].record_length))
        {
          /*  Unpack the continuation pointer.  */

          continuation_pointer[hnd] = pfm_double_bit_unpack (depth_record_data[hnd], dep_off[hnd].continuation_pointer_pos, hd[hnd].record_pointer_bits);


          /*  See if we need to flush the write buffer.  */

          if (depth_record_modified[hnd]) write_depth_buffer (hnd, depth_record_address[hnd]);


          /*  Move to the continuation pointer location.  */

          lfseek (ndx_file_hnd[hnd], continuation_pointer[hnd], SEEK_SET);

          if (!lfread (depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]))
            {
              sprintf (pfm_err_str, "Error reading depth record continuation pointer");
              return (pfm_error = READ_DEPTH_RECORD_CONTINUATION_ERROR);
            }


          /*  Set the position within the 'physical' record buffer.  */

          depth_record_pos[hnd] = 0;


          /*  Save the depth record address read.  */

          depth_record_address[hnd] = continuation_pointer[hnd];


          /* Graeme Sweet (IVS): bug fix */

          previous_depth_block[hnd] = depth_record_address[hnd];
        }
    }


  /*  Save the indices.  */

  depth->coord.x = bin_record[hnd].coord.x;
  depth->coord.y = bin_record[hnd].coord.y;


  /*  Save the block address and record position.  */

  depth->address.block = depth_record_address[hnd];
  depth->address.record = (depth_record_pos[hnd] / dep_off[hnd].single_point_bits) % hd[hnd].record_length;


  /*  Unpack the current depth record from the 'physical' record.  */

  depth->file_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].file_number_pos + depth_record_pos[hnd], hd[hnd].file_number_bits);

  depth->line_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].line_number_pos + depth_record_pos[hnd], hd[hnd].line_number_bits);

  depth->ping_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].ping_number_pos + depth_record_pos[hnd], hd[hnd].ping_number_bits);

  depth->beam_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].beam_number_pos + depth_record_pos[hnd], hd[hnd].beam_number_bits);

  depth->xyz.z = (double) (pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].depth_pos + depth_record_pos[hnd],
					       hd[hnd].depth_bits)) / (double)hd[hnd].depth_scale - (double)hd[hnd].depth_offset;
  depth->coord.x = bin_record[hnd].coord.x;
  depth->coord.y = bin_record[hnd].coord.y;


  /*  Stored as lat/lon.  */

  if (!hd[hnd].head.proj_data.projection)
    {
      x_offset = ((float) (pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].x_offset_pos + depth_record_pos[hnd], hd[hnd].offset_bits)) /
                  x_offset_scale[hnd]) * bin_header[hnd].x_bin_size_degrees;

      y_offset = ((float) (pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].y_offset_pos + depth_record_pos[hnd], hd[hnd].offset_bits)) /
                  y_offset_scale[hnd]) * bin_header[hnd].y_bin_size_degrees;


      /*  Compute the geographic position of the point. */

      depth->xyz.y = bin_header[hnd].mbr.min_y + depth->coord.y * bin_header[hnd].y_bin_size_degrees + y_offset;
      depth->xyz.x = bin_header[hnd].mbr.min_x + depth->coord.x * bin_header[hnd].x_bin_size_degrees + x_offset;
    }


  /*  Stored as x/y.  */

  else
    {
      x_offset = ((float) (pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].x_offset_pos + depth_record_pos[hnd], hd[hnd].offset_bits)) /
                  x_offset_scale[hnd]) * bin_header[hnd].bin_size_xy;

      y_offset = ((float) (pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].y_offset_pos + depth_record_pos[hnd], hd[hnd].offset_bits)) /
                  y_offset_scale[hnd]) * bin_header[hnd].bin_size_xy;


      /*  Compute the x/y position of the point. */

      depth->xyz.y = bin_header[hnd].mbr.min_y + depth->coord.y * bin_header[hnd].bin_size_xy + y_offset;
      depth->xyz.x = bin_header[hnd].mbr.min_x + depth->coord.x * bin_header[hnd].bin_size_xy + x_offset;
    }


  for (i = 0 ; i < hd[hnd].head.num_ndx_attr ; i++)
    {
      depth->attr[i] = (float) ((double) (pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].attr_pos[i] + depth_record_pos[hnd],
                                                          hd[hnd].ndx_attr_bits[i])) / (double) hd[hnd].head.ndx_attr_scale[i] +
                                (double) hd[hnd].head.min_ndx_attr[i]);
    }

  depth->validity = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].validity_pos + depth_record_pos[hnd], hd[hnd].validity_bits);

  if (hd[hnd].horizontal_error_bits)
    {
      depth->horizontal_error = (float) (pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].horizontal_error_pos + depth_record_pos[hnd],
                                                              hd[hnd].horizontal_error_bits)) / hd[hnd].head.horizontal_error_scale;
      if (depth->horizontal_error >= hd[hnd].horizontal_error_null) depth->horizontal_error = -999.0;
    }


  if (hd[hnd].vertical_error_bits)
    {
      depth->vertical_error = (float) (pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].vertical_error_pos + depth_record_pos[hnd],
                                                            hd[hnd].vertical_error_bits)) / hd[hnd].head.vertical_error_scale;
      if (depth->vertical_error >= hd[hnd].vertical_error_null) depth->vertical_error = -999.0;
    }


  /*  Decrement the counter.  */

  (*count)--;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_depth_array_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       August 2000

  - Purpose:            Reads all of the sounding records associated with
                        the bin at 'coord'.  This is how I should have done
                        it in the first place.

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   X and Y index values
                        - depth_array     =   pointer to DEPTH_RECORD array
                                              pointer
                        - numrecs         =   number of records read

  - Caveats:            The user/caller is responsible for freeing the
                        memory associated with 'depth_array' as follows:
                        <pre>

                        DEPTH_RECORD       *depth;

                                     o
                                     o
                                     o

                        read_depth_array_index (hnd, coord, &depth, recnum);

                                     o
                                     o
                                     o

                        for (i = 0 ; i < recnum ; i++)
                            DO SOMETHING WITH DATA IN depth;

                                     o
                                     o
                                     o

                        free (depth);

                        </pre>

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR
                        - READ_DEPTH_RECORD_NO_DATA
                        - READ_DEPTH_ARRAY_CALLOC_ERROR

****************************************************************************/

int32_t read_depth_array_index (int32_t hnd, NV_I32_COORD2 coord, DEPTH_RECORD **depth_array, int32_t *numrecs)
{
  int32_t                 status, recnum, i;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  status = read_bin_record_index (hnd, coord, &bin_record[hnd]);

  if (status) return (pfm_error = status);


  /*  If there is no depth data, pass back nothing.  */

  if (!bin_record[hnd].num_soundings)
    {
      sprintf (pfm_err_str, "No data in depth record being read");
      return (pfm_error = READ_DEPTH_RECORD_NO_DATA);
    }

  *numrecs = bin_record[hnd].num_soundings;
  recnum = *numrecs;

  *depth_array = (DEPTH_RECORD *) calloc (recnum, sizeof (DEPTH_RECORD));

  if (*depth_array == NULL)
    {
      sprintf (pfm_err_str, "Insufficient memory for depth array");
      return (pfm_error = READ_DEPTH_ARRAY_CALLOC_ERROR);
    }

  for (i = 0 ; i < *numrecs ; i++)
    {
      status = read_depth_record (hnd, &(*depth_array)[i], &recnum);

      if (status)
        {
          fprintf (stderr,"Bad status on depth array read at %d %d\n", coord.x, coord.y);
          fprintf (stderr,"Message: %s\n", pfm_err_str);
          /*
            fprintf (stderr,"Zeroing bin!\n");
            bin_record[hnd].num_soundings = 0;
            write_bin_record_index (hnd, &bin_record[hnd]);
          */
          return (pfm_error = status);
        }
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_depth_array_xy

  - Programmer(s):      Jan C. Depner

  - Date Written:       August 2000

  - Purpose:            Reads all of the sounding records associated with
                        the bin at 'xy'.  This is how I should have done it
                        in the first place.

  - Arguments:
                        - hnd             =   PFM file handle
                        - xy              =   position
                        - depth_array     =   pointer to DEPTH_RECORD array
                                              pointer
                        - numrecs         =   number of records read

  - Caveats:            The user/caller is responsible for freeing the
                        memory associated with 'depth_array' as follows:
                        <pre>

                        DEPTH_RECORD       *depth;

                                     o
                                     o
                                     o

                        read_depth_array_index (hnd, coord, &depth, recnum);

                                     o
                                     o
                                     o

                        for (i = 0 ; i < recnum ; i++)
                            DO SOMETHING WITH DATA IN depth;

                                     o
                                     o
                                     o

                        free (depth);

                        </pre>

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR
                        - READ_DEPTH_RECORD_NO_DATA
                        - READ_DEPTH_ARRAY_CALLOC_ERROR

****************************************************************************/

int32_t read_depth_array_xy (int32_t hnd, NV_F64_COORD2 xy, DEPTH_RECORD **depth_array, int32_t *numrecs)
{
  int32_t             status, i, recnum;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  status = read_bin_record_xy (hnd, xy, &bin_record[hnd]);

  if (status) return (pfm_error = status);


  /*  If there is no depth record, pass back nothing.  */

  if (!bin_record[hnd].num_soundings)
    {
      sprintf (pfm_err_str, "No data in depth record being read");
      return (pfm_error = READ_DEPTH_RECORD_NO_DATA);
    }

  *numrecs = bin_record[hnd].num_soundings;
  recnum = *numrecs;

  *depth_array = (DEPTH_RECORD *) calloc (recnum, sizeof (DEPTH_RECORD));

  if (*depth_array == NULL)
    {
      sprintf (pfm_err_str, "Insufficient memory for depth array");
      return (pfm_error = READ_DEPTH_ARRAY_CALLOC_ERROR);
    }

  for (i = 0 ; i < *numrecs ; i++)
    {
      status = read_depth_record (hnd, &(*depth_array)[i], &recnum);

      if (status)
        {
          fprintf (stderr,"%s %s %d Bad status on depth array read at %f %f\n", __FILE__, __FUNCTION__, __LINE__, xy.x, xy.y);
          /*
            fprintf (stderr,"Zeroing bin!\n");
            bin_record[hnd].num_soundings = 0;
            write_bin_record_xy (hnd, &bin_record[hnd]);
          */

          return (pfm_error = status);
        }
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_bin_depth_array_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       December 2000

  - Purpose:            Reads the bin record and all of the sounding
                        records associated with the bin at 'coord'.  This
                        function just combines a read_bin_record_index with
                        a series of read_depth_record calls.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   bin record with X and Y index
                                              values in the coord field
                        - depth_array     =   pointer to DEPTH_RECORD array
                                              pointer

  - Caveats:            The user/caller is responsible for freeing the
                        memory associated with 'depth_array' as follows:
                        <pre>

                        DEPTH_RECORD       *depth;

                                     o
                                     o
                                     o

                        read_depth_array_index (hnd, coord, &depth, recnum);

                                     o
                                     o
                                     o

                        for (i = 0 ; i < recnum ; i++)
                            DO SOMETHING WITH DATA IN depth;

                                     o
                                     o
                                     o

                        free (depth);

                        </pre>

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR
                        - READ_DEPTH_RECORD_NO_DATA
                        - READ_DEPTH_ARRAY_CALLOC_ERROR

****************************************************************************/

int32_t read_bin_depth_array_index (int32_t hnd, BIN_RECORD *bin, DEPTH_RECORD **depth_array)
{
  int32_t                 status, recnum, i;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  status = read_bin_record_index (hnd, bin->coord, bin);

  if (status) return (pfm_error = status);


  /*  If there is no depth data, pass back nothing.  */

  if (!bin->num_soundings)
    {
      sprintf (pfm_err_str, "No data in depth record being read");
      return (pfm_error = READ_DEPTH_RECORD_NO_DATA);
    }

  recnum = bin->num_soundings;

  *depth_array = (DEPTH_RECORD *) calloc (recnum, sizeof (DEPTH_RECORD));

  if (*depth_array == NULL)
    {
      sprintf (pfm_err_str, "Insufficient memory for depth array");
      return (pfm_error = READ_DEPTH_ARRAY_CALLOC_ERROR);
    }

  for (i = 0 ; i < bin->num_soundings ; i++) read_depth_record (hnd, &(*depth_array)[i], &recnum);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_bin_depth_array_xy

  - Programmer(s):      Jan C. Depner

  - Date Written:       December 2000

  - Purpose:            Reads the bin record and all of the sounding
                        records associated with the bin at 'xy'.  This
                        function just combines a read_bin_record_xy
                        with a series of read_depth_record calls.

  - Arguments:
                        - hnd             =   PFM file handle
                        - xy              =   position
                        - bin             =   bin record
                        - depth_array     =   pointer to DEPTH_RECORD array
                                              pointer

  - Caveats:            The user/caller is responsible for freeing the
                        memory associated with 'depth_array' as follows:
                        <pre>

                        DEPTH_RECORD       *depth;

                                     o
                                     o
                                     o

                        read_depth_array_index (hnd, coord, &depth, recnum);

                                     o
                                     o
                                     o

                        for (i = 0 ; i < recnum ; i++)
                            DO SOMETHING WITH DATA IN depth;

                                     o
                                     o
                                     o

                        free (depth);

                        </pre>

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR
                        - READ_DEPTH_RECORD_NO_DATA
                        - READ_DEPTH_ARRAY_CALLOC_ERROR

****************************************************************************/

int32_t read_bin_depth_array_xy (int32_t hnd, NV_F64_COORD2 xy, BIN_RECORD *bin, DEPTH_RECORD **depth_array)
{
  int32_t             status, i, recnum;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  status = read_bin_record_xy (hnd, xy, bin);

  if (status) return (pfm_error = status);


  /*  If there is no depth record, pass back nothing.  */

  if (!bin->num_soundings)
    {
      sprintf (pfm_err_str, "No data in depth record being read");
      return (pfm_error = READ_DEPTH_RECORD_NO_DATA);
    }

  recnum = bin->num_soundings;

  *depth_array = (DEPTH_RECORD *) calloc (recnum, sizeof (DEPTH_RECORD));

  if (*depth_array == NULL)
    {
      sprintf (pfm_err_str, "Insufficient memory for depth array");
      return (pfm_error = READ_DEPTH_ARRAY_CALLOC_ERROR);
    }

  for (i = 0 ; i < bin->num_soundings ; i++) read_depth_record (hnd, &(*depth_array)[i], &recnum);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        update_depth_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Updates the depth record at depth->address.  This
                        function checks for a match with the input depth
                        record.  The match is based on the file, ping, and
                        beam number from the input depth record.  The only
                        fields that are required are the address, file,
                        ping, beam, and validity.  If a match is not found,
                        the program will abort, otherwise the status
                        information from the input depth record is updated
                        in the index file.  This function is only used
                        internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure
                        - set_modified    =   BOOL, controls PFM_MODIFIED

  - Return Value:
                        - SUCCESS
                        - UPDATE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR

****************************************************************************/

static int32_t update_depth_record (int32_t hnd, DEPTH_RECORD *depth, uint8_t set_modified)
{
  int32_t             ping_number, record_pos;
  int16_t             file_number, beam_number;
  uint32_t            val;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if (read_bin_record_index (hnd, depth->coord, &bin_record[hnd]))
    {
      sprintf (pfm_err_str, "Error reading bin record in update_depth_record");
      return (pfm_error = UPDATE_DEPTH_RECORD_READ_BIN_RECORD_ERROR);
    }


  /*  Save the validity to see if we need to update the bin record.  */

  val = bin_record[hnd].validity;


  /*  Set the depth block address.  */

  depth_record_address[hnd] = depth->address.block;


  /*  Set the buffer position.  */

  record_pos = depth->address.record * dep_off[hnd].single_point_bits;


  /*  See if we need to flush the depth buffer.  */

  if (depth_record_address[hnd] != previous_depth_block[hnd])
    {
      if (depth_record_modified[hnd]) write_depth_buffer (hnd, previous_depth_block[hnd]);

      lfseek (ndx_file_hnd[hnd], depth_record_address[hnd], SEEK_SET);

      if (!lfread (depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]))
        {
          sprintf (pfm_err_str, "Error reading depth record");
          return (pfm_error = UPDATE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR);
        }


      /*  Save the previous block address.  */

      previous_depth_block[hnd] = depth_record_address[hnd];
    }


  /*  Unpack the file, ping, and beam numbers from the 'physical' record.  */

  file_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].file_number_pos + record_pos, hd[hnd].file_number_bits);

  ping_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].ping_number_pos + record_pos, hd[hnd].ping_number_bits);

  beam_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].beam_number_pos + record_pos, hd[hnd].beam_number_bits);


  /*  Check for a match, if we got one update only the status.  */

  if (file_number == depth->file_number && ping_number == depth->ping_number && beam_number == depth->beam_number)
    {
      /* JSB 11/20/2003 Set the PFM_MODIFIED bit in the validity bits, based on control argument. */

      if (set_modified) depth->validity |= PFM_MODIFIED;

      pfm_bit_pack (depth_record_data[hnd], dep_off[hnd].validity_pos + record_pos, hd[hnd].validity_bits, depth->validity);


      /*  Set the modified flag.  */

      depth_record_modified[hnd] = NVTrue;


      /*  If the record is valid, set the DATA FLAG in the bin record.  */

      if (!(depth->validity & PFM_INVAL) && !(depth->validity & PFM_DELETED)) bin_record[hnd].validity |= PFM_DATA;

      bin_record[hnd].validity |= depth->validity;


      /*  If we've changed the validity, write the bin record validity.  */

      if (val != bin_record[hnd].validity) write_bin_record_validity_index (hnd, &bin_record[hnd], PFM_VAL_MASK);
    }
  else
    {
      fprintf (stderr, "\n\n\nError in update_depth_record:\n");
      fprintf (stderr, "File, ping, or beam numbers did not match.\n");
      fprintf (stderr, "%d %d %d - %d %d %d %"PRId64"\n\n\n", file_number, ping_number, beam_number, depth->file_number,
               depth->ping_number, depth->beam_number, depth->address.block);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        update_depth_record_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Updates the depth record at depth->address in the
                        bin at the index coordinates (depth->coord).  This
                        function checks for a match with the input depth
                        record.  The match is based on the file, ping, and
                        beam number from the input depth record.  The only
                        fields that are required are the address, file,
                        ping, beam, validity, and coordinates.  If a match
                        is not found, the program will abort, otherwise the
                        status information from the input depth record is
                        updated in the index file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - UPDATE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_NO_DEPTH_DATA

****************************************************************************/

int32_t update_depth_record_index (int32_t hnd, DEPTH_RECORD *depth)
{
  uint8_t set_modified = 1;

  return (pfm_error = update_depth_record (hnd, depth, set_modified));
}



/***************************************************************************/
/*!

  - Module Name:        update_depth_record_xy

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Updates the depth record at depth->address in the
                        bin at the input position (depth->xyz).  This
                        function checks for a match with the input depth
                        record.  The match is based on the file, ping, and
                        beam number from the input depth record.  The only
                        fields that are required are the address, file,
                        ping, beam, validity, and coordinates.  If a match
                        is not found, the program will abort, otherwise the
                        status information from the input depth record is
                        updated in the index file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - UPDATE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_NO_DEPTH_DATA

****************************************************************************/

int32_t update_depth_record_xy (int32_t hnd, DEPTH_RECORD *depth)
{
  NV_I32_COORD2       coord;
  NV_F64_COORD2       xy;
  uint8_t             set_modified = 1;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  xy.y = depth->xyz.y;
  xy.x = depth->xyz.x;

  compute_index_ptr (xy, &coord, &bin_header[hnd]);


  depth->coord.x = coord.x;
  depth->coord.y = coord.y;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = update_depth_record (hnd, depth, set_modified));
}



/***************************************************************************/
/*!

  - Module Name:        update_depth_record_index_ext_flags

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 2003

  - Purpose:            Updates the depth record at depth->address in the
                        bin at the index coordinates (depth->coord).  This
                        function checks for a match with the input depth
                        record.  The match is based on the file, ping, and
                        beam number from the input depth record.  The only
                        fields that are required are the address, file,
                        ping, beam, validity, and coordinates.  If a match
                        is not found, the program will abort, otherwise the
                        status information from the input depth record is
                        updated in the index file.

  - NOTE:               This function differs from update_depth_record_index
                        in that control of the modified bit in validity
                        flags is the calling application's responsibility

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - UPDATE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_NO_DEPTH_DATA

****************************************************************************/

int32_t update_depth_record_index_ext_flags (int32_t hnd, DEPTH_RECORD *depth)
{
  uint8_t set_modified = 0;

  return (pfm_error = update_depth_record (hnd, depth, set_modified));
}



/***************************************************************************/
/*!

  - Module Name:        update_depth_record_xy_ext_flags

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 2003

  - Purpose:            Updates the depth record at depth->address in the
                        bin at the input position (depth->xyz).  This
                        function checks for a match with the input depth
                        record.  The match is based on the file, ping, and
                        beam number from the input depth record.  The only
                        fields that are required are the address, file,
                        ping, beam, validity, and coordinates.  If a match
                        is not found, the program will abort, otherwise the
                        status information from the input depth record is
                        updated in the index file.

  - NOTE:               This function differs from update_depth_record_index
                        in that control of the modified bit in validity
                        flags is the calling application's responsibility

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - UPDATE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - UPDATE_DEPTH_RECORD_NO_DEPTH_DATA

****************************************************************************/

int32_t update_depth_record_xy_ext_flags (int32_t hnd, DEPTH_RECORD *depth)
{
  NV_I32_COORD2       coord;
  NV_F64_COORD2       xy;
  uint8_t             set_modified = 0;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  xy.y = depth->xyz.y;
  xy.x = depth->xyz.x;

  compute_index_ptr (xy, &coord, &bin_header[hnd]);


  depth->coord.x = coord.x;
  depth->coord.y = coord.y;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = update_depth_record (hnd, depth, set_modified));
}



/***************************************************************************/
/*!

  - Module Name:        pack_depth_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Packs a logical depth record into a physical record.
                        This function is only used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - record_pos      =   The record number in the
                                              physical record
                        - depth_buffer    =   A buffer containing the
                                              physical record
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS

\***************************************************************************/

static int32_t pack_depth_record( uint8_t *depth_buffer, DEPTH_RECORD *depth, int32_t record_pos, int32_t hnd )
{
  int32_t             i;
  float               x_offset, y_offset;


  /*  Pack the current depth record into the 'physical' record.  */

  pfm_bit_pack (depth_buffer, dep_off[hnd].file_number_pos + record_pos, hd[hnd].file_number_bits, depth->file_number);

  pfm_bit_pack (depth_buffer, dep_off[hnd].line_number_pos + record_pos, hd[hnd].line_number_bits, depth->line_number);

  pfm_bit_pack (depth_buffer, dep_off[hnd].ping_number_pos + record_pos, hd[hnd].ping_number_bits, depth->ping_number);

  pfm_bit_pack (depth_buffer, dep_off[hnd].beam_number_pos + record_pos, hd[hnd].beam_number_bits, depth->beam_number);

  pfm_bit_pack (depth_buffer, dep_off[hnd].depth_pos + record_pos, hd[hnd].depth_bits, NINT ((depth->xyz.z + hd[hnd].depth_offset) * hd[hnd].depth_scale));


  /*  Compute the offsets from the position.   */

  /*  Stored as lat/lon.  */

  if (!hd[hnd].head.proj_data.projection)
    {
      x_offset = depth->xyz.x - (bin_header[hnd].mbr.min_x + (depth->coord.x * bin_header[hnd].x_bin_size_degrees));
      y_offset = depth->xyz.y - (bin_header[hnd].mbr.min_y + (depth->coord.y * bin_header[hnd].y_bin_size_degrees));


      pfm_bit_pack (depth_buffer, dep_off[hnd].x_offset_pos + record_pos, hd[hnd].offset_bits, NINT ((x_offset / bin_header[hnd].x_bin_size_degrees) *
                                                                                                     x_offset_scale[hnd]));

      pfm_bit_pack (depth_buffer, dep_off[hnd].y_offset_pos + record_pos, hd[hnd].offset_bits, NINT ((y_offset / bin_header[hnd].y_bin_size_degrees) *
                                                                                                     y_offset_scale[hnd]));
    }


  /*  Stored as x/y.  */

  else
    {
      x_offset = depth->xyz.x - (bin_header[hnd].mbr.min_x + (depth->coord.x * bin_header[hnd].bin_size_xy));
      y_offset = depth->xyz.y - (bin_header[hnd].mbr.min_y + (depth->coord.y * bin_header[hnd].bin_size_xy));


      pfm_bit_pack (depth_buffer, dep_off[hnd].x_offset_pos + record_pos, hd[hnd].offset_bits, NINT ((x_offset / bin_header[hnd].bin_size_xy) *
                                                                                                     x_offset_scale[hnd]));

      pfm_bit_pack (depth_buffer, dep_off[hnd].y_offset_pos + record_pos, hd[hnd].offset_bits, NINT ((y_offset / bin_header[hnd].bin_size_xy) *
                                                                                                     y_offset_scale[hnd]));
    }


  for (i = 0 ; i < hd[hnd].head.num_ndx_attr ; i++)
    {
      pfm_bit_pack (depth_buffer, dep_off[hnd].attr_pos[i] + record_pos, hd[hnd].ndx_attr_bits[i],
                    NINT ((double) (depth->attr[i] - (double) hd[hnd].head.min_ndx_attr[i]) * (double) hd[hnd].head.ndx_attr_scale[i]));
    }

  pfm_bit_pack (depth_buffer, dep_off[hnd].validity_pos + record_pos, hd[hnd].validity_bits, depth->validity);

  if (hd[hnd].horizontal_error_bits)
    {
      if (depth->horizontal_error >= hd[hnd].horizontal_error_null) depth->horizontal_error = hd[hnd].horizontal_error_null;
      pfm_bit_pack (depth_buffer, dep_off[hnd].horizontal_error_pos + record_pos, hd[hnd].horizontal_error_bits,
                    NINT (depth->horizontal_error * hd[hnd].head.horizontal_error_scale));
    }

  if (hd[hnd].vertical_error_bits)
    {
      if (depth->vertical_error >= hd[hnd].vertical_error_null) depth->vertical_error = hd[hnd].vertical_error_null;
      pfm_bit_pack (depth_buffer, dep_off[hnd].vertical_error_pos + record_pos, hd[hnd].vertical_error_bits,
                    NINT (depth->vertical_error * hd[hnd].head.vertical_error_scale));
    }

  return SUCCESS;
}



/***************************************************************************/
/*!

  - Module Name:        add_depth_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Adds a new depth record to the data chain in the
                        index file for a specific bin in the bin file.
                        NOTE: If the PFM_MODIFIED in the depth validity
                        bits is set, the EDITED FLAG bit is set in the bin
                        record associated with this depth.

  - NOTE:               If the PFM_SELECTED_SOUNDING flag is set in
                        the depth validity bits, the SELECTED FLAG is set
                        in the bin record associated with this depth.
                        This function is only used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - ADD_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - ADD_DEPTH_RECORD_TOO_MANY_SOUNDINGS_ERROR
                        - WRITE_BIN_RECORD_DATA_READ_ERROR
                        - FILE_NUMBER_TOO_LARGE_ERROR
                        - LINE_NUMBER_TOO_LARGE_ERROR
                        - PING_NUMBER_TOO_LARGE_ERROR
                        - BEAM_NUMBER_TOO_LARGE_ERROR
                        - ADD_DEPTH_RECORD_OUT_OF_LIMITS_ERROR

****************************************************************************/

static int32_t add_depth_record (int32_t hnd, DEPTH_RECORD *depth)
{
  int32_t             record_pos, status;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  We have to turn off use of the depth chain pointers in the bin record on the off chance that someone has done a read of a bin record prior
      to adding a depth record to the same chain.  This isn't normally a problem as the only place we usually add depth records is in a
      loader.  Even then we can mitigate this by closing and reopening the file after we have added all record but prior to recomputing
      the bins.  */

  use_chain_pointer[hnd] = NVFalse;


  /* Safety check, to make sure the X/Y fits within the limits of the PFM */
  /* Webb McDonald -- Tue May 31 00:39:42 2005 */

  if (depth->xyz.x > bin_header[hnd].mbr.max_x || depth->xyz.x < bin_header[hnd].mbr.min_x ||
      depth->xyz.y > bin_header[hnd].mbr.max_y || depth->xyz.y < bin_header[hnd].mbr.min_y)
    {
      sprintf (pfm_err_str, "Error, trying to add_depth_record outside limits of PFM!\n");
      return (pfm_error = ADD_DEPTH_RECORD_OUT_OF_LIMITS_ERROR);
    }


  /*  Other safety checks, make sure that the ping, beam, file, and line numbers are in range.  */

  if (depth->file_number > hd[hnd].head.max_input_files)
    {
      sprintf (pfm_err_str, "Error, file number %d too large!\n", depth->file_number);
      return (pfm_error = FILE_NUMBER_TOO_LARGE_ERROR);
    }

  if (depth->line_number > hd[hnd].head.max_input_lines)
    {
      sprintf (pfm_err_str, "Error, line number %d too large!\n", depth->line_number);
      return (pfm_error = LINE_NUMBER_TOO_LARGE_ERROR);
    }

  if (depth->ping_number > hd[hnd].head.max_input_pings)
    {
      sprintf (pfm_err_str, "Error, record number %d too large!\n", depth->ping_number);
      return (pfm_error = PING_NUMBER_TOO_LARGE_ERROR);
    }

  if (depth->beam_number > hd[hnd].head.max_input_beams)
    {
      sprintf (pfm_err_str, "Error, subrecord number %d too large!\n", depth->beam_number);
      return (pfm_error = BEAM_NUMBER_TOO_LARGE_ERROR);
    }


  if (read_bin_record_index (hnd, depth->coord, &bin_record[hnd]))
    {
      sprintf (pfm_err_str, "Error reading bin record in add_depth_record");
      return (pfm_error = ADD_DEPTH_RECORD_READ_BIN_RECORD_ERROR);
    }


  /*  If there is a depth record, go to the tail of the chain and add to it.  */

  if (bin_record[hnd].num_soundings)
    {
      /*  Compute the position within the buffer.  */

      record_pos = (bin_record[hnd].num_soundings % hd[hnd].record_length) * dep_off[hnd].single_point_bits;


      /*  No need to flush or read the buffer if we haven't changed buffers.  */

      /*
          BUT DO read if depth_record_data is uninitialized.  This prevents a corruption issue that can occur if you are
          appending data to the PFM that is *new* and you have had no reason to read the depth_record... UNTIL NOW, where you must.

          Webb McDonald -- Thu May 26 19:07:43 2005
      */

      if (bin_record_tail_pointer[hnd] != depth_record_address[hnd] || !record_pos || *(depth_record_data[hnd]) == 0x0)
        {
          /*  See if we need to flush the write buffer.  */

          if (depth_record_modified[hnd]) write_depth_buffer (hnd, depth_record_address[hnd]);


          /*  Move to the tail of the data chain.  */

          lfseek (ndx_file_hnd[hnd], bin_record_tail_pointer[hnd], SEEK_SET);


          /*  Read the 'physical' depth record.  */

          lfread (depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]);
        }


      /*  If the tail of the chain is at the end of a 'physical' record, write the continuation record pointer to the 'physical' record
          and move to the end of the file.  */

      if (!record_pos)
        {
          /*  If we are multithreading we need to lock the mutex and write a dummy buffer to the end of the file in order to save that space.
              This is done so that any other thread waiting to add to a depth chain won't acquire the same block for continuation.  */

#ifdef NVWIN3X
          if (threading && mutex[hnd] >= 0) WaitForSingleObject (pfm_write_lock[mutex[hnd]], INFINITE);
#else
          if (threading && mutex[hnd] >= 0) pthread_mutex_lock (&pfm_write_lock[mutex[hnd]]);
#endif

          lfseek (ndx_file_hnd[hnd], 0, SEEK_END);

          continuation_pointer[hnd] = lftell (ndx_file_hnd[hnd]);

          pfm_double_bit_pack (depth_record_data[hnd], dep_off[hnd].continuation_pointer_pos, hd[hnd].record_pointer_bits, continuation_pointer[hnd]);


          /*  If we are multithreading, here we write a dummy record to reserve the space at the end of the file and then unlock the mutex.  */

          if (threading && mutex[hnd] >= 0)
            {
              if (!(lfwrite (zero_depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd])))
                {
                  sprintf (pfm_err_str, "Unable to write zero record to end of index file");
                  return (pfm_error = WRITE_DEPTH_BUFFER_WRITE_ERROR);
                }

              lfflush (ndx_file_hnd[hnd]);


#ifdef NVWIN3X
              ReleaseMutex (pfm_write_lock[mutex[hnd]]);
#else
              pthread_mutex_unlock (&pfm_write_lock[mutex[hnd]]);
#endif
            }


          /*  Move to the tail of the data chain and write the buffer (with the new continuation record).  */

          lfseek (ndx_file_hnd[hnd], bin_record_tail_pointer[hnd], SEEK_SET);

          if ((status = write_depth_buffer (hnd, bin_record_tail_pointer[hnd]))) return (status);


          /*  Zero out the depth buffer.  */

          memset (depth_record_data[hnd], 0, dep_off[hnd].record_size);


          /*  Set the new tail pointer for the bin record.  */

          bin_record_tail_pointer[hnd] = continuation_pointer[hnd];
        }
    }
  else
    {
      /*  This is a new record, flush the depth buffer prior to finding the end of file.  */

      if (depth_record_modified[hnd]) write_depth_buffer (hnd, depth_record_address[hnd]);


      /*  If we are multithreading we need to lock the mutex and write a dummy buffer to the end of the file in order to save that space.
          This is done so that any other thread waiting to add to a depth chain won't acquire the same block for continuation.  */

#ifdef NVWIN3X
      if (threading && mutex[hnd] >= 0) WaitForSingleObject (pfm_write_lock[mutex[hnd]], INFINITE);
#else
      if (threading && mutex[hnd] >= 0) pthread_mutex_lock (&pfm_write_lock[mutex[hnd]]);
#endif


      /*  Seek the end of the file.  */

      lfseek (ndx_file_hnd[hnd], 0, SEEK_END);


      /*  Set the head pointer for the bin record.  */

      bin_record_head_pointer[hnd] = lftell (ndx_file_hnd[hnd]);


      /*  If we are multithreading, here we write a dummy record to reserve the space at the end of the file and then unlock the mutex.  */

      if (threading && mutex[hnd] >= 0)
        {
          if (!(lfwrite (zero_depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd])))
            {
              sprintf (pfm_err_str, "Unable to write zero record to end of index file");
              return (pfm_error = WRITE_DEPTH_BUFFER_WRITE_ERROR);
            }

          lfflush (ndx_file_hnd[hnd]);


#ifdef NVWIN3X
	  ReleaseMutex (pfm_write_lock[mutex[hnd]]);
#else
          pthread_mutex_unlock (&pfm_write_lock[mutex[hnd]]);
#endif
        }


      /*  Set the tail pointer.  */

      bin_record_tail_pointer[hnd] = bin_record_head_pointer[hnd];


      /*  Zero out the depth buffer.  */

      memset (depth_record_data[hnd], 0, dep_off[hnd].record_size);


      /*  Set the record position to 0.  */

      record_pos = 0;
    }


  /*  Pack the current depth record into the 'physical' record.  */

  pack_depth_record( depth_record_data[hnd], depth, record_pos, hnd );


  /*  Set the depth_record_address.  */

  depth_record_address[hnd] = bin_record_tail_pointer[hnd];


  /*  Set the depth record modified flag.  */

  depth_record_modified[hnd] = NVTrue;


  /*  Increment the number of soundings.  */

  bin_record[hnd].num_soundings++;


  /*  Make sure that we don't blow out the number of soundings count.  */

  if (bin_record[hnd].num_soundings >= count_size[hnd])
    {
      sprintf (pfm_err_str, "Too many soundings in this cell - %d %d.\n", bin_record[hnd].num_soundings, count_size[hnd]);
      return (pfm_error = ADD_DEPTH_RECORD_TOO_MANY_SOUNDINGS_ERROR);
    }


  /*  Set the bin_record_address so that the write_bin_record function will know that this record has already been read and the head and
      tail pointers are known.  */

  bin_record_address[hnd] = ((int64_t) depth->coord.y * (int64_t) bin_header[hnd].bin_width + depth->coord.x) *
    (int64_t) bin_off[hnd].record_size + hd[hnd].bin_header_size;


  /*  Set the bin validity to the same as the depth validity.  */

  bin_record[hnd].validity |= depth->validity;


  /*  Write out the bin record. */

  status = write_bin_record (hnd, &bin_record[hnd], bin_record_address[hnd]);

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = status);
}



/***************************************************************************/
/*!

  - Module Name:        add_depth_record_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a depth record to the bin/index file at
                        depth.coord.

  - NOTE:               If PFM_MODIFIED in the depth validity bits is
                        set, the PFM_MODIFIED bit is set in the bin record
                        associated with this depth.

  - ANOTHER NOTE:       If the PFM_SELECTED_SOUNDING flag is set in
                        the depth validity bits, the it will also be set in
                        the bin record associated with this depth.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - ADD_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - WRITE_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

int32_t add_depth_record_index (int32_t hnd, DEPTH_RECORD *depth)
{
  return (pfm_error = add_depth_record (hnd, depth));
}



/***************************************************************************/
/*!

  - Module Name:        add_depth_record_xy

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Writes a depth record to the bin/index file at
                        depth.xy.

  - NOTE:               If PFM_MODIFIED in the depth validity bits is
                        set, the PFM_MODIFIED bit is set in the bin record
                        associated with this depth.

  - ANOTHER NOTE:       If the PFM_SELECTED_SOUNDING flag is set in
                        the depth validity bits, the it will also be set in
                        the bin record associated with this depth.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - ADD_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - WRITE_BIN_RECORD_DATA_READ_ERROR

****************************************************************************/

int32_t add_depth_record_xy (int32_t hnd, DEPTH_RECORD *depth)
{
  NV_I32_COORD2       coord;
  NV_F64_COORD2       xy;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  xy.y = depth->xyz.y;
  xy.x = depth->xyz.x;

  compute_index_ptr (xy, &coord, &bin_header[hnd]);

  depth->coord.x = coord.x;
  depth->coord.y = coord.y;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = add_depth_record (hnd, depth));
}



/***************************************************************************/
/*!

  - Module Name:        recompute_bin_values

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Recomputes the bin record values from the depth
                        records that fall within the bin and writes the bin
                        record back to the file.  Also returns the modified
                        bin record to the caller.  This function is only
                        used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   BIN_RECORD structure
                        - mask            =   use this to decide which parts
                                              of the validity bits need to be
                                              set as they are in the input
                                              bin record (for example, if
                                              mask is set to PFM_MODIFIED
                                              | PFM_CHECKED only those two
                                              bits will be set from the input
                                              bin record)
                        - depth           =   pointer to a pre-existing depth
                                              record array (NULL if not
                                              available)

  - Return Value:
                        - SUCCESS
                        - RECOMPUTE_BIN_VALUES_READ_BIN_RECORD_ERROR
                        - RECOMPUTE_BIN_VALUES_READ_DEPTH_RECORD_ERROR
                        - RECOMPUTE_BIN_VALUES_NO_SOUNDING_DATA_ERROR
                        - RECOMPUTE_BIN_VALUES_WRITE_BIN_RECORD_ERROR

****************************************************************************/

static int32_t recompute_bin_values (int32_t hnd, BIN_RECORD *bin, uint32_t mask, DEPTH_RECORD *depth)
{
  int32_t             valid_count, non_count, i, numrecs;
  double              sum_valid, sum2_valid, sum_depth, temp;
  uint32_t            validity;
  uint32_t            val;
  DEPTH_RECORD        *depth_record;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Save the validity bits specified in the mask.  */

  val = bin->validity & mask;


  /*  Reset the previous bin address to force the read.  */

  previous_bin_address[hnd] = -1;

  if (read_bin_record_index (hnd, bin->coord, bin))
    {
      sprintf (pfm_err_str, "Error reading bin_record in recompute_bin_values");
      return (pfm_error = RECOMPUTE_BIN_VALUES_READ_BIN_RECORD_ERROR);
    }


  /*  Set the suspect, selected, user, and reference bits to zero.  We'll set them based on the data.  */

  /*  Dwayne MacLeod (QPS, 013014) PFM_INVAL was not being cleared as well. If bin was previously flagged as having invalid soundings
      but soundings in that bin were editted so that no soundings in the bin were invalid, PFM_INVAL was not being
      properly cleared from the bin validity flag */

  bin->validity &= (65535 ^ (PFM_SUSPECT | PFM_SELECTED | PFM_USER | PFM_REFERENCE | PFM_INVAL));


  /*  If there is depth data for this bin, recompute the bin values.  */

  if (bin->num_soundings)
    {
      /*  Mark as checked/modified if needed.  */

      bin->validity |= val;


      /*  Danny Neville's fix for +Z being larger than biggest -Z.  */

      bin->min_filtered_depth = 999999;
      bin->min_depth = 999999;
      bin->max_filtered_depth = -999999;
      bin->max_depth = -999999;


      sum_valid = 0.0;
      sum2_valid = 0.0;
      sum_depth = 0.0;

      valid_count = 0;
      non_count = 0;


      bin_record[hnd] = *bin;


      /*  If we passed in a pre-existing depth array don't read the depth chain.  */

      if (depth != NULL)
        {
          depth_record = depth;
          numrecs = bin->num_soundings;
        }
      else
        {
          if (read_depth_array_index (hnd, bin->coord, &depth_record, &numrecs))
            {
              sprintf (pfm_err_str, "Error reading depth record in recompute_bin_values");
              return (pfm_error = RECOMPUTE_BIN_VALUES_READ_DEPTH_RECORD_ERROR);
            }
        }


      for (i = 0 ; i < numrecs ; i++)
        {
          validity = depth_record[i].validity;


          /*  Check for reference soundings to set the flag.  */

          if (validity & PFM_REFERENCE) bin->validity |= PFM_REFERENCE;


          /*  DO NOT use records marked as file deleted, whose depth is set to the null value, or that are "reference" data.  */

          if ((!(validity & (PFM_DELETED | PFM_REFERENCE))) && depth_record[i].xyz.z < bin_header[hnd].null_depth)
            {
              /*  If the PFM_MODIFIED bit is set, and we're not forcing the setting (or unsetting) of this bit in the bin record,
                  set the PFM_MODIFIED flag in this bin.  */

              if ((!(mask & PFM_MODIFIED)) && (validity & PFM_MODIFIED)) bin->validity |= PFM_MODIFIED;

              if (!(validity & PFM_INVAL))
                {
                  if (!bin_header[hnd].class_type || (bin_header[hnd].class_type == 1 && (validity & PFM_USER_01)) ||
                      (bin_header[hnd].class_type == 2 && (validity & PFM_USER_02)) ||
                      (bin_header[hnd].class_type == 3 && (validity & PFM_USER_03)) ||
                      (bin_header[hnd].class_type == 4 && (validity & PFM_USER_04)) ||
                      (bin_header[hnd].class_type == 5 && (validity & PFM_USER_05)) ||
                      (bin_header[hnd].class_type == 6 && (validity & PFM_USER_06)) ||
                      (bin_header[hnd].class_type == 7 && (validity & PFM_USER_07)) ||
                      (bin_header[hnd].class_type == 8 && (validity & PFM_USER_08)) ||
                      (bin_header[hnd].class_type == 9 && (validity & PFM_USER_09)) ||
                      (bin_header[hnd].class_type == 10 && (validity & PFM_USER_10)))
                    {
                      if (depth_record[i].xyz.z < bin->min_filtered_depth)
                        {
                          bin->min_filtered_depth = depth_record[i].xyz.z;
                        }
                      if (depth_record[i].xyz.z > bin->max_filtered_depth)
                        {
                          bin->max_filtered_depth = depth_record[i].xyz.z;
                        }


                      sum_valid += depth_record[i].xyz.z;
                      sum2_valid += (depth_record[i].xyz.z * depth_record[i].xyz.z);

                      valid_count++;
                    }
                }


              /*  Compute non-filtered values.  */

              if (depth_record[i].xyz.z < bin->min_depth)
                {
                  bin->min_depth = depth_record[i].xyz.z;
                }
              if (depth_record[i].xyz.z > bin->max_depth)
                {
                  bin->max_depth = depth_record[i].xyz.z;
                }

              sum_depth += depth_record[i].xyz.z;

              non_count++;


              /*  Set the bin validity to the same as the depth validity with the exception of the values hard-wired in mask.  */

              bin->validity |= (depth_record[i].validity & (PFM_VAL_MASK ^ mask));


              /*  If the bin is marked as "checked", turn off the suspect bit.  Removed at the suggestion of IVS.
                  JCD 06/07/05  */

              /*if (bin->validity & PFM_CHECKED) bin->validity &= ~PFM_SUSPECT;*/
            }
        }
      if (depth == NULL) free (depth_record);



      if (!valid_count)
        {
          bin->min_filtered_depth = bin_header[hnd].null_depth;
          bin->max_filtered_depth = bin_header[hnd].null_depth;
          if (compute_average[hnd]) bin->avg_filtered_depth = bin_header[hnd].null_depth;
          bin->validity &= ~PFM_DATA;
          bin->num_valid = 0;
        }
      else
        {
          bin->standard_dev = 0.0;

          temp = sum_valid / (double) valid_count;
          if (compute_average[hnd]) bin->avg_filtered_depth = temp;
          if (valid_count > 1)
            {
              double variance;
              variance = ((sum2_valid - ((double) valid_count * (pow (temp, 2.0)))) / ((double) valid_count - 1.0));

              if (variance >= 0)
                {
                  bin->standard_dev = sqrt (variance);
                }
              else
                {
                  bin->standard_dev = 0.0;
                }
            }

          bin->validity |= PFM_DATA;
          bin->num_valid = valid_count;
        }


      if (!non_count)
        {
          bin->min_depth = bin_header[hnd].null_depth;
          bin->max_depth = bin_header[hnd].null_depth;
          if (compute_average[hnd]) bin->avg_depth = bin_header[hnd].null_depth;
        }
      else
        {
          if (compute_average[hnd]) bin->avg_depth = sum_depth / (double) non_count;
        }


      /*  Write the record out.   */

      if (write_bin_record_index (hnd, bin))
        {
          sprintf (pfm_err_str, "Error writing bin record in recompute_bin_values");
          return (pfm_error = RECOMPUTE_BIN_VALUES_WRITE_BIN_RECORD_ERROR);
        }


      /*  Update the coverage map.  */

      update_cov_map( hnd, bin_record_address[hnd] );
    }
  else
    {
      /*  This should never happen in real life ;-)  */

      sprintf (pfm_err_str, "No data in depth record in recompute_bin_values");
      return (pfm_error = RECOMPUTE_BIN_VALUES_NO_SOUNDING_DATA_ERROR);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        recompute_bin_values_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Recomputes the bin record values from the depth
                        records that fall within the bin and writes the bin
                        record back to the file.  Also returns the modified
                        bin record to the caller.  The bin is at coord.

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   X and Y index values
                        - bin             =   BIN_RECORD structure
                        - mask            =   use this to decide which parts
                                              of the validity bits need to be
                                              set as they are in the input
                                              bin record (for example, if
                                              mask is set to PFM_MODIFIED
                                              | PFM_CHECKED only those two
                                              bits will be set from the input
                                              bin record)

  - Return Value:
                        - SUCCESS
                        - RECOMPUTE_BIN_VALUES_READ_BIN_RECORD_ERROR
                        - RECOMPUTE_BIN_VALUES_READ_DEPTH_RECORD_ERROR
                        - RECOMPUTE_BIN_VALUES_NO_SOUNDING_DATA_ERROR
                        - RECOMPUTE_BIN_VALUES_WRITE_BIN_RECORD_ERROR

****************************************************************************/

int32_t recompute_bin_values_index (int32_t hnd, NV_I32_COORD2 coord, BIN_RECORD *bin, uint32_t mask)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  bin->coord.y = coord.y;
  bin->coord.x = coord.x;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = recompute_bin_values (hnd, bin, mask, NULL));
}



/***************************************************************************/
/*!

  - Module Name:        recompute_bin_values_xy

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 2001

  - Purpose:            Recomputes the bin record values from the depth
                        records that fall within the bin and writes the bin
                        record back to the file.  Also returns the modified
                        bin record to the caller.  The bin is at xy.

  - Arguments:
                        - hnd             =   PFM file handle
                        - xy              =   position
                        - bin             =   BIN_RECORD structure
                        - mask            =   use this to decide which parts
                                              of the validity bits need to be
                                              set as they are in the input
                                              bin record (for example, if
                                              mask is set to PFM_MODIFIED
                                              | PFM_CHECKED only those two
                                              bits will be set from the input
                                              bin record)

  - Return Value:
                        - SUCCESS
                        - RECOMPUTE_BIN_VALUES_READ_BIN_RECORD_ERROR
                        - RECOMPUTE_BIN_VALUES_READ_DEPTH_RECORD_ERROR
                        - RECOMPUTE_BIN_VALUES_NO_SOUNDING_DATA_ERROR
                        - RECOMPUTE_BIN_VALUES_WRITE_BIN_RECORD_ERROR

****************************************************************************/

int32_t recompute_bin_values_xy (int32_t hnd, NV_F64_COORD2 xy, BIN_RECORD *bin, uint32_t mask)
{
  NV_I32_COORD2       coord;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  compute_index_ptr (xy, &coord, &bin_header[hnd]);

  bin->coord.y = coord.y;
  bin->coord.x = coord.x;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = recompute_bin_values (hnd, bin, mask, NULL));
}



/***************************************************************************/
/*!

  - Module Name:        recompute_bin_values_from_depth_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       December 2000

  - Purpose:            Recomputes the bin record values from the depth
                        records that fall within the bin that were passed
                        in by the caller and writes the bin record back to
                        the file.  Also returns the modified bin record to
                        the caller.  The bin is at bin->coord.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   BIN_RECORD structure
                        - mask            =   use this to decide which parts
                                              of the validity bits need to be
                                              set as they are in the input
                                              bin record (for example, if
                                              mask is set to PFM_MODIFIED
                                              | PFM_CHECKED only those two
                                              bits will be set from the input
                                              bin record)
                        - depth_array     =   pointer to DEPTH_RECORD array

  - Return Value:
                        - SUCCESS
                        - RECOMPUTE_BIN_VALUES_READ_BIN_RECORD_ERROR
                        - RECOMPUTE_BIN_VALUES_READ_DEPTH_RECORD_ERROR
                        - RECOMPUTE_BIN_VALUES_NO_SOUNDING_DATA_ERROR
                        - RECOMPUTE_BIN_VALUES_WRITE_BIN_RECORD_ERROR

  - Caveats:            WARNING - It is assumed that if you call this
                        function with a depth array that it is the depth
                        array for the coords that you passed in.  Also, it
                        is assumed that the caller has performed an
                        update_depth_record for each depth record in the
                        array that was modified after being read.  If this
                        is not the case then there will be disagreement
                        between the depth records and the bin record.  This
                        can be rectified by using recompute_bin_values_index
                        but if you're going to do that why call this
                        function.  The purpose of this function is to speed
                        up filtering processes by reducing the amount of
                        I/O.  It is best used in conjunction with
                        read_bin_depth_array_index as in the following
                        example:
                        <pre>

                        DEPTH_RECORD       *depth;
                        BIN_RECORD         bin;

                                        o
                                        o
                                        o

                        read_bin_depth_array_index (hnd, &bin, &depth);

                                        o
                                        o
                                        o

                        for (i = 0 ; i < bin.num_soundings ; i++)
                        {
                            DO SOMETHING WITH DATA IN depth[i];
                            SET THE PFM_MODIFIED BIT IN depth[i];
                            update_depth_record_index (hnd, &depth[i]);
                        }
                        bin.validity &= ~PFM_CHECKED;
                        recompute_bin_values_from_depth_index (hnd, &bin,
                            PFM_CHECKED, depth);

                                        o
                                        o
                                        o

                        free (depth);

                        </pre>

****************************************************************************/

int32_t recompute_bin_values_from_depth_index (int32_t hnd, BIN_RECORD *bin, uint32_t mask, DEPTH_RECORD *depth_array)
{
#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (pfm_error = recompute_bin_values (hnd, bin, mask, depth_array));
}



/***************************************************************************/
/*!

  - Module Name:        change_depth_record_avec

  - Programmer(s):      Jan C. Depner

  - Date Written:       April 2000

  - Purpose:            Scans the depth records associated with a given bin
                        for a match with the input depth record.  The match
                        is based on the file, ping, and beam number from
                        the input depth record.  The actual depth value is
                        changed.  So far this is only used for the SHOALS
                        depth swap.  Hopefully that's all it will be used
                        for.  The only fields that need to be filled in are
                        the file, ping, beam, and depth.  This function is
                        only used internal to the library.

  - NOTE:               Now (post 4.7) we are using this function to modify
                        attribute values and H/V error values as well as
                        depths.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - CHANGE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_OUT_OF_RANGE_ERROR
                        - FILE_PING_BEAM_MISMATCH_ERROR

****************************************************************************/

static int32_t change_depth_record_avec (int32_t hnd, DEPTH_RECORD *depth, uint8_t mod)
{
  int32_t             i, ping_number, record_pos;
  int16_t             file_number, beam_number;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Check the sounding for out of range condition.  */

  if (depth->xyz.z >= hd[hnd].head.null_depth || depth->xyz.z <= -hd[hnd].depth_offset)
    {
      sprintf (pfm_err_str, "Depth value %f out of range in change_depth_record", depth->xyz.z);
      return (pfm_error = CHANGE_DEPTH_RECORD_OUT_OF_RANGE_ERROR);
    }


  if (read_bin_record_index (hnd, depth->coord, &bin_record[hnd]))
    {
      sprintf (pfm_err_str, "Error reading bin record in change_depth_record");
      return (pfm_error = CHANGE_DEPTH_RECORD_READ_BIN_RECORD_ERROR);
    }


  /*  Set the depth block address.  */

  depth_record_address[hnd] = depth->address.block;


  /*  Set the buffer position.  */

  record_pos = depth->address.record * dep_off[hnd].single_point_bits;


  /*  See if we need to flush the depth buffer.  */

  if (depth_record_address[hnd] != previous_depth_block[hnd])
    {
      if (depth_record_modified[hnd]) write_depth_buffer (hnd, previous_depth_block[hnd]);

      lfseek (ndx_file_hnd[hnd], depth_record_address[hnd], SEEK_SET);

      if (!lfread (depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]))
        {
          sprintf (pfm_err_str, "Error reading depth record");
          return (pfm_error = UPDATE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR);
        }


      /*  Save the previous block address.  */

      previous_depth_block[hnd] = depth_record_address[hnd];
    }


  /*  Unpack the file, ping, and beam numbers from the 'physical' record.  */

  file_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].file_number_pos + record_pos, hd[hnd].file_number_bits);

  ping_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].ping_number_pos + record_pos, hd[hnd].ping_number_bits);

  beam_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].beam_number_pos + record_pos, hd[hnd].beam_number_bits);


  /*  Check for a match, if we got one update the status and the depth.  */

  if (file_number == depth->file_number && ping_number == depth->ping_number && beam_number == depth->beam_number)
    {
      /*  Set the PFM_MODIFIED bit in the validity bits.  */

      if (mod == NVTrue) depth->validity |= PFM_MODIFIED;


      pfm_bit_pack (depth_record_data[hnd], dep_off[hnd].depth_pos + record_pos, hd[hnd].depth_bits,
                    NINT ((depth->xyz.z + hd[hnd].depth_offset) * hd[hnd].depth_scale));

      pfm_bit_pack (depth_record_data[hnd], dep_off[hnd].validity_pos + record_pos, hd[hnd].validity_bits, depth->validity);


      /*  Attributes and error values if present.  */

      for (i = 0 ; i < hd[hnd].head.num_ndx_attr ; i++)
        {
          pfm_bit_pack (depth_record_data[hnd], dep_off[hnd].attr_pos[i] + record_pos, hd[hnd].ndx_attr_bits[i],
                        NINT ((depth->attr[i] - hd[hnd].head.min_ndx_attr[i]) * hd[hnd].head.ndx_attr_scale[i]));
        }

      if (hd[hnd].horizontal_error_bits)
        {
          if (depth->horizontal_error >= hd[hnd].horizontal_error_null) depth->horizontal_error = hd[hnd].horizontal_error_null;
          pfm_bit_pack (depth_record_data[hnd], dep_off[hnd].horizontal_error_pos + record_pos, hd[hnd].horizontal_error_bits,
                        NINT (depth->horizontal_error * hd[hnd].head.horizontal_error_scale));
        }

      if (hd[hnd].vertical_error_bits)
        {
          if (depth->vertical_error >= hd[hnd].vertical_error_null) depth->vertical_error = hd[hnd].vertical_error_null;
          pfm_bit_pack (depth_record_data[hnd], dep_off[hnd].vertical_error_pos + record_pos, hd[hnd].vertical_error_bits,
                        NINT (depth->vertical_error * hd[hnd].head.vertical_error_scale));
        }


      depth_record_modified[hnd] = NVTrue;


      /*  Set the modified flag.  */

      if (mod == NVTrue)
        {
          /*  Set the modified bit in the associated bin record.  */

          bin_record[hnd].validity |= PFM_MODIFIED;
          write_bin_record_validity_index (hnd, &bin_record[hnd], PFM_MODIFIED);
        }
    }
  else
    {
      sprintf (pfm_err_str, "change_depth_record:File, ping, or beam numbers did not match.\n%d %d %d - %d %d %d\n\n\n", file_number, ping_number,
               beam_number, depth->file_number, depth->ping_number, depth->beam_number);
      return (pfm_error = FILE_PING_BEAM_MISMATCH_ERROR);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        change_depth_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       April 2000

  - Purpose:            Scans the depth records associated with a given bin
                        for a match with the input depth record.  The match
                        is based on the file, ping, and beam number from
                        the input depth record.  The actual depth value is
                        changed.  So far this is only used for the SHOALS
                        depth swap.  Hopefully that's all it will be used
                        for.  The only fields that need to be filled in are
                        the file, ping, beam, and depth.  This function is
                        only used internal to the library.

  - NOTE:               Now (post 4.7) we are using this function to modify
                        attribute values and H/V error values as well as
                        depths.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - CHANGE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_OUT_OF_RANGE_ERROR
                        - FILE_PING_BEAM_MISMATCH_ERROR

  - Caveats:            This was the original function but it was replaced by
                        change_depth_record_avec so that we could also have
                        change_record_nomod.

****************************************************************************/

static int32_t change_depth_record (int32_t hnd, DEPTH_RECORD *depth)
{
  return change_depth_record_avec (hnd, depth, NVTrue);
}



/***************************************************************************/
/*!

  - Module Name:        change_depth_record_nomod

  - Programmer(s):      Jan C. Depner

  - Date Written:       April 2000

  - Purpose:            Scans the depth records associated with a given bin
                        for a match with the input depth record.  The match
                        is based on the file, ping, and beam number from
                        the input depth record.  The actual depth value is
                        changed.  So far this is only used for the SHOALS
                        depth swap.  Hopefully that's all it will be used
                        for.  The only fields that need to be filled in are
                        the file, ping, beam, and depth.  This function is
                        only used internal to the library.

  - NOTE:               Now (post 4.7) we are using this function to modify
                        attribute values and H/V error values as well as
                        depths.

  - ANOTHER NOTE:       This function does NOT set the PFM_MODIFIED bit.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - CHANGE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_OUT_OF_RANGE_ERROR
                        - FILE_PING_BEAM_MISMATCH_ERROR

****************************************************************************/

static int32_t change_depth_record_nomod (int32_t hnd, DEPTH_RECORD *depth)
{
  return change_depth_record_avec (hnd, depth, NVFalse);
}



/***************************************************************************/
/*!

  - Module Name:        change_depth_record_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       April 2000

  - Purpose:            Scans the depth records associated with a given bin
                        for a match with the input depth record.  The match
                        is based on the file, ping, and beam number from
                        the input depth record.  The actual depth value is
                        changed.  So far this is only used for the SHOALS
                        depth swap.  Hopefully that's all it will be used
                        for.  The only fields that need to be filled in are
                        the file, ping, beam, depth and index coordinates
                        (depth.coord).

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - CHANGE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_NO_DEPTH_DATA
                        - CHANGE_DEPTH_RECORD_OUT_OF_RANGE_ERROR

****************************************************************************/

int32_t change_depth_record_index (int32_t hnd, DEPTH_RECORD *depth)
{
  return (pfm_error = change_depth_record (hnd, depth));
}



/***************************************************************************/
/*!

  - Module Name:        change_depth_record_nomod_index

  - Programmer(s):      Jan C. Depner

  - Date Written:       April 2000

  - Purpose:            Scans the depth records associated with a given bin
                        for a match with the input depth record.  The match
                        is based on the file, ping, and beam number from
                        the input depth record.  The actual depth value is
                        changed.  So far this is only used for the SHOALS
                        depth swap.  Hopefully that's all it will be used
                        for.  The only fields that need to be filled in are
                        the file, ping, beam, depth and index coordinates
                        (depth.coord).

  - NOTE:               This function does NOT set the PFM_MODIFIED bit.

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - CHANGE_DEPTH_RECORD_READ_BIN_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR
                        - CHANGE_DEPTH_RECORD_NO_DEPTH_DATA
                        - CHANGE_DEPTH_RECORD_OUT_OF_RANGE_ERROR

****************************************************************************/

int32_t change_depth_record_nomod_index (int32_t hnd, DEPTH_RECORD *depth)
{
  return (pfm_error = change_depth_record_nomod (hnd, depth));
}



/***************************************************************************/
/*!

  - Module Name:        change_bin_attribute_records

  - Programmer(s):      Jan C. Depner, Graeme Sweet

  - Date Written:       December 2002

  - Purpose:            Scans the depth records associated with a given bin
                        for a match with the input depth record.  The match
                        is based on the file, ping, and beam number from
                        the input depth record.  The attribute values are
                        changed.  The only fields that need to be filled
                        in are the file, ping, beam, attribute(s) and index
                        coordinates (depth.coord).

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - CHANGE_ATTRIBUTE_RECORD_READ_BIN_RECORD_ERROR
                        - CHANGE_ATTRIBUTE_RECORD_READ_DEPTH_RECORD_ERROR
                        - CHANGE_ATTRIBUTE_RECORD_NO_DEPTH_DATA
                        - CHANGE_ATTRIBUTE_RECORD_OUT_OF_RANGE_ERROR

****************************************************************************/

static int32_t change_bin_attribute_records (int32_t hnd, DEPTH_RECORD *depth)
{
  int32_t             i, ping_number, record_pos;
  int16_t             file_number, beam_number;
  int32_t             error = SUCCESS;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  /*  Check the sounding for out of range condition.  */
  /*  Returns out of range, but still saves the attributes. */

  for (i = 0 ; i < hd[hnd].head.num_ndx_attr ; i++)
    {
      if ((depth->attr[i] < hd[hnd].head.min_ndx_attr[i]) || (depth->attr[i] > hd[hnd].head.max_ndx_attr[i]))
        {
          sprintf (pfm_err_str, "Attribute value %f out of range in change_attribute_record", depth->attr[i]);
          error = CHANGE_ATTRIBUTE_RECORD_OUT_OF_RANGE_ERROR;
        }
    }


  if (read_bin_record_index (hnd, depth->coord, &bin_record[hnd]))
    {
      sprintf (pfm_err_str, "Error reading bin record in change_depth_record");
      return (pfm_error = CHANGE_ATTRIBUTE_RECORD_READ_BIN_RECORD_ERROR);
    }


  /*  Set the depth block address.  */

  depth_record_address[hnd] = depth->address.block;


  /*  Set the buffer position.  */

  record_pos = depth->address.record * dep_off[hnd].single_point_bits;


  /*  See if we need to flush the depth buffer.  */

  if (depth_record_address[hnd] != previous_depth_block[hnd])
    {
      if (depth_record_modified[hnd]) write_depth_buffer (hnd, previous_depth_block[hnd]);

      lfseek (ndx_file_hnd[hnd], depth_record_address[hnd], SEEK_SET);

      if (!lfread (depth_record_data[hnd], dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]))
        {
          sprintf (pfm_err_str, "Error reading depth record");
          return (pfm_error = CHANGE_ATTRIBUTE_RECORD_READ_DEPTH_RECORD_ERROR);
        }


      /*  Save the previous block address.  */

      previous_depth_block[hnd] = depth_record_address[hnd];
    }


  /*  Unpack the file, ping, and beam numbers from the 'physical' record.  */

  file_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].file_number_pos + record_pos, hd[hnd].file_number_bits);

  ping_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].ping_number_pos + record_pos, hd[hnd].ping_number_bits);

  beam_number = pfm_bit_unpack (depth_record_data[hnd], dep_off[hnd].beam_number_pos + record_pos, hd[hnd].beam_number_bits);


  /*  Check for a match, if we got one update the status and the depth.  */

  if (file_number == depth->file_number && ping_number == depth->ping_number && beam_number == depth->beam_number)
    {
      /*  Set the PFM_MODIFIED bit in the validity bits.  */

      depth->validity |= PFM_MODIFIED;

      for (i = 0 ; i < hd[hnd].head.num_ndx_attr ; i++)
        {
          pfm_bit_pack (depth_record_data[hnd], dep_off[hnd].attr_pos[i] + record_pos, hd[hnd].ndx_attr_bits[i],
                        NINT ((depth->attr[i] - hd[hnd].head.min_ndx_attr[i]) * hd[hnd].head.ndx_attr_scale[i]));
        }

      pfm_bit_pack (depth_record_data[hnd], dep_off[hnd].validity_pos + record_pos, hd[hnd].validity_bits, depth->validity);


      /*  Set the modified flag.  */

      depth_record_modified[hnd] = NVTrue;


      /*  Set the modified bit in the associated bin record.  */

      bin_record[hnd].validity |= PFM_MODIFIED;
      write_bin_record_validity_index (hnd, &bin_record[hnd], PFM_MODIFIED);
    }
  else
    {
      sprintf (pfm_err_str, "change_attribute_record:File, ping, or beam numbers did not match.\n%d %d %d - %d %d %d\n\n\n", file_number, ping_number,
               beam_number, depth->file_number, depth->ping_number, depth->beam_number);
      return (pfm_error = FILE_PING_BEAM_MISMATCH_ERROR);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (pfm_error = error);
}



/*!  This function is deprecated, use change_bin_attribute_records_index instead.  */

int32_t change_attribute_record_index (int32_t hnd, DEPTH_RECORD *depth)
{
  return (pfm_error = change_bin_attribute_records (hnd, depth));
}



/***************************************************************************/
/*!

  - Module Name:        change_bin_attribute_records_index

  - Programmer(s):      Jan C. Depner, Graeme Sweet

  - Date Written:       December 2002

  - Purpose:            Scans the depth records associated with a given bin
                        for a match with the input depth record.  The match
                        is based on the file, ping, and beam number from
                        the input depth record.  The attribute values are
                        changed.  The only fields that need to be filled
                        in are the file, ping, beam, attribute(s) and index
                        coordinates (depth.coord).

  - Arguments:
                        - hnd             =   PFM file handle
                        - depth           =   DEPTH_RECORD structure

  - Return Value:
                        - SUCCESS
                        - CHANGE_ATTRIBUTE_RECORD_READ_BIN_RECORD_ERROR
                        - CHANGE_ATTRIBUTE_RECORD_READ_DEPTH_RECORD_ERROR
                        - CHANGE_ATTRIBUTE_RECORD_NO_DEPTH_DATA
                        - CHANGE_ATTRIBUTE_RECORD_OUT_OF_RANGE_ERROR

****************************************************************************/

int32_t change_bin_attribute_records_index (int32_t hnd, DEPTH_RECORD *depth)
{
  return (pfm_error = change_bin_attribute_records (hnd, depth));
}



/***************************************************************************/
/*!

  - Module Name:        read_line_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 2001

  - Purpose:            Reads a line descriptor from the line list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - line_number     =   Number of the entry in the file

  - Return Value:
                        - char            =   Line descriptor

****************************************************************************/

char *read_line_file (int32_t hnd, int16_t line_number)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  if (line_file_fp[hnd] == NULL)
    {
      sprintf (pfm_err_str, "No line file available");
      return ("UNDEFINED");
    }


  fseek (line_file_fp[hnd], line_file_index[hnd][line_number], SEEK_SET);


  if ((pfm_ngets (read_line_file_string[hnd], sizeof (read_line_file_string[hnd]), line_file_fp[hnd])) == NULL)
    {
      sprintf (pfm_err_str, "Error reading line file");
      return ("UNDEFINED");
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  return (read_line_file_string[hnd]);
}



/***************************************************************************/
/*!

  - Module Name:        write_line_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       July 2001

  - Purpose:            Writes a line descriptor to the end of the line
                        list file.

  - Arguments:
                        - hnd             =   PFM file handle
                        - line            =   Line descriptor

  - Return Value:
                        - int16_t         =   Line number in the line list

****************************************************************************/

int16_t write_line_file (int32_t hnd, char *line)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif


  fseek (line_file_fp[hnd], 0, SEEK_END);


  fprintf (line_file_fp[hnd], "%s\n", line);


  line_file_index[hnd][line_file_count[hnd]] = ftell (line_file_fp[hnd]);
  line_file_count[hnd]++;

  fflush (line_file_fp[hnd]);

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd); fflush (stderr);
#endif

  return (line_file_count[hnd] - 1);
}



/***************************************************************************/
/*!

  - Module Name:        pfm_error_str

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1999

  - Purpose:            Returns an error message.

  - Arguments:
                        - status          =   error status (see pfm.h)

  - Return Value:
                        - void

****************************************************************************/

char *pfm_error_str (int32_t status)
{
  return (pfm_err_str);
}




/***************************************************************************/
/*!

  - Module Name:        pfm_error_exit

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1999

  - Purpose:            Exits from the calling program after printing out
                        an error explanation.

  - Arguments:
                        - status          =   error status (see pfm.h)

  - Return Value:
                        - void

****************************************************************************/

void pfm_error_exit (int32_t status)
{
  fprintf (stderr, "\n\n\n\t%s\n", pfm_error_str (status));

  exit (-1);
}



/***************************************************************************/
/*!

  - Module Name:        pfm_error_report

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1999

  - Purpose:            Prints out an error explanation.

  - Arguments:
                        - status          =   error status (see pfm.h)

  - Return Value:
                        - void

****************************************************************************/

void pfm_error_report (int32_t status)
{
  fprintf (stderr, "\n\n\n\t%s\n", pfm_error_str (status));
}



/***************************************************************************/
/*!
  - Module Name:        PointInsidePolygon

  - Programmer(s):      Jan C. Depner

  - Date Written:       December 2014

  - Purpose:            Based on the CrossingsMultiplyTest method described
                        by E. Haines:
                        http://tog.acm.org/resources/GraphicsGems/gemsiv/ptpoly_haines/ptinpoly.c


  - Arguments:
                        - poly     =   Array of NV_F64_COORD2 structures holding
                                       the polygon points
                        - npol     =   Number of points in the polygon
                        - x        =   X value of point to be tested
                        - y        =   Y value of point to be tested

  - Return Value:       1 if point is inside polygon, 0 if outside

  - Caveats:            The last point of the polygon must be the same as
                        the first point and the vertices of the polygon must
			be in order around the polygon.

****************************************************************************/

int32_t PointInsidePolygon (NV_F64_COORD2 *poly, int32_t npol, double x, double y)
{
  int32_t    i, j, yflag0, yflag1, inside_flag;

  inside_flag = 0;


  j = npol - 1;
  yflag0 = (poly[j].y >= y);

  for (i = 0 ; i < npol ; i++)
    {
      yflag1 = (poly[i].y >= y);


      if (yflag0 != yflag1)
	{
	  if (((poly[i].y - y) * (poly[j].x - poly[i].x) >= (poly[i].x - x) * (poly[j].y - poly[i].y)) == yflag1)
	    {
	      inside_flag = !inside_flag;
	    }
	}

      yflag0 = yflag1;
      j = i;
    }

  return (inside_flag);
}



/***************************************************************************/
/*!

  - Module Name:        bin_inside

  - Programmer(s):      Unknown

  - Date Written:       Unknown

  - Modified:           Jan C. Depner, ported to C.

  - Date Modified:      August 1992

  - Purpose:            Checks a geographic point to see if it falls within
                        the specified polygon.

  - Arguments:
                        - bin                 =   bin header
                        - xy                  =   position of point

  - Return Value:
                        - 1 if inside
                        - 0 if not

****************************************************************************/

int32_t bin_inside (BIN_HEADER bin, NV_F64_COORD2 xy)
{
  return (bin_inside_ptr (&bin, xy));
}



/***************************************************************************/
/*!

  - Module Name:        bin_inside_ptr

  - Programmer(s):      Unknown

  - Date Written:       Unknown

  - Modified:           Jan C. Depner, ported to C.

  - Date Modified:      August 1992

  - Purpose:            Checks a geographic point to see if it falls within
                        the specified polygon.

  - NOTE:               Passes the bin header as a pointer for speed.

  - Arguments:
                        - bin                 =   bin header
                        - xy                  =   position of point

  - Return Value:
                        - 1 if inside
                        - 0 if not

****************************************************************************/

int32_t bin_inside_ptr (BIN_HEADER *bin, NV_F64_COORD2 xy)
{
  /*  There have to be at least three points in the polygon.          */

  if (bin->polygon_count > 2)
    {
      return (PointInsidePolygon (bin->polygon, bin->polygon_count, xy.x, xy.y));
    }

  return (0);
}



/***************************************************************************/
/*!

  - Module Name:        pfm_geo_distance

  - Programmer(s):      Jan C. Depner

  - Date Written:       October 2007

  - Purpose:            Estimate distance in a geographic PFM (not
                        projected or equal lat/lon bins) using the actual
                        longitudinal bin sizes at each latitude bin
                        boundary.

  - Method:             What we do is pre-compute the X bin sizes when we
                        first call this function for a PFM.  Then we
                        compute the X bin size at each of the two positions
                        and assume that the X space is uniform having an X
                        bin size of the average of these two sizes.  This
                        gives a reasonable approximation of distance.  For
                        example, at 30 degrees north, with a 2 meter PFM
                        grid size, computations of distances in the PFM of
                        up to 10,000 meters yield differences between this
                        and the results from envgp of less than 0.0001
                        meters.  At 60 degrees north we still get
                        differences of less than 0.0001 meters.  At 60
                        degrees north, with a PFM bin size of 10 meters, we
                        get a difference of 0.06 meters when measuring a
                        distance of 50,000 meters.  This method of
                        computing distance is about 8 times faster than
                        using invgp.  A test run of 500,000,000 distance
                        computations using invgp took 12 minutes an 1
                        second while the same test took 1 minute and 28
                        seconds using pfm_geo_distance.

  - Arguments:
                        - hnd             =   PFM file handle
                        - lat0            =   latitude of first point
                        - lon0            =   longitude of first point
                        - lat1            =   latitude of second point
                        - lon1            =   longitude of first point
                        - *distance       =   distance between the points

  - Return Value:
                        - SUCCESS
                        - PFM_GEO_DISTANCE_NOT_GEOGRAPHIC_ERROR
                        - PFM_GEO_DISTANCE_LATLON_PFM_ERROR
                        - PFM_GEO_DISTANCE_ALLOCATE_ERROR
                        - PFM_GEO_DISTANCE_OUT_OF_BOUNDS

****************************************************************************/

int32_t pfm_geo_distance (int32_t hnd, double lat0, double lon0, double lat1, double lon1,
                           double *distance)
{
  int32_t                 i;
  double                  az, x_dist[2], x[2], y[2], x_bin_size, next_lat;
  NV_I32_COORD2           coord[2];


  /*  The first time we open a PFM we need to compute the actual X bin size at each Y bin boundary.  */

  if (!geo_dist_init[hnd])
    {
      /*  Error out if this is a projected PFM.  */

      if (bin_header[hnd].proj_data.projection)
        {
          sprintf (pfm_err_str, "pfm_geo_distance does not work with projected PFM structures");
          return (pfm_error = PFM_GEO_DISTANCE_NOT_GEOGRAPHIC_ERROR);
        }


      /*  Error out if this is a lat/lon unprojected PFM.  */

      if (bin_header[hnd].bin_size_xy == 0.0)
        {
          sprintf (pfm_err_str, "pfm_geo_distance does not work with equal lat/lon PFM structures");
          return (pfm_error = PFM_GEO_DISTANCE_LATLON_PFM_ERROR);
        }


      /*  Allocate the memory.  */

      geo_distance[hnd] = (double *) calloc (bin_header[hnd].bin_height + 2, sizeof (double));
      if (geo_distance[hnd] == NULL)
        {
          sprintf (pfm_err_str, "Unable to allocate geo_distance array in pfm_geo_distance");
          return (PFM_GEO_DISTANCE_ALLOCATE_ERROR);
        }

      geo_post[hnd] = (double *) calloc (bin_header[hnd].bin_height + 2, sizeof (double));
      if (geo_post[hnd] == NULL)
        {
          sprintf (pfm_err_str, "Unable to allocate geo_post array in pfm_geo_distance");
          return (PFM_GEO_DISTANCE_ALLOCATE_ERROR);
        }


      /*  Get the incremental distances.  Go one extra to cover points on the upper boundary.  */

      for (i = 0 ; i <= bin_header[hnd].bin_height + 1 ; i++)
        {
          /*  Get the latitude "post" positions.  */

          geo_post[hnd][i] = bin_header[hnd].mbr.min_y + (double) i * bin_header[hnd].y_bin_size_degrees;


          /*  Compute the actual X bin size at this lat band.  */

          pfm_invgp (PFM_A0, PFM_B0, geo_post[hnd][i], bin_header[hnd].mbr.min_x, geo_post[hnd][i], bin_header[hnd].mbr.min_x +
                     bin_header[hnd].x_bin_size_degrees, &geo_distance[hnd][i], &az);
        }

      geo_dist_init[hnd] = NVTrue;
    }


  /*  Check the points.  */

  if (lon0 > bin_header[hnd].mbr.max_x || lon0 < bin_header[hnd].mbr.min_x ||
      lat0 > bin_header[hnd].mbr.max_y || lat0 < bin_header[hnd].mbr.min_y ||
      lon1 > bin_header[hnd].mbr.max_x || lon1 < bin_header[hnd].mbr.min_x ||
      lat1 > bin_header[hnd].mbr.max_y || lat1 < bin_header[hnd].mbr.min_y)
    {
      sprintf (pfm_err_str, "One of the two points passed to pfm_geo_distance was outside the PFM bounds");
      return (PFM_GEO_DISTANCE_OUT_OF_BOUNDS);
    }


  /*  Compute our own indices so we can deal with round-off.  */

  coord[0].x = (int32_t) ((double) (lon0 - bin_header[hnd].mbr.min_x) /
                           (double) bin_header[hnd].x_bin_size_degrees + 0.05);
  coord[0].y = (int32_t) ((double) (lat0 - bin_header[hnd].mbr.min_y) /
                           (double) bin_header[hnd].y_bin_size_degrees + 0.05);

  coord[1].x = (int32_t) ((double) (lon1 - bin_header[hnd].mbr.min_x) /
                           (double) bin_header[hnd].x_bin_size_degrees + 0.05);
  coord[1].y = (int32_t) ((double) (lat1 - bin_header[hnd].mbr.min_y) /
                           (double) bin_header[hnd].y_bin_size_degrees + 0.05);


  /*  Get the Y positions in "meters".  */

  y[0] = ((double) coord[0].y + (lat0 - geo_post[hnd][coord[0].y]) / bin_header[hnd].y_bin_size_degrees) *
    bin_header[hnd].bin_size_xy;
  y[1] = ((double) coord[1].y + (lat1 - geo_post[hnd][coord[1].y]) / bin_header[hnd].y_bin_size_degrees) *
    bin_header[hnd].bin_size_xy;


  /*  Get the X positions in "meters" adjusted for the change in Y.  Interpolating the value between the
      posts on either side of the lat.  This is probably serious overkill but not too computationally
      taxing.  */

  next_lat = geo_post[hnd][coord[0].y] + bin_header[hnd].y_bin_size_degrees;
  x_dist[0] = geo_distance[hnd][coord[0].y] + (geo_distance[hnd][coord[0].y + 1] - geo_distance[hnd][coord[0].y]) *
    ((lat0 - geo_post[hnd][coord[0].y]) / (next_lat - geo_post[hnd][coord[0].y]));

  next_lat = geo_post[hnd][coord[1].y] + bin_header[hnd].y_bin_size_degrees;
  x_dist[1] = geo_distance[hnd][coord[1].y] + (geo_distance[hnd][coord[1].y + 1] - geo_distance[hnd][coord[1].y]) *
    ((lat1 - geo_post[hnd][coord[1].y]) / (next_lat - geo_post[hnd][coord[1].y]));


  x_bin_size = (x_dist[1] + x_dist[0]) / 2.0;


  x[0] = ((lon0 - bin_header[hnd].mbr.min_x) / bin_header[hnd].x_bin_size_degrees) * x_bin_size;
  x[1] = ((lon1 - bin_header[hnd].mbr.min_x) / bin_header[hnd].x_bin_size_degrees) * x_bin_size;


  /*  Damn, this looks familiar doesn't it?  I wonder what it is?  */

  *distance = sqrt ((x[1] - x[0]) * (x[1] - x[0]) + (y[1] - y[0]) * (y[1] - y[0]));


  return (pfm_error = SUCCESS);
}



/****************************************************************************\
 *
 * The pfm_internals functions, mostly just getters.
 *
\****************************************************************************/

float get_depth_precision(int32_t hnd)
{
    float       precision;

    precision = 1.0f / hd[hnd].depth_scale;

    return precision;
}

float get_std_precision(int32_t hnd)
{
    float       precision;

    precision = 1.0f / hd[hnd].std_scale;

    return precision;
}

float get_x_precision(int32_t hnd)
{
    float precision;

    precision = bin_header[hnd].x_bin_size_degrees / x_offset_scale[hnd];

    return precision;
}

float get_y_precision(int32_t hnd)
{
    float precision;

    precision = bin_header[hnd].y_bin_size_degrees / y_offset_scale[hnd];

    return precision;
}



/***************************************************************************\
 *
 *  Included the cached functions.
 *
\***************************************************************************/

#include "pfm_cached_io.c"
