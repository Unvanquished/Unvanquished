/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - emms C wrapper -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: emms.c,v 1.11 2005/11/22 10:23:01 suxen_drol Exp $
 *
 ****************************************************************************/

#include "emms.h"
#include "../portab.h"

/*****************************************************************************
 * Library data, declared here
 ****************************************************************************/

emmsFuncPtr emms;


/*****************************************************************************
 * emms functions
 *
 * emms functions are used to restored the fpu context after mmx operations
 * because mmx and fpu share their registers/context
 *
 ****************************************************************************/

/* The no op wrapper for non MMX platforms */
void
emms_c(void)
{
}
