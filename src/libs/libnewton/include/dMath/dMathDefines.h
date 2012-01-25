//********************************************************************
// Newton Game dynamics 
// copyright 2000-2004
// By Julio Jerez
// VC: 6.0
// basic Hierarchical Scene Node Class
//********************************************************************

#ifndef __dMathDefined__
#define __dMathDefined__

#include <math.h>
#include <float.h>

#ifndef dFloat
	#ifdef __USE_DOUBLE_PRECISION__
		typedef double dFloat;
	#else 
		typedef float dFloat;
	#endif
#endif 

// transcendental functions
#define	dAbs(x)		dFloat (fabs (dFloat(x))) 
#define	dSqrt(x)	dFloat (sqrt (dFloat(x))) 
#define	dFloor(x)	dFloat (floor (dFloat(x))) 
#define	dMod(x,y)	dFloat (fmod (dFloat(x), dFloat(y))) 

#define dSin(x)		dFloat (sin (dFloat(x)))
#define dCos(x)		dFloat (cos (dFloat(x)))
#define dAsin(x)	dFloat (asin (dFloat(x)))
#define dAcos(x)	dFloat (acos (dFloat(x)))
#define	dAtan2(x,y) dFloat (atan2 (dFloat(x), dFloat(y)))

#ifndef _MSC_VER
	#define _ASSERTE(x)
	#ifndef min
		#define min(a,b) ((a < b) ? (a) : (b))
		#define max(a,b) ((a > b) ? (a) : (b))
	#endif 
#endif


#ifdef _MSC_VER
	#ifdef _DEBUG
		void dExpandTraceMessage (const char *fmt, ...);
		#define dTrace(x)										\
		{														\
			dExpandTraceMessage x;								\
		}																	
	#else
		#define dTrace(x)
	#endif
#endif


#endif
