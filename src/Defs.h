#ifndef DEFS_H
#define DEFS_H

#include <crtdbg.h>

#ifdef _DEBUG
#define DBG_MALLOC(SIZE) (_malloc_dbg((SIZE), _NORMAL_BLOCK, __FILE__, __LINE__))
#else
#define DBG_MALLOC(SIZE) (malloc((SIZE)))
#endif

#endif // !DEFS_H