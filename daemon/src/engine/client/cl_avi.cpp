/*
===========================================================================
Copyright (C) 2005-2006 Tim Angus

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "client.h"

#define INDEX_FILE_EXTENSION ".index.dat"

static const int MAX_RIFF_CHUNKS = 16;

struct audioFormat_t
{
	int rate;
	int format;
	int channels;
	int bits;

	int sampleSize;
	int totalBytes;
};

struct aviFileData_t
{
	bool      fileOpen;
	fileHandle_t  f;
	char          fileName[ MAX_QPATH ];
	int           fileSize;
	int           moviOffset;
	int           moviSize;

	fileHandle_t  idxF;
	int           numIndices;

	int           frameRate;
	int           framePeriod;
	int           width, height;
	int           numVideoFrames;
	int           maxRecordSize;
	bool      motionJpeg;

	bool      audio;
	audioFormat_t a;
	int           numAudioFrames;

	int           chunkStack[ MAX_RIFF_CHUNKS ];
	int           chunkStackTop;

	byte          *cBuffer, *eBuffer;
};

static aviFileData_t afd;

static const int MAX_AVI_BUFFER = 2048;

static byte buffer[ MAX_AVI_BUFFER ];
static int  bufIndex;

/*
===============
SafeFS_Write
===============
*/
static INLINE void SafeFS_Write( const void *buffer, int len, fileHandle_t f )
{
	if ( FS_Write( buffer, len, f ) < len )
	{
		Com_Error( errorParm_t::ERR_DROP, "Failed to write avi file" );
	}
}

/*
===============
WRITE_STRING
===============
*/
static INLINE void WRITE_STRING( const char *s )
{
	Com_Memcpy( &buffer[ bufIndex ], s, strlen( s ) );
	bufIndex += strlen( s );
}

/*
===============
WRITE_4BYTES
===============
*/
static INLINE void WRITE_4BYTES( int x )
{
	buffer[ bufIndex + 0 ] = ( byte )( ( x >> 0 ) & 0xFF );
	buffer[ bufIndex + 1 ] = ( byte )( ( x >> 8 ) & 0xFF );
	buffer[ bufIndex + 2 ] = ( byte )( ( x >> 16 ) & 0xFF );
	buffer[ bufIndex + 3 ] = ( byte )( ( x >> 24 ) & 0xFF );
	bufIndex += 4;
}

/*
===============
WRITE_2BYTES
===============
*/
static INLINE void WRITE_2BYTES( int x )
{
	buffer[ bufIndex + 0 ] = ( byte )( ( x >> 0 ) & 0xFF );
	buffer[ bufIndex + 1 ] = ( byte )( ( x >> 8 ) & 0xFF );
	bufIndex += 2;
}

/*
===============
START_CHUNK
===============
*/
static INLINE void START_CHUNK( const char *s )
{
	if ( afd.chunkStackTop == MAX_RIFF_CHUNKS )
	{
		Com_Error( errorParm_t::ERR_DROP, "ERROR: Top of chunkstack breached" );
	}

	afd.chunkStack[ afd.chunkStackTop ] = bufIndex;
	afd.chunkStackTop++;
	WRITE_STRING( s );
	WRITE_4BYTES( 0 );
}

/*
===============
END_CHUNK
===============
*/
static INLINE void END_CHUNK()
{
	int endIndex = bufIndex;

	if ( afd.chunkStackTop <= 0 )
	{
		Com_Error( errorParm_t::ERR_DROP, "ERROR: Bottom of chunkstack breached" );
	}

	afd.chunkStackTop--;
	bufIndex = afd.chunkStack[ afd.chunkStackTop ];
	bufIndex += 4;
	WRITE_4BYTES( endIndex - bufIndex - 4 );
	bufIndex = endIndex;
	bufIndex = PAD( bufIndex, 2 );
}

/*
===============
CL_WriteAVIHeader
===============
*/
void CL_WriteAVIHeader()
{
	bufIndex = 0;
	afd.chunkStackTop = 0;

	START_CHUNK( "RIFF" );
	{
		WRITE_STRING( "AVI " );
		{
			START_CHUNK( "LIST" );
			{
				WRITE_STRING( "hdrl" );
				WRITE_STRING( "avih" );
				WRITE_4BYTES( 56 );  //"avih" "chunk" size
				WRITE_4BYTES( afd.framePeriod );  //dwMicroSecPerFrame
				WRITE_4BYTES( afd.maxRecordSize * afd.frameRate );  //dwMaxBytesPerSec
				WRITE_4BYTES( 0 );  //dwReserved1
				WRITE_4BYTES( 0x110 );  //dwFlags bits HAS_INDEX and IS_INTERLEAVED
				WRITE_4BYTES( afd.numVideoFrames );  //dwTotalFrames
				WRITE_4BYTES( 0 );  //dwInitialFrame

				if ( afd.audio ) //dwStreams
				{
					WRITE_4BYTES( 2 );
				}
				else
				{
					WRITE_4BYTES( 1 );
				}

				WRITE_4BYTES( afd.maxRecordSize );  //dwSuggestedBufferSize
				WRITE_4BYTES( afd.width );  //dwWidth
				WRITE_4BYTES( afd.height );  //dwHeight
				WRITE_4BYTES( 0 );  //dwReserved[ 0 ]
				WRITE_4BYTES( 0 );  //dwReserved[ 1 ]
				WRITE_4BYTES( 0 );  //dwReserved[ 2 ]
				WRITE_4BYTES( 0 );  //dwReserved[ 3 ]

				START_CHUNK( "LIST" );
				{
					WRITE_STRING( "strl" );
					WRITE_STRING( "strh" );
					WRITE_4BYTES( 56 );  //"strh" "chunk" size
					WRITE_STRING( "vids" );

					if ( afd.motionJpeg )
					{
						WRITE_STRING( "MJPG" );
					}
					else
					{
						WRITE_4BYTES( 0 );  // BI_RGB
					}

					WRITE_4BYTES( 0 );  //dwFlags
					WRITE_4BYTES( 0 );  //dwPriority
					WRITE_4BYTES( 0 );  //dwInitialFrame

					WRITE_4BYTES( 1 );  //dwTimescale
					WRITE_4BYTES( afd.frameRate );  //dwDataRate
					WRITE_4BYTES( 0 );  //dwStartTime
					WRITE_4BYTES( afd.numVideoFrames );  //dwDataLength

					WRITE_4BYTES( afd.maxRecordSize );  //dwSuggestedBufferSize
					WRITE_4BYTES( -1 );  //dwQuality
					WRITE_4BYTES( 0 );  //dwSampleSize
					WRITE_2BYTES( 0 );  //rcFrame
					WRITE_2BYTES( 0 );  //rcFrame
					WRITE_2BYTES( afd.width );  //rcFrame
					WRITE_2BYTES( afd.height );  //rcFrame

					WRITE_STRING( "strf" );
					WRITE_4BYTES( 40 );  //"strf" "chunk" size
					WRITE_4BYTES( 40 );  //biSize
					WRITE_4BYTES( afd.width );  //biWidth
					WRITE_4BYTES( afd.height );  //biHeight
					WRITE_2BYTES( 1 );  //biPlanes
					WRITE_2BYTES( 24 );  //biBitCount

					if ( afd.motionJpeg )
					{
						//biCompression
						WRITE_STRING( "MJPG" );
						WRITE_4BYTES( afd.width * afd.height );  //biSizeImage
					}
					else
					{
						WRITE_4BYTES( 0 );  // BI_RGB
						WRITE_4BYTES( afd.width * afd.height * 3 );  //biSizeImage
					}

					WRITE_4BYTES( 0 );  //biXPelsPetMeter
					WRITE_4BYTES( 0 );  //biYPelsPetMeter
					WRITE_4BYTES( 0 );  //biClrUsed
					WRITE_4BYTES( 0 );  //biClrImportant
				}
				END_CHUNK();

				if ( afd.audio )
				{
					START_CHUNK( "LIST" );
					{
						WRITE_STRING( "strl" );
						WRITE_STRING( "strh" );
						WRITE_4BYTES( 56 );  //"strh" "chunk" size
						WRITE_STRING( "auds" );
						WRITE_4BYTES( 0 );  //FCC
						WRITE_4BYTES( 0 );  //dwFlags
						WRITE_4BYTES( 0 );  //dwPriority
						WRITE_4BYTES( 0 );  //dwInitialFrame

						WRITE_4BYTES( afd.a.sampleSize );  //dwTimescale
						WRITE_4BYTES( afd.a.sampleSize * afd.a.rate );  //dwDataRate
						WRITE_4BYTES( 0 );  //dwStartTime
						WRITE_4BYTES( afd.a.totalBytes / afd.a.sampleSize );  //dwDataLength

						WRITE_4BYTES( 0 );  //dwSuggestedBufferSize
						WRITE_4BYTES( -1 );  //dwQuality
						WRITE_4BYTES( afd.a.sampleSize );  //dwSampleSize
						WRITE_2BYTES( 0 );  //rcFrame
						WRITE_2BYTES( 0 );  //rcFrame
						WRITE_2BYTES( 0 );  //rcFrame
						WRITE_2BYTES( 0 );  //rcFrame

						WRITE_STRING( "strf" );
						WRITE_4BYTES( 18 );  //"strf" "chunk" size
						WRITE_2BYTES( afd.a.format );  //wFormatTag
						WRITE_2BYTES( afd.a.channels );  //nChannels
						WRITE_4BYTES( afd.a.rate );  //nSamplesPerSec
						WRITE_4BYTES( afd.a.sampleSize * afd.a.rate );  //nAvgBytesPerSec
						WRITE_2BYTES( afd.a.sampleSize );  //nBlockAlign
						WRITE_2BYTES( afd.a.bits );  //wBitsPerSample
						WRITE_2BYTES( 0 );  //cbSize
					}
					END_CHUNK();
				}
			}
			END_CHUNK();

			afd.moviOffset = bufIndex;

			START_CHUNK( "LIST" );
			{
				WRITE_STRING( "movi" );
			}
		}
	}
}

/*
===============
CL_OpenAVIForWriting

Creates an AVI file and gets it into a state where
writing the actual data can begin
===============
*/
bool CL_OpenAVIForWriting( const char *fileName )
{
	if ( afd.fileOpen )
	{
		return false;
	}

	Com_Memset( &afd, 0, sizeof( aviFileData_t ) );

	// Don't start if a framerate has not been chosen
	if ( cl_aviFrameRate->integer <= 0 )
	{
		Com_Log( log_level_t::LOG_ERROR, "cl_aviFrameRate must be ≥ 1\n" );
		return false;
	}

	if ( ( afd.f = FS_FOpenFileWrite( fileName ) ) <= 0 )
	{
		return false;
	}

	if ( ( afd.idxF = FS_FOpenFileWrite( va( "%s" INDEX_FILE_EXTENSION, fileName ) ) ) <= 0 )
	{
		FS_FCloseFile( afd.f );
		return false;
	}

	Q_strncpyz( afd.fileName, fileName, MAX_QPATH );

	afd.frameRate = cl_aviFrameRate->integer;
	afd.framePeriod = ( int )( 1000000.0f / afd.frameRate );
	afd.width = cls.glconfig.vidWidth;
	afd.height = cls.glconfig.vidHeight;

	if ( cl_aviMotionJpeg->integer )
	{
		afd.motionJpeg = true;
	}
	else
	{
		afd.motionJpeg = false;
	}

	// Buffers only need to store RGB pixels.
	// Allocate a bit more space for the capture buffer to account for possible
	// padding at the end of pixel lines, and padding for alignment
	const int MAX_PACK_LEN = 16;
	afd.cBuffer = (byte*) Z_Malloc( ( afd.width * 3 + MAX_PACK_LEN - 1 ) * afd.height + MAX_PACK_LEN - 1 );
	// raw avi files have pixel lines start on 4-byte boundaries
	afd.eBuffer = (byte*) Z_Malloc( PAD( afd.width * 3, AVI_LINE_PADDING ) * afd.height );

	/*
 	 * TODO
	afd.a.rate = dma.speed;
	afd.a.format = WAV_FORMAT_PCM;
	afd.a.channels = dma.channels;
	afd.a.bits = dma.samplebits;
	afd.a.sampleSize = ( afd.a.bits / 8 ) * afd.a.channels;
	*/

	if ( afd.a.rate % afd.frameRate )
	{
		int suggestRate = afd.frameRate;

		while ( ( afd.a.rate % suggestRate ) && suggestRate >= 1 )
		{
			suggestRate--;
		}

		Com_Printf( S_WARNING "cl_aviFrameRate is not a divisor of the audio rate, suggest %d\n", suggestRate );
	}

	if ( !Cvar_VariableIntegerValue( "s_initsound" ) )
	{
		afd.audio = false;
	}
	else if ( Q_stricmp( Cvar_VariableString( "s_backend" ), "OpenAL" ) )
	{
		if ( afd.a.bits == 16 && afd.a.channels == 2 )
		{
			afd.audio = true;
		}
		else
		{
			afd.audio = false; //FIXME: audio not implemented for this case
		}
	}
	else
	{
		afd.audio = false;
		Com_Printf( S_WARNING "Audio capture is not supported with OpenAL. Set s_useOpenAL to 0 for audio capture\n" );
	}

	// This doesn't write a real header, but allocates the
	// correct amount of space at the beginning of the file
	CL_WriteAVIHeader();

	SafeFS_Write( buffer, bufIndex, afd.f );
	afd.fileSize = bufIndex;

	bufIndex = 0;
	START_CHUNK( "idx1" );
	SafeFS_Write( buffer, bufIndex, afd.idxF );

	afd.moviSize = 4; // For the "movi"
	afd.fileOpen = true;

	return true;
}

/*
===============
CL_CheckFileSize
===============
*/
static bool CL_CheckFileSize( int bytesToAdd )
{
	unsigned int newFileSize;

	newFileSize = afd.fileSize + // Current file size
	              bytesToAdd + // What we want to add
	              ( afd.numIndices * 16 ) + // The index
	              4; // The index size

	// I assume all the operating systems
	// we target can handle a 2Gb file
	if ( newFileSize > INT_MAX )
	{
		// Close the current file...
		CL_CloseAVI();

		// ...And open a new one
		CL_OpenAVIForWriting( va( "%s_", afd.fileName ) );

		return true;
	}

	return false;
}

/*
===============
CL_WriteAVIVideoFrame
===============
*/
void CL_WriteAVIVideoFrame( const byte *imageBuffer, int size )
{
	int  chunkOffset = afd.fileSize - afd.moviOffset - 8;
	int  chunkSize = 8 + size;
	int  paddingSize = PAD( size, 2 ) - size;
	byte padding[ 4 ] = { 0 };

	if ( !afd.fileOpen )
	{
		return;
	}

	// Chunk header + contents + padding
	if ( CL_CheckFileSize( 8 + size + 2 ) )
	{
		return;
	}

	bufIndex = 0;
	WRITE_STRING( "00dc" );
	WRITE_4BYTES( size );

	SafeFS_Write( buffer, 8, afd.f );
	SafeFS_Write( imageBuffer, size, afd.f );
	SafeFS_Write( padding, paddingSize, afd.f );
	afd.fileSize += ( chunkSize + paddingSize );

	afd.numVideoFrames++;
	afd.moviSize += ( chunkSize + paddingSize );

	if ( size > afd.maxRecordSize )
	{
		afd.maxRecordSize = size;
	}

	// Index
	bufIndex = 0;
	WRITE_STRING( "00dc" );  //dwIdentifier
	WRITE_4BYTES( 0x00000010 );  //dwFlags (all frames are KeyFrames)
	WRITE_4BYTES( chunkOffset );  //dwOffset
	WRITE_4BYTES( size );  //dwLength
	SafeFS_Write( buffer, 16, afd.idxF );

	afd.numIndices++;
}

/*
===============
CL_TakeVideoFrame
===============
*/
void CL_TakeVideoFrame()
{
	// AVI file isn't open
	if ( !afd.fileOpen )
	{
		return;
	}

	re.TakeVideoFrame( afd.width, afd.height, afd.cBuffer, afd.eBuffer, afd.motionJpeg );
}

/*
===============
CL_CloseAVI

Closes the AVI file and writes an index chunk
===============
*/
bool CL_CloseAVI()
{
	int        indexRemainder;
	int        indexSize = afd.numIndices * 16;
	const char *idxFileName = va( "%s" INDEX_FILE_EXTENSION, afd.fileName );

	// AVI file isn't open
	if ( !afd.fileOpen )
	{
		return false;
	}

	afd.fileOpen = false;

	FS_Seek( afd.idxF, 4, fsOrigin_t::FS_SEEK_SET );
	bufIndex = 0;
	WRITE_4BYTES( indexSize );
	SafeFS_Write( buffer, bufIndex, afd.idxF );
	FS_FCloseFile( afd.idxF );

	// Write index

	// Open the temp index file
	if ( ( indexSize = FS_FOpenFileRead( idxFileName, &afd.idxF, true ) ) <= 0 )
	{
		FS_FCloseFile( afd.f );
		return false;
	}

	indexRemainder = indexSize;

	// Append index to end of avi file
	while ( indexRemainder > MAX_AVI_BUFFER )
	{
		FS_Read( buffer, MAX_AVI_BUFFER, afd.idxF );
		SafeFS_Write( buffer, MAX_AVI_BUFFER, afd.f );
		afd.fileSize += MAX_AVI_BUFFER;
		indexRemainder -= MAX_AVI_BUFFER;
	}

	FS_Read( buffer, indexRemainder, afd.idxF );
	SafeFS_Write( buffer, indexRemainder, afd.f );
	afd.fileSize += indexRemainder;
	FS_FCloseFile( afd.idxF );

	// Remove temp index file
	FS_Delete( idxFileName );

	// Write the real header
	FS_Seek( afd.f, 0, fsOrigin_t::FS_SEEK_SET );
	CL_WriteAVIHeader();

	bufIndex = 4;
	WRITE_4BYTES( afd.fileSize - 8 );  // "RIFF" size

	bufIndex = afd.moviOffset + 4; // Skip "LIST"
	WRITE_4BYTES( afd.moviSize );

	SafeFS_Write( buffer, bufIndex, afd.f );

	Z_Free( afd.cBuffer );
	Z_Free( afd.eBuffer );
	FS_FCloseFile( afd.f );

	Com_Printf( "Wrote %d:%d frames to %s\n", afd.numVideoFrames, afd.numAudioFrames, afd.fileName );

	return true;
}

/*
===============
CL_VideoRecording
===============
*/
bool CL_VideoRecording()
{
	return afd.fileOpen;
}
