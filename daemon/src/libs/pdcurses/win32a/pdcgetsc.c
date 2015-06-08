/* Public Domain Curses */

#include "pdcwin.h"

RCSID("$Id: pdcgetsc.c,v 1.36 2008/07/14 04:24:52 wmcbrine Exp $")

/* get the cursor size/shape */

int PDC_get_cursor_mode(void)
{
    PDC_LOG(("PDC_get_cursor_mode() - called\n"));

    return SP->visibility;
}

/* return number of screen rows */

int PDC_get_rows(void)
{
    extern int PDC_n_rows;

    PDC_LOG(("PDC_get_rows() - called\n"));
    return( PDC_n_rows);
}

int PDC_get_columns(void)
{
    extern int PDC_n_cols;

    PDC_LOG(("PDC_get_columns() - called\n"));
    return( PDC_n_cols);
}
