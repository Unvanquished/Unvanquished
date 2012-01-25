/*
 *
 * Quake3Arena iPad Port by Alexander Pick
 * based on iPhone Quake 3 by Seth Kingsley
 *
 */

#import	"OWScreenView.h"
#import "sys_local.h"
#import	<QuartzCore/QuartzCore.h>
#import	<OpenGLES/ES1/glext.h>
#import	<UIKit/UITouch.h>

#include "../../../etmain/src/ui/keycodes.h"
#include "../renderer/tr_local.h"
#include "../client/client.h"

#define kColorFormat  kEAGLColorFormatRGB565
#define kNumColorBits 16
#define kDepthFormat  GL_DEPTH_COMPONENT16_OES
#define kNumDepthBits 16

@interface OWScreenView ()

- (BOOL)_commonInit;
- (void)_mainGameLoop;
- (BOOL)_createSurface;
- (void)_destroySurface;
- (void)_reCenter;
- (void)_handleMenuDragToPoint:(CGPoint)point;
- (void)_handleDragFromPoint:(CGPoint)location toPoint:(CGPoint)previousLocation;
@end

@implementation OWScreenView

+ (Class)layerClass
{
	return [CAEAGLLayer class];
}


- (void)_mainGameLoop {
			
	CGPoint newLocation = CGPointMake(oldLocation.x - distance/4 * cosf(touchAngle), oldLocation.y - distance/4 * sinf(touchAngle));
	
	CGSize mouseDelta;
	
	mouseDelta.width = roundf((oldLocation.y - newLocation.y) * _mouseScale.x);
	mouseDelta.height = roundf((newLocation.x - oldLocation.x) * _mouseScale.y);
	
	//Sys_QueEvent(Sys_Milliseconds(), SE_MOUSE, mouseDelta.width, mouseDelta.height, 0, NULL);
	
	//oldLocation = newLocation;
	
	//Com_Printf("%f\n", mouseDelta.width);
	
	
	if(distance > 30) {
		
		if(mouseDelta.height < -10) {
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_UPARROW, 1, 0, NULL);
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_DOWNARROW, 0, 0, NULL);
			//Com_Printf("%f\n", mouseDelta.height);
		} else if((mouseDelta.height > 10)) {
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_UPARROW, 0, 0, NULL);
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_DOWNARROW, 1, 0, NULL);
			//Com_Printf("Back: %f\n", mouseDelta.height);
		} else {
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_UPARROW, 0, 0, NULL);
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_DOWNARROW, 0, 0, NULL);		
		}
		
		
		if(mouseDelta.width < -25) {
			//Com_Printf("Left: %f\n", mouseDelta.width);
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'a', 1, 0, NULL);
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'd', 0, 0, NULL);
		} else if(mouseDelta.width > 25) {
			//Com_Printf("Right: %f\n", mouseDelta.width);	
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'a', 0, 0, NULL);
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'd', 1, 0, NULL);
		} else {
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'a', 0, 0, NULL);
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'd', 0, 0, NULL);
		}
				
	} else {
		Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_UPARROW, 0, 0, NULL);
		Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_DOWNARROW, 0, 0, NULL);
		Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'a', 0, 0, NULL);
		Sys_QueEvent(Sys_Milliseconds(), SE_KEY, 'd', 0, 0, NULL);		
	}
	
	
	
}

- (BOOL)_commonInit
{

	CAEAGLLayer *layer = (CAEAGLLayer *)self.layer;

	[layer setDrawableProperties:[NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
			kColorFormat, kEAGLDrawablePropertyColorFormat,
			nil]];

	if (!(_context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1]))
		return NO;

	if (![self _createSurface])
		return NO;

	[self setMultipleTouchEnabled:YES];

	_GUIMouseLocation = CGPointMake(0, 0);
	
	joypad = CGRectMake(500, 840, 250, 250);
	joypadCenterx = 550;
	joypadCentery = 890;
	joypadMaxRadius = 60;
	
	shootbutton = CGRectMake(500, 94, 68, 68);
	
	gameTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60 target:self selector:@selector(_mainGameLoop) userInfo:nil repeats:YES];	

	return YES;
}

- initWithCoder:(NSCoder *)coder
{
	if ((self = [super initWithCoder:coder]))
	{
		if (![self _commonInit])
		{
			[self release];
			return nil;
		}
	}

	return self;
}

- initWithFrame:(CGRect)frame
{
	if ((self = [super initWithFrame:frame]))
	{
		if (![self _commonInit])
		{
			[self release];
			return nil;
		}
	}

	return self;
}

- (void)dealloc
{
	[self _destroySurface];

	[_context release];

	[super dealloc];
}

- (BOOL)_createSurface
{
	CAEAGLLayer *layer = (CAEAGLLayer *)self.layer;
	GLint oldFrameBuffer, oldRenderBuffer;

	if (![EAGLContext setCurrentContext:_context])
		return NO;

	_size = layer.bounds.size;
	_size.width = roundf(_size.width);
	_size.height = roundf(_size.height);
	if (_size.width > _size.height)
	{
		_GUIMouseOffset.width = _GUIMouseOffset.height = 0; 
		_mouseScale.x = 99; //1024 / _size.width;
		_mouseScale.y = 768 / _size.height;
	}
	else
	{
		float aspect = _size.height / _size.width; // 5;

		_GUIMouseOffset.width = -roundf((768 * aspect - 1024) / 2.0);
		_GUIMouseOffset.height = 0;
		_mouseScale.x = 5; //(768 * aspect) / _size.height;
		_mouseScale.y = 768 / _size.width;
	}

	qglGetIntegerv(GL_RENDERBUFFER_BINDING_OES, &oldRenderBuffer);
	qglGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &oldFrameBuffer);

	glGenRenderbuffersOES(1, &_renderBuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, _renderBuffer);

	if (![_context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer])
	{
		glDeleteRenderbuffersOES(1, &_renderBuffer);
		glBindRenderbufferOES(GL_RENDERBUFFER_BINDING_OES, oldRenderBuffer);
		return NO;
	}

	glGenFramebuffersOES(1, &_frameBuffer);
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, _frameBuffer);
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, _renderBuffer);
	glGenRenderbuffersOES(1, &_depthBuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, _depthBuffer);
	glRenderbufferStorageOES(GL_RENDERBUFFER_OES, kDepthFormat, _size.width, _size.height);
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, _depthBuffer);

	glBindRenderbufferOES(GL_FRAMEBUFFER_OES, oldFrameBuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, oldRenderBuffer);
	
	return YES;
}

- (void)_destroySurface
{
	EAGLContext *oldContext = [EAGLContext currentContext];

	if (oldContext != _context)
		[EAGLContext setCurrentContext:_context];

	glDeleteRenderbuffersOES(1, &_depthBuffer);
	glDeleteRenderbuffersOES(1, &_renderBuffer);
	glDeleteFramebuffersOES(1, &_frameBuffer);

	if (oldContext != _context)
		[EAGLContext setCurrentContext:oldContext];
}

- (void)layoutSubviews
{
	CGSize boundsSize = self.bounds.size;

	if (roundf(boundsSize.width) != _size.width || roundf(boundsSize.height) != _size.height)
	{
		[self _destroySurface];
		[self _createSurface];
	}
}

- (void)_handleMenuDragToPoint:(CGPoint)point
{
	CGPoint mouseLocation, GUIMouseLocation;
	int deltaX, deltaY;
	
	mouseLocation.x = point.y;
	mouseLocation.y = point.x;

	GUIMouseLocation.x = roundf(_GUIMouseOffset.width - mouseLocation.x * _mouseScale.x);
	GUIMouseLocation.y = roundf(_GUIMouseOffset.height - mouseLocation.y * _mouseScale.y);
	
	GUIMouseLocation.x = MIN(MAX(GUIMouseLocation.x, 0), 1024);
	GUIMouseLocation.y = MIN(MAX(GUIMouseLocation.y, 0), 768);
	
	deltaX = (GUIMouseLocation.x + _GUIMouseLocation.x);
	deltaY = (GUIMouseLocation.y + _GUIMouseLocation.y);
	_GUIMouseLocation = GUIMouseLocation;
	
	if (deltaX || deltaY) {
		Sys_QueEvent(0, SE_MOUSE, deltaX, deltaY, 0, NULL);
	}	
}

- (void)_reCenter {
	
	if(!isFinger) {
		Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_END, 1, 0, NULL);
		Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_END, 0, 0, NULL);
	}
}

// handleDragFromPoint rotates the camera based on a touchedMoved event
- (void)_handleDragFromPoint:(CGPoint)location toPoint:(CGPoint)previousLocation
{
	CGSize mouseDelta;
		
	mouseDelta.width = roundf((previousLocation.y - location.y) * _mouseScale.x);
	mouseDelta.height = roundf((location.x - previousLocation.x) * _mouseScale.y);
	
	mouseDelta.width = mouseDelta.width - (mouseDelta.width * 2);
	mouseDelta.height = mouseDelta.height - (mouseDelta.height * 2);
		
	Sys_QueEvent(Sys_Milliseconds(), SE_MOUSE, mouseDelta.width, mouseDelta.height, 0, NULL);
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
	
	NSSet *allTouches = [event allTouches];
    
	for (UITouch *touch in allTouches) {
        
		CGPoint touchLocation = [touch locationInView:self];
        
		if (CGRectContainsPoint(joypad, touchLocation) && !joypadMoving) {
        
			joypadMoving = YES;
            joypadTouchHash = [touch hash];
        
		} else if(CGRectContainsPoint(shootbutton, touchLocation) && !shooting) {
		
			shooting = YES;
			
			isFinger = YES;
			
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_MOUSE1, 1, 0, NULL);
			
			[self _handleMenuDragToPoint:[touch locationInView:self]];
			
		} else {
			
			//look around
			
			isFinger = YES;
			
			[self _handleMenuDragToPoint:[touch locationInView:self]];
			
		}
    }
}


- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event {
    NSSet *allTouches = [event allTouches];
    
    for (UITouch *touch in allTouches) {
		
		if ([touch hash] == joypadTouchHash && joypadMoving) {
			CGPoint touchLocation = [touch locationInView:self];
			
			float dx = (float)joypadCenterx - (float)touchLocation.x;
			float dy = (float)joypadCentery - (float)touchLocation.y;
			
			distance = sqrtf((joypadCenterx - touchLocation.x) * (joypadCenterx - touchLocation.x) + 
							 (joypadCentery - touchLocation.y) * (joypadCentery - touchLocation.y));
			
			touchAngle = atan2(dy, dx);
			
			if (distance > joypadMaxRadius) {
				joypadCap.center = CGPointMake(joypadCenterx - cosf(touchAngle) * joypadMaxRadius, 
											   joypadCentery - sinf(touchAngle) * joypadMaxRadius);
			
			} else {
				joypadCap.center = touchLocation;
			}
		} else {
			[self _handleDragFromPoint:[touch previousLocationInView:self]
							   toPoint:[touch locationInView:self]];	
		}
	}
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
	for (UITouch *touch in touches) {
		if ([touch hash] == joypadTouchHash) {
		
			joypadMoving = NO;
			joypadTouchHash = 0;
			distance = 0;
			touchAngle = 0;
			joypadCap.center = CGPointMake(joypadCenterx, joypadCentery);
			return;
		
		} else {

			shooting = NO;
			
			Sys_QueEvent(Sys_Milliseconds(), SE_KEY, K_MOUSE1, 0, 0, NULL);

			isFinger = NO;
			
			[NSTimer scheduledTimerWithTimeInterval:4.0 target:self selector:@selector(_reCenter) userInfo:nil repeats:NO];
		}
	}
}


@dynamic numColorBits;

- (NSUInteger)numColorBits
{
	return kNumColorBits;
}

- (NSUInteger)numDepthBits
{
	return kNumDepthBits;
}

@synthesize context = _context;

- (void)swapBuffers
{
	EAGLContext *oldContext = [EAGLContext currentContext];
	GLuint oldRenderBuffer;

	if (oldContext != _context)
		[EAGLContext setCurrentContext:_context];

	qglGetIntegerv(GL_RENDERBUFFER_BINDING_OES, (GLint *)&oldRenderBuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, _renderBuffer);

	if (![_context presentRenderbuffer:GL_RENDERBUFFER_OES])
		NSLog(@"Failed to swap renderbuffer");

	if (oldContext != _context)
		[EAGLContext setCurrentContext:oldContext];
}

@end
