/*
 *
 * Quake3Arena iPad Port by Alexander Pick
 * based on iPhone Quake 3 by Seth Kingsley
 *
 */

#import <Foundation/Foundation.h>
#import <zlib.h>

@interface OWDownloader :
NSObject
{
	id             _delegate;
	long long      _archiveOffset, _downloadSize, _downloadedBytes, _extractedBytes;
	BOOL           _isDecompressing;
	z_stream       zstream;
	NSMutableArray *_downloadFiles;
}

@property( assign, readwrite, nonatomic ) id        delegate;
@property( assign, readwrite, nonatomic ) long long archiveOffset;
- ( BOOL ) addDownloadFileWithPath:
( NSString * ) path rangeInArchive:
( NSRange ) range;
- ( void ) startWithURL:
( NSURL * ) url;

@end

@interface NSObject( OWDownloaderDelegate )

- ( void ) downloader:
( OWDownloader * ) downloader didCompleteProgress:
( double ) progress withText:
( NSString * ) progressText;
- ( void ) downloader:
( OWDownloader * ) downloader didFinishDownloadingWithError:
( NSError * ) error;

@end
