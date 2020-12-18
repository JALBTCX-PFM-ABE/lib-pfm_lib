#ifndef LARGE_IO
#define LARGE_IO

#include "pfm_nvtypes.h"
#include "pfm.h"


#ifdef  __cplusplus
extern "C" {
#endif


int32_t lfseek (int32_t handle, int64_t offset, int32_t whence);
int32_t lfopen (char *path, char *mode);
size_t lfread (void *ptr, size_t size, size_t nmemb, int32_t handle);
size_t lfwrite (void *ptr, size_t size, size_t nmemb, int32_t handle);
int32_t lftruncate (int32_t handle, int64_t length);
int64_t lftell (int32_t handle);
int32_t lfclose (int32_t handle);
void lfflush (int32_t handle);


#ifdef  __cplusplus
}
#endif


#endif
