
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



/*! \mainpage Pure FIle Magic (PFM) API

    <br><br>\section disclaimer Disclaimer

    This is a work of the US Government. In accordance with 17 USC 105, copyright
    protection is not available for any work of the US Government.

    Neither the United States Government nor any employees of the United States
    Government, makes any warranty, express or implied, without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes
    any liability or responsibility for the accuracy, completeness, or usefulness
    of any information, apparatus, product, or process disclosed, or represents
    that its use would not infringe privately-owned rights. Reference herein to
    any specific commercial products, process, or service by trade name, trademark,
    manufacturer, or otherwise, does not necessarily constitute or imply its
    endorsement, recommendation, or favoring by the United States Government. The
    views and opinions of authors expressed herein do not necessarily state or
    reflect those of the United States Government, and shall not be used for
    advertising or product endorsement purposes.



    <br><br>\section date Introduction

    This library provides all of the I/O functions for the PFM data editor.  The PFM
    files consist of a file list (or control) file, a line file, a bin file, and an
    index file.<br><br>
    PFM, or Pure File Magic, was conceived (watch it) on a recording trip to Nashville.
    The design was refined over the next year or so by the usual band of suspects.
    The original purpose of this little piece of work was to allow hydrographers to
    geographically view minimum, maximum, and average binned surfaces, of whatever bin
    size they chose, and then allow them to edit the original depth data.  After editing
    the depth data, the bins would be recomputed and the binned surface redisplayed.
    The idea being that the hydrographer could view the min or max binned surface to find
    flyers and then just edit those areas that required it.  In addition to manual viewing
    and hand editing, the PFM format is helpful for automatic filtering in places where
    data from different files overlaps.  After editing of data is complete the status
    information can be loaded back into the original data files be they GSF, SHOALS, HOF,
    TOF, or any other file type for which a loader/unloader has been developed.  In this
    way you aren't forced to use a specific format to store/manage your data.

    I would like to acknowledge the contributions to the PFM design of the usual band
    of suspects:
         - Jim Hammack, NAVO
         - Dave Fabre, Neptune Sciences, Inc.
         - Becky Martinolich, NAVO
         - Dwight Johnson, NAVO
         - Shannon Byrne, SAIC
         - Mark Paton, IVS

         - Jan 'Evil Twin' Depner
         - Naval Oceanographic Office
         - are.based.editor@gmail.com


     More recent contributors to the project (without whom this dog wouldn't hunt):
         - Danny Neville, IVS
         - Jeff Parker, SAIC
         - Graeme Sweet, IVS
         - Webb McDonald, SAIC
         - William Burrow, IVS
         - R. Wade Ladner, NAVO



     <br><br>\section format PFM Format

     The PFM data structure consists of a single .pfm handle file and a .pfm.data data directory.  Inside the data directory are a
     number of other files.  In general terms, the PFM structure consists of an ASCII control file (.ctl) containing the names of
     all of the associated PFM and non-PFM files, a binned surface file (.bin) containing all of the binned surfaces and links to
     the indexed data, an indexed file (.ndx) containing the original input data and status information, an optional ASCII line
     file (.lin) containing line names, and an optional CUBE hypothesis file (.hyp) containing CUBE hypotheses.  A simple graphical
     overview of the structure follows.  Note that the diagram does not show the handle file or the optional CUBE hypotheses file.
     It is important to note that, since all of the data in these files, with the exception of the .hyp file, is stored as either
     ASCII characters or bit packed, scaled, offset, unsigned integers in unsigned character blocks there is no "ENDIANNESS" to
     the files.  That is, we do not have to worry about operating system byte ordering.

     \image html PFM.png

     As can be seen by the above diagram, PFM allows for very quick access to any data based on its geographic location within the
     binned surface. In addition, the original input file/record/subrecord can be accessed easily by using the file number, ping (record)
     number, and beam (subrecord) number from the index file in combination with the input file name from the ctl file.  The ability
     to directly address a record/subrecord in an input file is required for loading data into the PFM structure.  The other requirements
     for loading data into PFM are that the data points have some form of position, a Z value, and a way to save point status (if you
     intend to unload your edits back to the input files).  Note that positive Z is always down since this API was developed by the Naval
     Oceanographic Office (not the Naval Topographic Office ;-)



     <br><br>\section handle PFM Handle File (.pfm)

         - All records are line feed separated, ASCII character strings

         - First record :
             - Software version information (e.g. PFM Handle File - PFM Software - PFM I/O library V7.00 - 05/15/19)

         - Subsequent records:
             - Comments : all lines must start with a # character.  There is a boilerplate section of comments that gets added
               just to let people know that this is a handle file and not an older PFM version list file.  After that you can
               add any comments you want.  This file is handy for saving load information.  Just add comments to the end of the
               file.



     <br><br>\section control PFM Control File (.ctl)

         - All records are line feed separated, ASCII character strings

         - First record :
             - Software version information (e.g. PFM Software - PFM I/O library V7.00 - 05/15/19)

         - Second record :
             - Bin (.bin) file name (not used except in the case of older, pre-4.7 PFMs)

         - Third record :
             - Index (.ndx) file name (not used except in the case of older, pre-4.7 PFMs)

         - Fourth record :
             - Image mosaic (geoTIFF (.tif)) file name

         - Fifth record :
             - Feature (.bfd (Binary Feature Data) or .xml) file name

         - Subsequent records :
             - A + (non-deleted files) or - (deleted files), a space, a five digit sequence number, a space, a two digit data type number,
               a space, and the input file name.<br>
             - A + sign in the first column signifies a file that has not been marked as "deleted" in the PFM structure.  A - sign signifies
             that a file has been "deleted".  Files that are marked as "deleted" have had the PFM_DELETED bit flag set in the status
             information for each data point for that file in the indexed file.  These points should be completely ignored by any applications
             that use the PFM data.  The sequence number is used to make sure that a user has not inadvertently (or vertently for that
             matter ;-) deleted a line from the list file.  File names must never be deleted or added to this file nor may the order of the
             files be changed manually (using an editor).  In addition, do not ever manually change the +/- or the data type.  The currently
             defined data types are as follows:
             <br>
             <br>
             <pre>
             PFM_UNDEFINED_DATA = Undefined
             PFM_CHRTR_DATA = NAVOCEANO CHRTR format
             PFM_GSF_DATA = Generic Sensor Format
             PFM_SHOALS_OUT_DATA = Optech SHOALS .out format
             PFM_CHARTS_HOF_DATA = CHARTS Hydrographic Output Format
             PFM_NAVO_ASCII_DATA = NAVOCEANO ASCII XYZ format
             PFM_HTF_DATA = Royal Australian Navy HTF
             PFM_WLF_DATA = Waveform LIDAR Format
             PFM_DTM_DATA = IVS DTM data format
             PFM_HDCS_DATA = Caris HDCS data format
             PFM_ASCXYZ_DATA = Ascii XYZ data format
             PFM_CNCBIN_DATA = C&C Binary XYZ data format
             PFM_STBBIN_DATA = STB Binary XYZ data format
             PFM_XYZBIN_DATA = IVS XYZ Binary data format
             PFM_OMG_DATA = OMG Merged data format
             PFM_CNCTRACE_DATA = C&C Trace data format
             PFM_NEPTUNE_DATA = Simrad Neptune data format
             PFM_SHOALS_1K_DATA = Shoals 1K(HOF) data format
             PFM_SHOALS_ABH_DATA = Shoals Airborne data format
             PFM_SURF_DATA = Altas SURF data format
             PFM_SMF_DATA = French Carribes format
             PFM_VISE_DATA = Danish FAU data format
             PFM_PFM_DATA = NAVOCEANO PFM data format
             PFM_MIF_DATA = MapInfo MIF format
             PFM_SHOALS_TOF_DATA = Shoals TOF data format
             PFM_UNISIPS_DEPTH_DATA = UNISIPS depth data format
             PFM_HYD93B_DATA = Hydro93 Binary data format
             PFM_LADS_DATA = Lads Lidar data format
             PFM_HS2_DATA = Hypack data format
             PFM_9COLLIDAR = 9 Column Ascii Lidar data format
             PFM_FGE_DATA = Danish Geographic FAU data format
             PFM_PIVOT_DATA = SHOM Pivot data format
             PFM_MBSYSTEM_DATA = MBSystem data format
             PFM_LAS_DATA = LAS data format
             PFM_BDI_DATA = Swedish Binary DIS format
             PFM_NAVO_LLZ_DATA = NAVO lat/lon/depth data format
             PFM_LADSDB_DATA = Lads Database Link format
             PFM_DTED_DATA = NGA DTED format
             PFM_HAWKEYE_HYDRO_DATA = Hawkeye CSS Generic Binary Output Format (hydro)
             PFM_HAWKEYE_TOPO_DATA = Hawkeye CSS Generic Binary Output Format (topo)
             PFM_BAG_DATA = Bathymetric Attributed Grid format
             PFM_ZMIL_DATA = Coastal Zone Mapping and Imaging LIDAR Format
             </pre>


         - Sample .ctl file :
         <br>
         <br>
         <pre>
         PFM Software - PFM I/O library V6.12 - 01/24/12
         test.pfm.data/test.pfm.bin
         test.pfm.data/test.pfm.ndx
         /data1/tags1/test_990508_2/em3000/liberty.tif
         /data1/tags1/test_990508_2/em3000/liberty.bfd
         + 00000 02 /data1/tags1/test_990508_2/em3000/99mbg991281726.d01
         + 00001 02 /data1/tags1/test_990508_2/em3000/99mbg991281738.d01
         + 00002 02 /data1/tags1/test_990508_2/em3000/99mbg991281742.d01
         + 00003 02 /data1/tags1/test_990508_2/em3000/99mbg991281753.d01
         + 00004 02 /data1/tags1/test_990508_2/em3000/99mbg991281813.d01
         + 00005 02 /data1/tags1/test_990508_2/em3000/99mbg991281816.d01
         </pre>



     <br><br>\section line PFM Line File (.lin)

         - All records are line feed separated, ASCII character strings

         - All records :
             - Line descriptor (no more than 512 characters)

         - Caveats :
             - The line file is a text file containing line descriptor information. As with the ctl file this file can be edited to
               change the line descriptions.  This file is always called <pfm handle file>.lin.  Accidentally (or purposely) removing
               this file is not a problem since the contents are only labels.  If this file doesn't exist (as with a PFM 3.X file)
               lines are listed as UNDEFINED.  The line file can be modified with any ASCII editor and the contenets of any line can
               be changed.  For example, you can change "99mbg991281816.d01-1999-128-18:16:51" to "This line really sucks!".  Do not
               remove or swap any lines.  Their position in the file relates to their line number in the index file.  In many cases
               you will only have one line per input file.

         - Sample .lin file :
         <br>
         <br>
         <pre>
         99mbg991281721.d01-1999-128-17:21:27 
         99mbg991281721.d01-1999-128-17:23:30 
         99mbg991281726.d01-1999-128-17:26:07 
         99mbg991281726.d01-1999-128-17:31:49 
         99mbg991281726.d01-1999-128-17:32:07 
         99mbg991281726.d01-1999-128-17:34:19 
         99mbg991281738.d01-1999-128-17:38:02 
         99mbg991281738.d01-1999-128-17:39:19 
         99mbg991281742.d01-1999-128-17:42:41 
         99mbg991281742.d01-1999-128-17:43:37 
         99mbg991281742.d01-1999-128-17:47:57 
         99mbg991281742.d01-1999-128-17:48:01 
         99mbg991281753.d01-1999-128-17:53:07 
         99mbg991281753.d01-1999-128-17:54:22 
         99mbg991281753.d01-1999-128-18:02:12 
         99mbg991281813.d01-1999-128-18:13:39 
         99mbg991281813.d01-1999-128-18:13:56 
         99mbg991281816.d01-1999-128-18:16:51
         </pre>



     <br><br>\section bin PFM Bin File (.bin)

     The bin file is made up of an ASCII header block, a number of bin records (based on width times height of the area in
     bins), and a coverage map.  The bins are stored starting in the southwest corner of the area and proceeding west to
     east then south to north (row major).  In other words, if you have a 10X10 area, the southwest corner bin will be (0,0),
     the southeast corner bin will be (0,9), the northwest corner bin will be (9,0), and the northeast corner bin will be (9,9).
     The data in each bin record in the bin file includes minimum filtered/edited depth, maximum filtered/edited depth, average
     filtered/edited depth, minimum depth, maximum depth, average depth, number of valid and invalid soundings, standard
     deviation of the valid soundings, a validity field, up to ten optional attributes, a depth chain head pointer, and a depth
     chain tail pointer. The average filtered/edited depth field may be replaced with some other surface (such as a CUBE surface
     or a minimum curvature spline interpolated surface (MISP)) but the name of the average filtered surface ([AVERAGE FILTERED NAME]
     or [AVERAGE EDITED NAME]) must be changed in the bin header so that the library doesn't automatically try to insert the average
     value on recompute.  With the exception of the optional attributes, some of the status/validity flags, and, possibly, the
     average filtered depth (if you have changed its name), the fields in each bin record are computed from the contents of the
     associated depth chain in the index (.ndx) file.

         - Header block (address 0) :
             - [BIN HEADER SIZE] (default is 16384) byte header
             - Unless otherwise noted, all "BITS" fields are computed using the following standard equation (in pseudo-code):<br>
               NEAREST_INTEGER (LOG ((([MAXIMUM VALUE] - [MINIMUM VALUE] + 1.0) * [SCALE VALUE])) / LOG (2.0) + 0.5)<br>
               Even though this should be intuitively obvious to the most casual observer, some explanation may be necessary.  Taking
               the log of a number and dividing that by the log of 2 will give you the power to which you must raise 2 in order to
               get that number.  Since the number of bits needed to store a value is just the power to which you raise 2 to get that 
               value, this gives us the number of bits needed to store the maximum value in the range.  Adding 0.5 to the value and
               using NEAREST_INTEGER forces the integer answer up to the next power of 2 since we can't use partial bits (you actually
               can but it's a lot more complicated).  Adding 1.0 to the range of values gives us space to place a null value.  That is,
               the null value is defined as the offset maximum plus 1.  When we store a value in these fields it is converted from a
               floating point value to an offset, scaled integer value using the following equation (in pseudo-code):<br>
               NEAREST_INTEGER ((INPUT_VALUE - [MINIMUM VALUE]) * [SCALE VALUE])
             - All records are line feed separated, tagged ASCII character strings, including, but not limited to :
             - [VERSION] = PFM API software version information (e.g. [VERSION] = PFM Software - PFM I/O library V7.00 - 05/15/19)
             - [RECORD LENGTH] = Index file physical record (bucket) length (in number of soundings)
             - [DATE] = Date of creation in some human readable format (this isn't programmatically important, it's just there so you
               can read it)
             - [CLASSIFICATION] = Classification information
             - [CREATION SOFTWARE] = Software that created the PFM (e.g. [CREATION SOFTWARE] = PFM Software - pfmLoad V7.31 - 07/17/18)
             - [MIN Y] = geographic or projected coordinates
             - [MIN X] = geographic or projected coordinates
             - [MAX Y] = geographic or projected coordinates
             - [MAX X] = geographic or projected coordinates
             - [BIN SIZE XY] = Bin size in meters (if 0.0 then X BIN SIZE and Y BIN SIZE are actual sizes in degrees and may not be equal in distance)
             - [X BIN SIZE] = (in degrees or projected units, usually computed from BIN SIZE XY when header is written)
             - [Y BIN SIZE] = (in degrees or projected units, usually computed from BIN SIZE XY when header is written)
             - [BIN WIDTH] = (in number of bins, computed when header is written)
             - [BIN HEIGHT] = (in number of bins, computed when header is written)
             - [MIN FILTERED DEPTH] = minimum valid (i.e. filtered or edited) depth
             - [MAX FILTERED DEPTH] = maximum valie (i.e. filtered or edited) depth
             - [MIN FILTERED COORD] = bin coordinates of minimum valid depth
             - [MAX FILTERED COORD] = bin coordinates of maximum valid depth
             - [MIN DEPTH] = minimum depth (valid or invalid)
             - [MAX DEPTH] = maximum depth (valid or invalid)
             - [MIN COORD] = bin coordinates of minimum depth
             - [MAX COORD] = bin coordinates of maximum depth
             - [BIN HEADER SIZE] = size of bin header block (if this field is missing it is set to 16384)
             - [COUNT BITS] = number of bits used for the record count in the bin file (default = PFM_COUNT_BITS)
             - [STD BITS] = number of bits used for standard deviation value in the bin file (default = PFM_STD_BITS)
             - [STD SCALE] = scale factor for standard deviation values (default = PFM_STD_SCALE)
             - [DEPTH BITS] = number of bits used for depths in the index and bin files (computed using standard BITS equation)
             - [DEPTH SCALE] = scale factor for depths
             - [DEPTH OFFSET] = pre-scaled depth offset to make all depth values positive (so we don't have to deal with signed, bit-packed values)
             - [RECORD POINTER BITS] = number of bits used for record pointers in the bin file and the continuation pointers in the index file
               (default = PFM_RECORD_POINTER_BITS)
             - [FILE NUMBER BITS] = number of bits used for input file number in the index file (default = PFM_FILE_BITS)
             - [LINE NUMBER BITS] = number of bits used for input line number in the index file (default = PFM_LINE_BITS)
             - [PING NUMBER BITS] = number of bits used for input ping/record number in the index file (default = PFM_PING_BITS)
             - [BEAM NUMBER BITS] = number of bits used for input beam/subrecord number in the index file (default = PFM_BEAM_BITS)
             - [OFFSET BITS] = number of bits used for X/Y offsets in the index file (default = PFM_OFFSET_BITS)
             - [VALIDITY BITS] = number of bits used for data validity flags in the bin and index files (default = PFM_VALIDITY_BITS)
             - [POINT] = SXX.XXXXXXX,SXXX.XXXXXXX (repeating bounding polygon points for the PFM, up to 2000)
             - [MINIMUM BIN COUNT] = minimum number of valid or invalid points in any bin
             - [MAXIMUM BIN COUNT] = maximum number of valid or invalid points in any bin
             - [MIN COUNT COORD] = bin coordinates of bin with the fewest points
             - [MAX COUNT COORD] = bin coordinates of bin with the most points
             - [MIN STANDARD DEVIATION] = minimum standard deviation in any bin
             - [MAX STANDARD DEVIATION] = maximum standard deviation in any bin
             - [CHART SCALE] = chart scale (1:CHART SCALE - This is not required or used much anymore)
             - [CLASS TYPE] = 0, 1, 2, 3 ... 9 (bins computed from all data, or bins computed from user flag 1, 2, 3 ... 10 data.  This flag
               will be set by an external program to tell the library how to do the bin recompute process.  Normally, this is set to 0.
             - [WELL-KNOWN TEXT] = Well-known text is, according to Wikipedia, <b>a text markup language for representing vector geometry
               objects on a map, spatial reference systems of spatial objects and transformations between spatial reference systems.
               The formats are regulated by the Open Geospatial Consortium (OGC) and described in their Simple Feature Access and Coordinate
               Transformation Service specifications.</b>  The [WELL-KNOWN TEXT] field supecedes the [PROJECTION] fields (although the
               [PROJECTION] fields are still supported).  An example of a well-known text field for a simple geographic WGS84/WGS84
               datum would be:
               <br>
               <br>
               COMPD_CS["WGS84 with ellipsoid Z",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9108"]],AXIS["Lat",NORTH],AXIS["Long",EAST],AUTHORITY["EPSG","4326"]],VERT_CS["ellipsoid Z in meters",VERT_DATUM["Ellipsoid",2002],UNIT["metre",1],AXIS["Z",UP]]]
               <br>
               <br>
             - [PROJECTION] = Pre 6.0 projection information, see pfm.h for all projection definitions
             - [PROJECTION ZONE] = See pfm.h for all projection definitions
             - [HEMISPHERE] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 0] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 1] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 2] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 3] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 4] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 5] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 6] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 7] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 8] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 9] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 10] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 11] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 12] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 13] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 14] = See pfm.h for all projection definitions
             - [PROJECTION PARAMETER 15] = See pfm.h for all projection definitions
             - [AVERAGE FILTERED NAME] = Name of the "average filtered" surface.  This can be used for other surface types (e.g. CUBE or MISP).
               If this is set to "Average Filtered Depth" or "Average Edited Depth" the surface is recomputed by the library, otherwise it can
               be computed by outside applications and will not be modified by the library.  The default is "Average Filtered Depth".
             - [AVERAGE NAME] = Name of the "average" surface.  This can be used for other surface types (e.g. CUBE or MISP).  Whether the
               surface is recomputed or not depends on the AVERAGE FILTERED NAME above.  If that is set to "Average Filtered Depth" the value
               is recomputed by the library, otherwise it can be computed by outside applications and will not be modified by the library.
               The default for this is "Average Depth".
             - [DYNAMIC RELOAD] = Boolean flag to indicate that this file will be dynamically unloaded/reloaded during editing.  By default
               this is set to 1 so Fledermaus will unload after edit.  This flag is ignored by the Area-Based Editor (ABE).
             - [NUMBER OF BIN ATTRIBUTES] = number of populated bin attributes up to a max of NUM_ATTR
             - [BIN ATTRIBUTE 0] = Name of the first bin attribute, 30 characters max
                 - 
                 - 
                 - 
             - [BIN ATTRIBUTE N] = Name of the Nth bin attribute, 30 characters max
             - [BIN ATTRIBUTE OFFSET 0] = This is a holdover from when bin attributes were based on ndx attributes.  Unfortunately we
               can't easily get rid of it.  This will always be the same as the [MINIMUM BIN ATTRIBUTE].
                 -
                 -
                 -
             - [BIN ATTRIBUTE OFFSET N] = This is the last bin attribute offset
             - [BIN ATTRIBUTE MAX 0] = This is a holdover from when bin attributes were based on ndx attributes.  Unfortunately we
               can't easily get rid of it.  This will always be the same as the [MAXIMUM BIN ATTRIBUTE].
                 -
                 -
                 -
             - [BIN ATTRIBUTE MAX N] = This is the last bin attribute maximum
             - [BIN ATTRIBUTE NULL 0] = This is a holdover from when bin attributes were based on ndx attributes.  Unfortunately we
               can't easily get rid of it.  This will always be set to the offset [MAXIMUM BIN ATTRIBUTE] + 1.0.
                 -
                 -
                 -
             - [BIN ATTRIBUTE NULL N] = This is the last bin attribute null
             - [MINIMUM BIN ATTRIBUTE 0] = Minimum value for bin attribute 0
             - [MAXIMUM BIN ATTRIBUTE 0] = Maximum value for bin attribute 0
                 - 
                 - 
                 - 
             - [MINIMUM BIN ATTRIBUTE N] = Minimum value for bin attribute N
             - [MAXIMUM BIN ATTRIBUTE N] = Maximum value for bin attribute N
             - [BIN ATTRIBUTE BITS 0] = Number of bits used to store bin attribute 0 (computed using standard BITS equation)
                 - 
                 - 
                 - 
             - [BIN ATTRIBUTE BITS N] = Number of bits used to store bin attribute N
             - [BIN ATTRIBUTE SCALE 0] = Scale factor for bin attribute 0
                 - 
                 - 
                 - 
             - [BIN ATTRIBUTE SCALE N] = Scale factor for bin attribute N
             - [NUMBER OF NDX ATTRIBUTES] = number of populated index attributes up to a max of NUM_ATTR
             - [NDX ATTRIBUTE 0] = Name of the first ndx attribute, 30 characters max
                 - 
                 - 
                 - 
             - [NDX ATTRIBUTE N] = Name of the Nth ndx attribute, 30 characters max
             - [MINIMUM NDX ATTRIBUTE 0] = Minimum value for ndx attribute 0
             - [MAXIMUM NDX ATTRIBUTE 0] = Maximum value for ndx attribute 0
                 - 
                 - 
                 - 
             - [MINIMUM NDX ATTRIBUTE N] = Minimum value for ndx attribute N
             - [MAXIMUM NDX ATTRIBUTE N] = Maximum value for ndx attribute N
             - [NDX ATTRIBUTE BITS 0] = Number of bits used to store ndx attribute 0 (computed using standard BITS equation)
                 - 
                 - 
                 - 
             - [NDX ATTRIBUTE BITS N] = Number of bits used to store ndx attribute N
             - [NDX ATTRIBUTE SCALE 0] = Scale factor for ndx attribute 0
                 - 
                 - 
                 - 
             - [NDX ATTRIBUTE SCALE N] = Scale factor for ndx attribute N
             - [USER FLAG 1 NAME] = Name of the 1st application defined user flag, 30 characters max (default = PFM_USER_01)
             - [USER FLAG 2 NAME] = Name of the 2nd application defined user flag, 30 characters max (default = PFM_USER_02)
             - [USER FLAG 3 NAME] = Name of the 3rd application defined user flag, 30 characters max (default = PFM_USER_03)
             - [USER FLAG 4 NAME] = Name of the 4th application defined user flag, 30 characters max (default = PFM_USER_04)
             - [USER FLAG 5 NAME] = Name of the 5th application defined user flag, 30 characters max (default = PFM_USER_05)
             - [USER FLAG 6 NAME] = Name of the 6th application defined user flag, 30 characters max (default = PFM_USER_06)
             - [USER FLAG 7 NAME] = Name of the 7th application defined user flag, 30 characters max (default = PFM_USER_07)
             - [USER FLAG 8 NAME] = Name of the 8th application defined user flag, 30 characters max (default = PFM_USER_08)
             - [USER FLAG 9 NAME] = Name of the 9th application defined user flag, 30 characters max (default = PFM_USER_09)
             - [USER FLAG 10 NAME] = Name of the 10th application defined user flag, 30 characters max (default = PFM_USER_10)
             - [COVERAGE MAP ADDRESS] = address of the coverage map at the end of the bin file (or 0 for 7.0 to indicate the coverage map is
                                        external)
             - [HORIZONTAL ERROR BITS] = number of bits used to store the horizontal uncertainty (computed using standard BITS equation with min of 0.0)
             - [HORIZONTAL ERROR SCALE] = scale factor for horizontal uncertainty
             - [MAXIMUM HORIZONTAL ERROR] = maximum horizontal uncertainty value
             - [VERTICAL ERROR BITS] = number of bits used to store the vertical uncertainty (computed using standard BITS equation with min of 0.0)
             - [VERTICAL ERROR SCALE] = scale factor for vertical uncertainty
             - [MAXIMUM VERTICAL ERROR] = maximum vertical uncertainty value
             - [NULL DEPTH] = null depth value
             - The remainder of the header block (up to [BIN HEADER SIZE] after the [NULL DEPTH] entry) is zero filled.
         <br>
         <br>
         - Sample bin header:
         <br>
         <br>
         <pre>
         [VERSION] = PFM Software - PFM I/O library V7.00 - 05/15/19
         [RECORD LENGTH] = 6
         [DATE] = Wed Feb  8 14:54:21 2012
         [CLASSIFICATION] = UNCLASSIFIED
         [CREATION SOFTWARE] = PFM Software - pfmLoad V7.31 - 07/17/18
         [MIN Y] = 30.162500000
         [MIN X] = -88.759722222
         [MAX Y] = 30.169446352
         [MAX X] = -88.744440674
         [BIN SIZE XY] = 2.000102817037066
         [X BIN SIZE] = 0.000020762972994
         [Y BIN SIZE] = 0.000018042473833
         [BIN WIDTH] = 736
         [BIN HEIGHT] = 385
         [MIN FILTERED DEPTH] = 6.600006
         [MAX FILTERED DEPTH] = 20.349976
         [MIN FILTERED COORD] = 452,140
         [MAX FILTERED COORD] = 417,130
         [MIN DEPTH] = 6.600006
         [MAX DEPTH] = 20.349976
         [MIN COORD] = 452,140
         [MAX COORD] = 417,130
         [BIN HEADER SIZE] = 16384
         [COUNT BITS] = 27
         [STD BITS] = 32
         [STD SCALE] = 1000000.000000
         [DEPTH BITS] = 18
         [DEPTH SCALE] = 100.000000
         [DEPTH OFFSET] = 500.000000
         [RECORD POINTER BITS] = 38
         [FILE NUMBER BITS] = 13
         [LINE NUMBER BITS] = 15
         [PING NUMBER BITS] = 31
         [BEAM NUMBER BITS] = 16
         [OFFSET BITS] = 12
         [VALIDITY BITS] = 29
         [ATTRIBUTE FLAG BITS] = 1
         [POINT] = 30.169444444,-88.759722222
         [POINT] = 30.162500000,-88.759722222
         [POINT] = 30.162500000,-88.744444444
         [POINT] = 30.169444444,-88.744444444
         [MINIMUM BIN COUNT] = 1
         [MAXIMUM BIN COUNT] = 607
         [MIN COUNT COORD] = 502,11
         [MAX COUNT COORD] = 583,124
         [MIN STANDARD DEVIATION] = 0.000000
         [MAX STANDARD DEVIATION] = 2.874570
         [CHART SCALE] = 0.000000
         [CLASS TYPE] = 0
         [PROJECTION] = 0
         [PROJECTION ZONE] = 0
         [HEMISPHERE] = 0
         [WELL-KNOWN TEXT] = COMPD_CS["WGS84 with ellipsoid Z",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9108"]],AXIS["Lat",NORTH],AXIS["Long",EAST],AUTHORITY["EPSG","4326"]],VERT_CS["ellipsoid Z in meters",VERT_DATUM["Ellipsoid",2002],UNIT["metre",1],AXIS["Z",UP]]]
         [PROJECTION PARAMETER 0] = 0.000
         [PROJECTION PARAMETER 1] = 0.000
         [PROJECTION PARAMETER 2] = 0.000000000000
         [PROJECTION PARAMETER 3] = 0.000000000000
         [PROJECTION PARAMETER 4] = 0.000000000000
         [PROJECTION PARAMETER 5] = 0.000000000000
         [PROJECTION PARAMETER 6] = 0.000000000000
         [PROJECTION PARAMETER 7] = 0.000000000000
         [PROJECTION PARAMETER 8] = 0.000000000000
         [PROJECTION PARAMETER 9] = 0.000000000000
         [PROJECTION PARAMETER 10] = 0.000000000000
         [PROJECTION PARAMETER 11] = 0.000000000000
         [PROJECTION PARAMETER 12] = 0.000000000000
         [PROJECTION PARAMETER 13] = 0.000000000000
         [PROJECTION PARAMETER 14] = 0.000000000000
         [PROJECTION PARAMETER 15] = 0.000000000000
         [AVERAGE FILTERED NAME] = Average Filtered Depth
         [AVERAGE NAME] = Average Depth
         [DYNAMIC RELOAD] = 1
         [NUMBER OF BIN ATTRIBUTES] = 6
         [BIN ATTRIBUTE 0] = ###0
         [BIN ATTRIBUTE 1] = ###1
         [BIN ATTRIBUTE 2] = ###2
         [BIN ATTRIBUTE 3] = ###3
         [BIN ATTRIBUTE 4] = ###4
         [BIN ATTRIBUTE 5] = ###5
         [BIN ATTRIBUTE 6] =
         [BIN ATTRIBUTE 7] =
         [BIN ATTRIBUTE 8] =
         [BIN ATTRIBUTE 9] =
         [BIN ATTRIBUTE 10] =
         [BIN ATTRIBUTE 11] =
         [BIN ATTRIBUTE 12] =
         [BIN ATTRIBUTE 13] =
         [BIN ATTRIBUTE 14] =
         [BIN ATTRIBUTE 15] =
         [BIN ATTRIBUTE 16] =
         [BIN ATTRIBUTE 17] =
         [BIN ATTRIBUTE 18] =
         [BIN ATTRIBUTE 19] =
         [BIN ATTRIBUTE OFFSET 0] = 0.000000
         [BIN ATTRIBUTE OFFSET 1] = 0.000000
         [BIN ATTRIBUTE OFFSET 2] = 0.000000
         [BIN ATTRIBUTE OFFSET 3] = 0.000000
         [BIN ATTRIBUTE OFFSET 4] = 0.000000
         [BIN ATTRIBUTE OFFSET 5] = 0.000000
         [BIN ATTRIBUTE OFFSET 6] = 0.000000
         [BIN ATTRIBUTE OFFSET 7] = 0.000000
         [BIN ATTRIBUTE OFFSET 8] = 0.000000
         [BIN ATTRIBUTE OFFSET 9] = 0.000000
         [BIN ATTRIBUTE OFFSET 10] = 0.000000
         [BIN ATTRIBUTE OFFSET 11] = 0.000000
         [BIN ATTRIBUTE OFFSET 12] = 0.000000
         [BIN ATTRIBUTE OFFSET 13] = 0.000000
         [BIN ATTRIBUTE OFFSET 14] = 0.000000
         [BIN ATTRIBUTE OFFSET 15] = 0.000000
         [BIN ATTRIBUTE OFFSET 16] = 0.000000
         [BIN ATTRIBUTE OFFSET 17] = 0.000000
         [BIN ATTRIBUTE OFFSET 18] = 0.000000
         [BIN ATTRIBUTE OFFSET 19] = 0.000000
         [BIN ATTRIBUTE MAX 0] = 100.000000
         [BIN ATTRIBUTE MAX 1] = 100.000000
         [BIN ATTRIBUTE MAX 2] = 100.000000
         [BIN ATTRIBUTE MAX 3] = 32000.000000
         [BIN ATTRIBUTE MAX 4] = 100.000000
         [BIN ATTRIBUTE MAX 5] = 100.000000
         [BIN ATTRIBUTE MAX 6] = 0.000000
         [BIN ATTRIBUTE MAX 7] = 0.000000
         [BIN ATTRIBUTE MAX 8] = 0.000000
         [BIN ATTRIBUTE MAX 9] = 0.000000
         [BIN ATTRIBUTE MAX 10] = 0.000000
         [BIN ATTRIBUTE MAX 11] = 0.000000
         [BIN ATTRIBUTE MAX 12] = 0.000000
         [BIN ATTRIBUTE MAX 13] = 0.000000
         [BIN ATTRIBUTE MAX 14] = 0.000000
         [BIN ATTRIBUTE MAX 15] = 0.000000
         [BIN ATTRIBUTE MAX 16] = 0.000000
         [BIN ATTRIBUTE MAX 17] = 0.000000
         [BIN ATTRIBUTE MAX 18] = 0.000000
         [BIN ATTRIBUTE MAX 19] = 0.000000
         [BIN ATTRIBUTE NULL 0] = 101.000000
         [BIN ATTRIBUTE NULL 1] = 101.000000
         [BIN ATTRIBUTE NULL 2] = 101.000000
         [BIN ATTRIBUTE NULL 3] = 32001.000000
         [BIN ATTRIBUTE NULL 4] = 101.000000
         [BIN ATTRIBUTE NULL 5] = 101.000000
         [BIN ATTRIBUTE NULL 6] = 0.000000
         [BIN ATTRIBUTE NULL 7] = 0.000000
         [BIN ATTRIBUTE NULL 8] = 0.000000
         [BIN ATTRIBUTE NULL 9] = 0.000000
         [BIN ATTRIBUTE NULL 10] = 0.000000
         [BIN ATTRIBUTE NULL 11] = 0.000000
         [BIN ATTRIBUTE NULL 12] = 0.000000
         [BIN ATTRIBUTE NULL 13] = 0.000000
         [BIN ATTRIBUTE NULL 14] = 0.000000
         [BIN ATTRIBUTE NULL 15] = 0.000000
         [BIN ATTRIBUTE NULL 16] = 0.000000
         [BIN ATTRIBUTE NULL 17] = 0.000000
         [BIN ATTRIBUTE NULL 18] = 0.000000
         [BIN ATTRIBUTE NULL 19] = 0.000000
         [MINIMUM BIN ATTRIBUTE 0] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 0] = 100.000000
         [MINIMUM BIN ATTRIBUTE 1] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 1] = 100.000000
         [MINIMUM BIN ATTRIBUTE 2] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 2] = 100.000000
         [MINIMUM BIN ATTRIBUTE 3] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 3] = 32000.000000
         [MINIMUM BIN ATTRIBUTE 4] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 4] = 100.000000
         [MINIMUM BIN ATTRIBUTE 5] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 5] = 100.000000
         [MINIMUM BIN ATTRIBUTE 6] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 6] = 0.000000
         [MINIMUM BIN ATTRIBUTE 7] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 7] = 0.000000
         [MINIMUM BIN ATTRIBUTE 8] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 8] = 0.000000
         [MINIMUM BIN ATTRIBUTE 9] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 9] = 0.000000
         [MINIMUM BIN ATTRIBUTE 10] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 10] = 0.000000
         [MINIMUM BIN ATTRIBUTE 11] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 11] = 0.000000
         [MINIMUM BIN ATTRIBUTE 12] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 12] = 0.000000
         [MINIMUM BIN ATTRIBUTE 13] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 13] = 0.000000
         [MINIMUM BIN ATTRIBUTE 14] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 14] = 0.000000
         [MINIMUM BIN ATTRIBUTE 15] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 15] = 0.000000
         [MINIMUM BIN ATTRIBUTE 16] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 16] = 0.000000
         [MINIMUM BIN ATTRIBUTE 17] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 17] = 0.000000
         [MINIMUM BIN ATTRIBUTE 18] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 18] = 0.000000
         [MINIMUM BIN ATTRIBUTE 19] = 0.000000
         [MAXIMUM BIN ATTRIBUTE 19] = 0.000000
         [BIN ATTRIBUTE BITS 0] = 7
         [BIN ATTRIBUTE BITS 1] = 14
         [BIN ATTRIBUTE BITS 2] = 14
         [BIN ATTRIBUTE BITS 3] = 15
         [BIN ATTRIBUTE BITS 4] = 14
         [BIN ATTRIBUTE BITS 5] = 14
         [BIN ATTRIBUTE BITS 6] = 0
         [BIN ATTRIBUTE BITS 7] = 0
         [BIN ATTRIBUTE BITS 8] = 0
         [BIN ATTRIBUTE BITS 9] = 0
         [BIN ATTRIBUTE BITS 10] = 0
         [BIN ATTRIBUTE BITS 11] = 0
         [BIN ATTRIBUTE BITS 12] = 0
         [BIN ATTRIBUTE BITS 13] = 0
         [BIN ATTRIBUTE BITS 14] = 0
         [BIN ATTRIBUTE BITS 15] = 0
         [BIN ATTRIBUTE BITS 16] = 0
         [BIN ATTRIBUTE BITS 17] = 0
         [BIN ATTRIBUTE BITS 18] = 0
         [BIN ATTRIBUTE BITS 19] = 0
         [BIN ATTRIBUTE SCALE 0] = 1.000000
         [BIN ATTRIBUTE SCALE 1] = 100.000000
         [BIN ATTRIBUTE SCALE 2] = 100.000000
         [BIN ATTRIBUTE SCALE 3] = 1.000000
         [BIN ATTRIBUTE SCALE 4] = 100.000000
         [BIN ATTRIBUTE SCALE 5] = 100.000000
         [BIN ATTRIBUTE SCALE 6] = 0.000000
         [BIN ATTRIBUTE SCALE 7] = 0.000000
         [BIN ATTRIBUTE SCALE 8] = 0.000000
         [BIN ATTRIBUTE SCALE 9] = 0.000000
         [BIN ATTRIBUTE SCALE 10] = 0.000000
         [BIN ATTRIBUTE SCALE 11] = 0.000000
         [BIN ATTRIBUTE SCALE 12] = 0.000000
         [BIN ATTRIBUTE SCALE 13] = 0.000000
         [BIN ATTRIBUTE SCALE 14] = 0.000000
         [BIN ATTRIBUTE SCALE 15] = 0.000000
         [BIN ATTRIBUTE SCALE 16] = 0.000000
         [BIN ATTRIBUTE SCALE 17] = 0.000000
         [BIN ATTRIBUTE SCALE 18] = 0.000000
         [BIN ATTRIBUTE SCALE 19] = 0.000000
         [NUMBER OF NDX ATTRIBUTES] = 5
         [NDX ATTRIBUTE 0] = Time (POSIX minutes)
         [NDX ATTRIBUTE 1] = GSF Heading
         [NDX ATTRIBUTE 2] = GSF Pitch
         [NDX ATTRIBUTE 3] = GSF Roll
         [NDX ATTRIBUTE 4] = GSF Heave
         [NDX ATTRIBUTE 5] =
         [NDX ATTRIBUTE 6] =
         [NDX ATTRIBUTE 7] =
         [NDX ATTRIBUTE 8] =
         [NDX ATTRIBUTE 9] =
         [NDX ATTRIBUTE 10] =
         [NDX ATTRIBUTE 11] =
         [NDX ATTRIBUTE 12] =
         [NDX ATTRIBUTE 13] =
         [NDX ATTRIBUTE 14] =
         [NDX ATTRIBUTE 15] =
         [NDX ATTRIBUTE 16] =
         [NDX ATTRIBUTE 17] =
         [NDX ATTRIBUTE 18] =
         [NDX ATTRIBUTE 19] =
         [MINIMUM NDX ATTRIBUTE 0] = -64000000.000000
         [MAXIMUM NDX ATTRIBUTE 0] = 64000000.000000
         [MINIMUM NDX ATTRIBUTE 1] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 1] = 360.000000
         [MINIMUM NDX ATTRIBUTE 2] = -30.000000
         [MAXIMUM NDX ATTRIBUTE 2] = 30.000000
         [MINIMUM NDX ATTRIBUTE 3] = -50.000000
         [MAXIMUM NDX ATTRIBUTE 3] = 50.000000
         [MINIMUM NDX ATTRIBUTE 4] = -25.000000
         [MAXIMUM NDX ATTRIBUTE 4] = 25.000000
         [MINIMUM NDX ATTRIBUTE 5] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 5] = 0.000000
         [MINIMUM NDX ATTRIBUTE 6] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 6] = 0.000000
         [MINIMUM NDX ATTRIBUTE 7] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 7] = 0.000000
         [MINIMUM NDX ATTRIBUTE 8] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 8] = 0.000000
         [MINIMUM NDX ATTRIBUTE 9] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 9] = 0.000000
         [MINIMUM NDX ATTRIBUTE 10] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 10] = 0.000000
         [MINIMUM NDX ATTRIBUTE 11] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 11] = 0.000000
         [MINIMUM NDX ATTRIBUTE 12] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 12] = 0.000000
         [MINIMUM NDX ATTRIBUTE 13] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 13] = 0.000000
         [MINIMUM NDX ATTRIBUTE 14] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 14] = 0.000000
         [MINIMUM NDX ATTRIBUTE 15] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 15] = 0.000000
         [MINIMUM NDX ATTRIBUTE 16] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 16] = 0.000000
         [MINIMUM NDX ATTRIBUTE 17] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 17] = 0.000000
         [MINIMUM NDX ATTRIBUTE 18] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 18] = 0.000000
         [MINIMUM NDX ATTRIBUTE 19] = 0.000000
         [MAXIMUM NDX ATTRIBUTE 19] = 0.000000
         [NDX ATTRIBUTE BITS 0] = 27
         [NDX ATTRIBUTE BITS 1] = 16
         [NDX ATTRIBUTE BITS 2] = 13
         [NDX ATTRIBUTE BITS 3] = 14
         [NDX ATTRIBUTE BITS 4] = 13
         [NDX ATTRIBUTE BITS 5] = 0
         [NDX ATTRIBUTE BITS 6] = 0
         [NDX ATTRIBUTE BITS 7] = 0
         [NDX ATTRIBUTE BITS 8] = 0
         [NDX ATTRIBUTE BITS 9] = 0
         [NDX ATTRIBUTE BITS 10] = 0
         [NDX ATTRIBUTE BITS 11] = 0
         [NDX ATTRIBUTE BITS 12] = 0
         [NDX ATTRIBUTE BITS 13] = 0
         [NDX ATTRIBUTE BITS 14] = 0
         [NDX ATTRIBUTE BITS 15] = 0
         [NDX ATTRIBUTE BITS 16] = 0
         [NDX ATTRIBUTE BITS 17] = 0
         [NDX ATTRIBUTE BITS 18] = 0
         [NDX ATTRIBUTE BITS 19] = 0
         [NDX ATTRIBUTE SCALE 0] = 1.000000
         [NDX ATTRIBUTE SCALE 1] = 100.000000
         [NDX ATTRIBUTE SCALE 2] = 100.000000
         [NDX ATTRIBUTE SCALE 3] = 100.000000
         [NDX ATTRIBUTE SCALE 4] = 100.000000
         [NDX ATTRIBUTE SCALE 5] = 0.000000
         [NDX ATTRIBUTE SCALE 6] = 0.000000
         [NDX ATTRIBUTE SCALE 7] = 0.000000
         [NDX ATTRIBUTE SCALE 8] = 0.000000
         [NDX ATTRIBUTE SCALE 9] = 0.000000
         [NDX ATTRIBUTE SCALE 10] = 0.000000
         [NDX ATTRIBUTE SCALE 11] = 0.000000
         [NDX ATTRIBUTE SCALE 12] = 0.000000
         [NDX ATTRIBUTE SCALE 13] = 0.000000
         [NDX ATTRIBUTE SCALE 14] = 0.000000
         [NDX ATTRIBUTE SCALE 15] = 0.000000
         [NDX ATTRIBUTE SCALE 16] = 0.000000
         [NDX ATTRIBUTE SCALE 17] = 0.000000
         [NDX ATTRIBUTE SCALE 18] = 0.000000
         [NDX ATTRIBUTE SCALE 19] = 0.000000
         [USER FLAG 1 NAME] = PFM_USER_01
         [USER FLAG 2 NAME] = PFM_USER_02
         [USER FLAG 3 NAME] = PFM_USER_03
         [USER FLAG 4 NAME] = PFM_USER_04
         [USER FLAG 5 NAME] = PFM_USER_05
         [USER FLAG 6 NAME] = PFM_USER_06
         [USER FLAG 7 NAME] = PFM_USER_07
         [USER FLAG 8 NAME] = PFM_USER_08
         [USER FLAG 9 NAME] = PFM_USER_09
         [USER FLAG 10 NAME] = PFM_USER_10
         [COVERAGE MAP ADDRESS] = 0
         [HORIZONTAL ERROR BITS] = 12
         [HORIZONTAL ERROR SCALE] = 100.000000
         [MAXIMUM HORIZONTAL ERROR] = 20.000000
         [VERTICAL ERROR BITS] = 14
         [VERTICAL ERROR SCALE] = 100.000000
         [MAXIMUM VERTICAL ERROR] = 100.000000
         [NULL DEPTH] = 1001.000000
         </pre>
         <br>
         <br>



         - Subsequent ([BIN_WIDTH] * [BIN_HEIGHT]) bin records (address [BIN HEADER SIZE] or 16384 if [BIN HEADER SIZE] is not present) :
             - [COUNT BITS] : count of number of soundings in bin
             - [STD BITS] : standard deviation (offset/scaled integer)
             - [DEPTH BITS] : filtered average depth (offset/scaled integer)
             - [DEPTH BITS] : filtered minimum depth (offset/scaled integer)
             - [DEPTH BITS] : filtered maximum depth (offset/scaled integer)
             - [DEPTH BITS] : average depth (offset/scaled integer)
             - [DEPTH BITS] : minimum depth (offset/scaled integer)
             - [DEPTH BITS] : maximum depth (offset/scaled integer)
             - [BIN ATTRIBUTE BITS 0] : some offset/scaled integer value associated with bin attribute 0.  If [NUMBER OF BIN ATTRIBUTES] is 0
               these fields will not be present.
                 - 
                 - 
                 - 
             - [BIN ATTRIBUTE BITS [NUMBER OF BIN ATTRIBUTES] - 1] : some offset/scaled integer value associated with the last bin attribute
             - [VALIDITY BITS] : status information (see pfm.h)
             - [RECORD POINTER BITS] : pointer to first 'physical' sounding buffer for this bin (head pointer)
             - [RECORD POINTER BITS] : pointer to last 'physical' sounding buffer for this bin (tail pointer)

         - Following all of the bin records is the coverage map containing ([BIN_WIDTH] * [BIN_HEIGHT]) one byte records.
             - Each byte contains bit flags that mirror certain fields of the validity flags in the bin record.
             - The bit flags are COV_DATA, COV_CHECKED, COV_VERIFIED, COV_SURVEYED, and COV_GAP.



     <br><br>\section ndx PFM Index File (.ndx)

     The index file contains all of the input data points.  This is the file that actually gets edited (either automatically or
     manually).  Each physical record (sometimes called a "bucket" in the database world) in the index file contains [RECORD LENGTH]
     logical records and a continuation pointer (also known as an "overflow" pointer). Each logical record contains the file number,
     line number, ping/record number, beam/subrecord number, Z value (depth), horizontal uncertainty, vertical uncertainty, x and y
     position (projected or unprojected), status, and (optionally) up to ten attribute values.  All values are stored as bit-packed,
     scaled, unsigned integers to save space.  In addition, some incidental compression is being done.  For example, the x and y
     positions are stored as offsets from the lower left corner of the bin in units of 1/4095th (actually 1 over 2 raised to the
     PFM_OFFSET_BITS power minus 1) of the bin size.  This allows us to store the position with ridiculous resolution in 24 bits.
     For comparison, most systems store positions in two 64 bit floating point values.  The depth records are stored in "chains"
     of physical records.  The address of the start of a depth chain is stored in the associated bin record's head pointer.  The
     address of the end of a depth chain is stored in the tail pointer of the associated bin record.  A single, physical index file
     record contains the following:

         - [RECORD LENGTH] logical sounding records containing:
             - [FILE NUMBER BITS] : input file number
             - [LINE NUMBER BITS] : line number
             - [PING NUMBER BITS] : ping/record number
             - [BEAM NUMBER BITS] : beam/subrecord number
             - [DEPTH BITS] : depth (offset/scaled integer)
             - [OFFSET BITS] : X offset from lower left corner of bin in units of 1/4095th of the [X BIN SIZE]
             - [OFFSET BITS] : Y offset from lower left corner of bin in units of 1/4095th of the [Y BNIN SIZE]
             - [NDX ATTRIBUTE BITS 0] : ndx attribute 0 value (offset/scaled integer).  If [NUMBER OF NDX ATTRIBUTES] is 0 these fields
               will not be present.
                 - 
                 - 
                 - 
             - [NDX ATTRIBUTE BITS [NUMBER OF NDX ATTRIBUTES] - 1] : ndx attribute [NUMBER OF NDX ATTRIBUTES] - 1 offset/scaled integer value
             - VALIDITY BITS : status information (see pfm.h)
             - [HORIZONTAL ERROR BITS] : horizontal error (offset/scaled integer)
             - [VERTICAL ERROR BITS] : vertical (offset/scaled integer)

         - [RECORD POINTER BITS] : address of the next physical record in the depth chain.  If this is zero this is the last physical record
           in the depth chain.



     <br><br>\section hyp PFM Hypotheses File (.hyp)

     This section is (hopefully) under construction by IVS and/or SAIC.



     <br><br>\section notes Programming Notes

     This library uses the NAVO standard data types (see pfm_nvtypes.h).  You must set one of the following options on the compile
     command line:
         - -DNVLinux
         - -DNVWIN3X

     As of version 6.0, use of the projection parameters is deprecated.  We now use the Well-Known Text coordinate and datum
     information that is defined in the Open GIS Coordinate Transformations specification.  We will, however, still support old
     files containing the projection parameters so they are left in the header and the header structure.  One important thing to
     note - no adjustments based on projection or lack thereof are made in the API.  For example, if you are using geographic
     coordinates (lat/lon) to define your PFM and the area crosses the dateline the application must take care of the adjustment
     of west longitudes (negatives) to a positive value.  So, if you area went from 179E to 179W the application must convert this
     to +179 to +181 and add 360 to all negative longitudes.

     Also, as of version 6.0, creation of "huge" files (actually folders with sub-files) will no longer be done.  They will still
     be supported for read/update/append.  There is a tool (pfm_huge_convert.c) to convert "huge" folders to "standard"
     _LARGEFILE64_SOURCE files.  The PFM version does not indicate whether a PFM contains "huge" folders or normal files since the
     older PFM folders can be converted.  Of course, for that rule, there is the ever present exception - any PFM that is major
     version 6 or higher will always contain normal files.

     As of version 7.0, support for pre-6.0 PFM files is deprecated.  This means that "huge" file support is dead, yay!


     <br><br>\section threads Thread Safety

     The PFM I/O library is now thread safe if you follow some simple rules.  First, it is only thread safe if you use unique PFM
     file handles per thread.  Also, the open_pfm_file, open_existing_pfm_file, and close_pfm_file functions are not thread safe due
     to the fact that they assign or clear the PFM handles.  In order to use the library in multiple threads you must open the PFM
     files prior to starting the threads and close them after the threads have completed.  If your threads are going to work on a
     single PFM file, just open it multiple times to get unique handles.  Some common sense must be brought to bear when trying to
     create a multithreaded program that works with PFM files.  If you are just reading the file and not writing to it there should
     be no major problems.  If you are reading and just updating status there should also be no major problems.  If you are appending
     or creating a new file you could run into all kinds of problems due to the way the PFM file is designed.  In order to create or
     append to a single file with multiple threads you would have to guarantee that no two threads will ever access the same bin at
     the same time.  The easiest way to do that is be to break the area up into unique pieces and have a single thread work on each
     piece.  You also have to make sure that only one thread will add files to the control file list and make sure that the same input
     file is not added to the list multiple times.  The easiest, and most effective, way to do this is to add ALL of the input files
     to the PFM prior to beginning your load operation.  The down-side of this is that you will have file names in your .ctl file that
     don't have any associated data in the .ndx file.  The up-side is that, if you are appending to the file, files that don't have
     associated data in the .ndx file will not be re-scanned in subsequent append operations (assuming your loader checks for such
     things, mine does ;-).  Finally, and most importantly, you must guarantee that no two threads will try to get the same end of
     file location to append or extend a record from the same PFM for a continuation pointer.  This is much trickier but it has been
     taken care of in the library by using MUTEXES.  The simplest way to make sure you don't have any problems is to ALWAYS call
     pfm_thread_init prior to doing any multithreading with PFM files.  This is really only applicable if you are creating or appending
     to a single PFM using multiple handles in multiple threads but there is virtually no overhead to this if you don't append a record.
     After you finish the multithreaded section of your code just call pfm_thread_destroy to get rid of the mutex(es).  Also, if you are
     appending to a single PFM you should set the PFM_OPEN_ARGS structure's checkpoint field to 1 the first time you open the PFM (this
     will checkpoint the file) and set the checkpoint field to 3 when you open it for the append threads (this will cause the API to
     ignore the checkpoint file you created in the first open).  One other small item, if you are going to change the maximum cache size
     you must do it prior to running your threads.  This is because the set_cache_size_max function is not thread safe (it sets a single
     static global value);

     There are three other functions that aren't thread safe.  The pfm_basename, pfm_dirname, and pfm_standard_attr_name functions are
     not thread safe because they set a static character string to be passed back to the caller.  You wouldn't usually be using these in
     a thread anyway so it shouldn't be a problem.  In the API they're only used in the main library in the open functions (which aren't
     thread safe either).

     The hyp code is thread safe for read (with the same caveats as above).  It has not been tested for thread safety on write.



     <br><br>\section search Searching the Doxygen Documentation

     The easiest way to find documentation for a particular C function is by looking for it in the
     <a href="globals.html"><b>Files>Globals>All</b></a> tab.  So, for instance, if you are looking for documentation on the
     <b><i>get_next_list_file_number</i></b> function you would go to <a href="globals.html"><b>Files>Globals>All</b></a> and click on
     the <b><i>g</i></b> tab then look for <b><i>get_next_list_file_number</i></b>.  When you find <b><i>get_next_list_file_number</i></b>,
     just click on the .h file associated with it.  If you would like to see the history of changes that have been made to the PFM API you
     can look at pfm_version.h.<br><br>

*/


#ifndef __PFM_INCLUDE__
#define __PFM_INCLUDE__


#ifdef __PFM_IO__
#define __PFM_EXTERN__
#else
#define __PFM_EXTERN__ extern
#endif


#ifdef  __cplusplus
extern "C" {
#endif


#include "pfm_nvtypes.h"


  /*!  Debug switch for the PFM library software.  */

#undef          PFM_DEBUG


  /*!  Maximum number of polygon points for the defined area.  */

#define         MAX_POLY                    2000


  /*!  Maximum number of PFM files that can be opened at one time.
       WARNING - If you change this number you MUST change the declarations for pfm_hnd in pfm_io.c as well as the declarations
       for file_handle in large_io.c!  */

#define         MAX_PFM_FILES               150


  /*!  PFM error status            */

  __PFM_EXTERN__ int32_t              pfm_error;


  /*  Layer types (for bin data).  */

#define         NUM_SOUNDINGS               0    /*!<  Number of soundings layer  */ 
#define         STANDARD_DEV                1    /*!<  Standard deviation layer  */
#define         AVERAGE_FILTERED_DEPTH      2    /*!<  Average filtered/edited depth layer  */
#define         MIN_FILTERED_DEPTH          3    /*!<  Minimum filtered/edited depth layer  */
#define         MAX_FILTERED_DEPTH          4    /*!<  Maximum filtered/edited depth layer  */
#define         AVERAGE_DEPTH               5    /*!<  Average unfiltered/unedited depth layer  */
#define         MIN_DEPTH                   6    /*!<  Minimum unfiltered/unedited depth layer  */
#define         MAX_DEPTH                   7    /*!<  Maximum unfiltered/unedited depth layer  */
#define         PFM_NUM_VALID               8    /*!<  Number of valid soundings layer  */

#define         PFM_PERM_LAYERS             9

#define         ATTRIBUTE0                  9    /*!<  Application defined attribute 0 layer  */
#define         ATTRIBUTE1                  10   /*!<  Application defined attribute 1 layer  */
#define         ATTRIBUTE2                  11   /*!<  Application defined attribute 2 layer  */
#define         ATTRIBUTE3                  12   /*!<  Application defined attribute 3 layer  */
#define         ATTRIBUTE4                  13   /*!<  Application defined attribute 4 layer  */
#define         ATTRIBUTE5                  14   /*!<  Application defined attribute 5 layer  */
#define         ATTRIBUTE6                  15   /*!<  Application defined attribute 6 layer  */
#define         ATTRIBUTE7                  16   /*!<  Application defined attribute 7 layer  */
#define         ATTRIBUTE8                  17   /*!<  Application defined attribute 8 layer  */
#define         ATTRIBUTE9                  18   /*!<  Application defined attribute 9 layer  */
#define         ATTRIBUTE10                 19   /*!<  Application defined attribute 10 layer  */
#define         ATTRIBUTE11                 20   /*!<  Application defined attribute 11 layer  */
#define         ATTRIBUTE12                 21   /*!<  Application defined attribute 12 layer  */
#define         ATTRIBUTE13                 22   /*!<  Application defined attribute 13 layer  */
#define         ATTRIBUTE14                 23   /*!<  Application defined attribute 14 layer  */
#define         ATTRIBUTE15                 24   /*!<  Application defined attribute 15 layer  */
#define         ATTRIBUTE16                 25   /*!<  Application defined attribute 16 layer  */
#define         ATTRIBUTE17                 26   /*!<  Application defined attribute 17 layer  */
#define         ATTRIBUTE18                 27   /*!<  Application defined attribute 18 layer  */
#define         ATTRIBUTE19                 28   /*!<  Application defined attribute 19 layer  */

#define         NUM_ATTR                    20   /*!<  Maximum number of optional attribute layers  */

#define         NUM_LAYERS                  PFM_PERM_LAYERS + NUM_ATTR   /*!<  Maximum number of layers  */

#define         PFM_USER_FLAGS              10   /*!<  Number of available PFM_USER_XX flags  */

#define         PFM_FILE_BITS               14   /*!<  Default number of bits used for file number  */
#define         PFM_MAX_FILES               16384/*!<  This should always be set to 2 raised to the PFM_FILE_BITS power.  If you change
                                                       PFM_FILE_BITS you must change this too.  */
#define         PFM_LINE_BITS               15   /*!<  Default number of bits used for line number  */
#define         PFM_PING_BITS               32   /*!<  Default number of bits used for ping/record count.  Be careful with this value.  It's
                                                       stored as uint32_t.  If you need more than 4,294,967,295 (2 ** 32 - 1) beams/subrecords
                                                       per file we'll need to go to int64_t and double_bit_(un)pack.  With a rep rate of
                                                       90,000 soundings per second this will store ~48 hours of beams/subrecords.  I can't see
                                                       this rep rate for a sonar and I can't see 48 hour flight lines for LIDAR.  Correct me if
                                                       I'm wrong.  JCD  */
#define         PFM_BEAM_BITS               16   /*!<  Default number of bits used for beam/subrecord.  65,535 beams/subrecords seems like 
                                                       it should be enough.  Again, I could be wrong.  JCD  */
#define         PFM_RECORD_LENGTH           6    /*!<  Default number of logical depth records in a physical record (i.e. before we have
                                                       to set the continuation pointer and start a new physical record).  */
#define         PFM_COUNT_BITS              24   /*!<  The default number of bits used to store the count of soundings (valid and invalid) in
                                                       a single bin (24 bits = 16,777,216 soundings).  */
#define         PFM_STD_BITS                32   /*!<  The default number of bits used to store the standard deviation for a bin.  */
#define         PFM_STD_SCALE               1000000.0  /*!<  The default scale value to multiply the standard deviation value by.  */
#define         PFM_RECORD_POINTER_BITS     38   /*!<  The default number of bits used to store the continuation pointer.  At present this
                                                       will handle a pointer up to 214,877,906,943 bytes.  */
#define         PFM_OFFSET_BITS             12   /*!<  The default number of bits used to store the X and Y offsets of a depth from the corner
                                                       of the bin.  This equates to units of 1/4095th of the bin size in meters.  */


  /*!  IMPORTANT NOTE:  As of version 6.0, use of the projection parameters is deprecated.  We now use 
       the Well-Known Text coordinate and datum information that is defined in the Open GIS Coordinate
       Transformations specification.  We will, however, still support old files containing the
       projection parameters so they are left in the header and the header structure.  It's not a
       large price to pay really ;-)  The following describes the old projection parameters:

  <pre>

  February 1996

  Appendix A:



           Projection Transformation Package Projection Parameters

  -----------------------------------------------------------------------------
                          |                     Array Element                 |
   Code & Projection Id   |----------------------------------------------------
                          |   1  |   2  |  3   |  4   |   5   |    6    |7 | 8|
  -----------------------------------------------------------------------------
   0 Geographic           |      |      |      |      |       |         |  |  |
   1 U T M                |Lon/Z |Lat/Z |      |      |       |         |  |  |
   2 State Plane          |      |      |      |      |       |         |  |  |
   3 Albers Equal Area    |SMajor|SMinor|STDPR1|STDPR2|CentMer|OriginLat|FE|FN|
   4 Lambert Conformal C  |SMajor|SMinor|STDPR1|STDPR2|CentMer|OriginLat|FE|FN|
   5 Mercator             |SMajor|SMinor|      |      |CentMer|TrueScale|FE|FN|
   6 Polar Stereographic  |SMajor|SMinor|      |      |LongPol|TrueScale|FE|FN|
   7 Polyconic            |SMajor|SMinor|      |      |CentMer|OriginLat|FE|FN|
   8 Equid. Conic A       |SMajor|SMinor|STDPAR|      |CentMer|OriginLat|FE|FN|
     Equid. Conic B       |SMajor|SMinor|STDPR1|STDPR2|CentMer|OriginLat|FE|FN|
   9 Transverse Mercator  |SMajor|SMinor|Factor|      |CentMer|OriginLat|FE|FN|
  10 Stereographic        |Sphere|      |      |      |CentLon|CenterLat|FE|FN|
  11 Lambert Azimuthal    |Sphere|      |      |      |CentLon|CenterLat|FE|FN|
  12 Azimuthal            |Sphere|      |      |      |CentLon|CenterLat|FE|FN|
  13 Gnomonic             |Sphere|      |      |      |CentLon|CenterLat|FE|FN|
  14 Orthographic         |Sphere|      |      |      |CentLon|CenterLat|FE|FN|
  15 Gen. Vert. Near Per  |Sphere|      |Height|      |CentLon|CenterLat|FE|FN|
  16 Sinusoidal           |Sphere|      |      |      |CentMer|         |FE|FN|
  17 Equirectangular      |Sphere|      |      |      |CentMer|TrueScale|FE|FN|
  18 Miller Cylindrical   |Sphere|      |      |      |CentMer|         |FE|FN|
  19 Van der Grinten      |Sphere|      |      |      |CentMer|OriginLat|FE|FN|
  20 Hotin Oblique Merc A |SMajor|SMinor|Factor|      |       |OriginLat|FE|FN|
     Hotin Oblique Merc B |SMajor|SMinor|Factor|AziAng|AzmthPt|OriginLat|FE|FN|
  21 Robinson             |Sphere|      |      |      |CentMer|         |FE|FN|
  22 Space Oblique Merc A |SMajor|SMinor|      |IncAng|AscLong|         |FE|FN|
     Space Oblique Merc B |SMajor|SMinor|Satnum|Path  |       |         |FE|FN|
  23 Alaska Conformal     |SMajor|SMinor|      |      |       |         |FE|FN|
  24 Interrupted Goode    |Sphere|      |      |      |       |         |  |  |
  25 Mollweide            |Sphere|      |      |      |CentMer|         |FE|FN|
  26 Interrupt Mollweide  |Sphere|      |      |      |       |         |  |  |
  27 Hammer               |Sphere|      |      |      |CentMer|         |FE|FN|
  28 Wagner IV            |Sphere|      |      |      |CentMer|         |FE|FN|
  29 Wagner VII           |Sphere|      |      |      |CentMer|         |FE|FN|
  30 Oblated Equal Area   |Sphere|      |Shapem|Shapen|CentLon|CenterLat|FE|FN|
  -----------------------------------------------------------------------------




         ----------------------------------------------------
                                 |      Array Element       |
          Code & Projection Id   |---------------------------
                                 |  9  | 10 |  11 | 12 | 13 |
         ----------------------------------------------------
          0 Geographic           |     |    |     |    |    |
          1 U T M                |     |    |     |    |    |
          2 State Plane          |     |    |     |    |    |
          3 Albers Equal Area    |     |    |     |    |    |
          4 Lambert Conformal C  |     |    |     |    |    |
          5 Mercator             |     |    |     |    |    |
          6 Polar Stereographic  |     |    |     |    |    |
          7 Polyconic            |     |    |     |    |    |
          8 Equid. Conic A       |zero |    |     |    |    |
            Equid. Conic B       |one  |    |     |    |    |
          9 Transverse Mercator  |     |    |     |    |    |
         10 Stereographic        |     |    |     |    |    |
         11 Lambert Azimuthal    |     |    |     |    |    |
         12 Azimuthal            |     |    |     |    |    |
         13 Gnomonic             |     |    |     |    |    |
         14 Orthographic         |     |    |     |    |    |
         15 Gen. Vert. Near Per  |     |    |     |    |    |
         16 Sinusoidal           |     |    |     |    |    |
         17 Equirectangular      |     |    |     |    |    |
         18 Miller Cylindrical   |     |    |     |    |    |
         19 Van der Grinten      |     |    |     |    |    |
         20 Hotin Oblique Merc A |Long1|Lat1|Long2|Lat2|zero|
            Hotin Oblique Merc B |     |    |     |    |one |
         21 Robinson             |     |    |     |    |    |
         22 Space Oblique Merc A |PSRev|LRat|PFlag|    |zero|
            Space Oblique Merc B |     |    |     |    |one |
         23 Alaska Conformal     |     |    |     |    |    |
         24 Interrupted Goode    |     |    |     |    |    |
         25 Mollweide            |     |    |     |    |    |
         26 Interrupt Mollweide  |     |    |     |    |    |
         27 Hammer               |     |    |     |    |    |
         28 Wagner IV            |     |    |     |    |    |
         29 Wagner VII           |     |    |     |    |    |
         30 Oblated Equal Area   |Angle|    |     |    |    |
         ----------------------------------------------------

    where

    Lon/Z     Longitude of any point in the UTM zone or zero.  If zero,
              a zone code must be specified.
    Lat/Z     Latitude of any point in the UTM zone or zero.  If zero, a
              zone code must be specified.
    SMajor    Semi-major axis of ellipsoid.  If zero, Clarke 1866 in meters
              is assumed.
    SMinor    Eccentricity squared of the ellipsoid if less than zero,
              if zero, a spherical form is assumed, or if greater than
              zero, the semi-minor axis of ellipsoid.
    Sphere    Radius of reference sphere.  If zero, 6370997 meters is used.
    STDPAR    Latitude of the standard parallel
    STDPR1    Latitude of the first standard parallel
    STDPR2    Latitude of the second standard parallel
    CentMer   Longitude of the central meridian
    OriginLat Latitude of the projection origin
    FE        False easting in the same units as the semi-major axis
    FN        False northing in the same units as the semi-major axis
    TrueScale Latitude of true scale
    LongPol   Longitude down below pole of map
    Factor    Scale factor at central meridian (Transverse Mercator) or
              center of projection (Hotine Oblique Mercator)
    CentLon   Longitude of center of projection
    CenterLat Latitude of center of projection
    Height    Height of perspective point
    Long1     Longitude of first point on center line (Hotine Oblique
              Mercator, format A)
    Long2     Longitude of second point on center line (Hotine Oblique
              Mercator, format A)
    Lat1      Latitude of first point on center line (Hotine Oblique
              Mercator, format A)
    Lat2      Latitude of second point on center line (Hotine Oblique
              Mercator, format A)
    AziAng    Azimuth angle east of north of center line (Hotine Oblique
              Mercator, format B)
    AzmthPt   Longitude of point on central meridian where azimuth occurs
              (Hotine Oblique Mercator, format B)
    IncAng    Inclination of orbit at ascending node, counter-clockwise
              from equator (SOM, format A)
    AscLong   Longitude of ascending orbit at equator (SOM, format A)
    PSRev     Period of satellite revolution in minutes (SOM, format A)
    LRat      Landsat ratio to compensate for confusion at northern end
              of orbit (SOM, format A -- use 0.5201613)
    PFlag     End of path flag for Landsat:  0 = start of path,
              1 = end of path (SOM, format A)
    Satnum    Landsat Satellite Number (SOM, format B)
    Path      Landsat Path Number (Use WRS-1 for Landsat 1, 2 and 3 and
              WRS-2 for Landsat 4, 5 and 6.)  (SOM, format B)
    Shapem    Oblated Equal Area oval shape parameter m
    Shapen    Oblated Equal Area oval shape parameter n
    Angle     Oblated Equal Area oval rotation angle


                                   NOTES

    Array elements 14 and 15 are set to zero
    All array elements with blank fields are set to zero
    All angles (latitudes, longitudes, azimuths, etc.) are entered in packed
        degrees/ minutes/ seconds (DDDMMMSSS.SS) format



    The following notes apply to the Space Oblique Mercator A projection.

        A portion of Landsat rows 1 and 2 may also be seen as parts of rows
    246 or 247.  To place these locations at rows 246 or 247, set the end of
    path flag (parameter 11) to 1--end of path.  This flag defaults to zero.

        When Landsat-1,2,3 orbits are being used, use the following values
    for the specified parameters:

        Parameter 4   099005031.2
        Parameter 5   128.87 degrees - (360/251 * path number) in packed
                      DMS format
        Parameter 9   103.2669323
        Parameter 10  0.5201613

        When Landsat-4,5 orbits are being used, use the following values
    for the specified parameters:

        Parameter 4   098012000.0
        Parameter 5   129.30 degrees - (360/233 * path number) in packed
                      DMS format
        Parameter 9   98.884119
        Parameter 10  0.5201613

  </pre>*/


  typedef struct
  {
    uint8_t         projection;       /*!<  0 - geographic, 1 - UTM, . . . etc., as defined below (from GCTP appendix A)  */
    uint16_t        zone;             /*!<  Zone number, used only for UTM or State Plane projection  */
    uint8_t         hemisphere;       /*!<  Hemisphere, 0 - south, 1 - north  */
    double          params[16];       /*!<  Projection parameters as defined above (from GCTP appendix A)  */
    char            wkt[1024];        /*!<  Well-Known Text  */
  } PROJECTION_DATA;


  /*!

      - Bin Header record structure:

      - Reserved attribute names:

          - There will occasionally arise situations where we will want to use attributes for
            items that are useful to more than one processing entity.  For instance, CUBE
            processing parameters.  We don't want to make these part of the fixed PFM structure
            because "Son of CUBE" may be coming along soon.  Even so, CUBE will be used for quite
            some time to process sonar data so we want everyone that processes a PFM structure
            to be able to recognize "CUBE number of hypotheses", "CUBE computed vertical uncertainty",
            and other values regardless of which attribute they are placed in.  To this end we
            are implementing reserved attribute names.  Instead of fixing the names in English
            and making it difficult to translate them to other languages we are defining numbers
            and doing the translation in pfm_standard_attr_name (in pfm_extras.c, right now there
            is only an English translation).  Nonetheless, all reserved attribute "names" and
            definitions MUST be listed here as comments so that others can adjust their applications
            to deal with them.  The definitions here will be in American English (since that's all
            I know ;-)  A reserved attribute name will consist of three (3) pound signs (#) followed
            by a number.  Any addition of reserved attribute names should be forwarded to all
            interested parties (as with data types).

                - ###0             =      CUBE number of hypotheses
                - ###1             =      CUBE computed vertical uncertainty
                - ###2             =      CUBE hypothesis strength
                - ###3             =      CUBE number of contributing soundings
                - ###4             =      Average of vertical TPE for best hypothesis soundings
                - ###5             =      Final Uncertainty (MAX of CUBE StdDev and Average TPE)

            Please note that the min and max values and the scale may change for these reserved
            attributes.  The meaning of the attribute should be universally agreed upon.  If you add 
            to these be sure to modify pfm_standard_attr_name in pfm_extras.c which is also 
            where you would add translations.

  *********************************************************************************************/

  typedef struct
  {
    char            version[128];               /*!<  Version (ASCII)  */
    char            date[30];                   /*!<  Date (ASCII)  */
    char            classification[40];         /*!<  Classification (ASCII)  */
    char            creation_software[128];     /*!<  Creation software (ASCII)  */
    NV_F64_XYMBR    mbr;                        /*!<  Minimum bounding rectangle  */
    double          bin_size_xy;                /*!<  Bin size in projected coordinates  */
    float           chart_scale;                /*!<  ISS60 area file chart scale  */
    double          x_bin_size_degrees;         /*!<  X bin size in degrees  */
    double          y_bin_size_degrees;         /*!<  Y bin size in degrees  */
    int32_t         bin_width;                  /*!<  Width of MBR (# x bins)  */
    int32_t         bin_height;                 /*!<  Height of MBR (# y bins)  */
    NV_F64_COORD2   polygon[MAX_POLY];          /*!<  Polygon points  */
    int32_t         polygon_count;              /*!<  Polygon count  */
    float           min_filtered_depth;         /*!<  Min filtered depth for file  */
    float           max_filtered_depth;         /*!<  Max filtered depth for file  */
    NV_I32_COORD2   min_filtered_coord;         /*!<  Bin index of min filtered depth  */
    NV_I32_COORD2   max_filtered_coord;         /*!<  Bin index of max filtered depth  */
    float           min_depth;                  /*!<  Min depth for file  */
    float           max_depth;                  /*!<  Max depth for file  */
    NV_I32_COORD2   min_coord;                  /*!<  Bin index of min depth  */
    NV_I32_COORD2   max_coord;                  /*!<  Bin index of max depth  */
    float           null_depth;                 /*!<  Null depth for filtered bins  */
    uint32_t        min_bin_count;              /*!<  Min number of values per bin  */
    uint32_t        max_bin_count;              /*!<  Max number of values per bin  */
    NV_I32_COORD2   min_count_coord;            /*!<  Bin index of min count  */
    NV_I32_COORD2   max_count_coord;            /*!<  Bin index of max count  */
    double          min_standard_dev;           /*!<  Minimum standard deviation  */
    double          max_standard_dev;           /*!<  Maximum standard deviation  */
    int32_t         class_type;                 /*!<  0, 1, 2, 3 ... 9 - used to flag whether recomputes will use
                                                      only data of a specific user flag type (PFM_USER_01 through PFM_USER_10) */
    char            average_filt_name[30];      /*!<  Name of the average filtered data surface.  This may be
                                                      the CUBE surface.  If you want the library to recompute
                                                      the average surface set this to Average Filtered Depth 
                                                      or Average Edited Depth otherwise it will not modify it.
                                                      This is set to Average Filtered Depth by default.  */
    char            average_name[30];           /*!<  Name of the average data surface.  This may be the CUBE
                                                      or other surface.  Whether the surface is recomputed by the
                                                      library depends on the average filtered name above.  This
                                                      is set to Average Depth by default.  */
    uint8_t         dynamic_reload;             /*!<  Flag that indicates that the data is to be dynamically
                                                      unloaded/reloaded to the input files during editing.
                                                      Set to NVFalse by default.  */
    PROJECTION_DATA proj_data;                  /*!<  Projection data as described above  */
    char            bin_attr_name[NUM_ATTR][30];/*!<  Names of attributes  */
    float           min_bin_attr[NUM_ATTR];     /*!<  Minimum value for each of NUM_ATTR optional attributes  */
    float           max_bin_attr[NUM_ATTR];     /*!<  Maximum value for each of NUM_ATTR optional attributes  */
    float           bin_attr_scale[NUM_ATTR];   /*!<  Attribute scales (if set to 1.0 then this is an integer attribute and must be handled
                                                      appropriately by the application).  */
    float           bin_attr_null[NUM_ATTR];    /*!<  Null value for attributes  */
    uint16_t        num_bin_attr;               /*!<  Actual number of bin attributes.  */
    char            ndx_attr_name[NUM_ATTR][30];/*!<  Names of attributes  */
    float           min_ndx_attr[NUM_ATTR];     /*!<  Minimum value for each of NUM_ATTR optional attributes  */
    float           max_ndx_attr[NUM_ATTR];     /*!<  Maximum value for each of NUM_ATTR optional attributes  */
    float           ndx_attr_scale[NUM_ATTR];   /*!<  Attribute scales  (if set to 1.0 then this is an integer attribute and must be handled
                                                      appropriately by the application).  */
    uint16_t        num_ndx_attr;               /*!<  Actual number of ndx attributes  */
    char            user_flag_name[PFM_USER_FLAGS][30]; /*!<  Names of user flags  */
    float           horizontal_error_scale;     /*!<  Scale for horizontal error values.  0.0 if no horizontal
                                                      error values stored.  */
    float           max_horizontal_error;       /*!<  Maximum horizontal error.  */
    float           vertical_error_scale;       /*!<  Scale for vertical error values.  0.0 if no vertical
                                                      error values stored  */
    float           max_vertical_error;         /*!<  Maximum vertical error  */
    uint16_t        record_length;              /*!<  Number of "logical" depth records per "physical" record in the NDX file.  If this is set to a
                                                      value other than 0 when you create a PFM it will override the default PFM_RECORD_LENGTH.  
                                                      For more information see PFM_RECORD_LENGTH above.  */
    uint32_t        max_bin_soundings;          /*!<  Maximum number of soundings per bin.  If this is set to a value other than 0 when you
                                                      create a PFM it will be used to calculate the number of bits in which to store the number
                                                      of soundings per bin.  That computed value will override the default PFM_COUNT_BITS.
                                                      For more information see PFM_COUNT_BITS above.  */
    uint64_t        max_record_pointer;         /*!<  Maximum record offset in the BIN or NDX files.  This is essentailly the maximum file 
                                                      size.  If this is set when you create a PFM the number of bits used to store the address
                                                      of a record in the file will be computed from this value.  That computed number of bits
                                                      will override the default PFM_RECORD_POINTER_BITS.  For more information see
                                                      PFM_RECORD_POINTER_BITS above.  */
    uint32_t        max_input_files;            /*!<  Maximum number of input files.  If this is set to a value other than 0 when you create a PFM
                                                      it will be used to compute the number of bits used to store the input file number in a
                                                      depth record.  That computed number will override the default PFM_FILE_BITS.  For more
                                                      information see PFM_FILE_BITS above.  */
    uint32_t        max_input_lines;            /*!<  Maximum number of input lines.  If this is set to a value other than 0 when you create a PFM
                                                      it will be used to compute the number of bits used to store the input line number in a
                                                      depth record.  That computed number will override the default PFM_LINE_BITS.  For more
                                                      information see PFM_LINE_BITS above.  */
    uint32_t        max_input_pings;            /*!<  Maximum number of records per file.  If this is set to a value other than 0 when you
                                                      create a PFM it will be used to compute the number of bits used to store the input record
                                                      (ping) number in a depth record.  That computed number will override the default PFM_PING_BITS.
                                                      For more information see PFM_PING_BITS above.  */
    uint32_t        max_input_beams;            /*!<  Maximum number of subrecords per record.  If this is set to a value other than 0 when you
                                                      create a PFM it will be used to compute the number of bits used to store the input subrecord
                                                      (beam) number in a depth record.  That computed number will override the default PFM_BEAM_BITS.
                                                      For more information see PFM_BEAM_BITS above.  */
    uint32_t        offset_bits;                /*!<  Number of bits used to store the X and Y offset from the corner of the bin.  If this is 
                                                      set to a value other than 0 when you create a PFM it will override the default
                                                      PFM_OFFSET_BITS.  For more information see PFM_OFFSET_BITS above.  */
    uint8_t         pad[1024];                  /*!<  Pad area for future expansion.  This allows us to add to the
                                                      header record without creating shared memory incompatibilities.  */
  } BIN_HEADER;


  /*!
    Chain pointer structure.  Points to previous and subsequent physical record in the index file
    depth record chain.
  */

  typedef struct
  {
    int64_t         head;                       /*!<  Head pointer for depth chain  */
    int64_t         tail;                       /*!<  Tail pointer for depth chain  */
  } CHAIN;


  /*!  Bin record structure  */

  typedef struct
  {
    uint32_t        num_soundings;              /*!<  Number of soundings in this bin  */
    float           standard_dev;               /*!<  Standard deviation  */
    float           avg_filtered_depth;         /*!<  Avg depth for bin (filtered)  */
    float           min_filtered_depth;         /*!<  Min depth for bin (filtered)  */
    float           max_filtered_depth;         /*!<  Max depth for bin (filtered)  */
    float           avg_depth;                  /*!<  Avg depth for bin  */
    float           min_depth;                  /*!<  Min depth for bin  */
    float           max_depth;                  /*!<  Max depth for bin  */
    uint32_t        num_valid;                  /*!<  Number of VALID soundings in this bin (NOT including PFM_REFERENCE)  */
    float           attr[NUM_ATTR];             /*!<  Some value associated with NUM_ATTR optional attributes defined by loader  */
    uint32_t        validity;                   /*!<  Validity bits  */
    NV_I32_COORD2   coord;                      /*!<  X and Y indices for bin  */
    NV_F64_COORD2   xy;                         /*!<  Position of center of bin  */


    /*!
        - WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   WARNING
          <br>
          <br>
        - The depth_chain pointer values MUST NOT be modified by the calling
          application.  They are used to save a lot of redundant I/O calls.
          Any call to add_depth_record will disable use of these pointers on
          write_bin_record calls.  Closing and reopening the PFM file will
          re-enable their use.  This is useful in a loader after adding all
          of the data prior to recomputing bin values.
          <br>
          <br>
        - WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   WARNING
    */

    CHAIN           depth_chain;
    void*           temp;                       /*!<  Working pointer, only valid in small scope.
                                                      Expect routine using it to allocate and free memory.  */
    uint8_t         local_flags;                /*!<  Local flags to be used by the calling routine as needed.  */
  } BIN_RECORD;


  /*!  Block (physical record) address structure.  */

  typedef struct
  {
    int64_t         block;                      /*!<  Block address of the depth record  */
    uint8_t         record;                     /*!<  Position within the block of the depth record  */
  } BLOCK_ADDRESS;


  /*!
    The depth record structure.  This is a single depth from anywhere within a depth chain or
    physical record.
  */

  typedef struct
  {
    uint16_t        file_number;                /*!<  File number in file list (.ctl) file  */
    uint32_t        line_number;                /*!<  Line number in line file  */
    uint32_t        ping_number;                /*!<  Ping number in input file  */
    uint16_t        beam_number;                /*!<  Beam (subrecord) number in ping (record)  */
    uint32_t        validity;                   /*!<  Validity bits  */
    NV_I32_COORD2   coord;                      /*!<  X and Y indices for bin  */
    NV_F64_COORD3   xyz;                        /*!<  Position and depth  */
    float           horizontal_error;           /*!<  Horizontal error in meters.  Returns -999.0 for max error  */
    float           vertical_error;             /*!<  Vertical error in meters.  Returns -999.0 for max error  */
    float           attr[NUM_ATTR];             /*!<  NUM_ATTR attributes, defined by loader  */
    BLOCK_ADDRESS   address;                    /*!<  Physical record (block) address information for the depth record.  */
    uint8_t         local_flags;                /*!<  Local flags to be used by the calling routine as needed  */
  } DEPTH_RECORD;


  /*!  PFM file open arguments.  */

  typedef struct
  {
    char            list_path[512];             /*!<  This is the PFM list file for pre 4.6 structures.  In post
                                                      4.6 structures this should be the PFM handle file name.
                                                      The actual PFM  structure names will be derived from the
                                                      handle file name.  */
    char            ctl_path[512];              /*!<  File list (control) file path  */
    char            bin_path[512];              /*!<  Bin file path  */
    char            index_path[512];            /*!<  Index file path  */
    char            image_path[512];            /*!<  GeoTIFF mosaic file path  */
    char            target_path[512];           /*!<  Feature (target) file path  */
    BIN_HEADER      head;                       /*!<  Bin file header structure  */
    float           max_depth;                  /*!<  Maximum depth to be stored in the file  */
    float           offset;                     /*!<  Offset value so all depths will be positive (inverted minimum depth value)  */
    float           scale;                      /*!<  Chart scale (not used anymore... I think ;-)  */
    int8_t          checkpoint;                 /*!<
                                                      - 0 = check for a checkpoint file
                                                      - 1 = create a checkpoint file prior to opening the PFM file
                                                      - 2 = recover from a checkpoint if one exists
                                                      - 3 = don't recover from a checkpoint if one exists.  This is 
                                                            usually due to opening a single PFM for append multiple
                                                            times.  The first open is done with checkpoint set to 1
                                                            and subsequent opens set checkpoint to 3 so the
                                                            checkpoint file will be ignored.
                                                */
    float           bin_attr_offset[NUM_ATTR];  /*!<  Attribute offsets  */
    float           bin_attr_max[NUM_ATTR];     /*!<  Attribute max values  */
  } PFM_OPEN_ARGS;


  /*!  PFM's shared memory structure (don't change this structure).  */

  typedef struct
  {
    double        cur_x[50];                    /*!<  Polygonal area to be edited (X)  */
    double        cur_y[50];                    /*!<  Polygonal area to be edited (Y)  */
    int32_t       count;                        /*!<  Count of polygon points, 0 for rectangle  */
    NV_F64_XYMBR  edit_area;                    /*!<  Total rectangular area to be edited  */
    int32_t       ppid;                         /*!<  Parent process ID (bin viewer)  */
    float         cint;                         /*!<  Current contour interval (0 for user defined)  */
    float         contour_levels[200];          /*!<  User defined contour levels  */
    int32_t       num_levels;                   /*!<  Number of user defined contour levels  */
    int32_t       layer_type;                   /*!<  Type of bin data/contour to display  */
    NV_F64_XYMBR  displayed_area;               /*!<  Displayed area in the editor  */
    PFM_OPEN_ARGS open_args;                    /*!<  Opening arguments for open_pfm_file or open_existing_pfm_file  */
    char          nearest_file[512];            /*!<  Path of file containing nearest point to the cursor in 
                                                      some program (usually some kind of point cloud editor)  */
    char          nearest_line[512];            /*!<  Line name for nearest point to the cursor in some program  */
    int32_t       nearest_file_num;             /*!<  File number of nearest point to the cursor in some program  */
    int32_t       nearest_line_num;             /*!<  Line number of nearest point to the cursor in some program  */
    int16_t       nearest_type;                 /*!<  Data type of nearest point to the cursor in some program  */
    int32_t       nearest_record;               /*!<  Record number (in file) of nearest point to the cursor in some program  */
    int32_t       nearest_subrecord;            /*!<  Subrecord number (in file) of nearest point to the cursor in some program  */
    int32_t       nearest_point;                /*!<  Point number (in point cloud) of nearest point to the cursor in some program  */
    double        nearest_value;                /*!<  Z value of nearest point to the cursor in some program  */
    uint32_t      nearest_validity;             /*!<  Validity of nearest point to the cursor in some program  */
    uint32_t      key;                          /*!<  Keycode of last key pressed in some program.  Set to INT32_MAX
                                                      to indicate lock on shared memory.  */
    char          modified_file[512];           /*!<  Modified file path of file containing nearest point to the cursor in some
                                                      program (possibly modified by an external program)  */
    char          modified_line[512];           /*!<  Modified line name  */
    int32_t       modified_file_num;            /*!<  Modified file number  */
    int32_t       modified_line_num;            /*!<  Modified line number  */
    int16_t       modified_type;                /*!<  Modified data type  */
    int32_t       modified_record;              /*!<  Modified record number  */
    int32_t       modified_subrecord;           /*!<  Modified subrecord number  */
    int32_t       modified_point;               /*!<  Modified point number  */
    double        modified_value;               /*!<  Modified Z value  */
    uint32_t      modified_validity;            /*!<  Modified validity  */
    uint32_t      modcode;                      /*!<  Set to something other than zero to indicate a change in the shared data  */
    uint8_t       pad[1024];                    /*!<  Pad area for future expansion.  This allows us to add to the shared memory area
                                                      without creating shared memory incompatibilities.  */
  } EDIT_SHARE;


  /*!  Depth list structure for caching (linked list).  */

  typedef struct DEPTH_LIST_STRUCT
  {
    uint8_t                     *depths;
    struct DEPTH_LIST_STRUCT    *next;
  } DEPTH_LIST;


  /*!  Summary record for the DEPTH cache.  */

  typedef struct
  {
    uint64_t      continuation_pointer;
    uint32_t      record_pos;

    DEPTH_LIST    buffers;
    DEPTH_LIST    *last_depth_buffer;

    int64_t       previous_chain;

    /*!
        - WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   WARNING
          <br>
          <br>
        - The depth_chain pointer values MUST NOT be modified by the calling
          application.  They are used to save a lot of redundant I/O calls.
          Any call to add_depth_record will disable use of these pointers on
          write_bin_record calls.  Closing and reopening the PFM file will
          re-enable their use.  This is useful in a loader after adding all
          of the data prior to recomputing bin values.
          <br>
          <br>
        - WARNING   WARNING   WARNING   WARNING   WARNING   WARNING   WARNING
    */

    CHAIN         chain;                /*!<  Head and tail pointers for the depth chain.  */
  } DEPTH_SUMMARY;


  /*!  Summary record for the BIN cache. */

  typedef struct
  {
    uint8_t         dirty;                      /*!<  Whether the bin is in use or not.  */
    uint32_t        num_soundings;              /*!<  Number of soundings in this bin  */
    NV_I32_COORD2   coord;                      /*!<  X and Y indices for bin  */
    uint32_t        validity;                   /*!<  Validity bits  */
    DEPTH_SUMMARY   depth;                      /*!<  Depth record buffer for bin  */
    uint32_t        cov_flag;                   /*!<  Flag required SAIC.  */
  } BIN_RECORD_SUMMARY;


#define             SUCCESS                                         0
#define             CREATE_LIST_FILE_FILE_EXISTS                    -1
#define             CREATE_LIST_FILE_OPEN_ERROR                     -2
#define             OPEN_LIST_FILE_OPEN_ERROR                       -3
#define             OPEN_LIST_FILE_READ_VERSION_ERROR               -4
#define             OPEN_LIST_FILE_READ_BIN_ERROR                   -5
#define             OPEN_LIST_FILE_READ_INDEX_ERROR                 -6
#define             OPEN_LIST_FILE_READ_IMAGE_ERROR                 -7
#define             OPEN_LIST_FILE_READ_TARGET_ERROR                -8
#define             WRITE_BIN_HEADER_EXCEEDED_MAX_POLY              -9
#define             OPEN_BIN_OPEN_ERROR                             -10
#define             WRITE_DEPTH_BUFFER_WRITE_ERROR                  -11
#define             OPEN_INDEX_OPEN_ERROR                           -12
#define             READ_LIST_FILE_READ_NAME_ERROR                  -13
#define             READ_BIN_RECORD_DATA_READ_ERROR                 -14
#define             READ_DEPTH_RECORD_READ_ERROR                    -15
#define             READ_DEPTH_RECORD_CONTINUATION_ERROR            -16
#define             UPDATE_DEPTH_RECORD_READ_BIN_RECORD_ERROR       -17
#define             UPDATE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR     -18
#define             ADD_DEPTH_RECORD_READ_BIN_RECORD_ERROR          -19
#define             RECOMPUTE_BIN_VALUES_READ_BIN_RECORD_ERROR      -20
#define             RECOMPUTE_BIN_VALUES_READ_DEPTH_RECORD_ERROR    -21
#define             RECOMPUTE_BIN_VALUES_NO_SOUNDING_DATA_ERROR     -22
#define             RECOMPUTE_BIN_VALUES_WRITE_BIN_RECORD_ERROR     -23
#define             WRITE_BIN_BUFFER_WRITE_ERROR                    -24
#define             SET_OFFSETS_BIN_MALLOC_ERROR                    -25
#define             SET_OFFSETS_DEPTH_MALLOC_ERROR                  -26
#define             WRITE_BIN_RECORD_DATA_READ_ERROR                -27
#define             READ_DEPTH_RECORD_NO_DATA                       -28
#define             WRITE_BIN_RECORD_VALIDITY_INDEX_ERROR           -29
#define             CHECK_INPUT_FILE_OPEN_ERROR                     -30
#define             CHECK_INPUT_FILE_WRITE_ERROR                    -31
#define             CHANGE_DEPTH_RECORD_READ_BIN_RECORD_ERROR       -32
#define             CHANGE_DEPTH_RECORD_READ_DEPTH_RECORD_ERROR     -33
#define             CHANGE_DEPTH_RECORD_OUT_OF_RANGE_ERROR          -34
#define             READ_DEPTH_ARRAY_CALLOC_ERROR                   -35
#define             CREATE_LINE_FILE_FILE_EXISTS                    -36
#define             CREATE_LINE_FILE_OPEN_ERROR                     -37
#define             OPEN_LINE_FILE_OPEN_ERROR                       -38
#define             OPEN_LIST_FILE_CORRUPTED_FILE_ERROR             -39
#define             OPEN_BIN_CORRUPT_HEADER_ERROR                   -40
#define             ADD_DEPTH_RECORD_TOO_MANY_SOUNDINGS_ERROR       -41
#define             FILE_PING_BEAM_MISMATCH_ERROR                   -42
#define             CHANGE_ATTRIBUTE_RECORD_READ_BIN_RECORD_ERROR   -43
#define             CHANGE_ATTRIBUTE_RECORD_READ_DEPTH_RECORD_ERROR -44
#define             CHANGE_ATTRIBUTE_RECORD_OUT_OF_RANGE_ERROR      -45
#define             OPEN_EXISTING_PFM_ENOENT_ERROR                  -46
#define             OPEN_EXISTING_PFM_LIST_FILE_OPEN_ERROR          -47
#define             OPEN_HANDLE_FILE_CREATE_ERROR                   -48
#define             CREATE_PFM_DATA_DIRECTORY_ERROR                 -49
#define             CHECKPOINT_FILE_EXISTS_ERROR                    -50
#define             CHECKPOINT_FILE_UNRECOVERABLE_ERROR             -51
#define             FILE_NUMBER_TOO_LARGE_ERROR                     -52
#define             LINE_NUMBER_TOO_LARGE_ERROR                     -53
#define             PING_NUMBER_TOO_LARGE_ERROR                     -54
#define             BEAM_NUMBER_TOO_LARGE_ERROR                     -55
#define             ADD_DEPTH_RECORD_OUT_OF_LIMITS_ERROR            -56
#define             WRITE_BIN_HEADER_NEGATIVE_BIN_DIMENSION         -57
#define             PFM_GEO_DISTANCE_NOT_GEOGRAPHIC_ERROR           -58
#define             PFM_GEO_DISTANCE_LATLON_PFM_ERROR               -59
#define             PFM_GEO_DISTANCE_ALLOCATE_ERROR                 -60
#define             PFM_GEO_DISTANCE_OUT_OF_BOUNDS                  -61
#define             OPEN_LIST_FILE_NEWER_VERSION_ERROR              -62
#define             OPEN_HANDLE_FILE_OPEN_ERROR                     -63
#define             PFM_UNSUPPORTED_VERSION_ERROR                   -64
#define             MUTEX_UNAVAILABLE_ERROR                         -65
#define             MUTEX_RELEASE_ERROR                             -66
#define             MUTEX_NOT_NEEDED_ERROR                          -67
#define             OPEN_COV_FILE_OPEN_ERROR                        -68
#define             OPEN_COV_FILE_READ_VERSION_ERROR                -69
#define             OPEN_COV_FILE_NEWER_VERSION_ERROR               -70
#define             CREATE_COV_FILE_FILE_EXISTS                     -71
#define             CREATE_COV_FILE_OPEN_ERROR                      -72
#define             READ_COV_MAP_INDEX_READ_ERROR                   -73
#define             GCC_IGNORE_RETURN_VALUE_WARNING_ERROR           -74   /*  gcc spits out warnings if you don't check the return value of
                                                                              certain functions.  You should never see this error!  */


  /*!  <b> Validity bits.  These have different meanings for depth records and bin records </b><br><br>  */

#define       PFM_MANUALLY_INVAL       1       /*!<  0000 0000 0000 0000 0000 0001
                                                     - depth record has been marked as manually invalid
                                                     - bin record contains at least one manually invalidated data depth record  */
#define       PFM_FILTER_INVAL         2       /*!<  0000 0000 0000 0000 0000 0010
                                                     - depth record has been automatically marked as invalid (filtered) 
                                                     - bin record contains at least one filter invalidated depth record  */
#define       PFM_SELECTED_SOUNDING    4       /*!<  0000 0000 0000 0000 0000 0100
                                                     - depth record is a selected sounding (min, max, avg, depends on application) 
                                                     - bin record contains at least one selected sounding  */
#define       PFM_SUSPECT              8       /*!<  0000 0000 0000 0000 0000 1000
                                                     - depth record has been marked as suspect 
                                                     - bin record contains at least one suspect depth record  */
#define       PFM_SELECTED_FEATURE     16      /*!<  0000 0000 0000 0000 0001 0000
                                                     - depth record is on or near a selected feature 
                                                     - bin record contains at least one depth record on or near a selected feature  */
#define       PFM_MODIFIED             32      /*!<  0000 0000 0000 0000 0010 0000 
                                                     - depth record has been modified in some way since being loaded into the PFM 
                                                     - bin reecord contains at least one modified depth record  */
#define       PFM_USER_01              64      /*!<  0000 0000 0000 0000 0100 0000 
                                                     - depth record has been marked with the PFM_USER_01 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_01 marked depth record  */
#define       PFM_USER_02              128     /*!<  0000 0000 0000 0000 1000 0000 
                                                     - depth record has been marked with the PFM_USER_02 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_02 marked depth record  */
#define       PFM_USER_03              256     /*!<  0000 0000 0000 0001 0000 0000 
                                                     - depth record has been marked with the PFM_USER_03 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_03 marked depth record  */
#define       PFM_CHECKED              512     /*!<  0000 0000 0000 0010 0000 0000
                                                     - N/A for depth record 
                                                     - bin record has been marked as "checked"  */
#define       PFM_DATA                 1024    /*!<  0000 0000 0000 0100 0000 0000
                                                     - N/A for depth record 
                                                     - bin record contains at least one valid depth record  */
#define       PFM_DELETED              2048    /*!<  0000 0000 0000 1000 0000 0000
                                                     - depth record has been marked as deleted/non-existent by API.  This can be cleared if the
                                                       data was incorrectly marked.  <b>Records marked as PFM_DELETED should be ignored by applications.</b>
                                                     - bin record contains at least one depth record marked as deleted/non-existent  */
#define       PFM_USER_04              4096    /*!<  0000 0000 0001 0000 0000 0000
                                                     - depth record has been marked with the PFM_USER_04 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_04 marked depth record  */
#define       PFM_INTERPOLATED         8192    /*!<  0000 0000 0010 0000 0000 0000
                                                     - N/A for depth record 
                                                     - bin record has no depth records but has an interpolated value in the average surface bins  */
#define       PFM_REFERENCE            16384   /*!<  0000 0000 0100 0000 0000 0000
                                                     - depth record has been marked to be used for reference only, not used for computing surfaces.
                                                       This is useful for things like lead line data or sidescan depth data. 
                                                     - bin record contains at least one depth record marked as reference data  */
#define       PFM_VERIFIED             32768   /*!<  0000 0000 1000 0000 0000 0000
                                                     - N/A for depth record 
                                                     - bin record has been marked as "verified"  */
#define       PFM_USER_05              65536   /*!<  0000 0001 0000 0000 0000 0000
                                                     - depth record has been marked with the PFM_USER_05 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_05 marked depth record  */
#define       PFM_DESIGNATED_SOUNDING  131072  /*!<  0000 0010 0000 0000 0000 0000
                                                     - depth record overrides all hypotheses.  Not all 'designated' soundings will be 'features'. 
                                                     - bin record contains at least one depth record marked as a designated sounding  */
#define       PFM_USER_06              262144  /*!<  0000 0100 0000 0000 0000 0000
                                                     - depth record has been marked with the PFM_USER_06 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_06 marked depth record  */
#define       PFM_USER_07              524288  /*!<  0000 1000 0000 0000 0000 0000
                                                     - depth record has been marked with the PFM_USER_07 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_07 marked depth record  */
#define       PFM_USER_08              1048576 /*!<  0001 0000 0000 0000 0000 0000
                                                     - depth record has been marked with the PFM_USER_08 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_08 marked depth record  */
#define       PFM_USER_09              2097152 /*!<  0010 0000 0000 0000 0000 0000
                                                     - depth record has been marked with the PFM_USER_09 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_09 marked depth record  */
#define       PFM_USER_10              4194304 /*!<  0100 0000 0000 0000 0000 0000
                                                     - depth record has been marked with the PFM_USER_10 flag (application defined) 
                                                     - bin record contains at least one PFM_USER_10 marked depth record  */


#define       PFM_INVAL                3       /*!< 0000 0000 0000 0000 0000 0011  =  combination of the "invalid" flags  */
#define       PFM_SELECTED             20      /*!< 0000 0000 0000 0000 0001 0100  =  combination of the "selected" flags  */
#define       PFM_USER                 7672256 /*!< 0111 0101 0001 0001 1100 0000  =  combination of the "user" flags  */

#define       PFM_VAL_MASK             8388607 /*!< 0111 1111 1111 1111 1111 1111  =  complete 19 bit validity mask (for negating flags)  */
#define       PFM_VALIDITY_BITS        23      /*!< Number of bits used to store the validity information for bin or depth.  */


  /*!  Header size for coverage file.  For now (and probably the foreseeable future) the header is just the version string and the [END OF HEADER] sentinel.  */

#define       COV_HEADER_SIZE          65536



  /*!  Coverage map bits.  <br><br>  */

#define       COV_DATA                 128     /*!< 1000 0000 - indicates a bin that contains at least one valid depth record  */
#define       COV_CHECKED              64      /*!< 0100 0000 - indicates a bin that has been marked as "checked"  */
#define       COV_VERIFIED             32      /*!< 0010 0000 - indicates a bin that has been marked as "verified"  */
#define       COV_SURVEYED             16      /*!< 0001 0000 - indicates a bin that should have data (i.e. it's been surveyed)  */
#define       COV_GAP                  8       /*!< 0000 1000 - indicates a bin that should but does NOT have data  */


  /*!  Data types  <br><br>  */

#define PFM_UNDEFINED_DATA      0     /*!<  Undefined */
#define PFM_MRG_DATA            1001  /*!<  Retired */
#define PFM_CHRTR_DATA          1     /*!<  CHRTR(2) format (.fin, .ch2), records begin with 0, record is row (south
                                            to north), subrecord is column (west to east)  */
#define PFM_GSF_DATA            2     /*!<  Records begin with 1, subrecords (beams) begin with 1 */
#define PFM_SHOALS_OUT_DATA     3     /*!<  Records begin with 0, subrecords undefined but may be used for anything else */
#define PFM_SHOALS_XY2_DATA     1002  /*!<  Retired */
#define PFM_CHARTS_HOF_DATA     4     /*!<  Records begin with 1, subrecord 0 is primary (not to be confused with first) return,
                                            subrecord 1 is secondary (not to be confused with second) return.  This is a replacement for
                                            PFM_SHOALS_1K_DATA that alleviates the need for "swapping" primary and secondary returns.  */
#define PFM_NAVO_ASCII_DATA     5     /*!<  Records begin with 0, no subrecords */
#define PFM_HTF_DATA            6
#define PFM_DEMO_DATA           1003  /*!<  Retired */
#define PFM_WLF_DATA            7     /*!<  Waveform LIDAR Format, records begin at 0, no subrecords */
#define PFM_DTM_DATA            8
#define PFM_HDCS_DATA           9
#define PFM_ASCXYZ_DATA         10
#define PFM_CNCBIN_DATA         11
#define PFM_STBBIN_DATA         12
#define PFM_XYZBIN_DATA         13
#define PFM_OMG_DATA            14
#define PFM_CNCTRACE_DATA       15
#define PFM_NEPTUNE_DATA        16
#define PFM_SHOALS_1K_DATA      17    /*!<  Records begin with 1, subrecords undefined but may be used for anything else */
#define PFM_SHOALS_ABH_DATA     18
#define PFM_SURF_DATA           19
#define PFM_SMF_DATA            20
#define PFM_VISE_DATA           21
#define PFM_PFM_DATA            22
#define PFM_MIF_DATA            23
#define PFM_SHOALS_TOF_DATA     24    /*!<  Records begin with 1, subrecord 0 is first return, subrecord 1 is last return
                                            if first return is within 0.05 of last return, first return is not loaded.  On
                                            unload, if first return is within 0.05 of last return and last return is invalid,
                                            first return is marked invalid.  */
#define PFM_UNISIPS_DEPTH_DATA  25    /*!<  Records begin with 0, this is UNISIPS center depth */
#define PFM_HYD93B_DATA         26
#define PFM_LADS_DATA           27
#define PFM_HS2_DATA            28
#define PFM_9COLLIDAR           29
#define PFM_FGE_DATA            30
#define PFM_PIVOT_DATA          31
#define PFM_MBSYSTEM_DATA       32
#define PFM_LAS_DATA            33
#define PFM_BDI_DATA            34
#define PFM_NAVO_LLZ_DATA       35    /*!<  Records begin with 0, no subrecords */
#define PFM_LADSDB_DATA         36
#define PFM_DTED_DATA           37    /*!<  Records begin with 0, record is lon line number, subrecords begin with 0, 
                                            subrecord is lat point number.  */
#define PFM_HAWKEYE_HYDRO_DATA  38    /*!<  Records begin with 0, no subrecords.  */
#define PFM_HAWKEYE_TOPO_DATA   39    /*!<  Records begin with 0, no subrecords.  */
#define PFM_BAG_DATA            40    /*!<  Records begin with 0, record is row number, subrecords begin with 0, 
                                            subrecord is column number.  */
#define PFM_CZMIL_DATA          41    /*!<  Coastal Zone Mapping and Imaging LIDAR, records begin with 0, subrecords are
                                            channel (0-8) times 100 plus the return (0-31).  */

#define PFM_DATA_TYPES          42    /*!<  Total number of PFM data types  */


  /*!  typedef for progress callback  */

  typedef void (*PFM_PROGRESS_CALLBACK) (int32_t state, int32_t percent);



  /*!
      The following data type descriptive strings are set in open_pfm_file.
      THIS IS BAD BAD BAD ... and probably should be changed
  */

  __PFM_EXTERN__ char                  *PFM_data_type[PFM_DATA_TYPES];



  /*!
      - PFM Public API functions

      - Internal library functions are found in pfmP.h
  */

  int32_t *get_bin_density( void );
  float get_avg_per_bin( void );
  float get_stddev_per_bin( void );

  int32_t get_cache_hits(int32_t hnd);
  int32_t get_cache_misses(int32_t hnd);
  int32_t get_cache_flushes(int32_t hnd);
  int32_t get_cache_size(/*size_t*/int32_t hnd);
  int32_t get_cache_peak_size(int32_t hnd);
  int32_t get_cache_max_size();

  void set_cache_size (int32_t hnd, int32_t max_rows, int32_t max_cols, int32_t center_row, int32_t center_col);
  void set_cache_size_max (/*size_t*/int32_t max);
  void set_use_cov_flag (int32_t hnd, int32_t flag);
  int32_t set_cached_cov_flag (int32_t hnd, NV_I32_COORD2 coord, uint8_t flag);
  int32_t flush_cov_flag (int32_t hnd);

  int32_t open_cached_pfm_file (PFM_OPEN_ARGS *open_args);
  void close_cached_pfm_file (int32_t hnd);
  int32_t read_cached_bin_record (int32_t hnd, NV_I32_COORD2 coord, BIN_RECORD_SUMMARY **bin_summary);
  int32_t write_cached_bin_record (int32_t hnd, BIN_RECORD_SUMMARY *bin_summary );
  int32_t add_cached_depth_record (int32_t hnd, DEPTH_RECORD *depth);

  int32_t recompute_cached_bin_values (int32_t hnd, BIN_RECORD *bin, uint32_t mask, DEPTH_RECORD *depth);

  int32_t flush_bin_cache (int32_t hnd);

  int32_t dump_cached_record (int32_t hnd, NV_I32_COORD2 coord);
  int32_t dump_cached_records (int32_t hnd);
  int32_t get_cached_depth_records (int32_t hnd, NV_I32_COORD2 coord, DEPTH_RECORD **depth, BIN_RECORD_SUMMARY **bin_summary);
  int32_t put_cached_depth_records (int32_t hnd, NV_I32_COORD2 coord, DEPTH_RECORD **depth, BIN_RECORD_SUMMARY **bin_summary);
  int32_t read_cached_depth_records (int32_t hnd, BIN_RECORD_SUMMARY **bin_summary);


  void pfm_substitute (int32_t hnd, char *path);
  void get_version (char *version);
  void pfm_get_version (int32_t *major, int32_t *minor);
  int32_t pfm_thread_init ();
  int32_t pfm_thread_destroy ();
  int32_t read_bin_header (int32_t hnd, BIN_HEADER *head);
  int32_t write_bin_header (int32_t hnd, BIN_HEADER *head, uint8_t init);
  void set_pfm_data_types ();
  int32_t open_pfm_file (PFM_OPEN_ARGS *open_args);
  int32_t open_existing_pfm_file (PFM_OPEN_ARGS *open_args);
  void close_pfm_file (int32_t hnd);
  int32_t read_list_file (int32_t hnd, int16_t file_number, char *path, int16_t *type);
  int16_t write_list_file (int32_t hnd, char *path, int16_t type);
  void delete_list_file (int32_t hnd, int32_t file_number);
  void restore_list_file (int32_t hnd, int32_t file_number);
  void update_target_file (int32_t hnd, char *lst, char *path);
  void update_mosaic_file (int32_t hnd, char *lst, char *path);
  void get_target_file (int32_t hnd, char *lst, char *path);
  void get_mosaic_file (int32_t hnd, char *lst, char *path);
  int32_t read_bin_record_index (int32_t hnd, NV_I32_COORD2 coord, BIN_RECORD *bin);
  void compute_index (NV_F64_COORD2 xy, NV_I32_COORD2 *coord, BIN_HEADER bin);
  void compute_index_ptr (NV_F64_COORD2 xy, NV_I32_COORD2 *coord, BIN_HEADER *bin);
  int32_t read_bin_record_xy (int32_t hnd, NV_F64_COORD2 xy, BIN_RECORD *bin);
  int32_t read_bin_row (int32_t hnd, int32_t length, int32_t row, int32_t column, BIN_RECORD a[]);
  int32_t read_bin_record_validity_index (int32_t hnd, NV_I32_COORD2 coord, uint32_t *validity);
  int32_t read_cov_map_index (int32_t hnd, NV_I32_COORD2 coord, uint8_t *cov);
  int32_t write_cov_map_index (int32_t hnd, NV_I32_COORD2 coord, uint8_t cov);
  int32_t read_coverage_map_index (int32_t hnd, NV_I32_COORD2 coord, uint8_t *data, uint8_t *checked);
  int32_t write_bin_record_index (int32_t hnd, BIN_RECORD *bin);
  int32_t write_bin_record_xy (int32_t hnd, BIN_RECORD *bin);
  int32_t write_bin_record_validity_index (int32_t hnd, BIN_RECORD *bin, uint32_t mask);
  int32_t read_depth_array_index (int32_t hnd, NV_I32_COORD2 coord, DEPTH_RECORD **depth_array, int32_t *numrecs);
  int32_t read_depth_array_xy (int32_t hnd, NV_F64_COORD2 xy, DEPTH_RECORD **depth_array, int32_t *numrecs);
  int32_t read_bin_depth_array_index (int32_t hnd, BIN_RECORD *bin, DEPTH_RECORD **depth_array);
  int32_t read_bin_depth_array_xy (int32_t hnd, NV_F64_COORD2 xy, BIN_RECORD *bin, DEPTH_RECORD **depth_array);
  int32_t update_depth_record_index (int32_t hnd, DEPTH_RECORD *depth);
  int32_t update_depth_record_xy (int32_t hnd, DEPTH_RECORD *depth);
  int32_t update_depth_record_index_ext_flags (int32_t hnd, DEPTH_RECORD *depth);
  int32_t update_depth_record_xy_ext_flags (int32_t hnd, DEPTH_RECORD *depth);
  int32_t add_depth_record_index (int32_t hnd, DEPTH_RECORD *depth);
  int32_t add_depth_record_xy (int32_t hnd, DEPTH_RECORD *depth);
  int32_t recompute_bin_values_index (int32_t hnd, NV_I32_COORD2 coord, BIN_RECORD *bin, uint32_t mask);
  int32_t recompute_bin_values_xy (int32_t hnd, NV_F64_COORD2 xy, BIN_RECORD *bin, uint32_t mask);
  int32_t recompute_bin_values_from_depth_index (int32_t hnd, BIN_RECORD *bin, uint32_t mask, DEPTH_RECORD *depth_array);
  int32_t change_depth_record_index (int32_t hnd, DEPTH_RECORD *depth);
  int32_t change_depth_record_nomod_index (int32_t hnd, DEPTH_RECORD *depth);
  int32_t change_bin_attribute_records_index (int32_t hnd, DEPTH_RECORD *depth);
  int32_t change_attribute_record_index (int32_t hnd, DEPTH_RECORD *depth);
  char *read_line_file (int32_t hnd, int16_t line_number);
  int16_t write_line_file (int32_t hnd, char *line);
  int32_t pfm_get_io_type (int32_t hnd);
  char *pfm_error_str (int32_t status);
  void pfm_error_exit (int32_t status);
  int32_t bin_inside (BIN_HEADER bin, NV_F64_COORD2 xy);
  int32_t bin_inside_ptr (BIN_HEADER *bin, NV_F64_COORD2 xy);
  int16_t get_next_list_file_number (int32_t hnd);
  int16_t get_next_line_number (int32_t hnd);
  uint8_t check_flag (uint8_t field, uint8_t flag);
  void set_flag (uint8_t *field, uint8_t flag);
  void clear_flag (uint8_t *field, uint8_t flag);
  void pfm_register_progress_callback (PFM_PROGRESS_CALLBACK progressCB);
  int32_t get_data_extents (int32_t hnd, NV_I32_COORD2 *min_coord, NV_I32_COORD2 *max_coord);
  void compute_center_xy (NV_F64_COORD2 *xy, NV_I32_COORD2 coord, BIN_HEADER *bin);
  int32_t pfm_geo_distance (int32_t hnd, double lat0, double lon0, double lat1, double lon1, double *distance);
  int32_t pfm_get_io_type (int32_t hnd);
  float get_depth_precision(int32_t hnd);
  float get_std_precision(int32_t hnd);
  float get_x_precision(int32_t hnd);
  float get_y_precision(int32_t hnd);


#ifdef  __cplusplus
}
#endif


#endif
