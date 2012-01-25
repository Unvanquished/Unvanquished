/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - XviD plugin: dump pgm files of original and encoded frames  -
 *
 *  Copyright(C) 2003 Peter Ross <pross@xvid.org>
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
 * $Id: plugin_dump.c,v 1.3 2006/08/23 20:27:22 Skal Exp $
 *
 ****************************************************************************/

#include <stdio.h>

#include "../xvid.h"
#include "../image/image.h"


int xvid_plugin_dump(void * handle, int opt, void * param1, void * param2)
{
    switch(opt) {
    case XVID_PLG_INFO :
	{
        xvid_plg_info_t * info = (xvid_plg_info_t*)param1;
        info->flags = XVID_REQORIGINAL;
        return(0);
	}

    case XVID_PLG_CREATE :
		*((void**)param2) = NULL; /* We don't have any private data to bind here */
    case XVID_PLG_DESTROY :
    case XVID_PLG_BEFORE :
	case XVID_PLG_FRAME :
		return(0);

    case XVID_PLG_AFTER :
	{
		xvid_plg_data_t * data = (xvid_plg_data_t*)param1;
		IMAGE img;
		char tmp[100];
		img.y = data->original.plane[0];
		img.u = data->original.plane[1];
		img.v = data->original.plane[2];
		sprintf(tmp, "ori-%03i.pgm", data->frame_num);
		image_dump_yuvpgm(&img, data->original.stride[0], data->width, data->height, tmp);

		img.y = data->current.plane[0];
		img.u = data->current.plane[1];
		img.v = data->current.plane[2];
		sprintf(tmp, "enc-%03i.pgm", data->frame_num);
		image_dump_yuvpgm(&img, data->current.stride[0], data->width, data->height, tmp);
	}

	return(0);
    }

    return XVID_ERR_FAIL;
}
