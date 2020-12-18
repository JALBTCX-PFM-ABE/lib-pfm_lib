
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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef NVWIN3X
  #include <windows.h>
  #include <stdlib.h>
  #include <stdio.h>
  #include <io.h>
  #include <direct.h>
#else
  #include <unistd.h>
#endif

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

#define NUM_LARGE_FILES         MAX_PFM_FILES * 3
#define LARGE_NAME_SIZE         512

static FILE                     *file_pnt[NUM_LARGE_FILES];
static char                     file_mode[NUM_LARGE_FILES][10];
static char                     file_path[NUM_LARGE_FILES][LARGE_NAME_SIZE];
static char                     file_context[NUM_LARGE_FILES];
static int32_t                  file_handle[NUM_LARGE_FILES] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                                                -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};


/***************************************************************************/
/*!

  - Module Name:        lfseek

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2009

  - Purpose:            Positions to offset within the large file.

  - Arguments:
                        - handle          =   large file handle
                        - offset          =   See fseek (same except long long)
                        - whence          =   See fseek

  - Return Value:
                        - 0               =   success
                        - -1              =   failure, see errno for reason

  - Warning:            This does not work for a whence setting of SEEK_CUR

****************************************************************************/

int32_t lfseek (int32_t handle, int64_t offset, int32_t whence)
{
  /*  Check if a seek is required or not.  */
  /*  NOTE:  We don't use or support SEEK_CUR in the large_io package.  */

  if (whence == SEEK_SET && ftello64 (file_pnt[handle]) == offset) return 0;

  return (fseeko64 (file_pnt[handle], offset, whence));
}




/***************************************************************************/
/*!

  - Module Name:        lfopen

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2009

  - Purpose:            Opens a large file

  - Arguments:
                        - path            =   See fopen
                        - mode            =   See fopen

  - Return Value:       file handle (0 or higher) or -1 on failure

  - Explanation:        This library contains code to do normal I/O functions
                        on the _LARGEFILE64_SOURCE files used in PFM (i.e.
                        bin, ndx, hyp).

  - Caveats:            At present this library is only supported under gcc
                        on Linux/UNIX, Mac OS/X, and MinGW (32 and 64) on
                        Windows.  There is an arbitrary limit of 450 "large"
                        files that can be open at once.

****************************************************************************/

int32_t lfopen (char *path, char *mode)
{
  int32_t                     i, handle = -1;


  /*  Check for an available handle.  */

  handle = -1;
  for (i = 0 ; i < NUM_LARGE_FILES ; i++)
    {
      if (file_handle[i] == -1)
        {
          handle = i;
          file_handle[handle] = i;
          file_context[i] = ' ';
          break;
        }
    }


  /*  We may be out of handles.  */

  if (handle == -1) return (-1);


  /*  Save the path name.  */

  strcpy (file_path[handle], path);


  /*  Save the mode.   */
    
  strcpy (file_mode[handle], mode);


  /*  Open the file.  */

  if ((file_pnt[handle] = fopen64 (file_path[handle], mode)) == NULL) 
    {
      file_handle[handle] = -1;
      return (-1);
    }


  /*  Return the file handle.    */
    
  return (handle);
}




/***************************************************************************/
/*!

  - Module Name:        lfread

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2009

  - Purpose:            Reads data from the large file pointed to by handle.

  - Arguments:
                        - ptr             =   See fread
                        - size            =   See fread
                        - nmemb           =   See fread
                        - handle          =   handle of large file

  - Return Value:       Number of items read or 0 on failure

****************************************************************************/

size_t lfread (void *ptr, size_t size, size_t nmemb, int32_t handle)
{
  /*  Note if the file context switched or not.  If it did we need to flush the buffer.  */

  if (file_context[handle] != 'r')
    {
      file_context[handle] = 'r';
      fflush (file_pnt[handle]);
    }


  /*  Read the items and return the number of items read.    */
    
  return (fread (ptr, size, nmemb, file_pnt[handle]));
}




/***************************************************************************/
/*!

  - Module Name:        lfwrite

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2009

  - Purpose:            Writes data to the large file pointed to by handle.

  - Arguments:
                        - ptr             =   See fwrite
                        - size            =   See fwrite
                        - nmemb           =   See fwrite
                        - handle          =   handle of large file

  - Return Value:       Number of items written or 0 on failure

****************************************************************************/

size_t lfwrite (void *ptr, size_t size, size_t nmemb, int32_t handle)
{
  /*  Note if the file context switched or not.  If it did we need to flush the buffer.  */

  if (file_context[handle] != 'w')
    {
      file_context[handle] = 'w';
      fflush (file_pnt[handle]);
    }


    /*  Write the items and return the number of items written.    */
    
  return (fwrite (ptr, size, nmemb, file_pnt[handle]));
}




/***************************************************************************/
/*!

  - Module Name:        lftell

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2009

  - Purpose:            Get the current position within a large file.

  - Arguments:
                        - handle          =   handle of large file

  - Return Value:       Current position within the large file

****************************************************************************/

int64_t lftell (int32_t handle)
{
  return (ftello64 (file_pnt[handle]));
}




/***************************************************************************/
/*!

  - Module Name:        lftruncate

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2009

  - Purpose:            Truncates the file to "length" bytes.

  - Arguments:
                        - handle          =   handle of large file
                        - length          =   length

  - Return Value:
                        - 0               =   success
                        - -1              =   failure, see errno for reason

****************************************************************************/

int32_t lftruncate (int32_t handle, int64_t length)
{
  int32_t                     opened, fd, stat;


  /*  If this file has been previously opened, close it.   */

  opened = NVFalse;
  if (file_pnt[handle] != NULL)
    {
      fclose (file_pnt[handle]);
      file_pnt[handle] = NULL;
      opened = NVTrue;
    }


  /*  Truncate the file.  */

#ifdef NVWIN3X
  fd = open (file_path[handle], O_RDWR);

  if (fd < 0) return (fd);

  stat = _chsize (fd, length);
#else
  #ifdef __APPLE__
    fd = open (file_path[handle], O_RDWR);
  #else
    fd = open (file_path[handle], O_RDWR | O_LARGEFILE);
  #endif

  if (fd < 0) return (fd);


  stat = ftruncate64 (fd, length);
#endif

  if (stat) return (stat);

  close (fd);


  /*  If the file had been opened, re-open it.  */

  if (opened)
    {
      if ((file_pnt[handle] = fopen64 (file_path[handle], file_mode[handle])) == NULL) 
        {
          file_handle[handle] = -1;
          return (-1);
        }
    }


  return (0);
}




/***************************************************************************/
/*!

  - Module Name:        lfclose

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2009

  - Purpose:            Closes a large file

  - Arguments:
                        - handle          =   handle of large file

  - Return Value:
                        - 0               =   success
                        - EOF             =   failure

****************************************************************************/

int32_t lfclose (int32_t handle)
{
  int32_t             status;


  status = fclose (file_pnt[handle]);


  /*  Make this handle available again.  */

  file_handle[handle] = -1;


  /*  Return the status.  */
    
  return (status);
}



/***************************************************************************/
/*!

  - Module Name:        lfflush

  - Programmer(s):      Jan C. Depner

  - Date Written:       June 2009

  - Purpose:            Flushes the buffer for the opened file.

  - Arguments:
                        - handle          =   handle of large file

  - Return Value:       NONE

****************************************************************************/

void lfflush (int32_t handle)
{
  fflush (file_pnt[handle]);
}
