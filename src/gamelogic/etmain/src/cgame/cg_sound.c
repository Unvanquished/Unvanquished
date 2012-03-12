/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Daemon GPL Source Code (Daemon Source Code).  

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the Daemon 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

// Ridah, cg_sound.c - parsing and use of sound script files

#include "cg_local.h"

// we have to define these static lists, since we can't alloc memory within the cgame

#define FILE_HASH_SIZE          1024
static soundScript_t *hashTable[FILE_HASH_SIZE];

#define MAX_SOUND_SCRIPTS       4096
static soundScript_t soundScripts[MAX_SOUND_SCRIPTS];
int             numSoundScripts = 0;

#define MAX_SOUND_SCRIPT_SOUNDS 8192
static soundScriptSound_t soundScriptSounds[MAX_SOUND_SCRIPT_SOUNDS];
int             numSoundScriptSounds = 0;

/*
================
return a hash value for the filename
================
*/
static long generateHashValue(const char *fname)
{
	int             i;
	long            hash;
	char            letter;

	hash = 0;
	i = 0;
	while(fname[i] != '\0')
	{
		letter = tolower(fname[i]);
		if(letter == '.')
		{
			break;				// don't include extension
		}
		if(letter == '\\')
		{
			letter = '/';		// damn path names
		}
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash &= (FILE_HASH_SIZE - 1);
	return hash;
}

/*
==============
CG_SoundScriptPrecache

  returns the index+1 of the script in the global list, for fast calling
==============
*/
int CG_SoundScriptPrecache(const char *name)
{
	soundScriptSound_t *scriptSound;
	long            hash;
	char           *s;
	soundScript_t  *sound;

//  byte buf[1024];
	int             i;

	if(!name || !name[0])
	{
		return 0;
	}

	hash = generateHashValue(name);

	s = (char *)name;
	for(sound = hashTable[hash]; sound; sound = sound->nextHash)
	{
		if(!Q_stricmp(s, sound->name))
		{
			// found a match, precache these sounds
			scriptSound = sound->soundList;
			if(!sound->streaming)
			{
				for(; scriptSound; scriptSound = scriptSound->next)
				{
					for(i = 0; i < scriptSound->numsounds; i++)
					{
						scriptSound->sounds[i].sfxHandle = 0;
//                      scriptSound->sounds[i].sfxHandle = trap_S_RegisterSound( scriptSound->sounds[i].filename, qfalse ); // FIXME: make compressed settable through the soundscript
					}
				}
			}
			else				/*if (cg_buildScript.integer) */
			{					// Enabled this permanently so that streaming sounds get touched within file system on startup
				for(; scriptSound; scriptSound = scriptSound->next)
				{
					for(i = 0; i < scriptSound->numsounds; i++)
					{
						// just open the file so it gets copied to the build dir
/*						fileHandle_t f;
						trap_FS_FOpenFile( scriptSound->sounds[i].filename, &f, FS_READ );
						trap_FS_Read( buf, sizeof( buf ), f ); // read a few bytes so the operating system does a better job of caching it for us
						trap_FS_FCloseFile( f );*/
					}
				}
			}
			return sound->index + 1;
		}
	}

	return 0;
}

/*
==============
CG_SoundPickOldestRandomSound
==============
*/
int CG_SoundPickOldestRandomSound(soundScript_t * sound, vec3_t org, int entnum)
{
	int             oldestTime = 0;	// TTimo: init
	soundScriptSound_t *oldestSound;
	soundScriptSound_t *scriptSound;

	oldestSound = NULL;
	scriptSound = sound->soundList;
	while(scriptSound)
	{
		if(!oldestSound || (scriptSound->lastPlayed < oldestTime))
		{
			oldestTime = scriptSound->lastPlayed;
			oldestSound = scriptSound;
		}
		scriptSound = scriptSound->next;
	}

	if(oldestSound)
	{
		int             pos = rand() % oldestSound->numsounds;

		// play this sound
		if(!sound->streaming)
		{
			if(!oldestSound->sounds[pos].sfxHandle)
			{
				oldestSound->sounds[pos].sfxHandle = trap_S_RegisterSound(oldestSound->sounds[pos].filename, qfalse);	// FIXME: make compressed settable through the soundscript
			}
			trap_S_StartSound(org, entnum, sound->channel, oldestSound->sounds[pos].sfxHandle);
			return trap_S_GetSoundLength(oldestSound->sounds[pos].sfxHandle);
		}
		else
		{
			return trap_S_StartStreamingSound(oldestSound->sounds[pos].filename,
											  sound->looping ? oldestSound->sounds[pos].filename : NULL, entnum, sound->channel,
											  sound->attenuation);
		}
		oldestSound->lastPlayed = cg.time;
	}
	else
	{
		CG_Error("Unable to locate a valid sound for soundScript: %s\n", sound->name);
	}

	return 0;
}

void CG_AddBufferedSoundScript(soundScript_t * sound)
{
	if(cg.numbufferedSoundScripts >= MAX_BUFFERED_SOUNDSCRIPTS)
	{
		return;
	}

	cg.bufferSoundScripts[cg.numbufferedSoundScripts++] = sound;

	if(cg.numbufferedSoundScripts == 1)
	{
		cg.bufferedSoundScriptEndTime = cg.time + CG_SoundPickOldestRandomSound(cg.bufferSoundScripts[0], NULL, -1);
	}
}

void CG_UpdateBufferedSoundScripts(void)
{
	int             i;

	if(!cg.numbufferedSoundScripts)
	{
		return;
	}

	if(cg.time > cg.bufferedSoundScriptEndTime)
	{
		for(i = 1; i < MAX_BUFFERED_SOUNDSCRIPTS; i++)
		{
			cg.bufferSoundScripts[i - 1] = cg.bufferSoundScripts[i];
		}

		cg.numbufferedSoundScripts--;

		if(!cg.numbufferedSoundScripts)
		{
			return;
		}

		cg.bufferedSoundScriptEndTime = cg.time + CG_SoundPickOldestRandomSound(cg.bufferSoundScripts[0], NULL, -1);
	}
}

/*
==============
CG_SoundPlaySoundScript

  returns qtrue is a script is found
==============
*/
int CG_SoundPlaySoundScript(const char *name, vec3_t org, int entnum, qboolean buffer)
{
	long            hash;
	char           *s;
	soundScript_t  *sound;

	if(!name || !name[0])
	{
		return qfalse;
	}

	hash = generateHashValue(name);

	s = (char *)name;
	sound = hashTable[hash];
	while(sound)
	{
		if(!Q_stricmp(s, sound->name))
		{
			if(buffer)
			{
				CG_AddBufferedSoundScript(sound);
				return 1;

			}
			else
			{
				// found a match, pick the oldest sound
				return CG_SoundPickOldestRandomSound(sound, org, entnum);
			}
		}
		sound = sound->nextHash;
	}

	CG_Printf(S_COLOR_YELLOW "WARNING: CG_SoundPlaySoundScript: cannot find sound script '%s'\n", name);
	return 0;
}

/*
==============
CG_SoundPlayIndexedScript

  returns qtrue is a script is found
==============
*/
void CG_SoundPlayIndexedScript(int index, vec3_t org, int entnum)
{
	soundScript_t  *sound;

	if(!index)
	{
		return;
	}

	if(index > numSoundScripts)
	{
		return;
	}

	sound = &soundScripts[index - 1];
	// pick the oldest sound
	CG_SoundPickOldestRandomSound(sound, org, entnum);
}

/*
===============
CG_SoundParseSounds
===============
*/
static void CG_SoundParseSounds(char *filename, char *buffer)
{
	char           *token, **text;
	int             s;
	long            hash;
	soundScript_t   sound;		// the current sound being read
	soundScriptSound_t *scriptSound = NULL;
	qboolean        inSound, wantSoundName;

	memset(&sound, 0, sizeof(sound));

	s = 0;
	inSound = qfalse;
	wantSoundName = qtrue;
	text = &buffer;

	while(1)
	{
		token = COM_ParseExt(text, qtrue);
		if(!*token)
		{
			if(inSound)
			{
				CG_Error("no concluding '}' in sound %s, file %s\n", sound.name, filename);
			}
			return;
		}

		if(!Q_stricmp(token, "{"))
		{
			if(inSound)
			{
				CG_Error("no concluding '}' in sound %s, file %s\n", sound.name, filename);
			}
			if(wantSoundName)
			{
				CG_Error("'{' found but not expected, after %s, file %s\n", sound.name, filename);
			}
			inSound = qtrue;

			// grab a free scriptSound
			scriptSound = &soundScriptSounds[numSoundScriptSounds++];

			if(numSoundScripts == MAX_SOUND_SCRIPT_SOUNDS)
			{
				CG_Error("MAX_SOUND_SCRIPT_SOUNDS exceeded.\nReduce number of sound scripts.\n");
			}

			scriptSound->lastPlayed = 0;
			scriptSound->next = sound.soundList;
			scriptSound->numsounds = 0;
			sound.soundList = scriptSound;

			continue;
		}

		if(!Q_stricmp(token, "}"))
		{
			if(!inSound)
			{
				CG_Error("'}' unexpected after sound %s, file %s\n", sound.name, filename);
			}

			// end of a sound, copy it to the global list and stick it in the hashTable
			hash = generateHashValue(sound.name);
			sound.nextHash = hashTable[hash];
			soundScripts[numSoundScripts] = sound;
			hashTable[hash] = &soundScripts[numSoundScripts++];

			if(numSoundScripts == MAX_SOUND_SCRIPTS)
			{
				CG_Error("MAX_SOUND_SCRIPTS exceeded.\nReduce number of sound scripts.\n");
			}

			inSound = qfalse;
			wantSoundName = qtrue;

			CG_SoundScriptPrecache(sound.name);

			continue;
		}

		if(!inSound)
		{
			// this is the identifier for a new sound
			if(!wantSoundName)
			{
				CG_Error("'%s' unexpected after sound %s, file %s\n", token, sound.name, filename);
			}

			Q_strncpyz(sound.name, token, sizeof(sound.name));
			wantSoundName = qfalse;
			sound.index = numSoundScripts;
			// setup the new sound defaults
			sound.channel = CHAN_AUTO;
			sound.attenuation = 1;	// default to fade away with distance (for streaming sounds)
			//
			continue;
		}

		// we are inside a sound script

		if(!Q_stricmp(token, "channel"))
		{
			// ignore this now, just look for the channel identifiers explicitly
			continue;
		}
		if(!Q_stricmp(token, "local"))
		{
			sound.channel = CHAN_LOCAL;
			continue;
		}
		else if(!Q_stricmp(token, "announcer"))
		{
			sound.channel = CHAN_ANNOUNCER;
			continue;
		}
		else if(!Q_stricmp(token, "body"))
		{
			sound.channel = CHAN_BODY;
			continue;
		}
		else if(!Q_stricmp(token, "voice"))
		{
			sound.channel = CHAN_VOICE;
			continue;
		}
		else if(!Q_stricmp(token, "weapon"))
		{
			sound.channel = CHAN_WEAPON;
			continue;
		}
		else if(!Q_stricmp(token, "item"))
		{
			sound.channel = CHAN_ITEM;
			continue;
		}
		else if(!Q_stricmp(token, "auto"))
		{
			sound.channel = CHAN_AUTO;
			continue;
		}
		if(!Q_stricmp(token, "global"))
		{
			sound.attenuation = 0;
			continue;
		}
		if(!Q_stricmp(token, "streaming"))
		{
			sound.streaming = qtrue;
			continue;
		}
		if(!Q_stricmp(token, "looping"))
		{
			sound.looping = qtrue;
			continue;
		}
		if(!Q_stricmp(token, "sound"))
		{

			if(scriptSound->numsounds >= MAX_SOUNDSCRIPT_SOUNDS)
			{
				CG_Error("Too many sounds for soundscript %s\n", sound.name);
			}

			token = COM_ParseExt(text, qtrue);


			Q_strncpyz(scriptSound->sounds[scriptSound->numsounds].filename, token, sizeof(scriptSound->sounds[0].filename));
			scriptSound->numsounds++;

			continue;
		}
	}
}

/*
===============
CG_SoundLoadSoundFiles
===============
*/
extern char     bigTextBuffer[100000];	// we got it anyway, might as well use it

#define MAX_SOUND_FILES     128
static void CG_SoundLoadSoundFiles(void)
{
	char            soundFiles[MAX_SOUND_FILES][MAX_QPATH];
	char           *text;
	char            filename[MAX_QPATH];
	fileHandle_t    f;
	int             numSounds;
	int             i, len;
	char           *token;

	// scan for sound files
	Com_sprintf(filename, MAX_QPATH, "sound/scripts/filelist.txt");
	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(len <= 0)
	{
		CG_Printf(S_COLOR_RED "WARNING: no sound files found (filelist.txt not found in sound/scripts)\n");
		return;
	}
	if(len > sizeof(bigTextBuffer))
	{
		CG_Error("%s is too big, make it smaller (max = %i bytes)\n", filename, (int)sizeof(bigTextBuffer));
	}
	// load the file into memory
	trap_FS_Read(bigTextBuffer, len, f);
	bigTextBuffer[len] = 0;
	trap_FS_FCloseFile(f);
	// parse the list
	text = bigTextBuffer;
	numSounds = 0;
	while(1)
	{
		token = COM_ParseExt(&text, qtrue);
		if(!token[0])
		{
			break;
		}
		Q_strncpyz(soundFiles[numSounds++], token, MAX_QPATH);
	}

	// add the map specific soundfile
	Com_sprintf(soundFiles[numSounds++], MAX_QPATH, "%s.sounds", cgs.rawmapname);

	if(!numSounds)
	{
		CG_Printf(S_COLOR_RED "WARNING: no sound files found\n");
		return;
	}

	// load and parse sound files
	for(i = 0; i < numSounds; i++)
	{
		Com_sprintf(filename, sizeof(filename), "sound/scripts/%s", soundFiles[i]);
		CG_Printf("...loading '%s'\n", filename);
		len = trap_FS_FOpenFile(filename, &f, FS_READ);
		if(len <= 0)
		{
			if(i != (numSounds - 1))
			{
				CG_Error("Couldn't load %s", filename);
			}
			continue;
		}
		if(len > sizeof(bigTextBuffer))
		{
			CG_Error("%s is too big, make it smaller (max = %i bytes)\n", filename, (int)sizeof(bigTextBuffer));
		}
		memset(bigTextBuffer, 0, sizeof(bigTextBuffer));
		trap_FS_Read(bigTextBuffer, len, f);
		trap_FS_FCloseFile(f);
		CG_SoundParseSounds(filename, bigTextBuffer);
	}
}

/*
==============
CG_SoundInit
==============
*/
void CG_SoundInit(void)
{

	if(numSoundScripts)
	{
		// keep all the information, just reset the vars
		int             i, j;

		for(i = 0; i < numSoundScriptSounds; i++)
		{
			soundScriptSounds[i].lastPlayed = 0;

			for(j = 0; j < soundScriptSounds[i].numsounds; j++)
			{
				soundScriptSounds[i].sounds[j].sfxHandle = 0;
			}
		}
	}
	else
	{
		CG_Printf("\n.........................\n" "Initializing Sound Scripts\n");
		CG_SoundLoadSoundFiles();
		CG_Printf("done.\n");
	}

}

//
// Script Speakers
//

// Editing

typedef struct editHandle_s
{
	vec3_t          origin;
	vec3_t          oldOrigin;

	int             activeAxis;
} editHandle_t;

static qhandle_t speakerShader = 0;
static qhandle_t speakerShaderGrayScale = 0;
static bg_speaker_t *editSpeaker = NULL;
static bg_speaker_t undoSpeaker;
static int      undoSpeakerIndex;
static qboolean editSpeakerActive = qfalse;
static editHandle_t editSpeakerHandle;
static int      numSpeakersInPvs;

static const char *s_lt_string[] = {
	"no",
	"on",
	"off"
};

static const char *s_bt_string[] = {
	"no",
	"global",
	"nopvs"
};

qboolean CG_SaveSpeakersToScript(void)
{
	int             i;
	bg_speaker_t   *speaker;
	fileHandle_t    fh;
	char           *s;

	if(trap_FS_FOpenFile(va("sound/maps/%s.sps", cgs.rawmapname), &fh, FS_WRITE) < 0)
	{
		CG_Printf(S_COLOR_RED "ERROR: failed to save speakers to 'sound/maps/%s.sps'\n", cgs.rawmapname);
		return qfalse;
	}

	s = "speakerScript\n{";
	trap_FS_Write(s, strlen(s), fh);

	for(i = 0; i < BG_NumScriptSpeakers(); i++)
	{
		char            filenameStr[96] = "";
		char            originStr[96];
		char            targetnameStr[56] = "";
		char            loopedStr[32];
		char            broadcastStr[32];
		char            waitStr[32] = "";
		char            randomStr[32] = "";
		char            volumeStr[32] = "";
		char            rangeStr[32] = "";

		speaker = BG_GetScriptSpeaker(i);

		if(*speaker->filename)
		{
			Com_sprintf(filenameStr, sizeof(filenameStr), "\t\tnoise \"%s\"\n", speaker->filename);
		}

		Com_sprintf(originStr, sizeof(originStr), "\t\torigin %.2f %.2f %.2f\n", speaker->origin[0], speaker->origin[1],
					speaker->origin[2]);

		if(*speaker->targetname)
		{
			Com_sprintf(targetnameStr, sizeof(targetnameStr), "\t\ttargetname \"%s\"\n", speaker->targetname);
		}

		Com_sprintf(loopedStr, sizeof(loopedStr), "\t\tlooped \"%s\"\n", s_lt_string[speaker->loop]);

		Com_sprintf(broadcastStr, sizeof(broadcastStr), "\t\tbroadcast \"%s\"\n", s_bt_string[speaker->broadcast]);

		if(speaker->wait)
		{
			Com_sprintf(waitStr, sizeof(waitStr), "\t\twait %i\n", speaker->wait);
		}

		if(speaker->random)
		{
			Com_sprintf(randomStr, sizeof(randomStr), "\t\trandom %i\n", speaker->random);
		}

		if(speaker->volume)
		{
			Com_sprintf(volumeStr, sizeof(volumeStr), "\t\tvolume %i\n", speaker->volume);
		}

		if(speaker->range)
		{
			Com_sprintf(rangeStr, sizeof(rangeStr), "\t\trange %i\n", speaker->range);
		}

		s = va("\n\tspeakerDef {\n%s%s%s%s%s%s%s%s%s\t}\n",
			   filenameStr, originStr, targetnameStr, loopedStr, broadcastStr, waitStr, randomStr, volumeStr, rangeStr);

		trap_FS_Write(s, strlen(s), fh);
	}

	s = "}\n";
	trap_FS_Write(s, strlen(s), fh);

	trap_FS_FCloseFile(fh);

	CG_Printf("Saved %i speakers to 'sound/maps/%s.sps'\n", BG_NumScriptSpeakers(), cgs.rawmapname);

	return qtrue;
}

void CG_AddLineToScene(vec3_t start, vec3_t end, vec4_t colour)
{
	refEntity_t     re;

	memset(&re, 0, sizeof(re));
	re.reType = RT_RAIL_CORE;
	re.customShader = cgs.media.railCoreShader;
	VectorCopy(start, re.origin);
	VectorCopy(end, re.oldorigin);
	re.shaderRGBA[0] = colour[0] * 0xff;
	re.shaderRGBA[1] = colour[1] * 0xff;
	re.shaderRGBA[2] = colour[2] * 0xff;
	re.shaderRGBA[3] = colour[3] * 0xff;

	trap_R_AddRefEntityToScene(&re);
}

void CG_SetViewanglesForSpeakerEditor(void)
{
	vec3_t          vec;

	if(!editSpeakerActive)
	{
		return;
	}

	VectorSubtract(editSpeakerHandle.origin, cg.refdef_current->vieworg, vec);
	vectoangles(vec, cg.refdefViewAngles);
}

static void CG_RenderScriptSpeakers(void)
{
	int             i, j, closest;
	float           dist, minDist;
	vec3_t          vec;
	refEntity_t     re;
	bg_speaker_t   *speaker;

	closest = -1;
	minDist = Square(8.f);

	/*{
	   float    r, u;
	   vec3_t   axisOrg, dir;

	   closest = -1;
	   minDist = Square( 32.f );

	   r = -(cg.refdef_current->fov_x / 90.f) * (float)(cgs.cursorX - 320) / 320;
	   u = -(cg.refdef_current->fov_y / 90.f) * (float)(cgs.cursorY - 240) / 240;

	   for( i = 0; i < 3; i++ ) {
	   dir[i] = cg.refdef_current->viewaxis[0][i] * 1.f +
	   cg.refdef_current->viewaxis[1][i] * r +
	   cg.refdef_current->viewaxis[2][i] * u;
	   }
	   VectorNormalizeFast( dir );

	   VectorMA( cg.refdef_current->vieworg, 512, dir, vec );
	   //VectorCopy( cg.refdef_current->vieworg, axisOrg );
	   //axisOrg[2]+=.1f;
	   VectorMA( cg.refdef_current->vieworg, .1f, cg.refdef_current->viewaxis[0], axisOrg );
	   CG_AddLineToScene( vec, axisOrg, colorOrange );

	   } */

	numSpeakersInPvs = 0;

	for(i = 0; i < BG_NumScriptSpeakers(); i++)
	{
		speaker = BG_GetScriptSpeaker(i);

		if(editSpeakerActive && editSpeaker == speaker)
		{
			vec4_t          colour;

			for(j = 0; j < 3; j++)
			{
				VectorClear(colour);
				colour[3] = 1.f;
				if(editSpeakerHandle.activeAxis >= 0)
				{
					if(editSpeakerHandle.activeAxis == j)
					{
						colour[j] = 1.f;
					}
					else
					{
						colour[j] = .3f;
					}
				}
				else
				{
					colour[j] = 1.f;
				}
				VectorClear(vec);
				vec[j] = 1.f;
				VectorMA(editSpeakerHandle.origin, 32, vec, vec);
				CG_AddLineToScene(editSpeakerHandle.origin, vec, colour);

				memset(&re, 0, sizeof(re));
				re.reType = RT_SPRITE;
				VectorCopy(vec, re.origin);
				VectorCopy(vec, re.oldorigin);
				re.radius = 3;
				re.customShader = cgs.media.waterBubbleShader;
				re.shaderRGBA[0] = colour[0] * 0xff;
				re.shaderRGBA[1] = colour[1] * 0xff;
				re.shaderRGBA[2] = colour[2] * 0xff;
				re.shaderRGBA[3] = colour[3] * 0xff;
				trap_R_AddRefEntityToScene(&re);
			}

			if(trap_R_inPVS(cg.refdef_current->vieworg, speaker->origin))
			{
				numSpeakersInPvs++;
			}
		}
		else if(!trap_R_inPVS(cg.refdef_current->vieworg, speaker->origin))
		{
			continue;
		}
		else
		{
			numSpeakersInPvs++;
		}

		memset(&re, 0, sizeof(re));
		re.reType = RT_SPRITE;
		VectorCopy(speaker->origin, re.origin);
		VectorCopy(speaker->origin, re.oldorigin);
		re.radius = 8;
		re.customShader = speakerShader;

		if(editSpeaker)
		{
			re.customShader = speakerShaderGrayScale;

			if(editSpeaker == speaker)
			{
				re.shaderRGBA[0] = 0xff;
				re.shaderRGBA[1] = 0xaa;
				re.shaderRGBA[2] = 0xaa;
				re.shaderRGBA[3] = 0xff;
			}
			else
			{
				re.shaderRGBA[0] = 0x3f;
				re.shaderRGBA[1] = 0x3f;
				re.shaderRGBA[2] = 0x3f;
				re.shaderRGBA[3] = 0xff;
			}
		}
		else
		{
			re.customShader = speakerShader;

			re.shaderRGBA[0] = 0xff;
			re.shaderRGBA[1] = 0xff;
			re.shaderRGBA[2] = 0xff;
			re.shaderRGBA[3] = 0xff;
		}
		trap_R_AddRefEntityToScene(&re);

		// see which one is closest to our cursor
		if(!editSpeakerActive)
		{
			VectorSubtract(speaker->origin, cg.refdef_current->vieworg, vec);
			dist = DotProduct(vec, cg.refdef_current->viewaxis[0]);
			VectorMA(cg.refdef_current->vieworg, dist, cg.refdef_current->viewaxis[0], vec);
			VectorSubtract(speaker->origin, vec, vec);
			dist = VectorLengthSquared(vec);
			if(dist <= minDist)
			{
				minDist = dist;
				closest = i;
			}
		}
	}

	if(!editSpeakerActive)
	{
		if(closest >= 0)
		{
			editSpeaker = BG_GetScriptSpeaker(closest);
		}
		else
		{
			editSpeaker = NULL;
		}
	}
}

void CG_SpeakerInfo_Text(panel_button_t * button)
{
	char           *s, *ptr, *strptr;
	float           y, wMax, w, h;
	vec4_t          colour;
	char            originStr[96];
	char            filenameStr[96] = "";
	char            targetnameStr[56] = "";
	char            loopedStr[32];
	char            broadcastStr[32];
	char            waitStr[32] = "";
	char            randomStr[32] = "";
	char            volumeStr[32] = "";
	char            rangeStr[32] = "";

	if(!button->font)
	{
		return;
	}

	Com_sprintf(originStr, sizeof(originStr), "Speaker at %.2f %.2f %.2f\n", editSpeaker->origin[0], editSpeaker->origin[1],
				editSpeaker->origin[2]);
	wMax = CG_Text_Width_Ext(originStr, button->font->scalex, 0, button->font->font);
	h = 8.5f;

	if(*editSpeaker->filename)
	{
		Com_sprintf(filenameStr, sizeof(filenameStr), "noise: %s\n", editSpeaker->filename);
		w = CG_Text_Width_Ext(filenameStr, button->font->scalex, 0, button->font->font);
		if(w > wMax)
		{
			wMax = w;
		}
		h += 8.5f;
	}

	if(*editSpeaker->targetname)
	{
		Com_sprintf(targetnameStr, sizeof(targetnameStr), "targetname: %s\n", editSpeaker->targetname);
		w = CG_Text_Width_Ext(targetnameStr, button->font->scalex, 0, button->font->font);
		if(w > wMax)
		{
			wMax = w;
		}
		h += 8.5f;
	}

	Com_sprintf(loopedStr, sizeof(loopedStr), "looped: %s\n", s_lt_string[editSpeaker->loop]);
	w = CG_Text_Width_Ext(loopedStr, button->font->scalex, 0, button->font->font);
	if(w > wMax)
	{
		wMax = w;
	}
	h += 8.5f;

	Com_sprintf(broadcastStr, sizeof(broadcastStr), "broadcast: %s\n", s_bt_string[editSpeaker->broadcast]);
	w = CG_Text_Width_Ext(broadcastStr, button->font->scalex, 0, button->font->font);
	if(w > wMax)
	{
		wMax = w;
	}
	h += 8.5f;

	if(editSpeaker->wait)
	{
		Com_sprintf(waitStr, sizeof(waitStr), "wait: %i\n", editSpeaker->wait);
		w = CG_Text_Width_Ext(waitStr, button->font->scalex, 0, button->font->font);
		if(w > wMax)
		{
			wMax = w;
		}
		h += 8.5f;
	}

	if(editSpeaker->random)
	{
		Com_sprintf(randomStr, sizeof(randomStr), "random: %i\n", editSpeaker->random);
		w = CG_Text_Width_Ext(randomStr, button->font->scalex, 0, button->font->font);
		if(w > wMax)
		{
			wMax = w;
		}
		h += 8.5f;
	}

	if(editSpeaker->volume)
	{
		Com_sprintf(volumeStr, sizeof(volumeStr), "volume: %i\n", editSpeaker->volume);
		w = CG_Text_Width_Ext(volumeStr, button->font->scalex, 0, button->font->font);
		if(w > wMax)
		{
			wMax = w;
		}
		h += 8.5f;
	}

	if(editSpeaker->range)
	{
		Com_sprintf(rangeStr, sizeof(rangeStr), "range: %i\n", editSpeaker->range);
		w = CG_Text_Width_Ext(rangeStr, button->font->scalex, 0, button->font->font);
		if(w > wMax)
		{
			wMax = w;
		}
		h += 8.5f;
	}

	VectorCopy(colorMdBlue, colour);
	colour[3] = .5f;
	CG_FillRect(button->rect.x - 2, button->rect.y - 2, wMax + 4, h + 4, colour);
	VectorCopy(colorBlue, colour);
	CG_DrawRect(button->rect.x - 2, button->rect.y - 2, wMax + 4, h + 4, 1.f, colour);

	s = va("%s%s%s%s%s%s%s%s%s",
		   originStr, filenameStr, targetnameStr, loopedStr, broadcastStr, waitStr, randomStr, volumeStr, rangeStr);

	y = button->rect.y + 8;

	for(strptr = ptr = s; *ptr; ptr++)
	{
		if(*ptr == '\n')
		{
			*ptr = '\0';
			CG_Text_Paint_Ext(button->rect.x, y, button->font->scalex, button->font->scaley, button->font->colour, strptr, 0, 0,
							  button->font->style, button->font->font);
			y += 8;
			strptr = ptr + 1;
		}
	}
}

panel_button_text_t speakerEditorTxt = {
	0.2f, 0.2f,
	{1.0f, 1.0f, 1.0f, 0.5f}
	,
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

panel_button_t  speakerInfo = {
	NULL,
	NULL,
	{344, 184, 272, 72}
	,
	{0, 0, 0, 0, 0, 0, 0, 0}
	,
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerInfo_Text,
	NULL,
};

static panel_button_t *speakerInfoButtons[] = {
	&speakerInfo,
	NULL
};


void CG_SpeakerEditor_RenderEdit(panel_button_t * button)
{
	vec4_t          colour;

	if(button == BG_PanelButtons_GetFocusButton())
	{
		VectorCopy(colorYellow, colour);
		colour[3] = .3f;
	}
	else
	{
		VectorCopy(colorWhite, colour);
		colour[3] = .3f;
	}
	CG_FillRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, colour);

	button->rect.x += 2.f;
	button->rect.h -= 3.f;
	BG_PanelButton_RenderEdit(button);
	button->rect.x -= 2.f;
	button->rect.h += 3.f;
}

void CG_SpeakerEditor_RenderButton(panel_button_t * button)
{
	vec4_t          colour;
	float           oldX;

	if(button == BG_PanelButtons_GetFocusButton())
	{
		VectorCopy(colorMdBlue, colour);
		colour[3] = .5f;
	}
	else if(BG_PanelButtons_GetFocusButton() == NULL && BG_CursorInRect(&button->rect))
	{
		VectorCopy(colorWhite, colour);
		colour[3] = .5f;
	}
	else
	{
		VectorCopy(colorWhite, colour);
		colour[3] = .3f;
	}
	CG_FillRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, colour);

	VectorCopy(colorBlue, colour);
	CG_DrawRect(button->rect.x, button->rect.y, button->rect.w, button->rect.h, 1.f, colour);

	oldX = button->rect.x;
	button->rect.x =
		button->rect.x + (button->rect.w - CG_Text_Width_Ext(button->text, button->font->scalex, 0, button->font->font)) / 2.f;
	button->rect.y += 9.f;
	BG_PanelButtonsRender_Text(button);
	button->rect.x = oldX;
	button->rect.y -= 9.f;
}

char           *CG_GetStrFromStrArray(const char *in, const int index)
{
	char           *ptr, *s;
	int             i;

	s = ptr = (char *)in;
	i = 0;
	while(1)
	{
		if(i == index)
		{
			return s;
		}

		while(*ptr)
		{
			ptr++;
		}
		ptr++;

		s = ptr;
		i++;
	}
}

void CG_SpeakerEditor_RenderDropdown(panel_button_t * button)
{
	vec4_t          colour;
	float           textboxW;
	rectDef_t       rect;
	int             i;
	char           *s;

	memcpy(&rect, &button->rect, sizeof(rect));

	textboxW = button->rect.w - button->rect.h;
	rect.x += textboxW;
	rect.w = rect.h;

	if(button == BG_PanelButtons_GetFocusButton())
	{
		VectorCopy(colorYellow, colour);
		colour[3] = .3f;
	}
	else
	{
		VectorCopy(colorWhite, colour);
		colour[3] = .3f;
	}
	CG_FillRect(button->rect.x, button->rect.y, textboxW, button->rect.h, colour);
	VectorCopy(colorBlue, colour);
	CG_DrawRect(button->rect.x, button->rect.y, textboxW, button->rect.h, 1.f, colour);

	if(button == BG_PanelButtons_GetFocusButton())
	{
		VectorCopy(colorYellow, colour);
		colour[3] = .3f;
	}
	else if(BG_PanelButtons_GetFocusButton() == NULL && BG_CursorInRect(&button->rect))
	{
		VectorCopy(colorWhite, colour);
		colour[3] = .5f;
	}
	else
	{
		VectorCopy(colorWhite, colour);
		colour[3] = .3f;
	}

	CG_FillRect(rect.x, rect.y, rect.w, rect.h, colour);
	VectorCopy(colorBlue, colour);
	CG_DrawRect(rect.x, rect.y, rect.w, rect.h, 1.f, colour);

	VectorCopy(button->font->colour, colour);
	CG_Text_Paint_Ext(rect.x + (rect.w - CG_Text_Width_Ext("V", button->font->scalex, 0, button->font->font)) / 2.f,
					  button->rect.y + 9.f, button->font->scalex, button->font->scaley, colour, "V", 0, 0, 0, button->font->font);

	s = CG_GetStrFromStrArray(button->text, button->data[1]);

	CG_Text_Paint_Ext(button->rect.x + (textboxW - CG_Text_Width_Ext(s, button->font->scalex, 0, button->font->font)) / 2.f,
					  button->rect.y + 9.f,
					  button->font->scalex,
					  button->font->scaley, button->font->colour, s, 0, 0, button->font->style, button->font->font);

	if(button == BG_PanelButtons_GetFocusButton())
	{
		memcpy(&rect, &button->rect, sizeof(rect));

		for(i = 0; i < button->data[0]; i++)
		{
			if(i == button->data[1])
			{
				continue;
			}

			rect.y += 12.f;

			if(BG_CursorInRect(&rect))
			{
				VectorScale(colorYellow, .3f, colour);
				colour[3] = 1.f;
			}
			else
			{
				VectorScale(colorWhite, .3f, colour);
				colour[3] = 1.f;
			}

			CG_FillRect(rect.x, rect.y, rect.w, rect.h, colour);

			s = CG_GetStrFromStrArray(button->text, i);

			CG_Text_Paint_Ext(rect.x + (textboxW - CG_Text_Width_Ext(s, button->font->scalex, 0, button->font->font)) / 2.f,
							  rect.y + 9.f,
							  button->font->scalex,
							  button->font->scaley, button->font->colour, s, 0, 0, button->font->style, button->font->font);
		}

		VectorCopy(colorBlue, colour);
		colour[3] = .3f;
		CG_DrawRect(button->rect.x, button->rect.y + 12.f, button->rect.w, rect.y - button->rect.y, 1.f, colour);
	}
}

void CG_SpeakerEditor_Back(panel_button_t * button)
{
	vec4_t          colour;

	VectorCopy(colorMdBlue, colour);
	colour[3] = .5f;
	CG_FillRect(button->rect.x - 2, button->rect.y - 2, button->rect.w + 4, button->rect.h + 4, colour);
	VectorCopy(colorBlue, colour);
	CG_DrawRect(button->rect.x - 2, button->rect.y - 2, button->rect.w + 4, button->rect.h + 4, 1.f, colour);
}

void CG_SpeakerEditor_LocInfo(panel_button_t * button)
{
	CG_Text_Paint_Ext(button->rect.x, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour,
					  va("Speaker at %.2f %.2f %.2f", editSpeaker->origin[0], editSpeaker->origin[1], editSpeaker->origin[2]),
					  0, 0, button->font->style, button->font->font);
}

static char     noiseMatchString[MAX_QPATH];
static int      noiseMatchCount;
static int      noiseMatchIndex;

qboolean CG_SpeakerEditor_NoiseEdit_KeyDown(panel_button_t * button, int key)
{
	if(button == BG_PanelButtons_GetFocusButton())
	{
		if(key == K_TAB)
		{
			char            dirname[MAX_QPATH];
			char            filename[MAX_QPATH];
			char            match[MAX_QPATH];
			int             i, numfiles, filelen;
			char           *fileptr;

			COM_StripFilename((char *)button->text, dirname);
			Q_strncpyz(filename, COM_SkipPath((char *)button->text), sizeof(filename));

			if(!Q_stricmp(button->text, dirname))
			{
				return qtrue;
			}

			numfiles = trap_FS_GetFileList(dirname, "", bigTextBuffer, sizeof(bigTextBuffer));
			// FIXME: autocomplete directories?

			fileptr = bigTextBuffer;

			if(!*noiseMatchString || Q_stricmpn(noiseMatchString, filename, strlen(noiseMatchString)))
			{
				Q_strncpyz(noiseMatchString, filename, sizeof(noiseMatchString));
				noiseMatchCount = 0;
				noiseMatchIndex = 0;

				for(i = 0; i < numfiles; i++, fileptr += filelen + 1)
				{
					filelen = strlen(fileptr);
					if(Q_stricmpn(fileptr, filename, strlen(filename)))
					{
						continue;
					}

					noiseMatchCount++;
					if(noiseMatchCount == 1)
					{
						Q_strncpyz(match, fileptr, sizeof(match));
						continue;
					}

					/*if( strlen(fileptr) < strlen(match) ) {
					   Q_strncpyz( match, fileptr, sizeof(match) );
					   noiseMatchIndex++;
					   continue;
					   } */
				}
			}
			else
			{
				if(noiseMatchCount == 1)
				{
					return qtrue;
				}
				else
				{
					int             findMatchIndex = 0;

					noiseMatchIndex++;
					if(noiseMatchIndex == noiseMatchCount)
					{
						noiseMatchIndex = 0;
					}

					for(i = 0; i < numfiles; i++, fileptr += filelen + 1)
					{
						filelen = strlen(fileptr);
						if(Q_stricmpn(fileptr, noiseMatchString, strlen(noiseMatchString)))
						{
							continue;
						}

						if(findMatchIndex == noiseMatchIndex)
						{
							Q_strncpyz(match, fileptr, sizeof(match));
							break;
						}

						findMatchIndex++;
					}
				}
			}

			if(noiseMatchCount == 0)
			{
				noiseMatchString[0] = '\0';
				return qtrue;
			}

			Com_sprintf((char *)button->text, button->data[0], "%s%s", dirname, match);

			return qtrue;
		}
		else
		{
			if(key & K_CHAR_FLAG)
			{
				int             localkey = key;

				localkey &= ~K_CHAR_FLAG;
				if(localkey == 'h' - 'a' + 1 || localkey >= 32)
				{
					noiseMatchString[0] = '\0';
				}
			}
		}
	}

	return BG_PanelButton_EditClick(button, key);
}

void CG_SpeakerEditor_NoiseEditFinish(panel_button_t * button)
{
	Q_strncpyz(editSpeaker->filename, button->text, sizeof(editSpeaker->filename));

	if(*editSpeaker->filename)
	{
		editSpeaker->noise = trap_S_RegisterSound(editSpeaker->filename, qfalse);
	}
	else
	{
		editSpeaker->noise = 0;
	}
}

void CG_SpeakerEditor_TargetnameEditFinish(panel_button_t * button)
{
	Q_strncpyz(editSpeaker->targetname, button->text, sizeof(editSpeaker->targetname));
}

qboolean CG_SpeakerEditor_Dropdown_KeyDown(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

qboolean CG_SpeakerEditor_Looped_KeyUp(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		if(button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t       rect;
			int             i;

			memcpy(&rect, &button->rect, sizeof(rect));

			for(i = 0; i < 3; i++)
			{
				if(i == editSpeaker->loop)
				{
					continue;
				}

				rect.y += 12.f;

				if(BG_CursorInRect(&rect))
				{
					button->data[1] = editSpeaker->loop = i;
					break;
				}
			}

			if(editSpeaker->loop == S_LT_LOOPED_ON)
			{
				editSpeaker->activated = qtrue;
			}
			else
			{
				editSpeaker->activated = qfalse;
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

qboolean CG_SpeakerEditor_Broadcast_KeyUp(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		if(button == BG_PanelButtons_GetFocusButton())
		{
			rectDef_t       rect;
			int             i;

			memcpy(&rect, &button->rect, sizeof(rect));

			for(i = 0; i < 3; i++)
			{
				if(i == editSpeaker->broadcast)
				{
					continue;
				}

				rect.y += 12.f;

				if(BG_CursorInRect(&rect))
				{
					button->data[1] = editSpeaker->broadcast = i;
					break;
				}
			}

			BG_PanelButtons_SetFocusButton(NULL);

			return qtrue;
		}
	}

	return qfalse;
}

void CG_SpeakerEditor_WaitEditFinish(panel_button_t * button)
{
	if(*button->text)
	{
		editSpeaker->wait = atoi(button->text);
		if(editSpeaker->wait < 0)
		{
			editSpeaker->wait = 0;
			Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->range);
		}
	}
	else
	{
		editSpeaker->wait = 0;
		Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->wait);
	}
}

void CG_SpeakerEditor_RandomEditFinish(panel_button_t * button)
{
	if(*button->text)
	{
		editSpeaker->random = atoi(button->text);
		if(editSpeaker->random < 0)
		{
			editSpeaker->random = 0;
			Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->random);
		}
	}
	else
	{
		editSpeaker->random = 0;
		Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->random);
	}
}

void CG_SpeakerEditor_VolumeEditFinish(panel_button_t * button)
{
	if(*button->text)
	{
		editSpeaker->volume = atoi(button->text);
		if(editSpeaker->volume < 0)
		{
			editSpeaker->volume = 0;
			Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->volume);
		}
		else if(editSpeaker->volume > 65535)
		{
			editSpeaker->volume = 65535;
			Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->volume);
		}
	}
	else
	{
		editSpeaker->volume = 127;
		Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->volume);
	}
}

void CG_SpeakerEditor_RangeEditFinish(panel_button_t * button)
{
	if(*button->text)
	{
		editSpeaker->range = atoi(button->text);
		if(editSpeaker->range < 0)
		{
			editSpeaker->range = 0;
			Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->range);
		}
	}
	else
	{
		editSpeaker->range = 1250;
		Com_sprintf((char *)button->text, sizeof(button->text), "%i", editSpeaker->range);
	}
}

qboolean CG_SpeakerEditor_Ok_KeyDown(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

qboolean CG_SpeakerEditor_Ok_KeyUp(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		if(button == BG_PanelButtons_GetFocusButton())
		{
			BG_PanelButtons_SetFocusButton(NULL);

			if(BG_CursorInRect(&button->rect))
			{
				CG_SaveSpeakersToScript();

				editSpeakerActive = qfalse;
				CG_EventHandling(-CGAME_EVENT_SPEAKEREDITOR, qtrue);
			}
			return qtrue;
		}
	}

	return qfalse;
}

qboolean CG_SpeakerEditor_Cancel_KeyDown(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

qboolean CG_SpeakerEditor_Cancel_KeyUp(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		if(button == BG_PanelButtons_GetFocusButton())
		{
			BG_PanelButtons_SetFocusButton(NULL);

			if(BG_CursorInRect(&button->rect))
			{
				memcpy(editSpeaker, &undoSpeaker, sizeof(*editSpeaker));
				undoSpeakerIndex = -2;
				editSpeaker = NULL;
				editSpeakerActive = qfalse;
				CG_EventHandling(-CGAME_EVENT_SPEAKEREDITOR, qtrue);
			}
			return qtrue;
		}
	}

	return qfalse;
}

qboolean CG_SpeakerEditor_Delete_KeyDown(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

qboolean CG_SpeakerEditor_Delete_KeyUp(panel_button_t * button, int key)
{
	if(key == K_MOUSE1)
	{
		if(button == BG_PanelButtons_GetFocusButton())
		{
			BG_PanelButtons_SetFocusButton(NULL);

			if(BG_CursorInRect(&button->rect))
			{
				undoSpeakerIndex = -1;
				BG_SS_DeleteSpeaker(BG_GetIndexForSpeaker(editSpeaker));
				CG_SaveSpeakersToScript();
				editSpeaker = NULL;
				editSpeakerActive = qfalse;
				CG_EventHandling(-CGAME_EVENT_SPEAKEREDITOR, qtrue);
			}
			return qtrue;
		}
	}

	return qfalse;
}

panel_button_t  speakerEditorBack = {
	NULL,
	NULL,
	{360, 330, 272, 142}
	,
	{0, 0, 0, 0, 0, 0, 0, 0}
	,
	NULL,						/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerEditor_Back,
	NULL,
};

panel_button_t  speakerEditorLocInfo = {
	NULL,
	NULL,
	{361, 330 + 9, 272, 10}
	,
	{0, 0, 0, 0, 0, 0, 0, 0}
	,
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerEditor_LocInfo,
	NULL,
};

panel_button_t  speakerEditorNoiseLabel = {
	NULL,
	"Noise:",
	{361, 344 + 9, 0, 0}
	,
	{0, 0, 0, 0, 0, 0, 0, 0}
	,
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

char            noiseEditBuffer[MAX_QPATH];

panel_button_t  speakerEditorNoiseEdit = {
	NULL,
	noiseEditBuffer,
	{430, 344, 200, 12},
	{MAX_QPATH, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	CG_SpeakerEditor_NoiseEdit_KeyDown,	/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerEditor_RenderEdit,
	CG_SpeakerEditor_NoiseEditFinish,
};

panel_button_t  speakerEditorTargetnameLabel = {
	NULL,
	"Targetname:",
	{361, 358 + 9, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

char            targetnameEditBuffer[32];

panel_button_t  speakerEditorTargetnameEdit = {
	NULL,
	targetnameEditBuffer,
	{430, 358, 200, 12},
	{32, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	BG_PanelButton_EditClick,	/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerEditor_RenderEdit,
	CG_SpeakerEditor_TargetnameEditFinish,
};

panel_button_t  speakerEditorLoopedLabel = {
	NULL,
	"Looped:",
	{361, 372 + 9, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t  speakerEditorLoopedDropdown = {
	NULL,
	"no\0on\0off",
	{430, 372, 60, 12},
	{3, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	CG_SpeakerEditor_Dropdown_KeyDown,	/* keyDown  */
	CG_SpeakerEditor_Looped_KeyUp,	/* keyUp    */
	CG_SpeakerEditor_RenderDropdown,
	NULL,
};

panel_button_t  speakerEditorBroadcastLabel = {
	NULL,
	"Broadcast:",
	{361, 386 + 9, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t  speakerEditorBroadcastDropdown = {
	NULL,
	"no\0global\0nopvs",
	{430, 386, 60, 12},
	{3, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	CG_SpeakerEditor_Dropdown_KeyDown,	/* keyDown  */
	CG_SpeakerEditor_Broadcast_KeyUp,	/* keyUp    */
	CG_SpeakerEditor_RenderDropdown,
	NULL,
};

panel_button_t  speakerEditorWaitLabel = {
	NULL,
	"Wait:",
	{361, 400 + 9, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

char            waitEditBuffer[12];

panel_button_t  speakerEditorWaitEdit = {
	NULL,
	waitEditBuffer,
	{430, 400, 200, 12},
	{12, 2, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	BG_PanelButton_EditClick,	/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerEditor_RenderEdit,
	CG_SpeakerEditor_WaitEditFinish,
};

panel_button_t  speakerEditorRandomLabel = {
	NULL,
	"Random:",
	{361, 414 + 9, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

char            randomEditBuffer[12];

panel_button_t  speakerEditorRandomEdit = {
	NULL,
	randomEditBuffer,
	{430, 414, 200, 12},
	{12, 2, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	BG_PanelButton_EditClick,	/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerEditor_RenderEdit,
	CG_SpeakerEditor_RandomEditFinish,
};

panel_button_t  speakerEditorVolumeLabel = {
	NULL,
	"Volume:",
	{361, 428 + 9, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

char            volumeEditBuffer[12];

panel_button_t  speakerEditorVolumeEdit = {
	NULL,
	volumeEditBuffer,
	{430, 428, 200, 12},
	{12, 2, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	BG_PanelButton_EditClick,	/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerEditor_RenderEdit,
	CG_SpeakerEditor_VolumeEditFinish,
};

panel_button_t  speakerEditorRangeLabel = {
	NULL,
	"Range:",
	{361, 442 + 9, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	NULL,						/* keyDown  */
	NULL,						/* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

char            rangeEditBuffer[12];

panel_button_t  speakerEditorRangeEdit = {
	NULL,
	rangeEditBuffer,
	{430, 442, 200, 12},
	{12, 2, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	BG_PanelButton_EditClick,	/* keyDown  */
	NULL,						/* keyUp    */
	CG_SpeakerEditor_RenderEdit,
	CG_SpeakerEditor_RangeEditFinish,
};

panel_button_t  speakerEditorOkButton = {
	NULL,
	"Ok",
	{376, 458, 70, 12},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	CG_SpeakerEditor_Ok_KeyDown,	/* keyDown  */
	CG_SpeakerEditor_Ok_KeyUp,	/* keyUp    */
	CG_SpeakerEditor_RenderButton,
	NULL,
};

panel_button_t  speakerEditorCancelButton = {
	NULL,
	"Cancel",
	{461, 458, 70, 12},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	CG_SpeakerEditor_Cancel_KeyDown,	/* keyDown  */
	CG_SpeakerEditor_Cancel_KeyUp,	/* keyUp    */
	CG_SpeakerEditor_RenderButton,
	NULL,
};

panel_button_t  speakerEditorDeleteButton = {
	NULL,
	"Delete",
	{546, 458, 70, 12},
	{0, 0, 0, 0, 0, 0, 0, 0},
	&speakerEditorTxt,			/* font     */
	CG_SpeakerEditor_Delete_KeyDown,	/* keyDown  */
	CG_SpeakerEditor_Delete_KeyUp,	/* keyUp    */
	CG_SpeakerEditor_RenderButton,
	NULL,
};

static panel_button_t *speakerEditorButtons[] = {
	&speakerEditorBack,
	&speakerEditorLocInfo,
	&speakerEditorNoiseLabel,
	&speakerEditorNoiseEdit,
	&speakerEditorTargetnameLabel,
	&speakerEditorTargetnameEdit,
	&speakerEditorLoopedLabel,
	&speakerEditorBroadcastLabel,
	&speakerEditorWaitLabel,
	&speakerEditorWaitEdit,
	&speakerEditorRandomLabel,
	&speakerEditorRandomEdit,
	&speakerEditorVolumeLabel,
	&speakerEditorVolumeEdit,
	&speakerEditorRangeLabel,
	&speakerEditorRangeEdit,
	&speakerEditorOkButton,
	&speakerEditorCancelButton,
	&speakerEditorDeleteButton,

	// Below here all components that should draw on top
	&speakerEditorBroadcastDropdown,
	&speakerEditorLoopedDropdown,
	NULL
};

void CG_SpeakerEditorDraw(void)
{
	if(!cg.editingSpeakers)
	{
		return;
	}

	if(!editSpeakerActive)
	{
		int             bindingkey[2];
		char            binding[2][32];
		vec4_t          colour;
		float           x, y, w, h;

		VectorCopy(colorWhite, colour);
		colour[3] = .8f;

		if(undoSpeakerIndex == -2)
		{
			y = 452;
		}
		else
		{
			y = 442;
		}

		CG_Text_Paint_Ext(8, y, .2f, .2f, colour,
						  va("Current amount of speakers in map: %i (inpvs: %i max in map: %i)",
							 BG_NumScriptSpeakers(),
							 numSpeakersInPvs, 256), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		trap_Key_KeysForBinding("dumpspeaker", &bindingkey[0], &bindingkey[1]);
		trap_Key_KeynumToStringBuf(bindingkey[0], binding[0], sizeof(binding[0]));
		trap_Key_KeynumToStringBuf(bindingkey[1], binding[1], sizeof(binding[1]));
		Q_strupr(binding[0]);
		Q_strupr(binding[1]);
		CG_Text_Paint_Ext(8, y + 10, .2f, .2f, colour,
						  va("Create new speaker: %s%s", bindingkey[0] != -1 ? binding[0] : "???",
							 bindingkey[1] != -1 ? va(" or %s", binding[1]) : ""),
						  0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		trap_Key_KeysForBinding("modifyspeaker", &bindingkey[0], &bindingkey[1]);
		trap_Key_KeynumToStringBuf(bindingkey[0], binding[0], sizeof(binding[0]));
		trap_Key_KeynumToStringBuf(bindingkey[1], binding[1], sizeof(binding[1]));
		Q_strupr(binding[0]);
		Q_strupr(binding[1]);
		CG_Text_Paint_Ext(8, y + 20, .2f, .2f, colour,
						  va("Modify target speaker: %s%s", bindingkey[0] != -1 ? binding[0] : "???",
							 bindingkey[1] != -1 ? va(" or %s", binding[1]) : ""),
						  0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		if(undoSpeakerIndex != -2)
		{
			trap_Key_KeysForBinding("undospeaker", &bindingkey[0], &bindingkey[1]);
			trap_Key_KeynumToStringBuf(bindingkey[0], binding[0], sizeof(binding[0]));
			trap_Key_KeynumToStringBuf(bindingkey[1], binding[1], sizeof(binding[1]));
			Q_strupr(binding[0]);
			Q_strupr(binding[1]);
			CG_Text_Paint_Ext(8, y + 30, .2f, .2f, colour,
							  va("Undo %s speaker: %s%s", undoSpeakerIndex == -1 ? "remove" : "modify",
								 bindingkey[0] != -1 ? binding[0] : "???",
								 bindingkey[1] != -1 ? va(" or %s", binding[1]) : ""),
							  0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
		}

		// render crosshair

		x = cg_crosshairX.integer;
		y = cg_crosshairY.integer;
		w = h = cg_crosshairSize.value;

		CG_AdjustFrom640(&x, &y, &w, &h);

		trap_R_DrawStretchPic(x + 0.5 * (cg.refdef_current->width - w),
							  y + 0.5 * (cg.refdef_current->height - h),
							  w, h, 0, 0, 1, 1, cgs.media.crosshairShader[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);

		if(cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS])
		{
			trap_R_DrawStretchPic(x + 0.5 * (cg.refdef_current->width - w),
								  y + 0.5 * (cg.refdef_current->height - h),
								  w, h, 0, 0, 1, 1, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
		}


		if(editSpeaker)
		{
			// render interface
			BG_PanelButtonsRender(speakerInfoButtons);
		}

	}
	else
	{
		// render interface
		BG_PanelButtonsRender(speakerEditorButtons);

		// render cursor
		trap_R_SetColor(NULL);
		CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);
	}
}

void CG_SpeakerEditor_KeyHandling(int key, qboolean down)
{
	if(!BG_PanelButtonsKeyEvent(key, down, speakerEditorButtons))
	{
		switch (key)
		{
			case K_MOUSE1:
				if(!down)
				{
					editSpeakerHandle.activeAxis = -1;
					break;
				}
				else if(editSpeakerHandle.activeAxis == -1)
				{
					int             i, closest;
					float           dist, minDist, r, u;
					vec3_t          vec, axisOrg, dir;

					closest = -1;
					minDist = Square(16.f);

					r = -(cg.refdef_current->fov_x / 90.f) * (float)(cgs.cursorX - 320) / 320;
					u = -(cg.refdef_current->fov_y / 90.f) * (float)(cgs.cursorY - 240) / 240;

					for(i = 0; i < 3; i++)
					{
						dir[i] = cg.refdef_current->viewaxis[0][i] * 1.f +
							cg.refdef_current->viewaxis[1][i] * r + cg.refdef_current->viewaxis[2][i] * u;
					}
					VectorNormalizeFast(dir);

					for(i = 0; i < 3; i++)
					{
						VectorClear(vec);
						vec[i] = 1.f;
						VectorMA(editSpeakerHandle.origin, 32, vec, axisOrg);

						// see which one is closest to our cursor
						VectorSubtract(axisOrg, cg.refdef_current->vieworg, vec);
						dist = DotProduct(vec, dir);
						VectorMA(cg.refdef_current->vieworg, dist, dir, vec);
						dist = DistanceSquared(axisOrg, vec);
						if(dist <= minDist)
						{
							minDist = dist;
							closest = i;
						}
					}

					editSpeakerHandle.activeAxis = closest;

					if(editSpeakerHandle.activeAxis >= 0)
					{
						VectorCopy(editSpeakerHandle.origin, editSpeakerHandle.oldOrigin);
					}
					;
				}
				break;
			case K_ESCAPE:
				BG_PanelButtons_SetFocusButton(NULL);
				CG_SaveSpeakersToScript();
				editSpeakerActive = qfalse;
				CG_EventHandling(-CGAME_EVENT_SPEAKEREDITOR, qtrue);
				break;
		}
	}
}

void CG_SpeakerEditorMouseMove_Handling(int x, int y)
{
	if(!cg.editingSpeakers)
	{
		return;
	}

	if(editSpeakerActive)
	{
		if(editSpeakerHandle.activeAxis >= 0)
		{
			if(editSpeakerHandle.activeAxis == 0)
			{
				// this one and the next one are quite nasty, so do it the hacky way
				if(cgs.cursorX - x < 320)
				{
					editSpeaker->origin[0] -= x;
				}
				else
				{
					editSpeaker->origin[0] += x;
				}
			}
			else if(editSpeakerHandle.activeAxis == 1)
			{
				if(cgs.cursorX - x < 320)
				{
					editSpeaker->origin[1] -= x;
				}
				else
				{
					editSpeaker->origin[1] += x;
				}
			}
			else if(editSpeakerHandle.activeAxis == 2)
			{
				// but this one is easy
				editSpeaker->origin[2] -= y;
			}

			cgs.cursorX -= x;
			cgs.cursorY -= y;

			VectorCopy(editSpeakerHandle.origin, editSpeakerHandle.oldOrigin);
			VectorCopy(editSpeaker->origin, editSpeakerHandle.origin);
		}
	}
}

void CG_ActivateEditSoundMode(void)
{
	CG_Printf("Activating Speaker Edit mode.\n");
	cg.editingSpeakers = qtrue;

	editSpeaker = NULL;
	editSpeakerActive = qfalse;
	editSpeakerHandle.activeAxis = -1;
	undoSpeakerIndex = -2;

	if(!speakerShader)
	{
		speakerShader = trap_R_RegisterShader("gfx/misc/speaker");
		speakerShaderGrayScale = trap_R_RegisterShader("gfx/misc/speaker_gs");

		BG_PanelButtonsSetup(speakerInfoButtons);
		BG_PanelButtonsSetup(speakerEditorButtons);
	}
}

void CG_DeActivateEditSoundMode(void)
{
	CG_Printf("De-activating Speaker Edit mode.\n");
	cg.editingSpeakers = qfalse;

	if(editSpeakerActive)
	{
		CG_EventHandling(-CGAME_EVENT_SPEAKEREDITOR, qtrue);
	}

	editSpeaker = NULL;
	editSpeakerActive = qfalse;
	editSpeakerHandle.activeAxis = -1;
	undoSpeakerIndex = -2;
}

void CG_ModifyEditSpeaker(void)
{
	if(!editSpeaker || editSpeakerActive)
	{
		return;
	}

	CG_EventHandling(CGAME_EVENT_SPEAKEREDITOR, qfalse);

	editSpeakerActive = qtrue;
	memcpy(&undoSpeaker, editSpeaker, sizeof(undoSpeaker));
	undoSpeakerIndex = BG_GetIndexForSpeaker(editSpeaker);

	VectorCopy(editSpeaker->origin, editSpeakerHandle.origin);
	VectorCopy(editSpeaker->origin, editSpeakerHandle.oldOrigin);

	Q_strncpyz(noiseEditBuffer, editSpeaker->filename, sizeof(noiseEditBuffer));
	Q_strncpyz(targetnameEditBuffer, editSpeaker->targetname, sizeof(targetnameEditBuffer));
	speakerEditorLoopedDropdown.data[1] = editSpeaker->loop;
	speakerEditorBroadcastDropdown.data[1] = editSpeaker->broadcast;
	Com_sprintf(waitEditBuffer, sizeof(waitEditBuffer), "%i", editSpeaker->wait);
	Com_sprintf(randomEditBuffer, sizeof(randomEditBuffer), "%i", editSpeaker->random);
	Com_sprintf(volumeEditBuffer, sizeof(volumeEditBuffer), "%i", editSpeaker->volume);
	Com_sprintf(rangeEditBuffer, sizeof(rangeEditBuffer), "%i", editSpeaker->range);
}

void CG_UndoEditSpeaker(void)
{
	if(undoSpeakerIndex == -2)
	{
		return;
	}

	if(undoSpeakerIndex == -1)
	{
		if(!BG_SS_StoreSpeaker(&undoSpeaker))
		{
			CG_Printf(S_COLOR_YELLOW "UNDO: restoring deleted speaker failed, no storage memory for speaker\n");
		}
		else
		{
			CG_Printf("UNDO: restored deleted speaker at %.2f %.2f %.2f.\n", undoSpeaker.origin[0], undoSpeaker.origin[1],
					  undoSpeaker.origin[2]);
		}
	}
	else
	{
		bg_speaker_t   *speaker = BG_GetScriptSpeaker(undoSpeakerIndex);

		memcpy(speaker, &undoSpeaker, sizeof(*speaker));
		CG_Printf("UNDO: restoring modified settings of speaker at %.2f %.2f %.2f.\n", undoSpeaker.origin[0],
				  undoSpeaker.origin[1], undoSpeaker.origin[2]);
	}

	CG_SaveSpeakersToScript();
	undoSpeakerIndex = -2;
}

// Normal Use

void CG_ToggleActiveOnScriptSpeaker(int index)
{
	bg_speaker_t   *speaker = BG_GetScriptSpeaker(index);

	if(speaker)
	{
		speaker->activated = !speaker->activated;
	}
}

void CG_UnsetActiveOnScriptSpeaker(int index)
{
	bg_speaker_t   *speaker = BG_GetScriptSpeaker(index);

	if(speaker)
	{
		speaker->activated = qfalse;
	}
}

void CG_SetActiveOnScriptSpeaker(int index)
{
	bg_speaker_t   *speaker = BG_GetScriptSpeaker(index);

	if(speaker)
	{
		speaker->activated = qtrue;
	}
}

static void CG_PlayScriptSpeaker(bg_speaker_t * speaker, qboolean global)
{
	switch (speaker->loop)
	{
		case S_LT_NOT_LOOPED:
			if(global)
			{
				trap_S_StartLocalSound(speaker->noise, CHAN_ITEM);
			}
			else
			{
				//trap_S_StartSoundVControl(speaker->origin, -1, CHAN_ITEM, speaker->noise, speaker->volume);
			}
			break;
		case S_LT_LOOPED_ON:
		case S_LT_LOOPED_OFF:
			if(speaker->soundTime == 0)
			{
				speaker->soundTime = trap_S_GetCurrentSoundTime();
			}
			/*trap_S_AddRealLoopingSound(speaker->origin, vec3_origin, speaker->noise, speaker->range, speaker->volume,
									   speaker->soundTime);*/
			break;
	}
}

void CG_AddScriptSpeakers(void)
{
	int             i;
	bg_speaker_t   *speaker;

	if(cg.editingSpeakers)
	{
		CG_RenderScriptSpeakers();
	}

	for(i = 0; i < BG_NumScriptSpeakers(); i++)
	{
		speaker = BG_GetScriptSpeaker(i);

		// don't bother playing missing sounds
		if(!speaker->noise)
		{
			continue;
		}

		// activate if needed
		if(speaker->loop == S_LT_NOT_LOOPED)
		{
			if(cg.time >= speaker->nextActivateTime && (speaker->wait || speaker->random))
			{
				speaker->activated = qtrue;
				speaker->nextActivateTime = cg.time + speaker->wait + speaker->random * crandom();
			}
		}

		if(!speaker->activated)
		{
			speaker->soundTime = 0;
			continue;
		}

		switch (speaker->broadcast)
		{
			case S_BT_LOCAL:
				if(trap_R_inPVS(cg.refdef_current->vieworg, speaker->origin))
				{
					CG_PlayScriptSpeaker(speaker, qfalse);
				}
				break;
			case S_BT_GLOBAL:
				CG_PlayScriptSpeaker(speaker, qtrue);
				break;
			case S_BT_NOPVS:
				CG_PlayScriptSpeaker(speaker, qfalse);
				break;
		}

		if(speaker->loop == S_LT_NOT_LOOPED)
		{
			speaker->activated = qfalse;
		}
	}
}
