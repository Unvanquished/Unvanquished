////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 07:12:05 +0200 (Sa, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
// Title: BotExports
//		In order for the game to call functions from the bot, we must export
//		the functions to the game itself and allow it to call them. 
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __BOTEXPORTS_H__
#define __BOTEXPORTS_H__

#include "Functions_Bot.h"
#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"
#include "IEngineInterface.h"

//////////////////////////////////////////////////////////////////////////
// Export the function on platforms that require it.
#ifdef WIN32
#define OMNIBOT_API __declspec(dllexport)
#else
#define OMNIBOT_API 
#endif

// Typedef for the only exported bot function.
typedef eomnibot_error (*pfnGetFunctionsFromDLL)(Bot_EngineFuncs_t *_pBotFuncs, int _size);

// note: Export Functions with C Linkage
//	Export with C Linkage so the game interface can acccess it easier.
//	This gets rid of name mangling
//	Wrapped in #ifdef because the game SDK might be in pure C
#ifdef __cplusplus
extern "C" 
{
#endif
	// function: ExportBotFunctionsFromDLL
	//		Allow the bot dll to fill in a struct of bot functions the interface
	//		can then call.
	OMNIBOT_API eomnibot_error ExportBotFunctionsFromDLL(Bot_EngineFuncs_t *_pBotFuncs, int _size);
#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////
// Interfaces

extern Bot_EngineFuncs_t	g_BotFunctions;
extern IEngineInterface		*g_InterfaceFunctions;

//////////////////////////////////////////////////////////////////////////
// Utility Function
extern "C" const char *OB_VA(const char* _msg, ...);
//////////////////////////////////////////////////////////////////////////

eomnibot_error Omnibot_LoadLibrary(int version, const char *lib, const char *path);
void Omnibot_FreeLibrary();
bool IsOmnibotLoaded();
const char *Omnibot_ErrorString(eomnibot_error err);
const char *Omnibot_GetLibraryPath();
const char *Omnibot_FixPath(const char *_path);
#endif
