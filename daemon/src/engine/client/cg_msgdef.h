/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2011  Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef CG_MSGDEF_H
#define CG_MSGDEF_H

#include "cg_api.h"
#include "common/IPC/CommonSyscalls.h"

namespace Util {
	template<> struct SerializeTraits<snapshot_t> {
		static void Write(Writer& stream, const snapshot_t& snap)
		{
			stream.Write<uint32_t>(snap.snapFlags);
			stream.Write<uint32_t>(snap.ping);
			stream.Write<uint32_t>(snap.serverTime);
			stream.WriteData(&snap.areamask, MAX_MAP_AREA_BYTES);
			stream.Write<playerState_t>(snap.ps);
			stream.Write<std::vector<entityState_t>>(snap.entities);
			stream.Write<std::vector<std::string>>(snap.serverCommands);
		}
		static snapshot_t Read(Reader& stream)
		{
			snapshot_t snap;
			snap.snapFlags = stream.Read<uint32_t>();
			snap.ping = stream.Read<uint32_t>();
			snap.serverTime = stream.Read<uint32_t>();
			stream.ReadData(&snap.areamask, MAX_MAP_AREA_BYTES);
			snap.ps = stream.Read<playerState_t>();
			snap.entities = stream.Read<std::vector<entityState_t>>();
			snap.serverCommands = stream.Read<std::vector<std::string>>();
			return snap;
		}
	};

	// For skeletons, only send the bones which are used
	template<> struct SerializeTraits<refSkeleton_t> {
		static void Write(Writer& stream, const refSkeleton_t& skel)
		{
			stream.Write<uint32_t>(skel.type);
			stream.WriteSize(skel.numBones);
			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 3; j++) {
					stream.Write<float>(skel.bounds[i][j]);
				}
			}
			stream.Write<float>(skel.scale);
			size_t length = sizeof(refBone_t) * skel.numBones;
			stream.WriteData(&skel.bones, length);
		}
		static refSkeleton_t Read(Reader& stream)
		{
			refSkeleton_t skel;
			skel.type = static_cast<refSkeletonType_t>(stream.Read<uint32_t>());
			skel.numBones = stream.ReadSize<refBone_t>();
			for (int i = 0; i < 2; i++) {
				for (int j = 0; j < 3; j++) {
					skel.bounds[i][j] = stream.Read<float>();
				}
			}
			skel.scale = stream.Read<float>();

			if (skel.numBones > sizeof(skel.bones) / sizeof(refBone_t)) {
				Sys::Drop("IPC: Too many bones for refSkelon_t: %i", skel.numBones);
			}
			size_t length = sizeof(refBone_t) * skel.numBones;
			stream.ReadData(&skel.bones, length);
			return skel;
		}
	};

	// Use that bone optimization for refEntity_t
	template<> struct SerializeTraits<refEntity_t> {
		static void Write(Writer& stream, const refEntity_t& ent)
		{
			stream.WriteData(&ent, offsetof(refEntity_t, skeleton));
			stream.Write<refSkeleton_t>(ent.skeleton);
		}
		static refEntity_t Read(Reader& stream)
		{
			refEntity_t ent;
			stream.ReadData(&ent, offsetof(refEntity_t, skeleton));
			ent.skeleton = stream.Read<refSkeleton_t>();
			return ent;
		}
	};

	template<>
	struct SerializeTraits<Color::Color> {
		static void Write(Writer& stream, const Color::Color& value)
		{
			stream.WriteData(value.ToArray(), value.ArrayBytes());
		}
		static Color::Color Read(Reader& stream)
		{
			Color::Color value;
			stream.ReadData(value.ToArray(), value.ArrayBytes());
			return value;
		}
	};
}

enum cgameImport_t
{
  // Misc
  CG_SENDCLIENTCOMMAND,
  CG_UPDATESCREEN,
  CG_CM_MARKFRAGMENTS,
  CG_GETCURRENTSNAPSHOTNUMBER,
  CG_GETSNAPSHOT,
  CG_GETCURRENTCMDNUMBER,
  CG_GETUSERCMD,
  CG_SETUSERCMDVALUE,
  CG_GET_ENTITY_TOKEN,
  CG_REGISTER_BUTTON_COMMANDS,
  CG_GETCLIPBOARDDATA,
  CG_QUOTESTRING,
  CG_GETTEXT,
  CG_PGETTEXT,
  CG_GETTEXT_PLURAL,
  CG_NOTIFY_TEAMCHANGE,
  CG_PREPAREKEYUP,
  CG_GETNEWS,

  // Sound
  CG_S_STARTSOUND,
  CG_S_STARTLOCALSOUND,
  CG_S_CLEARLOOPINGSOUNDS,
  CG_S_ADDLOOPINGSOUND,
  CG_S_STOPLOOPINGSOUND,
  CG_S_UPDATEENTITYPOSITION,
  CG_S_RESPATIALIZE,
  CG_S_REGISTERSOUND,
  CG_S_STARTBACKGROUNDTRACK,
  CG_S_STOPBACKGROUNDTRACK,
  CG_S_UPDATEENTITYVELOCITY,
  CG_S_SETREVERB,
  CG_S_BEGINREGISTRATION,
  CG_S_ENDREGISTRATION,

  // Renderer
  CG_R_SETALTSHADERTOKENS,
  CG_R_GETSHADERNAMEFROMHANDLE,
  CG_R_SCISSOR_ENABLE,
  CG_R_SCISSOR_SET,
  CG_R_INPVVS,
  CG_R_LOADWORLDMAP,
  CG_R_REGISTERMODEL,
  CG_R_REGISTERSKIN,
  CG_R_REGISTERSHADER,
  CG_R_REGISTERFONT,
  CG_R_CLEARSCENE,
  CG_R_ADDREFENTITYTOSCENE,
  CG_R_ADDPOLYTOSCENE,
  CG_R_ADDPOLYSTOSCENE,
  CG_R_ADDLIGHTTOSCENE,
  CG_R_ADDADDITIVELIGHTTOSCENE,
  CG_R_RENDERSCENE,
  CG_R_ADD2DPOLYSINDEXED,
  CG_R_SETCOLOR,
  CG_R_SETCLIPREGION,
  CG_R_RESETCLIPREGION,
  CG_R_DRAWSTRETCHPIC,
  CG_R_DRAWROTATEDPIC,
  CG_R_MODELBOUNDS,
  CG_R_LERPTAG,
  CG_R_REMAP_SHADER,
  CG_R_INPVS,
  CG_R_LIGHTFORPOINT,
  CG_R_REGISTERANIMATION,
  CG_R_BUILDSKELETON,
  CG_R_BONEINDEX,
  CG_R_ANIMNUMFRAMES,
  CG_R_ANIMFRAMERATE,
  CG_REGISTERVISTEST,
  CG_ADDVISTESTTOSCENE,
  CG_CHECKVISIBILITY,
  CG_UNREGISTERVISTEST,
  CG_SETCOLORGRADING,
  CG_R_GETTEXTURESIZE,
  CG_R_GENERATETEXTURE,

  // Keys
  CG_KEY_GETCATCHER,
  CG_KEY_SETCATCHER,
  CG_KEY_GETKEYNUMFORBINDS,
  CG_KEY_KEYNUMTOSTRINGBUF,
  CG_KEY_SETBINDING,
  CG_KEY_CLEARSTATES,
  CG_KEY_CLEARCMDBUTTONS,
  CG_KEY_KEYSDOWN,

  // Mouse
  CG_MOUSE_SETMOUSEMODE,

  // Lan
  CG_LAN_GETSERVERCOUNT,
  CG_LAN_GETSERVERINFO,
  CG_LAN_GETSERVERPING,
  CG_LAN_MARKSERVERVISIBLE,
  CG_LAN_SERVERISVISIBLE,
  CG_LAN_UPDATEVISIBLEPINGS,
  CG_LAN_RESETPINGS,
  CG_LAN_SERVERSTATUS,
  CG_LAN_RESETSERVERSTATUS,

  // Rocket
  CG_ROCKET_INIT,
  CG_ROCKET_SHUTDOWN,
  CG_ROCKET_LOADDOCUMENT,
  CG_ROCKET_LOADCURSOR,
  CG_ROCKET_DOCUMENTACTION,
  CG_ROCKET_GETEVENT,
  CG_ROCKET_DELETEEVENT,
  CG_ROCKET_REGISTERDATASOURCE,
  CG_ROCKET_DSADDROW,
  CG_ROCKET_DSCLEARTABLE,
  CG_ROCKET_SETINNERRML,
  CG_ROCKET_GETATTRIBUTE,
  CG_ROCKET_SETATTRIBUTE,
  CG_ROCKET_GETPROPERTY,
  CG_ROCKET_SETPROPERTYBYID,
  CG_ROCKET_GETEVENTPARAMETERS,
  CG_ROCKET_REGISTERDATAFORMATTER,
  CG_ROCKET_DATAFORMATTERRAWDATA,
  CG_ROCKET_DATAFORMATTERFORMATTEDDATA,
  CG_ROCKET_REGISTERELEMENT,
  CG_ROCKET_GETELEMENTTAG,
  CG_ROCKET_GETELEMENTABSOLUTEOFFSET,
  CG_ROCKET_QUAKETORML,
  CG_ROCKET_SETCLASS,
  CG_ROCKET_INITHUDS,
  CG_ROCKET_LOADUNIT,
  CG_ROCKET_ADDUNITTOHUD,
  CG_ROCKET_SHOWHUD,
  CG_ROCKET_CLEARHUD,
  CG_ROCKET_ADDTEXT,
  CG_ROCKET_CLEARTEXT,
  CG_ROCKET_REGISTERPROPERTY,
  CG_ROCKET_SHOWSCOREBOARD,
  CG_ROCKET_SETDATASELECTINDEX,
  CG_ROCKET_LOADFONT
};

// All Miscs

// TODO really sync?
using SendClientCommandMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_SENDCLIENTCOMMAND>, std::string>
>;
// TODO really sync?
using UpdateScreenMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_UPDATESCREEN>>
>;
// TODO can move to VM too ?
using CMMarkFragmentsMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_CM_MARKFRAGMENTS>, std::vector<std::array<float, 3>>, std::array<float, 3>, int, int>,
	IPC::Reply<std::vector<std::array<float, 3>>, std::vector<markFragment_t>>
>;
// TODO send all snapshots at the beginning of the frame
using GetCurrentSnapshotNumberMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETCURRENTSNAPSHOTNUMBER>>,
	IPC::Reply<int, int>
>;
using GetSnapshotMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETSNAPSHOT>, int>,
	IPC::Reply<bool, snapshot_t>
>;
using GetCurrentCmdNumberMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETCURRENTCMDNUMBER>>,
	IPC::Reply<int>
>;
using GetUserCmdMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETUSERCMD>, int>,
	IPC::Reply<bool, usercmd_t>
>;
using SetUserCmdValueMsg = IPC::Message<IPC::Id<VM::QVM, CG_SETUSERCMDVALUE>, int, int, float>;
// TODO what?
using GetEntityTokenMsg =  IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GET_ENTITY_TOKEN>, int>,
	IPC::Reply<bool, std::string>
>;
using RegisterButtonCommandsMsg = IPC::Message<IPC::Id<VM::QVM, CG_REGISTER_BUTTON_COMMANDS>, std::string>;
using GetClipboardDataMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETCLIPBOARDDATA>, int>,
	IPC::Reply<std::string>
>;
// TODO using Command.h for that ?
using QuoteStringMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_QUOTESTRING>, int, std::string>,
	IPC::Reply<std::string>
>;
using GettextMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETTEXT>, int, std::string>,
	IPC::Reply<std::string>
>;
using PGettextMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_PGETTEXT>, int, std::string, std::string>,
	IPC::Reply<std::string>
>;
using GettextPluralMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETTEXT_PLURAL>, int, std::string, std::string, int>,
	IPC::Reply<std::string>
>;
using NotifyTeamChangeMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_NOTIFY_TEAMCHANGE>, int>
>;
using PrepareKeyUpMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_PREPAREKEYUP>>
>;
using GetNewsMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETNEWS>, bool>,
	IPC::Reply<bool>
>;

// All Sounds

namespace Audio {
	using RegisterSoundMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_S_REGISTERSOUND>, std::string>,
		IPC::Reply<int>
	>;

    //Command buffer syscalls

	using StartSoundMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_STARTSOUND>, bool, Vec3, int, int>;
	using StartLocalSoundMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_STARTLOCALSOUND>, int>;
	using ClearLoopingSoundsMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_CLEARLOOPINGSOUNDS>>;
	using AddLoopingSoundMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_ADDLOOPINGSOUND>, int, int>;
	using StopLoopingSoundMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_STOPLOOPINGSOUND>, int>;
	using UpdateEntityPositionMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_UPDATEENTITYPOSITION>, int, Vec3>;
	using RespatializeMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_RESPATIALIZE>, int, std::array<Vec3, 3>>;
	using StartBackgroundTrackMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_STARTBACKGROUNDTRACK>, std::string, std::string>;
	using StopBackgroundTrackMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_STOPBACKGROUNDTRACK>>;
	using UpdateEntityVelocityMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_UPDATEENTITYVELOCITY>, int, Vec3>;
	using SetReverbMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_SETREVERB>, int, std::string, float>;
	using BeginRegistrationMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_BEGINREGISTRATION>>;
	using EndRegistrationMsg = IPC::Message<IPC::Id<VM::QVM, CG_S_ENDREGISTRATION>>;
}

namespace Render {
	using SetAltShaderTokenMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_SETALTSHADERTOKENS>, std::string>;
	using GetShaderNameFromHandleMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_GETSHADERNAMEFROMHANDLE>, int>,
		IPC::Reply<std::string>
	>;
	// TODO not a renderer call, handle in CM in the VM?
	using InPVVSMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_INPVVS>, std::array<float, 3>, std::array<float, 3>>,
		IPC::Reply<bool>
	>;
	// TODO is it really async?
	using LoadWorldMapMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_LOADWORLDMAP>, std::string>;
	using RegisterModelMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERMODEL>, std::string>,
		IPC::Reply<int>
	>;
	using RegisterSkinMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERSKIN>, std::string>,
		IPC::Reply<int>
	>;
	using RegisterShaderMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERSHADER>, std::string, int>,
		IPC::Reply<int>
	>;
	using RegisterFontMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERFONT>, std::string, std::string, int>,
		IPC::Reply<fontMetrics_t>
	>;
	using ModelBoundsMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_MODELBOUNDS>, int>,
		IPC::Reply<std::array<float, 3>, std::array<float, 3>>
	>;
	using LerpTagMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_LERPTAG>, refEntity_t, std::string, int>,
		IPC::Reply<orientation_t, int>
	>;
	using RemapShaderMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_REMAP_SHADER>, std::string, std::string, std::string>;
	// TODO not a renderer call, handle in CM in the VM?
	using InPVSMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_INPVS>, std::array<float, 3>, std::array<float, 3>>,
		IPC::Reply<bool>
	>;
	using LightForPointMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_LIGHTFORPOINT>, std::array<float, 3>>,
		IPC::Reply<std::array<float, 3>, std::array<float, 3>, std::array<float, 3>, int>
	>;
	using RegisterAnimationMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERANIMATION>, std::string>,
		IPC::Reply<int>
	>;
	using BuildSkeletonMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_BUILDSKELETON>, int, int, int, float, bool>,
		IPC::Reply<refSkeleton_t, int>
	>;
	using BoneIndexMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_BONEINDEX>, int, std::string>,
		IPC::Reply<int>
	>;
	using AnimNumFramesMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_ANIMNUMFRAMES>, int>,
		IPC::Reply<int>
	>;
	using AnimFrameRateMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_ANIMFRAMERATE>, int>,
		IPC::Reply<int>
	>;
	using RegisterVisTestMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_REGISTERVISTEST>>,
		IPC::Reply<int>
	>;
	using CheckVisibilityMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_CHECKVISIBILITY>, int>,
		IPC::Reply<float>
	>;
	using GetTextureSizeMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_GETTEXTURESIZE>, qhandle_t>,
		IPC::Reply<int, int>
	>;
	using GenerateTextureMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_GENERATETEXTURE>, std::vector<byte>, int, int>,
		IPC::Reply<qhandle_t>
	>;

    // All command buffer syscalls

	using ScissorEnableMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_SCISSOR_ENABLE>, bool>;
	using ScissorSetMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_SCISSOR_SET>, int, int, int, int>;
	using ClearSceneMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_CLEARSCENE>>;
	using AddRefEntityToSceneMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_ADDREFENTITYTOSCENE>, refEntity_t>;
	using AddPolyToSceneMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_ADDPOLYTOSCENE>, int, std::vector<polyVert_t>>;
	using AddPolysToSceneMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_ADDPOLYSTOSCENE>, int, std::vector<polyVert_t>, int, int>;
	using AddLightToSceneMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_ADDLIGHTTOSCENE>, std::array<float, 3>, float, float, float, float, float, int, int>;
	using AddAdditiveLightToSceneMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_ADDADDITIVELIGHTTOSCENE>, std::array<float, 3>, float, float, float, float>;
	using SetColorMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_SETCOLOR>, Color::Color>;
	using SetClipRegionMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_SETCLIPREGION>, std::array<float, 4>>;
	using ResetClipRegionMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_RESETCLIPREGION>>;
	using DrawStretchPicMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_DRAWSTRETCHPIC>, float, float, float, float, float, float, float, float, int>;
	using DrawRotatedPicMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_DRAWROTATEDPIC>, float, float, float, float, float, float, float, float, int, float>;
	using AddVisTestToSceneMsg = IPC::Message<IPC::Id<VM::QVM, CG_ADDVISTESTTOSCENE>, int, std::array<float, 3>, float, float>;
	using UnregisterVisTestMsg = IPC::Message<IPC::Id<VM::QVM, CG_UNREGISTERVISTEST>, int>;
	using SetColorGradingMsg = IPC::Message<IPC::Id<VM::QVM, CG_SETCOLORGRADING>, int, int>;
	using RenderSceneMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_RENDERSCENE>, refdef_t>;
	using Add2dPolysIndexedMsg = IPC::Message<IPC::Id<VM::QVM, CG_R_ADD2DPOLYSINDEXED>, std::vector<polyVert_t>, int, std::vector<int>, int, int, int, qhandle_t>;
}

namespace Key {
	using GetCatcherMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_KEY_GETCATCHER>>,
		IPC::Reply<int>
	>;
	using SetCatcherMsg = IPC::Message<IPC::Id<VM::QVM, CG_KEY_SETCATCHER>, int>;
	using GetKeynumForBindsMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_KEY_GETKEYNUMFORBINDS>, int, std::vector<std::string>>,
		IPC::Reply<std::vector<std::vector<int>>>
	>;
	using KeyNumToStringMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_KEY_KEYNUMTOSTRINGBUF>, int>,
		IPC::Reply<std::string>
	>;
	using SetBindingMsg = IPC::Message<IPC::Id<VM::QVM, CG_KEY_SETBINDING>, int, int, std::string>;
	using ClearCmdButtonsMsg = IPC::Message<IPC::Id<VM::QVM, CG_KEY_CLEARCMDBUTTONS>>;
	using ClearStatesMsg = IPC::Message<IPC::Id<VM::QVM, CG_KEY_CLEARSTATES>>;
	using KeysDownMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_KEY_KEYSDOWN>, std::vector<int>>,
		IPC::Reply<std::vector<int>>
	>;
}

namespace Mouse {
	using SetMouseMode = IPC::Message<IPC::Id<VM::QVM, CG_MOUSE_SETMOUSEMODE>, MouseMode>;
}

namespace LAN {
	using GetServerCountMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_LAN_GETSERVERCOUNT>, int>,
		IPC::Reply<int>
	>;
	using GetServerInfoMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_LAN_GETSERVERINFO>, int, int, int>,
		IPC::Reply<std::string>
	>;
	using GetServerPingMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_LAN_GETSERVERPING>, int, int>,
		IPC::Reply<int>
	>;
	using MarkServerVisibleMsg = IPC::Message<IPC::Id<VM::QVM, CG_LAN_MARKSERVERVISIBLE>, int, int, bool>;
	using ServerIsVisibleMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_LAN_SERVERISVISIBLE>, int, int>,
		IPC::Reply<bool>
	>;
	using UpdateVisiblePingsMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_LAN_UPDATEVISIBLEPINGS>, int>,
		IPC::Reply<bool>
	>;
	using ResetPingsMsg = IPC::Message<IPC::Id<VM::QVM, CG_LAN_RESETPINGS>, int>;
	using ServerStatusMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_LAN_SERVERSTATUS>, std::string, int>,
		IPC::Reply<std::string, int>
	>;
	using ResetServerStatusMsg = IPC::Message<IPC::Id<VM::QVM, CG_LAN_RESETSERVERSTATUS>>;
}

namespace Rocket {
	// TODO all of these are declared as sync but some might be async
	// it is not really important as librocket will be moved to nacl

	using InitMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_INIT>>
	>;
	using ShutdownMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_SHUTDOWN>>
	>;
	using LoadDocumentMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_LOADDOCUMENT>, std::string>
	>;
	using LoadCursorMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_LOADCURSOR>, std::string>
	>;
	using DocumentActionMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_DOCUMENTACTION>, std::string, std::string>
	>;
	using GetEventMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_GETEVENT>>,
		IPC::Reply<bool, std::string>
	>;
	using DeleteEventMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_DELETEEVENT>>
	>;
	using RegisterDataSourceMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_REGISTERDATASOURCE>, std::string>
	>;
	using DSAddRowMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_DSADDROW>, std::string, std::string, std::string>
	>;
	using DSClearTableMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_DSCLEARTABLE>, std::string, std::string>
	>;
	using SetInnerRMLMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_SETINNERRML>, std::string, int>
	>;
	using GetAttributeMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_GETATTRIBUTE>, std::string, int>,
		IPC::Reply<std::string>
	>;
	using SetAttributeMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_SETATTRIBUTE>, std::string, std::string>
	>;
	using GetPropertyMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_GETPROPERTY>, std::string, int, int>,
		IPC::Reply<std::vector<char>>
	>;
	using SetPropertyMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_SETPROPERTYBYID>, std::string, std::string>
	>;
	using GetEventParametersMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_GETEVENTPARAMETERS>, int>,
		IPC::Reply<std::string>
	>;
	using RegisterDataFormatterMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_REGISTERDATAFORMATTER>, std::string>
	>;
	using DataFormatterDataMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_DATAFORMATTERRAWDATA>, int, int, int>,
		IPC::Reply<std::string, std::string>
	>;
	using DataFormatterFormattedDataMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_DATAFORMATTERFORMATTEDDATA>, int, std::string, bool>
	>;
	using RegisterElementMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_REGISTERELEMENT>, std::string>
	>;
	using GetElementTagMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_GETELEMENTTAG>, int>,
		IPC::Reply<std::string>
	>;
	using GetElementAbsoluteOffsetMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_GETELEMENTABSOLUTEOFFSET>>,
		IPC::Reply<float, float>
	>;
	using QuakeToRMLMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_QUAKETORML>, std::string, int>,
		IPC::Reply<std::string>
	>;
	using SetClassMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_SETCLASS>, std::string, bool>
	>;
	using InitHUDsMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_INITHUDS>, int>
	>;
	using LoadUnitMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_LOADUNIT>, std::string>
	>;
	using AddUnitToHUDMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_ADDUNITTOHUD>, int, std::string>
	>;
	using ShowHUDMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_SHOWHUD>, int>
	>;
	using ClearHUDMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_CLEARHUD>, int>
	>;
	using AddTextMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_ADDTEXT>, std::string, std::string, float, float>
	>;
	using ClearTextMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_CLEARTEXT>>
	>;
	using RegisterPropertyMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_REGISTERPROPERTY>, std::string, std::string, bool, bool, std::string>
	>;
	using ShowScoreboardMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_SHOWSCOREBOARD>, std::string, bool>
	>;
	using SetDataSelectIndexMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_SETDATASELECTINDEX>, int>
	>;
	using LoadFontMsg = IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_LOADFONT>, std::string>
	>;
}

enum cgameExport_t
{
  CG_STATIC_INIT,

  CG_INIT,
//  void CG_Init( int serverMessageNum, int serverCommandSequence )
  // called when the level loads or when the renderer is restarted
  // all media should be registered at this time
  // cgame will display loading status by calling SCR_Update, which
  // will call CG_DrawInformation during the loading process
  // reliableCommandSequence will be 0 on fresh loads, but higher for
  // demos or vid_restarts

  CG_SHUTDOWN,
//  void (*CG_Shutdown)();
  // oportunity to flush and close any open files

  CG_DRAW_ACTIVE_FRAME,
//  void (*CG_DrawActiveFrame)( int serverTime, bool demoPlayback );
  // Generates and draws a game scene and status information at the given time.
  // If demoPlayback is set, local movement prediction will not be enabled

  CG_CROSSHAIR_PLAYER,
//  int (*CG_CrosshairPlayer)();

  CG_KEY_EVENT,
//  void    (*CG_KeyEvent)( int key, bool down );

  CG_MOUSE_EVENT,
//  void    (*CG_MouseEvent)( int dx, int dy );

  CG_MOUSE_POS_EVENT,
//  void    (*CG_MousePosEvent)( int x, int y );

  CG_TEXT_INPUT_EVENT,
// pass in text input events from the engine

  CG_ROCKET_VM_INIT,
// Inits libRocket in the game.

  CG_ROCKET_FRAME,
// Rocket runs through a frame, including event processing, and rendering

  CG_CONSOLE_LINE,

  CG_FOCUS_EVENT
// void (*CG_FocusEvent)( bool focus);
};

using CGameStaticInitMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_STATIC_INIT>, int>
>;
using CGameInitMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_INIT>, int, int, glconfig_t, GameStateCSs>
>;
using CGameShutdownMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_SHUTDOWN>>
>;
using CGameDrawActiveFrameMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_DRAW_ACTIVE_FRAME>, int, bool>
>;
using CGameCrosshairPlayerMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_CROSSHAIR_PLAYER>>,
	IPC::Reply<int>
>;
using CGameKeyEventMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_KEY_EVENT>, int, bool>
>;
using CGameMouseEventMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_MOUSE_EVENT>, int, int>
>;
using CGameMousePosEventMsg = IPC::SyncMessage<
    IPC::Message<IPC::Id<VM::QVM, CG_MOUSE_POS_EVENT>, int, int>
>;
using CGameTextInptEvent = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_TEXT_INPUT_EVENT>, int>
>;
using CGameFocusEventMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_FOCUS_EVENT>, bool>
>;

//TODO Check all rocket calls
using CGameRocketInitMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_VM_INIT>, glconfig_t>
>;
using CGameRocketFrameMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_FRAME>, cgClientState_t>
>;

using CGameConsoleLineMsg = IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_CONSOLE_LINE>, std::string>
>;

#endif
