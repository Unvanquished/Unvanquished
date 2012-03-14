/*
 *
 * Quake3Arena iPad Port by Alexander Pick
 * based on iPhone Quake 3 by Seth Kingsley
 *
 */

#import <UIKit/UIApplication.h>
#import <UIKit/UINibDeclarations.h>
#import <UIKit/UILabel.h>

#import <UIKit/UIButton.h>

#import <UIKit/UIActivityIndicatorView.h>
#import <UIKit/UIProgressView.h>
#import <UIKit/UIAccelerometer.h>

@class OWScreenView;
@class OWDownloader;

@interface OWApplication :
UIApplication
{
	@protected
	IBOutlet OWScreenView            *_screenView;
	IBOutlet UIView                  *_loadingView;
	IBOutlet UILabel                 *_loadingLabel;
	IBOutlet UIActivityIndicatorView *_loadingActivity;
	IBOutlet UILabel                 *_downloadStatusLabel;
	IBOutlet UIProgressView          *_downloadProgress;
	IBOutlet UIButton                *_shootButton;
	IBOutlet UIButton                *_forwardButton;
	IBOutlet UIButton                *_backwardButton;
	IBOutlet UIButton                *_rightButton;
	IBOutlet UIButton                *_leftButton;
	IBOutlet UIButton                *_changeButton;

	IBOutlet UIButton                *_spaceKey;
	IBOutlet UIButton                *_escKey;
	IBOutlet UIButton                *_enterKey;

	IBOutlet UIButton                *_amokKey;

	OWDownloader                     *_demoDownloader;
#if IPHONE_USE_THREADS
	NSThread                         *_frameThread;
#else
	NSTimer                          *_frameTimer;
#endif // IPHONE_USE_THREADS
}

@property ( assign, readonly, nonatomic ) OWScreenView *screenView;
@property ( assign, readonly, nonatomic ) UIButton     *_shootButton;
@property ( assign, readonly, nonatomic ) UIButton     *_forwardButton;
@property ( assign, readonly, nonatomic ) UIButton     *_backwardButton;
@property ( assign, readonly, nonatomic ) UIButton     *_leftButton;
@property ( assign, readonly, nonatomic ) UIButton     *_rightButton;
@property ( assign, readonly, nonatomic ) UIButton     *_changeButton;
@property ( assign, readonly, nonatomic ) UIButton     *_spaceKey;
@property ( assign, readonly, nonatomic ) UIButton     *_escKey;
@property ( assign, readonly, nonatomic ) UIButton     *_enterKey;
@property ( assign, readonly, nonatomic ) UIButton     *_amokKey;

- ( void ) presentErrorMessage:
( NSString * ) errorMessage;
- ( void ) presentWarningMessage:
( NSString * ) warningMessage;

@end
