
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



#ifndef __PFMP_H
#define __PFMP_H

#ifdef  __cplusplus
extern "C" {
#endif


/*!
   
   - The pfmP.h file contains the private header file components of
   the PFM API. These functions are required internally but are not
   directly exposed to the user. The public pfm.h file exposes the
   set of API functions availble to applications that use the 
   libraries.
   
   - Mark A. Paton

*/

   
/*!  Bin record size and bit offsets in the bit-packed index record buffer.  */

typedef struct
{
  uint32_t                    record_size;             /*!<  Record size in bytes  */
  uint32_t                    num_soundings_pos;       /*!<  Bit position of the number of soundings in the record buffer  */
  uint32_t                    std_pos;                 /*!<  Bit position of the standard deviation  */
  uint32_t                    avg_filtered_depth_pos;  /*!<  Bit position of the average filtered depth  */
  uint32_t                    min_filtered_depth_pos;  /*!<  Bit position of the minimum filtered depth  */
  uint32_t                    max_filtered_depth_pos;  /*!<  Bit position of the maximum filtered depth  */
  uint32_t                    avg_depth_pos;           /*!<  Bit position of the average unfiltered depth  */
  uint32_t                    min_depth_pos;           /*!<  Bit position of the minimum unfiltered depth  */
  uint32_t                    max_depth_pos;           /*!<  Bit position of the maximum unfiltered depth  */
  uint32_t                    num_valid_pos;           /*!<  Bit position of the number of valid soundings  */
  uint32_t                    attr_pos[NUM_ATTR];      /*!<  Bit position of the attributes  */
  uint32_t                    validity_pos;            /*!<  Bit position of the validity data  */
  uint32_t                    head_pointer_pos;        /*!<  Bit position of the depth chain head pointer  */
  uint32_t                    tail_pointer_pos;        /*!<  Bit position of the depth chain tail pointer  */
} BIN_RECORD_OFFSETS;


/* **** Internal Functions (I believe - MP) **** */

static int32_t update_cov_map (int32_t hnd, int64_t address);
static void destroy_depth_buffer ( int32_t hnd, DEPTH_SUMMARY *depth);
static int32_t prepare_cached_depth_buffer (int32_t hnd, DEPTH_SUMMARY *depth, uint8_t *depth_buffer, uint8_t preallocateBuffer );
static int32_t write_cached_depth_buffer (int32_t hnd, uint8_t *buffer, int64_t address);
static int32_t write_cached_depth_summary (int32_t hnd, BIN_RECORD_SUMMARY *depth);
static int32_t pack_bin_record (int32_t hnd, uint8_t *bin_data, BIN_RECORD *bin, BIN_RECORD_OFFSETS *offsets,
                                BIN_HEADER_DATA *hd, int16_t list_file_ver, CHAIN *depth_chain);
static int32_t pack_depth_record( uint8_t *depth_buffer, DEPTH_RECORD *depth, int32_t record_pos, int32_t hnd);

#ifdef  __cplusplus
}
#endif

#endif
