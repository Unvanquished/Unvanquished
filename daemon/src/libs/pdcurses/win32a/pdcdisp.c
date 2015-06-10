/* Public Domain Curses */

#include "pdcwin.h"

RCSID("$Id: pdcdisp.c,v 1.47 2008/07/14 04:24:52 wmcbrine Exp $")

#include <stdlib.h>
#include <string.h>
#include <tchar.h>

// #ifdef CHTYPE_LONG

# define A(x) ((chtype)x | A_ALTCHARSET)

/* The console version of this has four '# ifdef PDC_WIDE's in the
acs_map definition.  In this "real Windows" version,  though,  we
_always_ want to use the wide versions,  so I've changed all four to read
USE_WIDE_ALWAYS, and then defined that to be true.  One could, of course,
just lop out the non-wide versions,  but this way,  the code doesn't
actually change all that much,  which may make maintenance easier. */

#define USE_WIDE_ALWAYS

chtype acs_map[128] =
{
    A(0), A(1), A(2), A(3), A(4), A(5), A(6), A(7), A(8), A(9), A(10),
    A(11), A(12), A(13), A(14), A(15), A(16), A(17), A(18), A(19),
    A(20), A(21), A(22), A(23), A(24), A(25), A(26), A(27), A(28),
    A(29), A(30), A(31), ' ', '!', '"', '#', '$', '%', '&', '\'', '(',
    ')', '*',

# ifdef USE_WIDE_ALWAYS
    0x2192, 0x2190, 0x2191, 0x2193,
# else
    A(0x1a), A(0x1b), A(0x18), A(0x19),
# endif

    '/',

# ifdef USE_WIDE_ALWAYS
    0x2588,
# else
    0xdb,
# endif

    '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
    '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', '[', '\\', ']', '^', '_',

# ifdef USE_WIDE_ALWAYS
    0x2666, 0x2592,
# else
    A(0x04), 0xb1,
# endif

    'b', 'c', 'd', 'e',

# ifdef USE_WIDE_ALWAYS
    0x00b0, 0x00b1, 0x2591, 0x00a4, 0x2518, 0x2510, 0x250c, 0x2514,
    0x253c, 0x23ba, 0x23bb, 0x2500, 0x23bc, 0x23bd, 0x251c, 0x2524,
    0x2534, 0x252c, 0x2502, 0x2264, 0x2265, 0x03c0, 0x2260, 0x00a3,
    0x00b7,
# else
    0xf8, 0xf1, 0xb0, A(0x0f), 0xd9, 0xbf, 0xda, 0xc0, 0xc5, 0x2d, 0x2d,
    0xc4, 0x2d, 0x5f, 0xc3, 0xb4, 0xc1, 0xc2, 0xb3, 0xf3, 0xf2, 0xe3,
    0xd8, 0x9c, 0xf9,
# endif

    A(127)
};

# undef A

// #endif

static const unsigned short starting_ascii_to_unicode[32] = {
   0,
   0x263a,        /*   1 smiling face              */
   0x263b,        /*   2 smiling face inverted     */
   0x2665,        /*   3 heart                     */
   0x2666,        /*   4 diamond                   */
   0x2663,        /*   5 club                      */
   0x2660,        /*   6 spade                     */
   0x2024,        /*   7 small bullet              */
   0x25d8,        /*   8 inverted bullet           */
   0x25bc,        /*   9 hollow bullet             */
   0x25d9,        /*  10 inverted hollow bullet    */
   0x2642,        /*  11 male/Mars symbol          */
   0x2640,        /*  12 female/Venus symbol       */
   0x266a,        /*  13 eighth note               */
   0x266c,        /*  14 two sixteenth notes       */
   0x263c,        /*  15 splat                     */
   0x25b6,        /*  16 right-pointing triangle   */
   0x25c0,        /*  17 left-pointing triangle    */
   0x2195,        /*  18 double up/down arrow      */
   0x203c,        /*  19 double exclamation !!     */
   0x00b6,        /*  20 pilcrow                   */
   0xa7,          /*  21                           */
   0x2582,        /*  22 lower 1/3 block           */
   0x280d,        /*  23 double up/down arrow      */
   0x2191,        /*  24 up arrow                  */
   0x2193,        /*  25 down arrow                */
   0x2192,        /*  26 right arrow               */
   0x2190,        /*  27 left arrow                */
   0x2319,        /*  28                           */
   0x280c,        /*  29 left & right arrow        */
   0x25b2,        /*  30 up triangle               */
   0x25bc};       /*  31 down triangle             */

/* Cursors may be added to the 'shapes' array.  A 'shapes' string
defines the cursor as one or more rectangles,  separated by semicolons.
The coordinates of the upper left and lower right corners are given,
usually just as integers from zero to eight.  Thus,  "0488" means a
rectangle running from (0,4),  middle of the left side,  to (8,8),
bottom right corner:  a rectangle filling the bottom half of the
character cell. "0048" would fill the left half of the cell,  and
"0082;6088" would fill the top and bottom quarters of the cell.

   However,  a coordinate may be followed by a + or -,  and then by a
single-digit offset in pixels.  So "08-4" refers to a point on the
left-hand side of the character cell,  four pixels from the bottom. I
admit that the cursor descriptions themselves look a little strange!
But this way of describing cursors is compact and lends itself to some
pretty simple code.

   The first three lines are standard PDCurses cursors:  0=no cursor,
1=four-pixel thick line at bottom of the cell,  2="high-intensity",
i.e., a filled block.  The rest are extended cursors,  not currently
available in other PDCurses flavors. */

#define N_CURSORS 9

static void redraw_cursor_from_index( const HDC hdc, const int idx)
{
    const char *shapes[N_CURSORS] = {
        "",                         /* 0: invisible */
        "08-488",                    /* 1: normal:  four lines at bottom */
        "0088",                       /* 2: full block */
        "0088;0+10+18-18-1",           /* 3: outlined block */
        "28-368;4-10+34+18-3;2060+3",  /* 4: caret */
        "0488",                       /* 5: bottom half block */
        "2266",                      /* 6: central block     */
        "0385;3053;3558",           /* 7: cross */
//      "0385;3058",                /* 7: cross */
        "0088;0+10+48-18-4"  };    /* 8: outlined block: heavy top/bottom*/
    const char *sptr = shapes[idx];
    LONG left, top;
    extern int PDC_cxChar, PDC_cyChar;

    left = SP->curscol * PDC_cxChar;
    top = SP->cursrow * PDC_cyChar;
    while( *sptr)
    {
        int i;
        LONG coords[4];
        RECT rect;

        for( i = 0; i < 4; i++)
        {
            coords[i] = (( i & 1) ?
                         top + (PDC_cyChar * (*sptr - '0') + 4) / 8 :
                         left + (PDC_cxChar * (*sptr - '0') + 4) / 8);
            sptr++;
            if( *sptr == '+' || *sptr == '-')
            {
                if( *sptr == '+')
                   coords[i] += sptr[1] - '0';
                else
                   coords[i] -= sptr[1] - '0';
                sptr += 2;
            }
        }
        rect.left = coords[0];
        rect.top = coords[1];
        rect.right = coords[2];
        rect.bottom = coords[3];
        InvertRect( hdc, &rect);
        if( *sptr == ';')
            sptr++;
    }
}

/* PDC_current_cursor_state( ) determines which cursor,  if any,
is currently shown.  This may depend on the blink state.  Also,
if the window currently lacks the focus,  we show cursor 3 (a hollow
box) in place of any visible cursor.  */

static int PDC_current_cursor_state( void)
{
    extern int PDC_blink_state;
    extern HWND PDC_hWnd;
    const int shift_amount = (PDC_blink_state ? 0 : 8);
    const int cursor_style_for_unfocussed_window =
               PDC_CURSOR( PDC_CURSOR_OUTLINE, PDC_CURSOR_OUTLINE);
    int cursor_style;

            /* for unfocussed windows, show an hollow box: */
    if( SP->visibility && (PDC_hWnd != GetForegroundWindow( )))
        cursor_style = cursor_style_for_unfocussed_window;
    else    /* otherwise,  just show the cursor "normally" */
        cursor_style = SP->visibility;
    return( (cursor_style >> shift_amount) & 0xff);
}

static void redraw_cursor( const HDC hdc)
{
    const int cursor_style = PDC_current_cursor_state( );

    if( cursor_style > 0 && cursor_style < N_CURSORS)
        redraw_cursor_from_index( hdc, cursor_style);
}

/* position "hardware" cursor at (y, x).  We don't have a for-real hardware */
/* cursor in this version,  of course,  but we can fake it.  Note that much */
/* of the logic was borrowed from the SDL version.  In particular,  the     */
/* cursor is moved by first overwriting the "original" location.            */

void PDC_gotoyx(int row, int col)
{
    extern int PDC_blink_state;

    PDC_LOG(("PDC_gotoyx() - called: row %d col %d from row %d col %d\n",
             row, col, SP->cursrow, SP->curscol));

                /* clear the old cursor,  if it's on-screen: */
    if( SP->cursrow >= 0 && SP->curscol >= 0 &&
         SP->cursrow < SP->lines && SP->curscol < SP->cols)
    {
        const int temp_visibility = SP->visibility;

        SP->visibility = 0;
        PDC_transform_line( SP->cursrow, SP->curscol, 1,
                           curscr->_y[SP->cursrow] + SP->curscol);
        SP->visibility = temp_visibility;
    }

               /* ...then draw the new (assuming it's actually visible).    */
               /* This used to require some logic.  Now the redraw_cursor() */
               /* function figures out what cursor should be drawn, if any. */
    PDC_blink_state = 1;
    if( SP->visibility)
    {
        extern HWND PDC_hWnd;
        HDC hdc = GetDC( PDC_hWnd) ;

        SP->curscol = col;
        SP->cursrow = row;
        redraw_cursor( hdc);
        ReleaseDC( PDC_hWnd, hdc) ;
    }
}

int PDC_font_size = 12;
TCHAR PDC_font_name[80];

static LOGFONT PDC_get_logical_font( const attr_t font_attrib)
{
    LOGFONT lf;

    memset(&lf, 0, sizeof(LOGFONT));        // Clear out structure.
    lf.lfHeight = -PDC_font_size;
#ifdef PDC_WIDE
    if( !*PDC_font_name)
        _tcscpy( PDC_font_name, _T("Courier New"));
    _tcscpy( lf.lfFaceName, PDC_font_name );
#else
    if( !*PDC_font_name)
        strcpy( PDC_font_name, "Courier New");
    strcpy( lf.lfFaceName, PDC_font_name);              // Request font
#endif
//  lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    lf.lfPitchAndFamily = FF_MODERN;
    lf.lfWeight = ((font_attrib & A_BOLD) ? FW_EXTRABOLD : FW_NORMAL);
    lf.lfItalic = ((font_attrib & A_ITALIC) ? TRUE : FALSE);
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfOutPrecision = OUT_RASTER_PRECIS;
    return( lf);
}

HFONT PDC_get_font_handle( const attr_t font_attrib)
{
    LOGFONT lf = PDC_get_logical_font( font_attrib);

    return( CreateFontIndirect( &lf));
}

int debug_printf( const char *format, ...);        /* pdcscrn.c */

int PDC_choose_a_new_font( void)
{
    LOGFONT lf = PDC_get_logical_font( 0);
    CHOOSEFONT cf;
    int rval;
    extern HWND PDC_hWnd;

    lf.lfHeight = -PDC_font_size;
    debug_printf( "In PDC_choose_a_new_font: %d\n", lf.lfHeight);
    memset( &cf, 0, sizeof( CHOOSEFONT));
    cf.lStructSize = sizeof( CHOOSEFONT);
    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
    cf.hwndOwner = PDC_hWnd;
    cf.lpLogFont = &lf;
    cf.rgbColors = RGB( 0, 0, 0);
    rval = ChooseFont( &cf);
    if( rval)
#ifdef PDC_WIDE
// should this be _tcscpy() ???
        wcscpy( PDC_font_name, lf.lfFaceName);
#else
        strcpy( PDC_font_name, lf.lfFaceName);
#endif
    debug_printf( "rval %d; %ld\n", rval, CommDlgExtendedError( ));
    debug_printf( "output size: %d\n", lf.lfHeight);
    PDC_font_size = -lf.lfHeight;
    return( rval);
}

    /* This function 'intensifies' a color by shifting it toward white. */
    /* It used to average the input color with white.  Then it did a    */
    /* weighted average:  2/3 of the input color,  1/3 white,   for a   */
    /* lower "intensification" level.                                   */
    /*    Then Mark Hessling suggested that the output level should     */
    /* remap zero to 85 (= 255 / 3, so one-third intensity),  and input */
    /* of 192 or greater should be remapped to 255 (full intensity).    */
    /* Assuming we want a linear response between zero and 192,  that   */
    /* leads to output = 85 + input * (255-85)/192.                     */
    /*    This should lead to proper handling of bold text in legacy    */
    /* apps,  where "bold" means "high intensity".                      */

static COLORREF intensified_color( COLORREF ival)
{
    int rgb, i;
    COLORREF oval = 0;

    for( i = 0; i < 3; i++, ival >>= 8)
    {
        rgb = (int)( ival & 0xff);
        if( rgb >= 192)
            rgb = 255;
        else
            rgb = 85 + rgb * (255 - 85) / 192;
        oval |= ((COLORREF)rgb << (i * 8));
    }
    return( oval);
}

   /* For use in adjusting colors for A_DIMmed characters.  Just */
   /* knocks down the intensity of R, G, and B by 1/3.           */

static COLORREF dimmed_color( COLORREF ival)
{
    unsigned i;
    COLORREF oval = 0;

    for( i = 0; i < 3; i++, ival >>= 8)
    {
        unsigned rgb = (unsigned)( ival & 0xff);

        rgb -= (rgb / 3);
        oval |= ((COLORREF)rgb << (i * 8));
    }
    return( oval);
}

#ifdef CHTYPE_LONG
    #if(CHTYPE_LONG >= 2)       /* "non-standard" 64-bit chtypes     */
            /* PDCurses stores RGBs in fifteen bits,  five bits each */
            /* for red, green, blue.  A COLORREF uses eight bits per */
            /* channel.  Hence the following.                        */
static COLORREF extract_packed_rgb( const chtype color)
{
    const int red   = (int)( (color << 3) & 0xf8);
    const int green = (int)( (color >> 2) & 0xf8);
    const int blue  = (int)( (color >> 7) & 0xf8);

    return( RGB( red, green, blue));
}

    #endif
#endif

/* update the given physical line to look like the corresponding line in
curscr.

   NOTE that if x > 0,  we decrement it and srcp,  and increment the
length.  In other words,  we draw the preceding character,  too.  This
is done because,  at certain font sizes,  characters break out and
overwrite the preceding character.  That results in debris left on
the screen.

   The code also now increments the length only,  drawing one more
character (i.e.,  draws the character following the "text we really
want").  Again,  this helps to avoid debris left on the screen.

   The 'ExtTextOut' function takes an lpDx array that specifies the exact
placement of each character relative to the previous character.  This seems
to help avoid most (but not all) stray pixels from being displayed.  The
problem is that,  at certain font sizes,  letters may be drawn that don't
fit properly in the clip rectangle;  and they aren't actually clipped
correctly,  despite the use of the ETO_CLIPPED flag.  But things do seem
to be much better than was the case back when plain 'TextOut' was used. */

static HFONT hFonts[4];

#define BUFFSIZE 50

int PDC_find_ends_of_selected_text( const int line,
          const RECT *rect, int *x);            /* pdcscrn.c */

void PDC_transform_line_given_hdc( const HDC hdc, const int lineno,
                             int x, int len, const chtype *srcp)
{
    extern int PDC_blink_state;
    HFONT hOldFont = (HFONT)0;
    extern int PDC_cxChar, PDC_cyChar;
    int i, curr_color = -1;
    attr_t font_attrib = (attr_t)-1;
    int cursor_overwritten = FALSE;
    extern COLORREF *pdc_rgbs;
    COLORREF foreground_rgb = 0;
    chtype prev_ch = 0;
    extern RECT PDC_mouse_rect;              /* see 'pdcscrn.c' */
    int selection[2];

    if( !srcp)             /* just freeing up fonts */
    {
        for( i = 0; i < 4; i++)
            if( hFonts[i])
            {
                DeleteObject( hFonts[i]);
                hFonts[i] = NULL;
            }
        return;
    }
                     /* Seems to me as if the input text to this function */
    if( x < 0)       /* should _never_ be off-screen.  But it sometimes is. */
    {                /* Clipping is therefore necessary. */
        len += x;
        srcp -= x;
        x = 0;
    }
    len++;    /* draw an extra char to avoid leaving garbage on screen */
    if( len > SP->cols - x)
        len = SP->cols - x;
    if( lineno >= SP->lines || len <= 0 || lineno < 0)
        return;
    if( x)           /* back up by one character to avoid */
    {                /* leaving garbage on the screen */
        x--;
        len++;
        srcp--;
    }
    if( lineno == SP->cursrow && SP->curscol >= x && SP->curscol < x + len)
        if( PDC_current_cursor_state( ))
            cursor_overwritten = TRUE;
    while( len)
    {
        const attr_t attrib = (attr_t)( *srcp >> PDC_REAL_ATTR_SHIFT);
        const int color = (int)(( *srcp & A_COLOR) >> PDC_COLOR_SHIFT);
        attr_t new_font_attrib = (*srcp & (A_BOLD | A_ITALIC));
        RECT clip_rect;
        wchar_t buff[BUFFSIZE];
        int lpDx[BUFFSIZE + 1];
        int olen = 0;
//      int funny_chars = 0;

        for( i = 0; i < len && olen < BUFFSIZE - 1 &&
                    attrib == (attr_t)( srcp[i] >> PDC_REAL_ATTR_SHIFT); i++)
        {
            chtype ch = (srcp[i] & A_CHARTEXT);

#ifdef CHTYPE_LONG
            if( ch > 0xffff)    /* use Unicode surrogates to fit */
                {               /* >64K values into 16-bit wchar_t: */
                ch -= 0x10000;
                buff[olen] = (wchar_t)( 0xd800 | (ch >> 10));
                lpDx[olen] = PDC_cxChar;          /* ^ upper 10 bits */
                olen++;
                ch = (wchar_t)( 0xdc00 | (ch & 0x3ff));  /* lower 10 bits */
//              funny_chars = 1;
//              printf( "Stored as %x, %x\n", buff[olen-1], (unsigned)ch);
                }
            if( (srcp[i] & A_ALTCHARSET) && ch < 0x80)
                ch = acs_map[ch & 0x7f];
            else if( ch < 32)
               ch = starting_ascii_to_unicode[ch];
#ifndef PDC_WIDE                          /* If we're in Unicode,  assume */
            else if( ch <= 0xff)          /* the incoming text doesn't need */
               {                          /* code-page translation          */
               char c = (char)ch;
               wchar_t z;

               mbtowc( &z, &c, 1);
               ch = (chtype)z;
               }
#endif
#endif
            buff[olen] = (wchar_t)( ch & A_CHARTEXT);
            lpDx[olen] = PDC_cxChar;
            olen++;
        }
        lpDx[olen] = PDC_cxChar;
        if( color != curr_color || ((prev_ch ^ *srcp) & (A_REVERSE | A_BLINK)))
        {
            extern int PDC_really_blinking;          /* see 'pdcsetsc.c' */
            int reverse_colors = ((*srcp & A_REVERSE) ? 1 : 0);
            int intensify_backgnd = 0;
            COLORREF background_rgb;

            curr_color = color;
#ifdef CHTYPE_LONG
    #if(CHTYPE_LONG >= 2)       /* "non-standard" 64-bit chtypes     */
            if( *srcp & A_RGB_COLOR)
            {
                /* Extract RGB from 30 bits of the color field */
                background_rgb = extract_packed_rgb( *srcp >> PDC_COLOR_SHIFT);
                foreground_rgb = extract_packed_rgb( *srcp >> (PDC_COLOR_SHIFT + 15));
            }
            else
    #endif
#endif
            {
                short foreground_index, background_index;

                PDC_pair_content( (short)color, &foreground_index, &background_index);
                foreground_rgb = pdc_rgbs[foreground_index];
                background_rgb = pdc_rgbs[background_index];
            }

            if( *srcp & A_BLINK)
            {
                if( !PDC_really_blinking)   /* convert 'blinking' to 'bold' */
                {
                    new_font_attrib &= ~A_BLINK;
                    intensify_backgnd = 1;
                }
                else if( PDC_blink_state)
                    reverse_colors ^= 1;
            }
            if( reverse_colors)
            {
                const COLORREF temp = foreground_rgb;

                foreground_rgb = background_rgb;
                background_rgb = temp;
            }

            if( *srcp & A_BOLD)
                foreground_rgb = intensified_color( foreground_rgb);
            if( *srcp & A_DIM)
                foreground_rgb = dimmed_color( foreground_rgb);
            SetTextColor( hdc, foreground_rgb);

            if( intensify_backgnd)
                background_rgb = intensified_color( background_rgb);
            if( *srcp & A_DIM)
                background_rgb = dimmed_color( background_rgb);
            SetBkColor( hdc, background_rgb);
        }
        if( new_font_attrib != font_attrib)
        {
            HFONT hFont;
            int idx = 0;

            font_attrib = new_font_attrib;
            if( font_attrib & A_BOLD)
                idx |= 1;
            if( font_attrib & A_ITALIC)
                idx |= 2;
            if( !hFonts[idx])
                hFonts[idx] = PDC_get_font_handle( font_attrib);
            hFont = SelectObject( hdc, hFonts[idx]);
            if( !hOldFont)
                hOldFont = hFont;
        }
        prev_ch = *srcp;
        clip_rect.left = x * PDC_cxChar;
        clip_rect.top = lineno * PDC_cyChar;
        clip_rect.right = clip_rect.left + i * PDC_cxChar;
        clip_rect.bottom = clip_rect.top + PDC_cyChar;
        ExtTextOutW( hdc, clip_rect.left, clip_rect.top,
                           ETO_CLIPPED | ETO_OPAQUE, &clip_rect,
                           buff, olen, (olen > 1 ? lpDx : NULL));
#ifdef A_OVERLINE
        if( *srcp & (A_UNDERLINE | A_RIGHTLINE | A_LEFTLINE | A_OVERLINE | A_STRIKEOUT))
#else
        if( *srcp & (A_UNDERLINE | A_RIGHTLINE | A_LEFTLINE))
#endif
        {
            const int y1 = clip_rect.top;
            const int y2 = clip_rect.bottom - 1;
            const int x1 = clip_rect.left;
            const int x2 = clip_rect.right;
            int j;
            const HPEN pen = CreatePen( PS_SOLID, 1, (SP->line_color == -1 ?
                             foreground_rgb : pdc_rgbs[SP->line_color]));
            const HPEN old_pen = SelectObject( hdc, pen);

            if( *srcp & A_UNDERLINE)
            {
                MoveToEx( hdc, x1, y2, NULL);
                LineTo(   hdc, x2, y2);
            }
#ifdef A_OVERLINE
            if( *srcp & A_OVERLINE)
            {
                MoveToEx( hdc, x1, y1, NULL);
                LineTo(   hdc, x2, y1);
            }
            if( *srcp & A_STRIKEOUT)
            {
                MoveToEx( hdc, x1, (y1 + y2) / 2, NULL);
                LineTo(   hdc, x2, (y1 + y2) / 2);
            }
#endif
            if( *srcp & A_RIGHTLINE)
                for( j = 0; j < i; j++)
                {
                    MoveToEx( hdc, x2 - j * PDC_cxChar - 1, y1, NULL);
                    LineTo(   hdc, x2 - j * PDC_cxChar - 1, y2);
                }
            if( *srcp & A_LEFTLINE)
                for( j = 0; j < i; j++)
                {
                    MoveToEx( hdc, x1 + j * PDC_cxChar, y1, NULL);
                    LineTo(   hdc, x1 + j * PDC_cxChar, y2);
                }
            SelectObject( hdc, old_pen);
        }
        if( PDC_find_ends_of_selected_text( lineno, &PDC_mouse_rect, selection))
            if( x <= selection[1] + 1 && x + i >= selection[0])
            {
                RECT rect;

                rect.top = lineno * PDC_cyChar;
                rect.bottom = rect.top + PDC_cyChar;
                rect.right = max( x, selection[0]);
                rect.left = min( x + i, selection[1] + 1);
                rect.right *= PDC_cxChar;
                rect.left *= PDC_cxChar;
                InvertRect( hdc, &rect);
            }
        len -= i;
        x += i;
        srcp += i;
    }
    SelectObject( hdc, hOldFont);
               /* ...did we step on the cursor?  If so,  redraw it: */
    if( cursor_overwritten)
        redraw_cursor( hdc);
}

void PDC_transform_line(int lineno, int x, int len, const chtype *srcp)
{
    if( !srcp)    /* just freeing up fonts */
        PDC_transform_line_given_hdc( 0, 0, 0, 0, NULL);
    else
    {
        extern HWND PDC_hWnd;
        const HDC hdc = GetDC( PDC_hWnd) ;

        PDC_transform_line_given_hdc( hdc, lineno, x, len, srcp);
        ReleaseDC( PDC_hWnd, hdc);
    }
}

