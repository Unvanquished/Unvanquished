/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_demos.h" 

void demoSaveLine( fileHandle_t fileHandle, const char *fmt, ... ) {
	va_list		argptr;
	char buf[1024];
	int len;

	va_start ( argptr, fmt );
	len = _vsnprintf ( buf, sizeof(buf), fmt, argptr );
	va_end ( argptr );
	trap_FS_Write( buf, len, fileHandle );
}

static qboolean captureParseStart( BG_XMLParse_t *parse,const char *line, void *data) {
	if (!line[0]) 
		return BG_XMLError( parse, "start has empty line" );
	demo.capture.start = atoi( line );
	return qtrue;
}

static qboolean captureParseEnd( BG_XMLParse_t *parse,const char *line, void *data) {
	if (!line[0]) 
		return BG_XMLError( parse, "end has empty line" );
	demo.capture.end = atoi( line );
	return qtrue;
}

static qboolean captureParseSpeed( BG_XMLParse_t *parse,const char *line, void *data) {
	if (!line[0]) 
		return BG_XMLError( parse, "speed has empty line" );
	demo.play.speed = atof( line );
	return qtrue;
}

static qboolean captureParseView( BG_XMLParse_t *parse,const char *line, void *data) {
	if (!Q_stricmp(line, "chase")) {
		demo.viewType = viewChase;
	} else if (!Q_stricmp(line, "camera")) {
		demo.viewType = viewCamera;
	} else {
		BG_XMLWarning( parse, "unknown view '%s', defaulting to chase", line );
		demo.viewType = viewChase;
	}
	return qtrue;
}

typedef struct {
	char name[1024];
	char value[1024];
} parseCvar_t;

static qboolean captureParseCvarName( BG_XMLParse_t *parse,const char *line, void *data) {
	parseCvar_t *cvar = (parseCvar_t *) data;
	Q_strncpyz( cvar->name, line, sizeof( cvar->name ));
	return qtrue;
}
static qboolean captureParseCvarValue( BG_XMLParse_t *parse,const char *line, void *data) {
	parseCvar_t *cvar = (parseCvar_t*) data;
	Q_strncpyz( cvar->value, line, sizeof( cvar->value ));
	return qtrue;
}

static qboolean captureParseCvar( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	parseCvar_t cvar;
	static BG_XMLParseBlock_t cvarParseBlock[] = {
		{"name",	0,		captureParseCvarName },
		{"value",	0,		captureParseCvarValue },
		{0, 0, 0}
	};
	cvar.name[0] = 0;
	cvar.value[0] = 0;
	if (!BG_XMLParse( parse, fromBlock, cvarParseBlock, &cvar )) {
		return qfalse;
	}
	if (cvar.name[0]) {
		trap_Cvar_Set( cvar.name, cvar.value );
	}
	return qtrue;
}
/*
static qboolean captureParseClientNumber( BG_XMLParse_t *parse,const char *line, void *data) {
	int *number = (int *) data;
	if (!line[0])
		return qfalse;
	*number = atoi( line );
	return qtrue;
}
static qboolean captureParseClientString( BG_XMLParse_t *parse,const char *line, void *data) {
	int number = *((int *)data);
	if (number <0 || number >= MAX_CLIENTS)
		return qfalse;
	Q_strncpyz( cgs.clientOverride[number], line, sizeof( cgs.clientOverride[number] ));
	CG_NewClientInfo( number );
	return qtrue;
}
static qboolean captureParseClient( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	int clientNum = -1;
	static BG_XMLParseBlock_t clientParseBlock[] = {
		{"number",	0,		captureParseClientNumber },
		{"string",	0,		captureParseClientString },
		{0, 0, 0}
	};
	if (!BG_XMLParse( parse, fromBlock, clientParseBlock, &clientNum )) {
		return qfalse;
	}
	return qtrue;
}

static qboolean captureParseGroupName( BG_XMLParse_t *parse,const char *line, void *data) {
	char **store = ((char **)data);
	if (!line[0])
		return qfalse;
	if ( !Q_stricmp( line, "red" )) {
		*store = cgs.redOverride;
	} else if ( !Q_stricmp( line, "blue" )) {
		*store = cgs.blueOverride;
	} else if ( !Q_stricmp( line, "all" )) {
		*store = cgs.allOverride;
	} else if ( !Q_stricmp( line, "enemy" )) {
		*store = cgs.enemyOverride;
	} else if ( !Q_stricmp( line, "friendly" )) {
		*store = cgs.friendlyOverride;
	} else if ( !Q_stricmp( line, "player" )) {
		*store = cgs.playerOverride;
	} else {
		return qfalse;
	}
	return qtrue;
}
static qboolean captureParseGroupString( BG_XMLParse_t *parse,const char *line, void *data) {
	char *store = ((char **)data)[0];
	if ( !store )
		return qfalse;
	Q_strncpyz( store, line, sizeof( cgs.redOverride ));
	return qtrue;
}
static qboolean captureParseGroup( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	char *store = 0;
	static BG_XMLParseBlock_t GroupParseBlock[] = {
		{"name",	0,		captureParseGroupName },
		{"string",	0,		captureParseGroupString },
		{0, 0, 0}
	};
	if (!BG_XMLParse( parse, fromBlock, GroupParseBlock, &store )) {
		return qfalse;
	}
	return qtrue;
}*/

qboolean captureParse( BG_XMLParse_t *parse, const struct BG_XMLParseBlock_s *fromBlock, void *data) {
	static BG_XMLParseBlock_t captureParseBlock[] = {
		{"start",	0,					captureParseStart },
		{"end",		0,					captureParseEnd	},
		{"speed",	0,					captureParseSpeed },
		{"view",	0,					captureParseView },
		{"cvar",	captureParseCvar,	0 },
//		{"client",	captureParseClient,	0 },
//		{"group",	captureParseGroup,	0 },
		{0, 0, 0}
	};

	if (!BG_XMLParse( parse, fromBlock, captureParseBlock, NULL )) {
		return qfalse;
	}
	return qtrue;
};

void captureSave( fileHandle_t fileHandle ) {
	char buf[MAX_STRING_TOKENS];
	char *listParse = buf;
	char *viewString;
	char cvarName[1024];
	char cvarBuf[1024];
	int cvarIndex, i;


	trap_Cvar_VariableStringBuffer( "mov_captureCvars", buf, sizeof( buf ));
	if (!buf[0]) {
		trap_SendConsoleCommand( "seta mov_captureCvars \"mov_captureName mov_captureFPS mov_musicFile mov_musicStart mov_captureCamera mov_fragFormat mov_filterMask mov_stencilMask mme_blurFrames mme_blurOverlap mme_blurType mme_depthFocus mme_depthRange mme_screenShotFormat mme_saveStencil mme_saveDepth mme_saveShot mme_saveWav\"" );
		trap_Cvar_VariableStringBuffer( "mov_captureCvars", buf, sizeof( buf ));
	}

	demoSaveLine( fileHandle, "<capture>\n" );
	demoSaveLine( fileHandle, "\t<start>%d</start>\n", demo.capture.start );
	demoSaveLine( fileHandle, "\t<end>%d</end>\n", demo.capture.end );
	demoSaveLine( fileHandle, "\t<speed>%2.8f</speed>\n", demo.play.speed );
	switch (demo.viewType) {
	case viewCamera:
		viewString = "camera";
		break;
	case viewChase:
		viewString = "chase";
		break;
	default:
		viewString = 0;
		break;
	}
	if (viewString)
		demoSaveLine( fileHandle, "\t<view>%s</view>\n", viewString );

	/* Save the select cvar's */
	cvarIndex = 0;
	while ( 1 ) {
		switch (listParse[0]) {
		case 0:
		case ' ':
			if (cvarIndex && cvarIndex < (sizeof(cvarName)) ) {
				cvarName[ cvarIndex ] = 0;
				cvarIndex = 0;
				trap_Cvar_VariableStringBuffer( cvarName, cvarBuf, sizeof( cvarBuf ));
				demoSaveLine( fileHandle, "\t<cvar>\n" );
				demoSaveLine( fileHandle, "\t\t<name>%s</name>\n", cvarName );
				demoSaveLine( fileHandle, "\t\t<value>%s</value>\n", cvarBuf );
				demoSaveLine( fileHandle, "\t</cvar>\n" );
			}
			break;
		default:
			if (cvarIndex < (sizeof(cvarName) -1) ) 
				cvarName[ cvarIndex++ ] = listParse[0];
			break;
		}
		if (!listParse[0])
			break;
		listParse++;
	}
/*	for (i = 0;i<MAX_CLIENTS;i++) {
		if ( !cgs.clientOverride[i][0])
			continue;
		demoSaveLine( fileHandle, "\t<client>\n" );
		demoSaveLine( fileHandle, "\t\t<number>%d</number>\n", i );
		demoSaveLine( fileHandle, "\t\t<string>%s</string>\n", cgs.clientOverride[i] );
		demoSaveLine( fileHandle, "\t</client>\n" );
	}
	for (i = 0;i<=5;i++) {
		const char* name;
		const char* line;
		switch ( i ) {
		case 0:	
			name = "red";
			line = cgs.redOverride;
			break;
		case 1:
			name = "blue";
			line = cgs.blueOverride;
			break;
		case 2:
			name = "enemy";
			line = cgs.enemyOverride;
			break;
		case 3:
			name = "friendly";
			line = cgs.friendlyOverride;
			break;
		case 4:
			name = "all";
			line = cgs.allOverride;
			break;
		case 5:
			name = "player";
			line = cgs.playerOverride;
			break;
		}
		demoSaveLine( fileHandle, "\t<group>\n" );
		demoSaveLine( fileHandle, "\t\t<name>%s</name>\n", name );
		demoSaveLine( fileHandle, "\t\t<string>%s</string>\n",line );
		demoSaveLine( fileHandle, "\t</group>\n" );
	}*/
	demoSaveLine( fileHandle, "</capture>\n" );
}

static BG_XMLParseBlock_t loadBlock[] = {
	{	"camera",	cameraParse,	0	},
	{	"chase",	chaseParse,		0,	},
	{	"line",		lineParse,		0,	},
	{	"dof",		dofParse,		0	},
	{	"capture",	captureParse,	0,	},
	{	0,			0,				0,	},
};

qboolean demoProjectLoad( const char *baseName ) {
	BG_XMLParse_t xmlParse;
	char fileName[MAX_OSPATH];
	int i;

	Com_sprintf(fileName, sizeof(fileName), "project/%s/%s.cfg", mme_demoFileName.string, baseName );
	if (!BG_XMLOpen( &xmlParse, fileName )) {
		return qfalse;
	}
	if (!BG_XMLParse( &xmlParse, 0, loadBlock, 0 )) {
		Com_Printf("Errors wile loading, check log\n");
		return qfalse;
	}
	for (i = 0;i<MAX_CLIENTS;i++) {
		CG_NewClientInfo( i );
	}
	CG_UpdateCvars();
	return qtrue;
}

qboolean demoProjectSave( const char *baseName ) {
	fileHandle_t fileHandle;
	char fileName[MAX_OSPATH];

	Com_sprintf(fileName, sizeof(fileName), "project/%s/%s.cfg", mme_demoFileName.string, baseName );
	trap_FS_FOpenFile( fileName, &fileHandle, FS_WRITE);
	if (!fileHandle) {
		Com_Printf("Failed to save to %s\n", fileName );
		return qfalse;
	}
	captureSave( fileHandle );
	cameraSave( fileHandle );
	chaseSave( fileHandle );
	lineSave( fileHandle );
	dofSave( fileHandle );
	trap_FS_FCloseFile( fileHandle );
	return qtrue;
}

static qboolean demoCheckProjectList( const char *list, const char *demoName, const char *projectName ) {
	fileHandle_t fileHandle;
	int fileSize;
	char word[MAX_OSPATH];
	int index;
	qboolean haveQuote;
	qboolean doName;
	qboolean goodDemo;
	qboolean ret;

	fileSize = trap_FS_FOpenFile( list, &fileHandle, FS_READ );
	if (!fileHandle) {
		trap_FS_FOpenFile( list, &fileHandle, FS_WRITE );
		if (!fileHandle)
			return qfalse;
		trap_FS_FCloseFile( fileHandle );
		return qfalse;
	}
	if (!fileSize) {
		trap_FS_FCloseFile( fileHandle );
		return qfalse;
	}

	doName = qtrue;
	haveQuote = qfalse;
	goodDemo = qfalse;
	index = 0;
	ret = qfalse;

	while (1) {
		char c;
		
		trap_FS_Read( &c, 1, fileHandle );
lastOne:
		--fileSize;
		switch (c) {
		case '\r':
			break;
		case '"':
			if (!haveQuote) {
				haveQuote = qtrue;
				break;
			}
		case '\n':
		case ' ':
		case '\t':
			if (haveQuote && c != '"') {
				if (index < (sizeof(word)-1)) {
	              word[index++] = c;  
				}
				break;
			}
			if (!index)
				break;
			haveQuote = qfalse;
			word[index++] = 0;
			if (doName) {
				goodDemo = !Q_stricmp( word, demoName );
				doName = qfalse;
			} else {
				if (goodDemo && !Q_stricmp( word, projectName )) {
					ret = qtrue;
					break;
				}
				doName = qtrue;
			}
			index = 0;
			break;
		default:
			if (index < (sizeof(word)-1)) {
              word[index++] = c;  
			}
			break;
		}
		if (!fileSize) {
			c = '\n';
			goto lastOne;
		} else if (fileSize < 0)
			break;
	}
	trap_FS_FCloseFile( fileHandle );
	return ret;
}

void demoSaveCommand_f( void ) {
	char projectName[MAX_OSPATH];
	char listName[MAX_OSPATH];
	char entryLine[1024];
	fileHandle_t fileHandle;

	if (trap_Argc() < 2) {
		Com_Printf("Require a project name and an optional list file\n");
		return;
	}
	Q_strncpyz( projectName, CG_Argv(1), sizeof( projectName ));
	Q_strncpyz( listName, CG_Argv(2), sizeof( listName ));

	if (demoProjectSave( projectName )) {
		Com_Printf("Project saved to %s", projectName );
		if (listName[0] && mme_demoFileName.string[0]) {
			if (!demoCheckProjectList( listName, mme_demoFileName.string, projectName )) {
				trap_FS_FOpenFile( listName, &fileHandle, FS_APPEND );
				if (!fileHandle) {
					Com_Printf("but failed to add to list %s\n", listName );
					return;
				}
				Com_sprintf( entryLine, sizeof(entryLine),
					"\"%s\" \"%s\"\r\n", mme_demoFileName.string, projectName );
				trap_FS_Write( entryLine, strlen( entryLine), fileHandle );
				trap_FS_FCloseFile( fileHandle );
			}
			Com_Printf(" and added to list %s\n",listName );
		} else {
			Com_Printf("\n");
		}
	} else {
		Com_Printf("Failed to save project to %s\n", projectName );
	}
}

void demoLoadCommand_f(void) {
	const char *cmd = CG_Argv(1);

	if (!cmd[0]) {
		Com_Printf("Require a project name\n");
		return;
	}

	if (demoProjectLoad(cmd)) {
		Com_Printf( "Loaded project %s\n", cmd );
	} else {
		Com_Printf( "Failed to open %s for loading\n", cmd );
	}
}

void demoCaptureMove(void) {


}

void demoCaptureCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "lock")) {
		demo.capture.locked = !demo.capture.locked;
		CG_DemosAddLog( "Capture range %s", demo.capture.locked ? "locked" : "unlocked" );
	} else if (!Q_stricmp(cmd, "start")) {
		demoLineCommand_f();
	} else if (!Q_stricmp(cmd, "end")) {
		demoLineCommand_f();
	} else if (!Q_stricmp(cmd, "stop")) {
		if (demo.capture.active)
			CG_DemosAddLog( "Capturing stopped" );
		else
			CG_DemosAddLog( "Not capturing at the moment" );
		demo.capture.active = qfalse;
		if ( demo.loop.total ) {
			CG_DemosAddLog( "Capture looping stopped too" );
			demo.loop.total = 0;
		}
	} else if (!Q_stricmp(cmd, "jpg") || !Q_stricmp(cmd, "tga") || !Q_stricmp(cmd, "png") || !Q_stricmp(cmd, "avi")){
		demo.capture.active = qtrue;
		demo.capture.locked = qfalse;
		
		trap_Cvar_Set( "mme_screenShotFormat", cmd );
		cmd = CG_Argv(2);
		trap_Cvar_Set( "mov_captureFPS", cmd);
		trap_Cvar_Update( &mov_captureFPS );
		if (!mov_captureFPS.value) {
			trap_Cvar_Set( "mov_captureFPS", "25" );
		}
		trap_Cvar_Update( &mov_captureFPS );
		cmd = CG_Argv(3);
		if (!cmd[0]) {
			cmd = "default";
		} 
		trap_Cvar_Set( "mov_captureName", cmd );
		trap_Cvar_Update( &mov_captureName );
		CG_DemosAddLog( "Capturing at %0.2ffps to %s", mov_captureFPS.value, cmd );
	} else if (!Q_stricmp( cmd, "loop" )) {
		demo.loop.start = demo.play.time;
		demo.loop.total = atoi( CG_Argv( 2 ) );
		demo.loop.range = 1000 * atof( CG_Argv( 3 ) );
		demo.loop.index = 0;
		demo.loop.lineDelay = 2000;
	} else if (!Q_stricmp( cmd, "loopstop" )) {
		demo.loop.start = 0;
		demo.loop.total = 0;
	} else {
		Com_Printf("capture usage:\n" );
		Com_Printf("capture jpg/tga/png/avi fps filename, start capturing to a specific file\n" );
		Com_Printf("capture lock, lock capturing to the selected range\n");
		Com_Printf("capture start/end, set start/end parts of capture range\n");
		Com_Printf("capture stop, stop capturing\n" );
		return;
	}
}

