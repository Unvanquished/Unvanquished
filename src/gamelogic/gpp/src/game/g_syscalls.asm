code

equ trap_Print                        -1
equ trap_Error                        -2
equ trap_Milliseconds                 -3
equ trap_Cvar_Register                -4
equ trap_Cvar_Update                  -5
equ trap_Cvar_Set                     -6
equ trap_Cvar_VariableIntegerValue    -7
equ trap_Cvar_VariableStringBuffer    -8
equ trap_Argc                         -9
equ trap_Argv                         -10
equ trap_FS_FOpenFile                 -11
equ trap_FS_Read                      -12
equ trap_FS_Write                     -13
equ trap_FS_FCloseFile                -14
equ trap_SendConsoleCommand           -15
equ trap_LocateGameData               -16
equ trap_DropClient                   -17
equ trap_SendServerCommand            -18
equ trap_SetConfigstring              -19
equ trap_GetConfigstring              -20
equ trap_SetConfigstringRestrictions  -21
equ trap_GetUserinfo                  -22
equ trap_SetUserinfo                  -23
equ trap_GetServerinfo                -24
equ trap_SetBrushModel                -25
equ trap_Trace                        -26
equ trap_PointContents                -27
equ trap_InPVS                        -28
equ trap_InPVSIgnorePortals           -29
equ trap_AdjustAreaPortalState        -30
equ trap_AreasConnected               -31
equ trap_LinkEntity                   -32
equ trap_UnlinkEntity                 -33
equ trap_EntitiesInBox                -34
equ trap_EntityContact                -35
equ trap_GetUsercmd                   -36
equ trap_GetEntityToken               -37
equ trap_FS_GetFileList               -38
equ trap_RealTime                     -39
equ trap_SnapVector                   -40
equ trap_TraceCapsule                 -41
equ trap_EntityContactCapsule         -42
equ trap_FS_Seek                      -43

equ trap_Parse_AddGlobalDefine        -44
equ trap_Parse_LoadSource             -45
equ trap_Parse_FreeSource             -46
equ trap_Parse_ReadToken              -47
equ trap_Parse_SourceFileAndLine      -48

equ trap_SendGameStat                 -49

equ trap_AddCommand                   -50
equ trap_RemoveCommand                -51

equ memset                            -101
equ memcpy                            -102
equ strncpy                           -103
equ sin                               -104
equ cos                               -105
equ atan2                             -106
equ sqrt                              -107
equ floor                             -111
equ ceil                              -112
equ testPrintInt                      -113
equ testPrintFloat                    -114
