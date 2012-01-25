/*
 *
 * Quake3Arena iPad Port by Alexander Pick
 * based on iPhone Quake 3 by Seth Kingsley
 *
 */

#import "OWDownloader.h"

@interface _OWDownloadFile : NSObject
{
	NSFileHandle *_fileHandle;
	NSRange _rangeInArchive;
}

+ (_OWDownloadFile *)fileWithPath:(NSString *)path rangeInArchive:(NSRange)range;
- initWithPath:(NSString *)path rangeInArchive:(NSRange)range;
- (void)didUnarchiveData:(NSData *)data inRange:(NSRange)range;
- (void)close;

@end

@implementation _OWDownloadFile

+ (_OWDownloadFile *)fileWithPath:(NSString *)path rangeInArchive:(NSRange)range
{
	return [[[self alloc] initWithPath:path rangeInArchive:range] autorelease];
}

- initWithPath:(NSString *)path rangeInArchive:(NSRange)range
{
	if ((self = [super init]))
	{
		NSFileManager *fileManager = [NSFileManager defaultManager];

		if (![fileManager createFileAtPath:path contents:[NSData data] attributes:nil])
			return nil;
		if (!(_fileHandle = [[NSFileHandle fileHandleForWritingAtPath:path] retain]))
			return nil;
		_rangeInArchive = range;
	}

	return self;
}

- (void)didUnarchiveData:(NSData *)data inRange:(NSRange)range
{
	NSUInteger endingUnarchivedLocation = range.location + range.length,
			endLocation = _rangeInArchive.location + _rangeInArchive.length;

	if (endingUnarchivedLocation > _rangeInArchive.location && range.location < endLocation)
	{
		NSRange dataRange;

		if (range.location < _rangeInArchive.location)
			dataRange.location = _rangeInArchive.location - range.location;
		else
			dataRange.location = 0;
		if (endingUnarchivedLocation > endLocation)
			dataRange.length = (endLocation - range.location) - dataRange.location;
		else
			dataRange.length = range.length - dataRange.location;

		[_fileHandle writeData:[data subdataWithRange:dataRange]];
	}
}

- (void)close
{
	[_fileHandle closeFile];
}

@end

@interface OWDownloader ()

- (void)_updateDownloadStatus;
- (NSError *)_zlibErrorWithOperation:(NSString *)operation code:(int)code stream:(const z_stream *)stream;
- (void)_finishDownloadingSharewareGameDataFromConnection:(NSURLConnection *)connection withError:(NSError *)error;

@end

@implementation OWDownloader

@synthesize delegate = _delegate, archiveOffset = _archiveOffset;

- init
{
	if ((self = [super init]))
	{
		_downloadFiles = [NSMutableArray new];
	}

	return self;
}

- (void)dealloc
{
	[_downloadFiles release];

	[super dealloc];
}

- (void)startWithURL:(NSURL *)url
{
	id delegate = self.delegate;
	NSURLConnection *connection = [NSURLConnection connectionWithRequest:[NSURLRequest requestWithURL:url]
																delegate:self];

	_downloadedBytes = 0;
	if ([delegate respondsToSelector:@selector(downloader:didCompleteProgress:withText:)])
		[delegate downloader:self
		 didCompleteProgress:0.0
					withText:[NSString stringWithFormat:@"Connecting to %@...", url.host]];
	[connection start];
}

- (void)_updateDownloadStatus
{
	id delegate = self.delegate;

	if ([delegate respondsToSelector:@selector(downloader:didCompleteProgress:withText:)])
	{
		NSUInteger kbSize = _downloadSize / 1024, kbDownloaded = _downloadedBytes / 1024;
		double progress;
		NSString *progressString;

		if (_downloadSize > 0)
		{
			progress = (double)_downloadedBytes / _downloadSize;
			progressString = [NSString stringWithFormat:@" (%u%%)", (unsigned)(progress * 100)];
		}
		else
		{
			progress = 0.0;
			progressString = @"";
		}

		[delegate downloader:self
		 didCompleteProgress:progress
					withText:[NSString stringWithFormat:@"Downloading: %u/%uK%@",
							  kbDownloaded, kbSize, progressString]];
	}
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
	_downloadSize = response.expectedContentLength;
	[self _updateDownloadStatus];
}

- (NSError *)_zlibErrorWithOperation:(NSString *)operation code:(int)code stream:(const z_stream *)stream
{
	return [NSError errorWithDomain:@"zlibError"
							   code:code
						   userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
								[NSString stringWithFormat:@"%@ failed: %s", operation, stream->msg],
								NSLocalizedDescriptionKey,
								nil]];
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
	long long endingDownloadedBytes = _downloadedBytes + data.length;
	long long archiveOffset = self.archiveOffset;

	if (endingDownloadedBytes > archiveOffset)
	{
		NSUInteger dataOffset;
		NSUInteger outDataSize;
		NSMutableData *outData;
		int code;

		if (!_isDecompressing)
		{
			bzero(&zstream, sizeof(zstream));
			zstream.zalloc = Z_NULL;
			zstream.zfree = Z_NULL;
			
			if ((code = inflateInit2(&zstream, 15 + 32)) != Z_OK)
			{
				[self _finishDownloadingSharewareGameDataFromConnection:connection
															  withError:[self _zlibErrorWithOperation:@"inflateInit()"
																								 code:code
																							   stream:&zstream]];
				return;
			}
			_isDecompressing = YES;
		}

		if (_downloadedBytes < archiveOffset)
			// First block, ignore leading data:
			dataOffset = (archiveOffset - _downloadedBytes);
		else
			dataOffset = 0;

		zstream.next_in = (Bytef *)data.bytes + dataOffset;
		zstream.avail_in = data.length - dataOffset;

		outDataSize = zstream.avail_in * 2;
		outData = [NSMutableData dataWithLength:outDataSize];

		while (zstream.avail_in)
		{
			zstream.next_out = (Bytef *)outData.bytes;
			zstream.avail_out = outDataSize;
			code = inflate(&zstream, Z_SYNC_FLUSH);
			if (code == Z_OK)
			{
				NSUInteger bytesWritten = outDataSize - zstream.avail_out;
				NSUInteger endingExtractedBytes = _extractedBytes + bytesWritten;

				for (_OWDownloadFile *downloadFile in _downloadFiles)
					[downloadFile didUnarchiveData:outData inRange:NSMakeRange(_extractedBytes, bytesWritten)];
				_extractedBytes = endingExtractedBytes;
			}
			else if (code == Z_STREAM_END)
			{
				if ((code = inflateEnd(&zstream)) != Z_OK)
				{
					NSError *error = [self _zlibErrorWithOperation:@"inflateEnd()" code:code stream:&zstream];

					[self _finishDownloadingSharewareGameDataFromConnection:connection withError:error];
					return;
				}
			}
			else
			{
				[self _finishDownloadingSharewareGameDataFromConnection:connection
															  withError:[self _zlibErrorWithOperation:@"inflate()"
																								 code:code
																							   stream:&zstream]];
				return;
			}
		}
	}

	_downloadedBytes = endingDownloadedBytes;
	[self _updateDownloadStatus];
}

- (void)_finishDownloadingSharewareGameDataFromConnection:(NSURLConnection *)connection withError:(NSError *)error
{
	id delegate = self.delegate;

	for (_OWDownloadFile *downloadFile in _downloadFiles)
		[downloadFile close];
	[_downloadFiles removeAllObjects];

	if (error)
		[connection cancel];

	if ([delegate respondsToSelector:@selector(downloader:didFinishDownloadingWithError:)])
		[delegate downloader:self didFinishDownloadingWithError:error];
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
	[self _finishDownloadingSharewareGameDataFromConnection:connection withError:nil];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
	[self _finishDownloadingSharewareGameDataFromConnection:connection withError:error];
}

- (BOOL)addDownloadFileWithPath:(NSString *)path rangeInArchive:(NSRange)range
{
	_OWDownloadFile *downloadFile;

	if (!(downloadFile = [_OWDownloadFile fileWithPath:path rangeInArchive:range]))
		return NO;
	[_downloadFiles addObject:downloadFile];
	return YES;
}

@end
