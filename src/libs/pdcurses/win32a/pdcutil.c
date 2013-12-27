/* Public Domain Curses */

#include <windows.h>
#include "pdcwin.h"

RCSID("$Id: pdcutil.c,v 1.14 2008/07/14 04:24:52 wmcbrine Exp $")

void PDC_beep(void)
{
    PDC_LOG(("PDC_beep() - called\n"));

/*  MessageBeep(MB_OK); */
    MessageBeep(0XFFFFFFFF);
}


void PDC_napms(int ms)
{
    /* RR: keep GUI window responsive while PDCurses sleeps */
    MSG msg;
    DWORD ms_sleep_limit = ms + GetTickCount();
    extern BOOL PDC_bDone;

    PDC_LOG(("PDC_napms() - called: ms=%d\n", ms));

    /* Pump all pending messages from WIN32 to the window handler */
    while( !PDC_bDone && GetTickCount() < ms_sleep_limit )
    {
        while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
        {
           TranslateMessage(&msg);
           DispatchMessage(&msg);
        }
        Sleep(1);
    }

    /* Sleep(ms); */
}

const char *PDC_sysname(void)
{
    return "Win32a";
}
