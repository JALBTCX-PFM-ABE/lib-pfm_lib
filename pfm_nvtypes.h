
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



/* pfm_nvtypes.h, NAVO Standard Data Type Definitions */

/*

  One of the following definitions should be added to CFLAGS or CXXFLAGS:

  -DNVLinux or -DNVWIN3X

*/


#ifndef __NVDEFS__
#define __NVDEFS__


#ifdef  __cplusplus
extern "C" {
#endif


#define NVFalse 0
#define NVTrue 1

#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined (NVWIN3X) || defined (__MINGW32__) || defined (__MINGW64__)
  #include <process.h>
  #define SEPARATOR '\\'
  #define PATH_SEPARATOR ';'
#else
  #include <libgen.h>
  #define SEPARATOR '/'
  #define PATH_SEPARATOR ':'
#endif


#if defined (NVWIN3X) || defined (__MINGW32__) || defined (__MINGW64__)
  #ifdef __WIN64__
    #define __NV64__
    #define NV_SIZE_T_SPEC "%I64d"
  #else
    #undef __NV64__
    #define NV_SIZE_T_SPEC "%d"
  #endif
#else
  #ifdef __LP64__
    #define __NV64__
  #else
    #undef __NV64__
  #endif
  #define NV_SIZE_T_SPEC "%zd"
#endif


  typedef struct
  {
    uint16_t        r;
    uint16_t        g;
    uint16_t        b;
  } NV_C_RGB;


  typedef struct
  {
    uint16_t        h;
    uint16_t        s;
    uint16_t        v;
  } NV_C_HSV;


  typedef struct
  {
    float           x;
    float           y;
  } NV_F32_COORD2;


  typedef struct
  {
    float           x;
    float           y;
    float           z;
  } NV_F32_COORD3;


  typedef struct
  {
    double          x;
    double          y;
  } NV_F64_COORD2;


  typedef struct
  {
    double          x;
    double          y;
    double          z;
  } NV_F64_COORD3;


  typedef struct
  {
    int32_t         x;
    int32_t         y;
  } NV_I32_COORD2;


  typedef struct
  {
    int32_t         x;
    int32_t         y;
    int32_t         z;
  } NV_I32_COORD3;


  typedef struct
  {
    float           lat;
    float           lon;
  } NV_F32_POS;


  typedef struct
  {
    float           lat;
    float           lon;
    float           dep;
  } NV_F32_POSDEP;


  typedef struct
  {
    double          lat;
    double          lon;
  } NV_F64_POS;


  typedef struct
  {
    double          lat;
    double          lon;
    double          dep;
  } NV_F64_POSDEP;


  typedef struct
  {
    double          slat;
    double          wlon;
    double          nlat;
    double          elon;
  } NV_F64_MBR;


  typedef struct
  {
    double          min_y;
    double          min_x;
    double          max_y;
    double          max_x;
  } NV_F64_XYMBR;


  typedef struct
  {
    double          slat;
    double          wlon;
    double          nlat;
    double          elon;
    double          min_depth;
    double          max_depth;
  } NV_F64_MBC;


  typedef struct
  {
    double          min_y;
    double          min_x;
    double          max_y;
    double          max_x;
    double          min_z;
    double          max_z;
  } NV_F64_XYMBC;


  typedef struct
  {
    uint16_t        r;
    uint16_t        g;
    uint16_t        b;
    uint16_t        a;
  } NV_C_RGBA;


#ifdef  __cplusplus
}
#endif


#endif
