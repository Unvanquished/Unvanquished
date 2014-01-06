/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// bg_lib.h -- standard C library replacement routines used by code
// compiled for the virtual machine

// This file is NOT included on native builds
#if !defined( BG_LIB_H ) && defined( Q3_VM )
#define BG_LIB_H

#ifndef NULL
#define NULL ((void *)0 )
#endif


#if defined( __clang__ ) || defined( __GNUC__ )
typedef __builtin_va_list va_list;
#define va_start(ap,v) __builtin_va_start(ap,v)
#define va_arg(ap,t)   __builtin_va_arg(ap,t)
#define va_end(ap)     __builtin_va_end(ap)
#else
typedef char          *va_list;
// possible alignment issues here? This is fine for 32-bit at least...
#define _INTSIZEOF(n)  ( ( sizeof( n ) + sizeof( int ) - 1 ) & ~( sizeof( int ) - 1 ) )
#define va_start(ap,v) ( ap = ( va_list ) & v + _INTSIZEOF(v) )
#define va_arg(ap,t)   ( *(t *)(( ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)     ( ap = (va_list)0 )
#endif

#define CHAR_BIT  8 /* number of bits in a char */
#define SCHAR_MAX 0x7f /* maximum signed char value */
#define SCHAR_MIN ( -SCHAR_MAX - 1 ) /* minimum signed char value */
#define UCHAR_MAX 0xff /* maximum unsigned char value */

#define SHRT_MAX  0x7fff /* maximum (signed) short value */
#define SHRT_MIN  ( -SHRT_MAX - 1 ) /* minimum (signed) short value */
#define USHRT_MAX 0xffff /* maximum unsigned short value */
#define INT_MAX   0x7fffffff /* maximum (signed) int value */
#define INT_MIN   ( -INT_MAX - 1 ) /* minimum (signed) int value */
#define UINT_MAX  0xffffffff /* maximum unsigned int value */
#define LONG_MAX  0x7fffffffL /* maximum (signed) long value */
#define LONG_MIN  ( -LONG_MAX - 1 ) /* minimum (signed) long value */
#define ULONG_MAX 0xffffffffUL /* maximum unsigned long value */

typedef   signed  char int8_t;
typedef unsigned  char uint8_t;
typedef   signed short int16_t;
typedef unsigned short uint16_t;
typedef   signed   int int32_t;
typedef unsigned   int uint32_t;

#if defined( __GNUC__ ) || defined( __clang__ )
typedef        __INT64_TYPE__ int64_t;
typedef signed __INT64_TYPE__ uint64_t;
#elif defined( _WIN32 )
typedef                _int64 int64_t;
typedef               _uint64 uint64_t;
#elif !defined( __LCC__ )
#error HELP! I need some 64-bit integer types!
#endif

#if defined( __GNUC__ ) || defined( __clang__ )
typedef        __SIZE_TYPE__ size_t;
#elif defined( _WIN64 )
typedef             uint64_t size_t;
#elif defined( _WIN32 ) || defined( __LCC__ )
typedef         unsigned int size_t;
#else
#error HELP! I need size_t!
#endif

#define isalnum(c)  ( isalpha(c) || isdigit(c))
#define isalpha(c)  ( isupper(c) || islower(c))
#define isascii(c)  (( c ) > 0 && ( c ) <= 0x7f )
#define iscntrl(c)  ((( c ) >= 0 ) && ((( c ) <= 0x1f ) || (( c ) == 0x7f )))
#define isdigit(c)  (( c ) >= '0' && ( c ) <= '9' )
#define isgraph(c)  (( c ) != ' ' && isprint(c))
#define islower(c)  (( c ) >=  'a' && ( c ) <= 'z' )
#define isprint(c)  (( c ) >= ' ' && ( c ) <= '~' )
#define ispunct(c)  ((( c ) > ' ' && ( c ) <= '~' ) && !isalnum(c))
#define isspace(c)  (( c ) ==  ' ' || ( c ) == '\f' || ( c ) == '\n' || ( c ) == '\r' || \
                     ( c ) == '\t' || ( c ) == '\v' )
#define isupper(c)  (( c ) >=  'A' && ( c ) <= 'Z' )
#define isxdigit(c) ( isxupper(c) || isxlower(c))
#define isxlower(c) ( isdigit(c) || ( c >= 'a' && c <= 'f' ))
#define isxupper(c) ( isdigit(c) || ( c >= 'A' && c <= 'F' ))

// Misc functions
#define assert( expr ) \
        if( !( expr ) ) \
    Com_Error( ERR_DROP, "%s:%d: Assertion `%s' failed", \
               __FILE__, __LINE__, #expr )
typedef int cmp_t( const void *, const void * );
void qsort( void *a, size_t n, size_t es, cmp_t *cmp );

#define RAND_MAX 0x7fff
void srand( unsigned seed );
int  rand( void );
void *bsearch( const void *key, const void *base, size_t nmemb,
               size_t size, cmp_t *compar );

// String functions
size_t strlen( const char *string );
char   *strcat( char *strDestination, const char *strSource );
char   *strcpy( char *strDestination, const char *strSource );
int    strcmp( const char *string1, const char *string2 );
char   *strchr( const char *string, int c );
char   *strrchr( const char *string, int c );
char   *strstr( const char *string, const char *strCharSet );
int    tolower( int c );
int    toupper( int c );

double atof( const char *string );
double _atof( const char **stringPtr );
double strtod( const char *nptr, char **endptr );
int    atoi( const char *string );
int    _atoi( const char **stringPtr );
long   strtol( const char *nptr, char **endptr, int base );

int    Q_vsnprintf( char *buffer, size_t length, const char *fmt, va_list argptr ) ;
int    Q_snprintf( char *buffer, size_t length, const char *fmt, ... ) ;

int    sscanf( const char *buffer, const char *fmt, ... ) ;

// Memory functions
void   *memmove( void *dest, const void *src, size_t count );
void   *memset( void *dest, int c, size_t count );
void   *memcpy( void *dest, const void *src, size_t count );

// Math functions
#define FLT_EPSILON 1.19209290E-07F

double ceil( double x );
float  ceilf( float x );
double floor( double x );
float  floorf( float x );
double sqrt( double x );
float  sqrtf( float x );
double sin( double x );
double cos( double x );
double atan( double x );
float  atanf( float x );
double atan2( double y, double x );
double tan( double x );
float  tanf( float x );
int    abs( int n );
double fabs( double x );
double asin( double x );
double acos( double x );
float  powf( float x, float y );
double rint( double v );

double exp( double ); // trap call

#endif // BG_LIB_H
