
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



/*  Do not modify the first portion of the VERSION.  The PFM library
    uses this as a file integrity check.  Anything after the section 
    'library V' may be modified.  'PFM Software - PFM I/O library V' must
    not be modified AT ALL!  */

#ifndef VERSION

#define     VERSION     "PFM Software - PFM I/O library V7.00 - 05/15/19"

#endif

/*!< <pre>

    Version 1.1
    12/27/99
    Jan C. Depner

    Improved the error string code so that it puts out more descriptive
    text.


    Version 1.2
    12/28/99
    Jan C. Depner

    Corrected bug in recompute_bin_values that would not clear the suspect
    flag if a point was deleted.


    Version 1.3
    01/05/00
    Jan C. Depner

    Corrected bugs in pfm_io related to accessing multiple PFM files.


    Version 1.4
    02/01/00
    Jan C. Depner

    If bin is marked as "checked", in recompute_bin_values we turn off the
    suspect data flag.


    Version 1.5
    02/12/00
    Jan C. Depner

    Changed max_depth and offset arguments in open_pfm_file to pointers so we
    could pass them back to caller.  This allowas appending to a pre-existing
    PFM file.


    Version 2.0
    02/15/00
    Jan C. Depner

    Added SELECTED FLAG to the bin record data.  This is a format change so
    we jumped to the next major version.


    Version 2.1
    03/29/00
    Jan C. Depner

    I've added projected X to the bin file.  Y should be the same as the 
    estimated since lat doesn't change enough to worry about for our purposes.
    Right now it comes out in meters from the southwest corner.  Eventually 
    I'll put out northing and easting (I was in a hurry, I didn't get started 
    until yesterday).  The test set ended up using 5 bits more than without
    the projected X.  I'm storing the difference between the actual distance 
    and the estimated distance (bin number times the bin size at center of
    chart in centimeters). 
    I've also added Class 1 and Class 2 flags to the bin.  These equate to the
    1xIHO and 2xIHO bit settings in the GSF validity data.  The classes as 
    well as checked data can be viewed in pfm_view. 
    I implemented a new set of status bits instead of using the GSF status.
    There are 12 bits with 8 in use and 4 spare. 
    We are now reading and writing SHOALS .out files with no problems (they 
    didn't have any status info in the status word so I used the PFM status 
    bits). 


    Version 2.2
    04/17/00
    Jan C. Depner

    Minor bug fix for the read_depth_record calls.  Had to static a variable.


    Version 2.3
    04/21/00
    Jan C. Depner

    Another minor bug fix.  This one was in read_bin_record.  If you don't
    read a new record or unpack it you need to place the contents of the 
    last record read into the BIN_RECORD structure that was passed in.  Duh.


    Version 2.4
    05/05/00
    Jan C. Depner

    Made update_depth_record set the bin record selected and data flags if 
    applicable.


    Version 2.5
    06/22/00
    Jan C. Depner

    Added PFM_DELETED flag to the PFM status bits.  Minor bug fixes.  Added
    function to change the target file name.


    Version 2.51
    08/18/00
    Jan C. Depner

    Fixed a minor bug in the read_depth_record function.  Added the 
    read_depth_array_index and read_depth_array_latlon functions (something
    I should have done a long time ago).


    Version 2.52
    09/04/00
    Jan C. Depner

    Removed the read_depth_record_index and read_depth_record_latlon from
    the library.  This is just an extension of the 2.51 change.


    Version 2.6
    10/12/00
    Jan C. Depner

    Major improvements in the update_depth_record function.  When a record is
    read the block address and record offset are saved and used to position
    the file on update.  Previously the function would read the entire record
    searching for a file, ping, beam match.  Hopefully this will speed up
    edits (filtering, etc).


    Version 3.0
    12/12/00
    Jan C. Depner

    Added coverage map to the end of the bin file.
    Added read_coverage_map_index function.
    Made read_bin_row read across the row in one shot.
    Added read_bin_depth_array_index and read_bin_depth_array_latlon.
    Added sequence numbers and +/- to the input file names in the PFM list
    file in order to maintain sequence.


    Version 3.1
    05/08/01
    Jan C. Depner

    Removed references to ACME Software.  Corrected bug reported by Graeme 
    Sweet (IVS).  Cleaned up the coverage map write.


    Version 3.11
    06/10/01
    Jan C. Depner

    Fixed a minor bug in the close_pfm_file function.  Passing structure args
    to open_pfm_file.


    Version 4.0
    07/16/01
    Jan C. Depner

    Major revision.  Handles projected coordinates instead of just lat/lon.
    Added three attributes to bin and depth records.  Added line numbers to
    depth records.  Put validity information in the bin record in the same
    format as the depth record validity - this gets rid of the flags.  Changed
    some constant and variable names to make more sense.  Added checkpointing
    of the files for disaster recovery.


    Version 4.01
    10/26/01
    Jan C. Depner

    Fixed minor bug in recompute_bin_values.  We were using null depths for 
    computation.


    Version 4.02
    11/14/01
    Jan C. Depner

    Fixed bug in update_target_file.


    Version 4.03
    02/01/02
    Jan C. Depner / Mark Paton

    - Numerous updates to make the PFM library compatible with windows NT/2K/XP.
      Most of the changes are minor. Some functions needed a windows compatible
      implementation and this has been added inside appropriate NVWIN3X blocks.
    - Moved the huge_io library functions into the pfm library.  Also, it no
      longer depends on the utility library.


    Version 4.04
    05/01/02
    Jan C. Depner / Jeff Parker

    - Added Jeff's changes to better support projected data.


    Version 4.10
    06/05/02
    Jan C. Depner

    Implemented changes defined at Toronto CHC 05/29/02 :

        Changed class flags to PFM_USER flags
        Changed PFM_NBR to PFM_USER_03
        Added PFM_USER_04 and PFM_USER_05
        Added names for PFM_USER flags in header
        No longer compute averages for attributes in bin record (must be set
            by the application)
        Store min, max, and scale for attributes (compute # of bits)
        Changed attributes to NV_FLOAT32
        Added hemisphere to projection structure


    Version 4.11
    07/10/02
    Jeff Parker

    Corrected bug in open_pfm_file so that projection information is 
    maintained and carried forward as part of the header.


    Version 4.12
    07/12/02
    Mark Paton

    Corrected bug in pfm.h and read_bin_row (possibility of loosing a changed
    depth_record).


    Version 4.13
    07/14/02
    Jan C. Depner

    Changed PFM_DEBUG print statements to be conditionally compiled instead 
    of "if" statements.  This should be a bit faster.


    Version 4.14
    07/25/02
    Jeff Parker

    Changed default std_scale to 1000000 instead of 100000000.  This allows 
    a greater range in the standard deviation.


    Version 4.15
    09/05/02
    Jan C. Depner

    Added AVERAGE NAME and DYNAMIC RELOAD to the bin header.  This is to make
    way for the CUBE surface to be used instead of the average surface and 
    to allow for unload/reload during editing.


    Version 4.16
    09/17/02
    Jan C. Depner/Graeme Sweet & Danny Neville (IVS)/Jeff Parker (SAIC)

    (JCD)-Moved a block of code in recompute_bin_values that was writing out 
    the bin even if there was no data in the bin.  Changed all occurrences of
    NV_U_CHAR to NV_U_BYTE.  This is purely cosmetic since they are both
    unsigned chars.  Changed the definition of NV_CHAR to be "char" instead
    of "signed char".  This saves on meaningless warnings.  (SAIC)-Modified 
    recompute_bin_values to use a temporary NV_FLOAT64 variable to compute 
    standard deviation.  This corrects a round-off bug.  (IVS)-Added callback 
    support for load operations.  Previously the library wrote to standard I/O
    percentage done. This is still supported but if you link a callback 
    function to the libary then GUI's can make nicer ways to convey this 
    information.  Significant fixes to the handling of attributes in the 
    libary. 


    Version 4.17
    09/30/02
    Jan C. Depner

    Set the default number of bits for x/y offsets to 10 instead of 12.
    Replaced almost all exits with error returns.


    Version 4.18
    11/18/02
    Mark Paton, Graeme Sweet, IVS

    Added new data types and fixed a few small bugs.


    Version 4.19
    12/12/02
    Jan C. Depner

    Added depth chain pointers to bin_record in order to save on I/O
    during write_bin_record.


    Version 4.20
    01/10/03
    Jeff Parker (SAIC)

    Fixed bug in open_bin that would give slightly different values for
    bin sizes and bounds when file is created versus when appended to.


    Version 4.21
    03/11/03
    Jeff Parker (SAIC)

    Fixed bug in open_pfm_file that was corrupting files, from Jeff - 
    "Open_pfm_file first opens the list file, then the line file, then tries 
    to open the bin file.  If the path is invalid, this fails and returns a 
    -1 but seems to leave the list and line files open and the pfm_hnd[hnd] 
    as NOT -1. The same thing would happen if it got to the open_index call."


    Version 4.22
    06/11/03
    Jan C. Depner

    Added PFM_UNISIPS_DEPTH_DATA type (19).  Added ability to "reference mask"
    data based on PFM_USER_ flags.  Data that are marked with a PFM_USER_ flag 
    that is "reference masked" will not be used for computation of surfaces
    or statistics.  They are for reference only.  How this is used outside of
    the library is up to the programmer.  I am using it to have UNISIPS depth
    data available for viewing but not used for surfaces or sounding selection.
    This allows me to load UNISIPS depth data and shell UNISIPS to view the 
    imagery without the lousy depth data screwing up my hydro data.


    Version 4.23
    07/03/03
    Jan C. Depner

    Added padding to BIN_HEADER and EDIT_SHARE structures so that minor
    additions to these structures (if added at the end of the structure using
    part of the pad space) will not cause shared memory incompatibilities
    between Fledermaus, SABER, and ABE.


    Version 4.3
    07/18/03
    Jan C. Depner

    Removed reference mask (which was a test only) and replaced it with
    PFM_REFERENCE flag.  Added PFM_VERIFIED flag.  Increased validity bits
    to 16.  Increased all validity variables to NV_U_INT32 from NV_U_INT16.
    Added IVS functions and bug fixes.  Added gsf_flags.c to library.


    Version 4.31
    08/06/03
    Jan C. Depner

    Fixed "brain-in-neutral" stupid mistake in header.h.


    Version 4.32
    08/12/03
    Jan C. Depner

    Bug fix in read_depth_record to catch bad continuation pointers.  I think
    I'm treating the symptom but I can't find the cause.


    Version 4.33
    08/19/03
    Jan C. Depner

    Bad case of bozo - fixed the PFM_USER mask in pfm.h.


    Version 4.34
    09/08/03
    Jan C. Depner

    It turned out that the problems I was seeing in FL were related to a bad
    disk drive.  I have added a print statement in the read_depth_array... 
    functions whenever there is a problem reading a depth array chain.  I
    also zero out the number of soundings in the bin record so that you won't
    do anything strange with the particular depth array chain.  I have not
    been able to recreate the problem I had in FL using the same data and
    runstream.  I have also not seen any errors in depth chain reading.


    Version 4.341
    03/16/04
    Graeme Sweet

    Minor update for IVS. No changes made to pfm library, but pfm files with
    a version number greater than 4.34 no support unloading for TOF formats.


    Version 4.342
    05/07/04
    Danny Neville

    Minor update for IVS. Also no changes to the PFM library, but required to 
    handle PFM files built from HDCS files using version 4.341 of the library.


    Version 4.343
    06/04/04
    Danny Neville

    Minor update for IVS. Fixes an obscure bug with consistency in bin record and
    puts back calculation of std. deviation when not using an average surface.


    Version 4.344
    069/23/04
    Danny Neville

    Integrated the bug fixes from SAIC/NAVO that are part of the 4.4 version, but we
    need in 4.3.
    

    Version 4.35
    11/14/03
    Jan C. Depner

    Added [SUBSTITUTE PATH] to the.pfm_cfg file.  This adds the ability to
    alias file names in the .pfm file.  This is handy for reading files that
    are resident on a Windoze system from Linux and vice versa.  The syntax is:

    [SUBSTITUTE PATH]=N:,/net/rznas2000/Raidzone
    [SUBSTITUTE PATH]=O:,/net/rznas3000/Raidzone

    Up to 10 substitute paths may be used.  Also added conversion for \ to /
    and vice versa.

    JSB, DF, BAC (SAIC)
    Ensure the line file gets closed if necessary.
    Trap to avoid taking square root of negative number in computing standard 
    deviation.
    Added entry points update_depth_record_xy_ext_flags, and 
    update_depth_record_index_ext_flags, these do not control the modified bit
    within the library.
    

    Version 4.36
    12/10/03
    Jan C. Depner

    Minor bug fix when computing standard deviation surface after a misp or
    other type of computed surface has been put in place of the average
    surface.  We don't want to recompute OR set the standard deviation to 0.0
    if we don't have a regular average surface.
    

    Version 4.37
    04/15/04
    Jan C. Depner

    Minor changes to get rid of warnings issued when running valgrind 
    (http://valgrind.kde.org/) on pfm_loader and pfm_view.  "Source and 
    destination overlap in strncpy" and a few uninitialized errors when 
    packing or unpacking attributes (I wasn't using any so it really didn't 
    matter as it was putting the uninitialized value into 0 bits).  On the 
    bright side their were no major memory SODs reported by valgrind.  Oh, I 
    almost forgot, let's sing a rousing chorus of "Taxman".  Happy April 15th!


    Version 4.371
    08/27/04
    Jan C. Depner

    Super-minor change to fix my .pfm_cfg [SUBSTITUTE PATH] screwup.  This
    shouldn't effect anyone (except me ;-)


    Version 4.4
    09/22/04
    Webb McDonald (SAIC), Graeme Sweet (IVS), Jan C. Depner

    Very subtle fix for a problem that has been plaguing us for a few 
    years.  Type-casting some of the NV_INT32 variables to NV_INT64 prior
    to multiplying them fixed problems with very large bin files.  This was
    not due to the multiplication blowing out the NV_INT32 result.  It
    appears to be more like a rounding error (if that makes any sense).  I
    had a repeatable corruption setup that I tested and these changes fix
    it.  Kudos to Webb!  Also, checking the bin record for modifications 
    in write_bin_record.  Great work from SAIC and IVS!


    Version 4.41
    12/09/04
    Danny Neville (IVS)

    Added NV_INT64_SPECIFIER to fix Micro$oft Windoze problem reading 64 bit
    integers with sscanf.


    Version 4.5
    12/15/04
    Jan C. Depner

    Forcing new PFM's to be built all in the same directory.


    Version 4.51
    01/18/05
    Webb McDonald (SAIC), Jan Depner

    No mod function calls for change_depth_record...  Handle Reference soundings
    in recompute_bin_values.


    Version 4.52
    01/19/05
    Jan Depner

    Optional average computation for attribute records.


    Version 4.53
    02/25/05
    Jan Depner

    Added open_existing_pfm_file function.  Changed all of the error numbers (except 
    SUCCESS) to negatives.  What the hell was I thinking???  Added the extern "C" stuff
    to pfm_extras.h.


    Version 4.54
    03/31/05
    Jan Depner

    Added PFM_DATA_TYPES to pfm.h.  This should only be used externally.  There were no
    changes to the library.


    Version 4.55
    05/10/05
    Jan Depner

    Made "Creating checkpoint file :" printout check for pfm_progress_callback so that 
    if you aren't printing percentage processed to screen it won't print out.


    Version 4.56
    06/07/05
    Graeme Sweet (IVS), Jan Depner

    Removed unset of suspect flag if bin is checked - in recompute_bin_values.


    Version 4.561
    08/14/05
    Danny Neville (IVS)
    
    Merged exisiting IVS PFM libraries with latest (4.56) from NAVO and added Windows equivalents 
    for stat and mkdir.


    Version 4.60
    10/26/05
    Jan Depner

    Changes to the 4.5 change to make all files exist in a single directory.  After 
    discussion with Mark Paton (IVS) we decided to have a "PFM handle" file in addition
    to the data directory.  This allows us to open a file instead of a directory.  This
    may seem minor but it was a PITA.  We may, at some future date, look into using
    the "PFM handle" file for PFM locking.


    Version 4.70
    11/03/05
    Jan Depner, Webb McDonald (SAIC)

    Changed attribute handling to loops with NUM_ATTR attributes in preparation for handling
    N and M attributes.  Bug fixes from Webb.


    Version 5.00
    11/08/05
    Jan Depner

    M bin and N ndx attributes, horizontal and vertical errors in ndx file, reserved name attributes.
    Minor change to allow the creation of PFMs with matching lat and lon bin sizes.  Change made to
    write_bin_header.  See comments before open_pfm_file.
    Fix for bug reading list file input names in open_list_file.  Removed list_file_seq array (not used).
    Added PFM_NAVO_LLZ_DATA type to PFM data types.  Moved the KEY structure back out to header.h.  Since 
    we're only including this in pfm_io.c it makes it removes a bunch of stuff that we really don't
    need to look at when we're editing (the already way too large) pfm_io.c.  Changed the name of header.h
    to pfm_header.h for clarity.


    Version 5.01
    01/19/07
    Jeff Parker (SAIC)

    Modified huge_io.c to prevent using an incorrect file pointer of an opened subfile when an hfread or hfwrite
    end exactly at the end of a subfile.
    Added a check in hyp_write_node when reading an address for a hypothesis against the number of hypotheses 
    expected.  Similar to the check existing in hyp_read_node. 

    Added PFM_LADSDB_DATA type.


    Version 5.1
    06/21/07
    Danny Neville (IVS)
    Graeme Sweet (IVS)
    William Burrow (IVS)
    Jeff Parker (SAIC)

    Major changes to speed up load.  This includes all of the pfm_cached_io code.  Really great stuff here!


    Version 5.2
    07/13/07
    Danny Neville (IVS)
    Graeme Sweet (IVS)
    William Burrow (IVS)
    Jeff Parker (SAIC)
    Jan Depner

    Final changes dealing with cache and bug fix from NAVO - in opening existing PFM file.
    open_args.bin_attr_offset and bin_attr_max were not being set and passed back to the caller.  Commented
    out "zeroing bin" code.  Added some comments to pfm_io.c.  Got rid of a couple of unused variables in
    pfm_cached_io.c.  Happy Friday the 13th ;-)


    Version 5.21
    08/06/07
    Jan Depner

    Bug fix for bad stupidity in read_bin_header (see stupidity comment in code ;-)  This was very insidious
    and may have been causing crashes when opening multiple PFMs at once.  That was how I found it.


    Version 5.22
    10/16/07
    Jeff Parker (SAIC)
    Jan Depner

    Bug fixes in cached code and change of max number of files (by Jeff).  Addition of pfm_geo_distance (JCD).


    Version 5.23
    01/11/08
    Jan Depner

    Changed substitute() to pfm_substitute().  Made it externally callable.  Fixed code to work on Windows.
    Assumption is that the substitute order is Windows first then UNIX.  For example:
    [SUBSTITUTE PATH] = X:,/data3/datasets
    Added path substitution to get_target_file() and get_mosaic_file().  Corrected error in pfm_find_startup().
    Added definition of SEPARATOR and PATH_SEPARATOR to pfm_nvtypes.h.
    Also, added DTED as data type 37.


    Version 5.24
    02/11/08
    Jan Depner

    Modified pfm_substitute to handle three path fields.  This will allow us to have a normal UNIX path name
    and a networked UNIX path name.  For example, /data1 and /net/pogo1/data1 or /.automount/pogo1/root/data1.
    On UNIX, the first UNIX path will have precedence over the second UNIX path.  For more details look at the code.
    Also, added the ability to ignore comments (lines beginning with #) in the .pfm_cfg file.


    Version 5.25
    04/03/08
    Jan Depner

    Minor bug fix in open_pfm_file.  Had to handle the new max_bin_attr values when opening a new file.  The exact
    comment is as follows:

	    If bin_attr_offset and bin_attr_max were not found, set them to min_bin_attr and max_bin_attr.

	    The bin_attr_max and bin_attr_offset values are holdovers from when bin and ndx attributes were
	    related.  Now that they are separate we want to use max_bin_attr and min_bin_attr from the header
	    structure.  Unfortunately the old values are still used throughout the code so we have to make
	    sure that we can use either the originals or the new ones.  I think Danny Neville or Graeme
	    Sweet took care of this when opening an existing file (the header strings were different
	    between old and new) but we need to do it here when opening a new file as well.

    The comments in the open_pfm_file header state that we need to set head.max_bin_attr but without this
    patch that is not the case.  I'm assuming everyone else has already dealt with this problem but, since
    I'm just getting in to the attributes it's new to me ;-)
    Also, added pfm_standard_attr_name function to pfm_extras.c and modified the comments about the 
    standard attribute names in pfm.h.
    One other thing.  I added a check for the IVS PCube header in the hypothesis files.  Hopefully, that is
    the only incompatibility ;-)


    Version 5.26
    12/10/08
    Jan Depner

    Re-use PFM_MRG_DATA (type 1).  Changed to PFM_CHRTR_DATA.


    Version 5.27
    03/02/09
    Jan Depner

    Re-use PFM_DEMO_DATA (type 7).  Changed to PFM_WLF_DATA.


    Version 5.28
    04/13/09
    Jan Depner

    Bug fix - in update_target_file and update_mosaic_file, if the temp file was shorter
    than the original list file we would leave garbage on the end of the list file when
    we wrote back to it from the temporary file.  So, instead of writing back to the
    original list file, we rename it to whatever.bak and then rename the temp file to
    whatever.ctl.
    Replaced PRId64 with NV_INT64_SPECIFIER in pfm_io.c and pfm_cached_io.c since PRId64
    is not supported on BSD and others.
    Fixed ftruncate on Windows problem by using _chsize.


    Version 5.29
    06/11/09
    Jan Depner

    Re-use PFM_SHOALS_XY2_DATA (type 4).  Changed to PFM_CHARTS_HOF_DATA.  Now loading both primary 
    and secondary returns with validity based on abdc and sec_abdc.  No longer performing swaps.
    Added check for file version newer than software version on open.


    Version 5.30
    06/26/09
    Jan Depner

    Back fit the double bit (un)pack fix for IVS files.
    IMPORTANT NOTE: This will not be carried forward into the (already existing) 6.0 since we
    switched the library to the IVS double bit (un)pack routines.


    Version 6.00
    02/05/10
    Jan Depner

    Removed redundant definition from pfm_io.c (already defined in pfm_nvtypes.h).  Added check for PFM_USER_05 to
    recompute_bin_values (must have missed this when we added the fifth flag).  Made open_pfm_file set the version
    and creation date when we are creating a new file (don't know how I missed that).


    Version 6.01
    03/12/10
    Jan Depner

    Corrected a very subtle bug when packing and unpacking attributes with a scale factor of 1.0.  If the numbers are large
    (greater than about 2^23 [see IEEE floating point format spec]) we were getting a round off error in the conversion from
    an NV_FLOAT32 to the nearest integer.  This isn't a problem with any of the normal values in the depth or bin record
    for two reasons.  First, the numbers don't exceed 2^23 (unless you're scaling to millimeters which is pretty absurd).
    Second, we were sending in floating point numbers expecting a bit of round off.  The integer values that we normally 
    store are passed in as integers and there's no scaling going on so they don't have that problem.  With the attributes
    we don't really know what we're going to be storing so we need to deal with this.  To fix the problem I type cast the parts
    (min, scale) to NV_FLOAT64 before doing the calculations.  This happened in read_depth_record/unpack_bin_record (unpack)
    and pack_depth_record/pack_bin_record (pack).  This is just a minor bug fix and it shouldn't have any effect on compatibility.


    Version 6.02
    06/23/10
    Jan Depner

    Added PFM_HAWKEYE_DATA type.


    Version 6.03
    11/15/10
    Jan Depner

    Added PFM_BAG_DATA type.


    Version 6.04
    12/02/10
    Jeff Parker (SAIC)

    Fix for very rare bug in hfread, hfwrite, hfseek, and hyp.c.  It took Jeff quite a while to dig this one out ;-)
    He also added read and write error checking in hyp.c.


    Version 6.05
    01/12/11
    Jan Depner

    I just realized that we had changed the number of bits for files from 12 (4096) to 13 (8192) but there were 4
    arrays in pfm_io.c that were set to 4096 instead of 8192..  Modified pfm_cached_io.c fprintf statements
    in the dump_cached_record(s) functions to handle the %lld spec properly.  Modified files to support __MINGW64__.
    Also, split PFM_HAWKEYE_DATA type to PFM_HAWKEYE_HYDRO_DATA and PFM_HAWKEYE_TOPO_DATA.


    Version 6.06
    04/11/11
    Jan Depner

    Simple changes to alleviate a problem in MINGW.  The rename function used in update_target_file and update_mosaic_file
    doesn't like mixed path separators (/ and \).  In order to fix it I change the functions to read the ctl file, make
    a backup file, remove the ctl file, and then create a new ctl file.  It's not quite as atomic as using rename but
    the ctl files only take a few seconds to copy (and we have a backup file) so it should be OK.  While I was in there
    I added fflush (stderr); to each debug output so that they will flush on Windows.


    Version 6.07
    06/22/11
    Jan Depner

    We can now open the bin and index files read-only and not fail.  This was originally supposed to work but it didn't.
    Also, moved the definition of the data types up to the top of open_pfm_file so that they get defined even if you
    error out on the open.


    Version 6.08
    09/16/11
    Jan Depner

    Changes to comments in all files in order to work with Doxygen.  If you have Doxygen on your system you can now
    just enter "doxygen" in this folder and HTML documentation will be generated in the PFM_API_Documentation/html
    folder.


    Version 6.09
    10/28/11
    Jan Depner

    Minor change to write_bin_header to speed up initialization of new bin files.


    Version 6.10
    12/29/11
    Jan Depner

    I missed commenting out one of the "forced fseek" print statements during the last change from SAIC.  I just
    hit it today so I went in and commented it out.  This should have absolutely no impact other than to get
    rid of a bunch of prints.


    Version 6.11
    01/03/12
    Jan Depner

    - Made code thread safe (per handle) and removed support for huge_io files.  Code is thread safe for reading
    and writing with the exception of the open_pfm_file, open_existing_pfm_file, close_pfm_file, pfm_basename,
    pfm_dirname, and pfm_standard_attr_name.
    - Removed support for pre 3.0 files.  We moved to 3.0 over ten years ago so this shouldn't be a problem.
    - Increased the maximum number of open PFM files to 150 from 32 (to support multi-threaded applications).


    Version 6.12
    01/24/12
    Jeff Parker (SAIC)

    - Changed values for PFM_MRG_DATA, PFM_SHOALS_XY2_DATA, PFM_DEMO_DATA (retired data types) from 0, 0, 0 to
    1001, 1002, 1003.
    - Set the [starting] min_bin_attr and max_bin_attr from 0.0, 0.0 to 99999.0, -99999.0.
    - Added 'shoal' to the hypothesis structures.  In order to calculate the shoal depth of the valid soundings
    in each hypothesis, [we] have to keep track at the hypothesis level and then save the best (chosen)
    hypothesis's shoal depth in the new PFM bin attribute.
    - Moved HYP_VERSION to 2.10.
    - Added code to read/write 'shoal' in the hypothesis structures if the hyp_version > 2.0 


    Version 6.13
    04/13/12
    Jan Depner
    Jeff Parker (SAIC)
    Danny Neville (QPS)

    - Fixed mutexes for multiple PFMs opened multiple times.
    - Combined flush_bin_cache and destroy_bin_cache since they were always called in tandem.
    - Removed the #define DESTROY_BIN_CACHE 1 statement since it was no longer used.
    - Changes to hyp.c, hyp.h, and pfm_nvtypes.h to match the last of the v5 API changes.


    Version 6.14
    10/16/12
    Will Burrow (QPS)

    - A patch of mostly cosmetic changes.  There is a typedef fix, a change to some function parameter names
    in pack_bin_record() to fix a problem with variable names hidden by scope, and parameter name changes to
    some cache functions to clarify usage.
    - Removed definition of NV_FLOAT80 from pfm_nvtypes.h


    Version 6.15
    07/18/13
    Will Burrow (QPS), Jan Depner (PFM Software)

    - Minor changes in read_depth_record to provide correct resolution to floating point calculations when 
      built on a 64-bit system using SSE.
    - Added get_depth_precision, get_std_precision, get_x_precision, get_y_precision, and file pfm_internals.h with
      declarations for those functions.
    - Bug fix for pre 4.0 files (wasn't setting the PFM_DATA flag in recompute_bin_values).
    - Removed -Wunused-but-set-variable "count" from recompute_bin_values.
    - Corrected -Wunused-but-set-variable warnings for "hit" and "k" in pfm_substitute (these are only used
      on Windows).


    Version 6.16
    01/30/14
    Jeff Parker (Leidos)
    Dwayne MacLeod (QPS)
    Jan Depner (PFM Software)

    - fixed an unititialized 'ptr' in hyp.c.
    - added code to 'add_cached_depth_record' in pfm_cached_io.c to remove the PFM_CHECKED flag from the validity 
      if a new sounding is added to a bin.
    - added PFM_INVAL to flags being initially cleared in recompute_bin_values in pfm_io.c.
    - Replaced all code where filename was expected to be 256 bytes or smaller with 512 bytes or smaller.


    Version 6.17
    05/06/14
    Jan Depner (PFM Software)

    - Increased polygon count from 200 to 2000 in BIN_HEADER structure.
    - Increased default header size from 16384 to 131072 to accommodate extra polygon points (2000 points would take 70KB in the header).
    - Added return value checks for fgets, fread, and fscanf to get rid of GCC warnings.  These should never happen.


    Version 6.18
    06/09/14 - J.Parker (Leidos)

    - Moved the prototypes from pfm_internals.h to pfm.h.
    - Removed reference to pfm_internals.h in pfm_io.c.
    - The file pfm_internal.h will not be distributed.


    Version 6.19
    07/23/14
    Jan Depner (PFM Software)

    - Switched from defining our own size constrained data types to using stdint.h and inttypes.h.  This should not be considered
      a change in terms of the format or anything else for that matter.  We've just changed the names of our fixed size fields to
      ones that are supported in the C99 standard stdint.h and inttypes.h files.
    - Fixed a number of possible resource leaks (not likely to happen) and one, for sure, memory leak in get_data_extents.
    - Removed M_PI and M_PI_2 definitions because they weren't being used.
    - Changed A0 and B0 to PFM_A0 and PFM_B0 to avoid any possible conflicts outside of the API.
    - Added #ifndef/#endif around definition of NINT to avoid possible conflicts outside of the API.


    Version 6.20
    12/18/14
    Jan Depner (PFM Software)

    - Replaced the PointInsidePolygon function.  The one we were using was a copy of W Randolph Franklin's function.
      His function was copyrighted and distribution restricted.  I don't know how it got in there but I've written
      my own version based on the CrossingsMultiplyTest method described by E. Haines.  It is just as fast as
      Franklin's function.


    Version 6.30
    04/09/15
    Will Burrow (QPS), Jan Depner (PFM Software)

    - Correction for computing bits and offset for bin attributes.  I (Jan Depner) broke it when we went to
      separate bin and index attributes back in August of 2006.  On the bright side, Will didn't discover it until
      a few weeks ago ;-)


    Version 6.40
    06/18/17
    Brett Goldenbloome (Leidos), Jan Depner (PFM Software)

    - After using the bit_pack and bit_unpack functions for 18 years with no apparent problems, Brett Goldenbloome
      at Leidos, Newport RI discovered that, in extremely rare circumstances, the functions would access one byte
      past the end of the buffer.  I have added an "if" statement (shudder) to correct the problem.


    Version 6.50
    12/21/18
    Jan Depner (PFM Software)

    - Increased PFM_FILE_BITS to 14 and PFM_MAX_FILES to 16384.
    - Increased size of some character strings to avoid possible overflows.


    Version 7.00
    05/15/19
    Jan Depner (PFM Software)

    - Added 10 attributes for a total of 20
    - Added a new permanent layer to the BIN file - number of valid points
    - Added 5 new single bit user flag to validity - PFM_USER_06 through PFM_USER_10
    - Added PFM_USER_FLAGS constant (number of user flags)
    - I was going to add an array of flags for the point and bin attributes to denote float or integer attribute
      but then I realized that they were already there... kinda.  In every case I've ever seen, integer attributes
      ALWAYS have a scale value of 1.0 (nothing else makes any sense).  Even if I had made a union of the float
      and integer attributes, the application would still have to check to see what type it is.  The easist way
      to handle this is to do the type check on file open, compute an array of formats based on the scale, then
      use the NINT macro to convert the float value to the proper integer for the integer attributes.
    - Moved the coverage map out of the bin file
    - Added PFM_PERM_LAYERS constant
    - REMOVED SUPPORT FOR PRE-6.0 PFMS!  Since v6 appeared 9 years ago, I'm not too worried about this.  If you
      have some 9 year old (or more) PFMs around, that's your problem.  Once again, PFM IS A WORKING FORMAT!
      Make your edits, then save the data back to the input files.  On the bright side, you'll never have to
      look at a "hugefile" again.

</pre>*/
