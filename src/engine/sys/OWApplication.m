/*
 *
 * Quake3Arena iPad Port by Alexander Pick
 * based on iPhone Quake 3 by Seth Kingsley
 *
 */

#import	"OWApplication.h"
#import "OWDownloader.h"
#import	"OWScreenView.h"
#import "sys_local.h"

#import "../renderer/tr_local.h"
#import "../client/client.h"

#import <UIKit/UIAlert.h>
#import <UIKit/UITextView.h>


@interface OWApplication ()

- (void)_startRunning;
- (void)_stopRunning;
- (BOOL)_checkForGameData;
- (void)_downloadSharewareGameData;
- (void)_quakeMain;
#ifdef IPHONE_USE_THREADS
- (void)_runMainLoop:(id)context;
- (void)keepAlive:(NSTimer *)timer;
#else
- (void)_runFrame:(NSTimer *)timer;
#endif // !IPHONE_USE_THREADS

@end

enum
{
	OWApp_ErrorTag,
	OWApp_WarningTag,
	OWApp_GameDataTag,
	OWApp_GameDataErrorTag
};

enum
{
	OWApp_Exit,
	OWApp_Yes,
	OWApp_No
};

extern cvar_t *com_maxfps;


static NSString * const kDemoArchiveURL =
	//	@"ftp://ftp.idsoftware.com/idstuff/quake3/linux/linuxq3ademo-1.11-6.x86.gz.sh";
		@"ftp://ftp-stud.fht-esslingen.de/pub/Mirrors/gentoo/distfiles/linuxq3ademo-1.11-6.x86.gz.sh";
static const long long kDemoArchiveOffset = 5468;
static NSString * const kPakFileName = @"pak0.pk3";
static const long long kDemoPakFileOffset = 5749248;
static const long long kDemoPakFileSize = 46853694;

@implementation OWApplication

@synthesize _shootButton;
@synthesize _forwardButton;
@synthesize _backwardButton;
@synthesize _rightButton;
@synthesize _leftButton;
@synthesize _changeButton;

@synthesize _spaceKey;
@synthesize _escKey;
@synthesize _enterKey;

@synthesize _amokKey;

- (NSString *)getDocumentsDirectory {  
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);  
    return [paths objectAtIndex:0];  
} 

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	return (interfaceOrientation == UIInterfaceOrientationLandscapeLeft);
}

- (void)_startRunning
{
	
	GLimp_SetMode(90.00);	
			  
#if IPHONE_USE_THREADS
	Com_Printf("Starting render thread...\n");

	GLimp_ReleaseGL();

	_frameThread = [NSThread detachNewThreadSelector:@selector(_runMainLoop:) toTarget:self withObject:nil];
#else
	_frameTimer = [NSTimer scheduledTimerWithTimeInterval:1.0 / com_maxfps->integer
												   target:self
												 selector:@selector(_runFrame:)
												 userInfo:nil
												  repeats:YES];
#endif // IPHONE_USE_THREADS
}

- (void)_stopRunning
{
#if IPHONE_USE_THREADS
	Com_Printf("Stopping the render thread...\n");
	[_frameThread cancel];
#else
	[_frameTimer invalidate];
#endif // IPHONE_USE_THREADS
}

- (BOOL)_checkForGameData
{
	
	NSString *libraryPath = [self getDocumentsDirectory]; 
	NSArray *knownGames = [NSArray arrayWithObjects:@"etmain", @"demoq3", nil];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	BOOL isDir;
	BOOL foundGame = NO;
	
	
	NSString *zFile = [libraryPath stringByAppendingPathComponent:@"pak0.pk3"];

#if DEBUG_MODE	
	// to see if there is crap placed in your app directly if moding this, logs to console
	NSFileManager *manager = [NSFileManager defaultManager];
    NSArray *fileList = [manager directoryContentsAtPath:[[ NSBundle mainBundle] resourcePath]];
    for (NSString *s in fileList){
        NSLog(s);
    }	
#endif
	
	NSLog(@"Checking for PAKfile");
	
	if ([fileManager fileExistsAtPath:zFile isDirectory:NO])
	{
		NSLog(@"Found a PAK in Root Directory");
		foundGame = YES;
	} else {
	
	for (NSString *knownGame in knownGames)
	{
		
		NSString *gamePath = [libraryPath stringByAppendingPathComponent:knownGame];
		if ([fileManager fileExistsAtPath:gamePath isDirectory:&isDir] &&
			isDir)
		{
			if ([knownGame isEqualToString:@"demoq3"])
			{
				NSDictionary *attributes =
				[fileManager fileAttributesAtPath:[gamePath stringByAppendingPathComponent:kPakFileName]
									 traverseLink:NO];

				if (attributes.fileSize != kDemoPakFileSize)
					continue;
			}

			foundGame = YES;
			break;
		}
	}

	}	
		
	if (foundGame)
		return YES;
	else
	{
		UIAlertView *alertView = [[[UIAlertView alloc] initWithTitle:@"Welcome to Quake3Arena!"
														     message:
		@"Do you want to download a copy of the shareware Quake 3 Arena Demo's data? Hint: The best way to play Quake 3 Arena on your iPad is by copying your baseq3 PAK files via iTunes filesharing to it."
															delegate:self
												   cancelButtonTitle:@"Exit"
												   otherButtonTitles:@"Yes",
																	@"No",
																	nil] autorelease];

		alertView.tag = OWApp_GameDataTag;
		[alertView show];

		return NO;
	}
}

- (void)downloader:(OWDownloader *)downloader didCompleteProgress:(double)progress withText:(NSString *)text
{
	[_downloadStatusLabel setText:text];
	_downloadProgress.progress = progress;
}

- (void)downloader:(OWDownloader *)downloader didFinishDownloadingWithError:(NSError *)error
{
	_demoDownloader = nil;

	_downloadStatusLabel.hidden =  YES;
	_downloadProgress.hidden =  YES;
	_loadingActivity.hidden =  NO;
	_loadingLabel.hidden =  NO;

	if (error)
	{
		UIAlertView *alertView = [[[UIAlertView alloc] initWithTitle:@"Download Failed"
															 message:error.localizedDescription
															delegate:self
												   cancelButtonTitle:@"Exit"
												   otherButtonTitles:@"Retry", nil] autorelease];
		NSString *gamePath = [[self getDocumentsDirectory] stringByAppendingPathComponent:@"demoq3"];
		NSError *error;

		if (![[NSFileManager defaultManager] removeItemAtPath:gamePath error:&error])
			NSLog(@"Could not delete %@: %@", gamePath, error.localizedDescription);

		alertView.tag = OWApp_GameDataErrorTag;
		[alertView show];
	}
	else
		[self _quakeMain];
}

- (void)_downloadSharewareGameData
{
	NSString *gamePath = [[self getDocumentsDirectory] stringByAppendingPathComponent:@"demoq3"];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSError *error;

	if (![fileManager createDirectoryAtPath:gamePath withIntermediateDirectories:YES attributes:nil error:&error])
		Sys_Error("Could not create folder %@: %@", gamePath, error.localizedDescription);

	_demoDownloader = [OWDownloader new];
	_demoDownloader.delegate = self;
	_demoDownloader.archiveOffset = kDemoArchiveOffset;
	if (![_demoDownloader addDownloadFileWithPath:[gamePath stringByAppendingPathComponent:kPakFileName]
								   rangeInArchive:NSMakeRange(kDemoPakFileOffset, kDemoPakFileSize)])
		Sys_Error("Could not create %@", gamePath);

	[_demoDownloader startWithURL:[NSURL URLWithString:kDemoArchiveURL]];

	_loadingActivity.hidden =  YES;
	_loadingLabel.hidden =  YES;
	_downloadStatusLabel.hidden =  NO;
	_downloadProgress.hidden =  NO;
}

// Events for buttons

-(void)startshoot:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_MOUSE1, 1, 0, NULL);
}

-(void)stopshoot:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_MOUSE1, 0, 0, NULL);
}

-(void)startmove:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_UPARROW, 1, 0, NULL);

}

-(void)stopmove:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_UPARROW, 0, 0, NULL);
}

-(void)startback:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_DOWNARROW, 1, 0, NULL);
}

-(void)stopback:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_DOWNARROW , 0, 0, NULL);
}

// right

-(void)startright:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'd', 1, 0, NULL);
}

-(void)stopright:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'd', 0, 0, NULL);
}

// left

-(void)startleft:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'a', 1, 0, NULL);
}

-(void)stopleft:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'a', 0, 0, NULL);
}

// jump space
-(void)spacePress:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_SPACE , 1, 0, NULL);
}

-(void)spaceRelease:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_SPACE ,0, 0, NULL);
}

// single press keys
-(void)changeWeapon:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, '/', 1, 0, NULL);
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, '/',0, 0, NULL);
}

-(void)escPress:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_ESCAPE , 1, 0, NULL);
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_ESCAPE ,0, 0, NULL);
}

-(void)enterPress:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_ENTER , 1, 0, NULL);
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_ENTER ,0, 0, NULL);
}

-(void)amokPress:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_MOUSE1, 1, 0, NULL);
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_UPARROW, 1, 0, NULL);
}

-(void)amokRelease:(id)Sender
{
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_MOUSE1, 0, 0, NULL);
	Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_UPARROW, 0, 0, NULL);
}

- (void)_quakeMain
{
	extern void Sys_Startup(int argc, char *argv[]);
	NSArray *arguments = [[NSProcessInfo processInfo] arguments];
	int ac, i;
	const char *av[32];

	ac = MIN([arguments count], sizeof(av) / sizeof(av[0]));
	for (i = 0; i < ac; ++i)
		av[i] = [[arguments objectAtIndex:i] cString];

	Sys_Startup(ac, (char **)av);

	[_loadingView removeFromSuperview];
	_screenView.hidden =  NO;

	[_shootButton addTarget:self action:@selector(startshoot:) forControlEvents:UIControlEventTouchDown];
	[_shootButton addTarget:self action:@selector(stopshoot:) forControlEvents:UIControlEventTouchUpInside];	
	
	[_forwardButton addTarget:self action:@selector(startmove:) forControlEvents:UIControlEventTouchDown];
	[_forwardButton addTarget:self action:@selector(stopmove:) forControlEvents:UIControlEventTouchUpInside];	

	[_backwardButton addTarget:self action:@selector(startback:) forControlEvents:UIControlEventTouchDown];
	[_backwardButton addTarget:self action:@selector(stopback:) forControlEvents:UIControlEventTouchUpInside];	

	[_leftButton addTarget:self action:@selector(startleft:) forControlEvents:UIControlEventTouchDown];
	[_leftButton addTarget:self action:@selector(stopleft:) forControlEvents:UIControlEventTouchUpInside];	
	
	[_rightButton addTarget:self action:@selector(startright:) forControlEvents:UIControlEventTouchDown];
	[_rightButton addTarget:self action:@selector(stopright:) forControlEvents:UIControlEventTouchUpInside];	
	
	[_changeButton addTarget:self action:@selector(changeWeapon:) forControlEvents:UIControlEventTouchDown];

	[_spaceKey addTarget:self action:@selector(spacePress:) forControlEvents:UIControlEventTouchDown];
	[_spaceKey addTarget:self action:@selector(spaceRelease:) forControlEvents:UIControlEventTouchUpInside];

	
	[_escKey addTarget:self action:@selector(escPress:) forControlEvents:UIControlEventTouchDown];

	[_enterKey addTarget:self action:@selector(enterPress:) forControlEvents:UIControlEventTouchDown];

	[_amokKey addTarget:self action:@selector(amokPress:) forControlEvents:UIControlEventTouchDown];
	[_amokKey addTarget:self action:@selector(amokRelease:) forControlEvents:UIControlEventTouchUpInside];

	[self _startRunning];
	
}

#ifdef IPHONE_USE_THREADS
- (void)_runMainLoop:(id)context
{
	NSThread *thread = [NSThread currentThread];

	while (!thread.isCancelled)
		Com_Frame();
}
#else
- (void)_runFrame:(NSTimer *)timer
{
	Com_Frame();
}
#endif // IPHONE_USE_THREADS

@synthesize screenView = _screenView;

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex
{
	extern void Sys_Exit(int ex);

	switch (alertView.tag)
	{
		case OWApp_ErrorTag: Sys_Exit(1);
		case OWApp_WarningTag: [self _startRunning]; break;

		case OWApp_GameDataTag:
		case OWApp_GameDataErrorTag:
			switch (buttonIndex)
			{
				case OWApp_Exit: Sys_Exit(0);

				case OWApp_Yes:
					[self performSelector:@selector(_downloadSharewareGameData) withObject:nil afterDelay:0.0];
					break;

				case OWApp_No: [self performSelector:@selector(_quakeMain) withObject:nil afterDelay:0.0];
			}
			break;
	}
}

- (void)presentErrorMessage:(NSString *)errorMessage
{
	UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Error"
													 message:errorMessage
													delegate:self
										   cancelButtonTitle:@"Exit"
										   otherButtonTitles:nil] autorelease];

	alert.tag = OWApp_ErrorTag;
	[self _stopRunning];
	[alert show];
	[[NSRunLoop currentRunLoop] run];
}

- (void)presentWarningMessage:(NSString *)warningMessage
{
	UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Warning"
													 message:warningMessage
													delegate:self
										   cancelButtonTitle:@"OK"
										   otherButtonTitles:nil] autorelease];

	alert.tag = OWApp_WarningTag;
	[self _stopRunning];
	[alert show];
	[[NSRunLoop currentRunLoop] run];
}

- (void)applicationDidFinishLaunching:(id)unused
{
	
	if ([self _checkForGameData])
		[self performSelector:@selector(_quakeMain) withObject:nil afterDelay:0.0];
}

-(void)applicationWillTerminate:(UIApplication *)application
{
	// explict, seems to avoid some strange zombies around
	
	[[NSThread mainThread] exit];	
} 

@end
