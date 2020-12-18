
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



/***************************************************************************\

    Even though this has my name on most of it, the majority of this code
    was done by the people at IVS.  I don't know exactly who did what as I
    got the code from Jeff Parker at SAIC but I'm pretty sure Will Burrow, 
    Danny Neville, and Graeme Sweet had a lot to do with it.  This is some
    seriously cool stuff!

    Jan Depner, 06/21/07

\***************************************************************************/

/* Comment out the define to enable the assert statement in destroy_bin_cache(). */
/*
#define NDEBUG
#include <assert.h>
#include <inttypes.h>
*/


#define MEM_DEBUG         0


static int32_t /*size_t*/       pfm_cache_size_max = 400000000;

static int32_t                  cache_max_rows[MAX_PFM_FILES];
static int32_t                  cache_max_cols[MAX_PFM_FILES];
static int32_t                  max_offset_rows[MAX_PFM_FILES];
static int32_t                  max_offset_cols[MAX_PFM_FILES];
static int32_t                  new_cache_max_rows[MAX_PFM_FILES];
static int32_t                  new_cache_max_cols[MAX_PFM_FILES];
static int32_t                  new_center_row[MAX_PFM_FILES];
static int32_t                  new_center_col[MAX_PFM_FILES];
static int32_t                  use_cov_flag[MAX_PFM_FILES];

static uint8_t                  bin_cache_empty[MAX_PFM_FILES];
static BIN_RECORD_SUMMARY       ***bin_cache_rows[MAX_PFM_FILES];
static int32_t /*size_t*/       pfm_cache_size[MAX_PFM_FILES];
static int32_t /*size_t*/       pfm_cache_size_peak[MAX_PFM_FILES];
static int32_t                  offset_rows[MAX_PFM_FILES];
static int32_t                  offset_cols[MAX_PFM_FILES];

static int32_t                  pfm_cache_hit[MAX_PFM_FILES];
static int32_t                  pfm_cache_miss[MAX_PFM_FILES];
static int32_t                  pfm_cache_flushes[MAX_PFM_FILES];
static int32_t                  force_cache_reset[MAX_PFM_FILES];


/*  This is used in write_cached_bin_record.  */

static BIN_RECORD               *tmp_bin[MAX_PFM_FILES];


/*  This is used in dump_cached_records.  */

static int32_t                  dump_cached_records_count[MAX_PFM_FILES];


/*  These are used in read_cached_bin_record and loop_read_cached_bin_record.  */

static int32_t                  cacheRow[MAX_PFM_FILES];
static int32_t                  cacheCol[MAX_PFM_FILES];


/***************************************************************************/
/*!

  - Module Name:        open_cached_pfm_file

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Opens all of the associated files.

  - Restrictions:       This function will try to open and append to an
                        existing PFM list file.  If the specified list file
                        does not exist a new PFM structure will be created.
                        In this case all of the arguments in the
                        PFM_OPEN_ARGS structure need to be set as well as
                        the following members of the PFM_OPEN_ARGS.head
                        structure :
                            - bin_size_xy or (x_bin_size_degrees and
                              y_bin_size_degrees while setting bin_size_xy
                              to 0.0)
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
                            - max_input_files (set to 0 for default)
                            - max_input_lines (set to 0 for default)
                            - max_input_pings (set to 0 for default)
                            - max_input_beams (set to 0 for default)

                        If bin_size_xy is 0.0 then we will use the
                        bin_sizes in degrees to define the final bin size
                        and the area dimensions.  This is useful for
                        defining PFMs for areas where you want to use
                        matching latitudinal and longitudinal bin sizes.
                        Pay special attention to checkpoint.  This can be
                        used to save/recover your file on an aborted load.
                        See pfm.h for better descriptions of these values.
                        <br><br>
                        IMPORTANT NOTE : For post 4.7 PFM structures the
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

  - Arguments:          open_args       -   check pfm.h

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

  - Caveats:                This code was actually developed by IVS in 
                            2007.  It was later modified by SAIC.  The
                            suspected culprits are listed below ;-)
                                - Danny Neville (IVS)
                                - Graeme Sweet (IVS)
                                - William Burrow (IVS)
                                - Jeff Parker (SAIC)
                                - Webb McDonald (SAIC)

  - Warning:                This function is not thread safe.

****************************************************************************/

int32_t open_cached_pfm_file (PFM_OPEN_ARGS *open_args)
{
  int32_t    hnd;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d\n",__FILE__,__FUNCTION__,__LINE__);fflush (stderr);
#endif


  hnd = open_pfm_file( open_args );

  if( hnd != -1 )
    {
      cache_max_rows[hnd] = 4000;
      cache_max_cols[hnd] = 4000;
      max_offset_rows[hnd] = 2000;
      max_offset_cols[hnd] = 2000;
      new_cache_max_rows[hnd] = 2000;
      new_cache_max_cols[hnd] = 2000;
      new_center_row[hnd] = -1;
      new_center_col[hnd] = -1;
      use_cov_flag[hnd] = 0;

      bin_cache_rows[hnd] = NULL;
      pfm_cache_size[hnd] = 0;
      pfm_cache_size_peak[hnd] = 0;
      bin_cache_empty[hnd] = NVTrue;

      pfm_cache_hit[hnd] = 0;
      pfm_cache_miss[hnd] = 0;
      pfm_cache_flushes[hnd] = 0;
      force_cache_reset[hnd] = 0;

      tmp_bin[hnd] = NULL;

      dump_cached_records_count[hnd] = 0;
    }

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__, hnd);fflush (stderr);
#endif

  return hnd;
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

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

  - Warning:            This function is not thread safe.

****************************************************************************/

void close_cached_pfm_file (int32_t hnd)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  flush_bin_cache (hnd);

  close_pfm_file (hnd);

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

}



/***************************************************************************/
/*!

  - Module Name:        loop_read_cached_bin_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       January 2012

  - Purpose:            See read_cached_bin_record and IMPORTANT NOTE.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin             =   BIN_RECORD structure
                        - address         =   Address of record

  - Return Value:
                        - SUCCESS
                        - READ_BIN_RECORD_DATA_READ_ERROR

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

static int32_t loop_read_cached_bin_record (int32_t hnd, NV_I32_COORD2 coord, BIN_RECORD_SUMMARY **bin_summary)
{
  int64_t            address;
  int32_t            i;
  BIN_RECORD         tempBin;
  uint8_t            cov = 0;



#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


  /* Set up the cache index pointers. */

  if (bin_cache_rows[hnd] == NULL )
    {
      force_cache_reset[hnd] = 0;
        
      if ((new_center_row[hnd] < 0) || (new_center_col[hnd] < 0))
        {
          new_center_row[hnd] = coord.y;
          new_center_col[hnd] = coord.x;
        }

      if ((new_cache_max_rows[hnd] != cache_max_rows[hnd]) || (new_cache_max_cols[hnd] != cache_max_cols[hnd]))
        {
          cache_max_rows[hnd]  = new_cache_max_rows[hnd];
          cache_max_cols[hnd]  = new_cache_max_cols[hnd];
          max_offset_rows[hnd] = (int32_t)(cache_max_rows[hnd] * 0.5);
          max_offset_cols[hnd] = (int32_t)(cache_max_cols[hnd] * 0.5);
        }

      bin_cache_rows[hnd] = (BIN_RECORD_SUMMARY ***) malloc (cache_max_rows[hnd] * sizeof (BIN_RECORD_SUMMARY **));

      for( i = 0; i < cache_max_rows[hnd]; i++ )
        {
          bin_cache_rows[hnd][i] = NULL;
        }

      bin_cache_empty[hnd] = NVTrue;

      pfm_cache_size[hnd] += cache_max_rows[hnd] * sizeof (BIN_RECORD_SUMMARY **);

#if MEM_DEBUG
      fprintf (stderr, "%s (Handle = %d):  Rows =    %p CacheSize = %d\n", __FUNCTION__, hnd, bin_cache_rows[hnd], pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];
    }

  if( bin_cache_empty[hnd] )
    {
      new_center_row[hnd] = coord.y;
      new_center_col[hnd] = coord.x;

      offset_rows[hnd] = (new_center_row[hnd] - cache_max_rows[hnd]) + max_offset_rows[hnd] + 1;

      if (offset_rows[hnd] < 0) offset_rows[hnd] = 0;


      offset_cols[hnd] = (new_center_col[hnd] - cache_max_cols[hnd]) + max_offset_cols[hnd] + 1;
      if (offset_cols[hnd] < 0) offset_cols[hnd] = 0;

      bin_cache_empty[hnd] = NVFalse;
    }

  cacheRow[hnd] = coord.y - offset_rows[hnd];
  cacheCol[hnd] = coord.x - offset_cols[hnd];


  if (bin_cache_rows[hnd][cacheRow[hnd]] == NULL )
    {
      bin_cache_rows[hnd][cacheRow[hnd]] = (BIN_RECORD_SUMMARY **) malloc (cache_max_cols[hnd] * sizeof (BIN_RECORD_SUMMARY *));
      for( i = 0; i < cache_max_cols[hnd]; i++ )
        {
          bin_cache_rows[hnd][cacheRow[hnd]][i] = NULL;
        }

      pfm_cache_size[hnd] +=  cache_max_cols[hnd] * sizeof (BIN_RECORD_SUMMARY *);

#if MEM_DEBUG
      fprintf (stderr, "%s (Handle = %d):  Row = %d Cols =           %p CacheSize = %d\n", __FUNCTION__, hnd, cacheRow[hnd],
               bin_cache_rows[hnd][cacheRow[hnd]], pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];
    }

  if( bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]] == NULL  || !bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]]->dirty )
    {
      pfm_cache_miss[hnd]++;
    }
  else
    {
      pfm_cache_hit[hnd]++;
    }

  if (bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]] == NULL )
    {
      /*  Allocate a new bin.  */

      if ((bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]] = (BIN_RECORD_SUMMARY *) malloc (sizeof (BIN_RECORD_SUMMARY))) == NULL)
        {
          sprintf (pfm_err_str, "Unable to allocate memory for bin record");
          return (pfm_error = SET_OFFSETS_BIN_MALLOC_ERROR);
        }

      pfm_cache_size[hnd] += sizeof (BIN_RECORD_SUMMARY);

#if MEM_DEBUG
      fprintf (stderr, "%s (Handle = %d):  RC = %d, %d Summary =    %p CacheSize = %d\n", __FUNCTION__, hnd, cacheRow[hnd], cacheCol[hnd],
               bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]], pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      memset( bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]], 0, sizeof( BIN_RECORD_SUMMARY ));
    }

  if ( ! bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]]->dirty )
    {
      /*  The bin has no data in it yet, fetch some from disk!  */

      bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]]->dirty = NVTrue;

      address = ((int64_t) coord.y * (int64_t) bin_header[hnd].bin_width + (int64_t)coord.x) * (int64_t) bin_off[hnd].record_size +
        hd[hnd].bin_header_size;

      lfseek (bin_file_hnd[hnd], address, SEEK_SET);


      /*  Read the record.  */

      if (!(lfread (bin_record_data[hnd], bin_off[hnd].record_size, 1, bin_file_hnd[hnd])))
        {
          sprintf (pfm_err_str, "Error reading from bin file");
          return (pfm_error = READ_BIN_RECORD_DATA_READ_ERROR);
        }

      unpack_bin_record (hnd, bin_record_data[hnd], &tempBin ); 

      *bin_summary = bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]];

      if (use_cov_flag[hnd])
        {
          read_cov_map_index (hnd, coord, &cov);
        }
      else
        {
          cov = 0;
        }

      (*bin_summary)->num_soundings = tempBin.num_soundings;
      (*bin_summary)->validity = tempBin.validity;
      (*bin_summary)->cov_flag = cov;
      (*bin_summary)->coord = coord;
      (*bin_summary)->depth.continuation_pointer = 0;
      (*bin_summary)->depth.record_pos = 0;
      (*bin_summary)->depth.buffers.depths = NULL;
      (*bin_summary)->depth.buffers.next = NULL;
      (*bin_summary)->depth.last_depth_buffer = NULL;
      (*bin_summary)->depth.previous_chain = 0;
      (*bin_summary)->depth.chain = tempBin.depth_chain;
    }
  else
    {
      *bin_summary = bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]];
    }

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        read_cached_bin_record

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

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

  - IMPORTANT NOTE:     This function was recursive!  It used to call itself.
                        According to the comments (and what I've been able
                        to check while debugging) it only ever did it once.
                        In the interest of making this thread safe I made an
                        almost identical function (loop_read_cached_bin_record
                        above) so that this function doesn't call itself.
                        JCD

****************************************************************************/

int32_t read_cached_bin_record (int32_t hnd, NV_I32_COORD2 coord, BIN_RECORD_SUMMARY **bin_summary)
{
  int64_t            address;
  int32_t            i;
  BIN_RECORD         tempBin;
  uint8_t            cov = 0;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


  /* Set up the cache index pointers. */

  if (bin_cache_rows[hnd] == NULL )
    {
      force_cache_reset[hnd] = 0;
        
      if ((new_center_row[hnd] < 0) || (new_center_col[hnd] < 0))
        {
          new_center_row[hnd] = coord.y;
          new_center_col[hnd] = coord.x;
        }

      if ((new_cache_max_rows[hnd] != cache_max_rows[hnd]) || (new_cache_max_cols[hnd] != cache_max_cols[hnd]))
        {
          cache_max_rows[hnd]  = new_cache_max_rows[hnd];
          cache_max_cols[hnd]  = new_cache_max_cols[hnd];
          max_offset_rows[hnd] = (int32_t) (cache_max_rows[hnd] * 0.5);
          max_offset_cols[hnd] = (int32_t) (cache_max_cols[hnd] * 0.5);
        }

      bin_cache_rows[hnd] = (BIN_RECORD_SUMMARY ***) malloc (cache_max_rows[hnd] * sizeof (BIN_RECORD_SUMMARY **));

      for( i = 0; i < cache_max_rows[hnd]; i++ )
        {
          bin_cache_rows[hnd][i] = NULL;
        }

      bin_cache_empty[hnd] = NVTrue;

      pfm_cache_size[hnd] +=  cache_max_rows[hnd] * sizeof (BIN_RECORD_SUMMARY **);

#if MEM_DEBUG
      fprintf ("%s (Handle = %d):  Rows =    %p CacheSize = %d\n", __FUNCTION__, hnd, bin_cache_rows[hnd], pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];
    }

  if( bin_cache_empty[hnd] )
    {
      new_center_row[hnd] = coord.y;
      new_center_col[hnd] = coord.x;

      offset_rows[hnd] = (new_center_row[hnd] - cache_max_rows[hnd]) + max_offset_rows[hnd] + 1;

      if (offset_rows[hnd] < 0) offset_rows[hnd] = 0;


      offset_cols[hnd] = (new_center_col[hnd] - cache_max_cols[hnd]) + max_offset_cols[hnd] + 1;
      if (offset_cols[hnd] < 0) offset_cols[hnd] = 0;

      bin_cache_empty[hnd] = NVFalse;
    }

  cacheRow[hnd] = coord.y - offset_rows[hnd];
  cacheCol[hnd] = coord.x - offset_cols[hnd];


  /*************************************************
   * Test for access outside the bounds of the cache.
   * CAUTION: recursive call - should only happen once though.
   */


  /*  Not anymore...  Now it calls loop_read_cache_bin_record to avoid the recursion.  */


  if( ( cacheRow[hnd] < 0 ) || ( cacheRow[hnd] >= cache_max_rows[hnd] ) || ( cacheCol[hnd] < 0 ) ||
      ( cacheCol[hnd] >= cache_max_cols[hnd] ) || force_cache_reset[hnd])
    {

#if MEM_DEBUG
      fprintf (stderr, "Extent of cache for handle %d exceeded in %s:  Cache Size = %d ", hnd, __FUNCTION__, pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      flush_bin_cache(hnd);


#if MEM_DEBUG
      fprintf (stderr, " => %d \n", pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      force_cache_reset[hnd] = 0;

      /*return read_cached_bin_record (hnd, coord, bin_summary);*/

      return loop_read_cached_bin_record (hnd, coord, bin_summary);
    }

  if ( pfm_cache_size[hnd] > pfm_cache_size_max )
    {
      fprintf (stderr, "\nMAX Cache Size (%d) reached for handle %d, resetting from %d\n", pfm_cache_size_max, hnd, pfm_cache_size[hnd]);
      fflush(stderr);

      flush_bin_cache(hnd);

      fprintf (stderr, "\nCache size reset to %d for handle %d\n", pfm_cache_size[hnd], hnd);
      fflush(stderr);

      /*return read_cached_bin_record (hnd, coord, bin_summary);*/
      return loop_read_cached_bin_record (hnd, coord, bin_summary);
    }


    /*************************************************/


  if (bin_cache_rows[hnd][cacheRow[hnd]] == NULL )
    {
      bin_cache_rows[hnd][cacheRow[hnd]] = (BIN_RECORD_SUMMARY **) malloc (cache_max_cols[hnd] * sizeof (BIN_RECORD_SUMMARY *));
      for( i = 0; i < cache_max_cols[hnd]; i++ )
        {
          bin_cache_rows[hnd][cacheRow[hnd]][i] = NULL;
        }

      pfm_cache_size[hnd] +=  cache_max_cols[hnd] * sizeof (BIN_RECORD_SUMMARY *);

#if MEM_DEBUG
      fprintf (stderr, "%s (Handle = %d):  Row = %d Cols = %p CacheSize = %d\n", __FUNCTION__, hnd, cacheRow[hnd], bin_cache_rows[hnd][cacheRow[hnd]],
               pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];
    }

  if( bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]] == NULL  || !bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]]->dirty )
    {
      pfm_cache_miss[hnd]++;
    }
  else
    {
      pfm_cache_hit[hnd]++;
    }

  if (bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]] == NULL )
    {
      /*  Allocate a new bin.  */

      if ((bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]] = (BIN_RECORD_SUMMARY *) malloc (sizeof (BIN_RECORD_SUMMARY))) == NULL)
        {
          sprintf (pfm_err_str, "Unable to allocate memory for bin record");
          return (pfm_error = SET_OFFSETS_BIN_MALLOC_ERROR);
        }

      pfm_cache_size[hnd] +=  sizeof (BIN_RECORD_SUMMARY);

#if MEM_DEBUG
      fprintf (stderr, "%s (Handle = %d):  RC = %d, %d Summary =    %p CacheSize = %d\n", __FUNCTION__, hnd, cacheRow[hnd], cacheCol[hnd],
              bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]], pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      memset( bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]], 0, sizeof( BIN_RECORD_SUMMARY ));
    }

  if ( ! bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]]->dirty )
    {
      /*  The bin has no data in it yet, fetch some from disk!  */

      bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]]->dirty = NVTrue;

      address = ((int64_t) coord.y * (int64_t) bin_header[hnd].bin_width + (int64_t)coord.x) * (int64_t) bin_off[hnd].record_size +
        hd[hnd].bin_header_size;

      lfseek (bin_file_hnd[hnd], address, SEEK_SET);


      /*  Read the record.  */

      if (!(lfread (bin_record_data[hnd], bin_off[hnd].record_size, 1, bin_file_hnd[hnd])))
        {
          sprintf (pfm_err_str, "Error reading from bin file");
          return (pfm_error = READ_BIN_RECORD_DATA_READ_ERROR);
        }

      unpack_bin_record (hnd, bin_record_data[hnd], &tempBin ); 

      *bin_summary = bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]];

      if (use_cov_flag[hnd])
        {
          read_cov_map_index (hnd, coord, &cov);
        }
      else
        {
          cov = 0;
        }

      (*bin_summary)->num_soundings = tempBin.num_soundings;
      (*bin_summary)->validity = tempBin.validity;
      (*bin_summary)->cov_flag = cov;
      (*bin_summary)->coord = coord;
      (*bin_summary)->depth.continuation_pointer = 0;
      (*bin_summary)->depth.record_pos = 0;
      (*bin_summary)->depth.buffers.depths = NULL;
      (*bin_summary)->depth.buffers.next = NULL;
      (*bin_summary)->depth.last_depth_buffer = NULL;
      (*bin_summary)->depth.previous_chain = 0;
      (*bin_summary)->depth.chain = tempBin.depth_chain;
    }
  else
    {
      *bin_summary = bin_cache_rows[hnd][cacheRow[hnd]][cacheCol[hnd]];
    }

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        write_cached_bin_record

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

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

int32_t write_cached_bin_record (int32_t hnd, BIN_RECORD_SUMMARY *bin_summary)
{
  int32_t             i;


  /*  If we haven't already allocated a temp BIN_RECORD for this handle, calloc one now.  */

  if( tmp_bin[hnd] == NULL )
    {
      tmp_bin[hnd] = (BIN_RECORD *) calloc (1, sizeof (BIN_RECORD));
      if (tmp_bin[hnd] == NULL)
        {
          perror ("Allocating tmp_bin in write_cached_bin_record");
          exit (-1);
        }
    }


  /*  Transfer the bin summary to the temp BIN_RECORD for writing to disk.  */

  tmp_bin[hnd]->num_soundings = bin_summary->num_soundings;
  tmp_bin[hnd]->validity      = bin_summary->validity;
  tmp_bin[hnd]->coord         = bin_summary->coord;
  tmp_bin[hnd]->depth_chain   = bin_summary->depth.chain;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


  /*  We can use the chain pointers passed to the caller instead of having
      to reread the bin record as long as we haven't done an
      add_depth_record since the file was opened.  This, of course, assumes
      that the caller hasn't messed with the chain pointers.
      DON'T DO THAT!!!!!!!!!!  'NUFF SAID?  */

  bin_record_head_pointer[hnd] = bin_summary->depth.chain.head;
  bin_record_tail_pointer[hnd] = bin_summary->depth.chain.tail;

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].num_soundings_pos, hd[hnd].count_bits, tmp_bin[hnd]->num_soundings);

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].std_pos, hd[hnd].std_bits, NINT (tmp_bin[hnd]->standard_dev * hd[hnd].std_scale));

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].avg_filtered_depth_pos, hd[hnd].depth_bits,
                NINT ((tmp_bin[hnd]->avg_filtered_depth + hd[hnd].depth_offset) * hd[hnd].depth_scale));

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].min_filtered_depth_pos, hd[hnd].depth_bits,
                NINT ((tmp_bin[hnd]->min_filtered_depth + hd[hnd].depth_offset) * hd[hnd].depth_scale));

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].max_filtered_depth_pos, hd[hnd].depth_bits,
                NINT ((tmp_bin[hnd]->max_filtered_depth + hd[hnd].depth_offset) * hd[hnd].depth_scale));

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].avg_depth_pos, hd[hnd].depth_bits,
                NINT ((tmp_bin[hnd]->avg_depth + hd[hnd].depth_offset) * hd[hnd].depth_scale));

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].min_depth_pos, hd[hnd].depth_bits,
                NINT ((tmp_bin[hnd]->min_depth + hd[hnd].depth_offset) * hd[hnd].depth_scale));

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].max_depth_pos, hd[hnd].depth_bits,
                NINT ((tmp_bin[hnd]->max_depth + hd[hnd].depth_offset) * hd[hnd].depth_scale));


  for (i = 0 ; i < hd[hnd].head.num_bin_attr ; i++)
    {
      pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].attr_pos[i], hd[hnd].bin_attr_bits[i],
                    NINT ((tmp_bin[hnd]->attr[i] + hd[hnd].bin_attr_offset[i]) * hd[hnd].head.bin_attr_scale[i]));
    }

  pfm_bit_pack (bin_record_data[hnd], bin_off[hnd].validity_pos, hd[hnd].validity_bits, tmp_bin[hnd]->validity);


  pfm_double_bit_pack (bin_record_data[hnd], bin_off[hnd].head_pointer_pos, hd[hnd].record_pointer_bits, bin_record_head_pointer[hnd] );

  pfm_double_bit_pack (bin_record_data[hnd], bin_off[hnd].tail_pointer_pos, hd[hnd].record_pointer_bits, bin_record_tail_pointer[hnd] );



  bin_record_address[hnd] = ((int64_t) bin_summary->coord.y * (int64_t) bin_header[hnd].bin_width + bin_summary->coord.x) *
    (int64_t) bin_off[hnd].record_size + hd[hnd].bin_header_size;

  write_bin_buffer_only (hnd, bin_record_address[hnd]);
  bin_record_modified[hnd] = NVFalse;


  /*  Reset the buffer and mark it as clean (bin_summary->dirty == 0).  */

  memset( bin_summary, 0, sizeof( BIN_RECORD_SUMMARY ));


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        write_cached_depth_buffer

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

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

static int32_t write_cached_depth_buffer (int32_t hnd, uint8_t *buffer, int64_t address)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


  lfseek (ndx_file_hnd[hnd], address, SEEK_SET);


  if (!(lfwrite (buffer, dep_off[hnd].record_size, 1, ndx_file_hnd[hnd])))
    {
      sprintf (pfm_err_str, "Unable to write to index file");
      return (pfm_error = WRITE_DEPTH_BUFFER_WRITE_ERROR);
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        prepare_cached_depth_buffer

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

  - Important note:     This function is ONLY called from write_cached_depth_summary.
                        The write_cached_depth_summary function is mutex locked in the 
                        case where we are doing multithreaded append operations.  That is
                        why we can do an fseek to the end of the file without having to
                        mutex lock in this function.

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

static int32_t prepare_cached_depth_buffer (int32_t hnd, DEPTH_SUMMARY *depth, uint8_t *depth_buffer, uint8_t preallocateBuffer)
{

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  if (depth->continuation_pointer == 0)
    {
      /*  There is no known place reserved in the file to write the depth buffer, yet.  */

      if ( depth->chain.tail == -1 )
        {
          /*  This is the first time a depth buffer will be written for this bin.  */
          /*  This buffer and space for any associated buffer will be stored at the end of the file.  */

          lfseek (ndx_file_hnd[hnd], 0LL, SEEK_END);


          /*  Set the head & tail pointers for the bin record.  */

          depth->chain.head = lftell (ndx_file_hnd[hnd]);


          depth->chain.tail = depth->chain.head;


          if( preallocateBuffer )
            {
              depth->continuation_pointer = depth->chain.head + dep_off[hnd].record_size;
            }
        }
      else
        {
          /*  The record was swapped out of cache before a continuation record could be
           *  written.  This situation is not handled here. */

          fprintf( stderr, "\nProgramming or hardware error in PFM cache code.\n");fflush (stderr);
        }
    }
  else
    {
      /*  There exists a place to write the buffer on the disk.  */

      depth->chain.tail = depth->continuation_pointer;

      if( preallocateBuffer )
        {
          depth->continuation_pointer = depth->chain.tail + dep_off[hnd].record_size;
        }
      else
        {
          depth->continuation_pointer = 0;
        }
    }


  /*  Store the updated continuation pointer.  */

  pfm_double_bit_pack (depth_buffer, dep_off[hnd].continuation_pointer_pos, hd[hnd].record_pointer_bits, depth->continuation_pointer);


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        write_cached_depth_summary

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Similar to the above function, but prepares the
                        physical records on disk, if necessary.

  - Arguments:
                        - hnd             =   PFM file handle
                        - address         =   Address of record

  - Return Value:
                        - SUCCESS
                        - WRITE_DEPTH_BUFFER_WRITE_ERROR

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

static int32_t write_cached_depth_summary (int32_t hnd, BIN_RECORD_SUMMARY *bin_summary)
{
  int32_t status;
  DEPTH_LIST *buffer;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


  /*  Fetch the buffers from memory and write them to disk.  */

  /*  Beware the flow of execution here, buffer is updated conditionally!  */

  if (bin_summary->num_soundings > 0)
    {
      /*  This function spins through the entire bin_summary->depth list and writes all of the depth buffers to disk.  It calls
          prepare_cached_depth_buffer to get the record ready and determine where on the disk to store it.  If we are multithreading
          we need to lock the mutex for this file so that any other thread waiting to write to the end of the file doesn't acquire
          the same addresses to write to.  If we're not multithreading and appending it won't matter.  Note that multiple handles
          for the same file will use a single mutex.  In other words, each unique file has a unique mutex and any handle that is
          opened for that file will use that mutex.  We don't want to lock the code if we're updating different files.  */

#ifdef NVWIN3X
      if (threading && mutex[hnd] >= 0) WaitForSingleObject (pfm_write_lock[mutex[hnd]], INFINITE);
#else
      if (threading && mutex[hnd] >= 0) pthread_mutex_lock (&pfm_write_lock[mutex[hnd]]);
#endif


      buffer = &(bin_summary->depth.buffers);

      if ( bin_summary->depth.previous_chain != -1 )
        {
          /*  Write the first buffer if it was read from disk.  It may need an updated continuation pointer.  */
        
          if( buffer->next != NULL )
            {
              lfseek (ndx_file_hnd[hnd], 0LL, SEEK_END);
              bin_summary->depth.continuation_pointer = lftell( ndx_file_hnd[hnd] );


              /*  Store the updated continuation pointer.  */
        
              pfm_double_bit_pack (buffer->depths, dep_off[hnd].continuation_pointer_pos, hd[hnd].record_pointer_bits,
                                bin_summary->depth.continuation_pointer);
            }

          if (buffer->depths != NULL)
            {
              status = write_cached_depth_buffer (hnd, buffer->depths, bin_summary->depth.previous_chain);
              if ( status != SUCCESS ) return (status);
            }

          buffer = buffer->next;
        }
        
      if( buffer != NULL )
        {
          while( buffer->next != NULL )
            {
              /*  Write the depth records to disk.  Set the continuation pointer to follow the
               *  current record, there will be another record to write soon.  */

              status = prepare_cached_depth_buffer (hnd, &(bin_summary->depth), buffer->depths, NVTrue);
              if ( status != SUCCESS ) return (status);
        
              status = write_cached_depth_buffer (hnd, buffer->depths, bin_summary->depth.chain.tail);
              if ( status != SUCCESS ) return (status);
        
              buffer = buffer->next;
            }


          /*  Write the final depth record without a continuation pointer.  */
        
          status = prepare_cached_depth_buffer (hnd, &(bin_summary->depth), buffer->depths, NVFalse);
          if ( status != SUCCESS ) return (status);

          status = write_cached_depth_buffer (hnd, buffer->depths, bin_summary->depth.chain.tail);
          if ( status != SUCCESS ) return (status);
        }


      /*  If we are multithreading and appending records, flush the ndx file I/O buffer and then unlock the mutex for this
          file.  */

      if (threading && mutex[hnd] >= 0)
        {
          lfflush (ndx_file_hnd[hnd]);

#ifdef NVWIN3X
          ReleaseMutex (pfm_write_lock[mutex[hnd]]);
#else
          pthread_mutex_unlock (&pfm_write_lock[mutex[hnd]]);
#endif
        }
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        add_cached_depth_record

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Adds a new depth record to the data chain in the
                        index file for a specific bin in the bin file.
                        NOTE: If the PFM_MODIFIED in the depth validity
                        bits is set, the EDITED FLAG bit is set in the bin
                        record associated with this depth.
                        NOTE: If the PFM_SELECTED_SOUNDING flag is set in
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

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

  - Important Note:     You MUST set the coord field of the depth argument
                        for this to work!

****************************************************************************/

int32_t add_cached_depth_record (int32_t hnd, DEPTH_RECORD *depth)
{
  int32_t             status = SUCCESS;
  BIN_RECORD_SUMMARY  *bin_summary;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


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

#if MEM_DEBUG
  fprintf (stderr, "To %s (Handle = %d):  RC = %2d, %2d XYZ = %0.2f, %0.2f, %6.2f  File = %2d Line = %2d Ping = %5d Beam = %3d\n", __FUNCTION__,
           hnd, depth->coord.y, depth->coord.x, depth->xyz.x, depth->xyz.y, depth->xyz.z, depth->file_number, depth->line_number,
           depth->ping_number, depth->beam_number);
  fflush(stderr);
#endif

  if (read_cached_bin_record(hnd, depth->coord, &bin_summary))
    {
      sprintf (pfm_err_str, "Error reading bin record in add_depth_record");
      return (pfm_error = ADD_DEPTH_RECORD_READ_BIN_RECORD_ERROR);
    }


  if (bin_summary->num_soundings > 0)
    {
      /*  Compute the position within the buffer.  */

      bin_summary->depth.record_pos = (bin_summary->num_soundings % hd[hnd].record_length) * dep_off[hnd].single_point_bits;


      /*
        BUT DO read if depth_record_data is uninitialized...
        This prevents a corruption issue that can occur if you are
        appending data to the PFM that is *new* and you have had no
        reason to read the depth_record... UNTIL NOW, where you must.

        Webb McDonald -- Thu May 26 19:07:43 2005
      */

      if( bin_summary->depth.last_depth_buffer == NULL ) 
        {
          /* There is no data in the depth buffer, but there should be, so load it from disk. */

          bin_summary->depth.buffers.depths = (uint8_t *) malloc (dep_off[hnd].record_size);
          bin_summary->depth.buffers.next = NULL;

          pfm_cache_size[hnd] +=  dep_off[hnd].record_size;

#if MEM_DEBUG
          fprintf (stderr, "0-%s (Handle = %d):  buffers.depths = %p CacheSize = %d\n", __FUNCTION__, hnd, bin_summary->depth.buffers.depths,
                   pfm_cache_size[hnd]);
          fflush(stderr);
#endif

          if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];


          bin_summary->depth.last_depth_buffer = &(bin_summary->depth.buffers);


          /*  Move to the tail of the data chain.  */

          lfseek (ndx_file_hnd[hnd], bin_summary->depth.chain.tail, SEEK_SET);


          bin_summary->depth.previous_chain = bin_summary->depth.chain.tail;


          /*  Read the 'physical' depth record.  */

          lfread (bin_summary->depth.buffers.depths, dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]);


          /* Need to retrieve continuation pointer from depth buffer.  */
          /* XXX Should be zero!  */

          bin_summary->depth.continuation_pointer = pfm_double_bit_unpack (bin_summary->depth.buffers.depths,
                                                                        dep_off[hnd].continuation_pointer_pos,
                                                                        hd[hnd].record_pointer_bits);
          if( bin_summary->depth.continuation_pointer != 0 )
            {
              /*  Break point for debugging.  */

              fprintf (stderr, "\nContinuation pointer not zero on read: ""%"PRId64", %d   %d, %d\n", bin_summary->depth.continuation_pointer,dep_off[hnd].continuation_pointer_pos, depth->coord.x,depth->coord.y );

              bin_summary->depth.continuation_pointer = 0;
            }
        }

      if( bin_summary->depth.record_pos == 0 )
        {
          /*  Buffer is full.  */
          /*  Save the record for writing later. */

          /*  Setup depth buffer linked list.  */

          bin_summary->depth.last_depth_buffer->next = (DEPTH_LIST *) malloc (sizeof (DEPTH_LIST));
          bin_summary->depth.last_depth_buffer = bin_summary->depth.last_depth_buffer->next;

          pfm_cache_size[hnd] += sizeof (DEPTH_LIST);

#if MEM_DEBUG
          fprintf ("1-%s (Handle = %d):  buffers->next = %p CacheSize = %d\n", __FUNCTION__, hnd, bin_summary->depth.last_depth_buffer,
                   pfm_cache_size[hnd]);
          fflush(stderr);
#endif


          /*  Prep a new record.  */

          bin_summary->depth.last_depth_buffer->depths = (uint8_t *) malloc (dep_off[hnd].record_size);
          bin_summary->depth.last_depth_buffer->next = NULL;

          pfm_cache_size[hnd] +=  dep_off[hnd].record_size;

#if MEM_DEBUG
          fprintf ("2-%s (Handle = %d):  buffers->depths = %p CacheSize = %d\n", __FUNCTION__, hnd, bin_summary->depth.last_depth_buffer->depths,
                   pfm_cache_size[hnd]);
          fflush(stderr);
#endif

          if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];
        }
    }
  else
    {
      /*  Set the head and tail pointers to indicate that disk space has yet to be allocated
       *  for the depth record.  */

      bin_summary->depth.chain.head = -1;

      bin_summary->depth.chain.tail = bin_summary->depth.chain.head;

      bin_summary->depth.previous_chain = -1;


      /* Setup the depth buffer. */

      bin_summary->depth.buffers.depths = (uint8_t *) malloc (dep_off[hnd].record_size);
      bin_summary->depth.buffers.next = NULL;

      pfm_cache_size[hnd] +=  dep_off[hnd].record_size;

#if MEM_DEBUG
      fprintf (stderr, "3-%s (Handle = %d):  buffers.depths = %p CacheSize = %d\n", __FUNCTION__, hnd, bin_summary->depth.buffers.depths,
               pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];


      bin_summary->depth.last_depth_buffer = &(bin_summary->depth.buffers);


      /*  Zero out the depth buffer.  */

      memset (bin_summary->depth.last_depth_buffer->depths, 0, dep_off[hnd].record_size);

      bin_summary->depth.record_pos = 0;
    }


  /*  Pack the current depth record into the 'physical' record.  */

  pack_depth_record( bin_summary->depth.last_depth_buffer->depths, depth, bin_summary->depth.record_pos, hnd );


  /*  Increment the number of soundings.  */

  bin_summary->num_soundings++;


  /*  Make sure that we don't blow out the number of soundings count.  */

  if (bin_summary->num_soundings >= count_size[hnd])
    {
      sprintf (pfm_err_str, "Too many soundings in this cell - %d %d.\n", bin_summary->num_soundings, count_size[hnd]);
      return (pfm_error = ADD_DEPTH_RECORD_TOO_MANY_SOUNDINGS_ERROR);
    }


  /*  Set the bin_record_address so that the write_bin_record function will know that this record has already been read and the head and
      tail pointers are known.  */

  /*  Set the bin validity to the same as the depth validity.  */

  bin_summary->validity |= depth->validity;


  /*  Unset the PFM_CHECKED flag in the bin when new data is added.  */

  bin_summary->validity &= ~PFM_CHECKED;

  return (pfm_error = status);
}



int32_t recompute_cached_bin_values (int32_t hnd, BIN_RECORD *bin, uint32_t mask, DEPTH_RECORD *depth)
{
  return recompute_bin_values(hnd, bin, mask, depth);
}



/***************************************************************************/
/*!

  - Module Name:        flush_depth_cache

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Closes the bin file.

  - Arguments:
                        - hnd         =   PFM file handle

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_BUFFER_WRITE_ERROR

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

static int32_t flush_depth_cache (int32_t hnd)
{
  int32_t   r, c;
  int32_t   status;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif


  if( bin_cache_rows[hnd] != NULL )
    {
      pfm_cache_flushes[hnd]++;

      for( r = 0; r < cache_max_rows[hnd]; r++ )
        {
          if( bin_cache_rows[hnd][r] != NULL )
            {
              for( c = 0; c < cache_max_cols[hnd]; c++ )
                {
                  if (bin_cache_rows[hnd][r][c] != NULL) 
                    {
                      if (bin_cache_rows[hnd][r][c]->dirty)
                        {
                          /*  Write the depth record associated with the bin. */

                          if ((bin_cache_rows[hnd][r][c]->dirty == 1) || (bin_cache_rows[hnd][r][c]->dirty == 2))
                            {
                              status = write_cached_depth_summary (hnd, bin_cache_rows[hnd][r][c]);
                              if ( status != SUCCESS )
                                {
                                  destroy_depth_buffer( hnd, &(bin_cache_rows[hnd][r][c]->depth) );
                                  return (status);
                                }
                            }
                        }


                      /*  Destroy the depth buffers.  */

#if MEM_DEBUG
                      fprintf (stderr, "To %s (Handle = %d): RC = %2d, %2d\n", __FUNCTION__, hnd, r, c);
                      fflush(stderr);
#endif

                      destroy_depth_buffer( hnd, &(bin_cache_rows[hnd][r][c]->depth) );
                    }
                }
            }
        }
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  return(pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        destroy_depth_buffer

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Frees a depth buffer.

  - Arguments:
                        - hnd         =   PFM file handle
                        - depth       =   The depth buffer to be freed.

  - Return Value:
                        - void

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

static void destroy_depth_buffer (int32_t hnd, DEPTH_SUMMARY *depth)
{
  DEPTH_LIST  *buffer, *next;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  buffer = &(depth->buffers);

  if (buffer->depths != NULL)
    {
      pfm_cache_size[hnd] -=  dep_off[hnd].record_size;

#if MEM_DEBUG
      fprintf (stderr, "0-%s (Handle = %d):  Buffer->depths = %p CacheSize = %d\n", __FUNCTION__, hnd, buffer->depths, pfm_cache_size[hnd]);
      fflush (stderr);
#endif

      free (buffer->depths);
      buffer->depths = NULL;
    }

  buffer = buffer->next;
  while (buffer != NULL)
    {
      pfm_cache_size[hnd] -=  dep_off[hnd].record_size;

#if MEM_DEBUG
      fprintf (stderr, "1-%s (Handle = %d):  Buffer->depths = %p CacheSize = %d\n", __FUNCTION__, hnd, buffer->depths, pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      free( buffer->depths );
      buffer->depths = NULL;

      next = buffer->next;

      pfm_cache_size[hnd] -=  sizeof (*buffer);

#if MEM_DEBUG
      fprintf (stderr, "2-%s (Handle = %d):  Buffer = %p CacheSize = %d\n", __FUNCTION__, hnd, buffer, pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      free( buffer );
      buffer = next;
    }
  depth->last_depth_buffer = NULL;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

}



/***************************************************************************/
/*!

  - Module Name:        flush_cov_flag

  - Programmer(s):      J. Parker

  - Date Written:       May 2007

  - Purpose:            Writes cov_flag to coverage map in bin file.

  - Arguments:
                        - hnd         =   PFM file handle

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_BUFFER_WRITE_ERROR

****************************************************************************/

int32_t flush_cov_flag (int32_t hnd)
{
  int32_t       r, c;
  NV_I32_COORD2 coord;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  if( bin_cache_rows[hnd] != NULL )
    {
      for( r = 0; r < cache_max_rows[hnd]; r++ )
        {
          if( bin_cache_rows[hnd][r] != NULL )
            {
              for( c = 0; c < cache_max_cols[hnd]; c++ )
                {
                  if( bin_cache_rows[hnd][r][c] != NULL )
                    {
                      coord.x = c + offset_cols[hnd];
                      coord.y = r + offset_rows[hnd];

                      write_cov_map_index (hnd, coord, bin_cache_rows[hnd][r][c]->cov_flag);
                    }
                }
            }
        }
    }


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  return(pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        flush_bin_cache

  - Programmer(s):      Jan C. Depner

  - Date Written:       November 1998

  - Purpose:            Flushes the bin and depth cache to disk and destroys the
                        cache memory.

  - Arguments:
                        - hnd         =   PFM file handle

  - Return Value:
                        - SUCCESS
                        - WRITE_BIN_BUFFER_WRITE_ERROR

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

int32_t flush_bin_cache (int32_t hnd)
{
  int32_t   r, c;


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
  dump_cached_records (hnd);    /* for debugging purposes */
#endif


  /*  Flush the depths before the bins.  */

  flush_depth_cache (hnd);


  /*  Flush the coverage map flag (used by SAIC).  */

  if (use_cov_flag[hnd]) flush_cov_flag (hnd);


  /*  Flush the bins.  */

  if( bin_cache_rows[hnd] != NULL )
    {
      pfm_cache_flushes[hnd]++;

      for( r = 0; r < cache_max_rows[hnd]; r++ )
        {
          if( bin_cache_rows[hnd][r] != NULL )
            {
              for( c = 0; c < cache_max_cols[hnd]; c++ )
                {
                  if(( bin_cache_rows[hnd][r][c] != NULL ) && (( bin_cache_rows[hnd][r][c]->dirty == 1 ) ||
                                                               ( bin_cache_rows[hnd][r][c]->dirty == 2 )))
                    {
                      /*  Write the bin and associated depth record. */

                      if( write_cached_bin_record( hnd, bin_cache_rows[hnd][r][c] ) ) return pfm_error;
                    }
                }
            }
        }
    }

  bin_cache_empty[hnd] = NVTrue;


  /*  Destroy the bins.  */

  if( bin_cache_rows[hnd] != NULL )
    {
      for( r = 0; r < cache_max_rows[hnd]; r++ )
        {
          if( bin_cache_rows[hnd][r] != NULL )
            {
              for( c = 0; c < cache_max_cols[hnd]; c++ )
                {
                  if( bin_cache_rows[hnd][r][c] != NULL )
                    {
                      pfm_cache_size[hnd] -=  sizeof( *(bin_cache_rows[hnd][r][c]) );

#if MEM_DEBUG
                      fprintf (stderr,"0-%s (Handle = %d): Row = %d Col = %d BinCacheCols = %p CacheSize = %d\n", __FUNCTION__, hnd, r, c,
                               bin_cache_rows[hnd][r][c], pfm_cache_size[hnd]);
                      fflush(stderr);
#endif

                      free( bin_cache_rows[hnd][r][c] );
                      bin_cache_rows[hnd][r][c] = NULL;
                    }
                }

              pfm_cache_size[hnd] -=  cache_max_cols[hnd] *  sizeof( *(bin_cache_rows[hnd][r]) );

#if MEM_DEBUG
              fprintf ("1-%s (Handle = %d): Row = %d BinCacheRows = %p CacheSize = %d\n", __FUNCTION__, hnd, r, bin_cache_rows[hnd][r],
                       pfm_cache_size[hnd]);
              fflush(stderr);
#endif

              free( bin_cache_rows[hnd][r] );
              bin_cache_rows[hnd][r] = NULL;
            }
        }

      pfm_cache_size[hnd] -=  cache_max_rows[hnd] *  sizeof( *(bin_cache_rows[hnd]) );

#if MEM_DEBUG
      fprintf (stderr, "2-%s (Handle = %d):  BinCacheRows = %p CacheSize = %d\n", __FUNCTION__, hnd, bin_cache_rows[hnd], pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      free( bin_cache_rows[hnd] );
      bin_cache_rows[hnd] = NULL;
    }


  /*assert( pfm_cache_size[hnd] == 0 );*/


#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  return(pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        set_cached_cov_flag

  - Programmer(s):      J. Parker

  - Date Written:       May 2007

  - Purpose:            Sets coverage byte in cache.

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   COORD2 structure
                        - flag            =   Coverage flag

  - Return Value:
                        - SUCCESS

****************************************************************************/

int32_t set_cached_cov_flag (int32_t hnd, NV_I32_COORD2 coord, uint8_t flag)
{
  int32_t             status = SUCCESS;
  BIN_RECORD_SUMMARY  *bin_summary;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  if (read_cached_bin_record(hnd, coord, &bin_summary))
    {
      sprintf (pfm_err_str, "Error reading bin record in set_cached_cov_flag");
      return (pfm_error = ADD_DEPTH_RECORD_READ_BIN_RECORD_ERROR);
    }

  bin_summary->cov_flag |= flag;

#ifdef PFM_DEBUG
  fprintf (stderr,"%s %s %d (Handle = %d)\n",__FILE__,__FUNCTION__,__LINE__,hnd);fflush (stderr);
#endif

  return (pfm_error = status);
}



void set_use_cov_flag (int32_t hnd, int32_t flag)
{
  use_cov_flag[hnd] = flag;
}



void set_cache_size (int32_t hnd, int32_t max_rows, int32_t max_cols, int32_t center_row, int32_t center_col)
{
  if ((max_rows > 0) && (max_cols > 0))
    {
      /* insure that new values are even numbers */
      new_cache_max_rows[hnd] = (int32_t)(floor(max_rows * 0.5) * 2.0); 
      new_cache_max_cols[hnd] = (int32_t)(floor(max_cols * 0.5) * 2.0); 
      fprintf (stderr, "\nSetting Cache Size for handle %d to %d rows X %d columns ...\n", hnd, new_cache_max_rows[hnd], new_cache_max_cols[hnd]);
      fflush(stderr);
    }

  if ((center_row >= 0) && (center_col >= 0))
    {
      new_center_row[hnd] = center_row;
      new_center_col[hnd] = center_col; 
    }
  force_cache_reset[hnd] = 1;
}



/*!  - Warning:            This function is not thread safe.  */

void set_cache_size_max (/*size_t*/int32_t max)
{
  pfm_cache_size_max = max;
  fprintf (stderr, /*"Setting MAXIMUM Cache Size to %lu ...\n"*/"Setting MAXIMUM Cache Size to %d ...\n", pfm_cache_size_max);
  fflush(stderr);
}



int32_t get_cache_hits(int32_t hnd)
{
  return pfm_cache_hit[hnd];
}



int32_t get_cache_misses(int32_t hnd)
{
  return pfm_cache_miss[hnd];
}



int32_t get_cache_flushes(int32_t hnd)
{
  return pfm_cache_flushes[hnd];
}



int32_t get_cache_size(/*size_t*/int32_t hnd)
{
  return pfm_cache_size[hnd];
}



int32_t get_cache_peak_size(int32_t hnd)
{
  return pfm_cache_size_peak[hnd];
}



int32_t get_cache_max_size()
{
  return pfm_cache_size_max;
}



/***************************************************************************/
/*!

  - Module Name:        read_cached_depth_records

  - Programmer(s):

  - Date Written:

  - Purpose:            Reads depth records from the index file.  This
                        function is only used internal to the library.

  - Arguments:
                        - hnd             =   PFM file handle
                        - bin_summary     =   BIN_RECORD_SUMMARY pointer

  - Return Value:       1

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

int32_t read_cached_depth_records (int32_t hnd, BIN_RECORD_SUMMARY **bin_summary)
{
  DEPTH_LIST    *buffer;

#if MEM_DEBUG
  fprintf (stderr, "%s (Handle = %d):  Sndgs = %2d  RC = %2d, %2d\n", __FUNCTION__, hnd, (*bin_summary)->num_soundings, (*bin_summary)->coord.y,
           (*bin_summary)->coord.x);
  fflush(stderr);
#endif

  if ((*bin_summary)->num_soundings > 0)
    {
      buffer = &((*bin_summary)->depth.buffers);
        
      (*bin_summary)->depth.record_pos = ((*bin_summary)->num_soundings % hd[hnd].record_length) * dep_off[hnd].single_point_bits;
        
      if (buffer->depths != NULL) return 1;

      buffer->depths = (uint8_t *) malloc (dep_off[hnd].record_size);
      buffer->next = NULL;

      pfm_cache_size[hnd] += dep_off[hnd].record_size;

#if MEM_DEBUG
      fprintf (stderr, "%s (Handle = %d): Sndgs = %2d 0-Buffer->depths = %p CacheSize = %d\n", __FUNCTION__, hnd, (*bin_summary)->num_soundings,
               buffer->depths, pfm_cache_size[hnd]);
      fflush(stderr);
#endif

      if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];

      (*bin_summary)->depth.last_depth_buffer = &((*bin_summary)->depth.buffers);


      /*  Move to the head of the data chain.  */

      lfseek (ndx_file_hnd[hnd], (*bin_summary)->depth.chain.head, SEEK_SET);

      (*bin_summary)->depth.previous_chain = (*bin_summary)->depth.chain.head;


      /*  Read the 'physical' depth record.  */

      lfread (buffer->depths, dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]);


      /* Need to retrieve continuation pointer from depth buffer.  */

      (*bin_summary)->depth.continuation_pointer = pfm_double_bit_unpack ((*bin_summary)->depth.buffers.depths,
                                                                       dep_off[hnd].continuation_pointer_pos,
                                                                       hd[hnd].record_pointer_bits);
        
      while ((*bin_summary)->depth.continuation_pointer != 0)
        {
          lfseek (ndx_file_hnd[hnd], (*bin_summary)->depth.continuation_pointer, SEEK_SET);
            
          buffer->next = (DEPTH_LIST *) malloc (sizeof (DEPTH_LIST));
          pfm_cache_size[hnd] +=  sizeof (DEPTH_LIST);

#if MEM_DEBUG
          fprintf ("%s (Handle = %d):  Buffer->next = %p CacheSize = %d\n", __FUNCTION__, hnd, buffer->next, pfm_cache_size[hnd]);
          fflush(stderr);
#endif

          buffer = buffer->next;
            
          buffer->next = NULL;

          buffer->depths = (uint8_t *) malloc (dep_off[hnd].record_size);
          pfm_cache_size[hnd] +=  dep_off[hnd].record_size;

#if MEM_DEBUG
          fprintf ("%s (Handle = %d):  1-Buffer->depths = %p CacheSize = %d\n", __FUNCTION__, hnd, buffer->depths, pfm_cache_size[hnd]);
          fflush(stderr);
#endif

          if( pfm_cache_size[hnd] > pfm_cache_size_peak[hnd] ) pfm_cache_size_peak[hnd] = pfm_cache_size[hnd];

          lfread (buffer->depths, dep_off[hnd].record_size, 1, ndx_file_hnd[hnd]);

          (*bin_summary)->depth.continuation_pointer = pfm_double_bit_unpack (buffer->depths, dep_off[hnd].continuation_pointer_pos,
                                                                           hd[hnd].record_pointer_bits);
        }
    }
  return 1;
}



/***************************************************************************/
/*!

  - Module Name:        put_cached_depth_records

  - Programmer(s):

  - Date Written:

  - Purpose:

  - Arguments:

  - Return Value:       1

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

int32_t put_cached_depth_records (int32_t hnd, NV_I32_COORD2 coord, DEPTH_RECORD **depth, BIN_RECORD_SUMMARY **bsum)
{
  int32_t             i = 0, j, record_pos;
  DEPTH_LIST         *buffer;
  float               x_offset, y_offset;

  record_pos = 0;

  buffer = &((*bsum)->depth.buffers);
  if ((*bsum)->num_soundings > 0)
    {
      (*bsum)->dirty = 2;
      while (buffer != NULL)
        {
          /*  pack the current depth record into the 'physical' record.  */

          pfm_bit_pack (buffer->depths, dep_off[hnd].file_number_pos + record_pos, hd[hnd].file_number_bits, (*depth)[i].file_number);
            
          pfm_bit_pack (buffer->depths, dep_off[hnd].line_number_pos + record_pos, hd[hnd].line_number_bits, (*depth)[i].line_number);
            
          pfm_bit_pack (buffer->depths, dep_off[hnd].ping_number_pos + record_pos, hd[hnd].ping_number_bits, (*depth)[i].ping_number);
            
          pfm_bit_pack (buffer->depths, dep_off[hnd].beam_number_pos + record_pos, hd[hnd].beam_number_bits, (*depth)[i].beam_number);
            
          pfm_bit_pack (buffer->depths, dep_off[hnd].depth_pos + record_pos, hd[hnd].depth_bits,
                        NINT (((*depth)[i].xyz.z + hd[hnd].depth_offset) * hd[hnd].depth_scale));


          /*  Compute the offsets from the position.   */
          /*  Stored as lat/lon.  */

          if (!hd[hnd].head.proj_data.projection)
            {
              x_offset = (*depth)[i].xyz.x - (bin_header[hnd].mbr.min_x + ((*depth)[i].coord.x * bin_header[hnd].x_bin_size_degrees));
              y_offset = (*depth)[i].xyz.y - (bin_header[hnd].mbr.min_y + ((*depth)[i].coord.y * bin_header[hnd].y_bin_size_degrees));


              pfm_bit_pack (buffer->depths, dep_off[hnd].x_offset_pos + record_pos, hd[hnd].offset_bits,
                            NINT ((x_offset / bin_header[hnd].x_bin_size_degrees) * x_offset_scale[hnd]));
            
              pfm_bit_pack (buffer->depths, dep_off[hnd].y_offset_pos + record_pos, hd[hnd].offset_bits,
                            NINT ((y_offset / bin_header[hnd].y_bin_size_degrees) * y_offset_scale[hnd]));
            }

            
          /*  Stored as x/y.  */

          else
            {
              x_offset = (*depth)[i].xyz.x - (bin_header[hnd].mbr.min_x + ((*depth)[i].coord.x * bin_header[hnd].bin_size_xy));
              y_offset = (*depth)[i].xyz.y - (bin_header[hnd].mbr.min_y + ((*depth)[i].coord.y * bin_header[hnd].bin_size_xy));
            
            
              pfm_bit_pack (buffer->depths, dep_off[hnd].x_offset_pos + record_pos, hd[hnd].offset_bits,
                            NINT ((x_offset / bin_header[hnd].bin_size_xy) * x_offset_scale[hnd]));

              pfm_bit_pack (buffer->depths, dep_off[hnd].y_offset_pos + record_pos, hd[hnd].offset_bits,
                            NINT ((y_offset / bin_header[hnd].bin_size_xy) * y_offset_scale[hnd]));
            }


          for (j = 0 ; j < hd[hnd].head.num_ndx_attr ; j++)
            {
              pfm_bit_pack (buffer->depths, dep_off[hnd].attr_pos[j] + record_pos, hd[hnd].ndx_attr_bits[j],
                            NINT (((*depth)[i].attr[j] - hd[hnd].head.min_ndx_attr[j]) * hd[hnd].head.ndx_attr_scale[j]));
            }

          pfm_bit_pack (buffer->depths, dep_off[hnd].validity_pos + record_pos, hd[hnd].validity_bits, (*depth)[i].validity);


          if (hd[hnd].horizontal_error_bits)
            {
              if ((*depth)[i].horizontal_error >= hd[hnd].horizontal_error_null) (*depth)[i].horizontal_error = hd[hnd].horizontal_error_null;
              pfm_bit_pack (buffer->depths, dep_off[hnd].horizontal_error_pos + record_pos,
                            hd[hnd].horizontal_error_bits, NINT ((*depth)[i].horizontal_error * hd[hnd].head.horizontal_error_scale));
            }

          if (hd[hnd].vertical_error_bits)
            {
              if ((*depth)[i].vertical_error >= hd[hnd].vertical_error_null) (*depth)[i].vertical_error = hd[hnd].vertical_error_null;
              pfm_bit_pack (buffer->depths, dep_off[hnd].vertical_error_pos + record_pos,
                            hd[hnd].vertical_error_bits, NINT ((*depth)[i].vertical_error * hd[hnd].head.vertical_error_scale));
            }
                            
          record_pos += dep_off[hnd].single_point_bits;
          i++;

          if (i >= (*bsum)->num_soundings) break;

          if ((i > 0) && !(i % hd[hnd].record_length))
            {
              record_pos = 0;
              buffer = buffer->next;
            }
        }  /* while buffer */
    }  /* if num_soundings */
  else
    {
      return 0;
    }

  return (1);
}



/***************************************************************************/
/*!

  - Module Name:        get_cached_depth_records

  - Programmer(s):

  - Date Written:

  - Purpose:

  - Arguments:

  - Return Value:       1

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

int32_t get_cached_depth_records (int32_t hnd, NV_I32_COORD2 coord, DEPTH_RECORD **depth, BIN_RECORD_SUMMARY **bsum)
{
  int32_t             i = 0, j;
  DEPTH_LIST         *buffer;
  float               x_offset, y_offset;

  depth_record_pos[hnd] = 0;

  buffer = &((*bsum)->depth.buffers);
  if ((*bsum)->num_soundings > 0)
    {
      *depth = (DEPTH_RECORD *) calloc ((*bsum)->num_soundings, sizeof(DEPTH_RECORD));
      while (buffer != NULL)
        {
          (*depth)[i].coord.x = coord.x;
          (*depth)[i].coord.y = coord.y;


          /*  Unpack the current depth record from the 'physical' record.  */
            
          (*depth)[i].file_number = pfm_bit_unpack (buffer->depths, dep_off[hnd].file_number_pos + depth_record_pos[hnd], hd[hnd].file_number_bits);
            
          (*depth)[i].line_number = pfm_bit_unpack (buffer->depths, dep_off[hnd].line_number_pos + depth_record_pos[hnd], hd[hnd].line_number_bits);
            
          (*depth)[i].ping_number = pfm_bit_unpack (buffer->depths, dep_off[hnd].ping_number_pos + depth_record_pos[hnd], hd[hnd].ping_number_bits);
            
          (*depth)[i].beam_number = pfm_bit_unpack (buffer->depths, dep_off[hnd].beam_number_pos + depth_record_pos[hnd], hd[hnd].beam_number_bits);

          (*depth)[i].xyz.z = (float) (pfm_bit_unpack (buffer->depths, dep_off[hnd].depth_pos + depth_record_pos[hnd],
                                                            hd[hnd].depth_bits)) / hd[hnd].depth_scale - hd[hnd].depth_offset;


          /*  Stored as lat/lon.  */

          if (!hd[hnd].head.proj_data.projection)
            {
              x_offset = ((float) (pfm_bit_unpack (buffer->depths, dep_off[hnd].x_offset_pos + depth_record_pos[hnd],
                                                        hd[hnd].offset_bits)) / x_offset_scale[hnd]) * bin_header[hnd].x_bin_size_degrees;

              y_offset = ((float) (pfm_bit_unpack (buffer->depths, dep_off[hnd].y_offset_pos + depth_record_pos[hnd],
                                                        hd[hnd].offset_bits)) / y_offset_scale[hnd]) * bin_header[hnd].y_bin_size_degrees;


              /*  Compute the geographic position of the point. */

              (*depth)[i].xyz.y = bin_header[hnd].mbr.min_y + coord.y * bin_header[hnd].y_bin_size_degrees + y_offset;
              (*depth)[i].xyz.x = bin_header[hnd].mbr.min_x + coord.x * bin_header[hnd].x_bin_size_degrees + x_offset;
            }


          /*  Stored as x/y.  */

          else
            {
              x_offset = ((float) (pfm_bit_unpack (buffer->depths, dep_off[hnd].x_offset_pos + depth_record_pos[hnd],
                                                        hd[hnd].offset_bits)) / x_offset_scale[hnd]) * bin_header[hnd].bin_size_xy;

              y_offset = ((float) (pfm_bit_unpack (buffer->depths, dep_off[hnd].y_offset_pos + depth_record_pos[hnd],
                                                        hd[hnd].offset_bits)) / y_offset_scale[hnd]) * bin_header[hnd].bin_size_xy;


              /*  Compute the x/y position of the point. */

              (*depth)[i].xyz.y = bin_header[hnd].mbr.min_y + coord.y * bin_header[hnd].bin_size_xy + y_offset;
              (*depth)[i].xyz.x = bin_header[hnd].mbr.min_x + coord.x * bin_header[hnd].bin_size_xy + x_offset;
            }


          for (j = 0 ; j < hd[hnd].head.num_ndx_attr ; j++)
            {
              (*depth)[i].attr[j] = (float) (pfm_bit_unpack (buffer->depths, dep_off[hnd].attr_pos[j] + depth_record_pos[hnd],
                                                             hd[hnd].ndx_attr_bits[j])) / hd[hnd].head.ndx_attr_scale[j] +
                hd[hnd].head.min_ndx_attr[j];
            }

          (*depth)[i].validity = pfm_bit_unpack (buffer->depths, dep_off[hnd].validity_pos + depth_record_pos[hnd], hd[hnd].validity_bits);


          if (hd[hnd].horizontal_error_bits)
            {
              (*depth)[i].horizontal_error = (float) (pfm_bit_unpack (buffer->depths, dep_off[hnd].horizontal_error_pos +
                                                                           depth_record_pos[hnd], hd[hnd].horizontal_error_bits)) /
                hd[hnd].head.horizontal_error_scale;
              if ((*depth)[i].horizontal_error >= hd[hnd].horizontal_error_null) (*depth)[i].horizontal_error = -999.0;
            }

          if (hd[hnd].vertical_error_bits)
            {
              (*depth)[i].vertical_error = (float) (pfm_bit_unpack (buffer->depths, dep_off[hnd].vertical_error_pos +
                                                                         depth_record_pos[hnd], hd[hnd].vertical_error_bits)) /
                hd[hnd].head.vertical_error_scale;
              if ((*depth)[i].vertical_error >= hd[hnd].vertical_error_null) (*depth)[i].vertical_error = -999.0;
            }

          depth_record_pos[hnd] += dep_off[hnd].single_point_bits;

          i++;
          if (i >= (*bsum)->num_soundings) break;

          if ((i > 0) && !(i % hd[hnd].record_length))
            {
              depth_record_pos[hnd] = 0;
              buffer = buffer->next;
            }
        }  /* while buffer */
    }  /* if num_soundings */
  else
    {
      return 0;
    }

  return (1);
}



/***************************************************************************/
/*!

  - Module Name:        dump_cached_record

  - Programmer(s):

  - Date Written:

  - Purpose:            Print out the contents of a cached record.

  - Arguments:
                        - hnd             =   PFM file handle
                        - coord           =   COORD2 structure

  - Return Value:
                        - SUCCESS

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

int32_t dump_cached_record (int32_t hnd, NV_I32_COORD2 coord)
{
  int32_t             k;
  BIN_RECORD_SUMMARY *bsum;
  DEPTH_LIST         *buffer;

  if (bin_cache_rows[hnd] != NULL)
    {
      if (bin_cache_rows[hnd][coord.y] != NULL)
        {
          if (bin_cache_rows[hnd][coord.y][coord.x] != NULL)
            {
              bsum = bin_cache_rows[hnd][coord.y][coord.x];

              printf ("DUMP: RC = %03d, %03d BSumRC = %03d, %03d NumSndgs = %2d  Dirty = %d  CovFlag = %3d Head = %6"PRId64" Tail = %6"PRId64" RecPos = %3"PRId64" Depths = %p\n",
                      coord.y, 
                      coord.x, 
                      bsum->coord.y, 
                      bsum->coord.x, 
                      (int32_t) bsum->num_soundings, 
                      bsum->dirty, 
                      (int32_t) bsum->cov_flag, 
                      (int64_t) bsum->depth.chain.head,
                      (int64_t) bsum->depth.chain.tail,
                      (int64_t) bsum->depth.record_pos,
                      (void *) bsum->depth.buffers.depths);


              /* dump the depth buffer bytes of a 6 depth physical record. */

              buffer = &(bsum->depth.buffers);
              while( buffer != NULL)
                {
                  for (k = 0; k < 131; k++)
                    {
                      if (!(k % 21) && (k > 0)) printf ("\n");
                      printf (" %02x", buffer->depths[k]);
                    }
                  buffer = buffer->next;
                  printf ("\n");
                }
            }
        }
    }

  fflush (stdout);

  return (pfm_error = SUCCESS);
}



/***************************************************************************/
/*!

  - Module Name:        dump_cached_records

  - Programmer(s):

  - Date Written:

  - Purpose:            Print out the contents of all cached records.

  - Arguments:
                        - hnd             =   PFM file handle

  - Return Value:
                        - SUCCESS

  - Caveats:            This code was actually developed by IVS in 
                        2007.  It was later modified by SAIC.  The
                        suspected culprits are listed below ;-)
                            - Danny Neville (IVS)
                            - Graeme Sweet (IVS)
                            - William Burrow (IVS)
                            - Jeff Parker (SAIC)
                            - Webb McDonald (SAIC)

****************************************************************************/

int32_t dump_cached_records (int32_t hnd)
{
  int32_t             i, j, k;
  BIN_RECORD_SUMMARY *bsum;
  FILE               *bsum_ptr;
  DEPTH_LIST         *buffer;

  if (dump_cached_records_count[hnd])
    {
      if ((bsum_ptr = fopen ("bsum.out", "a")) == NULL)
        {
          printf ("Can't open bsum.out ...\n");
          fflush (stdout);
          return 0;
        }
    }
  else
    {
      if ((bsum_ptr = fopen ("bsum.out", "w")) == NULL)
        {
          printf ("Can't open bsum.out ...\n");
          fflush (stdout);
          return 0;
        }
    }

  dump_cached_records_count[hnd]++;
  fprintf (bsum_ptr, "NEW DUMP %d ...\n", dump_cached_records_count[hnd]);
  fflush (bsum_ptr);

  if (bin_cache_rows[hnd] != NULL)
    {
      for (i = 0; i < cache_max_rows[hnd]; i++)
        {
          if (bin_cache_rows[hnd][i] != NULL)
            {
              for (j = 0; j < cache_max_cols[hnd]; j++)
                {
                  if (bin_cache_rows[hnd][i][j] != NULL)
                    {
                      bsum = bin_cache_rows[hnd][i][j];

                      fprintf (bsum_ptr, "RC = %03d, %03d  NumSndgs = %2d  Dirty = %d  CovFlag = %3d Head = %6"PRId64" Tail = %6"PRId64" RecPos = %3"PRId64" Depths = %p\n",
                               bsum->coord.y, 
                               bsum->coord.x, 
                               (int32_t) bsum->num_soundings, 
                               bsum->dirty, 
                               (int32_t) bsum->cov_flag, 
                               (int64_t) bsum->depth.chain.head,
                               (int64_t) bsum->depth.chain.tail,
                               (int64_t) bsum->depth.record_pos,
                               (void *) bsum->depth.buffers.depths);


                      /* dump the depth buffer bytes of a 6 depth physical record. */

                      buffer = &(bsum->depth.buffers);
                      if (bsum->num_soundings > 0)
                        { 
                          while( buffer != NULL)
                            {
                              for (k = 0; k < 131; k++)
                                {
                                  if (!(k % 21) && (k > 0)) fprintf (bsum_ptr, "\n");
                                  fprintf (bsum_ptr, " %02x", buffer->depths[k]);
                                }
                              buffer = buffer->next;
                              fprintf (bsum_ptr, "\n");
                              fflush (bsum_ptr);
                            }
                        }
                    }
                }
            }
        }
    }

  fclose (bsum_ptr);

  return (pfm_error = SUCCESS);
}
