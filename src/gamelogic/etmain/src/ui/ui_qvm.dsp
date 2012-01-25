# Microsoft Developer Studio Project File - Name="q3_ui_qvm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=q3_ui_qvm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "q3_ui_qvm.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "q3_ui_qvm.mak" CFG="q3_ui_qvm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "q3_ui_qvm - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "q3_ui_qvm - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "q3_ui_qvm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f q3_ui_qvm.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "q3_ui_qvm.exe"
# PROP BASE Bsc_Name "q3_ui_qvm.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "vc_qvm q3_ui_qvm.dsp "C:\games\Quake III Arena\Utilities\source\code\ui" ..\q3_ui\ui_syscalls ui_main -DNDEBUG -DQ3_VM -DCGAME -S -Wf-target=bytecode -Wf-g -I..\cgame -I..\game -I..\q3_ui -I..\ui"
# PROP Rebuild_Opt "/a"
# PROP Target_File "ui.qvm"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "q3_ui_qvm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f q3_ui_qvm.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "q3_ui_qvm.exe"
# PROP BASE Bsc_Name "q3_ui_qvm.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "vc_qvm q3_ui_qvm.dsp "\games\Quakei~1\Utilities\source\ccmengine\code\vm\ui" \games\Quakei~1\Utilities\source\ccmengine\code\q3_ui\ui_syscalls ui_main -D_DEBUG -DQ3_VM -DCGAME -S -Wf-target=bytecode -Wf-g -I..\cgame -I..\game -I..\q3_ui -I..\ui"
# PROP Rebuild_Opt "/a"
# PROP Target_File "ui.qvm"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "q3_ui_qvm - Win32 Release"
# Name "q3_ui_qvm - Win32 Debug"

!IF  "$(CFG)" == "q3_ui_qvm - Win32 Release"

!ELSEIF  "$(CFG)" == "q3_ui_qvm - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\game\bg_lib.c
# End Source File
# Begin Source File

SOURCE=..\game\bg_misc.c
# End Source File
# Begin Source File

SOURCE=..\game\q_math.c
# End Source File
# Begin Source File

SOURCE=..\game\q_shared.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_addbots.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_atoms.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_cdkey.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_cinematics.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_confirm.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_connect.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_controls2.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_credits.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_demo2.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_display.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_dynamicmenu.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_gameinfo.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_ingame.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_ingame_mapvote.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_loadconfig.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_main.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_menu.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_mfield.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_mods.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_network.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_options.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_playermodel.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_players.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_playersettings.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_preferences.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_qmenu.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_removebots.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_serverinfo.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_servers2.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_setup.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_sound.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_sparena.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_specifyserver.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_splevel.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_sppostgame.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_spreset.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_spskill.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_bot.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_botsel.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_common.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_custommaps.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_data.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_items.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_items_old.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_map.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_mapsel.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_script.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_server.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_team.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_teamorders.c
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_video.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\game\bg_lib.h
# End Source File
# Begin Source File

SOURCE=..\q3_ui\keycodes.h
# End Source File
# Begin Source File

SOURCE=..\game\q_shared.h
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_dynamicmenu.h
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_local.h
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_public.h
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver.h
# End Source File
# Begin Source File

SOURCE=..\q3_ui\ui_startserver_q3.h
# End Source File
# End Group
# End Target
# End Project
