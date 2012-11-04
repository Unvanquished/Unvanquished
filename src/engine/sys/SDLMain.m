/*
   SDLMain.m - main entry point for our Cocoa-ized SDL app

   Initial Version: Darrell Walisser <dwaliss1@purdue.edu>
   Non-NIB-Code & other changes: Max Horn <max@quendi.de>

   Feel free to customize this file to suit your needs
*/

#import <Cocoa/Cocoa.h>
#import <sys/param.h>

#import "SDL/SDL.h"
#import "SDLMain.h"

#define SDL_USE_CPS 1

#ifdef SDL_USE_CPS

/* Portions of CPS.h */
typedef struct CPSProcessSerNum
{
	UInt32 lo;
	UInt32 hi;
} CPSProcessSerNum;

extern OSErr CPSGetCurrentProcess( CPSProcessSerNum *psn );
extern OSErr CPSEnableForegroundOperation( CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5 );
extern OSErr CPSSetFrontProcess( CPSProcessSerNum *psn );

#endif

static int  gArgc;
static char **gArgv;
static BOOL gFinderLaunch;
static BOOL gCalledAppMainline = FALSE;

static NSString *getApplicationName( void )
{
	const NSDictionary *dict;
	NSString           *appName = 0;

	/* Determine the application name */
	dict = (const NSDictionary *) CFBundleGetInfoDictionary( CFBundleGetMainBundle() );

	if ( dict )
	{
		appName = [dict objectForKey: @"CFBundleName"];
	}

	if ( ![appName length] )
	{
		appName = [[NSProcessInfo processInfo] processName];
	}

	return appName;
}

@implementation SDLMain

static void setApplicationMenu( void )
{
	/* warning: this code is very odd */
	NSMenu     *appleMenu;
	NSMenuItem *menuItem;
	NSString   *title;
	NSString   *appName;

	appName   = getApplicationName();
	appleMenu = [[NSMenu alloc] initWithTitle:@""];

	/* Add menu items */
	title = [@"About " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];

	[appleMenu addItem:[NSMenuItem separatorItem]];

	title = [@"Hide " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(hide:) keyEquivalent:@"h"];

	menuItem = (NSMenuItem *) [appleMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
	[menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];

	[appleMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];

	[appleMenu addItem:[NSMenuItem separatorItem]];

	title = [@"Quit " stringByAppendingString:appName];
	[appleMenu addItemWithTitle:title action:@selector(terminate:) keyEquivalent:@"q"];

	/* Put menu into the menubar */
	menuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
	[menuItem setSubmenu:appleMenu];
	[[NSApp mainMenu] addItem:menuItem];

	/* Tell the application object that this is now the application menu */
	[NSApp performSelector:NSSelectorFromString(@"setAppleMenu:") withObject:appleMenu];

	/* Finally give up our references to the objects */
	[appleMenu release];
	[menuItem release];
}

/* Create a window menu */
static void setupWindowMenu( void )
{
	NSMenu     *windowMenu;
	NSMenuItem *windowMenuItem;
	NSMenuItem *menuItem;

	windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];

	/* "Minimize" item */
	menuItem = [[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];
	[windowMenu addItem:menuItem];
	[menuItem release];

	/* Put menu into the menubar */
	windowMenuItem = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
	[windowMenuItem setSubmenu:windowMenu];
	[[NSApp mainMenu] addItem:windowMenuItem];

	/* Tell the application object that this is now the window menu */
	[NSApp setWindowsMenu:windowMenu];

	/* Finally give up our references to the objects */
	[windowMenu release];
	[windowMenuItem release];
}

/* Replacement for NSApplicationMain */
static void CustomApplicationMain( int argc, char **argv )
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	SDLMain           *sdlMain;

	/* Ensure the application object is initialised */
	[NSApplication sharedApplication];

	/* Set up the menubar */
	[NSApp setMainMenu:[[NSMenu alloc] init]];
	setApplicationMenu();
	setupWindowMenu();

	/* Create SDLMain and make it the app delegate */
	sdlMain = [[SDLMain alloc] init];
	[NSApp setDelegate:sdlMain];

#ifdef SDL_USE_CPS
	/* Tell the dock about us */
	CPSProcessSerNum PSN;
	if ( !CPSGetCurrentProcess( &PSN ) )
		if ( !CPSEnableForegroundOperation( &PSN, 0x03, 0x3C, 0x2C, 0x1103 ) )
			if ( !CPSSetFrontProcess( &PSN ) )
				[NSApplication sharedApplication];
#endif

	/* Handle URI events */
	[[NSAppleEventManager sharedAppleEventManager] setEventHandler:sdlMain andSelector:@selector(getUrl:withReplyEvent:) forEventClass:kInternetEventClass andEventID:kAEGetURL];

	/* Start the main event loop */
	[NSApp run];

	[sdlMain release];
	[pool release];
}

/*
 * Catch document open requests...this lets us notice files when the app
 *  was launched by double-clicking a document, or when a document was
 *  dragged/dropped on the app's icon. You need to have a
 *  CFBundleDocumentTypes section in your Info.plist to get this message,
 *  apparently.
 *
 * Files are added to gArgv, so to the app, they'll look like command line
 *  arguments. Previously, apps launched from the finder had nothing but
 *  an argv[0].
 *
 * This message may be received multiple times to open several docs on launch.
 *
 * This message is ignored once the app's mainline has been called.
 */
- (BOOL) application: (NSApplication *) theApplication openFile: (NSString *) filename
{
	int _gArgc = gArgc;

	if ( gCalledAppMainline )
	{
		return FALSE;
	}

	[self addArgc:filename];

	return _gArgc != gArgc ? TRUE : FALSE;
}

- (void) addArgc: (NSString *) argc
{
	const char *temparg;
	size_t     arglen;
	char       *arg;
	char       **newargv;

	if ( !gFinderLaunch )  /* MacOS is passing command line args. */
	{
		return;
	}

	temparg = [argc UTF8String];
	arglen  = strlen( temparg ) + 1;
	arg     = (char *) malloc( arglen );

	if (arg == NULL)
	{
		return;
	}

	newargv = (char **) realloc(gArgv, sizeof (char *) * (gArgc + 2));

	if (newargv == NULL)
	{
		free(arg);
		return;
	}

	gArgv = newargv;
	strlcpy( arg, temparg, arglen );
	gArgv[ gArgc++ ] = arg;
	gArgv[ gArgc ]   = NULL;
	return;
}

/* Set the working directory to the .app's parent directory */
- (void) setupWorkingDirectory
{
	NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
	[[NSFileManager defaultManager] changeCurrentDirectoryPath:resourcePath];
	printf( "Base Directory: %s\n", [resourcePath UTF8String] );
}

void Cbuf_AddText( const char *text );

- (void) getUrl: (NSAppleEventDescriptor *) event withReplyEvent: (NSAppleEventDescriptor *) replyEvent
{
	NSString *uri = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
	char     buffer[ MAXPATHLEN ];

	if ( gCalledAppMainline )
	{
		snprintf( buffer, sizeof( buffer ), "connect \"%s\";", [uri UTF8String] );
		Cbuf_AddText( buffer );
	}
	else
	{
		[self addArgc:@"+connect"];
		[self addArgc:uri];
	}
}

/* Called when the internal event loop has just started running */
- (void) applicationDidFinishLaunching: (NSNotification *) note
{
	int status;

	/* Set the working directory to the .app's parent directory */
	[self setupWorkingDirectory];

	/* Hand off to main application code */
	gCalledAppMainline = TRUE;
	status             = SDL_main( gArgc, gArgv );

	/* We're done, thank you for playing */
	exit( status );
}
@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int main ( int argc, char **argv )
{
	/* Copy the arguments into a global variable */
	/* This is passed if we are launched by double-clicking */
	if ( argc >= 2 && strncmp( argv[ 1 ], "-psn", 4 ) == 0 )
	{
		gArgv = (char **) SDL_malloc( sizeof( char * ) * 2 );
		gArgv[ 0 ] = argv[ 0 ];
		gArgv[ 1 ] = NULL;
		gArgc = 1;
		gFinderLaunch = YES;
	}
	else
	{
		int i;

		gArgc = argc;
		gArgv = (char **) SDL_malloc( sizeof( char * ) * ( argc + 1 ) );

		for ( i = 0; i <= argc; i++ )
		{
			gArgv[i] = argv[i];
		}

		gFinderLaunch = NO;
	}

	CustomApplicationMain( argc, argv );
	return 0;
}
