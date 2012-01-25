//********************************************************************
// Newton Game dynamics 
// copyright 2000
// By Julio Jerez
// VC: 6.0
// simple 4d matrix class
//********************************************************************
#include "dStdAfxMath.h"
#include "dMathDefines.h"


#ifdef _DEBUG
#include <stdio.h>

#if (_MSC_VER >= 1400)
#	pragma warning (disable: 4996) // for 2005 users declared deprecated
#endif

void dExpandTraceMessage (const char *fmt, ...)
{
	va_list v_args;
	char text[1024];

	text[0] = 0;
	va_start (v_args, fmt);     
	vsprintf(text, fmt, v_args);
	va_end (v_args);            

	OutputDebugStringA (text);
}
#endif
