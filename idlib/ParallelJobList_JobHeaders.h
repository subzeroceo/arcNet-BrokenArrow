#ifndef __PARALLELJOBLIST_JOBHEADERS_H__
#define __PARALLELJOBLIST_JOBHEADERS_H__

/*
================================================================================================

	Minimum set of headers needed to compile the code for a job.

================================================================================================
*/

#include "sys/sys_defines.h"

#include <stddef.h>					// for offsetof
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <basetsd.h>				// for UINT_PTR
#include <intrin.h>
#pragma warning( disable : 4100 )	// unreferenced formal parameter
#pragma warning( disable : 4127 )	// conditional expression is constant
#include "sys/sys_assert.h"
#include "sys/sys_types.h"
#include "math/Math.h"
#include "ParallelJobList.h"

#if _MSC_VER >= 1600
#undef nullptr
#define nullptr 0
#endif

#endif // !__PARALLELJOBLIST_JOBHEADERS_H__
