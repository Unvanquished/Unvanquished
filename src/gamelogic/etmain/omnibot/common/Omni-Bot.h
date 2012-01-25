////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy: jswigart $
// $LastChangedDate: 2010-08-28 07:12:05 +0200 (Sa, 28 Aug 2010) $
// $LastChangedRevision: 32 $
//
// about: Exported function definitions
//		In order for the game to call functions from the bot, we must export
//		the functions to the game itself and allow it to call them. 
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __OMNIBOT_H__
#define __OMNIBOT_H__

#include "Functions_Bot.h"
#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"
#include "MessageHelper.h"
#include "IEngineInterface.h"

// function: BotInitialise
//		Initializes the bot library and sets the bot up with the callbacks to 
//		the game in the form of function pointers to functions within the game.
omnibot_error BotInitialise(IEngineInterface *_pEngineFuncs, int _version);
// function: BotShutdown
//		Shuts down and frees memory from the bot system
void BotShutdown();
// function: BotUpdate
//		Called regularly by the game in order for the bots to perform their "thinking"
void BotUpdate();
// function: BotConsoleCommand
//		Any time commands from the game are executed, this will get called
//		to allow the bot to process it and perform any necessary actions.
void BotConsoleCommand(const Arguments &_args);
// function: BotAddGoal
//		Allows the game to register a goal with the bot that the bots can use
void BotAddGoal(const MapGoalDef &goaldef);
// function: BotAddBBRecord
//		Allows the game to enter blackboard records into the bots knowledge database.
void BotAddBBRecord(BlackBoard_Key _type, int _posterID, int _targetID, obUserData *_data);
// function: BotSendTrigger
//		Allows the game to notify the bot of triggered events.
void BotSendTrigger(const TriggerInfo &_triggerInfo);
// function: BotSendEvent
//		New Messagehelper based event handler.
void BotSendEvent(int _dest, const MessageHelper &_message);
// function: BotSendGlobalEvent
//		New Messagehelper based event handler.
void BotSendGlobalEvent(const MessageHelper &_message);
// function: BotUpdateEntity
//		Update map goal entity
void BotUpdateEntity(GameEntity oldent,GameEntity newent);
// function: BotDeleteMapGoal
//		Delete map goal by name
void BotDeleteMapGoal(const char *goalname);

//SubscriberHandle Message_SubscribeToMsg(int _msg, pfnMessageFunction _func);
//void Message_Unsubscribe(const SubscriberHandle _handle);
//MessageHelper Message_BeginMessage(int _msgId, size_t _messageSize);
//MessageHelper Message_BeginMessageEx(int _msgId, void *_mem, size_t _messageSize);
//void Message_EndMessage(const MessageHelper &_helper);
//void Message_EndMessageEx(const MessageHelper &_helper);

#endif
