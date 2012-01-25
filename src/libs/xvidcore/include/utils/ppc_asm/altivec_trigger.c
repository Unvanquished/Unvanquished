/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Altivec SIGILL trigger -
 *
 *  Copyright(C) 2004 Edouard Gomez <ed.gomez@free.fr>
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
 * $Id: altivec_trigger.c,v 1.1 2004/04/05 20:36:37 edgomez Exp $
 *
 ****************************************************************************/

#ifdef HAVE_ALTIVEC_H
#include <altivec.h>
#endif

#include "../../portab.h"
#include "../emms.h"

void
altivec_trigger(void)
{
	vector unsigned int var1 = (vector unsigned int)AVV(0);
	vector unsigned int var2 = (vector unsigned int)AVV(1);
	var1 = vec_add(var1, var2);
	return;
}
